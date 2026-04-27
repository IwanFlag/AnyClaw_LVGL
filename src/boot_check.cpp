/* ═══════════════════════════════════════════════════════════════
 *  boot_check.cpp - Comprehensive environment health check + auto-repair
 *  AnyClaw LVGL v2.0 - BOOT-01
 * ═══════════════════════════════════════════════════════════════ */

#include "boot_check.h"
#include "app.h"
#include "app_config.h"
#include "app_log.h"
#include "paths.h"
#include "permissions.h"
#include "runtime_mgr.h"
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <sstream>
#include <fstream>
#include <algorithm>

namespace fs = std::filesystem;

/* ── Helper: run command and capture output ── */
static bool boot_run_cmd(const char* cmd, std::string& output, DWORD timeout_ms = 8000) {
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

    if (!CreateProcessA(nullptr, buf.data(), nullptr, nullptr, TRUE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        CloseHandle(hRead);
        CloseHandle(hWrite);
        return false;
    }

    CloseHandle(hWrite);

    output.clear();
    char readBuf[4096];
    DWORD bytesRead = 0;
    while (ReadFile(hRead, readBuf, sizeof(readBuf), &bytesRead, nullptr) && bytesRead > 0) {
        output.append(readBuf, bytesRead);
    }

    WaitForSingleObject(pi.hProcess, timeout_ms);
    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hRead);

    /* Trim trailing whitespace */
    while (!output.empty() && (output.back() == '\r' || output.back() == '\n' || output.back() == ' '))
        output.pop_back();

    return exitCode == 0;
}

/* ── Helper: get exe directory ── */
static std::string get_exe_dir() {
    char path[MAX_PATH] = {0};
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    char* p = strrchr(path, '\\');
    if (p) *(p + 1) = '\0';
    return std::string(path);
}

/* ── Helper: check if a TCP port is in use ── */
static bool is_port_in_use(int port) {
    /* Use netstat to check if port is listening */
    std::string cmd = "netstat -ano | findstr \":" + std::to_string(port) + "\" | findstr \"LISTENING\"";
    std::string output;
    return boot_run_cmd(cmd.c_str(), output);
}

/* ── Helper: parse major version from "v22.14.0" format ── */
static int parse_major(const std::string& ver) {
    if (ver.empty()) return 0;
    const char* s = ver.c_str();
    if (*s == 'v' || *s == 'V') s++;
    int major = 0;
    while (*s >= '0' && *s <= '9') {
        major = major * 10 + (*s - '0');
        s++;
    }
    return major;
}

/* ═══════════════════════════════════════════════════════════════
 *  Individual Check Functions
 * ═══════════════════════════════════════════════════════════════ */

BootCheckResult check_nodejs_version() {
    BootCheckResult r;
    r.check_name = "Node.js";

    if (!perm_check_exec(PermKey::EXEC_SHELL, "node --version")) {
        r.status = BootCheckStatus::Warn;
        r.message = "Node.js check skipped (permission denied)";
        LOG_W("BOOT", "Node.js: permission denied");
        return r;
    }

    std::string output;
    if (!boot_run_cmd("node --version", output) || output.empty()) {
        r.status = BootCheckStatus::Error;
        r.message = "Node.js not found — please install Node.js >= 22.14";
        r.auto_fix_available = false;
        LOG_W("BOOT", "Node.js: NOT FOUND");
        return r;
    }

    int major = parse_major(output);
    if (major >= 22) {
        r.status = BootCheckStatus::Ok;
        r.message = "Node.js " + output;
        LOG_I("BOOT", "Node.js: %s", output.c_str());
    } else {
        r.status = BootCheckStatus::Error;
        r.message = "Node.js " + output + " — need >= v22.14";
        r.auto_fix_available = false;
        LOG_W("BOOT", "Node.js: %s (too old, need >= 22)", output.c_str());
    }

    return r;
}

BootCheckResult check_npm() {
    BootCheckResult r;
    r.check_name = "npm";

    if (!perm_check_exec(PermKey::EXEC_SHELL, "npm --version")) {
        r.status = BootCheckStatus::Warn;
        r.message = "npm check skipped (permission denied)";
        LOG_W("BOOT", "npm: permission denied");
        return r;
    }

    std::string output;
    if (!boot_run_cmd("npm --version", output) || output.empty()) {
        r.status = BootCheckStatus::Error;
        r.message = "npm not found or broken";
        r.auto_fix_available = true; /* cache clean may help */
        LOG_W("BOOT", "npm: NOT FOUND or ERROR");
        return r;
    }

    r.status = BootCheckStatus::Ok;
    r.message = "npm " + output;
    LOG_I("BOOT", "npm: %s", output.c_str());
    return r;
}

