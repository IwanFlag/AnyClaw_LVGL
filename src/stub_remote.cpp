/* Stub for Remote TCP channel + RemoteProtocolManager — active only when ENABLE_REMOTE is OFF */
#ifndef ENABLE_REMOTE

#include "remote_tcp_channel.h"
#include "remote_protocol.h"
#include <string>

/* ── RemoteTcpChannel stubs ── */
RemoteTcpChannel& RemoteTcpChannel::instance() {
    static RemoteTcpChannel inst;
    return inst;
}

RemoteTcpChannel::RemoteTcpChannel() {}
RemoteTcpChannel::~RemoteTcpChannel() {}

bool RemoteTcpChannel::start(const RemoteTcpConfig&) { return false; }
void RemoteTcpChannel::stop() {}
bool RemoteTcpChannel::is_connected() const { return false; }
std::string RemoteTcpChannel::last_error() const { return "Remote is disabled in this build"; }
bool RemoteTcpChannel::send_text_frame(const char*, const char*) { return false; }
std::string RemoteTcpChannel::last_rx_frame() const { return ""; }

/* ── RemoteProtocolManager stubs ── */
RemoteProtocolManager& RemoteProtocolManager::instance() {
    static RemoteProtocolManager inst;
    return inst;
}

RemoteSessionRecord RemoteProtocolManager::create_session(const char*, unsigned int) {
    return {};
}

bool RemoteProtocolManager::verify_token(const char*, const char*, const char*, bool*) {
    return false;
}

bool RemoteProtocolManager::try_update_state(const char*, const char*, const char**) {
    return false;
}

void RemoteProtocolManager::update_state(const char*, const char*) {}

void RemoteProtocolManager::close_session(const char*) {}

std::vector<RemoteSessionRecord> RemoteProtocolManager::recent_records(int) const {
    return {};
}

#endif /* !ENABLE_REMOTE */
