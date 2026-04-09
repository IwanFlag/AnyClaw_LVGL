#include "remote_protocol.h"

#include <algorithm>
#include <cstdio>
#include <mutex>
#include <unordered_map>
#include <windows.h>

namespace {
std::mutex g_remote_mtx;
std::unordered_map<std::string, RemoteSessionRecord> g_sessions;
std::vector<RemoteSessionRecord> g_recent;
unsigned int g_seq = 1;
unsigned long long g_rng_state = 0x9E3779B97F4A7C15ull;

unsigned long long now_tick() {
    return (unsigned long long)GetTickCount64();
}

unsigned long long mix64(unsigned long long x) {
    x ^= x >> 30;
    x *= 0xBF58476D1CE4E5B9ull;
    x ^= x >> 27;
    x *= 0x94D049BB133111EBull;
    x ^= x >> 31;
    return x;
}

unsigned long long next_rand64() {
    unsigned long long t = now_tick();
    LARGE_INTEGER qpc{};
    QueryPerformanceCounter(&qpc);
    g_rng_state ^= mix64((unsigned long long)qpc.QuadPart ^ t);
    g_rng_state = mix64(g_rng_state + 0xA0761D6478BD642Full);
    return g_rng_state;
}

std::string make_token(const char* seed_hint) {
    unsigned long long r1 = next_rand64();
    unsigned long long r2 = next_rand64();
    char tok[128] = {0};
    snprintf(tok, sizeof(tok), "auth-%08llX-%08llX-%s",
             (r1 & 0xFFFFFFFFull),
             (r2 & 0xFFFFFFFFull),
             (seed_hint && seed_hint[0]) ? seed_hint : "any");
    return tok;
}

void prune_expired_locked() {
    const auto n = now_tick();
    for (auto it = g_sessions.begin(); it != g_sessions.end(); ) {
        if (n > it->second.expire_tick) {
            RemoteSessionRecord rec = it->second;
            rec.state = "expired";
            g_recent.push_back(rec);
            it = g_sessions.erase(it);
        } else {
            ++it;
        }
    }
    if (g_recent.size() > 64) {
        g_recent.erase(g_recent.begin(), g_recent.end() - 64);
    }
}
}

RemoteProtocolManager& RemoteProtocolManager::instance() {
    static RemoteProtocolManager inst;
    return inst;
}

RemoteSessionRecord RemoteProtocolManager::create_session(const char* seed_hint, unsigned int ttl_ms) {
    std::lock_guard<std::mutex> lk(g_remote_mtx);
    prune_expired_locked();
    RemoteSessionRecord r{};
    unsigned long long n = now_tick();
    char sid[96] = {0};
    snprintf(sid, sizeof(sid), "rp-%08llX-%04u", n & 0xFFFFFFFFull, g_seq++);
    r.session_id = sid;
    r.auth_token = make_token(seed_hint);
    r.expire_tick = n + ttl_ms;
    r.state = "pending";
    g_sessions[r.session_id] = r;
    g_recent.push_back(r);
    if (g_recent.size() > 64) g_recent.erase(g_recent.begin(), g_recent.end() - 64);
    return r;
}

bool RemoteProtocolManager::verify_token(const char* session_id, const char* token, const char* expected_state, bool* expired) {
    std::lock_guard<std::mutex> lk(g_remote_mtx);
    prune_expired_locked();
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
    if (rec.auth_token != token) return false;

    // One-time acceptance token rotation: reduce replay risk on pending_accept.
    if (rec.state == "pending_accept") {
        it->second.auth_token = make_token("rotated");
        g_recent.push_back(it->second);
        if (g_recent.size() > 64) g_recent.erase(g_recent.begin(), g_recent.end() - 64);
    }
    return true;
}

void RemoteProtocolManager::update_state(const char* session_id, const char* new_state) {
    const char* ignore_reason = nullptr;
    try_update_state(session_id, new_state, &ignore_reason);
}

bool RemoteProtocolManager::try_update_state(const char* session_id, const char* new_state, const char** reason) {
    std::lock_guard<std::mutex> lk(g_remote_mtx);
    prune_expired_locked();
    if (reason) *reason = "ok";
    if (!session_id || !new_state) {
        if (reason) *reason = "invalid_input";
        return false;
    }
    auto it = g_sessions.find(session_id);
    if (it == g_sessions.end()) {
        if (reason) *reason = "session_not_found";
        return false;
    }
    const std::string cur = it->second.state;
    const std::string next = new_state;
    auto allow = [&](const char* from, const char* to) {
        return cur == from && next == to;
    };
    bool valid =
        allow("pending", "requesting") ||
        allow("requesting", "pending_accept") ||
        allow("pending_accept", "connected") ||
        allow("pending_accept", "closed") ||
        allow("connected", "closed") ||
        allow("pending", "closed") ||
        allow("requesting", "closed");
    if (!valid && cur == next) valid = true;
    if (!valid) {
        if (reason) *reason = "invalid_transition";
        return false;
    }
    it->second.state = next;
    g_recent.push_back(it->second);
    if (g_recent.size() > 64) g_recent.erase(g_recent.begin(), g_recent.end() - 64);
    return true;
}

void RemoteProtocolManager::close_session(const char* session_id) {
    std::lock_guard<std::mutex> lk(g_remote_mtx);
    prune_expired_locked();
    if (!session_id) return;
    auto it = g_sessions.find(session_id);
    if (it == g_sessions.end()) return;
    it->second.state = "closed";
    g_recent.push_back(it->second);
    if (g_recent.size() > 64) g_recent.erase(g_recent.begin(), g_recent.end() - 64);
    g_sessions.erase(it);
}

std::vector<RemoteSessionRecord> RemoteProtocolManager::recent_records(int max_count) const {
    std::lock_guard<std::mutex> lk(g_remote_mtx);
    prune_expired_locked();
    std::vector<RemoteSessionRecord> out = g_recent;
    if (max_count > 0 && (int)out.size() > max_count) {
        out.erase(out.begin(), out.end() - max_count);
    }
    return out;
}
