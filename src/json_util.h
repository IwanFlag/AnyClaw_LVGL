#ifndef JSON_UTIL_H
#define JSON_UTIL_H

#include <cstdint>

int json_extract_int(const char* json, const char* key, int default_val);
int64_t json_extract_int64(const char* json, const char* key, int64_t default_val);
bool json_extract_string(const char* json, const char* key, char* out, int out_size);

#endif // JSON_UTIL_H
