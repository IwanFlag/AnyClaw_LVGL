#include "runtime_mgr.h"
#include "app.h"
#include "app_log.h"
#include "tracing.h"
#include "permissions.h"
#include "json_util.h"
#include <fstream>
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <string>
#include <filesystem>

/* ═══════════════════════════════════════════════════════════════════
 * GitHub Release URLs for bundled install scripts
 * 上传至: https://github.com/IwanFlag/AnyClaw_Tools/releases/tag/v1.0.0
 * ═══════════════════════════════════════════════════════════════════ */
static const char* HERMES_INSTALL_URL =
    "https://github.com/IwanFlag/AnyClaw_Tools/releases/download/v1.0.0/install-hermes.ps1";
static const char* CLAUDE_DOWNLOAD_URL =
    "https://github.com/IwanFlag/AnyClaw_Tools/releases/download/v1.0.0/download-claude.ps1";

/* Bundled script paths (relative to exe directory) */
static void get_bundled_dir(char* out, int size) {
    char exe_path[MAX_PATH] = {0};
    GetModuleFileNameA(NULL, exe_path, sizeof(exe_path));
    char* last_bslash = strrchr(exe_path, '\\');
    if (last_bslash) *last_bslash = '\0';
    snprintf(out, size, "%s\\bundled", exe_path);
}

/* ═══════════════════════════════════════════════════════════════════
 * HTTP 下载文件到目标路径
 * ═══════════════════════════════════════════════════════════════════ */
static bool download_file(const char* url, const char* dest_path, DWORD timeout_ms = 300000) {
    /* curl 优先，带 resume 支持 */
    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
        "curl -L -f -C - --retry 3 --retry-delay 5 "
        "--max-time %lu -o \"%s\" \"%s\"",
        timeout_ms / 1000, dest_path, url);

    SECURITY_ATTRIBUTES sa = {};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE hRead = nullptr, hWrite = nullptr;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) return false;
    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;

    PROCESS_INFORMATION pi = {};
    if (!CreateProcessA(NULL, cmd, NULL, NULL, TRUE,
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(hRead);
        CloseHandle(hWrite);
        /* Fallback: PowerShell */
        snprintf(cmd, sizeof(cmd),
            "powershell -ExecutionPolicy Bypass -Command "
            "\"& { $ProgressPreference='SilentlyContinue'; "
            "Invoke-WebRequest -Uri '%s' -OutFile '%s' -TimeoutSec %lu }\"",
            url, dest_path, timeout_ms / 1000);
        if (!CreateProcessA(NULL, cmd, NULL, NULL, TRUE,
                            CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            CloseHandle(hRead);
            CloseHandle(hWrite);
            return false;
        }
    }

    CloseHandle(hWrite);
    char buf[1024];
    DWORD total = 0;
    DWORD start = GetTickCount();
    while (total < sizeof(buf) - 1) {
        DWORD bytes = 0;
        if (!ReadFile(hRead, buf + total, sizeof(buf) - 1 - total, &bytes, NULL) || bytes == 0) break;
        total += bytes;
        if (GetTickCount() - start > timeout_ms) {
            TerminateProcess(pi.hProcess, 1);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            CloseHandle(hRead);
            return false;
        }
    }
    buf[total] = '\0';
    CloseHandle(hRead);

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exit_code = 0;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return (exit_code == 0);
}

/* ═══════════════════════════════════════════════════════════════════
 * 执行 PowerShell 脚本（文件路径），等待完成
 * ═══════════════════════════════════════════════════════════════════ */
static bool exec_ps_script(const char* script_path, DWORD timeout_ms = 600000) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
        "powershell -ExecutionPolicy Bypass -NoProfile -File \"%s\"",
        script_path);

    SECURITY_ATTRIBUTES sa = {};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE hRead = nullptr, hWrite = nullptr;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) return false;
    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;

    PROCESS_INFORMATION pi = {};
    if (!CreateProcessA(NULL, cmd, NULL, NULL, TRUE,
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(hRead);
        CloseHandle(hWrite);
        return false;
    }
    CloseHandle(hWrite);

    char buf[4096];
    DWORD total = 0;
    DWORD start = GetTickCount();
    while (total < sizeof(buf) - 1) {
        if (GetTickCount() - start > timeout_ms) {
            TerminateProcess(pi.hProcess, 1);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            CloseHandle(hRead);
            LOG_W("RUNTIME", "exec_ps_script timeout: %s", script_path);
            return false;
        }
        DWORD avail = 0;
        if (!PeekNamedPipe(hRead, NULL, 0, NULL, &avail, NULL) || avail == 0) {
            if (WaitForSingleObject(pi.hProcess, 500) == WAIT_OBJECT_0) break;
            continue;
        }
        DWORD bytes = 0;
        if (!ReadFile(hRead, buf + total, sizeof(buf) - 1 - total, &bytes, NULL) || bytes == 0) break;
        total += bytes;
    }
    buf[total] = '\0';
    LOG_D("RUNTIME", "ps_script output: %.256s", buf);
    CloseHandle(hRead);

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exit_code = 0;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return (exit_code == 0);
}

