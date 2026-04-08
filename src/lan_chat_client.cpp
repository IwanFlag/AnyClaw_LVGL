#include "lan_chat_client.h"
#include "lan_chat_protocol.h"

#include <algorithm>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

static bool g_wsa_inited = false;

static bool ensure_wsa() {
    if (g_wsa_inited) return true;
    WSADATA wsa{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;
    g_wsa_inited = true;
    return true;
}

static void close_sock(SOCKET s) {
    if (s != INVALID_SOCKET) closesocket(s);
}

LanChatClient::LanChatClient() {}

LanChatClient::~LanChatClient() {
    disconnect_server();
    stop_host();
}

void LanChatClient::set_event_callback(EventCallback cb) {
    std::lock_guard<std::mutex> lk(cb_mtx_);
    cb_ = std::move(cb);
}

void LanChatClient::emit(const LanChatEvent& ev) {
    std::lock_guard<std::mutex> lk(cb_mtx_);
    if (cb_) cb_(ev);
}

bool LanChatClient::is_connected() const { return connected_.load(); }
bool LanChatClient::is_hosting() const { return hosting_.load(); }

bool LanChatClient::start_host(unsigned short port, std::string& err) {
    err.clear();
    if (hosting_.load()) return true;
    if (!ensure_wsa()) {
        err = "WSAStartup failed";
        return false;
    }
    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_sock == INVALID_SOCKET) {
        err = "socket() failed";
        return false;
    }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    BOOL opt = TRUE;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    if (bind(listen_sock, (sockaddr*)&addr, sizeof(addr)) != 0) {
        err = "bind() failed";
        close_sock(listen_sock);
        return false;
    }
    if (listen(listen_sock, 8) != 0) {
        err = "listen() failed";
        close_sock(listen_sock);
        return false;
    }
    host_listen_sock_ = (void*)listen_sock;
    hosting_.store(true);
    host_thread_ = std::thread([this]() { host_accept_loop(); });
    return true;
}

void LanChatClient::stop_host() {
    if (!hosting_.load()) return;
    hosting_.store(false);
    SOCKET ls = (SOCKET)host_listen_sock_;
    host_listen_sock_ = nullptr;
    close_sock(ls);
    if (host_thread_.joinable()) host_thread_.join();
    std::lock_guard<std::mutex> lk(host_mtx_);
    for (auto& c : host_clients_) close_sock((SOCKET)c.sock);
    host_clients_.clear();
    groups_.clear();
}

bool LanChatClient::connect_server(const std::string& host, unsigned short port, const std::string& nickname, std::string& err) {
    err.clear();
    disconnect_server();
    if (!ensure_wsa()) {
        err = "WSAStartup failed";
        return false;
    }
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        err = "socket() failed";
        return false;
    }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (InetPtonA(AF_INET, host.c_str(), &addr.sin_addr) != 1) {
        err = "invalid host";
        close_sock(s);
        return false;
    }
    if (connect(s, (sockaddr*)&addr, sizeof(addr)) != 0) {
        err = "connect() failed";
        close_sock(s);
        return false;
    }
    client_sock_ = (void*)s;
    nickname_ = nickname.empty() ? "user" : nickname;
    connected_.store(true);
    stop_recv_.store(false);
    send_packet(lan_pack({"HELLO", nickname_, "", "", ""}));
    recv_thread_ = std::thread([this]() { recv_loop(); });
    return true;
}

void LanChatClient::disconnect_server() {
    stop_recv_.store(true);
    connected_.store(false);
    SOCKET s = (SOCKET)client_sock_;
    client_sock_ = nullptr;
    close_sock(s);
    if (recv_thread_.joinable()) recv_thread_.join();
}

bool LanChatClient::send_packet(const std::string& line) {
    std::lock_guard<std::mutex> lk(send_mtx_);
    if (!connected_.load() || !client_sock_) return false;
    SOCKET s = (SOCKET)client_sock_;
    int total = 0;
    int need = (int)line.size();
    while (total < need) {
        int n = send(s, line.c_str() + total, need - total, 0);
        if (n <= 0) return false;
        total += n;
    }
    return true;
}

bool LanChatClient::send_global(const std::string& text) {
    return send_packet(lan_pack({"BROADCAST", nickname_, "", "", text}));
}

bool LanChatClient::send_private(const std::string& to_user, const std::string& text) {
    return send_packet(lan_pack({"PRIVATE", nickname_, to_user, "", text}));
}

bool LanChatClient::create_group(const std::string& group_name) {
    return send_packet(lan_pack({"GROUP_CREATE", nickname_, "", group_name, ""}));
}

bool LanChatClient::join_group(const std::string& group_name) {
    return send_packet(lan_pack({"GROUP_JOIN", nickname_, "", group_name, ""}));
}

bool LanChatClient::send_group(const std::string& group_name, const std::string& text) {
    return send_packet(lan_pack({"GROUP_MSG", nickname_, "", group_name, text}));
}

