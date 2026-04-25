/* ═══════════════════════════════════════════════════════════════
 *  selfcheck.cpp - Startup self-check and auto-repair
 *  AnyClaw LVGL v2.0 - Desktop Manager
 * ═══════════════════════════════════════════════════════════════ */

#include "selfcheck.h"
#include "boot_check.h"
#include "app.h"
#include "app_log.h"
#include "permissions.h"
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>
#include <mutex>

/* FIX Bug 8: Cross-thread mutex for startup error strings */
extern std::mutex g_startup_error_mtx;

namespace fs = std::filesystem;

/* ── Helper: run command and capture output ── */
static bool run_cmd(const char* cmd, char* output, int out_size, DWORD timeout_ms = 5000) {
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

    if (output && out_size > 0) {
        strncpy(output, result.c_str(), out_size - 1);
        output[out_size - 1] = '\0';
    }
    return exitCode == 0;
}

/* ── Check Node.js ── */
static void check_nodejs(SelfCheckItem& item) {
    if (!perm_check_exec(PermKey::EXEC_SHELL, "node --version")) {
        item.ok = false;
        strncpy(item.message, "Node.js check skipped (permission denied)", sizeof(item.message) - 1);
        ui_log("[SelfCheck] Node.js: permission denied");
        LOG_W("SELF", "Node.js: permission denied");
        return;
    }
    char output[256] = {0};
    if (run_cmd("node --version", output, sizeof(output)) && output[0]) {
        item.ok = true;
        snprintf(item.message, sizeof(item.message), "Node.js %s", output);
        ui_log("[SelfCheck] Node.js: %s", output);
        LOG_I("SELF", "Node.js: %s", output);
    } else {
        item.ok = false;
        strncpy(item.message, "Node.js not found", sizeof(item.message) - 1);
        ui_log("[SelfCheck] Node.js: NOT FOUND");
        LOG_W("SELF", "Node.js: NOT FOUND");
    }
}

/* ── Check npm ── */
static void check_npm(SelfCheckItem& item) {
    if (!perm_check_exec(PermKey::EXEC_SHELL, "npm --version")) {
        item.ok = false;
        strncpy(item.message, "npm check skipped (permission denied)", sizeof(item.message) - 1);
        ui_log("[SelfCheck] npm: permission denied");
        LOG_W("SELF", "npm: permission denied");
        return;
    }
    char output[256] = {0};
    if (run_cmd("npm --version", output, sizeof(output)) && output[0]) {
        item.ok = true;
        snprintf(item.message, sizeof(item.message), "npm %s", output);
        ui_log("[SelfCheck] npm: %s", output);
        LOG_I("SELF", "npm: %s", output);
    } else {
        item.ok = false;
        strncpy(item.message, "npm not found or error", sizeof(item.message) - 1);
        ui_log("[SelfCheck] npm: NOT FOUND or ERROR");
        LOG_W("SELF", "npm: NOT FOUND or ERROR");
    }
}

/* ── Check network ── */
static void check_network(SelfCheckItem& item) {
    /* Use http_get to test connectivity to a known endpoint */
    char response[256] = {0};
    int code = http_get("https://www.google.com", response, sizeof(response), 5);

    if (code > 0) {
        item.ok = true;
        snprintf(item.message, sizeof(item.message), "Network OK (HTTP %d)", code);
        ui_log("[SelfCheck] Network: OK (HTTP %d)", code);
        LOG_I("SELF", "Network: OK (HTTP %d)", code);
    } else {
        /* Try alternative */
        code = http_get("https://openrouter.ai/api/v1/models", response, sizeof(response), 5);
        if (code > 0) {
            item.ok = true;
            snprintf(item.message, sizeof(item.message), "Network OK (via OpenRouter)");
            ui_log("[SelfCheck] Network: OK (via OpenRouter)");
            LOG_I("SELF", "Network: OK (via OpenRouter)");
        } else {
            item.ok = false;
            strncpy(item.message, "Network unreachable", sizeof(item.message) - 1);
            ui_log("[SelfCheck] Network: UNREACHABLE");
            LOG_W("SELF", "Network: UNREACHABLE");
        }
    }
}

/* ── Check config directory ── */
static void check_config_dir(SelfCheckItem& item) {
    const char* appdata = std::getenv("APPDATA");
    if (!appdata) {
        item.ok = false;
        strncpy(item.message, "APPDATA env not set", sizeof(item.message) - 1);
        ui_log("[SelfCheck] Config dir: APPDATA not set");
        LOG_W("SELF", "Config dir: APPDATA not set");
        return;
    }

    std::string config_dir = std::string(appdata) + "\\openclaw";
    strncpy(item.message, config_dir.c_str(), sizeof(item.message) - 1);

    if (fs::exists(config_dir)) {
        /* Test writability by creating a temp file */
        std::string test_file = config_dir + "\\.anyclaw_test";
        std::ofstream f(test_file);
        if (f.is_open()) {
            f << "test";
            f.close();
            fs::remove(test_file);
            item.ok = true;
            ui_log("[SelfCheck] Config dir: writable (%s)", config_dir.c_str());
            LOG_I("SELF", "Config dir: writable (%s)", config_dir.c_str());
        } else {
            item.ok = false;
            ui_log("[SelfCheck] Config dir: NOT writable (%s)", config_dir.c_str());
            LOG_W("SELF", "Config dir: NOT writable (%s)", config_dir.c_str());
        }
    } else {
        /* Directory doesn't exist yet - try to create it */
        item.ok = false;
        ui_log("[SelfCheck] Config dir: does not exist (%s)", config_dir.c_str());
        LOG_W("SELF", "Config dir: does not exist (%s)", config_dir.c_str());
    }
}

/* ── Public API ── */

