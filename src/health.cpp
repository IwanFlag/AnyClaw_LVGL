#include "health.h"
#include "app.h"
#include "app_config.h"
#include "app_log.h"
#include "session_manager.h"
#include "permissions.h"
#include "tracing.h"
#include <windows.h>
#include <tlhelp32.h>
#include <process.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

static HANDLE g_hThread = nullptr;
static volatile bool g_running = false;
static HealthStatusCallback g_callback = nullptr;
static int g_httpFailCount = 0;
static bool g_autoRestarted = false;
static bool g_firstPoll = true;  /* First poll → Checking state */
static volatile LONG g_last_status_valid = 0;
static volatile LONG g_last_status = (LONG)ClawStatus::Checking;
extern int g_refresh_interval_ms;  /* Configurable check interval from ui_main.cpp */

/* Check if node.exe process exists */
static bool is_node_running() {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32W pe = {};
    pe.dwSize = sizeof(pe);
    bool found = false;

    if (Process32FirstW(hSnap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, L"node.exe") == 0) {
                found = true;
                break;
            }
        } while (Process32NextW(hSnap, &pe));
    }

    CloseHandle(hSnap);
    return found;
}

/* Check HTTP health endpoint — dynamically resolves port from user config */
static bool check_http_health() {
    char url[128] = {0};
    /* Try to read user gateway port from ~/.openclaw/openclaw.json */
    int port = 0;
    {
        const char* userprofile = std::getenv("USERPROFILE");
        if (userprofile) {
            std::string path = std::string(userprofile) + "\\.openclaw\\openclaw.json";
            FILE* fp = fopen(path.c_str(), "r");
            if (fp) {
                char buf[4096] = {0};
                fread(buf, 1, sizeof(buf)-1, fp);
                fclose(fp);
                /* Look for "localhost:PORT" or "127.0.0.1:PORT" patterns */
                static const char* pats[] = { "localhost:", "127.0.0.1:", nullptr };
                for (int i = 0; pats[i] && !port; i++) {
                    const char* p = strstr(buf, pats[i]);
                    if (!p) continue;
                    p += strlen(pats[i]);
                    while (*p >= '0' && *p <= '9') {
                        port = port * 10 + (*p++ - '0');
                    }
                    if (port <= 1024 || port >= 65536) port = 0;
                }
            }
        }
    }
    if (port <= 0) port = GATEWAY_PORT;  /* fallback */
    snprintf(url, sizeof(url), "http://127.0.0.1:%d/health", port);
    char response[256] = {0};
    int code = http_get(url, response, sizeof(response), HEALTH_HTTP_TIMEOUT);
    return (code == 200);
}

static bool sleep_interruptible(int total_ms) {
    const int slice_ms = 200;
    int left = total_ms;
    while (left > 0 && g_running) {
        int step = (left < slice_ms) ? left : slice_ms;
        Sleep(step);
        left -= step;
    }
    return g_running;
}

/*
 * Execute a CLI command and capture stdout.
 * Returns true if exit code == 0.
 */
