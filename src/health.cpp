#include "health.h"
#include "app.h"
#include <windows.h>
#include <tlhelp32.h>
#include <process.h>
#include <cstdio>

static HANDLE g_hThread = nullptr;
static volatile bool g_running = false;
static HealthStatusCallback g_callback = nullptr;
static int g_httpFailCount = 0;
static bool g_autoRestarted = false;

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

/* Check HTTP health endpoint */
static bool check_http_health() {
    char response[256] = {0};
    int code = http_get("http://127.0.0.1:18789/health", response, sizeof(response), 3);
    return (code == 200);
}

/* Health monitoring thread */
static unsigned __stdcall health_thread(void* arg) {
    (void)arg;
    printf("[HEALTH] Monitoring thread started\n");

    while (g_running) {
        Sleep(10000);  /* 10 second interval */
        if (!g_running) break;

        bool nodeRunning = is_node_running();
        bool httpOk = check_http_health();

        ClawStatus newStatus;

        if (!nodeRunning && !httpOk) {
            /* Not running - try auto-restart once */
            newStatus = ClawStatus::Detected;
            if (!g_autoRestarted) {
                g_autoRestarted = true;
                printf("[HEALTH] Node not found, attempting auto-restart...\n");
                if (app_start_gateway()) {
                    printf("[HEALTH] Auto-restart initiated\n");
                    newStatus = ClawStatus::Running;  /* Optimistic */
                } else {
                    printf("[HEALTH] Auto-restart failed\n");
                    newStatus = ClawStatus::Error;
                }
            }
        } else if (nodeRunning && !httpOk) {
            /* Process exists but HTTP failed */
            g_httpFailCount++;
            printf("[HEALTH] HTTP check failed (%d/3)\n", g_httpFailCount);
            if (g_httpFailCount >= 3) {
                newStatus = ClawStatus::Error;
                /* Reset auto-restart so next NotRunning triggers it again */
                g_autoRestarted = false;
            } else {
                newStatus = ClawStatus::Running;  /* Give it time */
            }
        } else if (httpOk) {
            /* Everything OK */
            newStatus = ClawStatus::Running;
            g_httpFailCount = 0;
            g_autoRestarted = false;
        } else {
            /* Process not found but HTTP works (unlikely) */
            newStatus = ClawStatus::Running;
            g_httpFailCount = 0;
        }

        /* Update tray state based on status */
        switch (newStatus) {
            case ClawStatus::Running:
                tray_set_state(TrayState::Green);
                break;
            case ClawStatus::Detected:
                tray_set_state(TrayState::Yellow);
                break;
            case ClawStatus::Error:
                tray_set_state(TrayState::Red);
                break;
            default:
                tray_set_state(TrayState::Gray);
                break;
        }

        /* Fire callback */
        if (g_callback) {
            g_callback(newStatus);
        }
    }

    printf("[HEALTH] Monitoring thread stopped\n");
    return 0;
}

void health_start() {
    if (g_hThread) return;
    g_running = true;
    g_httpFailCount = 0;
    g_autoRestarted = false;

    g_hThread = (HANDLE)_beginthreadex(nullptr, 0, health_thread, nullptr, 0, nullptr);
    if (!g_hThread) {
        printf("[HEALTH] Failed to create thread\n");
    }
}

void health_stop() {
    if (!g_hThread) return;
    g_running = false;
    WaitForSingleObject(g_hThread, 5000);
    CloseHandle(g_hThread);
    g_hThread = nullptr;
    printf("[HEALTH] Monitoring stopped\n");
}

void health_set_callback(HealthStatusCallback cb) {
    g_callback = cb;
}
