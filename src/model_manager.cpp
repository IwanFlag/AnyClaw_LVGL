#include "model_manager.h"
#include "app.h"
#include "app_log.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <process.h>

/* ── Custom models JSON path (relative to exe dir) ── */
#define CUSTOM_ADD_MODELS_FILE "assets/custom_add_models.json"

/* ── Default models ── */
static const char* default_models[] = {
    "minimax-text/MiniMax-M2.7-highspeed",          /* AnyClaw 默认模型 */
    nullptr
};
static const int default_count = 1;

/* ── Cached model list ──────────────────────────────────────── */
static char model_cache[MODEL_MAX_COUNT][MODEL_MAX_NAME_LEN];
static int  model_count = 0;
static bool from_api = false;
static bool has_gateway_config = false;

/* ── Forward declarations ──────────────────────────────────── */
static void custom_add_models_load();
static bool model_in_list(const char* name);

/* ── Callback for app_get_all_models: add model to list ───── */
static void add_model_cb(const char* model_id, void* ctx) {
    (void)ctx;
    if (!model_id || !model_id[0]) return;
    if (!model_in_list(model_id) && model_count < MODEL_MAX_COUNT) {
        strncpy(model_cache[model_count], model_id, MODEL_MAX_NAME_LEN - 1);
        model_cache[model_count][MODEL_MAX_NAME_LEN - 1] = '\0';
        model_count++;
    }
}

/* ── Load all models from openclaw.json providers config ──── */
void model_load_from_openclaw_config() {
    int before = model_count;
    int found = app_get_all_models(add_model_cb, nullptr);
    int added = model_count - before;
    if (added > 0) {
        LOG_I("MODEL", "Loaded %d models from openclaw.json (%d total providers models, %d new)", added, found, added);
    } else {
        LOG_D("MODEL", "No new models from openclaw.json (found %d, all already in list)", found);
    }
}

/* ── Helper: check if model already in list ─────────────────── */
static bool model_in_list(const char* name) {
    if (!name || !name[0]) return false;
    for (int i = 0; i < model_count; i++) {
        if (strcmp(model_cache[i], name) == 0) return true;
    }
    return false;
}

/* ── Helper: insert model at top of list (index 0) ──────────── */
static void model_insert_top(const char* name) {
    if (!name || !name[0] || model_count >= MODEL_MAX_COUNT) return;
    if (model_in_list(name)) return;

    /* Shift everything down by 1 */
    for (int i = model_count; i > 0; i--) {
        strncpy(model_cache[i], model_cache[i - 1], MODEL_MAX_NAME_LEN - 1);
        model_cache[i][MODEL_MAX_NAME_LEN - 1] = '\0';
    }
    /* Insert at top */
    strncpy(model_cache[0], name, MODEL_MAX_NAME_LEN - 1);
    model_cache[0][MODEL_MAX_NAME_LEN - 1] = '\0';
    model_count++;
    LOG_I("MODEL", "Inserted user model at top: %s", name);
}

/* ── Initialize: load defaults + merge user's Gateway config ── */
/* Note: openclaw config get reads the file directly, no daemon needed */
void model_manager_init() {
    model_count = 0;
    from_api = false;
    has_gateway_config = false;

    /* 1. Load default models */
    for (int i = 0; default_models[i] && model_count < MODEL_MAX_COUNT; i++) {
        strncpy(model_cache[model_count], default_models[i], MODEL_MAX_NAME_LEN - 1);
        model_cache[model_count][MODEL_MAX_NAME_LEN - 1] = '\0';
        model_count++;
    }
    LOG_I("MODEL", "Loaded %d default models", model_count);

    /* 1.5 Load user custom models from JSON file */
    custom_add_models_load();

    /* 1.6 Load all models from openclaw.json providers config */
    model_load_from_openclaw_config();

    /* 2. Try reading current model from Gateway config (reads file, no daemon needed) */
    char gw_model[256] = {0};
    if (app_get_current_model(gw_model, sizeof(gw_model)) && gw_model[0]) {
        has_gateway_config = true;
        LOG_I("MODEL", "Gateway model found: %s", gw_model);
        model_insert_current(gw_model);
    } else {
        LOG_I("MODEL", "No Gateway model configured yet");
    }
}

/* ── Insert current Gateway model into list (called from UI) ── */
/* Exact match against list (provider prefix preserved), inserts at top if not found.
 * Returns the index of the model (or -1 if no gateway model). */
int model_insert_current(const char* gw_model) {
    if (!gw_model || !gw_model[0]) return -1;

    has_gateway_config = true;

    /* Exact match (provider prefix kept as-is in both list and comparison) */
    /* Check if already in list */
    for (int i = 0; i < model_count; i++) {
        if (strcmp(model_cache[i], gw_model) == 0) {
            LOG_D("MODEL", "Gateway model already in list at index %d: %s", i, gw_model);
            return i;
        }
    }

    /* Not in list → insert at top (full prefixed name) */
    model_insert_top(gw_model);
    return 0;
}

int model_get_count() {
    return model_count;
}

const char* model_get_name(int index) {
    if (index < 0 || index >= model_count) return nullptr;
    return model_cache[index];
}