BootCheckResult check_openclaw() {
    BootCheckResult r;
    r.check_name = "OpenClaw";

    std::string output;
    if (!boot_run_cmd("where openclaw", output) || output.empty()) {
        r.status = BootCheckStatus::Error;
        r.message = "OpenClaw not installed — run npm install -g openclaw";
        r.auto_fix_available = false;
        LOG_W("BOOT", "OpenClaw: NOT FOUND");
        return r;
    }

    /* Try to get version */
    std::string ver;
    if (boot_run_cmd("openclaw --version", ver) && !ver.empty()) {
        r.status = BootCheckStatus::Ok;
        r.message = "OpenClaw " + ver;
        LOG_I("BOOT", "OpenClaw: %s", ver.c_str());
    } else {
        r.status = BootCheckStatus::Warn;
        r.message = "OpenClaw found but version check failed";
        LOG_W("BOOT", "OpenClaw: found but version unknown");
    }

    return r;
}

BootCheckResult check_gateway_port() {
    BootCheckResult r;
    r.check_name = "Gateway Port";
    int port = GATEWAY_PORT;

    if (!is_port_in_use(port)) {
        r.status = BootCheckStatus::Ok;
        r.message = "Port " + std::to_string(port) + " available";
        LOG_I("BOOT", "Gateway port %d: available", port);
        return r;
    }

    /* Port is in use — check if it's the OpenClaw gateway (expected) or something else.
     * Primary check via WinHTTP; fallback via raw TCP socket (WinHTTP may be intercepted
     * by system proxy software such as Clash/V2Ray running on loopback). */
    std::string resp;
    int code = http_get(GATEWAY_HEALTH_URL, resp.data(), 0, 2);
    bool port_ok = (code > 0);

    if (!port_ok) {
        /* Fallback: raw TCP connect bypasses proxy hooks */
        SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (s != INVALID_SOCKET) {
            struct sockaddr_in addr = {};
            addr.sin_family = AF_INET;
            addr.sin_port = htons((u_short)port);
            addr.sin_addr.s_addr = inet_addr("127.0.0.1");
            u_long nonblock = 1;
            ioctlsocket(s, FIONBIO, &nonblock);
            connect(s, (struct sockaddr*)&addr, sizeof(addr));
            fd_set wfds;
            FD_ZERO(&wfds);
            FD_SET(s, &wfds);
            struct timeval tv = {1, 0};
            int sel = select(0, nullptr, &wfds, nullptr, &tv);
            closesocket(s);
            if (sel > 0) {
                port_ok = true;
                LOG_D("BOOT", "Gateway port %d: raw TCP reachable (WinHTTP intercepted by proxy)", port);
            }
        }
    }

    if (port_ok) {
        r.status = BootCheckStatus::Ok;
        r.message = "Port " + std::to_string(port) + " — OpenClaw gateway running";
        LOG_I("BOOT", "Gateway port %d: OpenClaw gateway active", port);
    } else {
        /* Port occupied by something else */
        r.status = BootCheckStatus::Error;
        r.message = "Port " + std::to_string(port) + " blocked by another process";
        r.auto_fix_available = true;
        LOG_W("BOOT", "Gateway port %d: BLOCKED by unknown process", port);
    }

    return r;
}

BootCheckResult check_config_dir() {
    BootCheckResult r;
    r.check_name = "Config Directory";

    const char* appdata = std::getenv("APPDATA");
    if (!appdata) {
        r.status = BootCheckStatus::Error;
        r.message = "APPDATA environment variable not set";
        LOG_W("BOOT", "Config dir: APPDATA not set");
        return r;
    }

    std::string config_dir = std::string(appdata) + "\\" + OPENCLAW_CONFIG_DIR;

    if (fs::exists(config_dir)) {
        /* Test writability */
        std::string test_file = config_dir + "\\.boot_check_test";
        std::ofstream f(test_file);
        if (f.is_open()) {
            f << "test";
            f.close();
            fs::remove(test_file);
            r.status = BootCheckStatus::Ok;
            r.message = "Config dir writable (" + config_dir + ")";
            LOG_I("BOOT", "Config dir: writable (%s)", config_dir.c_str());
        } else {
            r.status = BootCheckStatus::Error;
            r.message = "Config dir exists but not writable: " + config_dir;
            LOG_W("BOOT", "Config dir: NOT writable (%s)", config_dir.c_str());
        }
    } else {
        r.status = BootCheckStatus::Warn;
        r.message = "Config dir does not exist: " + config_dir;
        r.auto_fix_available = true;
        LOG_W("BOOT", "Config dir: does not exist (%s)", config_dir.c_str());
    }

    return r;
}

