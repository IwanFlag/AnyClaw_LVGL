#include "app.h"
#include "app_log.h"
#include "permissions.h"
#include "workspace.h"
#include "runtime_mgr.h"
#include <windows.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include <shlobj.h>
#include <winhttp.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <vector>
#include <sstream>

namespace fs = std::filesystem;
static volatile LONG g_setup_cancel_requested = 0;

/* ── Helpers ──────────────────────────────────────────────── */

static PermKey classify_exec_command(const char* cmd) {
    PermKey key = PermKey::EXEC_SHELL;
    if (!cmd) return key;
    if (strstr(cmd, "npm install") || strstr(cmd, "apt install") || strstr(cmd, "pip install")) {
        key = PermKey::EXEC_INSTALL;
    } else if (strstr(cmd, " rm ") || strstr(cmd, " del ") || strstr(cmd, " rmdir ")
               || strstr(cmd, "taskkill ")) {
        key = PermKey::EXEC_DELETE;
    }
    return key;
}

static bool is_sandbox_required(PermKey key) {
    return key == PermKey::EXEC_INSTALL || key == PermKey::EXEC_DELETE;
}

static bool check_exec_permission(const char* cmd, char* output, int out_size, PermKey* out_key) {
    if (!cmd) return false;
    PermKey key = classify_exec_command(cmd);
    if (out_key) *out_key = key;

    const char* decision_name = "exec_shell";
    if (key == PermKey::EXEC_INSTALL) decision_name = "exec_install";
    else if (key == PermKey::EXEC_DELETE) decision_name = "exec_delete";

    if (perm_check_exec(key, cmd)) return true;
    if (output && out_size > 0) {
        snprintf(output, out_size, "DENY: %s permission blocked or rejected by user", decision_name);
    }
    return false;
}

static bool exec_cmd(const char* cmd, char* output, int out_size, DWORD timeout_ms = 5000) {
    PermKey exec_key = PermKey::EXEC_SHELL;
    if (!check_exec_permission(cmd, output, out_size, &exec_key)) {
        LOG_W("PERM", "Blocked command: %s", cmd ? cmd : "(null)");
        return false;
    }

    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE hRead = nullptr, hWrite = nullptr;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) return false;
    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

    /* FIX: Redirect stdin to NUL to prevent child processes from blocking on interactive prompts */
    HANDLE hNul = CreateFileA("NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                              nullptr, OPEN_EXISTING, 0, nullptr);

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.hStdOutput = hWrite;
    si.hStdError  = hWrite;
    si.hStdInput  = hNul ? hNul : GetStdHandle(STD_INPUT_HANDLE);
    si.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION pi{};

    // Detection: if cmd starts with "openclaw " or "node ", skip the cmd /C wrapper.
    // CREATE_NO_WINDOW flag is set so no console window appears.
    bool needs_shell_wrapper = true;
    if (strncmp(cmd, "openclaw ", 9) == 0 ||
        strncmp(cmd, "node ", 5) == 0 ||
        strncmp(cmd, "npm ", 4) == 0) {
        needs_shell_wrapper = false;
    }
    std::string full = needs_shell_wrapper ? (std::string("cmd /C \"") + cmd + "\"") : std::string(cmd);
    std::vector<char> buf(full.begin(), full.end());
    buf.push_back('\0');

    std::string sandbox_dir;
    const char* work_dir = nullptr;
    if (is_sandbox_required(exec_key)) {
        sandbox_dir = workspace_get_root() + "\\.sandbox_exec";
        try { fs::create_directories(sandbox_dir); } catch (...) {}
        if (!sandbox_dir.empty()) {
            work_dir = sandbox_dir.c_str();
            perm_audit_log("sandbox_exec", cmd, "enabled");
        }
    }

    if (!CreateProcessA(nullptr, buf.data(), nullptr, nullptr, TRUE,
                        CREATE_NO_WINDOW, nullptr, work_dir, &si, &pi)) {
        CloseHandle(hRead);
        CloseHandle(hWrite);
        if (output && out_size > 0) snprintf(output, out_size, "ERROR: command not found");
        return false;
    }

    HANDLE hJob = nullptr;
    if (is_sandbox_required(exec_key)) {
        hJob = CreateJobObjectA(nullptr, nullptr);
        if (hJob) {
            JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli{};
            jeli.BasicLimitInformation.LimitFlags =
                JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE |
                JOB_OBJECT_LIMIT_ACTIVE_PROCESS |
                JOB_OBJECT_LIMIT_PROCESS_MEMORY;
            jeli.BasicLimitInformation.ActiveProcessLimit = 1;
            jeli.ProcessMemoryLimit = 512ull * 1024ull * 1024ull; /* 512MB */
            SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli));
            if (!AssignProcessToJobObject(hJob, pi.hProcess)) {
                perm_audit_log("sandbox_exec", cmd, "assign_failed");
            }
        }
    }

    CloseHandle(hWrite);

    /* Non-blocking read with timeout: use WaitForSingleObject on pipe */
    std::string result;
    char readBuf[4096];
    DWORD bytesRead = 0;
    DWORD start_tick = GetTickCount();
    while (true) {
        if (InterlockedCompareExchange(&g_setup_cancel_requested, 0, 0) != 0) {
            TerminateProcess(pi.hProcess, 1);
            if (output && out_size > 0) snprintf(output, out_size, "CANCELLED: setup cancelled by user");
            break;
        }
        /* Check if process has exited and pipe is empty */
        DWORD avail = 0;
        if (PeekNamedPipe(hRead, nullptr, 0, nullptr, &avail, nullptr)) {
            if (avail > 0) {
                if (ReadFile(hRead, readBuf, ((DWORD)sizeof(readBuf) < avail ? (DWORD)sizeof(readBuf) : avail), &bytesRead, nullptr) && bytesRead > 0) {
                    result.append(readBuf, bytesRead);
                }
            } else {
                /* No data available, check if process is done */
                if (WaitForSingleObject(pi.hProcess, 100) == WAIT_OBJECT_0) {
                    /* Process exited, drain remaining pipe data */
                    while (PeekNamedPipe(hRead, nullptr, 0, nullptr, &avail, nullptr) && avail > 0) {
                        if (ReadFile(hRead, readBuf, ((DWORD)sizeof(readBuf) < avail ? (DWORD)sizeof(readBuf) : avail), &bytesRead, nullptr) && bytesRead > 0) {
                            result.append(readBuf, bytesRead);
                        } else break;
                    }
                    break;
                }
            }
        } else {
            /* Pipe broken, process likely exited */
            break;
        }
        /* Global timeout check */
        if (GetTickCount() - start_tick > timeout_ms) {
            TerminateProcess(pi.hProcess, 1);
            break;
        }
    }

    WaitForSingleObject(pi.hProcess, 1000);
    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hRead);
    if (hNul) CloseHandle(hNul);
    if (hJob) CloseHandle(hJob);

    /* Trim */
    while (!result.empty() && (result.back() == '\r' || result.back() == '\n' || result.back() == ' '))
        result.pop_back();

    strncpy(output, result.c_str(), out_size - 1);
    output[out_size - 1] = '\0';
    return exitCode == 0;
}

/* FIX: Check if running with admin privileges */
static bool is_running_as_admin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = nullptr;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(nullptr, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin != FALSE;
}

/* FIX: Check if a command exists in PATH */
static bool command_exists(const char* cmd_name) {
    char tmp[512] = {0};
    std::string check = std::string("where ") + cmd_name;
    return exec_cmd(check.c_str(), tmp, sizeof(tmp), 5000);
}

/* FIX: Download file with curl, fallback to PowerShell if curl unavailable */
static bool download_file(const char* url, const char* dest_path,
                          char* output, int out_size, DWORD timeout_ms = 600000,
                          bool resume = false) {
    char cmd[2048];

    /* Try curl first */
    if (command_exists("curl")) {
        const char* resume_opt = "";
        char resume_buf[32] = {0};
        if (resume) {
            try {
                if (fs::exists(dest_path) && fs::file_size(dest_path) > 0) {
                    snprintf(resume_buf, sizeof(resume_buf), "-C - ");
                }
            } catch (...) {}
            resume_opt = resume_buf;
        }
        snprintf(cmd, sizeof(cmd),
                 "curl -L --fail --retry 1 --connect-timeout 15 --speed-time 30 "
                 "--speed-limit 1024 --max-time %lu %s-o \"%s\" \"%s\"",
                 (unsigned long)(timeout_ms / 1000), resume_opt, dest_path, url);
        if (exec_cmd(cmd, output, out_size, timeout_ms + 5000)) return true;
        ui_log("[Download] curl failed, trying PowerShell fallback...");
    }

    /* Fallback: PowerShell Invoke-WebRequest (no resume support) */
    snprintf(cmd, sizeof(cmd),
             "powershell -NoProfile -Command \""
             "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; "
             "try { Invoke-WebRequest -Uri '%s' -OutFile '%s' -UseBasicParsing -TimeoutSec %lu; "
             "Write-Output 'OK' } catch { Write-Error $_.Exception.Message; exit 1 }\"",
             url, dest_path, (unsigned long)(timeout_ms / 1000));
    if (exec_cmd(cmd, output, out_size, timeout_ms + 10000)) return true;

    snprintf(output, out_size, "Download failed: both curl and PowerShell failed");
    return false;
}

static bool is_process_running(const char* process_name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);

    std::wstring wname(process_name, process_name + strlen(process_name));

    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, wname.c_str()) == 0) {
                CloseHandle(snap);
                return true;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return false;
}

/* ── OpenClaw single-instance management ───────────────────── */

/* Count running node.exe processes */
int app_count_node_processes() {
    int count = 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);

    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, L"node.exe") == 0) {
                count++;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return count;
}

/* Kill all node.exe processes, wait for port release.
 * Returns true if all killed and port is free. */