int model_get_default_index() {
    /* Default to first model (google/gemini-2.5-pro-preview) */
    return 0;
}

bool model_is_from_api() {
    return from_api;
}

bool model_is_free(int index) {
    const char* name = model_get_name(index);
    if (!name) return false;
    int len = (int)strlen(name);
    return (len >= 5 && strcmp(name + len - 5, ":free") == 0);
}

bool model_is_free_by_name(const char* name) {
    if (!name) return false;
    int len = (int)strlen(name);
    return (len >= 5 && strcmp(name + len - 5, ":free") == 0);
}

/*
 * Parse model IDs from OpenRouter API JSON response.
 * The JSON format is: { "data": [ { "id": "provider/model", ... }, ... ] }
 * We extract all "id" values.
 */
static int parse_models_json(const char* json, char out[][MODEL_MAX_NAME_LEN], int max_count) {
    int count = 0;
    const char* p = json;

    while ((p = strstr(p, "\"id\"")) != nullptr && count < max_count) {
        p += 5;  /* skip "id" */
        while (*p == ' ' || *p == ':' || *p == '"') p++;

        /* Extract model ID until closing quote */
        int len = 0;
        while (*p && *p != '"' && len < MODEL_MAX_NAME_LEN - 1) {
            out[count][len++] = *p++;
        }
        out[count][len] = '\0';

        if (len > 0) {
            count++;
        }
    }

    return count;
}

/* ── Fetch models from OpenRouter API ───────────────────────── */
bool model_refresh_from_api() {
    LOG_I("MODEL", "Fetching models from OpenRouter API...");

    /* OpenRouter models endpoint returns ~100KB+ JSON.
     * We need a large buffer. Allocate dynamically. */
    const int buf_size = 256 * 1024;  /* 256KB should be enough */
    char* response = new char[buf_size];
    if (!response) {
        LOG_E("MODEL", "Failed to allocate response buffer");
        return false;
    }
    memset(response, 0, buf_size);

    int code = http_get("https://openrouter.ai/api/v1/models", response, buf_size, 10);

    if (code != 200) {
        LOG_W("MODEL", "API request failed (HTTP %d)", code);
        delete[] response;
        return false;
    }

    /* Parse the response */
    char new_models[MODEL_MAX_COUNT][MODEL_MAX_NAME_LEN];
    int new_count = parse_models_json(response, new_models, MODEL_MAX_COUNT);

    delete[] response;

    if (new_count == 0) {
        LOG_W("MODEL", "No models parsed from API response");
        return false;
    }

    /* Update cache */
    model_count = 0;
    for (int i = 0; i < new_count && model_count < MODEL_MAX_COUNT; i++) {
        strncpy(model_cache[model_count], new_models[i], MODEL_MAX_NAME_LEN - 1);
        model_cache[model_count][MODEL_MAX_NAME_LEN - 1] = '\0';
        model_count++;
    }
    from_api = true;

    LOG_I("MODEL", "Fetched %d models from API", model_count);
    return true;
}

/* ── Check if Gateway has any model config ──────────────────── */
bool model_has_gateway_config() {
    return has_gateway_config;
}

/* ── Ensure default config exists (for first-time setup) ────── */
/* Returns true if config already existed or was created successfully */
bool model_ensure_default_config() {
    if (has_gateway_config) {
        LOG_D("MODEL", "Gateway already configured, skip auto-setup");
        return true;
    }

    LOG_I("MODEL", "No Gateway config, creating default (MiniMax M2.7 highspeed)...");

    /* Set default model + provider via app_update_model_config.
     * This calls "openclaw config set" which is merge behavior (no overwrite). */
    bool ok = app_update_model_config(nullptr, "minimax-text/MiniMax-M2.7-highspeed");
    if (ok) {
        has_gateway_config = true;
        LOG_I("MODEL", "Default config created: minimax-text/MiniMax-M2.7-highspeed");
    } else {
        LOG_E("MODEL", "Failed to create default config");
    }
    return ok;
}

/* ═══════════════════════════════════════════════════════════════
 *  Custom Model Management
 *  Users can add their own models (free or paid) with API keys.
 *  Persisted to assets/custom_add_models.json as simple JSON array.
 * ═══════════════════════════════════════════════════════════════ */

/* Simple JSON parse: read custom_add_models.json into model list */
static void custom_add_models_load() {
    FILE* f = fopen(CUSTOM_ADD_MODELS_FILE, "r");
    if (!f) return; /* No file = no custom models, fine */

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size <= 0 || size > 64*1024) { fclose(f); return; }

    char* buf = new char[size + 1];
    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);

    /* Parse JSON array of strings: ["model1", "model2", ...] */
    const char* p = buf;
    int loaded = 0;
    while ((p = strstr(p, "\"")) != nullptr) {
        p++; /* skip opening quote */
        const char* start = p;
        while (*p && *p != '"') p++;
        int len = (int)(p - start);
        if (len > 0 && len < MODEL_MAX_NAME_LEN && *p == '"') {
            char name[MODEL_MAX_NAME_LEN];
            memcpy(name, start, len);
            name[len] = '\0';
            if (!model_in_list(name) && model_count < MODEL_MAX_COUNT) {
                strncpy(model_cache[model_count], name, MODEL_MAX_NAME_LEN - 1);
                model_cache[model_count][MODEL_MAX_NAME_LEN - 1] = '\0';
                model_count++;
                loaded++;
            }
        }
        if (*p) p++; /* skip closing quote */
    }

    delete[] buf;
    if (loaded > 0) {
        LOG_I("MODEL", "Loaded %d custom models from %s", loaded, CUSTOM_ADD_MODELS_FILE);
    }
}