BootCheckResult check_workspace() {
    BootCheckResult r;
    r.check_name = "Workspace";

    std::string root = workspace_get_root();

    if (root.empty()) {
        r.status = BootCheckStatus::Error;
        r.message = "Workspace root not configured";
        r.auto_fix_available = true;
        LOG_W("BOOT", "Workspace: root not configured");
        return r;
    }

    if (!fs::exists(root)) {
        r.status = BootCheckStatus::Warn;
        r.message = "Workspace directory missing: " + root;
        r.auto_fix_available = true;
        LOG_W("BOOT", "Workspace: missing (%s)", root.c_str());
        return r;
    }

    /* Test writability */
    std::string test_file = root + "\\.boot_check_test";
    std::ofstream f(test_file);
    if (f.is_open()) {
        f << "test";
        f.close();
        fs::remove(test_file);
        r.status = BootCheckStatus::Ok;
        r.message = "Workspace OK (" + root + ")";
        LOG_I("BOOT", "Workspace: OK (%s)", root.c_str());
    } else {
        r.status = BootCheckStatus::Error;
        r.message = "Workspace not writable: " + root;
        LOG_W("BOOT", "Workspace: NOT writable (%s)", root.c_str());
    }

    return r;
}

BootCheckResult check_disk_space() {
    BootCheckResult r;
    r.check_name = "Disk Space";

    try {
        auto space = fs::space(fs::current_path());
        uint64_t free_mb = space.available / (1024 * 1024);

        if (free_mb >= 500) {
            r.status = BootCheckStatus::Ok;
            r.message = std::to_string(free_mb) + " MB free";
            LOG_I("BOOT", "Disk space: %llu MB free", (unsigned long long)free_mb);
        } else if (free_mb >= 100) {
            r.status = BootCheckStatus::Warn;
            r.message = "Low disk space: " + std::to_string(free_mb) + " MB free (recommend >= 500 MB)";
            LOG_W("BOOT", "Disk space: LOW — %llu MB free", (unsigned long long)free_mb);
        } else {
            r.status = BootCheckStatus::Error;
            r.message = "Critical disk space: " + std::to_string(free_mb) + " MB free";
            LOG_W("BOOT", "Disk space: CRITICAL — %llu MB free", (unsigned long long)free_mb);
        }
    } catch (const std::exception& e) {
        r.status = BootCheckStatus::Warn;
        r.message = std::string("Disk space check failed: ") + e.what();
        LOG_W("BOOT", "Disk space: exception — %s", e.what());
    }

    return r;
}

BootCheckResult check_network() {
    BootCheckResult r;
    r.check_name = "Network";

    /* First: check local OpenClaw gateway (always reliable if configured) */
    char resp_buf[256] = {0};
    int code = http_get("http://127.0.0.1:18789/health", resp_buf, sizeof(resp_buf), 3);
    if (code > 0) {
        r.status = BootCheckStatus::Ok;
        r.message = "Network OK (local gateway reachable)";
        LOG_I("BOOT", "Network: OK (gateway health OK)");
        return r;
    }

    /* Fallback: try OpenRouter */
    memset(resp_buf, 0, sizeof(resp_buf));
    code = http_get("https://openrouter.ai/api/v1/models", resp_buf, sizeof(resp_buf), 5);

    if (code > 0) {
        r.status = BootCheckStatus::Ok;
        r.message = "Network OK (openrouter.ai reachable)";
        LOG_I("BOOT", "Network: OK (openrouter.ai)");
        return r;
    }

    /* Fallback: try google.com */
    memset(resp_buf, 0, sizeof(resp_buf));
    code = http_get("https://www.google.com", resp_buf, sizeof(resp_buf), 5);

    if (code > 0) {
        r.status = BootCheckStatus::Warn;
        r.message = "Network OK but openrouter.ai unreachable";
        LOG_W("BOOT", "Network: openrouter.ai unreachable, general connectivity OK");
    } else {
        r.status = BootCheckStatus::Error;
        r.message = "Network unreachable — check internet connection";
        LOG_W("BOOT", "Network: UNREACHABLE");
    }

    return r;
}