static bool exec_cmd_local(const char* cmd, char* output, int out_size, DWORD timeout_ms = 8000) {
    if (!perm_check_exec(PermKey::EXEC_SHELL, cmd)) {
        if (output && out_size > 0) snprintf(output, out_size, "DENY: exec_shell blocked or rejected");
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

    if (!CreateProcessA(nullptr, buf.data(), nullptr, nullptr, TRUE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        CloseHandle(hRead);
        CloseHandle(hWrite);
        return false;
    }

    CloseHandle(hWrite);

    std::string result;
    char readBuf[4096];
    DWORD bytesRead = 0;
    while (ReadFile(hRead, readBuf, sizeof(readBuf), &bytesRead, nullptr) && bytesRead > 0) {
        result.append(readBuf, bytesRead);
    }

    WaitForSingleObject(pi.hProcess, timeout_ms);
    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hRead);

    /* Trim trailing whitespace */
    while (!result.empty() && (result.back() == '\r' || result.back() == '\n' || result.back() == ' '))
        result.pop_back();

    strncpy(output, result.c_str(), out_size - 1);
    output[out_size - 1] = '\0';
    return exitCode == 0;
}

static int g_active_session_count = 0;  /* number of active sessions */
static char g_active_session_info[256] = {0};  /* formatted session summary for UI */

/*
 * Count active sessions via SessionManager.
 * Calls SessionManager::refresh() which internally calls sessions.list --json.
 * Updates g_active_session_count and g_active_session_info.
 */
static int count_active_sessions() {
    SessionManager& sm = session_mgr();
    if (!sm.refresh()) {
        LOG_W("HEALTH", "Session refresh failed: %s", sm.last_error());
        return 0;
    }

    g_active_session_count = sm.active_count(SESSION_ACTIVE_AGE_MS);

    /* Format info string: "channel:age_s, channel:age_s" */
    std::string info = sm.format_active_info(SESSION_ACTIVE_AGE_MS);
    snprintf(g_active_session_info, sizeof(g_active_session_info), "%s", info.c_str());

    LOG_D("HEALTH", "Active sessions: %d (%s)", g_active_session_count, g_active_session_info);
    return g_active_session_count;
}

/* Backward-compatible wrapper */
static bool has_active_sessions() {
    return count_active_sessions() > 0;
}

int health_get_session_count() {
    return g_active_session_count;
}

const char* health_get_session_info() {
    return g_active_session_info;
}

/* Health monitoring thread */
static unsigned __stdcall health_thread(void* arg) {
    (void)arg;
    LOG_I("HEALTH", "Monitoring thread started (interval=%dms)", g_refresh_interval_ms);

    bool first_iteration = true;
    while (g_running) {
        /* Quick initial check (2s) then normal interval */
        int sleep_ms = first_iteration ? 2000 : g_refresh_interval_ms;
        first_iteration = false;
        if (!sleep_interruptible(sleep_ms)) break;

        TraceSpan span("health_check_cycle");
        DWORD tick_start = GetTickCount();
        bool nodeRunning = is_node_running();
        bool httpOk = false;
        if (nodeRunning) {
            httpOk = check_http_health();
        }
        DWORD check_ms = GetTickCount() - tick_start;

        LOG_D("HEALTH", "Check: node=%d http=%d elapsed=%lums", nodeRunning, httpOk, check_ms);

        ClawStatus newStatus;
        std::string reason;

        if (!nodeRunning && !httpOk) {
            /* Gateway process not found + HTTP down */
            if (!g_autoRestarted) {
                /* Try auto-restart once */
                g_autoRestarted = true;
                LOG_W("HEALTH", "Node not found, attempting auto-restart...");
                reason = "Gateway 未运行";
                if (app_start_gateway()) {
                    LOG_I("HEALTH", "Auto-restart initiated");
                    newStatus = ClawStatus::Checking;
                } else {
                    LOG_E("HEALTH", "Auto-restart failed");
                    newStatus = ClawStatus::Error;
                    reason = "Gateway 启动失败";
                }
            } else {
                newStatus = ClawStatus::Error;
                reason = "Gateway 未运行";
            }
        } else if (nodeRunning && !httpOk) {
            /* Process exists but HTTP not responding */
            g_httpFailCount++;
            LOG_W("HEALTH", "HTTP check failed (%d/3)", g_httpFailCount);
            if (g_httpFailCount >= HEALTH_FAIL_THRESHOLD) {
                newStatus = ClawStatus::Error;
                reason = "端口无响应";
                g_autoRestarted = false;  /* Allow retry next time process dies */
            } else {
                newStatus = ClawStatus::Checking;  /* Give it time */
                reason = "等待 Gateway 响应...";
            }
        } else if (httpOk) {
            /* Gateway process + HTTP both OK → check sessions for busy/idle */
            g_httpFailCount = 0;
            g_autoRestarted = false;
            g_firstPoll = false;

            /* Count active sessions for task display */
            g_active_session_count = count_active_sessions();

            if (g_active_session_count > 0) {
                newStatus = ClawStatus::Busy;
            } else {
                newStatus = ClawStatus::Ready;
            }
        } else {
            /* HTTP works but process not found (unlikely) */
            newStatus = ClawStatus::Ready;
            g_httpFailCount = 0;
        }

        /* Update global failure reason */
        g_status_reason = reason;

        /* Update tray state based on status */
        g_last_status = (LONG)newStatus;
        g_last_status_valid = 1;

        switch (newStatus) {
            case ClawStatus::Ready:
                tray_set_state(TrayState::Green);
                break;
            case ClawStatus::Busy:
            case ClawStatus::Checking:
                tray_set_state(TrayState::Yellow);
                break;
            case ClawStatus::Error:
                tray_set_state(TrayState::Red);
                break;
            default:
                tray_set_state(TrayState::White);
                break;
        }

        /* Fire callback */
        if (g_callback) {
            g_callback(newStatus);
        }
    }

    LOG_I("HEALTH", "Monitoring thread stopped");
    return 0;
}

void health_start() {
    if (g_hThread) return;
    g_running = true;
    g_httpFailCount = 0;
    g_autoRestarted = false;
    g_firstPoll = true;
    g_last_status_valid = 1;
    g_last_status = (LONG)ClawStatus::Checking;

    g_hThread = (HANDLE)_beginthreadex(nullptr, 0, health_thread, nullptr, 0, nullptr);
    if (!g_hThread) {
        LOG_E("HEALTH", "Failed to create thread");
    }
}

void health_stop() {
    if (!g_hThread) return;
    g_running = false;
    DWORD wr = WaitForSingleObject(g_hThread, 5000);
    if (wr == WAIT_TIMEOUT) {
        LOG_W("HEALTH", "Monitoring thread did not exit in 5s, closing handle anyway");
    }
    CloseHandle(g_hThread);
    g_hThread = nullptr;
    g_callback = nullptr;
    LOG_I("HEALTH", "Monitoring stopped");
}

void health_set_callback(HealthStatusCallback cb) {
    g_callback = cb;
}

bool health_try_get_last_status(ClawStatus* out_status) {
    if (!out_status) return false;
    if (!g_last_status_valid) return false;
    *out_status = (ClawStatus)g_last_status;
    return true;
}
