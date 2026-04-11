#include "remote_tcp_channel.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include "app_log.h"

#include <atomic>
#include <cstring>
#include <mutex>
#include <thread>

struct RemoteTcpChannel::Impl {
    std::mutex mtx;
    std::thread worker;
    std::atomic<bool> running{false};
    std::atomic<bool> connected{false};
    SOCKET sock = INVALID_SOCKET;
    RemoteTcpConfig cfg;
    std::string last_err;
    std::string last_rx;
    bool wsa_ready = false;
};

static void close_sock_safe(SOCKET& s) {
    if (s != INVALID_SOCKET) {
        closesocket(s);
        s = INVALID_SOCKET;
    }
}

RemoteTcpChannel& RemoteTcpChannel::instance() {
    static RemoteTcpChannel inst;
    return inst;
}

RemoteTcpChannel::RemoteTcpChannel() {
    p_ = new Impl();
}

RemoteTcpChannel::~RemoteTcpChannel() {
    stop();
    delete p_;
}

bool RemoteTcpChannel::start(const RemoteTcpConfig& cfg) {
    if (!p_) return false;
    stop();
    {
        std::lock_guard<std::mutex> lk(p_->mtx);
        p_->cfg = cfg;
        p_->last_err.clear();
    }

    if (!p_->wsa_ready) {
        WSADATA wsa{};
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
            std::lock_guard<std::mutex> lk(p_->mtx);
            p_->last_err = "WSAStartup failed";
            return false;
        }
        p_->wsa_ready = true;
    }

    p_->running = true;
    p_->worker = std::thread([this]() {
        int reconnect_fail = 0;
        while (p_->running) {
            if (p_->sock == INVALID_SOCKET) {
                RemoteTcpConfig cfg_local;
                {
                    std::lock_guard<std::mutex> lk(p_->mtx);
                    cfg_local = p_->cfg;
                }
                SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                if (s == INVALID_SOCKET) {
                    reconnect_fail++;
                } else {
                    sockaddr_in addr{};
                    addr.sin_family = AF_INET;
                    addr.sin_port = htons((u_short)cfg_local.port);
                    if (inet_pton(AF_INET, cfg_local.host.c_str(), &addr.sin_addr) == 1 &&
                        connect(s, (sockaddr*)&addr, sizeof(addr)) == 0) {
                        p_->sock = s;
                        p_->connected = true;
                        reconnect_fail = 0;
                        LOG_I("REMOTE-TCP", "Connected to %s:%d", cfg_local.host.c_str(), cfg_local.port);
                    } else {
                        close_sock_safe(s);
                        reconnect_fail++;
                    }
                }

                if (reconnect_fail > cfg_local.reconnect_max) {
                    p_->connected = false;
                    std::lock_guard<std::mutex> lk(p_->mtx);
                    p_->last_err = "connect failed after retries";
                    break;
                }
                if (p_->sock == INVALID_SOCKET) {
                    Sleep(1200);
                    continue;
                }
            }

            const char* ping = "PING\n";
            int sent = send(p_->sock, ping, (int)strlen(ping), 0);
            if (sent <= 0) {
                p_->connected = false;
                close_sock_safe(p_->sock);
                Sleep(1000);
                continue;
            }

            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(p_->sock, &readfds);
            timeval tv{};
            tv.tv_sec = 0;
            tv.tv_usec = 120000;
            int sel = select(0, &readfds, nullptr, nullptr, &tv);
            if (sel > 0 && FD_ISSET(p_->sock, &readfds)) {
                char buf[128] = {0};
                int r = recv(p_->sock, buf, sizeof(buf) - 1, 0);
                if (r <= 0) {
                    p_->connected = false;
                    close_sock_safe(p_->sock);
                    Sleep(1000);
                    continue;
                }
                {
                    std::lock_guard<std::mutex> lk(p_->mtx);
                    p_->last_rx.assign(buf, r);
                }
            }

            int hb = 2;
            {
                std::lock_guard<std::mutex> lk(p_->mtx);
                hb = p_->cfg.heartbeat_sec > 0 ? p_->cfg.heartbeat_sec : 2;
            }
            for (int i = 0; i < hb * 10 && p_->running; ++i) Sleep(100);
        }
        close_sock_safe(p_->sock);
        p_->connected = false;
    });
    return true;
}

void RemoteTcpChannel::stop() {
    if (!p_) return;
    p_->running = false;
    close_sock_safe(p_->sock);
    if (p_->worker.joinable()) p_->worker.join();
    p_->connected = false;
}

bool RemoteTcpChannel::is_connected() const {
    return p_ && p_->connected;
}

std::string RemoteTcpChannel::last_error() const {
    if (!p_) return "";
    std::lock_guard<std::mutex> lk(p_->mtx);
    return p_->last_err;
}

bool RemoteTcpChannel::send_text_frame(const char* channel, const char* payload) {
    if (!p_ || !channel || !channel[0] || !payload) return false;
    if (p_->sock == INVALID_SOCKET) return false;
    char frame[1024];
    snprintf(frame, sizeof(frame), "CH:%s|%s\n", channel, payload);
    int sent = send(p_->sock, frame, (int)strlen(frame), 0);
    return sent > 0;
}

std::string RemoteTcpChannel::last_rx_frame() const {
    if (!p_) return "";
    std::lock_guard<std::mutex> lk(p_->mtx);
    return p_->last_rx;
}
