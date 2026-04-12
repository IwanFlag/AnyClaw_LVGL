/*
 * ═══════════════════════════════════════════════════════════════
 *  AnyClaw LVGL — License Validation Implementation
 *  Uses Windows BCrypt for HMAC-SHA256
 * ═══════════════════════════════════════════════════════════════
 */

#include "license.h"
#include "app_log.h"
#include "app.h"  /* For json_extract_int/string declarations */

#include <windows.h>
#include <bcrypt.h>
#include <iphlpapi.h>
#include <cstring>
#include <cstdio>
#include <fstream>
#include <string>
#include <ctime>
#include <cstdlib>
#include <cctype>

#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "iphlpapi.lib")

/* ═══ Constants ═══ */
static const char* LICENSE_SECRET = "AnyClaw-2026-LOVE-GARLIC";
static const int BASE_DURATION_H = 12;

/* ═══ State ═══ */
static int   g_license_seq    = 0;
static int64_t g_license_expiry = 0;  /* Unix timestamp */
static char g_hw_key[128] = {0};
static char g_tm_key[128] = {0};
static int64_t g_tm_expiry = 0;
static bool  g_license_loaded  = false;

/* ═══ Helpers ═══ */

/* Get config path */
static std::string lic_get_config_path() {
    const char* appdata = std::getenv("APPDATA");
    if (appdata) {
        std::string dir = std::string(appdata) + "\\AnyClaw_LVGL";
        return dir + "\\config.json";
    }
    return "AnyClaw_config.json";
}

/* HMAC-SHA256 using Windows BCrypt */
static bool hmac_sha256(const unsigned char* key, int key_len,
                        const unsigned char* data, int data_len,
                        unsigned char* out_mac, int mac_len = 32) {
    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_HASH_HANDLE hHash = NULL;
    NTSTATUS status;

    status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG);
    if (status < 0) return false;

    status = BCryptCreateHash(hAlg, &hHash, NULL, 0, (PUCHAR)key, key_len, 0);
    if (status < 0) { BCryptCloseAlgorithmProvider(hAlg, 0); return false; }

    BCryptHashData(hHash, (PUCHAR)data, data_len, 0);
    status = BCryptFinishHash(hHash, out_mac, mac_len, 0);

    BCryptDestroyHash(hHash);
    BCryptCloseAlgorithmProvider(hAlg, 0);
    return status >= 0;
}

static std::string normalize_mac(const char* input) {
    std::string out;
    if (!input) return out;
    for (const char* p = input; *p; ++p) {
        if (std::isxdigit((unsigned char)*p)) {
            out.push_back((char)std::toupper((unsigned char)*p));
        }
    }
    return out;
}

static bool get_primary_mac_raw(std::string& mac12) {
    mac12.clear();
    ULONG len = sizeof(IP_ADAPTER_INFO);
    IP_ADAPTER_INFO* info = (IP_ADAPTER_INFO*)std::malloc(len);
    if (!info) return false;
    DWORD ret = GetAdaptersInfo(info, &len);
    if (ret == ERROR_BUFFER_OVERFLOW) {
        IP_ADAPTER_INFO* bigger = (IP_ADAPTER_INFO*)std::realloc(info, len);
        if (!bigger) {
            std::free(info);
            return false;
        }
        info = bigger;
        ret = GetAdaptersInfo(info, &len);
    }
    if (ret != NO_ERROR) {
        std::free(info);
        return false;
    }
    for (IP_ADAPTER_INFO* p = info; p; p = p->Next) {
        if (p->Type == MIB_IF_TYPE_LOOPBACK) continue;
        if (p->AddressLength < 6) continue;
        char b[13] = {0};
        snprintf(b, sizeof(b), "%02X%02X%02X%02X%02X%02X",
                 p->Address[0], p->Address[1], p->Address[2],
                 p->Address[3], p->Address[4], p->Address[5]);
        mac12 = b;
        std::free(info);
        return true;
    }
    std::free(info);
    return false;
}

static std::string sign_payload_hex12(const std::string& payload) {
    unsigned char mac[32] = {0};
    if (!hmac_sha256((const unsigned char*)LICENSE_SECRET, (int)strlen(LICENSE_SECRET),
                     (const unsigned char*)payload.data(), (int)payload.size(), mac, 32)) {
        return "";
    }
    char hex[13] = {0};
    for (int i = 0; i < 6; ++i) {
        snprintf(hex + i * 2, 3, "%02X", mac[i]);
    }
    return std::string(hex);
}

