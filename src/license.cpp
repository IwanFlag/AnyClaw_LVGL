/*
 * ═══════════════════════════════════════════════════════════════
 *  AnyClaw LVGL — License Validation Implementation
 *  Uses Windows BCrypt for HMAC-SHA256
 * ═══════════════════════════════════════════════════════════════
 */

#include "license.h"
#include "app_log.h"
#include "json_util.cpp"  /* For json_extract_int/string */

#include <windows.h>
#include <bcrypt.h>
#include <cstring>
#include <cstdio>
#include <fstream>
#include <string>

#pragma comment(lib, "bcrypt.lib")

/* ═══ Constants ═══ */
static const char* LICENSE_SECRET = "AnyClaw-2026-LOVE-GARLIC";
static const int BASE_DURATION_H = 12;

/* ═══ State ═══ */
static int   g_license_seq    = 0;
static int64_t g_license_expiry = 0;  /* Unix timestamp */
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

/* Base32 decode (RFC 4648, no padding required) */
static int base32_decode(const char* input, unsigned char* output, int output_max) {
    /* Build lookup table */
    static unsigned char b32table[256];
    static bool table_init = false;
    if (!table_init) {
        memset(b32table, 0xFF, sizeof(b32table));
        for (int i = 0; i < 26; i++) b32table['A' + i] = i;
        for (int i = 0; i < 26; i++) b32table['a' + i] = i;
        for (int i = 0; i < 10; i++) b32table['2' + i] = 26 + i;
        table_init = true;
    }

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
    /* license_expiry is stored as a large number (Unix timestamp) */
    /* json_extract_int only handles int, so we parse manually */
    size_t pos = content.find("\"license_expiry\"");
    if (pos != std::string::npos) {
        pos = content.find(':', pos);
        if (pos != std::string::npos) {
            pos++;
            while (pos < content.size() && (content[pos] == ' ' || content[pos] == '"')) pos++;
            g_license_expiry = 0;
            while (pos < content.size() && content[pos] >= '0' && content[pos] <= '9') {
                g_license_expiry = g_license_expiry * 10 + (content[pos] - '0');
                pos++;
            }
        }
    }

    g_license_loaded = true;
    LOG_I("License", "Loaded: seq=%d expiry=%lld", g_license_seq, (long long)g_license_expiry);
}

bool license_is_valid() {
    license_init();
    if (g_license_expiry <= 0) return true;  /* No license data = trial period */
    return (int64_t)time(NULL) < g_license_expiry;
}

int64_t license_remaining_seconds() {
    license_init();
    if (g_license_expiry <= 0) return 9999999;  /* No license = unlimited trial */
    int64_t now = (int64_t)time(NULL);
    int64_t remaining = g_license_expiry - now;
    return remaining > 0 ? remaining : 0;
}

int license_get_seq() {
    license_init();
    return g_license_seq;
}

void license_get_remaining_str(char* buf, int buf_size) {
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

    /* Save to config */
    {
        std::string path = lic_get_config_path();
        std::string content;
        {
            std::ifstream rf(path);
            if (rf.is_open()) {
                content.assign((std::istreambuf_iterator<char>(rf)), std::istreambuf_iterator<char>());
                rf.close();
            }
        }

        /* Remove old license fields if present */
        auto remove_field = [](std::string& s, const char* field) {
            size_t pos = s.find(field);
            if (pos == std::string::npos) return;
            /* Find start of line */
            size_t line_start = s.rfind('\n', pos);
            if (line_start == std::string::npos) line_start = 0;
            else line_start++; /* skip the \n */
            /* Find end of value (next comma or newline) */
            size_t val_end = s.find(',', pos);
            if (val_end == std::string::npos) val_end = s.find('\n', pos);
            if (val_end == std::string::npos) val_end = s.size();
            else val_end++; /* include delimiter */
            s.erase(line_start, val_end - line_start);
        };

        remove_field(content, "\"license_seq\"");
        remove_field(content, "\"license_expiry\"");

        /* Insert before last } */
        size_t last_brace = content.rfind('}');
        if (last_brace != std::string::npos) {
            char buf[256];
            snprintf(buf, sizeof(buf),
                     "  \"license_seq\": %d,\n  \"license_expiry\": %lld,\n",
                     seq, (long long)expiry);
            content.insert(last_brace, buf);
        }

        std::ofstream wf(path);
        if (wf.is_open()) {
            wf << content;
            wf.close();
        }
    }

    LOG_I("License", "Activated: seq=%d expiry=%lld (%.1fh)", seq, (long long)expiry,
          (expiry - now) / 3600.0);

    if (out_seq) *out_seq = seq;
    if (out_expiry_hours) *out_expiry_hours = (int)((expiry - now) / 3600);

    return true;
}
