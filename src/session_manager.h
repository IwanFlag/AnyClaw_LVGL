#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <string>
#include <vector>
#include <mutex>

/* Parsed session info from Gateway */
struct SessionInfo {
    std::string key;          /* Full session key e.g. "agent:main:webchat:main" */
    std::string agentId;      /* Agent ID e.g. "main" */
    std::string channel;      /* Provider/channel e.g. "webchat", "telegram" */
    std::string chatType;     /* "direct" or "group" */
    long long ageMs = 0;      /* Milliseconds since last activity */
    long long tokenCount = 0; /* Approximate token usage */
    bool isSubAgent = false;  /* Is this a sub-agent session? */
    bool isCron = false;      /* Is this a cron-triggered session? */

    /* Derived display helpers */
    const char* display_name() const;
    const char* display_channel() const;
    int age_seconds() const { return (int)(ageMs / 1000); }
    bool is_active(long long threshold_ms = 300000) const { return ageMs > 0 && ageMs < threshold_ms; }
};

/* Session manager: fetch, parse, abort sessions */
class SessionManager {
public:
    /* Fetch session list from Gateway CLI (blocking, ~1.5s) */
    bool refresh();

    /* Get all parsed sessions */
    std::vector<SessionInfo> sessions() const;

    /* Get only active sessions (ageMs < threshold) */
    std::vector<SessionInfo> active_sessions(long long threshold_ms = 300000) const;

    /* Get count of active sessions */
    int active_count(long long threshold_ms = 300000) const;

    /* Abort/reset a specific session by key */
    bool abort_session(const char* session_key);

    /* Reset all sessions */
    bool abort_all();

    /* Formatted info string for UI display (legacy compat) */
    std::string format_active_info(long long threshold_ms = 300000) const;

    /* Get last error message */
    std::string last_error() const;

private:
    std::vector<SessionInfo> sessions_;
    std::string last_error_;
    mutable std::mutex mtx_;

    bool parse_json(const char* json, std::vector<SessionInfo>& out_sessions) const;
};

/* Global singleton */
SessionManager& session_mgr();

#endif /* SESSION_MANAGER_H */
