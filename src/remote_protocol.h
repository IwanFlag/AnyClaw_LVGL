#ifndef REMOTE_PROTOCOL_H
#define REMOTE_PROTOCOL_H

#include <string>
#include <vector>

struct RemoteSessionRecord {
    std::string session_id;
    std::string auth_token;
    unsigned long long expire_tick = 0;
    std::string state;
};

class RemoteProtocolManager {
public:
    static RemoteProtocolManager& instance();

    RemoteSessionRecord create_session(const char* seed_hint, unsigned int ttl_ms);
    bool verify_token(const char* session_id, const char* token, const char* expected_state, bool* expired = nullptr);
    void update_state(const char* session_id, const char* new_state);
    void close_session(const char* session_id);
    std::vector<RemoteSessionRecord> recent_records(int max_count) const;

private:
    RemoteProtocolManager() = default;
};

#endif