bool app_kill_duplicate_openclaw() {
    int count = app_count_node_processes();
    if (count == 0) {
        LOG_I("OC", "No node.exe running, nothing to kill");
        return true;
    }

    LOG_W("OC", "Found %d node.exe process(es), killing all for clean restart", count);

    /* Use taskkill to terminate all node.exe (cleaner than individual TerminateProcess) */
    char output[512] = {0};
    exec_cmd("taskkill /F /IM node.exe", output, sizeof(output), 10000);
    LOG_I("OC", "taskkill output: %.200s", output);

    /* Wait for processes to actually exit */
    int retries = 0;
    while (retries < 20) {
        int remaining = app_count_node_processes();
        if (remaining == 0) {
            LOG_I("OC", "All node.exe processes terminated (%d retries)", retries);
            break;
        }
        LOG_D("OC", "Waiting for %d node.exe to exit... (retry %d)", remaining, retries);
        Sleep(500);
        retries++;
    }

    /* Wait for port to release (Gateway might take a moment to free 18789) */
    const int gw_port = 18789; /* default Gateway port */
    char url[128];
    snprintf(url, sizeof(url), "http://127.0.0.1:%d/health", gw_port);

    retries = 0;
    while (retries < 10) {
        char resp[64] = {0};
        int code = http_get(url, resp, sizeof(resp), 2);
        if (code == 0 || code < 0) {
            /* Connection refused = port is free */
            LOG_I("OC", "Port %d is free (%d retries)", gw_port, retries);
            return true;
        }
        LOG_D("OC", "Port %d still responding (HTTP %d), waiting... (retry %d)", gw_port, code, retries);
        Sleep(500);
        retries++;
    }

    LOG_W("OC", "Port %d still occupied after %d retries, proceeding anyway", gw_port, retries);
    return false; /* Port still occupied */
}

/* FIX 1: Use robust JSON extraction from json_util.cpp */
static int extract_int(const std::string& content, const std::string& key) {
    return json_extract_int(content.c_str(), key.c_str(), -1);
}

/* Extract port from gateway.bind URL like "http://localhost:18789" or "http://0.0.0.0:18789" */
static int extract_port_from_bind_url(const std::string& content) {
    char bind_url[256] = {0};
    if (!json_extract_string(content.c_str(), "bind", bind_url, sizeof(bind_url)))
        return 0;
    /* Find last ':' which precedes the port number */
    const char* colon = strrchr(bind_url, ':');
    if (!colon || colon == bind_url) return 0;
    colon++;
    int port = 0;
    while (*colon >= '0' && *colon <= '9') {
        port = port * 10 + (*colon - '0');
        colon++;
    }
    return (port > 0 && port < 65536) ? port : 0;
}

/* Extract port from URL-like strings in raw JSON (e.g. "http://localhost:18789") */
static int extract_port_from_url_in_json(const std::string& content) {
    /* Look for "localhost:PORT" or "127.0.0.1:PORT" pattern in raw JSON text */
    static const char* patterns[] = { "localhost:", "127.0.0.1:", nullptr };
    for (int i = 0; patterns[i]; i++) {
        size_t pos = content.find(patterns[i]);
        if (pos == std::string::npos) continue;
        pos += strlen(patterns[i]);
        if (pos >= content.size() || content[pos] < '0' || content[pos] > '9') continue;
        int port = 0;
        while (pos < content.size() && content[pos] >= '0' && content[pos] <= '9') {
            port = port * 10 + (content[pos] - '0');
            pos++;
        }
        if (port > 1024 && port < 65536) return port;
    }
    return 0;
}

/* Read gateway port from user config ~/.openclaw/openclaw.json */
static int read_user_gateway_port() {
    const char* userprofile = std::getenv("USERPROFILE");
    if (!userprofile) return 0;
    std::string user_cfg = std::string(userprofile) + "\\.openclaw\\openclaw.json";
    std::ifstream uf(user_cfg);
    if (!uf.is_open()) return 0;
    std::string content((std::istreambuf_iterator<char>(uf)),
                         std::istreambuf_iterator<char>());
    /* Try gateway.controlUi.allowedOrigins URL pattern (e.g. "http://localhost:18789") */
    int port = extract_port_from_url_in_json(content);
    if (port > 0) return port;
    /* Try gateway.bind URL format (e.g. "bind": "http://0.0.0.0:18789") */
    port = extract_port_from_bind_url(content);
    if (port > 0) return port;
    /* Fallback: top-level "port" key */
    port = extract_int(content, "port");
    return (port > 0) ? port : 0;
}

/* FIX 1: Use robust JSON extraction from json_util.cpp */
static void extract_string(const std::string& content, const std::string& key, char* out, int out_size) {
    json_extract_string(content.c_str(), key.c_str(), out, out_size);
}

/* ── Detect ───────────────────────────────────────────────── */

OpenClawInfo app_detect_openclaw() {
    OpenClawInfo info;
    char output[1024] = {0};

    /* Strategy 1: PATH lookup */
    if (exec_cmd("where openclaw", output, sizeof(output)) && output[0]) {
        /* Take first line */
        char* nl = strchr(output, '\n');
        if (nl) *nl = '\0';
        while (strlen(output) > 0 && (output[strlen(output)-1] == '\r' || output[strlen(output)-1] == '\n'))
            output[strlen(output)-1] = '\0';

        if (fs::exists(output)) {
            strncpy(info.executable, output, sizeof(info.executable)-1);

            std::string dir = fs::path(output).parent_path().string();
            strncpy(info.install_dir, dir.c_str(), sizeof(info.install_dir)-1);

            std::string cfg = dir + "\\openclaw.json";
            strncpy(info.config_file, cfg.c_str(), sizeof(info.config_file)-1);

            /* Read config for port */
            std::ifstream f(cfg);
            if (f.is_open()) {
                std::string content((std::istreambuf_iterator<char>(f)),
                                     std::istreambuf_iterator<char>());
                int port = extract_int(content, "port");
                if (port > 0) info.gateway_port = port;
                extract_string(content, "version", info.version, sizeof(info.version));
            }

            /* Override with user config gateway port (e.g. gateway.bind = "http://localhost:18789") */
            {
                int user_port = read_user_gateway_port();
                if (user_port > 0) info.gateway_port = user_port;
            }

            /* Get version from CLI */
            if (!info.version[0]) {
                if (exec_cmd("openclaw --version", output, sizeof(output))) {
                    strncpy(info.version, output, sizeof(info.version)-1);
                }
            }
            return info;
        }
    }

    /* Strategy 2: npm global */
    const char* appdata = std::getenv("APPDATA");
    if (appdata) {
        std::string npm_path = std::string(appdata) + "\\npm\\node_modules\\openclaw\\openclaw.cmd";
        if (fs::exists(npm_path)) {
            strncpy(info.executable, npm_path.c_str(), sizeof(info.executable)-1);
            std::string dir = fs::path(npm_path).parent_path().string();
            strncpy(info.install_dir, dir.c_str(), sizeof(info.install_dir)-1);

            std::string cfg = dir + "\\openclaw.json";
            strncpy(info.config_file, cfg.c_str(), sizeof(info.config_file)-1);

            /* Read user config gateway port */
            int user_port = read_user_gateway_port();
            if (user_port > 0) info.gateway_port = user_port;

            return info;
        }
    }

    return info;
}

/* ── Check Status ─────────────────────────────────────────── */

ClawStatus app_check_status(const OpenClawInfo& info) {
    bool running = is_process_running("node.exe");

    if (info.executable[0] == '\0' && !running)
        return ClawStatus::NotInstalled;

    char url[256];
    snprintf(url, sizeof(url), "http://127.0.0.1:%d/health", info.gateway_port);
    char response[4096] = {0};
    int status = http_get(url, response, sizeof(response), 3);

    if (running && status == 200) return ClawStatus::Ready;
    if (running && status != 200) return ClawStatus::Error;
    if (!running && info.executable[0]) return ClawStatus::Detected;

    return ClawStatus::NotInstalled;
}

/* ── Start/Stop ───────────────────────────────────────────── */

bool app_start_gateway() {
    char output[512];
    return exec_cmd("openclaw gateway start", output, sizeof(output), 10000);
}

bool app_stop_gateway() {
    char output[512];
    return exec_cmd("openclaw gateway stop", output, sizeof(output), 10000);
}

/* Ensure chat completions HTTP endpoint is enabled (auto-fix 404)
 * Safely: backup config → modify → test → rollback if fail */
