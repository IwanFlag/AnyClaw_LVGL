/* Stub definitions for Remote — active only when ENABLE_REMOTE is OFF
   Provides minimal type/var definitions so ui_main.cpp compiles without remote. */
#ifndef STUB_REMOTE_H
#define STUB_REMOTE_H

#include <string>

#ifndef ENABLE_REMOTE

/* Stub session state enum — matches RemoteSessionState layout */
enum StubRemoteSessionState { STUB_REMOTE_IDLE = 0, STUB_REMOTE_REQUESTING = 1,
                               STUB_REMOTE_PENDING_ACCEPT = 2, STUB_REMOTE_CONNECTED = 3 };

/* Stub globals (weak — real ones live in ui_main.cpp when ENABLE_REMOTE=ON) */
struct StubRemoteVars {
    StubRemoteSessionState state = STUB_REMOTE_IDLE;
    bool guard_armed = false;
    int session_left = 0;
    int retry_attempt = 0;
    int link_down_ticks = 0;
    char last_reason[128] = "";
    char stub_session_id[64] = "";
    char stub_auth_token[64] = "";
    unsigned int stub_session_seq = 1;
    unsigned long long stub_auth_expire_tick = 0;
};

extern StubRemoteVars g_stub_remote;

/* Stub RemoteTcpChannel — matches remote_tcp_channel.h interface */
class StubRemoteTcpChannel {
public:
    static StubRemoteTcpChannel& instance();
    bool start(const struct RemoteTcpConfig& cfg);
    void stop();
    bool is_connected() const;
    std::string last_error() const;
    bool send_text_frame(const char* channel, const char* payload);
    std::string last_rx_frame() const;
};

/* Stub RemoteProtocolManager — matches remote_protocol.h interface */
struct StubRemoteSessionRecord {
    std::string session_id;
    std::string auth_token;
    unsigned long long expire_tick = 0;
    std::string state;
};

class StubRemoteProtocolManager {
public:
    static StubRemoteProtocolManager& instance();
    StubRemoteSessionRecord create_session(const char* seed_hint, unsigned int ttl_ms);
    bool verify_token(const char* session_id, const char* token, const char* expected_state, bool* expired = nullptr);
    bool try_update_state(const char* session_id, const char* new_state, const char** reason = nullptr);
    void update_state(const char* session_id, const char* new_state);
    void close_session(const char* session_id);
    std::vector<StubRemoteSessionRecord> recent_records(int max_count) const;
};

/* When remote is disabled, map remote symbols to stubs in ui_main.cpp scope */
#define RemoteSessionState StubRemoteSessionState
#define REMOTE_IDLE STUB_REMOTE_IDLE
#define REMOTE_REQUESTING STUB_REMOTE_REQUESTING
#define REMOTE_PENDING_ACCEPT STUB_REMOTE_PENDING_ACCEPT
#define REMOTE_CONNECTED STUB_REMOTE_CONNECTED
#define g_remote_state g_stub_remote.state
#define g_remote_guard_armed g_stub_remote.guard_armed
#define g_remote_session_left g_stub_remote.session_left
#define g_remote_retry_attempt g_stub_remote.retry_attempt
#define g_remote_link_down_ticks g_stub_remote.link_down_ticks
#define g_remote_last_reason g_stub_remote.last_reason
#define g_remote_stub_session_id g_stub_remote.stub_session_id
#define g_remote_stub_auth_token g_stub_remote.stub_auth_token
#define g_remote_stub_session_seq g_stub_remote.stub_session_seq
#define g_remote_stub_auth_expire_tick g_stub_remote.stub_auth_expire_tick
#define RemoteTcpChannel StubRemoteTcpChannel
#define RemoteProtocolManager StubRemoteProtocolManager
#define RemoteSessionRecord StubRemoteSessionRecord
#define REMOTE_RETRY_MAX 3

#endif /* !ENABLE_REMOTE */

#endif /* STUB_REMOTE_H */