BootCheckResult check_sdl2dll() {
    BootCheckResult r;
    r.check_name = "SDL2.dll";

    std::string exe_dir = get_exe_dir();
    std::string dll_path = exe_dir + "SDL2.dll";

    if (fs::exists(dll_path)) {
        r.status = BootCheckStatus::Ok;
        r.message = "SDL2.dll found";
        LOG_I("BOOT", "SDL2.dll: found at %s", dll_path.c_str());
    } else {
        r.status = BootCheckStatus::Error;
        r.message = "SDL2.dll missing from " + exe_dir;
        LOG_W("BOOT", "SDL2.dll: NOT FOUND at %s", dll_path.c_str());
    }

    return r;
}

BootCheckResult check_hermes() {
    BootCheckResult r;
    r.check_name = "Hermes";

    char version[64] = {0};
    bool installed = app_detect_hermes(version, sizeof(version));

    if (!installed) {
        r.status = BootCheckStatus::Warn;
        r.message = "Hermes not installed";
        LOG_W("BOOT", "Hermes: NOT FOUND");
        return r;
    }

    /* Hermes installed — check if gateway is running */
    bool healthy = app_check_hermes_health();
    if (healthy) {
        r.status = BootCheckStatus::Ok;
        r.message = std::string("Hermes ") + version + " (running)";
        LOG_I("BOOT", "Hermes: %s (running)", version);
    } else {
        r.status = BootCheckStatus::Warn;
        r.message = std::string("Hermes ") + version + " (gateway not responding)";
        LOG_W("BOOT", "Hermes: %s (gateway not responding)", version);
    }

    return r;
}

BootCheckResult check_claude() {
    BootCheckResult r;
    r.check_name = "Claude CLI";

    char version[64] = {0};
    bool installed = app_detect_claude_cli(version, sizeof(version));

    if (!installed) {
        r.status = BootCheckStatus::Warn;
        r.message = "Claude CLI not installed";
        LOG_W("BOOT", "Claude CLI: NOT FOUND");
        return r;
    }

    bool healthy = app_check_claude_cli_health();
    if (healthy) {
        r.status = BootCheckStatus::Ok;
        r.message = std::string("Claude CLI ") + version + " (running)";
        LOG_I("BOOT", "Claude CLI: %s (running)", version);
    } else {
        r.status = BootCheckStatus::Warn;
        r.message = std::string("Claude CLI ") + version + " (not running)";
        LOG_W("BOOT", "Claude CLI: %s (not running)", version);
    }

    return r;
}

BootCheckResult check_api_connectivity() {
    BootCheckResult r;
    r.check_name = "API Connectivity";

    /* Probe current model's API endpoint for connectivity.
     * Fallback order: api.minimaxi.com -> api.minimax.chat -> openrouter.ai.
     * If no model is configured, show N/A (not an error). */
    if (g_selected_model[0] == '\0' || g_api_key[0] == '\0') {
        r.status = BootCheckStatus::Warn;
        r.message = "No model configured (N/A)";
        LOG_W("BOOT", "API Connectivity: no model/key configured");
        return r;
    }

    /* Probe api.minimaxi.com (MiniMax international) */
    const char* endpoints[] = {
        "https://api.minimaxi.com/v1/models",
        "https://api.minimax.chat/v1/models",
        "https://openrouter.ai/api/v1/models"
    };
    const char* labels[] = {
        "MiniMax API (api.minimaxi.com)",
        "MiniMax API (api.minimax.chat)",
        "OpenRouter API"
    };

    char resp[256] = {0};
    int found = -1;
    for (int i = 0; i < 3; i++) {
        int code = http_get(endpoints[i], resp, sizeof(resp), 5);
        if (code > 0) {
            found = i;
            break;
        }
    }

    if (found < 0) {
        r.status = BootCheckStatus::Error;
        r.message = "API endpoints unreachable";
        LOG_W("BOOT", "API Connectivity: all endpoints failed");
    } else {
        r.status = BootCheckStatus::Ok;
        r.message = std::string(labels[found]) + " reachable";
        LOG_I("BOOT", "API Connectivity: %s OK", labels[found]);
    }

    return r;
}

/* ═══════════════════════════════════════════════════════════════
 *  BootCheckManager Implementation
 * ═══════════════════════════════════════════════════════════════ */