/* ═══════════════════════════════════════════════════════════════════
 * Hermes 辅助：查找 hermes.exe 路径
 * ═══════════════════════════════════════════════════════════════════ */
static bool find_hermes_exe(char* path_out, int path_size) {
    /* 检查已知路径 */
    const char* known_paths[] = {
        "%LOCALAPPDATA%\\hermes\\hermes-agent\\venv\\Scripts\\hermes.exe",
        "%USERPROFILE%\\.local\\bin\\hermes.exe",
        NULL
    };
    for (int i = 0; known_paths[i]; i++) {
        char expanded[MAX_PATH];
        ExpandEnvironmentStringsA(known_paths[i], expanded, sizeof(expanded));
        if (GetFileAttributesA(expanded) != INVALID_FILE_ATTRIBUTES) {
            snprintf(path_out, path_size, "%s", expanded);
            return true;
        }
    }
    /* PATH 搜索 */
    char buf[8192] = {0};
    if (!GetEnvironmentVariableA("PATH", buf, sizeof(buf))) return false;
    char* ctx = NULL;
    char* dir = strtok_s(buf, ";", &ctx);
    while (dir) {
        char exe_path[MAX_PATH];
        snprintf(exe_path, sizeof(exe_path), "%s\\hermes.exe", dir);
        if (GetFileAttributesA(exe_path) != INVALID_FILE_ATTRIBUTES) {
            snprintf(path_out, path_size, "%s", exe_path);
            return true;
        }
        dir = strtok_s(NULL, ";", &ctx);
    }
    return false;
}

/* ═══════════════════════════════════════════════════════════════════
 * Claude CLI 辅助：查找 claude.exe 路径
 * ═══════════════════════════════════════════════════════════════════ */
static bool find_claude_exe(char* path_out, int path_size) {
    const char* known_paths[] = {
        "%LOCALAPPDATA%\\Programs\\Claude\\claude.exe",
        "%ProgramFiles%\\Claude\\claude.exe",
        "%USERPROFILE%\\.claude\\bin\\claude.exe",
        NULL
    };
    for (int i = 0; known_paths[i]; i++) {
        char expanded[MAX_PATH];
        ExpandEnvironmentStringsA(known_paths[i], expanded, sizeof(expanded));
        if (GetFileAttributesA(expanded) != INVALID_FILE_ATTRIBUTES) {
            snprintf(path_out, path_size, "%s", expanded);
            return true;
        }
    }
    char buf[8192] = {0};
    if (!GetEnvironmentVariableA("PATH", buf, sizeof(buf))) return false;
    char* ctx = NULL;
    char* dir = strtok_s(buf, ";", &ctx);
    while (dir) {
        char exe_path[MAX_PATH];
        snprintf(exe_path, sizeof(exe_path), "%s\\claude.exe", dir);
        if (GetFileAttributesA(exe_path) != INVALID_FILE_ATTRIBUTES) {
            snprintf(path_out, path_size, "%s", exe_path);
            return true;
        }
        dir = strtok_s(NULL, ";", &ctx);
    }
    return false;
}

/* ═══════════════════════════════════════════════════════════════════
 * Hermes 检测
 * ═══════════════════════════════════════════════════════════════════ */
bool app_detect_hermes(char* version_out, int ver_size) {
    if (version_out && ver_size > 0) version_out[0] = '\0';
    char exe_path[MAX_PATH] = {0};
    if (!find_hermes_exe(exe_path, sizeof(exe_path))) return false;

    if (version_out) {
        char output[256] = {0};
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "\"%s\" --version", exe_path);
        FILE* fp = _popen(cmd, "r");
        if (fp) {
            if (fgets(output, sizeof(output), fp)) {
                /* 去掉换行 */
                char* nl = strrchr(output, '\n');
                if (nl) *nl = '\0';
                if (nl) { nl = strrchr(output, '\r'); if (nl) *nl = '\0'; }
                snprintf(version_out, ver_size, "%.63s", output);
            }
            _pclose(fp);
        }
    }
    return true;
}