/* Save current custom models (non-default ones) to JSON file */
bool model_save_custom(const char* model_name) {
    if (!model_name || !model_name[0]) return false;

    /* Add to runtime list if not present */
    if (!model_in_list(model_name) && model_count < MODEL_MAX_COUNT) {
        strncpy(model_cache[model_count], model_name, MODEL_MAX_NAME_LEN - 1);
        model_cache[model_count][MODEL_MAX_NAME_LEN - 1] = '\0';
        model_count++;
    }

    /* Read existing custom models */
    char existing[MODEL_MAX_COUNT][MODEL_MAX_NAME_LEN];
    int existing_count = 0;

    FILE* f = fopen(CUSTOM_ADD_MODELS_FILE, "r");
    if (f) {
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);
        if (size > 0 && size < 64*1024) {
            char* buf = new char[size + 1];
            fread(buf, 1, size, f);
            buf[size] = '\0';
            const char* p = buf;
            while ((p = strstr(p, "\"")) != nullptr && existing_count < MODEL_MAX_COUNT) {
                p++;
                const char* start = p;
                while (*p && *p != '"') p++;
                int len = (int)(p - start);
                if (len > 0 && len < MODEL_MAX_NAME_LEN && *p == '"') {
                    memcpy(existing[existing_count], start, len);
                    existing[existing_count][len] = '\0';
                    existing_count++;
                }
                if (*p) p++;
            }
            delete[] buf;
        }
        fclose(f);
    }

    /* Check if already saved */
    for (int i = 0; i < existing_count; i++) {
        if (strcmp(existing[i], model_name) == 0) {
            LOG_D("MODEL", "Custom model already saved: %s", model_name);
            return true;
        }
    }

    /* Append */
    if (existing_count < MODEL_MAX_COUNT) {
        strncpy(existing[existing_count], model_name, MODEL_MAX_NAME_LEN - 1);
        existing[existing_count][MODEL_MAX_NAME_LEN - 1] = '\0';
        existing_count++;
    }

    /* Write back */
    f = fopen(CUSTOM_ADD_MODELS_FILE, "w");
    if (!f) {
        LOG_E("MODEL", "Failed to write %s", CUSTOM_ADD_MODELS_FILE);
        return false;
    }
    fprintf(f, "[\n");
    for (int i = 0; i < existing_count; i++) {
        fprintf(f, "  \"%s\"%s\n", existing[i], (i < existing_count - 1) ? "," : "");
    }
    fprintf(f, "]\n");
    fclose(f);

    LOG_I("MODEL", "Saved custom model: %s (total %d)", model_name, existing_count);
    return true;
}

/* Remove a custom model from saved list */
bool model_remove_custom(const char* model_name) {
    if (!model_name || !model_name[0]) return false;

    /* Read existing */
    char existing[MODEL_MAX_COUNT][MODEL_MAX_NAME_LEN];
    int existing_count = 0;

    FILE* f = fopen(CUSTOM_ADD_MODELS_FILE, "r");
    if (!f) return false;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size > 0 && size < 64*1024) {
        char* buf = new char[size + 1];
        fread(buf, 1, size, f);
        buf[size] = '\0';
        const char* p = buf;
        while ((p = strstr(p, "\"")) != nullptr && existing_count < MODEL_MAX_COUNT) {
            p++;
            const char* start = p;
            while (*p && *p != '"') p++;
            int len = (int)(p - start);
            if (len > 0 && len < MODEL_MAX_NAME_LEN && *p == '"') {
                memcpy(existing[existing_count], start, len);
                existing[existing_count][len] = '\0';
                existing_count++;
            }
            if (*p) p++;
        }
        delete[] buf;
    }
    fclose(f);

    /* Filter out the target */
    f = fopen(CUSTOM_ADD_MODELS_FILE, "w");
    if (!f) return false;
    fprintf(f, "[\n");
    int written = 0;
    for (int i = 0; i < existing_count; i++) {
        if (strcmp(existing[i], model_name) == 0) continue;
        fprintf(f, "  \"%s\"%s\n", existing[i], written > 0 ? "," : "");
        written++;
    }
    fprintf(f, "]\n");
    fclose(f);

    LOG_I("MODEL", "Removed custom model: %s", model_name);
    return true;
}

/* Check if a model is a user-added custom model (not in default_models[]) */
bool model_is_custom(int index) {
    const char* name = model_get_name(index);
    if (!name) return false;
    for (int i = 0; default_models[i]; i++) {
        if (strcmp(name, default_models[i]) == 0) return false;
    }
    return true;
}

/* ═══════════════════════════════════════════════════════════════
 *  Gemma Local (llama.cpp) runtime
 * ═══════════════════════════════════════════════════════════════ */
