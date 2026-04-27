#include "session_manager.h"
#include "app.h"
#include "app_log.h"
#include "permissions.h"
#include "tracing.h"
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
    if (!perm_check_exec(PermKey::EXEC_SHELL, cmd)) {
        if (output && out_size > 0) snprintf(output, out_size, "DENY: exec_shell blocked or rejected");
        return false;
    }

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

    bool timed_out = false;
    while (total < (DWORD)out_size - 1) {
        if (GetTickCount() - start > timeout_ms) {
            timed_out = true;
            break;
        }

        DWORD avail = 0;
        if (!PeekNamedPipe(hRead, nullptr, 0, nullptr, &avail, nullptr) || avail == 0) {
            if (WaitForSingleObject(pi.hProcess, 100) == WAIT_OBJECT_0) {
                /* Process ended, drain remaining */
                while (ReadFile(hRead, tmp, sizeof(tmp), &avail, nullptr) && avail > 0) {
                    DWORD copy = std::min(avail, (DWORD)out_size - 1 - total);
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
        DWORD copy = std::min(bytes, (DWORD)out_size - 1 - total);
        if (copy > 0 && output) {
            memcpy(output + total, tmp, copy);
            total += copy;
        }
    }

    if (output && out_size > 0) output[total] = '\0';

    if (timed_out) {
        TerminateProcess(pi.hProcess, 1);
        LOG_W("SESSION", "exec timeout: %s (%lums)", cmd, (unsigned long)timeout_ms);
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hRead);
    return !timed_out && total > 0;
}

/* ═══════════════════════════════════════════════════════════════════
 * SessionManager implementation
 * ═══════════════════════════════════════════════════════════════════ */

static SessionManager g_session_mgr;
SessionManager& session_mgr() { return g_session_mgr; }
static volatile LONG g_session_refresh_inflight = 0;

struct SessionRefreshInflightGuard {
    bool held = false;
    SessionRefreshInflightGuard() {
        held = (InterlockedCompareExchange(&g_session_refresh_inflight, 1, 0) == 0);
    }
    ~SessionRefreshInflightGuard() {
        if (held) InterlockedExchange(&g_session_refresh_inflight, 0);
    }
};

bool SessionManager::refresh() {
    TraceSpan span("session_refresh");
    SessionRefreshInflightGuard inflight_guard;
    if (!inflight_guard.held) {
        LOG_D("SESSION", "Skip refresh: previous refresh still in flight");
        return true;
    }

    if (app_is_chat_streaming()) {
        LOG_D("SESSION", "Skip refresh while chat streaming");
        return true;
    }

    std::vector<SessionInfo> new_sessions;
    const unsigned long long now_ms = GetTickCount64();
    static const unsigned long long kHttpProbeBackoffMs = 60000;
    static const unsigned long long kGatewayDownBackoffMs = 30000;
    static const unsigned long long kCliRetryBackoffMs = 120000;
    static unsigned long long s_cli_warn_after_ms = 0;

    bool should_probe_http = true;
    bool had_gateway_unreachable = false;
    std::string preferred_http_url;

    {
        std::lock_guard<std::mutex> lk(mtx_);
        should_probe_http = (now_ms >= http_probe_after_ms_);
        preferred_http_url = preferred_http_url_;
    }

    auto try_parse_http_json = [&](const char* url) -> bool {
        char response[16384] = {0};
        int code = http_get(url, response, sizeof(response), 1);

        if (code <= 0) {
            had_gateway_unreachable = true;
            return false;
        }

        const bool looks_json = response[0] == '{' || response[0] == '[';
        if (code >= 200 && code < 300 && looks_json) {
            if (parse_json(response, new_sessions)) {
                std::lock_guard<std::mutex> lk(mtx_);
                sessions_ = std::move(new_sessions);
                last_error_.clear();
                preferred_http_url_ = url;
                http_probe_after_ms_ = 0;
                return true;
            }
        }
        return false;
    };

    if (should_probe_http) {
        bool http_ok = false;

        if (!preferred_http_url.empty()) {
            http_ok = try_parse_http_json(preferred_http_url.c_str());
        }

        if (!http_ok) {
            const char* candidates[] = {
                "http://127.0.0.1:18789/api/sessions",
                "http://127.0.0.1:18789/api/session/list",
                "http://127.0.0.1:18789/api/v1/sessions",
                "http://127.0.0.1:18789/api/sessions/list"
            };
            for (const char* url : candidates) {
                if (!preferred_http_url.empty() && preferred_http_url == url) continue;
                if (try_parse_http_json(url)) {
                    http_ok = true;
                    break;
                }
                if (had_gateway_unreachable) {
                    /* Gateway is down, don't fan out remaining endpoint probes this round. */
                    break;
                }
            }
        }

        if (http_ok) {
            return true;
        }

        {
            std::lock_guard<std::mutex> lk(mtx_);
            preferred_http_url_.clear();
            if (had_gateway_unreachable) {
                http_probe_after_ms_ = now_ms + kGatewayDownBackoffMs;
            } else {
                http_probe_after_ms_ = now_ms + kHttpProbeBackoffMs;
                LOG_D("SESSION", "No valid sessions HTTP endpoint, probe backoff %llums", kHttpProbeBackoffMs);
            }
        }

        if (had_gateway_unreachable) {
            std::lock_guard<std::mutex> lk(mtx_);
            sessions_.clear();
            last_error_ = "Gateway unreachable";
            LOG_D("SESSION", "Gateway unreachable, skip CLI fallback");
            span.set_fail();
            return false;
        }
    }

    bool allow_cli = true;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        if (now_ms < cli_next_retry_ms_) {
            allow_cli = false;
        }
    }

    if (!allow_cli) {
        return true;
    }

    static const DWORD kSessionListTimeoutMs = 500;
    char output[16384] = {0};
    bool ok = exec_cmd_local(
        "openclaw gateway call sessions.list --json",
        output, sizeof(output), kSessionListTimeoutMs
    );

    if (!ok || output[0] == '\0') {
        std::lock_guard<std::mutex> lk(mtx_);
        sessions_.clear();
        last_error_ = "Failed to query sessions";
        cli_next_retry_ms_ = now_ms + kCliRetryBackoffMs;
        if (now_ms >= s_cli_warn_after_ms) {
            LOG_W("SESSION", "sessions.list CLI fallback failed (cooldown=%llums)", kCliRetryBackoffMs);
            s_cli_warn_after_ms = now_ms + 300000; /* warn at most once per 5 minutes */
        } else {
            LOG_D("SESSION", "sessions.list CLI fallback failed (suppressed)");
        }
        span.set_fail();
        return false;
    }

    if (!parse_json(output, new_sessions)) {
        std::lock_guard<std::mutex> lk(mtx_);
        sessions_.clear();
        last_error_ = "Invalid sessions JSON";
        cli_next_retry_ms_ = now_ms + kCliRetryBackoffMs;
        span.set_fail();
        return false;
    }

    {
        std::lock_guard<std::mutex> lk(mtx_);
        sessions_ = std::move(new_sessions);
        last_error_.clear();
        cli_next_retry_ms_ = now_ms;
    }
    return true;
}

bool SessionManager::parse_json(const char* json, std::vector<SessionInfo>& out_sessions) const {
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

        out_sessions.push_back(std::move(si));
        count++;

        /* Move past this session block */
        p += 5;
    }

    LOG_I("SESSION", "Parsed %d sessions from Gateway", count);
    if (count == 0) {
        /* Empty list is a valid idle state. */
        return (strstr(json, "[") != nullptr && strstr(json, "]") != nullptr);
    }
    return true;
}