bool app_check_hermes_health(void) {
    char resp[128] = {0};
    int code = http_get("http://127.0.0.1:18790/health", resp, sizeof(resp), 3);
    return (code > 0 && code < 500);
}

/* ═══════════════════════════════════════════════════════════════════
 * Claude CLI 检测
 * ═══════════════════════════════════════════════════════════════════ */
bool app_detect_claude_cli(char* version_out, int ver_size) {
    if (version_out && ver_size > 0) version_out[0] = '\0';
    char exe_path[MAX_PATH] = {0};
    if (!find_claude_exe(exe_path, sizeof(exe_path))) return false;

    if (version_out) {
        char output[256] = {0};
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "\"%s\" --version", exe_path);
        FILE* fp = _popen(cmd, "r");
        if (fp) {
            if (fgets(output, sizeof(output), fp)) {
                char* nl = strrchr(output, '\n');
                if (nl) *nl = '\0';
                nl = strrchr(output, '\r');
                if (nl) *nl = '\0';
                snprintf(version_out, ver_size, "%.63s", output);
            }
            _pclose(fp);
        }
    }
    return true;
}

bool app_check_claude_cli_health(void) {
    char exe_path[MAX_PATH] = {0};
    if (!find_claude_exe(exe_path, sizeof(exe_path))) return false;
    /* 进程存活检查：尝试 --version（快速，轻量） */
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "\"%s\" --version", exe_path);
    FILE* fp = _popen(cmd, "r");
    if (!fp) return false;
    char buf[64] = {0};
    bool ok = (fgets(buf, sizeof(buf), fp) != nullptr);
    _pclose(fp);
    return ok;
}

/* 快速文件存在检查 — 主线程安全，无子进程 */
bool app_check_claude_cli_exists(void) {
    char exe_path[MAX_PATH] = {0};
    return find_claude_exe(exe_path, sizeof(exe_path));
}

/* ═══════════════════════════════════════════════════════════════════
 * Hermes 安装
 * ═══════════════════════════════════════════════════════════════════ */
bool app_install_hermes(const char* mode,
                        void (*progress_cb)(const char* step, int pct, void* ctx),
                        void* ctx) {
    TraceSpan span("hermes_install", mode ? mode : "auto");
    bool is_auto  = strcmp(mode, "auto")   == 0 || mode == nullptr;
    bool is_local = strcmp(mode, "local")  == 0;
    bool is_net   = strcmp(mode, "network") == 0;

    /* 快速路径：已安装则跳过 */
    if (app_detect_hermes(NULL, 0)) {
        if (progress_cb) progress_cb("Hermes already installed", 100, ctx);
        LOG_I("RUNTIME", "Hermes already installed, skipping install");
        return true;
    }

    char bundled_dir[MAX_PATH];
    get_bundled_dir(bundled_dir, sizeof(bundled_dir));

    /* ── local 模式 ── */
    if (is_local) {
        char script_path[MAX_PATH];
        snprintf(script_path, sizeof(script_path), "%s\\hermes\\install-hermes.ps1", bundled_dir);
        if (GetFileAttributesA(script_path) != INVALID_FILE_ATTRIBUTES) {
            if (progress_cb) progress_cb("Installing Hermes (local)...", 20, ctx);
            bool ok = exec_ps_script(script_path);
            if (progress_cb) progress_cb(ok ? "Hermes installed" : "Hermes install failed", ok ? 100 : 0, ctx);
            if (ok) LOG_I("RUNTIME", "Hermes installed from bundled script");
            return ok;
        }
        LOG_W("RUNTIME", "Local Hermes script not found: %s", script_path);
        span.set_fail();
        return false;
    }

    /* ── network 模式（以及 auto 的第一阶段）── */
    if (is_net || is_auto) {
        /* 下载安装脚本到临时目录 */
        char tmp_script[MAX_PATH];
        snprintf(tmp_script, sizeof(tmp_script), "%s\\hermes_install.ps1", getenv("TEMP") ? getenv("TEMP") : "C:\\Temp");

        /* 确保目录存在 */
        char* tmp_dir = getenv("TEMP");
        if (tmp_dir) {
            snprintf(tmp_script, sizeof(tmp_script), "%s\\hermes_install.ps1", tmp_dir);
            if (progress_cb) progress_cb("Downloading Hermes install script...", 10, ctx);
            if (download_file(HERMES_INSTALL_URL, tmp_script)) {
                if (progress_cb) progress_cb("Running Hermes install script...", 40, ctx);
                bool ok = exec_ps_script(tmp_script);
                /* 清理 */
                DeleteFileA(tmp_script);
                if (ok) {
                    if (progress_cb) progress_cb("Hermes installed", 100, ctx);
                    LOG_I("RUNTIME", "Hermes installed from GitHub: %s", HERMES_INSTALL_URL);
                    return true;
                }
            }
        }

        if (is_net) {
            /* network 模式不允许降级到 local */
            LOG_E("RUNTIME", "Hermes network install failed");
            span.set_fail();
            return false;
        }

        /* auto 降级到 local */
        LOG_W("RUNTIME", "Hermes network install failed, trying bundled...");
    }

    /* ── auto 降级到 local ── */
    char script_path[MAX_PATH];
    snprintf(script_path, sizeof(script_path), "%s\\hermes\\install-hermes.ps1", bundled_dir);
    if (GetFileAttributesA(script_path) != INVALID_FILE_ATTRIBUTES) {
        if (progress_cb) progress_cb("Installing Hermes (bundled)...", 20, ctx);
        bool ok = exec_ps_script(script_path);
        if (progress_cb) progress_cb(ok ? "Hermes installed" : "Hermes install failed", ok ? 100 : 0, ctx);
        if (ok) {
            LOG_I("RUNTIME", "Hermes installed from bundled script");
            return true;
        }
    }

    LOG_E("RUNTIME", "Hermes install failed (auto tried network then bundled)");
    span.set_fail();
    return false;
}

