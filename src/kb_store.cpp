#include "kb_store.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
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

int count_occurs(const std::string& text, const std::string& key) {
    if (text.empty() || key.empty()) return 0;
    int c = 0;
    size_t pos = 0;
    while ((pos = text.find(key, pos)) != std::string::npos) {
        ++c;
        pos += key.size();
    }
    return c;
}

std::string file_name_only(const std::string& path) {
    std::filesystem::path p(path);
    return p.filename().string();
}

std::string collapse_spaces(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    bool prev_sp = false;
    for (char ch : s) {
        char t = (ch == '\r' || ch == '\n' || ch == '\t') ? ' ' : ch;
        bool sp = (t == ' ');
        if (sp && prev_sp) continue;
        out.push_back(t);
        prev_sp = sp;
    }
    return out;
}

std::string snippet_around_hit(const std::string& origin, const std::string& key_lower, int max_len) {
    if (origin.empty()) return "";
    std::string lower = to_lower(origin);
    size_t p = lower.find(key_lower);
    if (p == std::string::npos) {
        std::string s = collapse_spaces(origin);
        if ((int)s.size() > max_len) s.resize(max_len);
        return s;
    }
    int left = (int)p - max_len / 3;
    if (left < 0) left = 0;
    int right = left + max_len;
    if (right > (int)origin.size()) {
        right = (int)origin.size();
        left = std::max(0, right - max_len);
    }
    std::string s = origin.substr(left, right - left);
    s = collapse_spaces(s);
    if (left > 0) s = "... " + s;
    if (right < (int)origin.size()) s += " ...";
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
    struct HitScore {
        int score = 0;
        KbDoc doc;
    };
    std::vector<HitScore> ranked;
    std::lock_guard<std::mutex> lk(g_kb_mtx);
    for (const auto& d : g_docs) {
        std::string text = to_lower(d.content);
        size_t pos = text.find(k);
        if (pos == std::string::npos) continue;
        int freq = count_occurs(text, k);
        int score = 100;
        score += std::min(120, freq * 15);
        score += std::max(0, 40 - (int)(pos / 120));
        std::string fn = to_lower(file_name_only(d.path));
        if (fn.find(k) != std::string::npos) score += 80;
        ranked.push_back({score, d});
    }
    std::sort(ranked.begin(), ranked.end(), [](const HitScore& a, const HitScore& b) {
        if (a.score != b.score) return a.score > b.score;
        return a.doc.path < b.doc.path;
    });
    for (const auto& it : ranked) {
        out.push_back(it.doc);
        if (limit > 0 && (int)out.size() >= limit) break;
    }
    return out;
}

std::string KbStore::build_context_snippet(const char* keyword, int max_docs, int max_chars) {
    auto docs = search_keyword(keyword, max_docs);
    if (docs.empty()) return "";
    std::string key = keyword ? keyword : "";
    key = to_lower(key);
    std::ostringstream oss;
    oss << "[KB Context]\n";
    int used = 0;
    std::vector<size_t> seen_hashes;
    for (size_t i = 0; i < docs.size(); ++i) {
        const auto& d = docs[i];
        std::string chunk = snippet_around_hit(d.content, key, 260);
        size_t h = std::hash<std::string>{}(to_lower(chunk));
        if (std::find(seen_hashes.begin(), seen_hashes.end(), h) != seen_hashes.end()) {
            continue;
        }
        seen_hashes.push_back(h);
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