void LanChatClient::recv_loop() {
    SOCKET s = (SOCKET)client_sock_;
    std::string buf;
    char chunk[1024];
    while (!stop_recv_.load()) {
        int n = recv(s, chunk, sizeof(chunk), 0);
        if (n <= 0) break;
        buf.append(chunk, chunk + n);
        for (;;) {
            size_t pos = buf.find('\n');
            if (pos == std::string::npos) break;
            std::string line = buf.substr(0, pos + 1);
            buf.erase(0, pos + 1);
            LanPacket p;
            if (!lan_unpack(line, p)) continue;
            LanChatEvent ev{};
            if (p.cmd == "MSG_GLOBAL") ev.type = LanChatEventType::GlobalMessage;
            else if (p.cmd == "MSG_PRIVATE") ev.type = LanChatEventType::PrivateMessage;
            else if (p.cmd == "MSG_GROUP") ev.type = LanChatEventType::GroupMessage;
            else if (p.cmd == "USER_LIST") ev.type = LanChatEventType::UserList;
            else if (p.cmd == "GROUP_LIST") ev.type = LanChatEventType::GroupList;
            else ev.type = LanChatEventType::System;
            ev.from = p.from;
            ev.target = p.group.empty() ? p.to : p.group;
            ev.text = p.payload;
            emit(ev);
        }
    }
    connected_.store(false);
    emit({LanChatEventType::System, "system", "", "LAN connection closed"});
}

void LanChatClient::host_accept_loop() {
    SOCKET listen_sock = (SOCKET)host_listen_sock_;
    auto send_to = [](SOCKET dst, const LanPacket& p) {
        std::string line = lan_pack(p);
        int off = 0;
        while (off < (int)line.size()) {
            int n = send(dst, line.c_str() + off, (int)line.size() - off, 0);
            if (n <= 0) return false;
            off += n;
        }
        return true;
    };
    auto broadcast = [&](const LanPacket& p) {
        std::lock_guard<std::mutex> lk(host_mtx_);
        for (auto& c : host_clients_) send_to((SOCKET)c.sock, p);
    };
    auto push_user_list = [&]() {
        std::vector<std::string> users;
        {
            std::lock_guard<std::mutex> lk(host_mtx_);
            for (auto& c : host_clients_) users.push_back(c.nick);
        }
        broadcast({"USER_LIST", "system", "", "", lan_join_csv(users)});
    };
    auto push_group_list = [&]() {
        std::vector<std::string> gs;
        {
            std::lock_guard<std::mutex> lk(host_mtx_);
            gs = groups_;
        }
        broadcast({"GROUP_LIST", "system", "", "", lan_join_csv(gs)});
    };

    while (hosting_.load()) {
        sockaddr_in cli_addr{};
        int len = sizeof(cli_addr);
        SOCKET cli = accept(listen_sock, (sockaddr*)&cli_addr, &len);
        if (cli == INVALID_SOCKET) break;

        std::thread([&, cli]() {
            std::string nick = "user";
            std::string rbuf;
            char tmp[1024];
            bool joined = false;
            while (hosting_.load()) {
                int n = recv(cli, tmp, sizeof(tmp), 0);
                if (n <= 0) break;
                rbuf.append(tmp, tmp + n);
                for (;;) {
                    size_t pos = rbuf.find('\n');
                    if (pos == std::string::npos) break;
                    std::string line = rbuf.substr(0, pos + 1);
                    rbuf.erase(0, pos + 1);
                    LanPacket p;
                    if (!lan_unpack(line, p)) continue;
                    if (p.cmd == "HELLO") {
                        nick = p.from.empty() ? "user" : p.from;
                        {
                            std::lock_guard<std::mutex> lk(host_mtx_);
                            host_clients_.push_back({(void*)cli, nick});
                        }
                        joined = true;
                        broadcast({"SYSTEM", "system", "", "", nick + " joined LAN chat"});
                        push_user_list();
                        push_group_list();
                    } else if (p.cmd == "BROADCAST") {
                        broadcast({"MSG_GLOBAL", nick, "", "", p.payload});
                    } else if (p.cmd == "PRIVATE") {
                        std::lock_guard<std::mutex> lk(host_mtx_);
                        for (auto& c : host_clients_) {
                            if (c.nick == p.to || c.nick == nick) {
                                send_to((SOCKET)c.sock, {"MSG_PRIVATE", nick, p.to, "", p.payload});
                            }
                        }
                    } else if (p.cmd == "GROUP_CREATE") {
                        {
                            std::lock_guard<std::mutex> lk(host_mtx_);
                            if (std::find(groups_.begin(), groups_.end(), p.group) == groups_.end()) groups_.push_back(p.group);
                        }
                        broadcast({"SYSTEM", "system", "", "", nick + " created group: " + p.group});
                        push_group_list();
                    } else if (p.cmd == "GROUP_JOIN") {
                        broadcast({"SYSTEM", "system", "", "", nick + " joined group: " + p.group});
                    } else if (p.cmd == "GROUP_MSG") {
                        broadcast({"MSG_GROUP", nick, "", p.group, p.payload});
                    }
                }
            }
            close_sock(cli);
            if (joined) {
                {
                    std::lock_guard<std::mutex> lk(host_mtx_);
                    host_clients_.erase(std::remove_if(host_clients_.begin(), host_clients_.end(),
                        [&](const HostClient& hc) { return (SOCKET)hc.sock == cli; }), host_clients_.end());
                }
                broadcast({"SYSTEM", "system", "", "", nick + " left LAN chat"});
                push_user_list();
            }
        }).detach();
    }
}
