#include "output_renderer.h"

#include <cstdio>
#include <cstring>
#include <string>

static bool has_any(const char* s, const char* a, const char* b, const char* c) {
    if (!s) return false;
    return (a && strstr(s, a)) || (b && strstr(s, b)) || (c && strstr(s, c));
}

static std::string clip_payload(const char* payload, size_t max_len = 3800) {
    std::string s = payload ? payload : "";
    if (s.size() > max_len) {
        s.resize(max_len);
        s += "\n... [truncated]";
    }
    return s;
}

static void append_terminal(char* doc_buf, size_t doc_cap, const char* payload) {
    std::string s = clip_payload(payload);
    char block[6144] = {0};
    snprintf(block, sizeof(block), "\n## TerminalRenderer\n\n```text\n%s\n```\n", s.c_str());
    strncat(doc_buf, block, doc_cap - strlen(doc_buf) - 1);
}

static void append_preview(char* doc_buf, size_t doc_cap, const char* payload) {
    std::string s = clip_payload(payload);
    char block[6144] = {0};
    snprintf(block, sizeof(block), "\n## PreviewRenderer\n\n%s\n", s.c_str());
    strncat(doc_buf, block, doc_cap - strlen(doc_buf) - 1);
}

static void append_diff(char* doc_buf, size_t doc_cap, const char* payload) {
    std::string s = clip_payload(payload);
    char block[6144] = {0};
    snprintf(block, sizeof(block),
             "\n## DiffRenderer\n\n> 变更预览（`+`新增 / `-`删除）\n\n```diff\n%s\n```\n",
             s.c_str());
    strncat(doc_buf, block, doc_cap - strlen(doc_buf) - 1);
}

static void append_table(char* doc_buf, size_t doc_cap, const char* payload) {
    std::string s = clip_payload(payload);
    char block[6144] = {0};
    snprintf(block, sizeof(block),
             "\n## TableRenderer\n\n> 表格结果\n\n```text\n%s\n```\n",
             s.c_str());
    strncat(doc_buf, block, doc_cap - strlen(doc_buf) - 1);
}

static void append_web(char* doc_buf, size_t doc_cap, const char* payload) {
    std::string s = clip_payload(payload);
    char block[6144] = {0};
    snprintf(block, sizeof(block), "\n## WebRenderer\n\n> Web 结果\n\n%s\n", s.c_str());
    strncat(doc_buf, block, doc_cap - strlen(doc_buf) - 1);
}

static void append_file(char* doc_buf, size_t doc_cap, const char* payload) {
    std::string s = clip_payload(payload);
    char block[6144] = {0};
    snprintf(block, sizeof(block),
             "\n## FileRenderer\n\n> 文件内容/路径结果\n\n```text\n%s\n```\n",
             s.c_str());
    strncat(doc_buf, block, doc_cap - strlen(doc_buf) - 1);
}

static bool can_diff(const char* tool_name, const char* payload) {
    return has_any(tool_name, "diff", "patch", "git_diff") || has_any(payload, "*** Begin Patch", "@@", "diff --git");
}
static bool can_table(const char* tool_name, const char* payload) {
    if (!payload) return false;
    return has_any(payload, "|---", "\t", ",") && has_any(tool_name, "table", "list", "query");
}
static bool can_web(const char* tool_name, const char* payload) {
    return has_any(tool_name, "web", "fetch", "search") || has_any(payload, "http://", "https://", "<html");
}
static bool can_file(const char* tool_name, const char* payload) {
    if (has_any(tool_name, "read_file", "write_file", "glob")) return true;
    if (!payload) return false;
    return (strstr(payload, "\\") || strstr(payload, "/")) &&
           has_any(payload, ".md", ".cpp", ".json") &&
           !has_any(payload, "http://", "https://", "<html");
}
static bool can_terminal(const char* tool_name, const char* payload) {
    return has_any(tool_name, "exec", "shell", "terminal") ||
           has_any(payload, "$ ", "powershell", "exit_code");
}
static bool can_preview(const char*, const char*) { return true; }

static const OutputRenderer k_registry[] = {
    {WorkRenderType::Diff, can_diff, append_diff},
    {WorkRenderType::Table, can_table, append_table},
    {WorkRenderType::Web, can_web, append_web},
    {WorkRenderType::File, can_file, append_file},
    {WorkRenderType::Terminal, can_terminal, append_terminal},
    {WorkRenderType::Preview, can_preview, append_preview},
};

const char* output_renderer_name(WorkRenderType type) {
    switch (type) {
        case WorkRenderType::Terminal: return "terminal";
        case WorkRenderType::Preview:  return "preview";
        case WorkRenderType::Diff:     return "diff";
        case WorkRenderType::Table:    return "table";
        case WorkRenderType::Web:      return "web";
        case WorkRenderType::File:     return "file";
        default: return "preview";
    }
}

WorkRenderType output_renderer_pick(const char* tool_name, const char* payload) {
    for (const auto& r : k_registry) {
        if (r.can_handle && r.can_handle(tool_name, payload)) return r.type;
    }
    return WorkRenderType::Preview;
}

void output_renderer_append_markdown(char* doc_buf,
                                     size_t doc_cap,
                                     const char* tool_name,
                                     const char* payload) {
    if (!doc_buf || doc_cap == 0 || !payload || !payload[0]) return;
    WorkRenderType type = output_renderer_pick(tool_name, payload);
    char block[6144] = {0};
    for (const auto& r : k_registry) {
        if (r.type == type && r.append) {
            r.append(block, sizeof(block), payload);
            break;
        }
    }
    size_t cur = strlen(doc_buf);
    size_t add = strlen(block);
    if (cur + add + 1 >= doc_cap) {
        size_t half = doc_cap / 2;
        memmove(doc_buf, doc_buf + half, half);
        doc_buf[half] = '\0';
    }
    strncat(doc_buf, block, doc_cap - strlen(doc_buf) - 1);
}

const OutputRenderer* output_renderer_registry(size_t* out_count) {
    if (out_count) *out_count = sizeof(k_registry) / sizeof(k_registry[0]);
    return k_registry;
}