std::vector<BootCheckResult> BootCheckManager::run_all_checks() {
    std::vector<BootCheckResult> results;

    ui_log("[BootCheck] Running comprehensive health check...");
    LOG_I("BOOT", "Running comprehensive health check...");

    results.push_back(check_nodejs_version());
    results.push_back(check_npm());
    results.push_back(check_network());
    results.push_back(check_openclaw());
    results.push_back(check_hermes());
    results.push_back(check_claude());
    results.push_back(check_disk_space());
    results.push_back(check_api_connectivity());
    results.push_back(check_sdl2dll());
    /* 代码额外检测项（不计入 9 项，用于自动修复） */
    results.push_back(check_config_dir());
    results.push_back(check_workspace());

    ui_log("[BootCheck] %s", summarize(results).c_str());
    LOG_I("BOOT", "%s", summarize(results).c_str());

    return results;
}

bool BootCheckManager::auto_fix(BootCheckResult& result) {
    if (result.status == BootCheckStatus::Ok) return true;
    if (!result.auto_fix_available) return false;

    ui_log("[BootCheck] Auto-fixing: %s ...", result.check_name.c_str());
    LOG_I("BOOT", "Auto-fixing: %s", result.check_name.c_str());

    /* ── npm broken → cache clean ── */
    if (result.check_name == "npm") {
        if (!perm_check_exec(PermKey::EXEC_SHELL, "npm cache clean --force")) {
            ui_log("[BootCheck] npm fix skipped (permission denied)");
            LOG_W("BOOT", "npm fix skipped (permission denied)");
            result.fix_applied = false;
            return false;
        }

        std::string output;
        bool ok = boot_run_cmd("npm cache clean --force", output, 15000);

        if (ok) {
            /* Re-check */
            BootCheckResult fresh = check_npm();
            if (fresh.status == BootCheckStatus::Ok) {
                result = fresh;
                result.fix_applied = true;
                ui_log("[BootCheck] npm fixed after cache clean");
                LOG_I("BOOT", "npm fixed after cache clean");
                return true;
            }
        }

        result.fix_applied = false;
        ui_log("[BootCheck] npm fix failed");
        LOG_W("BOOT", "npm fix failed");
        return false;
    }

    /* ── Gateway port blocked → try to kill blocking process ── */
    if (result.check_name == "Gateway Port") {
        if (!perm_check_exec(PermKey::EXEC_SHELL, "netstat -ano")) {
            ui_log("[BootCheck] Port fix skipped (permission denied)");
            LOG_W("BOOT", "Port fix skipped (permission denied)");
            result.fix_applied = false;
            return false;
        }

        /* Find PID occupying the port */
        std::string cmd = "netstat -ano | findstr \":"
                        + std::to_string(GATEWAY_PORT) + "\" | findstr \"LISTENING\"";
        std::string output;
        if (boot_run_cmd(cmd.c_str(), output)) {
            /* Parse PID from last column of netstat output */
            std::istringstream iss(output);
            std::string line;
            DWORD pid = 0;
            while (std::getline(iss, line)) {
                /* Skip empty lines */
                if (line.empty()) continue;
                /* Find last number (PID) */
                auto pos = line.find_last_of("0123456789");
                if (pos != std::string::npos) {
                    auto start = line.find_last_not_of("0123456789", pos);
                    std::string pid_str = (start == std::string::npos)
                        ? line.substr(0, pos + 1)
                        : line.substr(start + 1, pos - start);
                    try { pid = (DWORD)std::stoul(pid_str); } catch (...) { pid = 0; }
                }
            }

            if (pid > 0 && pid != GetCurrentProcessId()) {
                /* Safety: never kill node.exe — it is likely the OpenClaw gateway.
                 * WinHTTP health check may fail due to local proxy interception. */
                bool is_node = false;
                {
                    HANDLE hQry = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
                    if (hQry) {
                        char exePath[MAX_PATH] = {};
                        DWORD nameLen = MAX_PATH;
                        if (QueryFullProcessImageNameA(hQry, 0, exePath, &nameLen)) {
                            const char* base = strrchr(exePath, '\\');
                            if (base && _stricmp(base + 1, "node.exe") == 0) is_node = true;
                        }
                        CloseHandle(hQry);
                    }
                }
                if (is_node) {
                    LOG_I("BOOT", "Port %d: node.exe PID %lu — assumed OpenClaw gateway, skipping kill", GATEWAY_PORT, (unsigned long)pid);
                    result.status = BootCheckStatus::Ok;
                    result.message = "Port " + std::to_string(GATEWAY_PORT) + " — node.exe (OpenClaw gateway)";
                    result.fix_applied = false;
                    return true;
                }
                HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
                if (hProc) {
                    TerminateProcess(hProc, 1);
                    CloseHandle(hProc);
                    Sleep(1000);

                    /* Re-check */
                    if (!is_port_in_use(GATEWAY_PORT)) {
                        BootCheckResult fresh = check_gateway_port();
                        result = fresh;
                        result.fix_applied = true;
                        ui_log("[BootCheck] Port %d freed (killed PID %lu)", GATEWAY_PORT, (unsigned long)pid);
                        LOG_I("BOOT", "Port %d freed (killed PID %lu)", GATEWAY_PORT, (unsigned long)pid);
                        return true;
                    }
                }
            }
        }

        result.fix_applied = false;
        ui_log("[BootCheck] Port fix failed — could not identify/free blocking process");
        LOG_W("BOOT", "Port fix failed");
        return false;
    }

    /* ── Config dir missing → create it ── */
    if (result.check_name == "Config Directory") {
        const char* appdata = std::getenv("APPDATA");
        if (!appdata) {
            result.fix_applied = false;
            return false;
        }

        std::string config_dir = std::string(appdata) + "\\" + OPENCLAW_CONFIG_DIR;
        try {
            if (fs::create_directories(config_dir)) {
                BootCheckResult fresh = check_config_dir();
                if (fresh.status == BootCheckStatus::Ok) {
                    result = fresh;
                    result.fix_applied = true;
                    ui_log("[BootCheck] Config dir created: %s", config_dir.c_str());
                    LOG_I("BOOT", "Config dir created: %s", config_dir.c_str());
                    return true;
                }
            }
        } catch (const std::exception& e) {
            ui_log("[BootCheck] Config dir create failed: %s", e.what());
            LOG_W("BOOT", "Config dir create failed: %s", e.what());
        }

        result.fix_applied = false;
        return false;
    }

    /* ── Workspace missing → initialize ── */
    if (result.check_name == "Workspace") {
        std::string root = workspace_get_root();
        if (root.empty()) {
            result.fix_applied = false;
            return false;
        }

        try {
            if (workspace_init(root.c_str())) {
                BootCheckResult fresh = check_workspace();
                result = fresh;
                result.fix_applied = true;
                ui_log("[BootCheck] Workspace initialized: %s", root.c_str());
                LOG_I("BOOT", "Workspace initialized: %s", root.c_str());
                return true;
            }
        } catch (const std::exception& e) {
            ui_log("[BootCheck] Workspace init failed: %s", e.what());
            LOG_W("BOOT", "Workspace init failed: %s", e.what());
        }

        result.fix_applied = false;
        return false;
    }

    result.fix_applied = false;
    return false;
}