bool app_enable_chat_endpoint() {
    char output[512];

    /* Find openclaw.json path */
    const char* userprofile = std::getenv("USERPROFILE");
    std::string cfg_path;
    if (userprofile) {
        cfg_path = std::string(userprofile) + "\\.openclaw\\openclaw.json";
    } else {
        cfg_path = "Z:\\root\\.openclaw\\openclaw.json";
    }
    std::string bak_path = cfg_path + ".bak";

    /* Step 1: Backup */
    LOG_I("Config", "Backing up %s", cfg_path.c_str());
    {
        std::ifstream src(cfg_path, std::ios::binary);
        if (!src.is_open()) {
            LOG_E("Config", "Cannot open %s for backup", cfg_path.c_str());
            return false;
        }
        std::ofstream dst(bak_path, std::ios::binary);
        dst << src.rdbuf();
        src.close();
        dst.close();
        if (!dst.good()) {
            LOG_E("Config", "Backup write failed");
            return false;
        }
    }

    /* Step 2: Enable endpoint */
    LOG_I("Config", "Enabling chatCompletions endpoint");
    bool ok = exec_cmd(
        "openclaw config set gateway.http.endpoints.chatCompletions.enabled true",
        output, sizeof(output), 8000);
    if (!ok) {
        LOG_E("Config", "config set failed: %s", output);
        std::ifstream src(bak_path, std::ios::binary);
        std::ofstream dst(cfg_path, std::ios::binary);
        dst << src.rdbuf();
        remove(bak_path.c_str());
        return false;
    }

    /* Step 3: Restart gateway — must wait for port release */
    LOG_I("Config", "Stopping gateway, waiting for port release...");
    app_stop_gateway();

    /* Read gateway port */
    int gw_port = 18789;
    {
        char port_out[64] = {0};
        if (exec_cmd("openclaw config get gateway.port", port_out, sizeof(port_out), 3000)) {
            int p = atoi(port_out);
            if (p > 0 && p < 65536) gw_port = p;
        }
    }

    /* Wait for port to be released (max 10s) */
    char test_url[256];
    snprintf(test_url, sizeof(test_url), "http://127.0.0.1:%d/health", gw_port);
    for (int i = 0; i < 20; i++) {
        Sleep(500);
        char probe[64] = {0};
        int s = http_get(test_url, probe, sizeof(probe), 2);
        if (s <= 0) {
            LOG_I("Config", "Port %d released after %dms", gw_port, (i + 1) * 500);
            break;
        }
    }

    /* Start gateway */
    LOG_I("Config", "Starting gateway...");
    app_start_gateway();

    /* Wait for gateway to be ready (max 15s) */
    bool gw_ready = false;
    for (int i = 0; i < 30; i++) {
        Sleep(500);
        char probe[64] = {0};
        int s = http_get(test_url, probe, sizeof(probe), 2);
        if (s == 200) {
            LOG_I("Config", "Gateway ready after %dms", (i + 1) * 500);
            gw_ready = true;
            break;
        }
    }
    if (!gw_ready) {
        LOG_E("Config", "Gateway did not become ready, rolling back");
        std::ifstream src(bak_path, std::ios::binary);
        std::ofstream dst(cfg_path, std::ios::binary);
        dst << src.rdbuf();
        remove(bak_path.c_str());
        app_stop_gateway();
        Sleep(2000);
        app_start_gateway();
        return false;
    }

    /* Step 4: Test endpoint */
    LOG_I("Config", "Testing /v1/chat/completions endpoint");
    char test_resp[512] = {0};
    char chat_url[256];
    snprintf(chat_url, sizeof(chat_url), "http://127.0.0.1:%d/v1/chat/completions", gw_port);
    int test_status = http_post(chat_url,
        "{\"model\":\"openclaw:main\",\"messages\":[{\"role\":\"user\",\"content\":\"ping\"}],\"stream\":false}",
        nullptr, test_resp, sizeof(test_resp), 30);

    if (test_status == 404 || test_status <= 0) {
        /* Step 5: Test failed — rollback */
        LOG_W("Config", "Endpoint test failed (HTTP %d), rolling back config", test_status);
        std::ifstream src(bak_path, std::ios::binary);
        std::ofstream dst(cfg_path, std::ios::binary);
        dst << src.rdbuf();
        remove(bak_path.c_str());
        app_stop_gateway();
        Sleep(2000);
        app_start_gateway();
        return false;
    }

    /* Test passed — clean up backup */
    LOG_I("Config", "Endpoint test passed (HTTP %d), keeping new config", test_status);
    remove(bak_path.c_str());
    return true;
}

/* Update OpenClaw config with model and API key, then restart gateway */
bool app_update_model_config(const char* api_key, const char* model_name) {
    char output[512];
    char cmd[2048];
    bool all_ok = true;

    /* ── Step 0: Find openclaw.json path ── */
    const char* userprofile = std::getenv("USERPROFILE");
    std::string cfg_path;
    if (userprofile) {
        cfg_path = std::string(userprofile) + "\\.openclaw\\openclaw.json";
    } else {
        cfg_path = "Z:\\root\\.openclaw\\openclaw.json";
    }
    std::string bak_path = cfg_path + ".bak";

    /* ── Step 1: Validate JSON format ── */
    {
        std::ifstream f(cfg_path);
        if (!f.is_open()) {
            LOG_E("Config", "Cannot open %s", cfg_path.c_str());
            return false;
        }
        std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        f.close();
        /* Basic validation: must start with { and contain "gateway" or "models" */
        bool has_brace = content.find('{') != std::string::npos;
        bool has_gateway = content.find("\"gateway\"") != std::string::npos;
        bool has_models = content.find("\"models\"") != std::string::npos;
        if (!has_brace || (!has_gateway && !has_models)) {
            LOG_E("Config", "openclaw.json format invalid (no gateway/models keys)");
            return false;
        }
        LOG_I("Config", "openclaw.json validated OK (%zu bytes)", content.size());
    }

    /* ── Step 2: Backup ── */
    {
        std::ifstream src(cfg_path, std::ios::binary);
        std::ofstream dst(bak_path, std::ios::binary);
        if (!src.is_open() || !dst.is_open()) {
            LOG_E("Config", "Cannot create backup %s", bak_path.c_str());
            return false;
        }
        dst << src.rdbuf();
        src.close();
        dst.close();
        LOG_I("Config", "Backed up to %s", bak_path.c_str());
    }

    /* 1. Set model (user provides full ID like "openrouter/google/gemma-3-4b-it:free" 
     *    or short like "google/gemma-3-4b-it:free" → auto-prefix provider) */
    if (model_name && model_name[0]) {
        /* Use local var to avoid mutating caller's pointer */
        const char* mn = model_name;

        /* Detect provider from model name */
        const char* provider = "openrouter";  /* default */
        const char* base_url = "https://openrouter.ai/api/v1";
        
        if (strncmp(mn, "xiaomi/", 7) == 0) {
            provider = "xiaomi";
            base_url = "https://api.xiaomimimo.com/v1";
        } else if (strncmp(mn, "minimax-cn/", 11) == 0) {
            provider = "minimax-cn";
            base_url = "https://api.minimaxi.com/anthropic";
            mn += 11;  /* strip prefix */
        } else if (strncmp(mn, "minimax/", 8) == 0) {
            provider = "minimax";
            base_url = "https://api.minimaxi.com/anthropic";
            mn += 8;  /* strip prefix */
        } else if (strncmp(mn, "openrouter/", 11) == 0) {
            provider = "openrouter";
            mn += 11;  /* strip prefix */
        }

        char full_model[256];
        snprintf(full_model, sizeof(full_model), "%s/%s", provider, mn);

        snprintf(cmd, sizeof(cmd),
            "openclaw config set agents.defaults.model.primary \"%s\"", full_model);
        if (!exec_cmd(cmd, output, sizeof(output), 8000)) {
            LOG_E("Config", "Failed to set model.primary");
            all_ok = false;
        } else {
            LOG_I("Config", "Model set to: %s", full_model);
        }

        /* Ensure provider exists with baseUrl */
        snprintf(cmd, sizeof(cmd),
            "openclaw config set models.providers.%s.baseUrl %s", provider, base_url);
        exec_cmd(cmd, output, sizeof(output), 8000);

        snprintf(cmd, sizeof(cmd),
            "openclaw config set models.providers.%s.api openai-completions", provider);
        exec_cmd(cmd, output, sizeof(output), 8000);
    }

    /* 2. Set API key for the provider (detect from original model_name) */
    if (api_key && api_key[0] && model_name && model_name[0]) {
        const char* provider = "openrouter";
        if (strncmp(model_name, "xiaomi/", 7) == 0) provider = "xiaomi";
        else if (strncmp(model_name, "minimax-cn/", 11) == 0) provider = "minimax-cn";
        else if (strncmp(model_name, "minimax/", 8) == 0) provider = "minimax";

        snprintf(cmd, sizeof(cmd),
            "openclaw config set models.providers.%s.apiKey \"%s\"", provider, api_key);
        if (!exec_cmd(cmd, output, sizeof(output), 8000)) {
            LOG_E("Config", "Failed to set apiKey for %s", provider);
            all_ok = false;
        } else {
            LOG_I("Config", "API key set for provider: %s", provider);
        }
    }

    /* 3. Apply — Gateway watches openclaw.json and hot-reloads model changes.
     *    Model/agent config does NOT require a restart (docs: "Agent & models → No").
     *    Wait briefly for file watcher debounce (~300ms default) to pick up changes. */
    if (all_ok) {
        LOG_I("Config", "Config written, Gateway will hot-apply (no restart needed)");
        Sleep(500); /* allow file watcher debounce */
    }

    /* ── Rollback on failure ── */
    if (!all_ok) {
        LOG_W("Config", "Config update failed, rolling back from %s", bak_path.c_str());
        std::ifstream src(bak_path, std::ios::binary);
        std::ofstream dst(cfg_path, std::ios::binary);
        if (src.is_open() && dst.is_open()) {
            dst << src.rdbuf();
            src.close();
            dst.close();
            LOG_I("Config", "Rollback complete, Gateway will hot-apply restored config");
            Sleep(500);
        } else {
            LOG_E("Config", "Rollback FAILED — cannot restore %s", bak_path.c_str());
        }
    } else {
        /* Success: delete backup */
        remove(bak_path.c_str());
    }

    return all_ok;
}

/* Read current model from Gateway config */
bool app_get_current_model(char* model_out, int out_size) {
    if (!model_out || out_size <= 0) return false;
    model_out[0] = '\0';
    char output[512];
    if (exec_cmd("openclaw config get agents.defaults.model.primary", output, sizeof(output), 1200)) {
        char* start = output;
        while (*start == '"' || *start == ' ' || *start == '\n' || *start == '\r') start++;
        char* end = start + strlen(start) - 1;
        while (end > start && (*end == '"' || *end == ' ' || *end == '\n' || *end == '\r')) { *end = '\0'; end--; }
        if (start[0]) {
            strncpy(model_out, start, out_size - 1);
            model_out[out_size - 1] = '\0';
            return true;
        }
    }
    return false;
}

/* Read all configured model IDs from openclaw.json (all providers).
 * Calls the callback for each model found. Returns total count. */
