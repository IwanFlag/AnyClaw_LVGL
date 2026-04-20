/* Stub for LanChatClient — used when ENABLE_LAN_CHAT is OFF */
#ifndef LAN_CHAT_CLIENT_H
#define LAN_CHAT_CLIENT_H

#include "lan_chat_types.h"
#include <atomic>
#include <functional>
#include <string>
#include <vector>

class LanChatClient {
public:
    using EventCallback = std::function<void(const LanChatEvent&)>;
    LanChatClient() {}
    ~LanChatClient() {}
    bool start_host(unsigned short, std::string& err) { err = "LAN chat disabled"; return false; }
    void stop_host() {}
    bool connect_server(const std::string&, unsigned short, const std::string&, std::string& err) { err = "LAN chat disabled"; return false; }
    void disconnect_server() {}
    bool is_connected() const { return false; }
    bool is_hosting() const { return false; }
    bool send_global(const std::string&) { return false; }
    bool send_private(const std::string&, const std::string&) { return false; }
    bool create_group(const std::string&) { return false; }
    bool join_group(const std::string&) { return false; }
    bool send_group(const std::string&, const std::string&) { return false; }
    std::vector<std::string> discover_hosts(unsigned short, int) { return {}; }
    void set_event_callback(EventCallback) {}
};

#endif /* LAN_CHAT_CLIENT_H */
