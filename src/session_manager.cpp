#include "session_manager.h"
#include "app.h"
#include "app_log.h"
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <algorithm>

/* ═══════════════════════════════════════════════════════════════════
 * SessionInfo helpers
 * ═══════════════════════════════════════════════════════════════════ */

static const char* channel_display_name(const char* ch) {
    if (!ch || !ch[0]) return "Unknown";
    if (strcmp(ch, "webchat") == 0)    return "WebChat";
    if (strcmp(ch, "telegram") == 0)   return "Telegram";
    if (strcmp(ch, "discord") == 0)    return "Discord";
    if (strcmp(ch, "feishu") == 0)     return "飞书";
    if (strcmp(ch, "wechat") == 0)     return "微信";
    if (strcmp(ch, "signal") == 0)     return "Signal";
    if (strcmp(ch, "slack") == 0)      return "Slack";
    if (strcmp(ch, "whatsapp") == 0)   return "WhatsApp";
    if (strcmp(ch, "cli") == 0)        return "CLI";
    if (strcmp(ch, "line") == 0)       return "LINE";
    return ch;  /* Unknown channel: show raw name */
}

const char* SessionInfo::display_name() const {
    static char buf[128];
    if (isSubAgent) {
        snprintf(buf, sizeof(buf), "Sub-Agent");
    } else if (isCron) {
        snprintf(buf, sizeof(buf), "Cron: %s", agentId.c_str());
    } else {
        snprintf(buf, sizeof(buf), "%s", agentId.c_str());
    }
    return buf;
}

const char* SessionInfo::display_channel() const {
    return channel_display_name(channel.c_str());
}

/* ═══════════════════════════════════════════════════════════════════
 * JSON parsing helpers (lightweight, no external dependency)
 * ═══════════════════════════════════════════════════════════════════ */

/* Extract string value after key: "key":"value" */
static bool json_str(const char* json, const char* key, std::string& out) {
    char search[64];
    snprintf(search, sizeof(search), "\"%s\"", key);
    const char* p = strstr(json, search);
    if (!p) return false;
    p += strlen(search);
    while (*p == ' ' || *p == ':') p++;
    if (*p != '"') return false;
    p++;
    out.clear();
    while (*p && *p != '"') {
        out += *p++;
    }
    return true;
}

/* Extract long long value after key: "key":12345 */
static bool json_ll(const char* json, const char* key, long long& out) {
    char search[64];
    snprintf(search, sizeof(search), "\"%s\"", key);
    const char* p = strstr(json, search);
    if (!p) return false;
    p += strlen(search);
    while (*p == ' ' || *p == ':') p++;
    out = 0;
    bool neg = false;
    if (*p == '-') { neg = true; p++; }
    while (*p >= '0' && *p <= '9') {
        out = out * 10 + (*p - '0');
        p++;
    }
    if (neg) out = -out;
    return true;
}

/* ═══════════════════════════════════════════════════════════════════
 * Exec command helper (same pattern as openclaw_mgr.cpp)
 * ═══════════════════════════════════════════════════════════════════ */

static bool exec_cmd_local(const char* cmd, char* output, int out_size, DWORD timeout_ms = 8000) {
    if (output && out_size > 0) output[0] = '\0';

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
    char buf[1024];
    snprintf(buf, sizeof(buf), "cmd /c %s", cmd);

    if (!CreateProcessA(nullptr, buf, nullptr, nullptr, TRUE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        CloseHandle(hRead);
        CloseHandle(hWrite);
        return false;
    }

    CloseHandle(hWrite);

    DWORD total = 0;
    char tmp[4096];
    DWORD start = GetTickCount();

    while (total < (DWORD)out_size - 1) {
        if (GetTickCount() - start > timeout_ms) break;

        DWORD avail = 0;
        if (!PeekNamedPipe(hRead, nullptr, 0, nullptr, &avail, nullptr) || avail == 0) {
            if (WaitForSingleObject(pi.hProcess, 100) == WAIT_OBJECT_0) {
                /* Process ended, drain remaining */
                while (ReadFile(hRead, tmp, sizeof(tmp), &avail, nullptr) && avail > 0) {
                    DWORD copy = min(avail, (DWORD)out_size - 1 - total);
                    if (copy > 0 && output) {
                        memcpy(output + total, tmp, copy);
                        total += copy;
                    }
                }
                break;
            }
            continue;
        }

        DWORD bytes;
        if (!ReadFile(hRead, tmp, sizeof(tmp), &bytes, nullptr) || bytes == 0) break;
        DWORD copy = min(bytes, (DWORD)out_size - 1 - total);
        if (copy > 0 && output) {
            memcpy(output + total, tmp, copy);
            total += copy;
        }
    }

    if (output && out_size > 0) output[total] = '\0';

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hRead);
    return total > 0;
}

