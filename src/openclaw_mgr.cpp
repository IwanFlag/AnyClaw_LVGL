#include "app.h"
#include "app_log.h"
#include "permissions.h"
#include "workspace.h"
#include <windows.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include <shlobj.h>
#include <winhttp.h>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>
#include <sstream>

namespace fs = std::filesystem;

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

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.hStdOutput = hWrite;
    si.hStdError  = hWrite;
    si.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION pi{};

    std::string full = std::string("cmd /C \"") + cmd + "\"";
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
    if (hJob) CloseHandle(hJob);

    /* Trim */
    while (!result.empty() && (result.back() == '\r' || result.back() == '\n' || result.back() == ' '))
        result.pop_back();

    strncpy(output, result.c_str(), out_size - 1);
    output[out_size - 1] = '\0';
    return exitCode == 0;
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
    if (exec_cmd("openclaw config get agents.defaults.model.primary", output, sizeof(output))) {
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
    if (!exec_cmd("openclaw config get models", output, sizeof(output))) {
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
bool app_check_nodejs(char* version_out, int out_size) {
    if (version_out && out_size > 0) version_out[0] = '\0';
    char output[256] = {0};
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
    {"nodejs.org",       "https://nodejs.org/dist/v22.22.1/node-v22.22.1-x64.msi"},
    {"npmmirror (CN)",   "https://registry.npmmirror.com/-/binary/node/v22.22.1/node-v22.22.1-x64.msi"},
    {nullptr, nullptr}
#else
    {"nodejs.org",       "https://nodejs.org/dist/v22.22.1/node-v22.22.1-linux-x64.tar.xz"},
    {"npmmirror (CN)",   "https://registry.npmmirror.com/-/binary/node/v22.22.1/node-v22.22.1-linux-x64.tar.xz"},
    {nullptr, nullptr}
#endif
};

const char* app_get_nodejs_download_url() {
    return nodejs_sources[0].url;
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
/* Searches relative to exe dir for bundled/openclaw-*.tgz */
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
        /* Check if file exists using stdio */
        FILE* f = fopen(search_paths[i], "rb");
        if (f) {
            fclose(f);
            strncpy(path_out, search_paths[i], out_size - 1);
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
    const char* official_url;
    const char* file_name;
};

static const GemmaSource kGemmaSources[] = {
    {
        "Gemma 4 2B",
        "https://hf-mirror.com/bartowski/google_gemma-2-2b-it-GGUF/resolve/main/google_gemma-2-2b-it-Q4_K_M.gguf",
        "https://huggingface.co/bartowski/google_gemma-2-2b-it-GGUF/resolve/main/google_gemma-2-2b-it-Q4_K_M.gguf",
        "gemma-2-2b-it-q4_k_m.gguf"
    },
    {
        "Gemma 4 9B",
        "https://hf-mirror.com/bartowski/google_gemma-2-9b-it-GGUF/resolve/main/google_gemma-2-9b-it-Q4_K_M.gguf",
        "https://huggingface.co/bartowski/google_gemma-2-9b-it-GGUF/resolve/main/google_gemma-2-9b-it-Q4_K_M.gguf",
        "gemma-2-9b-it-q4_k_m.gguf"
    },
    {
        "Gemma 4 27B",
        "https://hf-mirror.com/bartowski/google_gemma-2-27b-it-GGUF/resolve/main/google_gemma-2-27b-it-Q4_K_M.gguf",
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

static bool download_file_with_fallback(const char* primary_url,
                                        const char* fallback_url,
                                        const char* target_path,
                                        char* output, int out_size) {
    if (!primary_url || !fallback_url || !target_path) return false;
    if (!ensure_parent_dir(target_path)) {
        snprintf(output, out_size, "Cannot create model directory");
        return false;
    }

    const char* urls[2] = {primary_url, fallback_url};
    char cmd[2048];
    for (int i = 0; i < 2; i++) {
        snprintf(cmd, sizeof(cmd),
                 "curl -L --fail --retry 2 --connect-timeout 15 --max-time 0 -o \"%s\" \"%s\"",
                 target_path, urls[i]);
        if (exec_cmd(cmd, output, out_size, 4 * 60 * 60 * 1000)) {
            if (file_size_ok(target_path, 10ull * 1024ull * 1024ull)) {
                return true;
            }
            snprintf(output, out_size, "Downloaded file too small, possible incomplete: %s", target_path);
        }
    }
    return false;
}

/* ── Install OpenClaw ──────────────────────────────────────── */
/* mode: "network" = npm registry, "local" = bundled tarball, "auto" = try network then fallback */
bool app_install_openclaw_ex(char* output, int out_size, const char* mode) {
    if (!output || out_size <= 0) return false;
    output[0] = '\0';

    /* Check Node.js first */
    if (!app_check_nodejs(nullptr, 0)) {
        snprintf(output, out_size, "Node.js not installed");
        return false;
    }

    char cmd[512];
    bool ok = false;

    /* Mode: local (bundled) */
    if (strcmp(mode, "local") == 0) {
        char tarball[256] = {0};
        if (!find_bundled_tarball(tarball, sizeof(tarball))) {
            snprintf(output, out_size, "Bundled OpenClaw package not found");
            return false;
        }
        snprintf(cmd, sizeof(cmd), "npm install -g \"%s\"", tarball);
        LOG_I("Setup", "Installing from bundled: %s", tarball);
        ok = exec_cmd(cmd, output, out_size, 120000);
    }
    /* Mode: network */
    else if (strcmp(mode, "network") == 0) {
        snprintf(cmd, sizeof(cmd), "npm install -g openclaw");
        LOG_I("Setup", "Installing from npm registry...");
        ok = exec_cmd(cmd, output, out_size, 120000);
    }
    /* Mode: auto (network first, fallback to local) */
    else {
        snprintf(cmd, sizeof(cmd), "npm install -g openclaw");
        LOG_I("Setup", "Trying network install...");
        ok = exec_cmd(cmd, output, out_size, 120000);

        if (!ok) {
            LOG_W("Setup", "Network install failed, trying bundled...");
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
    } else {
        LOG_E("OpenClaw", "Install failed (%s): %s", mode, output);
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
    if (model_mask <= 0 || model_mask > 7) {
        snprintf(output, out_size, "Invalid model mask: %d", model_mask);
        return false;
    }
    if (!perm_check_exec(PermKey::EXEC_INSTALL, "gemma model download")) {
        snprintf(output, out_size, "DENY: exec_install permission blocked");
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
    for (int i = 0; i < 3; i++) {
        int bit = (1 << i);
        if ((model_mask & bit) == 0) continue;
        need_count++;
        std::string target = model_root + "\\" + kGemmaSources[i].file_name;
        if (file_size_ok(target, 10ull * 1024ull * 1024ull)) {
            LOG_I("GEMMA", "%s already exists, skip: %s", kGemmaSources[i].name, target.c_str());
            ok_count++;
            continue;
        }
        LOG_I("GEMMA", "Downloading %s to %s", kGemmaSources[i].name, target.c_str());
        char dl_out[512] = {0};
        bool ok = download_file_with_fallback(
            kGemmaSources[i].mirror_url,
            kGemmaSources[i].official_url,
            target.c_str(),
            dl_out, sizeof(dl_out)
        );
        if (ok) {
            ok_count++;
            LOG_I("GEMMA", "Downloaded %s", kGemmaSources[i].name);
        } else {
            snprintf(last_err, sizeof(last_err), "%s", dl_out[0] ? dl_out : "download failed");
            LOG_E("GEMMA", "Download failed %s: %s", kGemmaSources[i].name, last_err);
        }
    }

    if (ok_count == need_count) {
        snprintf(output, out_size, "Gemma download complete: %d/%d at %s", ok_count, need_count, model_root.c_str());
        return true;
    }

    snprintf(output, out_size, "Gemma partial/failed: %d/%d success, last_error=%s", ok_count, need_count, last_err);
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

    LOG_I("OpenClaw", "Initializing gateway...");

    char tmp[512] = {0};

    /* 1. Start gateway to generate default config */
    bool start_ok = exec_cmd("openclaw gateway start", tmp, sizeof(tmp), 15000);

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

    /* 1. Stop gateway first */
    char tmp[256] = {0};
    exec_cmd("openclaw gateway stop", tmp, sizeof(tmp), 10000);

    /* 2. npm uninstall */
    LOG_I("Setup", "Uninstalling openclaw via npm...");
    bool ok = exec_cmd("npm uninstall -g openclaw", output, out_size, 30000);

    /* 3. Remove config/workspace if requested */
    if (ok && full_cleanup) {
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

    if (ok) LOG_I("Setup", "Uninstall complete (full_cleanup=%d)", full_cleanup);
    return ok;
}

bool app_uninstall_openclaw(char* output, int out_size) {
    return app_uninstall_openclaw_full(output, out_size, false);
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

    /* npm */
    char npm_out[64] = {0};
    result.npm_ok = exec_cmd("npm --version", npm_out, sizeof(npm_out)) && npm_out[0];
    if (result.npm_ok) {
        char* p = npm_out + strlen(npm_out) - 1;
        while (p > npm_out && (*p == '\r' || *p == '\n' || *p == ' ')) { *p = '\0'; p--; }
        strncpy(result.npm_ver, npm_out, sizeof(result.npm_ver) - 1);
    }

    /* OpenClaw */
    result.openclaw_ok = app_verify_openclaw(result.openclaw_ver, sizeof(result.openclaw_ver));

    LOG_I("Env", "Node=%s(%s) npm=%s(%s) OpenClaw=%s(%s)",
           result.node_ok ? "OK" : "MISSING", result.node_ver,
           result.npm_ok ? "OK" : "MISSING", result.npm_ver,
           result.openclaw_ok ? "OK" : "MISSING", result.openclaw_ver);

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
