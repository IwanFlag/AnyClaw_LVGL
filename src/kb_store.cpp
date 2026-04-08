#include "kb_store.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>

namespace {
std::mutex g_kb_mtx;
std::vector<KbDoc> g_docs;
bool g_loaded = false;

std::string kb_index_path() {
    const char* appdata = std::getenv("APPDATA");
    if (!appdata || !appdata[0]) return "kb_index.txt";
    std::filesystem::path p(appdata);
    p /= "AnyClaw_LVGL";
    std::error_code ec;
    std::filesystem::create_directories(p, ec);
    p /= "kb_index.txt";
    return p.string();
}

void save_index_locked() {
    std::ofstream out(kb_index_path(), std::ios::trunc);
    if (!out.is_open()) return;
    for (const auto& d : g_docs) {
        out << d.path << "\n";
    }
}

std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}
}

KbStore& KbStore::instance() {
    static KbStore inst;
    return inst;
}

bool KbStore::ensure_loaded() {
    std::lock_guard<std::mutex> lk(g_kb_mtx);
    if (g_loaded) return true;
    g_loaded = true;
    std::ifstream in(kb_index_path());
    if (!in.is_open()) return true;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::ifstream f(line, std::ios::binary);
        if (!f.is_open()) continue;
        std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        if (content.empty()) continue;
        g_docs.push_back({line, content});
    }
    return true;
}

bool KbStore::add_source_file(const char* path, std::string& err) {
    err.clear();
    ensure_loaded();
    if (!path || !path[0]) {
        err = "empty path";
        return false;
    }
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) {
        err = "open failed";
        return false;
    }
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (content.empty()) {
        err = "empty file";
        return false;
    }
    std::lock_guard<std::mutex> lk(g_kb_mtx);
    for (auto& d : g_docs) {
        if (d.path == path) {
            d.content = content;
            save_index_locked();
            return true;
        }
    }
    g_docs.push_back({path, content});
    save_index_locked();
    return true;
}

std::vector<KbDoc> KbStore::search_keyword(const char* keyword, int limit) {
    std::vector<KbDoc> out;
    ensure_loaded();
    if (!keyword || !keyword[0]) return out;
    std::string k = to_lower(keyword);
    std::lock_guard<std::mutex> lk(g_kb_mtx);
    for (const auto& d : g_docs) {
        std::string text = to_lower(d.content);
        if (text.find(k) != std::string::npos) out.push_back(d);
        if (limit > 0 && (int)out.size() >= limit) break;
    }
    return out;
}

std::string KbStore::build_context_snippet(const char* keyword, int max_docs, int max_chars) {
    auto docs = search_keyword(keyword, max_docs);
    if (docs.empty()) return "";
    std::ostringstream oss;
    oss << "[KB Context]\n";
    int used = 0;
    for (size_t i = 0; i < docs.size(); ++i) {
        const auto& d = docs[i];
        std::string chunk = d.content.substr(0, 360);
        chunk.erase(std::remove(chunk.begin(), chunk.end(), '\r'), chunk.end());
        std::replace(chunk.begin(), chunk.end(), '\n', ' ');
        if ((int)chunk.size() > 300) chunk.resize(300);
        oss << (i + 1) << ". " << d.path << "\n   " << chunk << "\n";
        used += (int)chunk.size();
        if (used >= max_chars) break;
    }
    oss << "[End KB Context]\n";
    return oss.str();
}

int KbStore::doc_count() const {
    const_cast<KbStore*>(this)->ensure_loaded();
    std::lock_guard<std::mutex> lk(g_kb_mtx);
    return (int)g_docs.size();
}
