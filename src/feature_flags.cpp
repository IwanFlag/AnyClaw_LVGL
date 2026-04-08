#include "feature_flags.h"
#include "app.h"
#include "app_log.h"
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <algorithm>

/* ═══ Config path ═══ */

static std::string get_config_path() {
    const char* appdata = std::getenv("APPDATA");
    if (!appdata) return "";
    return std::string(appdata) + "\\AnyClaw_LVGL\\config.json";
}

/* ═══ JSON boolean extractor (no existing util for bool) ═══ */

static bool json_extract_bool(const char* json, const char* key, bool default_val) {
    if (!json || !key) return default_val;
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
            while (*after == ' ' || *after == '\t' || *after == '\n' || *after == '\r') after++;
            if (*after == ':') {
                after++;
                while (*after == ' ' || *after == '\t' || *after == '\n' || *after == '\r') after++;
                if (strncmp(after, "true", 4) == 0) return true;
                if (strncmp(after, "false", 5) == 0) return false;
            }
        }
        p++;
    }
    return default_val;
}

/* ═══ Default flag definitions ═══ */

void FeatureFlagManager::init_defaults() {
    if (!flags_.empty()) return;

    flags_.push_back({
        FLAG_SBX_SANDBOX_EXEC,
        "Sandbox Execution",
        false,  /* enabled */
        false,  /* default_value */
        false   /* requires_restart */
    });

    flags_.push_back({
        FLAG_OBS_TRACING,
        "Observability Tracing",
        false,
        false,
        true    /* requires_restart: tracing init is startup-only */
    });

    flags_.push_back({
        FLAG_BOOT_SELF_HEAL,
        "Boot Self-Heal",
        false,
        false,
        false
    });

    flags_.push_back({
        FLAG_MEM_LONG_TERM,
        "Long-Term Memory",
        false,
        false,
        false
    });

    flags_.push_back({
        FLAG_KB_KNOWLEDGE_BASE,
        "Knowledge Base",
        false,
        false,
        false
    });

    flags_.push_back({
        FLAG_REMOTE_COLLAB,
        "Remote Collaboration",
        false,
        false,
        true    /* requires_restart: networking init is startup-only */
    });
}

/* ═══ Flag lookup ═══ */

int FeatureFlagManager::find_flag(const char* flag_name) const {
    for (int i = 0; i < (int)flags_.size(); i++) {
        if (flags_[i].name == flag_name) return i;
    }
    return -1;
}

const FeatureFlag* FeatureFlagManager::get_flag(const char* flag_name) const {
    int idx = find_flag(flag_name);
    return (idx >= 0) ? &flags_[idx] : nullptr;
}

/* ═══ Load / Save ═══ */

bool FeatureFlagManager::load_from_config() {
    init_defaults();

    std::string path = get_config_path();
    std::ifstream f(path);
    if (!f.is_open()) {
        LOG_I("FFLAG", "No config.json, using flag defaults");
        return false;
    }

    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    f.close();

    /* Locate the "feature_flags" object in config.json */
    /* json_find_key is static in json_util.cpp — we re-parse inline */
    const char* p = content.c_str();
    const char* ff_section = nullptr;
    int depth = 0;
    bool in_string = false;
    bool escape = false;

    while (*p) {
        if (escape) { escape = false; p++; continue; }
        if (*p == '\\') { escape = true; p++; continue; }
        if (*p == '"') { in_string = !in_string; p++; continue; }
        if (in_string) { p++; continue; }

        if (*p == '{' || *p == '[') depth++;
        else if (*p == '}' || *p == ']') depth--;

        if (depth == 1 && strncmp(p, "\"feature_flags\"", 15) == 0) {
            const char* after = p + 15;
            while (*after == ' ' || *after == '\t' || *after == '\n' || *after == '\r') after++;
            if (*after == ':') {
                after++;
                while (*after == ' ' || *after == '\t' || *after == '\n' || *after == '\r') after++;
                if (*after == '{') {
                    ff_section = after;
                }
            }
            break;
        }
        p++;
    }

    if (!ff_section) {
        LOG_I("FFLAG", "No feature_flags section in config.json, using defaults");
        return false;
    }

    /* Extract the feature_flags sub-object for individual key lookup.
     * json_extract_bool looks for top-level keys, so we need to parse
     * within the sub-object scope. We extract a substring of the object. */
    int sub_depth = 0;
    const char* obj_start = ff_section;
    const char* obj_end = obj_start;
    bool sub_in_str = false;
    bool sub_esc = false;

    while (*obj_end) {
        if (sub_esc) { sub_esc = false; obj_end++; continue; }
        if (*obj_end == '\\') { sub_esc = true; obj_end++; continue; }
        if (*obj_end == '"') { sub_in_str = !sub_in_str; obj_end++; continue; }
        if (sub_in_str) { obj_end++; continue; }

        if (*obj_end == '{' || *obj_end == '[') sub_depth++;
        else if (*obj_end == '}' || *obj_end == ']') {
            sub_depth--;
            if (sub_depth == 0) { obj_end++; break; }
        }
        obj_end++;
    }

    /* Create a temporary string for the sub-object so json_extract_bool
     * sees depth 0 as the feature_flags scope (it looks for depth 1) */
    std::string sub_json(obj_start, obj_end - obj_start);

    /* Now extract each flag — json_extract_bool looks for depth 1 keys
     * within the passed JSON string. Since sub_json starts with { at pos 0,
     * our keys will be at depth 1. Perfect. */
    for (auto& flag : flags_) {
        flag.enabled = json_extract_bool(sub_json.c_str(), flag.name.c_str(), flag.default_value);
    }

    LOG_I("FFLAG", "Loaded %zu feature flags from config.json", flags_.size());
    return true;
}

