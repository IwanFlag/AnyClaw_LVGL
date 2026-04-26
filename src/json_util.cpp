#include "app.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cctype>

/* Find a top-level key in JSON and return position after the colon */
static const char* json_find_key(const char* json, const char* key) {
    if (!json || !key) return nullptr;

    const size_t key_len = strlen(key);
    int depth = 0;
    const char* p = json;

    while (*p) {
        if (*p == '{' || *p == '[') {
            depth++;
            p++;
            continue;
        }
        if (*p == '}' || *p == ']') {
            if (depth > 0) depth--;
            p++;
            continue;
        }

        if (*p == '"') {
            /* Parse one JSON string token (handles escapes). */
            const char* s = p + 1;
            bool esc = false;
            while (*s) {
                if (esc) {
                    esc = false;
                } else if (*s == '\\') {
                    esc = true;
                } else if (*s == '"') {
                    break;
                }
                s++;
            }
            if (!*s) return nullptr; /* malformed JSON */

            /* At root object depth, treat string token followed by ':' as a key candidate. */
            if (depth == 1) {
                size_t cur_len = (size_t)(s - (p + 1));
                if (cur_len == key_len && strncmp(p + 1, key, key_len) == 0) {
                    const char* after = s + 1;
                    while (*after == ' ' || *after == '\t' || *after == '\n' || *after == '\r') after++;
                    if (*after == ':') {
                        after++;
                        while (*after == ' ' || *after == '\t' || *after == '\n' || *after == '\r') after++;
                        return after;
                    }
                }
            }

            p = s + 1;
            continue;
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