/* ═══════════════════════════════════════════════════════════════════
 * Claude CLI 安装
 * ═══════════════════════════════════════════════════════════════════ */
bool app_install_claude_cli(const char* mode,
                             void (*progress_cb)(const char* step, int pct, void* ctx),
                             void* ctx) {
    TraceSpan span("claude_install", mode ? mode : "auto");
    bool is_auto  = strcmp(mode, "auto")   == 0 || mode == nullptr;
    bool is_local = strcmp(mode, "local")  == 0;
    bool is_net   = strcmp(mode, "network") == 0;

    /* 快速路径：已安装则跳过 */
    if (app_detect_claude_cli(NULL, 0)) {
        if (progress_cb) progress_cb("Claude CLI already installed", 100, ctx);
        LOG_I("RUNTIME", "Claude CLI already installed, skipping install");
        return true;
    }

    char bundled_dir[MAX_PATH];
    get_bundled_dir(bundled_dir, sizeof(bundled_dir));

    /* ── local 模式 ── */
    if (is_local) {
        if (progress_cb) progress_cb("Claude CLI local install: provide MSI manually", 0, ctx);
        LOG_W("RUNTIME", "Claude CLI local install requires manual MSI");
        span.set_fail();
        return false;
    }

    /* ── network 模式（以及 auto 的第一阶段）── */
    if (is_net || is_auto) {
        char tmp_script[MAX_PATH];
        char* tmp_dir = getenv("TEMP");
        if (tmp_dir) {
            snprintf(tmp_script, sizeof(tmp_script), "%s\\download-claude.ps1", tmp_dir);
            if (progress_cb) progress_cb("Downloading Claude CLI install script...", 10, ctx);
            if (download_file(CLAUDE_DOWNLOAD_URL, tmp_script)) {
                if (progress_cb) progress_cb("Running Claude CLI install...", 30, ctx);
                bool ok = exec_ps_script(tmp_script);
                DeleteFileA(tmp_script);
                if (ok) {
                    if (progress_cb) progress_cb("Claude CLI installed", 100, ctx);
                    LOG_I("RUNTIME", "Claude CLI installed");
                    return true;
                }
            }
        }

        if (is_net) {
            LOG_E("RUNTIME", "Claude CLI network install failed");
            span.set_fail();
            return false;
        }

        LOG_W("RUNTIME", "Claude CLI network install failed");
    }

    LOG_W("RUNTIME", "Claude CLI install: no offline package available");
    span.set_fail();
    return false;
}

/* ═══════════════════════════════════════════════════════════════════
 * Hermes 完整初始化（启动 gateway）
 * ═══════════════════════════════════════════════════════════════════ */
bool app_init_hermes(void) {
    TraceSpan span("hermes_init");
    char exe_path[MAX_PATH] = {0};
    if (!find_hermes_exe(exe_path, sizeof(exe_path))) {
        LOG_E("RUNTIME", "Hermes exe not found for init");
        span.set_fail();
        return false;
    }

    /* hermes gateway start */
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "\"%s\" gateway start", exe_path);
    FILE* fp = _popen(cmd, "r");
    if (fp) {
        char buf[256] = {0};
        if (fgets(buf, sizeof(buf), fp)) LOG_I("RUNTIME", "Hermes gateway start: %.128s", buf);
        _pclose(fp);
    }

    /* 等待健康检查 */
    for (int i = 0; i < 10; i++) {
        if (app_check_hermes_health()) {
            LOG_I("RUNTIME", "Hermes gateway is healthy");
            return true;
        }
        Sleep(1000);
    }

    LOG_W("RUNTIME", "Hermes gateway health check failed after init");
    return false;  /* 不算致命，gateway 可能在别处启动 */
}