bool FeatureFlagManager::save_to_config() const {
    std::string path = get_config_path();
    std::string content;

    /* Read existing config */
    {
        std::ifstream f(path);
        if (f.is_open()) {
            content.assign((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
            f.close();
        }
    }

    /* Build feature_flags JSON block */
    std::string ff_block = "  \"feature_flags\": {\n";
    for (int i = 0; i < (int)flags_.size(); i++) {
        ff_block += "    \"" + flags_[i].name + "\": " +
                    (flags_[i].enabled ? "true" : "false");
        if (i < (int)flags_.size() - 1) ff_block += ",";
        ff_block += "\n";
    }
    ff_block += "  }";

    if (content.empty() || content.find('{') == std::string::npos) {
        /* No config yet — create minimal */
        content = "{\n" + ff_block + "\n}\n";
    } else {
        /* Check if feature_flags section already exists */
        size_t ff_pos = content.find("\"feature_flags\"");
        if (ff_pos != std::string::npos) {
            /* Find the opening brace of feature_flags object */
            size_t obj_start = content.find('{', ff_pos);
            if (obj_start != std::string::npos) {
                /* Find matching closing brace */
                int sub_depth = 0;
                size_t obj_end = obj_start;
                for (size_t j = obj_start; j < content.size(); j++) {
                    if (content[j] == '{') sub_depth++;
                    else if (content[j] == '}') {
                        sub_depth--;
                        if (sub_depth == 0) { obj_end = j; break; }
                    }
                }
                /* Replace entire { ... } with our new block's inner content */
                /* ff_block starts with "  \"feature_flags\": {" — we want just the inner part */
                size_t inner_start = ff_block.find('{');
                std::string inner = ff_block.substr(inner_start);
                content.replace(obj_start, obj_end - obj_start + 1, inner);
            }
        } else {
            /* Insert feature_flags before the last } */
            size_t last_brace = content.rfind('}');
            if (last_brace != std::string::npos) {
                /* Check if the last non-whitespace char before } is a comma */
                size_t check = last_brace;
                while (check > 0 && (content[check - 1] == ' ' || content[check - 1] == '\t' ||
                                     content[check - 1] == '\n' || content[check - 1] == '\r')) {
                    check--;
                }
                std::string insert_str;
                if (check > 0 && content[check - 1] != ',' && content[check - 1] != '{') {
                    insert_str = ",\n" + ff_block + "\n";
                } else {
                    insert_str = ff_block + "\n";
                }
                content.insert(last_brace, insert_str);
            }
        }
    }

    /* Ensure directory exists */
    size_t last_sep = path.find_last_of("\\/");
    if (last_sep != std::string::npos) {
        std::string dir = path.substr(0, last_sep);
        if (!std::filesystem::exists(dir)) {
            try { std::filesystem::create_directories(dir); } catch (...) {}
        }
    }

    std::ofstream f(path);
    if (!f.is_open()) {
        LOG_E("FFLAG", "Failed to write config.json");
        return false;
    }

    f << content;
    f.close();
    LOG_I("FFLAG", "Saved feature flags to config.json");
    return true;
}

/* ═══ Get / Set ═══ */

bool FeatureFlagManager::is_enabled(const char* flag_name) const {
    int idx = find_flag(flag_name);
    if (idx < 0) return false;
    return flags_[idx].enabled;
}

void FeatureFlagManager::set_enabled(const char* flag_name, bool enabled) {
    int idx = find_flag(flag_name);
    if (idx < 0) return;
    if (flags_[idx].enabled != enabled) {
        flags_[idx].enabled = enabled;
        LOG_I("FFLAG", "Flag '%s' set to %s", flag_name, enabled ? "ON" : "OFF");
    }
}

void FeatureFlagManager::reset_to_defaults() {
    for (auto& flag : flags_) {
        flag.enabled = flag.default_value;
    }
    LOG_I("FFLAG", "All flags reset to defaults");
}

/* ═══ Global singleton ═══ */

static FeatureFlagManager g_feature_flags;

FeatureFlagManager& feature_flags() {
    return g_feature_flags;
}

void feature_flags_init() {
    g_feature_flags.load_from_config();
    LOG_I("FFLAG", "Feature flags initialized (%zu flags)", g_feature_flags.get_all_flags().size());
}