static std::string trim_copy(const char* input) {
    std::string s = input ? input : "";
    size_t l = 0, r = s.size();
    while (l < r && std::isspace((unsigned char)s[l])) l++;
    while (r > l && std::isspace((unsigned char)s[r - 1])) r--;
    return s.substr(l, r - l);
}

static bool parse_prefixed_key(const char* key, std::string& kind, std::string& body) {
    std::string s = trim_copy(key);
    if (s.size() < 4) return false;
    for (char& ch : s) ch = (char)std::toupper((unsigned char)ch);
    if (s.rfind("HW-", 0) == 0) {
        kind = "HW";
    } else if (s.rfind("TM-", 0) == 0) {
        kind = "TM";
    } else {
        return false;
    }
    body.clear();
    for (size_t i = 3; i < s.size(); ++i) {
        if (s[i] == '-' || s[i] == ' ') continue;
        if (!std::isxdigit((unsigned char)s[i])) return false;
        body.push_back(s[i]);
    }
    return !body.empty();
}

static bool verify_hw_key_core(const std::string& key_body, std::string* out_error = nullptr) {
    if (key_body.size() != 24) {
        if (out_error) *out_error = "Invalid HW key length";
        return false;
    }
    std::string mac = key_body.substr(0, 12);
    std::string sig = key_body.substr(12, 12);
    std::string current_mac;
    if (!get_primary_mac_raw(current_mac)) {
        if (out_error) *out_error = "Cannot read machine MAC";
        return false;
    }
    if (mac != current_mac) {
        if (out_error) *out_error = "HW key does not match this machine";
        return false;
    }
    std::string expect = sign_payload_hex12(std::string("HW|") + mac);
    if (expect.empty() || expect != sig) {
        if (out_error) *out_error = "Invalid HW signature";
        return false;
    }
    return true;
}

static bool verify_tm_key_core(const std::string& key_body, int64_t* out_expiry = nullptr, std::string* out_error = nullptr) {
    if (key_body.size() != 20) {
        if (out_error) *out_error = "Invalid TM key length";
        return false;
    }
    std::string expiry_hex = key_body.substr(0, 8);
    std::string sig = key_body.substr(8, 12);
    std::string expect = sign_payload_hex12(std::string("TM|") + expiry_hex);
    if (expect.empty() || expect != sig) {
        if (out_error) *out_error = "Invalid TM signature";
        return false;
    }
    int64_t expiry = 0;
    for (char c : expiry_hex) {
        expiry <<= 4;
        if (c >= '0' && c <= '9') expiry |= (c - '0');
        else expiry |= (c - 'A' + 10);
    }
    if (expiry <= (int64_t)time(NULL)) {
        if (out_error) *out_error = "TM key has expired";
        return false;
    }
    if (out_expiry) *out_expiry = expiry;
    return true;
}

static void remove_json_field(std::string& s, const char* field) {
    size_t pos = s.find(field);
    if (pos == std::string::npos) return;
    size_t line_start = s.rfind('\n', pos);
    if (line_start == std::string::npos) line_start = 0;
    else line_start++;
    size_t val_end = s.find(',', pos);
    if (val_end == std::string::npos) val_end = s.find('\n', pos);
    if (val_end == std::string::npos) val_end = s.size();
    else val_end++;
    s.erase(line_start, val_end - line_start);
}

static void license_save_to_config() {
    std::string path = lic_get_config_path();
    std::string content;
    {
        std::ifstream rf(path);
        if (rf.is_open()) {
            content.assign((std::istreambuf_iterator<char>(rf)), std::istreambuf_iterator<char>());
            rf.close();
        }
    }
    if (content.empty()) {
        content = "{\n}\n";
    }
    remove_json_field(content, "\"license_seq\"");
    remove_json_field(content, "\"license_expiry\"");
    remove_json_field(content, "\"hw_key\"");
    remove_json_field(content, "\"tm_key\"");
    remove_json_field(content, "\"tm_expiry\"");
    size_t last_brace = content.rfind('}');
    if (last_brace == std::string::npos) {
        content = "{\n}\n";
        last_brace = content.rfind('}');
    }
    char buf[512];
    snprintf(buf, sizeof(buf),
             "  \"license_seq\": %d,\n"
             "  \"license_expiry\": %lld,\n"
             "  \"hw_key\": \"%s\",\n"
             "  \"tm_key\": \"%s\",\n"
             "  \"tm_expiry\": %lld,\n",
             g_license_seq, (long long)g_license_expiry,
             g_hw_key, g_tm_key, (long long)g_tm_expiry);
    content.insert(last_brace, buf);
    std::ofstream wf(path);
    if (wf.is_open()) {
        wf << content;
        wf.close();
    }
}