static PROCESS_INFORMATION g_llama_pi = {};
static char g_llama_model_name[64] = {0};
static const int GEMMA_LOCAL_PORT = 18800;

static bool file_exists_a(const char* path) {
    if (!path || !path[0]) return false;
    DWORD a = GetFileAttributesA(path);
    return (a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY));
}

static bool gemma_local_model_file(const char* model_name, char* out, int out_size) {
    if (!out || out_size <= 0) return false;
    out[0] = '\0';
    if (!gemma_local_is_model(model_name)) return false;
    const char* userprofile = std::getenv("USERPROFILE");
    if (!userprofile) return false;
    const char* file_name = "gemma-2-2b-it-q4_k_m.gguf";
    if (strstr(model_name, "9b")) file_name = "gemma-2-9b-it-q4_k_m.gguf";
    else if (strstr(model_name, "27b")) file_name = "gemma-2-27b-it-q4_k_m.gguf";
    snprintf(out, out_size, "%s\\.anyclaw\\models\\gemma\\%s", userprofile, file_name);
    return true;
}

static bool gemma_local_find_server_bin(char* out, int out_size) {
    if (!out || out_size <= 0) return false;
    out[0] = '\0';
    const char* userprofile = std::getenv("USERPROFILE");
    char c1[MAX_PATH] = {0}, c2[MAX_PATH] = {0};
    if (userprofile) {
        snprintf(c1, sizeof(c1), "%s\\.anyclaw\\llama\\llama-server.exe", userprofile);
        snprintf(c2, sizeof(c2), "%s\\.anyclaw\\bin\\llama-server.exe", userprofile);
    }
    const char* candidates[] = {
        c1, c2,
        "tools\\llama-server.exe",
        "bundled\\llama\\llama-server.exe",
        nullptr
    };
    for (int i = 0; candidates[i]; i++) {
        if (file_exists_a(candidates[i])) {
            snprintf(out, out_size, "%s", candidates[i]);
            return true;
        }
    }
    return false;
}

bool gemma_local_is_model(const char* model_name) {
    return model_name && strstr(model_name, "gemma-local-") == model_name;
}

bool gemma_local_server_running() {
    if (!g_llama_pi.hProcess) return false;
    return WaitForSingleObject(g_llama_pi.hProcess, 0) == WAIT_TIMEOUT;
}

int gemma_local_server_port() { return GEMMA_LOCAL_PORT; }

void gemma_local_server_stop() {
    if (!g_llama_pi.hProcess) return;
    if (gemma_local_server_running()) {
        TerminateProcess(g_llama_pi.hProcess, 0);
        WaitForSingleObject(g_llama_pi.hProcess, 2000);
    }
    CloseHandle(g_llama_pi.hProcess);
    CloseHandle(g_llama_pi.hThread);
    g_llama_pi = {};
    g_llama_model_name[0] = '\0';
    LOG_I("GEMMA_LOCAL", "llama-server stopped");
}

