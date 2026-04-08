#ifndef APP_H
#define APP_H

#include "lvgl.h"
#include "app_config.h"
#include <string>

/* Startup error - set before UI init, shown as LVGL modal if non-empty */
extern std::string g_startup_error;
extern std::string g_startup_error_title;

/* OpenClaw status — reflects full chain reachability, not just gateway process */
enum class ClawStatus {
    NotInstalled,   /* OpenClaw not found on system */
    Detected,       /* Found but gateway not running */
    Ready,          /* Gateway in + HTTP OK + agent idle → can send messages */
    Busy,           /* Gateway in + HTTP OK + active session processing */
    Checking,       /* First poll / transient state */
    Error           /* Gateway down or HTTP unreachable */
};

/* Human-readable failure reason (set by health thread when status == Error) */
extern std::string g_status_reason;

struct OpenClawInfo {
    char executable[512] = {0};
    char install_dir[512] = {0};
    char config_dir[512] = {0};
    char config_file[512] = {0};
    char version[128] = {0};
    int gateway_port = GATEWAY_PORT;
};

/* UI functions */
void app_ui_init();
void app_refresh_status();
void update_ui_language();  /* Refresh all translatable UI text */
void ui_log(const char* fmt, ...);  /* Append to log area */

/* Manager functions */
OpenClawInfo app_detect_openclaw();
ClawStatus app_check_status(const OpenClawInfo& info);
bool app_start_gateway();
bool app_stop_gateway();
bool app_enable_chat_endpoint();
bool app_update_model_config(const char* api_key, const char* model_name);
bool app_get_current_model(char* model_out, int out_size);
int app_get_all_models(void (*cb)(const char* model_id, void* ctx), void* ctx);
bool app_get_provider_api_key(const char* provider, char* key_out, int out_size);

/* Node.js & OpenClaw setup */
bool app_check_nodejs(char* version_out, int out_size);
bool app_check_nodejs_version_ok(const char* version);
const char* app_get_nodejs_download_url();
bool app_install_openclaw(char* output, int out_size); /* auto: network → local fallback */
bool app_install_openclaw_ex(char* output, int out_size, const char* mode); /* "network"/"local"/"auto" */
bool app_init_openclaw(char* output, int out_size); /* gateway start → stop (generate default config) */
bool app_install_gemma_models(int model_mask, char* output, int out_size); /* bit0=2B bit1=9B bit2=27B */
bool app_full_setup(const char* install_mode, const char* api_key, const char* model_name,
                    char* output, int out_size); /* install → init → configure → verify */
bool app_uninstall_openclaw(char* output, int out_size); /* npm uninstall only */
bool app_uninstall_openclaw_full(char* output, int out_size, bool full_cleanup); /* + remove config */
bool app_verify_openclaw(char* detail_out, int out_size); /* CLI + config + gateway health */

/* OpenClaw single-instance management */
int  app_count_node_processes();         /* Count running node.exe processes */
bool app_kill_duplicate_openclaw();      /* Kill all node.exe, wait for port release */

/* Full environment check */
struct EnvCheckResult {
    bool node_ok;
    bool node_version_ok;
    bool npm_ok;
    bool openclaw_ok;
    char node_ver[64];
    char npm_ver[32];
    char openclaw_ver[64];
    char error_msg[256];
};
EnvCheckResult app_check_environment();

/* HTTP helper */
int http_get(const char* url, char* response, int resp_size, int timeout_sec = 3);
int http_post(const char* url, const char* json_body, const char* api_key,
              char* response, int resp_size, int timeout_sec = 30);
int http_post_stream(const char* url, const char* json_body, const char* api_key,
                     void (*stream_cb)(const char* chunk, void* ctx), void* stream_ctx,
                     int timeout_sec = 60);

/* Auto-start (registry) */
bool is_auto_start();
void set_auto_start(bool enable);

/* SDL window access (for title bar drag) */
struct SDL_Window;
SDL_Window* app_get_window();

/* Window topmost control */
void app_set_topmost(bool topmost);
bool app_is_topmost();

/* Runtime font access (for ui_settings.cpp) */
lv_font_t* app_get_cjk_font();
lv_font_t* app_get_cjk_font_small();

/* Exit confirmation */
bool is_exit_confirmation_enabled();
void set_exit_confirmation(bool enabled);

/* Window config persistence (called before window creation) */
void load_window_config(int* w, int* h);
void load_exit_confirm_config();
void load_dpi_config();

/* DPI scale */
void app_set_dpi_scale(int scale_percent);
int app_get_dpi_scale();
/* Convert base(96dpi) pixels to current DPI */
inline int SCALE(int px) { return px; }  /* DPI-aware: no double-scaling */

/* Model & API Key globals */
extern char g_selected_model[256];
extern char g_api_key[256];

/* Config persistence */
void save_theme_config();

/* Settings UI functions */
void ui_settings_init(lv_obj_t* parent);
void ui_settings_open();
void ui_settings_close();
bool ui_settings_is_open();

/* Setup wizard (first-launch / reconfigure) */
void ui_show_setup_wizard();

/* JSON utility: extract integer/string from simple JSON (handles whitespace, nesting level 1) */
int json_extract_int(const char* json, const char* key, int default_val = 0);
bool json_extract_string(const char* json, const char* key, char* out, int out_size);

/* UTF-8 utility */
int utf8_safe_truncate(const char* str, int max_bytes);

/* Secure memory clearing */
void secure_zero(void* ptr, size_t size);

/* Session management */
bool app_abort_session(const char* session_key);  /* Abort/reset specific session */
bool app_abort_all_sessions();                     /* Reset all sessions */

/* Workspace management (WS-01) */
std::string workspace_get_root();
bool workspace_set_root(const char* path);
struct WorkspaceHealth;
bool workspace_init(const char* root_path);

#endif /* APP_H */