SelfCheckResult selfcheck_run() {
    SelfCheckResult result{};
    ui_log("[SelfCheck] Running startup self-check...");
    LOG_I("SELF", "Running startup self-check...");

    check_nodejs(result.nodejs);
    check_npm(result.npm);
    check_network(result.network);
    check_config_dir(result.config_dir);

    result.all_ok = result.nodejs.ok && result.npm.ok &&
                    result.network.ok && result.config_dir.ok;

    if (result.all_ok) {
        ui_log("[SelfCheck] All checks PASSED");
        LOG_I("SELF", "All checks PASSED");
    } else {
        ui_log("[SelfCheck] Some checks FAILED - attempting fixes...");
        LOG_W("SELF", "Some checks FAILED - attempting fixes...");
    }

    return result;
}

bool selfcheck_fix(SelfCheckResult& result) {
    bool fixed = true;

    /* Node.js missing - cannot auto-fix */
    if (!result.nodejs.ok) {
        ui_log("[SelfCheck] Node.js missing - cannot auto-fix, user must install");
        LOG_W("SELF", "Node.js missing - cannot auto-fix, user must install");
        fixed = false;
    }

    /* npm issues - try cache clean */
    if (!result.npm.ok && result.nodejs.ok) {
        if (!perm_check_exec(PermKey::EXEC_SHELL, "npm cache clean --force")) {
            ui_log("[SelfCheck] npm cache clean skipped (permission denied)");
            LOG_W("SELF", "npm cache clean skipped (permission denied)");
            fixed = false;
        } else {
            ui_log("[SelfCheck] Attempting: npm cache clean --force");
            LOG_I("SELF", "Attempting: npm cache clean --force");
        char output[256] = {0};
        if (run_cmd("npm cache clean --force", output, sizeof(output), 10000)) {
            /* Re-check */
            check_npm(result.npm);
            if (result.npm.ok) {
                ui_log("[SelfCheck] npm fixed after cache clean");
                LOG_I("SELF", "npm fixed after cache clean");
            } else {
                ui_log("[SelfCheck] npm still broken after cache clean");
                LOG_W("SELF", "npm still broken after cache clean");
                fixed = false;
            }
        } else {
            ui_log("[SelfCheck] npm cache clean failed");
            LOG_W("SELF", "npm cache clean failed");
            fixed = false;
        }
        }  /* else (permission allowed) */
    }

    /* Config dir issues - try to create */
    if (!result.config_dir.ok) {
        const char* appdata = std::getenv("APPDATA");
        if (appdata) {
            std::string config_dir = std::string(appdata) + "\\openclaw";
            ui_log("[SelfCheck] Attempting to create config dir: %s", config_dir.c_str());
            LOG_I("SELF", "Attempting to create config dir: %s", config_dir.c_str());
            try {
                if (fs::create_directories(config_dir)) {
                    /* Re-check */
                    check_config_dir(result.config_dir);
                    if (result.config_dir.ok) {
                        ui_log("[SelfCheck] Config dir created successfully");
                        LOG_I("SELF", "Config dir created successfully");
                    } else {
                        ui_log("[SelfCheck] Config dir created but still not writable");
                        LOG_I("SELF", "Config dir created but still not writable");
                        fixed = false;
                    }
                } else {
                    ui_log("[SelfCheck] Failed to create config dir");
                    LOG_E("SELF", "Failed to create config dir");
                    fixed = false;
                }
            } catch (const std::exception& e) {
                ui_log("[SelfCheck] Exception creating config dir: %s", e.what());
                LOG_W("SELF", "Exception creating config dir: %s", e.what());
                fixed = false;
            }
        }
    }

    /* Network - cannot auto-fix */
    if (!result.network.ok) {
        ui_log("[SelfCheck] Network unreachable - check internet connection");
        LOG_W("SELF", "Network unreachable - check internet connection");
        fixed = false;
    }

    result.all_ok = result.nodejs.ok && result.npm.ok &&
                    result.network.ok && result.config_dir.ok;

    if (!result.all_ok) {
        std::lock_guard<std::mutex> lk(g_startup_error_mtx);
        g_startup_error_title = "检测到问题";

        std::string msg;
        auto append_issue = [&msg](const char* item, const char* reason) {
            msg += "● ";
            msg += item;
            msg += "\n    原因: ";
            msg += reason;
            msg += "\n\n";
        };

        if (!result.nodejs.ok) {
            append_issue("Node.js: 未检测到", result.nodejs.message);
        }
        if (!result.npm.ok) {
            append_issue("npm: 不可用", result.npm.message);
        }
        if (!result.network.ok) {
            append_issue("网络: 无法连接", result.network.message);
        }
        if (!result.config_dir.ok) {
            append_issue("配置目录: 不可写", result.config_dir.message);
        }

        msg += "请修复上述问题后重新启动应用。";
        g_startup_error = msg;
    }

    return fixed;
}

bool selfcheck_run_and_fix() {
    SelfCheckResult result = selfcheck_run();
    if (!result.all_ok) {
        selfcheck_fix(result);
    }
    return result.all_ok;
}

/* ── Comprehensive health check (BOOT-01) ── */
std::string selfcheck_run_full() {
    BootCheckManager mgr;
    auto results = mgr.run_and_fix();

    /* Build human-readable report */
    std::string report;
    for (const auto& r : results) {
        const char* icon = "✓";
        if (r.status == BootCheckStatus::Warn)  icon = "⚠";
        if (r.status == BootCheckStatus::Error) icon = "✗";

        report += icon;
        report += " ";
        report += r.check_name;
        report += ": ";
        report += r.message;
        if (r.fix_applied) report += " [fixed]";
        report += "\n";
    }

    report += "\n";
    report += mgr.summarize(results);
    return report;
}