bool BootCheckManager::auto_fix_all(std::vector<BootCheckResult>& results) {
    bool all_fixed = true;

    for (auto& r : results) {
        if (r.status != BootCheckStatus::Ok && r.auto_fix_available) {
            if (!auto_fix(r)) {
                all_fixed = false;
            }
        }
    }

    return all_fixed;
}

std::vector<BootCheckResult> BootCheckManager::run_and_fix() {
    auto results = run_all_checks();

    bool needs_fix = false;
    for (const auto& r : results) {
        if (r.status != BootCheckStatus::Ok && r.auto_fix_available) {
            needs_fix = true;
            break;
        }
    }

    if (needs_fix) {
        ui_log("[BootCheck] Attempting auto-repair...");
        LOG_I("BOOT", "Attempting auto-repair...");
        auto_fix_all(results);
    }

    return results;
}

std::string BootCheckManager::summarize(const std::vector<BootCheckResult>& results) {
    int ok_count = 0, warn_count = 0, error_count = 0;
    int fixed_count = 0;

    for (const auto& r : results) {
        switch (r.status) {
            case BootCheckStatus::Ok:    ok_count++;    break;
            case BootCheckStatus::Warn:  warn_count++;  break;
            case BootCheckStatus::Error: error_count++; break;
        }
        if (r.fix_applied) fixed_count++;
    }

    char buf[256];
    snprintf(buf, sizeof(buf),
             "Health check: %d OK, %d warnings, %d errors (auto-fixed: %d)",
             ok_count, warn_count, error_count, fixed_count);
    return std::string(buf);
}
