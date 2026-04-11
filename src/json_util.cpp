#include "app.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>

/* Find a top-level key in JSON and return position after the colon */
static const char* json_find_key(const char* json, const char* key) {
    if (!json || !key) return nullptr;
    char pattern[256];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    
    int depth = 0;
    bool in_string = false;
    bool escape = false;
    const char* p = json;
    
    while (*p) {
        if (escape) { escape = false; p++; continue; }
        if (*p == '\\') { escape = true; p++; continue; }
        if (*p == '"') { in_string = !in_string; p++; continue; }
        if (in_string) { p++; continue; }
        
        if (*p == '{' || *p == '[') depth++;
        else if (*p == '}' || *p == ']') depth--;
        
        if (depth == 1 && strncmp(p, pattern, strlen(pattern)) == 0) {
            const char* after = p + strlen(pattern);
            /* FIX P4: Ensure key is a complete token (not a prefix of a longer key) */
            if (*after != '"' && *after != '\0') { p++; continue; }
            while (*after == ' ' || *after == '\t' || *after == '\n' || *after == '\r') after++;
            if (*after == ':') {
                after++;
                while (*after == ' ' || *after == '\t' || *after == '\n' || *after == '\r') after++;
                return after;
            }
        }
        p++;
    }
    return nullptr;
}

int json_extract_int(const char* json, const char* key, int default_val) {
    const char* val = json_find_key(json, key);
    if (!val) return default_val;
    bool neg = false;
    if (*val == '-') { neg = true; val++; }
    int result = 0;
    while (*val >= '0' && *val <= '9') {
        result = result * 10 + (*val - '0');
        val++;
    }
    return neg ? -result : result;
}

/* Extract int64 value (for large timestamps like license_expiry) */
int64_t json_extract_int64(const char* json, const char* key, int64_t default_val) {
    const char* val = json_find_key(json, key);
    if (!val) return default_val;
    bool neg = false;
    if (*val == '-') { neg = true; val++; }
    int64_t result = 0;
    while (*val >= '0' && *val <= '9') {
        result = result * 10 + (*val - '0');
        val++;
    }
    return neg ? -result : result;
}

bool json_extract_string(const char* json, const char* key, char* out, int out_size) {
    if (!out || out_size <= 0) return false;
    out[0] = '\0';
    const char* val = json_find_key(json, key);
    if (!val || *val != '"') return false;
    val++;
    
    int i = 0;
    while (*val && *val != '"' && i < out_size - 1) {
        if (*val == '\\' && *(val + 1)) {
            val++;
            switch (*val) {
                case '"': out[i++] = '"'; break;
                case '\\': out[i++] = '\\'; break;
                case '/': out[i++] = '/'; break;
                case 'n': out[i++] = '\n'; break;
                case 't': out[i++] = '\t'; break;
                case 'r': out[i++] = '\r'; break;
                default: out[i++] = *val; break;
            }
        } else {
            out[i++] = *val;
        }
        val++;
    }
    out[i] = '\0';
    return true;
}

void secure_zero(void* ptr, size_t size) {
    volatile unsigned char* p = (volatile unsigned char*)ptr;
    while (size--) *p++ = 0;
}