int app_get_all_models(void (*cb)(const char* model_id, void* ctx), void* ctx) {
    char output[8192] = {0};
    if (!exec_cmd("openclaw config get models", output, sizeof(output), 1200)) {
        LOG_W("OpenClaw", "Failed to read models config");
        return 0;
    }

    /* Parse: find all "id": "..." entries in the JSON output */
    int count = 0;
    const char* p = output;
    while ((p = strstr(p, "\"id\"")) != nullptr) {
        p += 5; /* skip "id" */
        /* Skip whitespace and colon */
        while (*p == ' ' || *p == ':' || *p == '\t') p++;
        if (*p == '"') p++; /* skip opening quote */
        const char* start = p;
        while (*p && *p != '"') p++;
        int len = (int)(p - start);
        if (len > 0 && len < 256 && *p == '"') {
            char model_id[256];
            memcpy(model_id, start, len);
            model_id[len] = '\0';
            if (cb) cb(model_id, ctx);
            count++;
        }
        if (*p) p++;
    }

    LOG_I("OpenClaw", "Found %d models in openclaw config", count);
    return count;
}

/* Read API key for a specific provider from openclaw config */
bool app_get_provider_api_key(const char* provider, char* key_out, int out_size) {
    if (!provider || !provider[0] || !key_out || out_size <= 0) return false;
    key_out[0] = '\0';

    /* Try direct file read first (bypasses OpenClaw redaction) */
    const char* userprofile = std::getenv("USERPROFILE");
    std::string cfg_path;
    if (userprofile) {
        cfg_path = std::string(userprofile) + "\\.openclaw\\openclaw.json";
    } else {
        cfg_path = "Z:\\root\\.openclaw\\openclaw.json";
    }

    std::ifstream f(cfg_path);
    if (f.is_open()) {
        std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        f.close();

        /* Find "providers" → "provider_name" → "apiKey" */
        std::string prov_key = std::string("\"") + provider + "\"";
        size_t prov_pos = content.find(prov_key);
        if (prov_pos != std::string::npos) {
            /* Find "apiKey" after provider */
            size_t key_pos = content.find("\"apiKey\"", prov_pos);
            if (key_pos != std::string::npos) {
                size_t colon = content.find(':', key_pos + 8);
                if (colon != std::string::npos) {
                    size_t val_start = content.find('"', colon + 1);
                    if (val_start != std::string::npos) {
                        val_start++; /* skip opening quote */
                        size_t val_end = content.find('"', val_start);
                        if (val_end != std::string::npos && val_end > val_start) {
                            int len = (int)(val_end - val_start);
                            if (len > 0 && len < out_size) {
                                memcpy(key_out, content.c_str() + val_start, len);
                                key_out[len] = '\0';
                                /* Skip redacted placeholder */
                                if (strcmp(key_out, "__OPENCLAW_REDACTED__") == 0) {
                                    key_out[0] = '\0'; /* treat as not available */
                                    LOG_D("OpenClaw", "API key for %s is redacted by OpenClaw", provider);
                                } else {
                                    LOG_D("OpenClaw", "Read API key for %s: %d chars", provider, len);
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    /* Fallback: CLI (will be redacted, but check if key exists at all) */
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "openclaw config get models.providers.%s.apiKey", provider);
    char output[512];
    if (exec_cmd(cmd, output, sizeof(output))) {
        char* start = output;
        while (*start == '"' || *start == ' ' || *start == '\n' || *start == '\r') start++;
        char* end = start + strlen(start) - 1;
        while (end > start && (*end == '"' || *end == ' ' || *end == '\n' || *end == '\r')) { *end = '\0'; end--; }
        if (start[0] && strcmp(start, "__OPENCLAW_REDACTED__") != 0) {
            strncpy(key_out, start, out_size - 1);
            key_out[out_size - 1] = '\0';
            return true;
        }
    }
    return false;
}

/* ── Check if Node.js is installed ─────────────────────────── */
static char g_manual_node_path[MAX_PATH] = {0};

void app_set_node_path(const char* path) {
    if (path && path[0]) {
        strncpy(g_manual_node_path, path, sizeof(g_manual_node_path) - 1);
        g_manual_node_path[sizeof(g_manual_node_path) - 1] = '\0';
        LOG_I("OpenClaw", "Manual node path set: %s", path);
    }
}

bool app_check_nodejs(char* version_out, int out_size) {
    if (version_out && out_size > 0) version_out[0] = '\0';
    char output[256] = {0};

    /* Try manual path first if set */
    if (g_manual_node_path[0]) {
        char cmd[512] = {0};
        snprintf(cmd, sizeof(cmd), "\"%s\\node.exe\" --version", g_manual_node_path);
        if (exec_cmd(cmd, output, sizeof(output)) && output[0]) {
            char* p = output + strlen(output) - 1;
            while (p > output && (*p == '\r' || *p == '\n' || *p == ' ')) { *p = '\0'; p--; }
            if (version_out && out_size > 0) {
                strncpy(version_out, output, out_size - 1);
                version_out[out_size - 1] = '\0';
            }
            LOG_I("OpenClaw", "Node.js found via manual path: %s", output);
            return true;
        }
    }

    /* Fallback: check in PATH */
    if (exec_cmd("node --version", output, sizeof(output)) && output[0]) {
        /* Trim whitespace */
        char* p = output + strlen(output) - 1;
        while (p > output && (*p == '\r' || *p == '\n' || *p == ' ')) { *p = '\0'; p--; }
        if (version_out && out_size > 0) {
            strncpy(version_out, output, out_size - 1);
            version_out[out_size - 1] = '\0';
        }
        LOG_I("OpenClaw", "Node.js found: %s", output);
        return true;
    }
    LOG_W("OpenClaw", "Node.js NOT found");
    return false;
}

/* ── Platform-specific install URLs ────────────────────────── */
/* Primary and backup download URLs for Node.js */
struct DownloadSource {
    const char* name;
    const char* url;
};

static const DownloadSource nodejs_sources[] = {
#ifdef _WIN32
    {"GitHub (AnyClaw)", "https://github.com/IwanFlag/AnyClaw_Tools/releases/download/v1.0.0/node-v22.22.1-x64.msi"},
    {"nodejs.org",       "https://nodejs.org/dist/v22.22.1/node-v22.22.1-x64.msi"},
    {"npmmirror (CN)",   "https://registry.npmmirror.com/-/binary/node/v22.22.1/node-v22.22.1-x64.msi"},
    {"Tencent mirror",   "https://mirrors.cloud.tencent.com/nodejs-release/v22.22.1/node-v22.22.1-x64.msi"},
    {nullptr, nullptr}
#else
    {"GitHub (AnyClaw)", "https://github.com/IwanFlag/AnyClaw_Tools/releases/download/v1.0.0/node-v22.22.1-linux-x64.tar.xz"},
    {"nodejs.org",       "https://nodejs.org/dist/v22.22.1/node-v22.22.1-linux-x64.tar.xz"},
    {"npmmirror (CN)",   "https://registry.npmmirror.com/-/binary/node/v22.22.1/node-v22.22.1-linux-x64.tar.xz"},
    {"Tencent mirror",   "https://mirrors.cloud.tencent.com/nodejs-release/v22.22.1/node-v22.22.1-linux-x64.tar.xz"},
    {nullptr, nullptr}
#endif
};

const char* app_get_nodejs_download_url() {
    return nodejs_sources[0].url; /* GitHub (AnyClaw) */
}

/* Minimum Node.js major version required (OpenClaw needs 22.14+) */
#define NODE_MIN_MAJOR 22

/* Parse major version from "v22.22.1" → 22 */
static int parse_node_major(const char* ver) {
    if (!ver || ver[0] != 'v') return 0;
    return atoi(ver + 1);
}

/* Check if Node.js version meets minimum requirement */
bool app_check_nodejs_version_ok(const char* version) {
    int major = parse_node_major(version);
    return major >= NODE_MIN_MAJOR;
}

/* ── Find bundled openclaw tarball ─────────────────────────── */
/* Searches relative to exe dir for bundled/openclaw-*.tgz, returns absolute path */
static bool find_bundled_tarball(char* path_out, int out_size) {
    if (!path_out || out_size <= 0) return false;
    path_out[0] = '\0';

    /* Candidate paths relative to exe */
    const char* search_paths[] = {
        "bundled\\openclaw.tgz",           /* same dir as exe */
        "..\\bundled\\openclaw.tgz",       /* one level up */
        "bundled/openclaw.tgz",            /* forward slash variant */
        "../bundled/openclaw.tgz",
        nullptr
    };

    for (int i = 0; search_paths[i]; i++) {
        FILE* f = fopen(search_paths[i], "rb");
        if (f) {
            fclose(f);
            /* FIX: Convert to absolute path so child processes (sandbox CWD) can find it */
            char abs_path[MAX_PATH] = {0};
            if (_fullpath(abs_path, search_paths[i], MAX_PATH)) {
                strncpy(path_out, abs_path, out_size - 1);
            } else {
                strncpy(path_out, search_paths[i], out_size - 1);
            }
            path_out[out_size - 1] = '\0';
            LOG_I("OpenClaw", "Found bundled tarball: %s", path_out);
            return true;
        }
    }

    LOG_D("OpenClaw", "No bundled tarball found");
    return false;
}

struct GemmaSource {
    const char* name;
    const char* mirror_url;
    const char* mirror2_url;
    const char* official_url;
    const char* file_name;
};

static const GemmaSource kGemmaSources[] = {
    {
        "Gemma 4 2B",
        "https://hf-mirror.com/bartowski/google_gemma-2-2b-it-GGUF/resolve/main/google_gemma-2-2b-it-Q4_K_M.gguf",
        "https://www.modelscope.cn/models/bartowski/google_gemma-2-2b-it-GGUF/resolve/master/google_gemma-2-2b-it-Q4_K_M.gguf",
        "https://huggingface.co/bartowski/google_gemma-2-2b-it-GGUF/resolve/main/google_gemma-2-2b-it-Q4_K_M.gguf",
        "gemma-2-2b-it-q4_k_m.gguf"
    },
    {
        "Gemma 4 9B",
        "https://hf-mirror.com/bartowski/google_gemma-2-9b-it-GGUF/resolve/main/google_gemma-2-9b-it-Q4_K_M.gguf",
        "https://www.modelscope.cn/models/bartowski/google_gemma-2-9b-it-GGUF/resolve/master/google_gemma-2-9b-it-Q4_K_M.gguf",
        "https://huggingface.co/bartowski/google_gemma-2-9b-it-GGUF/resolve/main/google_gemma-2-9b-it-Q4_K_M.gguf",
        "gemma-2-9b-it-q4_k_m.gguf"
    },
    {
        "Gemma 4 27B",
        "https://hf-mirror.com/bartowski/google_gemma-2-27b-it-GGUF/resolve/main/google_gemma-2-27b-it-Q4_K_M.gguf",
        "https://www.modelscope.cn/models/bartowski/google_gemma-2-27b-it-GGUF/resolve/master/google_gemma-2-27b-it-Q4_K_M.gguf",
        "https://huggingface.co/bartowski/google_gemma-2-27b-it-GGUF/resolve/main/google_gemma-2-27b-it-Q4_K_M.gguf",
        "gemma-2-27b-it-q4_k_m.gguf"
    }
};

static bool ensure_parent_dir(const std::string& path) {
    try {
        fs::create_directories(fs::path(path).parent_path());
        return true;
    } catch (...) {
        return false;
    }
}

static bool file_size_ok(const std::string& path, uint64_t min_bytes) {
    try {
        if (!fs::exists(path) || fs::is_directory(path)) return false;
        return fs::file_size(path) >= min_bytes;
    } catch (...) {
        return false;
    }
}

static bool install_nodejs_if_missing(char* output, int out_size) {
    if (app_check_nodejs(nullptr, 0)) return true;
#ifdef _WIN32
    const char* temp = std::getenv("TEMP");
    std::string installer = std::string(temp ? temp : ".") + "\\anyclaw_nodejs_installer.msi";
    char cmd[2048];
    char node_ver[64] = {0};
    bool admin = is_running_as_admin();
    ui_log("[Setup] Running as admin: %s", admin ? "yes" : "no");
    ui_progress_begin("Node.js Setup", "Node.js missing, starting download", 5);
    for (int i = 0; nodejs_sources[i].name; i++) {
        if (app_is_setup_cancelled()) {
            snprintf(output, out_size, "CANCELLED: setup cancelled by user");
            ui_progress_finish("Node.js Setup", false, output);
            return false;
        }
        ui_progress_update("Node.js Setup", nodejs_sources[i].name, 10 + i * 20);
        /* FIX: Use download_file() which handles curl fallback to PowerShell */
        if (!download_file(nodejs_sources[i].url, installer.c_str(), output, out_size, 25 * 60 * 1000)
            || !file_size_ok(installer, 5ull * 1024ull * 1024ull)) {
            ui_log("[Setup] Node source %d failed/stalled, switching...", i + 1);
            continue;
        }
        ui_progress_update("Node.js Setup", "Installing Node.js", 70);
        /* FIX: msiexec UAC handling — non-admin uses per-user install */
        if (admin) {
            snprintf(cmd, sizeof(cmd), "msiexec /i \"%s\" /qn /norestart", installer.c_str());
        } else {
            /* Per-user install: set ALLUSERS="" and INSTALLDIR to user-local path */
            const char* localappdata = std::getenv("LOCALAPPDATA");
            if (localappdata) {
                std::string user_dir = std::string(localappdata) + "\\Programs\\NodeJS";
                snprintf(cmd, sizeof(cmd),
                         "msiexec /i \"%s\" /qn /norestart ALLUSERS=\"\" INSTALLDIR=\"%s\"",
                         installer.c_str(), user_dir.c_str());
            } else {
                /* Fallback: try ALLUSERS="" which triggers per-user without custom dir */
                snprintf(cmd, sizeof(cmd), "msiexec /i \"%s\" /qn /norestart ALLUSERS=\"\"", installer.c_str());
            }
        }
        if (!exec_cmd(cmd, output, out_size, 20 * 60 * 1000)) {
            ui_log("[Setup] Node installer failed on source %d: %s", i + 1,
                   output[0] ? output : "(timeout or error)");
            continue;
        }
        ui_progress_update("Node.js Setup", "Verifying node --version", 90);
        /* FIX: Child processes inherit parent env, can't see PATH updated by msiexec.
         * Check standard Node.js install dirs directly, bypassing stale PATH. */
        {
            std::string prog_files = std::getenv("PROGRAMFILES") ? std::getenv("PROGRAMFILES") : "";
            std::string local_app  = std::getenv("LOCALAPPDATA") ? std::getenv("LOCALAPPDATA") : "";
            std::string candidates[3];
            int nc = 0;
            if (!prog_files.empty()) candidates[nc++] = prog_files + "\\nodejs\\node.exe";
            if (!local_app.empty())  candidates[nc++] = local_app  + "\\Programs\\NodeJS\\node.exe";
            candidates[nc++] = "C:\\Program Files\\nodejs\\node.exe";
            for (int di = 0; di < nc; di++) {
                if (GetFileAttributesA(candidates[di].c_str()) != INVALID_FILE_ATTRIBUTES) {
                    char ver_buf[64] = {0};
                    std::string ver_cmd = std::string("\"") + candidates[di] + "\" --version";
                    if (exec_cmd(ver_cmd.c_str(), ver_buf, sizeof(ver_buf), 5000) && ver_buf[0]) {
                        /* Found — add dir to PATH for subsequent commands */
                        std::string node_dir = candidates[di];
                        size_t pos = node_dir.rfind('\\');
                        if (pos != std::string::npos) node_dir = node_dir.substr(0, pos);
                        const char* cur = std::getenv("PATH");
                        if (cur && strstr(cur, node_dir.c_str()) == nullptr) {
                            _putenv_s("PATH", (node_dir + ";" + cur).c_str());
                        }
                        ui_log("[Setup] Node.js found at %s, ver=%s", candidates[di].c_str(), ver_buf);
                    }
                }
            }
        }
        if (app_check_nodejs(node_ver, sizeof(node_ver))) {
            ui_progress_finish("Node.js Setup", true, "Node.js installed");
            return true;
        }
    }
    snprintf(output, out_size, "Node.js download/install failed from all sources");
    ui_progress_finish("Node.js Setup", false, output);
    return false;
#else
    snprintf(output, out_size, "Node.js auto-install is implemented for Windows only");
    ui_progress_finish("Node.js Setup", false, output);
    return false;
#endif
}

static bool download_file_with_fallback(const char* primary_url,
                                        const char* secondary_url,
                                        const char* fallback_url,
                                        const char* target_path,
                                        char* output, int out_size) {
    if (!primary_url || !secondary_url || !fallback_url || !target_path) return false;
    if (!ensure_parent_dir(target_path)) {
        snprintf(output, out_size, "Cannot create model directory");
        return false;
    }

    const char* urls[3] = {primary_url, secondary_url, fallback_url};
    for (int i = 0; i < 3; i++) {
        ui_log("[Download] Source %d/3: %s", i + 1, urls[i]);
        /* FIX: Use download_file() with resume support, handles curl/PowerShell fallback */
        if (download_file(urls[i], target_path, output, out_size, 4 * 60 * 60 * 1000, true)) {
            if (file_size_ok(target_path, 10ull * 1024ull * 1024ull)) {
                ui_log("[Download] Source %d succeeded", i + 1);
                return true;
            }
            snprintf(output, out_size, "Downloaded file too small, possible incomplete: %s", target_path);
        }
        ui_log("[Download] Source %d failed or stalled, switching...", i + 1);
    }
    return false;
}

/* ── Install OpenClaw ──────────────────────────────────────── */
/* mode: "network" = npm registry, "local" = bundled tarball, "auto" = try network then fallback */
bool app_install_openclaw_ex(char* output, int out_size, const char* mode) {
    if (!output || out_size <= 0) return false;
    output[0] = '\0';
    if (app_is_setup_cancelled()) {
        snprintf(output, out_size, "CANCELLED: setup cancelled by user");
        ui_progress_finish("OpenClaw Setup", false, output);
        return false;
    }

    bool is_local = (strcmp(mode, "local") == 0);

    ui_progress_begin("OpenClaw Setup", "Pre-check environment", is_local ? 3 : 6);

    /* ── Node.js pre-check ── */
    if (is_local) {
        /* Local (offline): do NOT auto-download Node.js — just verify it exists */
        char node_ver[64] = {0};
        if (!app_check_nodejs(node_ver, sizeof(node_ver))) {
            snprintf(output, out_size,
                     "Node.js not found. Offline install requires Node.js pre-installed.\n"
                     "Download: https://nodejs.org/ (LTS >= v22)");
            ui_progress_finish("OpenClaw Setup", false, output);
            return false;
        }
        if (!app_check_nodejs_version_ok(node_ver)) {
            snprintf(output, out_size,
                     "Node.js %s is too old (need >= v22). Offline install cannot auto-update.\n"
                     "Download: https://nodejs.org/ (LTS >= v22)", node_ver);
            ui_progress_finish("OpenClaw Setup", false, output);
            return false;
        }
        char npm_ver[32] = {0};
        if (!exec_cmd("npm --version", npm_ver, sizeof(npm_ver)) || !npm_ver[0]) {
            snprintf(output, out_size,
                     "npm not found. Offline install requires npm.\n"
                     "Install Node.js from https://nodejs.org/ (includes npm)");
            ui_progress_finish("OpenClaw Setup", false, output);
            return false;
        }
        ui_log("[Setup] Local pre-check OK: Node %s, npm %s", node_ver, npm_ver);
    } else {
        /* Network/auto: auto-download Node.js if missing */
        if (!install_nodejs_if_missing(output, out_size)) {
            ui_progress_finish("OpenClaw Setup", false, output);
            return false;
        }
    }

    /* FIX: Ensure npm global prefix is in PATH for subsequent openclaw commands.
     * npm install -g installs to the npm prefix dir, which may not be in our stale PATH. */
    {
        char npm_prefix[512] = {0};
        if (exec_cmd("npm prefix -g", npm_prefix, sizeof(npm_prefix), 5000) && npm_prefix[0]) {
            char* nl = strchr(npm_prefix, '\r');
            if (nl) *nl = '\0';
            nl = strchr(npm_prefix, '\n');
            if (nl) *nl = '\0';
            if (fs::exists(npm_prefix)) {
                const char* cur_path = std::getenv("PATH");
                if (cur_path && strstr(cur_path, npm_prefix) == nullptr) {
                    std::string new_path = std::string(npm_prefix) + ";" + cur_path;
                    _putenv_s("PATH", new_path.c_str());
                    ui_log("[Setup] Added npm global prefix to PATH: %s", npm_prefix);
                }
            }
        }
    }

    char cmd[512];
    bool ok = false;

    /* GitHub release URL for OpenClaw tgz (primary source) */
    const char* github_openclaw_url =
        "https://github.com/IwanFlag/AnyClaw_Tools/releases/download/v1.0.0/openclaw-2026.4.8.tgz";

    /* Mode: local (bundled) */
    if (strcmp(mode, "local") == 0) {
        ui_progress_update("OpenClaw Setup", "Locating bundled package", 30);
        char tarball[256] = {0};
        if (!find_bundled_tarball(tarball, sizeof(tarball))) {
            snprintf(output, out_size, "Bundled OpenClaw package not found (bundled\\openclaw.tgz)");
            ui_progress_finish("OpenClaw Setup", false, output);
            return false;
        }
        ui_progress_update("OpenClaw Setup", "Installing from bundled package", 50);
        snprintf(cmd, sizeof(cmd), "npm install -g --yes \"%s\"", tarball);
        LOG_I("Setup", "Installing from bundled: %s", tarball);
        ok = exec_cmd(cmd, output, out_size, 120000);
        if (ok) ui_progress_update("OpenClaw Setup", "Package installed", 80);
    }
    /* Mode: network */
    else if (strcmp(mode, "network") == 0) {
        /* Try GitHub release first */
        ui_progress_update("OpenClaw Setup", "GitHub release (AnyClaw)", 35);
        {
            const char* temp = std::getenv("TEMP");
            std::string tgz_path = std::string(temp ? temp : ".") + "\\anyclaw_openclaw.tgz";
            /* FIX: Use download_file() which handles curl fallback to PowerShell */
            if (download_file(github_openclaw_url, tgz_path.c_str(), output, out_size, 10 * 60 * 1000)
                && file_size_ok(tgz_path, 1ull * 1024ull * 1024ull)) {
                snprintf(cmd, sizeof(cmd), "npm install -g --yes \"%s\"", tgz_path.c_str());
                ok = exec_cmd(cmd, output, out_size, 120000);
                if (ok) ui_log("[Setup] OpenClaw installed from GitHub release");
            }
            if (!ok) ui_log("[Setup] GitHub source failed, falling back to npm registry...");
        }
        /* Fallback: npm registries */
        if (!ok) {
            const char* registries[] = {
                "https://registry.npmjs.org/",
                "https://registry.npmmirror.com/",
                "https://mirrors.cloud.tencent.com/npm/",
                nullptr
            };
            for (int i = 0; registries[i]; i++) {
                ui_progress_update("OpenClaw Setup", registries[i], 40 + i * 15);
                ui_log("[Setup] OpenClaw npm source %d/3: %s", i + 1, registries[i]);
                snprintf(cmd, sizeof(cmd), "set \"npm_config_registry=%s\" && npm install -g --yes openclaw", registries[i]);
                ok = exec_cmd(cmd, output, out_size, 8 * 60 * 1000);
                if (ok) break;
                ui_log("[Setup] Source %d failed/timeout, auto switch next source", i + 1);
            }
        }
    }
    /* Mode: auto (GitHub → npm registries → bundled) */
    else {
        /* Step 1: Try GitHub release */
        ui_progress_update("OpenClaw Setup", "GitHub release (AnyClaw)", 30);
        {
            const char* temp = std::getenv("TEMP");
            std::string tgz_path = std::string(temp ? temp : ".") + "\\anyclaw_openclaw.tgz";
            /* FIX: Use download_file() which handles curl fallback to PowerShell */
            if (download_file(github_openclaw_url, tgz_path.c_str(), output, out_size, 10 * 60 * 1000)
                && file_size_ok(tgz_path, 1ull * 1024ull * 1024ull)) {
                snprintf(cmd, sizeof(cmd), "npm install -g --yes \"%s\"", tgz_path.c_str());
                ok = exec_cmd(cmd, output, out_size, 120000);
                if (ok) ui_log("[Setup] OpenClaw installed from GitHub release");
            }
            if (!ok) ui_log("[Setup] GitHub source failed, falling back to npm registry...");
        }

        /* Step 2: Fallback to npm registries */
        if (!ok) {
            const char* registries[] = {
                "https://registry.npmjs.org/",
                "https://registry.npmmirror.com/",
                "https://mirrors.cloud.tencent.com/npm/",
                nullptr
            };
            for (int i = 0; registries[i]; i++) {
                ui_progress_update("OpenClaw Setup", registries[i], 40 + i * 10);
                ui_log("[Setup] OpenClaw npm source %d/3: %s", i + 1, registries[i]);
                snprintf(cmd, sizeof(cmd), "set \"npm_config_registry=%s\" && npm install -g --yes openclaw", registries[i]);
                ok = exec_cmd(cmd, output, out_size, 8 * 60 * 1000);
                if (ok) break;
                ui_log("[Setup] Source %d failed/timeout, auto switch next source", i + 1);
            }
        }

        /* Step 3: Fallback to bundled */
        if (!ok) {
            LOG_W("Setup", "Network install failed, trying bundled...");
            ui_progress_update("OpenClaw Setup", "Network failed, fallback to bundled", 75);
            char local_out[1024] = {0};
            ok = app_install_openclaw_ex(local_out, sizeof(local_out), "local");
            if (!ok && out_size > 0) {
                snprintf(output, out_size, "Network failed. Local fallback also failed: %s", local_out);
            }
        }
    }

    if (ok) {
        LOG_I("OpenClaw", "Install succeeded (%s)", mode);
        LOG_I("Setup", "OpenClaw installed (%s)", mode);
        ui_progress_finish("OpenClaw Setup", true, "OpenClaw installed");
    } else {
        LOG_E("OpenClaw", "Install failed (%s): %s", mode, output);
        ui_progress_finish("OpenClaw Setup", false, output[0] ? output : "OpenClaw install failed");
    }
    return ok;
}

/* Simple wrapper: try network, fallback to local */
bool app_install_openclaw(char* output, int out_size) {
    return app_install_openclaw_ex(output, out_size, "auto");
}

bool app_install_gemma_models(int model_mask, char* output, int out_size) {
    if (!output || out_size <= 0) return false;
    output[0] = '\0';
    if (app_is_setup_cancelled()) {
        snprintf(output, out_size, "CANCELLED: setup cancelled by user");
        ui_progress_finish("Gemma Setup", false, output);
        return false;
    }
    if (model_mask <= 0 || model_mask > 7) {
        snprintf(output, out_size, "Invalid model mask: %d", model_mask);
        return false;
    }
    if (!perm_check_exec(PermKey::EXEC_INSTALL, "gemma model download")) {
        snprintf(output, out_size, "DENY: exec_install permission blocked");
        ui_progress_finish("Gemma Setup", false, output);
        return false;
    }

    const char* userprofile = std::getenv("USERPROFILE");
    if (!userprofile) {
        snprintf(output, out_size, "USERPROFILE not found");
        return false;
    }
    std::string model_root = std::string(userprofile) + "\\.anyclaw\\models\\gemma";
    try { fs::create_directories(model_root); } catch (...) {}

    int ok_count = 0;
    int need_count = 0;
    char last_err[512] = {0};
    ui_progress_begin("Gemma Setup", "Preparing model downloads", 5);
    for (int i = 0; i < 3; i++) {
        if (app_is_setup_cancelled()) {
            snprintf(output, out_size, "CANCELLED: setup cancelled by user");
            ui_progress_finish("Gemma Setup", false, output);
            return false;
        }
        int bit = (1 << i);
        if ((model_mask & bit) == 0) continue;
        need_count++;
        std::string target = model_root + "\\" + kGemmaSources[i].file_name;
        int base_pct = 10 + (need_count - 1) * 30;
        ui_progress_update("Gemma Setup", kGemmaSources[i].name, base_pct);
        if (file_size_ok(target, 10ull * 1024ull * 1024ull)) {
            LOG_I("GEMMA", "%s already exists, skip: %s", kGemmaSources[i].name, target.c_str());
            ok_count++;
            continue;
        }
        LOG_I("GEMMA", "Downloading %s to %s", kGemmaSources[i].name, target.c_str());
        char dl_out[512] = {0};
        bool ok = download_file_with_fallback(
            kGemmaSources[i].mirror_url,
            kGemmaSources[i].mirror2_url,
            kGemmaSources[i].official_url,
            target.c_str(),
            dl_out, sizeof(dl_out)
        );
        if (ok) {
            ok_count++;
            LOG_I("GEMMA", "Downloaded %s", kGemmaSources[i].name);
            ui_progress_update("Gemma Setup", "Downloaded one model", base_pct + 25);
        } else {
            try {
                if (fs::exists(target)) {
                    fs::remove(target);
                    LOG_W("GEMMA", "Removed partial file after failed download: %s", target.c_str());
                }
            } catch (...) {}
            snprintf(last_err, sizeof(last_err), "%s", dl_out[0] ? dl_out : "download failed");
            LOG_E("GEMMA", "Download failed %s: %s", kGemmaSources[i].name, last_err);
        }
    }

    if (ok_count == need_count) {
        int verified = 0;
        for (int i = 0; i < 3; i++) {
            int bit = (1 << i);
            if ((model_mask & bit) == 0) continue;
            std::string target = model_root + "\\" + kGemmaSources[i].file_name;
            if (file_size_ok(target, 10ull * 1024ull * 1024ull)) verified++;
        }
        if (verified == need_count) {
            snprintf(output, out_size, "Gemma download+verify complete: %d/%d at %s", verified, need_count, model_root.c_str());
            ui_progress_finish("Gemma Setup", true, output);
            return true;
        }
        snprintf(output, out_size, "Gemma verify failed: %d/%d valid model files at %s", verified, need_count, model_root.c_str());
        ui_progress_finish("Gemma Setup", false, output);
        return false;
    }

    snprintf(output, out_size, "Gemma partial/failed: %d/%d success, last_error=%s", ok_count, need_count, last_err);
    ui_progress_finish("Gemma Setup", false, output);
    return false;
}

/* ── Check if openclaw.json exists ─────────────────────────── */
static bool openclaw_config_exists() {
    /* Check USERPROFILE\.openclaw\openclaw.json */
    const char* userprofile = std::getenv("USERPROFILE");
    if (userprofile) {
        std::string path = std::string(userprofile) + "\\.openclaw\\openclaw.json";
        std::ifstream f(path);
        if (f.is_open()) return true;
    }
    /* Check test path (cross-compile env) */
    std::ifstream f2("Z:\\root\\.openclaw\\openclaw.json");
    if (f2.is_open()) return true;
    return false;
}

/* ── Copy workspace template files to %USERPROFILE%\.openclaw\workspace\ ── */
/* Called after OpenClaw init to set up default agent workspace files */
static bool setup_workspace_template() {
    const char* userprofile = std::getenv("USERPROFILE");
    if (!userprofile) {
        LOG_W("Setup", "USERPROFILE not set, skipping workspace template");
        return false;
    }

    /* Target: %USERPROFILE%\.openclaw\workspace\ */
    char ws_dir[512];
    snprintf(ws_dir, sizeof(ws_dir), "%s\\.openclaw\\workspace", userprofile);
    CreateDirectoryA(ws_dir, nullptr);

    /* Source: bundled\workspace\ relative to exe */
    char exe_dir[512] = {0};
    GetModuleFileNameA(nullptr, exe_dir, sizeof(exe_dir));
    /* Strip exe name to get directory */
    char* last_slash = strrchr(exe_dir, '\\');
    if (last_slash) *last_slash = '\0';

    char src_dir[512];
    snprintf(src_dir, sizeof(src_dir), "%s\\bundled\\workspace", exe_dir);

    /* Template files to copy */
    const char* files[] = {
        "AGENTS.md", "SOUL.md", "HEARTBEAT.md", "BOOTSTRAP.md",
        "IDENTITY.md", "USER.md", "TOOLS.md", nullptr
    };

    int copied = 0;
    for (int i = 0; files[i]; i++) {
        char src[512], dst[512];
        snprintf(src, sizeof(src), "%s\\%s", src_dir, files[i]);
        snprintf(dst, sizeof(dst), "%s\\%s", ws_dir, files[i]);

        /* Only copy if target doesn't exist (don't overwrite user customizations) */
        DWORD attrs = GetFileAttributesA(dst);
        if (attrs == INVALID_FILE_ATTRIBUTES) {
            if (CopyFileA(src, dst, FALSE)) {
                copied++;
                LOG_I("Setup", "Copied workspace template: %s", files[i]);
            } else {
                LOG_W("Setup", "Failed to copy %s (err=%d)", files[i], GetLastError());
            }
        } else {
            LOG_D("Setup", "Workspace file exists, skip: %s", files[i]);
        }
    }

    LOG_I("Setup", "Workspace template: %d files copied to %s", copied, ws_dir);
    return copied > 0;
}

/* ── Init OpenClaw after fresh install ─────────────────────── */
/* Runs: openclaw gateway start (generates default config + starts daemon)
 * Validates: config file exists
 * Then: openclaw gateway stop (AnyClaw will manage it later) */
bool app_init_openclaw(char* output, int out_size) {
    if (!output || out_size <= 0) return false;
    output[0] = '\0';
    if (app_is_setup_cancelled()) {
        snprintf(output, out_size, "CANCELLED: setup cancelled by user");
        return false;
    }

    LOG_I("OpenClaw", "Initializing gateway...");

    char tmp[512] = {0};

    /* 1. Start gateway to generate default config (--yes skips any interactive prompts) */
    /* FIX: stdin redirected to NUL in exec_cmd, --yes as belt-and-suspenders */
    bool start_ok = exec_cmd("openclaw gateway start --yes 2>nul || openclaw gateway start",
                             tmp, sizeof(tmp), 15000);

    /* 2. Verify config file was created */
    bool config_ok = openclaw_config_exists();
    if (!config_ok) {
        /* Maybe it's the first run and needs a moment */
        exec_cmd("timeout 2 >nul 2>&1 || sleep 2", tmp, sizeof(tmp), 5000);
        config_ok = openclaw_config_exists();
    }

    /* 3. Stop gateway — AnyClaw manages lifecycle */
    exec_cmd("openclaw gateway stop", tmp, sizeof(tmp), 10000);

    if (!start_ok) {
        snprintf(output, out_size, "gateway start failed");
        LOG_E("OpenClaw", "Init FAILED: gateway start returned error");
        return false;
    }
    if (!config_ok) {
        snprintf(output, out_size, "config file not created after gateway start");
        LOG_E("OpenClaw", "Init FAILED: openclaw.json not found");
        return false;
    }

    LOG_I("OpenClaw", "Init OK (config exists)");
    return true;
}

/* ── Full setup: install + init + configure ────────────────── */
/* One-call for wizard: install OpenClaw, init gateway, apply config */
bool app_full_setup(const char* install_mode, const char* api_key, const char* model_name,
                    char* output, int out_size) {
    if (!output || out_size <= 0) return false;
    output[0] = '\0';

    /* 1. Install */
    if (!app_install_openclaw_ex(output, out_size, install_mode)) {
        return false;
    }

    /* 2. Init (generate default config) */
    if (!app_init_openclaw(output, out_size)) {
        return false;
    }

    /* 2.5. Setup workspace template (AGENTS.md, SOUL.md, etc.) */
    setup_workspace_template();

    /* 3. Configure (model + API key → restart) */
    if (model_name && model_name[0]) {
        app_update_model_config(api_key, model_name);
    }

    /* 4. Verify */
    return app_verify_openclaw(output, out_size);
}

/* ── Uninstall OpenClaw ────────────────────────────────────── */
/* full_cleanup: also remove USERPROFILE\.openclaw\ + APPDATA\AnyClaw_LVGL\ */
bool app_uninstall_openclaw_full(char* output, int out_size, bool full_cleanup) {
    if (!output || out_size <= 0) return false;
    output[0] = '\0';

    ui_progress_begin("OpenClaw Uninstall", "Stopping gateway...", 5);

    /* 1. Stop gateway first */
    char tmp[256] = {0};
    exec_cmd("openclaw gateway stop", tmp, sizeof(tmp), 10000);
    ui_progress_update("OpenClaw Uninstall", "Uninstalling npm package...", 30);

    /* 2. npm uninstall */
    LOG_I("Setup", "Uninstalling openclaw via npm...");
    bool ok = exec_cmd("npm uninstall -g openclaw", output, out_size, 30000);

    /* 3. Remove config/workspace if requested */
    if (ok && full_cleanup) {
        ui_progress_update("OpenClaw Uninstall", "Cleaning up config...", 70);
        char rm_cmd[512];

        /* OpenClaw config: USERPROFILE\.openclaw\ */
        const char* userprofile = std::getenv("USERPROFILE");
        if (userprofile) {
            snprintf(rm_cmd, sizeof(rm_cmd), "rmdir /s /q \"%s\\.openclaw\"", userprofile);
            LOG_I("Setup", "Removing: %s\\.openclaw\\", userprofile);
            exec_cmd(rm_cmd, tmp, sizeof(tmp), 10000);
        }

        /* AnyClaw config: APPDATA\AnyClaw_LVGL\ */
        const char* appdata = std::getenv("APPDATA");
        if (appdata) {
            snprintf(rm_cmd, sizeof(rm_cmd), "rmdir /s /q \"%s\\AnyClaw_LVGL\"", appdata);
            LOG_I("Setup", "Removing: %s\\AnyClaw_LVGL\\", appdata);
            exec_cmd(rm_cmd, tmp, sizeof(tmp), 10000);
        }
    }

    if (ok) {
        LOG_I("Setup", "Uninstall complete (full_cleanup=%d)", full_cleanup);
        ui_progress_finish("OpenClaw Uninstall", true, "OpenClaw removed");
    } else {
        LOG_E("Setup", "Uninstall failed: %s", output[0] ? output : "unknown error");
        ui_progress_finish("OpenClaw Uninstall", false, output[0] ? output : "npm uninstall failed");
    }
    return ok;
}

bool app_uninstall_openclaw(char* output, int out_size) {
    return app_uninstall_openclaw_full(output, out_size, false);
}

void app_request_setup_cancel() {
    InterlockedExchange(&g_setup_cancel_requested, 1);
}

void app_reset_setup_cancel() {
    InterlockedExchange(&g_setup_cancel_requested, 0);
}

bool app_is_setup_cancelled() {
    return InterlockedCompareExchange(&g_setup_cancel_requested, 0, 0) != 0;
}

/* ── Verify OpenClaw is working ────────────────────────────── */
/* Returns true if: openclaw --version works + config exists + gateway can start */
bool app_verify_openclaw(char* detail_out, int out_size) {
    if (detail_out && out_size > 0) detail_out[0] = '\0';

    /* 1. Check openclaw CLI works */
    char ver[256] = {0};
    if (!exec_cmd("openclaw --version", ver, sizeof(ver)) || !ver[0]) {
        if (detail_out) snprintf(detail_out, out_size, "openclaw command not found");
        return false;
    }

    /* Trim */
    char* p = ver + strlen(ver) - 1;
    while (p > ver && (*p == '\r' || *p == '\n' || *p == ' ')) { *p = '\0'; p--; }

    /* 2. Check config file exists */
    if (!openclaw_config_exists()) {
        if (detail_out) snprintf(detail_out, out_size, "OpenClaw %s (no config)", ver);
        LOG_W("OpenClaw", "Verified CLI but no config");
        return true; /* CLI works, config will be created by wizard */
    }

    /* 3. Check gateway responds */
    char resp[128] = {0};
    int code = http_get("http://127.0.0.1:18789/health", resp, sizeof(resp), 3);
    bool gw_ok = (code > 0 && code < 500);

    if (detail_out) {
        snprintf(detail_out, out_size, "OpenClaw %s %s", ver,
                 gw_ok ? "(gateway OK)" : "(gateway not running)");
    }
    LOG_I("OpenClaw", "Verified: %s, gateway=%s", ver, gw_ok ? "OK" : "offline");
    return true;
}

/* ── Helper: find npm.cmd in known paths (no subprocess needed) ── */
static bool find_npm_cmd(char* path_out, int path_size) {
    if (!path_out || path_size <= 0) return false;
    path_out[0] = '\0';

    /* Try node.exe sibling + npm.cmd (most reliable) */
    const char* node_candidates[] = {
        "C:\\Program Files\\nodejs\\node.exe",
        "C:\\Program Files (x86)\\nodejs\\node.exe",
    };
    for (int i = 0; i < (int)(sizeof(node_candidates) / sizeof(node_candidates[0])); i++) {
        if (GetFileAttributesA(node_candidates[i]) != INVALID_FILE_ATTRIBUTES) {
            /* Found node.exe — check for npm.cmd in same dir */
            std::string npm_cmd = std::string(node_candidates[i]);
            size_t pos = npm_cmd.rfind('\\');
            if (pos != std::string::npos) npm_cmd = npm_cmd.substr(0, pos) + "\\npm.cmd";
            if (GetFileAttributesA(npm_cmd.c_str()) != INVALID_FILE_ATTRIBUTES) {
                snprintf(path_out, path_size, "\"%s\"", npm_cmd.c_str());
                return true;
            }
        }
    }

    /* Try npm.cmd directly in common locations */
    const char* npm_cmd_candidates[] = {
        "C:\\Program Files\\nodejs\\npm.cmd",
        "C:\\Program Files (x86)\\nodejs\\npm.cmd",
        "C:\\Users\\thundersoft\\AppData\\Roaming\\npm\\npm.cmd",
    };
    for (int i = 0; i < (int)(sizeof(npm_cmd_candidates) / sizeof(npm_cmd_candidates[0])); i++) {
        if (GetFileAttributesA(npm_cmd_candidates[i]) != INVALID_FILE_ATTRIBUTES) {
            snprintf(path_out, path_size, "\"%s\"", npm_cmd_candidates[i]);
            return true;
        }
    }
    return false;
}

/* ── Helper: find openclaw.cmd in known paths (no subprocess needed) ── */
static bool find_openclaw_cmd(char* path_out, int path_size) {
    if (!path_out || path_size <= 0) return false;
    path_out[0] = '\0';

    const char* candidates[] = {
        "C:\\Users\\thundersoft\\AppData\\Roaming\\npm\\openclaw.cmd",
        "C:\\Users\\thundersoft\\AppData\\Roaming\\npm\\openclaw",
        "C:\\Program Files\\nodejs\\openclaw.cmd",
    };
    for (int i = 0; i < (int)(sizeof(candidates) / sizeof(candidates[0])); i++) {
        DWORD attr = GetFileAttributesA(candidates[i]);
        if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
            snprintf(path_out, path_size, "\"%s\"", candidates[i]);
            return true;
        }
    }
    return false;
}

/* ── Full environment check ────────────────────────────────── */
EnvCheckResult app_check_environment() {
    EnvCheckResult result = {};
    result.error_msg[0] = '\0';

    /* Node.js */
    result.node_ok = app_check_nodejs(result.node_ver, sizeof(result.node_ver));
    if (result.node_ok) {
        result.node_version_ok = app_check_nodejs_version_ok(result.node_ver);
        if (!result.node_version_ok) {
            snprintf(result.error_msg, sizeof(result.error_msg),
                     "Node.js %s is too old (need >= v%d)", result.node_ver, NODE_MIN_MAJOR);
        }
    }

    /* npm — try direct file path first (works when npm is a bash script in MSYS2 env) */
    {
        char npm_path[512] = {0};
        if (find_npm_cmd(npm_path, sizeof(npm_path))) {
            std::string ver_cmd = std::string(npm_path) + " --version";
            char npm_out[64] = {0};
            if (exec_cmd(ver_cmd.c_str(), npm_out, sizeof(npm_out)) && npm_out[0]) {
                result.npm_ok = true;
                char* p = npm_out + strlen(npm_out) - 1;
                while (p > npm_out && (*p == '\r' || *p == '\n' || *p == ' ')) { *p = '\0'; p--; }
                strncpy(result.npm_ver, npm_out, sizeof(result.npm_ver) - 1);
            }
        }
        /* Fallback: try PATH search */
        if (!result.npm_ok) {
            char npm_out[64] = {0};
            result.npm_ok = exec_cmd("npm --version", npm_out, sizeof(npm_out)) && npm_out[0];
            if (result.npm_ok) {
                char* p = npm_out + strlen(npm_out) - 1;
                while (p > npm_out && (*p == '\r' || *p == '\n' || *p == ' ')) { *p = '\0'; p--; }
                strncpy(result.npm_ver, npm_out, sizeof(result.npm_ver) - 1);
            }
        }
    }

    /* OpenClaw — try direct file path first (works when openclaw is a bash script in MSYS2 env) */
    {
        char oc_path[512] = {0};
        if (find_openclaw_cmd(oc_path, sizeof(oc_path))) {
            std::string ver_cmd = std::string(oc_path) + " --version";
            char ver_out[256] = {0};
            if (exec_cmd(ver_cmd.c_str(), ver_out, sizeof(ver_out)) && ver_out[0]) {
                result.openclaw_ok = true;
                char* p = ver_out + strlen(ver_out) - 1;
                while (p > ver_out && (*p == '\r' || *p == '\n' || *p == ' ')) { *p = '\0'; p--; }
                strncpy(result.openclaw_ver, ver_out, sizeof(result.openclaw_ver) - 1);
            }
        }
        /* Fallback: standard PATH search */
        if (!result.openclaw_ok) {
            result.openclaw_ok = app_verify_openclaw(result.openclaw_ver, sizeof(result.openclaw_ver));
        }
    }

    /* Hermes Agent */
    result.hermes_ok = app_detect_hermes(result.hermes_ver, sizeof(result.hermes_ver));
    if (result.hermes_ok) {
        result.hermes_healthy = app_check_hermes_health();
    }

    /* Claude CLI */
    result.claude_ok = app_detect_claude_cli(result.claude_ver, sizeof(result.claude_ver));
    if (result.claude_ok) {
        result.claude_healthy = app_check_claude_cli_health();
    }

    LOG_I("Env", "Node=%s(%s) npm=%s OpenClaw=%s Hermes=%s Claude=%s",
           result.node_ok ? "OK" : "MISSING", result.node_ver,
           result.npm_ok ? result.npm_ver : "MISSING",
           result.openclaw_ok ? result.openclaw_ver : "MISSING",
           result.hermes_ok ? result.hermes_ver : "MISSING",
           result.claude_ok ? result.claude_ver : "MISSING");

    return result;
}

/* ═══ Session Management ═══ */

bool app_abort_session(const char* session_key) {
    if (!session_key || !session_key[0]) return false;

    char cmd[512];
    snprintf(cmd, sizeof(cmd),
             "openclaw gateway call sessions.reset --json -d \"{\\\"sessionKey\\\":\\\"%s\\\"}\"",
             session_key);

    char output[2048] = {0};
    bool ok = exec_cmd(cmd, output, sizeof(output), 10000);

    if (!ok || !output[0]) {
        LOG_W("Session", "Abort '%s' failed", session_key);
        return false;
    }

    LOG_I("Session", "Aborted '%s': %.100s", session_key, output);
    return true;
}

bool app_abort_all_sessions() {
    char output[4096] = {0};
    bool ok = exec_cmd(
        "openclaw gateway call sessions.reset --json",
        output, sizeof(output), 15000
    );

    if (!ok) {
        LOG_W("Session", "Abort all sessions failed");
        return false;
    }

    LOG_I("Session", "Reset all sessions: %.200s", output);
    return true;
}

bool app_cron_list(char* output, int out_size) {
    if (!output || out_size <= 0) return false;
    output[0] = '\0';
    bool ok = exec_cmd("openclaw cron list", output, out_size, 10000);
    if (!ok && out_size > 0) snprintf(output, out_size, "Failed to run: openclaw cron list");
    return ok;
}

bool app_cron_run(const char* cron_id, char* output, int out_size) {
    if (!cron_id || !cron_id[0] || !output || out_size <= 0) return false;
    char cmd[512] = {0};
    snprintf(cmd, sizeof(cmd), "openclaw cron run \"%s\"", cron_id);
    bool ok = exec_cmd(cmd, output, out_size, 15000);
    if (!ok && out_size > 0) snprintf(output, out_size, "Failed to run cron: %s", cron_id);
    return ok;
}

bool app_cron_enable(const char* cron_id, bool enable, char* output, int out_size) {
    if (!cron_id || !cron_id[0] || !output || out_size <= 0) return false;
    char cmd[512] = {0};
    snprintf(cmd, sizeof(cmd), "openclaw cron %s \"%s\"", enable ? "enable" : "disable", cron_id);
    bool ok = exec_cmd(cmd, output, out_size, 10000);
    if (!ok && out_size > 0) snprintf(output, out_size, "Failed to %s cron: %s", enable ? "enable" : "disable", cron_id);
    return ok;
}

bool app_cron_delete(const char* cron_id, char* output, int out_size) {
    if (!cron_id || !cron_id[0] || !output || out_size <= 0) return false;
    char cmd[512] = {0};
    snprintf(cmd, sizeof(cmd), "openclaw cron delete \"%s\"", cron_id);
    bool ok = exec_cmd(cmd, output, out_size, 10000);
    if (!ok && out_size > 0) snprintf(output, out_size, "Failed to delete cron: %s", cron_id);
    return ok;
}