std::vector<SessionInfo> SessionManager::active_sessions(long long threshold_ms) const {
    std::vector<SessionInfo> result;
    std::lock_guard<std::mutex> lk(mtx_);
    for (const auto& s : sessions_) {
        if (s.is_active(threshold_ms)) {
            result.push_back(s);
        }
    }
    return result;
}

int SessionManager::active_count(long long threshold_ms) const {
    int count = 0;
    std::lock_guard<std::mutex> lk(mtx_);
    for (const auto& s : sessions_) {
        if (s.is_active(threshold_ms)) count++;
    }
    return count;
}

bool SessionManager::abort_session(const char* session_key) {
    TraceSpan span("session_abort", session_key ? session_key : "");
    if (!session_key || !session_key[0]) {
        last_error_ = "Invalid session key";
        span.set_fail();
        return false;
    }

    char cmd[512];
    snprintf(cmd, sizeof(cmd),
             "openclaw gateway call sessions.reset --json -d \"{\\\"sessionKey\\\":\\\"%s\\\"}\"",
             session_key);

    char output[2048] = {0};
    bool ok = exec_cmd_local(cmd, output, sizeof(output), 10000);

    if (!ok) {
        std::lock_guard<std::mutex> lk(mtx_);
        last_error_ = "Failed to execute session reset";
        LOG_W("SESSION", "Abort session '%s' failed: exec error", session_key);
        span.set_fail();
        return false;
    }

    LOG_I("SESSION", "Aborted session '%s': %.100s", session_key, output);
    return true;
}

bool SessionManager::abort_all() {
    TraceSpan span("session_abort_all");
    char output[4096] = {0};
    bool ok = exec_cmd_local(
        "openclaw gateway call sessions.reset --json",
        output, sizeof(output), 15000
    );

    if (!ok) {
        std::lock_guard<std::mutex> lk(mtx_);
        last_error_ = "Failed to reset all sessions";
        LOG_W("SESSION", "Abort all sessions failed");
        span.set_fail();
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

std::vector<SessionInfo> SessionManager::sessions() const {
    std::lock_guard<std::mutex> lk(mtx_);
    return sessions_;
}

std::string SessionManager::last_error() const {
    std::lock_guard<std::mutex> lk(mtx_);
    return last_error_;
}
