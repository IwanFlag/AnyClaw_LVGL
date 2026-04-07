#ifndef MODEL_MANAGER_H
#define MODEL_MANAGER_H

#include <windows.h>  /* for DWORD */

/*
 * ═══════════════════════════════════════════════════════════════
 *  AnyClaw LVGL — Dynamic Model List Manager
 *  Fetches available models from OpenRouter API, with fallback defaults.
 * ═══════════════════════════════════════════════════════════════
 */

#define MODEL_MAX_COUNT     64      /* Max models to cache */
#define MODEL_MAX_NAME_LEN  128     /* Max model ID length */

/* Initialize model manager (call once at startup) */
void model_manager_init();

/* Get number of available models */
int model_get_count();

/* Get model name at index (returns nullptr if out of range) */
const char* model_get_name(int index);

/* Get default model index */
int model_get_default_index();

/* Refresh model list from API (blocks, call from background thread) */
bool model_refresh_from_api();

/* Check if models were fetched from API (vs using defaults) */
bool model_is_from_api();

/* Check if model at index is free (ends with :free) */
bool model_is_free(int index);

/* Check if model name is free (ends with :free) */
bool model_is_free_by_name(const char* name);

/* Insert current Gateway model into list (called from UI at Settings open).
 * Strips provider prefix, inserts at top if not in default list.
 * Returns the index of the model (or -1 if no gateway model). */
int model_insert_current(const char* gw_model);

/* Load all models from openclaw.json providers config into the model list */
void model_load_from_openclaw_config();

/* Check if Gateway has any model config (read from openclaw config) */
bool model_has_gateway_config();

/* Ensure default config exists for first-time users.
 * If no model is configured, sets openrouter/google/gemma-3-4b-it:free.
 * Returns true if config existed or was created. */
bool model_ensure_default_config();

/* ── Custom Model Management ── */

/* Save a custom model name to assets/custom_add_models.json and add to runtime list.
 * Also used for API-based models the user wants to keep. */
bool model_save_custom(const char* model_name);

/* Remove a custom model from saved list. */
bool model_remove_custom(const char* model_name);

/* Check if a model at index is user-added (not in the hardcoded default list). */
bool model_is_custom(int index);

/* ═══════════════════════════════════════════════════════════════
 *  Model Failover (弹性通道)
 *  Auto-switch to healthy backup model when current model fails.
 * ═══════════════════════════════════════════════════════════════ */

#define FAILOVER_MAX_MODELS  16
#define FAILOVER_CHECK_INTERVAL_MS 300000  /* 5 minutes */
#define FAILOVER_DECAY_INTERVAL 20         /* Reset counters every 20 checks */
#define FAILOVER_CONSEC_FAIL_THRESHOLD 3   /* 3 consecutive fails → cooldown */
#define FAILOVER_COOLDOWN_MS 600000        /* 10 min cooldown after consecutive fails */

struct ModelHealth {
    char model_name[MODEL_MAX_NAME_LEN];
    bool enabled;            /* user checked this model for failover */
    int  success_count;
    int  fail_count;
    int  consec_fail_count;  /* consecutive failures (reset on success) */
    int  check_count;        /* total checks performed (for decay) */
    DWORD last_check_ms;
    DWORD last_latency_ms;
    DWORD last_fail_ms;      /* timestamp of last failure (for cooldown) */
    bool  is_healthy;
};

/* Init failover system (call after model_manager_init) */
void failover_init();

/* Get failover enabled state */
bool failover_is_enabled();

/* Set failover enabled state */
void failover_set_enabled(bool enabled);

/* Get number of failover models */
int failover_get_count();

/* Get failover model info at index */
const ModelHealth* failover_get_model(int index);

/* Toggle a model's failover status (by model name) */
void failover_toggle_model(const char* model_name, bool enabled);

/* Check if a model is in failover pool and enabled */
bool failover_is_model_enabled(const char* model_name);

/* Get the best model by weighted score (excluding current). Returns nullptr if none. */
const char* failover_get_next_healthy(const char* current_model);

/* Calculate composite score for a model (stability + speed + capability) */
double failover_calc_score(const ModelHealth* h);

/* Record a result for a model (called from chat flow) */
void failover_record_result(const char* model_name, bool success, DWORD latency_ms);

/* Run health checks on all enabled models (background thread) */
void failover_run_checks();

/* Save failover config to config.json */
void failover_save_config();

/* Load failover config from config.json */
void failover_load_config();

/* Scan all models: detect which have API keys configured, add to failover list */
void failover_scan_available_models();

/* Start background health check thread */
void failover_start_health_thread();

/* Stop background health check thread */
void failover_stop_health_thread();

#endif /* MODEL_MANAGER_H */