bool gemma_local_server_start(const char* model_name, char* err_out, int err_size) {
    if (err_out && err_size > 0) err_out[0] = '\0';
    if (!gemma_local_is_model(model_name)) return false;

    if (gemma_local_server_running() && strcmp(g_llama_model_name, model_name) == 0) {
        return true;
    }
    if (gemma_local_server_running()) {
        gemma_local_server_stop();
    }

    char model_path[MAX_PATH] = {0};
    if (!gemma_local_model_file(model_name, model_path, sizeof(model_path)) || !file_exists_a(model_path)) {
        if (err_out && err_size > 0) snprintf(err_out, err_size, "Gemma model missing: %s", model_path);
        return false;
    }

    char server_bin[MAX_PATH] = {0};
    if (!gemma_local_find_server_bin(server_bin, sizeof(server_bin))) {
        if (err_out && err_size > 0) snprintf(err_out, err_size, "llama-server.exe not found");
        return false;
    }

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    char cmd[1024] = {0};
    snprintf(cmd, sizeof(cmd),
             "\"%s\" -m \"%s\" --host 127.0.0.1 --port %d --ctx-size 4096",
             server_bin, model_path, GEMMA_LOCAL_PORT);
    BOOL ok = CreateProcessA(nullptr, cmd, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    if (!ok) {
        if (err_out && err_size > 0) snprintf(err_out, err_size, "CreateProcess failed: %lu", GetLastError());
        return false;
    }
    g_llama_pi = pi;
    snprintf(g_llama_model_name, sizeof(g_llama_model_name), "%s", model_name);
    Sleep(1000);
    if (!gemma_local_server_running()) {
        if (err_out && err_size > 0) snprintf(err_out, err_size, "llama-server exited immediately");
        gemma_local_server_stop();
        return false;
    }
    LOG_I("GEMMA_LOCAL", "llama-server started model=%s port=%d", model_name, GEMMA_LOCAL_PORT);
    return true;
}

/* ═══════════════════════════════════════════════════════════════
 *  Model Failover (弹性通道) — 完整实现
 *
 *  评分公式: score = 0.6×stability + 0.25×speed + 0.15×capability
 *  健康检查: 每5分钟后台ping一次（最小请求，10s超时）
 *  冷却机制: 连续3次失败 → 冷却10分钟
 *  计数衰减: 每20次检查重置计数器
 * ═══════════════════════════════════════════════════════════════ */

#include <windows.h>
#include <process.h>
#include <cmath>

static bool g_failover_enabled = true;
static ModelHealth g_failover_models[FAILOVER_MAX_MODELS];
static int g_failover_count = 0;
static HANDLE g_health_thread = nullptr;
static volatile LONG g_health_thread_running = 0;

/* ── Capability scoring table ──────────────────────────────── */
struct ModelCapability {
    const char* substr;   /* substring match in model name */
    double score;         /* 0.0 ~ 1.0 */
};

static const ModelCapability capability_table[] = {
    {"405b",   1.00},
    {"120b",   0.85},
    {"110b",   0.83},
    {"100b",   0.80},
    {"90b",    0.78},
    {"80b",    0.75},
    {"72b",    0.72},
    {"70b",    0.70},
    {"65b",    0.68},
    {"56b",    0.65},
    {"50b",    0.62},
    {"40b",    0.58},
    {"34b",    0.55},
    {"32b",    0.53},
    {"30b",    0.50},
    {"27b",    0.48},
    {"24b",    0.45},
    {"20b",    0.42},
    {"14b",    0.38},
    {"13b",    0.36},
    {"12b",    0.35},
    {"9b",     0.30},
    {"8b",     0.28},
    {"7b",     0.25},
    {"4b",     0.18},
    {"3b",     0.15},
    {"2b",     0.12},
    {"1.2b",   0.10},
    {"1b",     0.08},
    {nullptr,  0.50}  /* unknown → neutral */
};

/* Get capability score for a model name */
static double get_capability_score(const char* model_name) {
    if (!model_name) return 0.5;
    /* Lowercase copy for matching */
    char lower[MODEL_MAX_NAME_LEN];
    int i;
    for (i = 0; model_name[i] && i < MODEL_MAX_NAME_LEN - 1; i++) {
        lower[i] = (char)tolower((unsigned char)model_name[i]);
    }
    lower[i] = '\0';

    for (int j = 0; capability_table[j].substr; j++) {
        if (strstr(lower, capability_table[j].substr)) {
            return capability_table[j].score;
        }
    }
    return 0.5; /* unknown model */
}

/* ── Init / cleanup ────────────────────────────────────────── */

void failover_init() {
    g_failover_count = 0;
    memset(g_failover_models, 0, sizeof(g_failover_models));
    failover_load_config();
    LOG_I("FAILOVER", "Init: enabled=%d, models=%d", g_failover_enabled, g_failover_count);
}

bool failover_is_enabled() { return g_failover_enabled; }

void failover_set_enabled(bool enabled) {
    g_failover_enabled = enabled;
    LOG_I("FAILOVER", "Set enabled=%d", enabled);
}

int failover_get_count() { return g_failover_count; }

const ModelHealth* failover_get_model(int index) {
    if (index < 0 || index >= g_failover_count) return nullptr;
    return &g_failover_models[index];
}

void failover_toggle_model(const char* model_name, bool enabled) {
    if (!model_name || !model_name[0]) return;
    for (int i = 0; i < g_failover_count; i++) {
        if (strcmp(g_failover_models[i].model_name, model_name) == 0) {
            g_failover_models[i].enabled = enabled;
            LOG_I("FAILOVER", "Toggle %s: %s", model_name, enabled ? "ON" : "OFF");
            return;
        }
    }
    if (g_failover_count < FAILOVER_MAX_MODELS) {
        ModelHealth& h = g_failover_models[g_failover_count];
        memset(&h, 0, sizeof(h));
        snprintf(h.model_name, sizeof(h.model_name), "%s", model_name);
        h.enabled = enabled;
        h.is_healthy = true;
        g_failover_count++;
        LOG_I("FAILOVER", "Added %s: %s", model_name, enabled ? "ON" : "OFF");
    }
}

bool failover_is_model_enabled(const char* model_name) {
    if (!model_name) return false;
    for (int i = 0; i < g_failover_count; i++) {
        if (strcmp(g_failover_models[i].model_name, model_name) == 0) {
            return g_failover_models[i].enabled;
        }
    }
    return false;
}

/* ── Scoring algorithm ─────────────────────────────────────── */

double failover_calc_score(const ModelHealth* h) {
    if (!h) return 0.0;

    /* Stability: success / (success + fail) */
    int total = h->success_count + h->fail_count;
    double stability = (total > 0) ? (double)h->success_count / total : 0.5;

    /* Speed: 1 - (latency / 15000), clamped to [0, 1] */
    double speed = 0.5; /* default if no data */
    if (h->last_latency_ms > 0 && total > 0) {
        speed = 1.0 - (double)h->last_latency_ms / 15000.0;
        if (speed < 0.0) speed = 0.0;
        if (speed > 1.0) speed = 1.0;
    }

    /* Capability: from lookup table */
    double capability = get_capability_score(h->model_name);

    /* Composite score */
    return 0.6 * stability + 0.25 * speed + 0.15 * capability;
}

/* Check if a model is in cooldown (too many consecutive fails recently) */
static bool is_in_cooldown(const ModelHealth* h) {
    if (h->consec_fail_count < FAILOVER_CONSEC_FAIL_THRESHOLD) return false;
    DWORD now = GetTickCount();
    return (now - h->last_fail_ms) < FAILOVER_COOLDOWN_MS;
}

/* ── Model selection (scored) ──────────────────────────────── */

const char* failover_get_next_healthy(const char* current_model) {
    if (!g_failover_enabled || g_failover_count == 0) return nullptr;

    int best_idx = -1;
    double best_score = -1.0;

    for (int i = 0; i < g_failover_count; i++) {
        if (!g_failover_models[i].enabled) continue;
        if (current_model && strcmp(g_failover_models[i].model_name, current_model) == 0) continue;

        /* Skip models in cooldown */
        if (is_in_cooldown(&g_failover_models[i])) {
            LOG_D("FAILOVER", "Skip %s: in cooldown (%d consec fails)",
                  g_failover_models[i].model_name, g_failover_models[i].consec_fail_count);
            continue;
        }

        double score = failover_calc_score(&g_failover_models[i]);
        LOG_D("FAILOVER", "Score %s: %.3f (stab=%.2f speed=%.2f cap=%.2f healthy=%d)",
              g_failover_models[i].model_name, score,
              (g_failover_models[i].success_count + g_failover_models[i].fail_count > 0)
                  ? (double)g_failover_models[i].success_count / (g_failover_models[i].success_count + g_failover_models[i].fail_count)
                  : 0.5,
              1.0 - (double)g_failover_models[i].last_latency_ms / 15000.0,
              get_capability_score(g_failover_models[i].model_name),
              g_failover_models[i].is_healthy);

        if (score > best_score) {
            best_score = score;
            best_idx = i;
        }
    }

    /* Fallback: any enabled model not in cooldown (even unhealthy) */
    if (best_idx < 0) {
        for (int i = 0; i < g_failover_count; i++) {
            if (!g_failover_models[i].enabled) continue;
            if (current_model && strcmp(g_failover_models[i].model_name, current_model) == 0) continue;
            if (is_in_cooldown(&g_failover_models[i])) continue;
            best_idx = i;
            break;
        }
    }

    if (best_idx >= 0) {
        LOG_I("FAILOVER", "Selected: %s (score=%.3f)", g_failover_models[best_idx].model_name, best_score);
        return g_failover_models[best_idx].model_name;
    }

    LOG_W("FAILOVER", "No available backup model (all in cooldown or disabled)");
    return nullptr;
}

/* ── Record results ────────────────────────────────────────── */

void failover_record_result(const char* model_name, bool success, DWORD latency_ms) {
    if (!model_name) return;
    for (int i = 0; i < g_failover_count; i++) {
        if (strcmp(g_failover_models[i].model_name, model_name) != 0) continue;

        ModelHealth& h = g_failover_models[i];
        h.check_count++;
        h.last_check_ms = GetTickCount();
        h.last_latency_ms = latency_ms;

        if (success) {
            h.success_count++;
            h.consec_fail_count = 0; /* reset consecutive on success */
            h.is_healthy = true;
        } else {
            h.fail_count++;
            h.consec_fail_count++;
            h.last_fail_ms = GetTickCount();
            /* Mark unhealthy if fail rate > 50% with enough samples */
            int total = h.success_count + h.fail_count;
            if (total >= 5 && h.fail_count * 2 > total) {
                h.is_healthy = false;
            }
        }

        /* Counter decay: reset every N checks to avoid stale data */
        if (h.check_count >= FAILOVER_DECAY_INTERVAL) {
            LOG_I("FAILOVER", "Decay %s: reset counters (was succ=%d fail=%d)",
                  h.model_name, h.success_count, h.fail_count);
            h.success_count = h.success_count / 2;
            h.fail_count = h.fail_count / 2;
            h.check_count = 0;
        }

        LOG_D("FAILOVER", "Record %s: %s (succ=%d fail=%d consec=%d latency=%lums healthy=%d)",
              h.model_name, success ? "OK" : "FAIL",
              h.success_count, h.fail_count, h.consec_fail_count, latency_ms, h.is_healthy);
        return;
    }
}

/* ── Direct Model Probing (active health check) ────────────── */
/* Probes models directly via OpenRouter API, bypassing Gateway entirely.
 * No config change, no restart, no Gateway dependency. */

/* Probe a single model: send minimal request to OpenRouter.
 * Returns true on HTTP 200 with valid response. */
bool failover_probe_model(const char* model_name) {
    if (!model_name || !model_name[0]) return false;

    /* Get API key */
    char api_key[256] = {0};
    app_get_provider_api_key("openrouter", api_key, sizeof(api_key));
    if (!api_key[0]) {
        LOG_W("FAILOVER", "Probe %s: no OpenRouter API key, skipping", model_name);
        return false;
    }

    /* Build minimal request body — 1 token, no stream */
    char body[512];
    snprintf(body, sizeof(body),
        "{\"model\":\"%s\",\"messages\":[{\"role\":\"user\",\"content\":\"hi\"}],"
        "\"max_tokens\":1,\"stream\":false}",
        model_name);

    /* Direct POST to OpenRouter (bypass Gateway) */
    DWORD tick_start = GetTickCount();
    char response[1024] = {0};
    int status = http_post("https://openrouter.ai/api/v1/chat/completions",
                           body, api_key, response, sizeof(response),
                           FAILOVER_PROBE_TIMEOUT_SEC);
    DWORD elapsed = GetTickCount() - tick_start;

    bool ok = (status == 200 && response[0] != '\0' &&
               strstr(response, "\"choices\"") != nullptr);

    /* Update health record */
    failover_record_result(model_name, ok, elapsed);

    LOG_I("FAILOVER", "Probe %s: HTTP %d %s (%lums)",
          model_name, status, ok ? "OK" : "FAIL", elapsed);
    return ok;
}

/* Probe all enabled models sequentially */
void failover_probe_all() {
    if (!g_failover_enabled || g_failover_count == 0) return;

    LOG_I("FAILOVER", "Starting probe round (%d models)...", g_failover_count);
    DWORD round_start = GetTickCount();

    for (int i = 0; i < g_failover_count; i++) {
        if (!g_failover_models[i].enabled) continue;
        /* Skip models in cooldown — no point probing them yet */
        if (is_in_cooldown(&g_failover_models[i])) continue;

        failover_probe_model(g_failover_models[i].model_name);

        /* Small delay between probes to avoid rate limiting */
        if (i < g_failover_count - 1) Sleep(300);
    }

    DWORD round_elapsed = GetTickCount() - round_start;
    LOG_I("FAILOVER", "Probe round complete (%lums)", round_elapsed);
}

void failover_probe_now() {
    LOG_I("FAILOVER", "Immediate probe requested");
    failover_probe_all();
}

/* ── Background health check thread ────────────────────────── */
/* Actively probes models via direct API + does counter decay. */

static unsigned __stdcall health_check_thread(void* arg) {
    (void)arg;
    LOG_I("FAILOVER", "Health check thread started (interval=%dms)", FAILOVER_CHECK_INTERVAL_MS);

    /* Initial probe round on startup — build health map immediately */
    Sleep(2000); /* brief delay to let Gateway/api_key settle */
    failover_probe_all();

    while (InterlockedCompareExchange(&g_health_thread_running, 0, 0)) {
        Sleep(FAILOVER_CHECK_INTERVAL_MS);
        if (!InterlockedCompareExchange(&g_health_thread_running, 0, 0)) break;
        if (!g_failover_enabled) continue;

        /* Active probe round */
        failover_probe_all();

        /* Log health summary after probe */
        int healthy = 0, unhealthy = 0, cooldown = 0;
        for (int i = 0; i < g_failover_count; i++) {
            if (!g_failover_models[i].enabled) continue;
            if (is_in_cooldown(&g_failover_models[i])) {
                cooldown++;
            } else if (g_failover_models[i].is_healthy) {
                healthy++;
            } else {
                unhealthy++;
            }
        }
        LOG_I("FAILOVER", "Health summary: %d healthy, %d unhealthy, %d cooldown",
              healthy, unhealthy, cooldown);
    }

    LOG_I("FAILOVER", "Health check thread stopped");
    InterlockedExchange(&g_health_thread_running, 0);
    return 0;
}

void failover_start_health_thread() {
    if (g_health_thread) return;
    InterlockedExchange(&g_health_thread_running, 1);
    g_health_thread = (HANDLE)_beginthreadex(nullptr, 0, health_check_thread, nullptr, 0, nullptr);
    LOG_I("FAILOVER", "Health check thread created");
}

void failover_stop_health_thread() {
    if (!g_health_thread) return;
    InterlockedExchange(&g_health_thread_running, 0);
    WaitForSingleObject(g_health_thread, 5000);
    CloseHandle(g_health_thread);
    g_health_thread = nullptr;
    LOG_I("FAILOVER", "Health check thread stopped");
}

/* Save failover config to AnyClaw config.json */
void failover_save_config() {
    const char* userprofile = std::getenv("USERPROFILE");
    if (!userprofile) return;
    char path[512];
    snprintf(path, sizeof(path), "%s\\AppData\\Roaming\\AnyClaw_LVGL\\config.json", userprofile);

    /* Read existing config */
    std::string content;
    {
        std::ifstream f(path);
        if (f.is_open()) {
            content.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
        }
    }

    /* Remove old failover entries if present */
    auto remove_key = [](std::string& s, const char* key) {
        size_t pos = s.find(key);
        if (pos == std::string::npos) return;
        size_t end = s.find('\n', pos);
        if (end == std::string::npos) end = s.size();
        else end++; /* include newline */
        /* FIX Bug 9: Also remove preceding comma+whitespace to avoid double commas */
        size_t start = pos;
        /* Walk back over whitespace */
        while (start > 0 && (s[start - 1] == ' ' || s[start - 1] == '\t')) start--;
        /* If there's a comma before this key, remove from the comma */
        if (start > 0 && s[start - 1] == ',') {
            start--;
        }
        s.erase(start, end - start);
    };
    remove_key(content, "\"failover_enabled\"");
    remove_key(content, "\"failover_models\"");
    remove_key(content, "\"failover_check_interval_ms\"");

    /* Build failover models array */
    std::string models_json = "[";
    for (int i = 0; i < g_failover_count; i++) {
        if (g_failover_models[i].enabled) {
            if (models_json.size() > 1) models_json += ",";
            models_json += "\"";
            models_json += g_failover_models[i].model_name;
            models_json += "\"";
        }
    }
    models_json += "]";

    /* Insert before last } */
    size_t last_brace = content.rfind('}');
    if (last_brace != std::string::npos) {
        /* Check if there's already content (add comma) */
        std::string insert;
        if (last_brace > 0) {
            size_t prev = content.find_last_not_of(" \t\n\r", last_brace - 1);
            if (prev != std::string::npos && content[prev] != ',' && content[prev] != '{') {
                insert += ",";
            }
        }
        char buf[512];
        snprintf(buf, sizeof(buf),
            "\n  \"failover_enabled\": %s,\n  \"failover_models\": %s,\n  \"failover_check_interval_ms\": %d\n",
            g_failover_enabled ? "true" : "false",
            models_json.c_str(),
            FAILOVER_CHECK_INTERVAL_MS);
        insert += buf;
        content.insert(last_brace, insert);
    }

    std::ofstream f(path);
    if (f.is_open()) {
        f << content;
        f.close();
        LOG_I("FAILOVER", "Config saved: enabled=%d models=%d", g_failover_enabled, g_failover_count);
    }
}

/* Load failover config from AnyClaw config.json */
void failover_load_config() {
    const char* userprofile = std::getenv("USERPROFILE");
    if (!userprofile) return;
    char path[512];
    snprintf(path, sizeof(path), "%s\\AppData\\Roaming\\AnyClaw_LVGL\\config.json", userprofile);

    std::ifstream f(path);
    if (!f.is_open()) return;
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    f.close();

    /* Parse failover_enabled */
    size_t pos = content.find("\"failover_enabled\"");
    if (pos != std::string::npos) {
        pos = content.find(':', pos + 18);
        if (pos != std::string::npos) {
            g_failover_enabled = (content.find("true", pos) != std::string::npos &&
                                  content.find("true", pos) < content.find('\n', pos));
        }
    }

    /* Parse failover_models array */
    pos = content.find("\"failover_models\"");
    if (pos != std::string::npos) {
        pos = content.find('[', pos);
        if (pos != std::string::npos) {
            size_t end = content.find(']', pos);
            if (end != std::string::npos) {
                std::string arr = content.substr(pos + 1, end - pos - 1);
                const char* p = arr.c_str();
                while (*p) {
                    while (*p && *p != '"') p++;
                    if (!*p) break;
                    p++; /* skip opening quote */
                    const char* start = p;
                    while (*p && *p != '"') p++;
                    if (*p == '"') {
                        char name[MODEL_MAX_NAME_LEN];
                        int len = (int)(p - start);
                        if (len > 0 && len < MODEL_MAX_NAME_LEN) {
                            memcpy(name, start, len);
                            name[len] = '\0';
                            failover_toggle_model(name, true);
                        }
                        p++;
                    }
                }
            }
        }
    }

    LOG_I("FAILOVER", "Config loaded: enabled=%d models=%d", g_failover_enabled, g_failover_count);
}

/* Scan all models: detect which have API keys configured, add to failover list */
void failover_scan_available_models() {
    LOG_I("FAILOVER", "Scanning available models...");

    /* Check which providers have API keys */
    bool has_openrouter = false;
    bool has_xiaomi = false;
    {
        char key[256] = {0};
        has_openrouter = app_get_provider_api_key("openrouter", key, sizeof(key)) && key[0];
        has_xiaomi = app_get_provider_api_key("xiaomi", key, sizeof(key)) && key[0];
    }
    LOG_I("FAILOVER", "API keys: openrouter=%d xiaomi=%d", has_openrouter, has_xiaomi);

    /* Scan model list: add models whose provider has an API key */
    int added = 0;
    for (int i = 0; i < model_get_count(); i++) {
        const char* mname = model_get_name(i);
        if (!mname) continue;

        /* Determine provider from model name */
        bool has_key = false;
        if (strncmp(mname, "xiaomi/", 7) == 0) {
            has_key = has_xiaomi;
        } else {
            /* Default to openrouter for all other models */
            has_key = has_openrouter;
        }

        if (has_key) {
            /* Check if already in failover list */
            bool exists = false;
            for (int j = 0; j < g_failover_count; j++) {
                if (strcmp(g_failover_models[j].model_name, mname) == 0) {
                    exists = true;
                    break;
                }
            }
            if (!exists && g_failover_count < FAILOVER_MAX_MODELS) {
                ModelHealth& h = g_failover_models[g_failover_count];
                snprintf(h.model_name, sizeof(h.model_name), "%s", mname);
                h.enabled = false; /* unchecked by default */
                h.success_count = 0;
                h.fail_count = 0;
                h.last_check_ms = 0;
                h.last_latency_ms = 0;
                h.is_healthy = true;
                g_failover_count++;
                added++;
            }
        }
    }
    LOG_I("FAILOVER", "Scan complete: %d new models added (%d total)", added, g_failover_count);
}
