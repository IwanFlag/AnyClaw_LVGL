#include "kb_store.h"

#include <algorithm>
#include <fstream>
#include <mutex>

namespace {
std::mutex g_kb_mtx;
std::vector<KbDoc> g_docs;
}

KbStore& KbStore::instance() {
    static KbStore inst;
    return inst;
}

bool KbStore::add_source_file(const char* path, std::string& err) {
    err.clear();
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
            return true;
        }
    }
    g_docs.push_back({path, content});
    return true;
}

std::vector<KbDoc> KbStore::search_keyword(const char* keyword, int limit) {
    std::vector<KbDoc> out;
    if (!keyword || !keyword[0]) return out;
    std::string k = keyword;
    std::transform(k.begin(), k.end(), k.begin(), ::tolower);
    std::lock_guard<std::mutex> lk(g_kb_mtx);
    for (const auto& d : g_docs) {
        std::string text = d.content;
        std::transform(text.begin(), text.end(), text.begin(), ::tolower);
        if (text.find(k) != std::string::npos) out.push_back(d);
        if (limit > 0 && (int)out.size() >= limit) break;
    }
    return out;
}

int KbStore::doc_count() const {
    std::lock_guard<std::mutex> lk(g_kb_mtx);
    return (int)g_docs.size();
}
