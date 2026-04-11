#pragma once

#include <cstddef>

enum class WorkRenderType {
    Terminal = 0,
    Preview = 1,
    Diff = 2,
    Table = 3,
    Web = 4,
    File = 5
};

struct OutputRenderer {
    WorkRenderType type;
    bool (*can_handle)(const char* tool_name, const char* payload);
    void (*append)(char* doc_buf, size_t doc_cap, const char* payload);
};

const char* output_renderer_name(WorkRenderType type);
WorkRenderType output_renderer_pick(const char* tool_name, const char* payload);
void output_renderer_append_markdown(char* doc_buf,
                                     size_t doc_cap,
                                     const char* tool_name,
                                     const char* payload);
const OutputRenderer* output_renderer_registry(size_t* out_count);
