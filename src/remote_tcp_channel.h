#ifndef REMOTE_TCP_CHANNEL_H
#define REMOTE_TCP_CHANNEL_H

#include <string>

struct RemoteTcpConfig {
    std::string host = "127.0.0.1";
    int port = 21999;
    int heartbeat_sec = 2;
    int reconnect_max = 3;
};

class RemoteTcpChannel {
public:
    static RemoteTcpChannel& instance();

    bool start(const RemoteTcpConfig& cfg);
    void stop();
    bool is_connected() const;
    std::string last_error() const;
    bool send_text_frame(const char* channel, const char* payload);
    std::string last_rx_frame() const;

private:
    RemoteTcpChannel();
    ~RemoteTcpChannel();
    RemoteTcpChannel(const RemoteTcpChannel&) = delete;
    RemoteTcpChannel& operator=(const RemoteTcpChannel&) = delete;

    struct Impl;
    Impl* p_;
};

#endif