/* ═══════════════════════════════════════════════════════════════════
 * 全量检测结果（供 openclaw_mgr.cpp 填充）
 * ═══════════════════════════════════════════════════════════════════ */
static HermesCheckResult g_hermes_result = {};
static ClaudeCheckResult g_claude_result = {};

const HermesCheckResult* app_get_hermes_result(void) { return &g_hermes_result; }
const ClaudeCheckResult* app_get_claude_result(void) { return &g_claude_result; }

/* 由 app_check_environment() 调用以填充结果 */
void runtime_refresh_results(void) {
    g_hermes_result.installed = app_detect_hermes(g_hermes_result.version, sizeof(g_hermes_result.version));
    g_hermes_result.healthy = app_check_hermes_health();

    g_claude_result.installed = app_detect_claude_cli(g_claude_result.version, sizeof(g_claude_result.version));
    g_claude_result.healthy = app_check_claude_cli_health();

    LOG_I("RUNTIME", "Hermes: installed=%d healthy=%d v=%s",
          g_hermes_result.installed, g_hermes_result.healthy, g_hermes_result.version);
    LOG_I("RUNTIME", "Claude:  installed=%d healthy=%d v=%s",
          g_claude_result.installed, g_claude_result.healthy, g_claude_result.version);
}

/* ═══════════════════════════════════════════════════════════════════
 * Active runtime persistence (config.json)
 * ═══════════════════════════════════════════════════════════════════ */
static std::string get_config_path() {
    const char* appdata = std::getenv("APPDATA");
    if (!appdata) return "";
    return std::string(appdata) + "\\AnyClaw_LVGL\\config.json";
}

static bool json_update_string(const char* path, const char* key, const char* new_value) {
    std::ifstream in(path);
    if (!in.is_open()) return false;
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    char search[256];
    snprintf(search, sizeof(search), "\"%s\"", key);
    std::string insert = search + std::string(" : \"") + new_value + "\"";

    std::string result = content;
    std::string::size_type pos = result.find(search);
    if (pos != std::string::npos) {
        /* Find the colon and replace the whole "key" : "value" */
        std::string::size_type colon = result.find(':', pos);
        if (colon != std::string::npos) {
            std::string::size_type end = result.find('"', colon + 1);
            if (end != std::string::npos) {
                std::string::size_type start_quote = colon + 1;
                while (start_quote < end && (result[start_quote] == ' ' || result[start_quote] == '\t')) start_quote++;
                result.erase(start_quote, end - start_quote + 1);
                result.insert(start_quote, std::string("\"") + new_value + "\"");
            }
        }
    } else {
        /* Key not found — insert before the closing brace if possible */
        std::string::size_type last_brace = result.rfind('}');
        if (last_brace != std::string::npos) {
            while (last_brace > 0 && (result[last_brace - 1] == ' ' || result[last_brace - 1] == '\t' || result[last_brace - 1] == '\n' || result[last_brace - 1] == '\r')) last_brace--;
            result.insert(last_brace, insert.c_str());
        }
    }

    std::ofstream out(path);
    if (!out.is_open()) return false;
    out << result;
    out.close();
    return true;
}

Runtime app_get_active_runtime() {
    char val[32] = {0};
    std::string path = get_config_path();
    if (!path.empty()) {
        std::ifstream in(path);
        if (in.is_open()) {
            std::string json_content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            in.close();
            if (json_extract_string(json_content.c_str(), "active_runtime", val, sizeof(val))) {
                if (strcmp(val, "hermes") == 0) return Runtime::Hermes;
                if (strcmp(val, "claude") == 0) return Runtime::Claude;
            }
        }
    }
    return Runtime::OpenClaw; /* default */
}

void app_set_active_runtime(Runtime r) {
    const char* val = (r == Runtime::Hermes) ? "hermes"
                      : (r == Runtime::Claude) ? "claude"
                      : "openclaw";
    std::string path = get_config_path();
    if (!path.empty()) {
        json_update_string(path.c_str(), "active_runtime", val);
        LOG_I("RUNTIME", "active_runtime saved: %s", val);
    }
}
