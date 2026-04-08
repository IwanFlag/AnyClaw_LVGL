#ifndef LAN_CHAT_CLIENT_H
#define LAN_CHAT_CLIENT_H

#include "lan_chat_types.h"
#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class LanChatClient {
public:
    using EventCallback = std::function<void(const LanChatEvent&)>;

    LanChatClient();
    ~LanChatClient();

    bool start_host(unsigned short port, std::string& err);
    void stop_host();

    bool connect_server(const std::string& host, unsigned short port, const std::string& nickname, std::string& err);
    void disconnect_server();
    bool is_connected() const;
    bool is_hosting() const;

    bool send_global(const std::string& text);
    bool send_private(const std::string& to_user, const std::string& text);
    bool create_group(const std::string& group_name);
    bool join_group(const std::string& group_name);
    bool send_group(const std::string& group_name, const std::string& text);
    std::vector<std::string> discover_hosts(unsigned short discovery_port, int timeout_ms);

    void set_event_callback(EventCallback cb);

private:
    void emit(const LanChatEvent& ev);
    bool send_packet(const std::string& line);
    void recv_loop();
    void host_accept_loop();

    struct HostClient {
        void* sock = nullptr;
        std::string nick;
    };

    std::atomic<bool> hosting_{false};
    std::atomic<bool> connected_{false};
    std::atomic<bool> stop_recv_{false};
    void* host_listen_sock_ = nullptr;
    void* host_discovery_sock_ = nullptr;
    std::thread host_thread_;
    std::thread host_discovery_thread_;
    std::mutex host_mtx_;
    std::vector<HostClient> host_clients_;
    std::vector<std::string> groups_;

    void* client_sock_ = nullptr;
    std::thread recv_thread_;
    std::mutex send_mtx_;
    std::string nickname_;

    std::mutex cb_mtx_;
    EventCallback cb_;
};

#endif
