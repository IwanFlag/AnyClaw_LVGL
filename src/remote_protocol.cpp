#include "remote_protocol.h"

#include <algorithm>
#include <mutex>
#include <unordered_map>
#include <windows.h>

namespace {
std::mutex g_remote_mtx;
std::unordered_map<std::string, RemoteSessionRecord> g_sessions;
std::vector<RemoteSessionRecord> g_recent;
unsigned int g_seq = 1;

unsigned long long now_tick() {
    return (unsigned long long)GetTickCount64();
}
}

RemoteProtocolManager& RemoteProtocolManager::instance() {
    static RemoteProtocolManager inst;
    return inst;
}

RemoteSessionRecord RemoteProtocolManager::create_session(const char* seed_hint, unsigned int ttl_ms) {
    std::lock_guard<std::mutex> lk(g_remote_mtx);
    RemoteSessionRecord r{};
    unsigned long long n = now_tick();
    char sid[96] = {0};
    char tok[96] = {0};
    snprintf(sid, sizeof(sid), "rp-%08llX-%04u", n & 0xFFFFFFFFull, g_seq++);
    snprintf(tok, sizeof(tok), "auth-%08llX-%s", (n >> 8) & 0xFFFFFFFFull, seed_hint ? seed_hint : "any");
    r.session_id = sid;
    r.auth_token = tok;
    r.expire_tick = n + ttl_ms;
    r.state = "pending";
    g_sessions[r.session_id] = r;
    g_recent.push_back(r);
    if (g_recent.size() > 32) g_recent.erase(g_recent.begin());
    return r;
}

bool RemoteProtocolManager::verify_token(const char* session_id, const char* token, const char* expected_state, bool* expired) {
    std::lock_guard<std::mutex> lk(g_remote_mtx);
    if (expired) *expired = false;
    if (!session_id || !token) return false;
    auto it = g_sessions.find(session_id);
    if (it == g_sessions.end()) return false;
    const auto& rec = it->second;
    if (now_tick() > rec.expire_tick) {
        if (expired) *expired = true;
        return false;
    }
    if (expected_state && expected_state[0] && rec.state != expected_state) return false;
    return rec.auth_token == token;
}

void RemoteProtocolManager::update_state(const char* session_id, const char* new_state) {
    std::lock_guard<std::mutex> lk(g_remote_mtx);
    if (!session_id || !new_state) return;
    auto it = g_sessions.find(session_id);
    if (it == g_sessions.end()) return;
    it->second.state = new_state;
    g_recent.push_back(it->second);
    if (g_recent.size() > 32) g_recent.erase(g_recent.begin());
}

void RemoteProtocolManager::close_session(const char* session_id) {
    std::lock_guard<std::mutex> lk(g_remote_mtx);
    if (!session_id) return;
    auto it = g_sessions.find(session_id);
    if (it == g_sessions.end()) return;
    it->second.state = "closed";
    g_recent.push_back(it->second);
    if (g_recent.size() > 32) g_recent.erase(g_recent.begin());
    g_sessions.erase(it);
}

std::vector<RemoteSessionRecord> RemoteProtocolManager::recent_records(int max_count) const {
    std::lock_guard<std::mutex> lk(g_remote_mtx);
    std::vector<RemoteSessionRecord> out = g_recent;
    if (max_count > 0 && (int)out.size() > max_count) {
        out.erase(out.begin(), out.end() - max_count);
    }
    return out;
}