/* Base32 decode (RFC 4648, no padding required) */
static int base32_decode(const char* input, unsigned char* output, int output_max) {
    /* Build lookup table (thread-safe via static init) */
    static const unsigned char* b32table = []() -> unsigned char* {
        static unsigned char table[256];
        memset(table, 0xFF, sizeof(table));
        for (int i = 0; i < 26; i++) table['A' + i] = (unsigned char)i;
        for (int i = 0; i < 26; i++) table['a' + i] = (unsigned char)i;
        for (int i = 0; i < 10; i++) table['2' + i] = (unsigned char)(26 + i);
        return table;
    }();

    int in_len = (int)strlen(input);
    int out_idx = 0;
    int bits = 0;
    int value = 0;

    for (int i = 0; i < in_len; i++) {
        unsigned char c = (unsigned char)input[i];
        if (c == '=' || c == '-' || c == ' ') continue;
        unsigned char val = b32table[c];
        if (val == 0xFF) return -1;  /* Invalid character */

        value = (value << 5) | val;
        bits += 5;

        if (bits >= 8) {
            if (out_idx >= output_max) return -1;
            bits -= 8;
            output[out_idx++] = (unsigned char)(value >> bits);
            value &= (1 << bits) - 1;
        }
    }

    return out_idx;
}

/* ═══ License Functions ═══ */

void license_init() {
    if (g_license_loaded) return;

    /* Read from config.json */
    std::ifstream f(lic_get_config_path());
    if (!f.is_open()) {
        LOG_D("License", "No config file, using defaults");
        g_license_loaded = true;
        return;
    }
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    f.close();

    g_license_seq = json_extract_int(content.c_str(), "license_seq", 0);
    /* FIX Bug 7: Use json_extract_int64 instead of manual parsing */
    g_license_expiry = json_extract_int64(content.c_str(), "license_expiry", 0);
    json_extract_string(content.c_str(), "hw_key", g_hw_key, sizeof(g_hw_key));
    json_extract_string(content.c_str(), "tm_key", g_tm_key, sizeof(g_tm_key));
    g_tm_expiry = json_extract_int64(content.c_str(), "tm_expiry", 0);

    g_license_loaded = true;
    LOG_I("License", "Loaded: seq=%d expiry=%lld hw=%d tm=%d",
          g_license_seq, (long long)g_license_expiry,
          g_hw_key[0] ? 1 : 0, g_tm_key[0] ? 1 : 0);
}

bool license_is_valid() {
    license_init();
    std::string kind, body, err;
    if (parse_prefixed_key(g_hw_key, kind, body) && kind == "HW" && verify_hw_key_core(body, &err)) {
        return true;
    }
    if (parse_prefixed_key(g_tm_key, kind, body) && kind == "TM") {
        int64_t exp = 0;
        if (verify_tm_key_core(body, &exp, &err)) return true;
    }
    if (g_license_expiry > 0) {
        return (int64_t)time(NULL) < g_license_expiry;
    }
    return false;
}

int64_t license_remaining_seconds() {
    license_init();
    std::string kind, body, err;
    if (parse_prefixed_key(g_hw_key, kind, body) && kind == "HW" && verify_hw_key_core(body, &err)) {
        return 365LL * 24LL * 3600LL * 50LL;
    }
    if (parse_prefixed_key(g_tm_key, kind, body) && kind == "TM") {
        int64_t exp = 0;
        if (verify_tm_key_core(body, &exp, &err)) {
            int64_t remain = exp - (int64_t)time(NULL);
            return remain > 0 ? remain : 0;
        }
    }
    if (g_license_expiry <= 0) return 0;
    int64_t now = (int64_t)time(NULL);
    int64_t remaining = g_license_expiry - now;
    return remaining > 0 ? remaining : 0;
}

int license_get_seq() {
    license_init();
    return g_license_seq;
}

void license_get_remaining_str(char* buf, int buf_size) {
    std::string kind, body, err;
    if (parse_prefixed_key(g_hw_key, kind, body) && kind == "HW" && verify_hw_key_core(body, &err)) {
        snprintf(buf, buf_size, "Perpetual (HW)");
        return;
    }
    int64_t secs = license_remaining_seconds();
    if (secs <= 0) {
        snprintf(buf, buf_size, "Expired");
        return;
    }
    double hours = secs / 3600.0;
    if (hours >= 24) {
        snprintf(buf, buf_size, "%.0fd remaining", hours / 24.0);
    } else if (hours >= 1) {
        snprintf(buf, buf_size, "%.1fh remaining", hours);
    } else {
        snprintf(buf, buf_size, "%lldm remaining", (long long)(secs / 60));
    }
}

