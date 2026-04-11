#ifndef KB_STORE_H
#define KB_STORE_H

#include <string>
#include <vector>

struct KbDoc {
    std::string path;
    std::string content;
};

class KbStore {
public:
    static KbStore& instance();
    bool ensure_loaded();
    bool add_source_file(const char* path, std::string& err);
    bool add_source_dir(const char* dir_path, int* out_added, std::string& err);
    bool export_manifest(const char* out_dir, std::string& err);
    std::vector<KbDoc> search_keyword(const char* keyword, int limit = 5);
    std::string build_context_snippet(const char* keyword, int max_docs = 3, int max_chars = 1200);
    int doc_count() const;
    int source_count() const;

private:
    KbStore() = default;
};

#endif