/* ═══════════════════════════════════════════════════════════════════
 * SessionManager implementation
 * ═══════════════════════════════════════════════════════════════════ */

static SessionManager g_session_mgr;
SessionManager& session_mgr() { return g_session_mgr; }

bool SessionManager::refresh() {
    sessions_.clear();
    last_error_.clear();

    char output[16384] = {0};
    bool ok = exec_cmd_local(
        "openclaw gateway call sessions.list --json",
        output, sizeof(output), 12000
    );

    if (!ok || output[0] == '\0') {
        last_error_ = "Failed to query sessions";
        LOG_W("SESSION", "sessions.list returned empty or failed");
        return false;
    }

    return parse_json(output);
}

bool SessionManager::parse_json(const char* json) {
    /* Walk through JSON array of session objects.
     * Each session has: key, agentId, origin{provider,surface,chatType}, lastChannel, ageMs, etc.
     * We find each "key":"..." entry and extract fields from surrounding context.
     */
    const char* p = json;
    int count = 0;

    while ((p = strstr(p, "\"key\"")) != nullptr) {
        SessionInfo si;

        /* Extract key */
        if (!json_str(p, "key", si.key)) { p += 5; continue; }

        /* Extract agentId */
        json_str(p, "agentId", si.agentId);

        /* Extract origin.provider */
        const char* origin_p = strstr(p, "\"origin\"");
        if (origin_p) {
            json_str(origin_p, "provider", si.channel);
            json_str(origin_p, "chatType", si.chatType);
        }

        /* Fallback: extract lastChannel */
        if (si.channel.empty()) {
            json_str(p, "lastChannel", si.channel);
        }

        /* Extract ageMs */
        json_ll(p, "ageMs", si.ageMs);

        /* Detect sub-agent */
        if (si.key.find("subagent:") != std::string::npos) {
            si.isSubAgent = true;
        }

        /* Detect cron */
        if (si.key.find("cron:") != std::string::npos) {
            si.isCron = true;
        }

        /* Extract token count if available */
        long long tokens = 0;
        if (json_ll(p, "approximateTokens", tokens)) {
            si.tokenCount = tokens;
        } else if (json_ll(p, "totalTokens", tokens)) {
            si.tokenCount = tokens;
        }

        sessions_.push_back(std::move(si));
        count++;

        /* Move past this session block */
        p += 5;
    }

    LOG_I("SESSION", "Parsed %d sessions from Gateway", count);
    return count > 0;
}

std::vector<SessionInfo> SessionManager::active_sessions(long long threshold_ms) const {
    std::vector<SessionInfo> result;
    for (const auto& s : sessions_) {
        if (s.is_active(threshold_ms)) {
            result.push_back(s);
        }
    }
    return result;
}

int SessionManager::active_count(long long threshold_ms) const {
    int count = 0;
    for (const auto& s : sessions_) {
        if (s.is_active(threshold_ms)) count++;
    }
    return count;
}

bool SessionManager::abort_session(const char* session_key) {
    if (!session_key || !session_key[0]) {
        last_error_ = "Invalid session key";
        return false;
    }

    char cmd[512];
    snprintf(cmd, sizeof(cmd),
             "openclaw gateway call sessions.reset --json -d \"{\\\"sessionKey\\\":\\\"%s\\\"}\"",
             session_key);

    char output[2048] = {0};
    bool ok = exec_cmd_local(cmd, output, sizeof(output), 10000);

    if (!ok) {
        last_error_ = "Failed to execute session reset";
        LOG_W("SESSION", "Abort session '%s' failed: exec error", session_key);
        return false;
    }

    LOG_I("SESSION", "Aborted session '%s': %.100s", session_key, output);
    return true;
}

bool SessionManager::abort_all() {
    char output[4096] = {0};
    bool ok = exec_cmd_local(
        "openclaw gateway call sessions.reset --json",
        output, sizeof(output), 15000
    );

    if (!ok) {
        last_error_ = "Failed to reset all sessions";
        LOG_W("SESSION", "Abort all sessions failed");
        return false;
    }

    LOG_I("SESSION", "Reset all sessions: %.200s", output);
    return true;
}

std::string SessionManager::format_active_info(long long threshold_ms) const {
    std::string result;
    auto active = active_sessions(threshold_ms);

    for (size_t i = 0; i < active.size(); i++) {
        if (i > 0) result += ", ";
        char buf[64];
        snprintf(buf, sizeof(buf), "%s:%ds",
                 active[i].channel.empty() ? "?" : active[i].channel.c_str(),
                 active[i].age_seconds());
        result += buf;
    }
    return result;
}