bool license_activate(const char* key, int* out_seq, int* out_expiry_hours, const char** out_error) {
    license_init();

    std::string kind, body;
    if (parse_prefixed_key(key, kind, body)) {
        std::string err;
        if (kind == "HW") {
            if (!verify_hw_key_core(body, &err)) {
                if (out_error) *out_error = err.c_str();
                return false;
            }
            snprintf(g_hw_key, sizeof(g_hw_key), "HW-%s", body.c_str());
            license_save_to_config();
            if (out_seq) *out_seq = 0;
            if (out_expiry_hours) *out_expiry_hours = 24 * 365 * 50;
            LOG_I("License", "Activated HW key for current machine");
            return true;
        }
        if (kind == "TM") {
            int64_t exp = 0;
            if (!verify_tm_key_core(body, &exp, &err)) {
                if (out_error) *out_error = err.c_str();
                return false;
            }
            snprintf(g_tm_key, sizeof(g_tm_key), "TM-%s", body.c_str());
            g_tm_expiry = exp;
            license_save_to_config();
            if (out_seq) *out_seq = 0;
            if (out_expiry_hours) *out_expiry_hours = (int)((exp - (int64_t)time(NULL)) / 3600);
            LOG_I("License", "Activated TM key exp=%lld", (long long)exp);
            return true;
        }
    }

    /* Clean up key: remove whitespace */
    char clean_key[128];
    int ci = 0;
    for (int i = 0; key[i] && ci < 120; i++) {
        char c = key[i];
        if (c == '-' || c == ' ') continue;
        clean_key[ci++] = c >= 'a' && c <= 'z' ? c - 32 : c;  /* To uppercase */
    }
    clean_key[ci] = '\0';

    if (ci < 10) {
        if (out_error) *out_error = "Key too short";
        return false;
    }

    /* Base32 decode */
    unsigned char raw[32];
    int raw_len = base32_decode(clean_key, raw, sizeof(raw));
    if (raw_len != 18) {
        if (out_error) *out_error = "Invalid key format";
        return false;
    }

    /* Extract payload (10 bytes) and MAC (8 bytes) */
    unsigned char payload[10];
    memcpy(payload, raw, 10);

    /* Verify HMAC-SHA256 */
    unsigned char mac_calc[32];
    if (!hmac_sha256((const unsigned char*)LICENSE_SECRET, (int)strlen(LICENSE_SECRET),
                     payload, 10, mac_calc, 32)) {
        if (out_error) *out_error = "Crypto error";
        return false;
    }

    if (memcmp(raw + 10, mac_calc, 8) != 0) {
        if (out_error) *out_error = "Invalid key (HMAC mismatch)";
        return false;
    }

    /* Parse payload: seq(2B big-endian) + expiry(8B big-endian) */
    int seq = (payload[0] << 8) | payload[1];
    int64_t expiry = 0;
    for (int i = 2; i < 10; i++) {
        expiry = (expiry << 8) | payload[i];
    }

    /* Check expiry */
    int64_t now = (int64_t)time(NULL);
    if (expiry <= now) {
        if (out_error) *out_error = "Key has expired";
        return false;
    }

    /* Check sequence: must be >= current */
    if (seq < g_license_seq) {
        if (out_error) *out_error = "Key sequence too low (already used)";
        return false;
    }

    /* Accept the key! */
    g_license_seq = seq;
    g_license_expiry = expiry;

    license_save_to_config();

    LOG_I("License", "Activated: seq=%d expiry=%lld (%.1fh)", seq, (long long)expiry,
          (expiry - now) / 3600.0);

    if (out_seq) *out_seq = seq;
    if (out_expiry_hours) *out_expiry_hours = (int)((expiry - now) / 3600);

    return true;
}

bool license_get_machine_id(char* buf, int buf_size) {
    if (!buf || buf_size <= 0) return false;
    std::string mac;
    if (!get_primary_mac_raw(mac) || mac.size() != 12) {
        buf[0] = '\0';
        return false;
    }
    snprintf(buf, buf_size, "%c%c:%c%c:%c%c:%c%c:%c%c:%c%c",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
             mac[6], mac[7], mac[8], mac[9], mac[10], mac[11]);
    return true;
}
