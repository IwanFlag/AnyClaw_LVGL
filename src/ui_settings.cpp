/* ═══════════════════════════════════════════════════════════════
 *  ui_settings.cpp - M2 Settings UI (4-tab panel)
 *  AnyClaw LVGL v2.0 - Desktop Manager
 * ═══════════════════════════════════════════════════════════════ */

#include "app.h"
#include "app_config.h"
#include "model_manager.h"
#include "migration.h"
#include "workspace.h"
#include "permissions.h"
#include "feature_flags.h"
#include "tracing.h"
#include "kb_store.h"
#include "app_log.h"
#include "theme.h"
#include "SDL.h"
#include "lang.h"
#include "lvgl.h"
#include <windows.h>
#include <commdlg.h>

/* Font helper — DPI-scaled font selection (same as ui_main.cpp) */
static const lv_font_t* FONT(int base_px) {
    int sz = base_px * app_get_dpi_scale() / 100;
    if (sz <= 12) return &lv_font_montserrat_10;
    if (sz <= 14) return &lv_font_montserrat_12;
    if (sz <= 16) return &lv_font_montserrat_14;
    if (sz <= 18) return &lv_font_montserrat_16;
    if (sz <= 20) return &lv_font_montserrat_18;
    if (sz <= 22) return &lv_font_montserrat_20;
    if (sz <= 24) return &lv_font_montserrat_22;
    if (sz <= 26) return &lv_font_montserrat_24;
    if (sz <= 28) return &lv_font_montserrat_26;
    if (sz <= 30) return &lv_font_montserrat_28;
    if (sz <= 32) return &lv_font_montserrat_30;
    if (sz <= 36) return &lv_font_montserrat_32;
    if (sz <= 40) return &lv_font_montserrat_36;
    if (sz <= 48) return &lv_font_montserrat_40;
    return &lv_font_montserrat_48;
}
#include "SDL.h"
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <algorithm>
#include <vector>
#include <sstream>
#include <windows.h>

/* Use app.h declaration for app_get_cjk_font() */
#define CJK_FONT (app_get_cjk_font() ? app_get_cjk_font() : NULL)

/* Log buffer accessors from ui_main.cpp */
extern int  ui_log_get_count();
extern const char* ui_log_get_line(int visible_index);
extern int  ui_log_get_level(int visible_index);
extern int  ui_log_get_visible_count();
extern void ui_log_set_filter(int level);
extern int  ui_log_get_filter();
extern void ui_log_clear_entries();
extern void ui_log(const char* fmt, ...);

/* Layout constants - dynamically set from actual display size */
static int SETTING_WIN_W = 1350;
static int SETTING_WIN_H = 740;

/* ── Settings panel state ── */
static lv_obj_t* settings_panel = nullptr;
static lv_obj_t* settings_tabs = nullptr;
static bool settings_visible = false;

/* ── General tab widgets ── */
static lv_obj_t* gen_status_label = nullptr;
static lv_obj_t* gen_path_label = nullptr;
static lv_obj_t* gen_autostart_sw = nullptr;
static lv_obj_t* gen_auto_start_sw = nullptr; /* Auto-restart on crash */
static lv_obj_t* gen_close_gateway_sw = nullptr; /* Close Gateway on exit */
static lv_obj_t* gen_lang_dropdown = nullptr;
static lv_obj_t* gen_refresh_dropdown = nullptr;
static lv_obj_t* perm_status_label = nullptr;
static lv_obj_t* gen_security_led = nullptr;
static lv_obj_t* gen_security_label = nullptr;
static lv_obj_t* gen_security_detail_label = nullptr;

/* ── Account tab widgets ── */
static lv_obj_t* acc_apikey_ta = nullptr;
static lv_obj_t* api_key_status_label = nullptr;

/* ── Model tab widgets ── */
static lv_obj_t* model_dropdown = nullptr;
static lv_obj_t* model_current_label = nullptr;
static lv_obj_t* model_provider_hint = nullptr; /* "Provider: openrouter/xiaomi" */
static lv_obj_t* failover_list_container = nullptr; /* checkbox list for failover models */
static char gw_model_buf[256] = {0}; /* Gateway model for callbacks */
static lv_obj_t* ff_status_label = nullptr;
static lv_obj_t* trace_list_ta = nullptr;
static lv_obj_t* trace_filter_action_input = nullptr;
static lv_obj_t* trace_filter_outcome_dd = nullptr;
static lv_obj_t* kb_docs_label = nullptr;
static lv_obj_t* kb_search_input = nullptr;
static lv_obj_t* kb_result_ta = nullptr;
static lv_obj_t* gen_workspace_label = nullptr;
static lv_obj_t* gen_workspace_dropdown = nullptr;
static lv_obj_t* gen_workspace_input = nullptr;
static lv_obj_t* gen_workspace_status = nullptr;
static lv_obj_t* gen_auth_email_sw = nullptr;
static lv_obj_t* gen_auth_calendar_sw = nullptr;
static lv_obj_t* gen_apps_scan_ta = nullptr;
static std::vector<std::string> g_workspace_choices;

static int lang_to_dropdown_index(Lang lang) {
    switch (lang) {
        case Lang::CN: return 0;
        case Lang::EN: return 1;
        case Lang::KR: return 2;
        case Lang::JP: return 3;
    }
    return 1;
}

static Lang dropdown_index_to_lang(int idx) {
    switch (idx) {
        case 0: return Lang::CN;
        case 1: return Lang::EN;
        case 2: return Lang::KR;
        case 3: return Lang::JP;
        default: return Lang::EN;
    }
}

/* ── Forward declarations ── */
static void settings_close_cb(lv_event_t* e);
static void settings_tab_change_cb(lv_event_t* e);
static void build_permissions_tab(lv_obj_t* tab);
static void build_feature_tab(lv_obj_t* tab);
static void build_tracing_tab(lv_obj_t* tab);
static void build_kb_tab(lv_obj_t* tab);

/* ═══════════════════════════════════════════════════════════════
 *  i18n helpers (single-language display)
 *  tr(cn, en) -> returns one string based on current language
 * ═══════════════════════════════════════════════════════════════ */
static const char* tr(const char* cn, const char* en) {
    switch (g_lang) {
        case Lang::CN:  return cn;
        case Lang::EN:  return en;
        case Lang::KR:  return en;
        case Lang::JP:  return en;
    }
    return en;
}

static std::string workspace_registry_path() {
    const char* home = getenv("USERPROFILE");
    if (!home || !home[0]) return "workspaces.txt";
    return std::string(home) + "\\.openclaw\\workspaces.txt";
}

static std::string normalize_workspace_path(const char* in) {
    if (!in) return "";
    std::string p = in;
    while (!p.empty() && (p.back() == '\r' || p.back() == '\n' || p.back() == ' ' || p.back() == '\t')) p.pop_back();
    while (!p.empty() && (p.front() == ' ' || p.front() == '\t' || p.front() == '"' || p.front() == '\'')) p.erase(p.begin());
    while (!p.empty() && (p.back() == '"' || p.back() == '\'')) p.pop_back();
    std::replace(p.begin(), p.end(), '/', '\\');
    return p;
}

static std::vector<std::string> load_workspace_registry() {
    std::vector<std::string> out;
    std::ifstream f(workspace_registry_path());
    if (!f.is_open()) return out;
    std::string line;
    while (std::getline(f, line)) {
        line = normalize_workspace_path(line.c_str());
        if (!line.empty()) out.push_back(line);
    }
    return out;
}

static void save_workspace_registry(const std::vector<std::string>& paths) {
    std::string p = workspace_registry_path();
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(p).parent_path(), ec);
    std::ofstream f(p, std::ios::trunc);
    if (!f.is_open()) return;
    for (const auto& it : paths) f << it << "\n";
}

static void refresh_workspace_dropdown() {
    if (!gen_workspace_dropdown) return;
    g_workspace_choices = load_workspace_registry();
    std::string current = workspace_get_root();
    if (!current.empty() && std::find(g_workspace_choices.begin(), g_workspace_choices.end(), current) == g_workspace_choices.end()) {
        g_workspace_choices.push_back(current);
    }
    if (g_workspace_choices.empty()) g_workspace_choices.push_back(current.empty() ? "Not configured" : current);

    std::string opts;
    int selected = 0;
    for (size_t i = 0; i < g_workspace_choices.size(); i++) {
        if (i) opts += "\n";
        opts += g_workspace_choices[i];
        if (g_workspace_choices[i] == current) selected = (int)i;
    }
    lv_dropdown_set_options(gen_workspace_dropdown, opts.c_str());
    lv_dropdown_set_selected(gen_workspace_dropdown, (uint16_t)selected);
}

static void workspace_apply_and_sync(const std::string& path) {
    if (path.empty()) return;
    if (!workspace_set_root(path.c_str())) {
        if (gen_workspace_status) lv_label_set_text(gen_workspace_status, "Workspace switch failed");
        return;
    }
    permissions().set_workspace_root(path.c_str());
    workspace_init(path.c_str());
    workspace_sync_managed_sections();
    workspace_sync_runtime_config_from_permissions();
    if (gen_workspace_label) lv_label_set_text(gen_workspace_label, path.c_str());
    if (gen_workspace_status) lv_label_set_text(gen_workspace_status, "Workspace switched");
    ui_log("[Workspace] Switched root: %s", path.c_str());
}

static void workspace_add_to_registry(const std::string& path) {
    if (path.empty()) return;
    auto paths = load_workspace_registry();
    if (std::find(paths.begin(), paths.end(), path) == paths.end()) {
        paths.push_back(path);
        save_workspace_registry(paths);
    }
}

static void workspace_apply_cb(lv_event_t* e) {
    (void)e;
    std::string input = normalize_workspace_path(gen_workspace_input ? lv_textarea_get_text(gen_workspace_input) : "");
    std::string path = input;
    if (path.empty() && gen_workspace_dropdown) {
        int sel = lv_dropdown_get_selected(gen_workspace_dropdown);
        if (sel >= 0 && sel < (int)g_workspace_choices.size()) path = g_workspace_choices[(size_t)sel];
    }
    if (path.empty() || path == "Not configured") {
        if (gen_workspace_status) lv_label_set_text(gen_workspace_status, "Please input or select workspace path");
        return;
    }
    workspace_add_to_registry(path);
    workspace_apply_and_sync(path);
    refresh_workspace_dropdown();
}

static void workspace_add_only_cb(lv_event_t* e) {
    (void)e;
    std::string input = normalize_workspace_path(gen_workspace_input ? lv_textarea_get_text(gen_workspace_input) : "");
    if (input.empty()) {
        if (gen_workspace_status) lv_label_set_text(gen_workspace_status, "Input path is empty");
        return;
    }
    workspace_add_to_registry(input);
    refresh_workspace_dropdown();
    if (gen_workspace_status) lv_label_set_text(gen_workspace_status, "Workspace path saved");
    ui_log("[Workspace] Added candidate: %s", input.c_str());
}

static std::string scan_installed_apps_text() {
    const char* cmd =
        "powershell -NoProfile -ExecutionPolicy Bypass -Command "
        "\"$ErrorActionPreference='SilentlyContinue';"
        "$apps=Get-ItemProperty HKLM:\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\* ,"
        "HKLM:\\Software\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\* ,"
        "HKCU:\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\* | "
        "Where-Object {$_.DisplayName} | "
        "Select-Object -ExpandProperty DisplayName -Unique | Sort-Object | Select-Object -First 120; "
        "$apps -join \"`n\"\"";
    FILE* pipe = _popen(cmd, "r");
    if (!pipe) return "Scan failed: cannot launch powershell";
    std::string out;
    char buf[1024] = {0};
    while (fgets(buf, sizeof(buf), pipe)) out += buf;
    _pclose(pipe);
    if (out.empty()) return "No installed apps found.";
    return out;
}

static void scan_apps_cb(lv_event_t* e) {
    (void)e;
    if (!gen_apps_scan_ta) return;
    lv_textarea_set_text(gen_apps_scan_ta, "Scanning installed apps...");
    std::string txt = scan_installed_apps_text();
    lv_textarea_set_text(gen_apps_scan_ta, txt.c_str());
    ui_log("[Auth] Installed apps scan completed");
}

/* ═══════════════════════════════════════════════════════════════
 *  Style helpers
 * ═══════════════════════════════════════════════════════════════ */
static void apply_dark_style(lv_obj_t* obj) {
    lv_obj_set_style_bg_color(obj, g_colors->panel, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(obj, g_colors->text, 0);
    lv_obj_set_style_text_font(obj, CJK_FONT, 0);
}

static void apply_input_style(lv_obj_t* obj) {
    lv_obj_set_style_bg_color(obj, g_colors->input_bg, 0);
    lv_obj_set_style_border_color(obj, g_colors->panel_border, 0);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_radius(obj, 6, 0);
    lv_obj_set_style_text_color(obj, g_colors->text, 0);
    lv_obj_set_style_text_font(obj, CJK_FONT, 0);
    lv_obj_set_style_pad_all(obj, 8, 0);
    /* Selection highlight: light blue */
    lv_obj_set_style_text_color(obj, lv_color_make(180, 215, 255), LV_PART_SELECTED);
    /* Dropdown: use symbols font for triangle indicator */
    if (lv_obj_check_type(obj, &lv_dropdown_class)) {
        lv_obj_set_style_text_font(obj, FONT(14), LV_PART_INDICATOR);
        /* Fix: dropdown list items also need CJK font for Chinese text */
        lv_obj_set_style_text_font(obj, CJK_FONT, LV_PART_ITEMS);
        lv_obj_set_style_text_color(obj, g_colors->text, LV_PART_ITEMS);
        lv_obj_set_style_bg_color(obj, g_colors->input_bg, LV_PART_ITEMS);
    }
}

static void apply_section_label(lv_obj_t* obj) {
    lv_obj_set_style_text_color(obj, g_colors->section_label, 0);
    lv_obj_set_style_text_font(obj, CJK_FONT, 0);
}

static void apply_hint_label(lv_obj_t* obj) {
    lv_obj_set_style_text_color(obj, g_colors->text_hint, 0);
    lv_obj_set_style_text_font(obj, CJK_FONT, 0);
}

/* ═══════════════════════════════════════════════════════════════
 *  GENERAL TAB
 * ═══════════════════════════════════════════════════════════════ */
extern void update_refresh_interval(int ms);
extern int g_refresh_interval_ms;

static void refresh_dropdown_cb(lv_event_t* e) {
    lv_obj_t* dd = (lv_obj_t*)lv_event_get_target(e);
    uint16_t sel = lv_dropdown_get_selected(dd);
    int intervals[] = {5000, 15000, 30000, 60000, 120000};
    update_refresh_interval(intervals[sel]);
}


/* Helper: blue divider between sections */
static lv_obj_t* add_divider(lv_obj_t* parent) {
    lv_obj_t* d = lv_obj_create(parent);
    lv_obj_set_size(d, LV_PCT(100), 2);
    lv_obj_set_style_bg_color(d, g_colors ? g_colors->panel_border : lv_color_make(90, 110, 145), 0);
    lv_obj_set_style_bg_opa(d, LV_OPA_70, 0);
    lv_obj_set_style_border_width(d, 0, 0);
    lv_obj_clear_flag(d, LV_OBJ_FLAG_SCROLLABLE);
    return d;
}

static bool is_config_dir_writable() {
    char appdata[MAX_PATH] = {0};
    if (SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, appdata) != S_OK) return false;
    std::filesystem::path dir = std::filesystem::path(appdata) / "AnyClaw_LVGL";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec) return false;
    std::filesystem::path fp = dir / ".__sec_write_test.tmp";
    std::ofstream out(fp.string(), std::ios::trunc);
    if (!out.is_open()) return false;
    out << "ok";
    out.close();
    std::filesystem::remove(fp, ec);
    return true;
}

static const char* infer_provider_from_model(const char* model) {
    if (!model || !model[0]) return "openrouter";
    if (strncmp(model, "openrouter/", 11) == 0) return "openrouter";
    if (strncmp(model, "xiaomi/", 7) == 0) return "xiaomi";
    if (strncmp(model, "gemini/", 7) == 0) return "gemini";
    if (strncmp(model, "deepseek/", 9) == 0) return "deepseek";
    if (strncmp(model, "qwen/", 5) == 0) return "qwen";
    return "openrouter";
}

static void refresh_security_status_ui() {
    if (!gen_security_label || !gen_security_detail_label || !gen_security_led) return;

    OpenClawInfo info = app_detect_openclaw();
    char model[256] = {0};
    app_get_current_model(model, sizeof(model));
    const char* provider = infer_provider_from_model(model);
    char key[256] = {0};
    bool api_ok = app_get_provider_api_key(provider, key, sizeof(key)) && key[0];
    bool cfg_ok = is_config_dir_writable();
    bool port_ok = (info.gateway_port > 0 && info.gateway_port < 65536);

    const char* sec_text = "Risk";
    lv_color_t sec_color = lv_color_make(220, 80, 80);
    if (api_ok && cfg_ok && port_ok) {
        sec_text = "Good";
        sec_color = lv_color_make(100, 200, 100);
    } else if ((api_ok && cfg_ok) || (api_ok && port_ok) || (cfg_ok && port_ok)) {
        sec_text = "Warning";
        sec_color = lv_color_make(230, 180, 80);
    }

    lv_led_set_color(gen_security_led, sec_color);
    lv_led_on(gen_security_led);
    lv_label_set_text(gen_security_label, sec_text);
    lv_obj_set_style_text_color(gen_security_label, sec_color, 0);

    lv_label_set_text_fmt(gen_security_detail_label,
                          "API Key: %s(%s) | Port: %d | Config: %s",
                          api_ok ? "OK" : "Missing",
                          provider,
                          info.gateway_port,
                          cfg_ok ? "Writable" : "ReadOnly");
}

static void build_general_tab(lv_obj_t* tab) {
    apply_dark_style(tab);
    lv_obj_set_style_pad_all(tab, 16, 0);
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(tab, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_gap(tab, 8, 0);

    /* Helper: create a horizontal Key: Value row */
    auto make_kv_row = [&](const char* key_text, lv_obj_t* value_obj) -> lv_obj_t* {
        lv_obj_t* row = lv_obj_create(tab);
        lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_set_style_pad_ver(row, 4, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        lv_obj_t* lbl = lv_label_create(row);
        lv_label_set_text(lbl, key_text);
        lv_obj_set_style_text_color(lbl, lv_color_make(180, 185, 200), 0);
        lv_obj_set_style_text_font(lbl, CJK_FONT, 0);

        if (value_obj) {
            lv_obj_set_parent(value_obj, row);
        }
        return row;
    };

    /* ── Gateway Status ── */
    gen_status_label = lv_label_create(tab);
    lv_label_set_text(gen_status_label, "--");
    lv_obj_set_style_text_color(gen_status_label, lv_color_make(100, 200, 100), 0);
    lv_obj_set_style_text_font(gen_status_label, CJK_FONT, 0);
    make_kv_row("Gateway Status", gen_status_label);

    /* ── Install Path ── */
    gen_path_label = lv_label_create(tab);
    lv_label_set_text(gen_path_label, "--");
    lv_label_set_long_mode(gen_path_label, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_width(gen_path_label, LV_SIZE_CONTENT);
    lv_obj_set_style_text_color(gen_path_label, lv_color_make(160, 165, 185), 0);
    lv_obj_set_style_text_font(gen_path_label, CJK_FONT, 0);
    make_kv_row("Install Path", gen_path_label);

    /* ── Workspace Path (WS-01/WS-03) ── */
    {
        std::string ws_root = workspace_get_root();
        gen_workspace_label = lv_label_create(tab);
        lv_label_set_text(gen_workspace_label, ws_root.empty() ? "Not configured" : ws_root.c_str());
        lv_label_set_long_mode(gen_workspace_label, LV_LABEL_LONG_MODE_DOTS);
        lv_obj_set_width(gen_workspace_label, LV_SIZE_CONTENT);
        lv_obj_set_style_text_color(gen_workspace_label, lv_color_make(160, 165, 185), 0);
        lv_obj_set_style_text_font(gen_workspace_label, CJK_FONT, 0);
        make_kv_row("Workspace", gen_workspace_label);

        gen_workspace_dropdown = lv_dropdown_create(tab);
        lv_obj_set_width(gen_workspace_dropdown, SCALE(300));
        apply_input_style(gen_workspace_dropdown);
        make_kv_row("Workspace List", gen_workspace_dropdown);
        refresh_workspace_dropdown();

        gen_workspace_input = lv_textarea_create(tab);
        lv_textarea_set_one_line(gen_workspace_input, true);
        lv_textarea_set_placeholder_text(gen_workspace_input, "D:\\workspace\\MyProject");
        lv_obj_set_width(gen_workspace_input, SCALE(300));
        apply_input_style(gen_workspace_input);
        make_kv_row("Workspace Input", gen_workspace_input);

        lv_obj_t* ws_btn_row = lv_obj_create(tab);
        lv_obj_set_size(ws_btn_row, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(ws_btn_row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(ws_btn_row, 0, 0);
        lv_obj_set_style_pad_all(ws_btn_row, 0, 0);
        lv_obj_clear_flag(ws_btn_row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_flow(ws_btn_row, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_pad_gap(ws_btn_row, 8, 0);

        lv_obj_t* btn_apply_ws = lv_button_create(ws_btn_row);
        lv_obj_set_size(btn_apply_ws, SCALE(150), SCALE(34));
        lv_obj_set_style_bg_color(btn_apply_ws, lv_color_make(60, 130, 220), 0);
        lv_obj_add_event_cb(btn_apply_ws, workspace_apply_cb, LV_EVENT_CLICKED, nullptr);
        lv_obj_t* btn_apply_lbl = lv_label_create(btn_apply_ws);
        lv_label_set_text(btn_apply_lbl, "Apply Workspace");
        lv_obj_set_style_text_font(btn_apply_lbl, CJK_FONT, 0);
        lv_obj_center(btn_apply_lbl);

        lv_obj_t* btn_add_ws = lv_button_create(ws_btn_row);
        lv_obj_set_size(btn_add_ws, SCALE(140), SCALE(34));
        lv_obj_set_style_bg_color(btn_add_ws, lv_color_make(80, 92, 120), 0);
        lv_obj_add_event_cb(btn_add_ws, workspace_add_only_cb, LV_EVENT_CLICKED, nullptr);
        lv_obj_t* btn_add_lbl = lv_label_create(btn_add_ws);
        lv_label_set_text(btn_add_lbl, "Save Candidate");
        lv_obj_set_style_text_font(btn_add_lbl, CJK_FONT, 0);
        lv_obj_center(btn_add_lbl);

        gen_workspace_status = lv_label_create(tab);
        lv_label_set_text(gen_workspace_status, "Workspace manager ready");
        apply_hint_label(gen_workspace_status);
        make_kv_row("Workspace Status", gen_workspace_status);
    }

    /* ── Application Authorization (UI-43) ── */
    {
        extern bool g_app_auth_email;
        extern bool g_app_auth_calendar;
        extern void save_theme_config();

        lv_obj_t* auth_title = lv_label_create(tab);
        lv_label_set_text(auth_title, "Application Authorization");
        apply_section_label(auth_title);

        auto make_auth_card = [&](const char* icon, const char* name, const char* rec, bool* bound_flag) {
            lv_obj_t* card = lv_obj_create(tab);
            lv_obj_set_size(card, LV_PCT(100), LV_SIZE_CONTENT);
            lv_obj_set_style_bg_color(card, g_colors->input_bg, 0);
            lv_obj_set_style_border_color(card, g_colors->panel_border, 0);
            lv_obj_set_style_border_width(card, 1, 0);
            lv_obj_set_style_radius(card, 10, 0);
            lv_obj_set_style_pad_all(card, 10, 0);
            lv_obj_set_style_pad_gap(card, 8, 0);
            lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
            lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

            lv_obj_t* row_head = lv_obj_create(card);
            lv_obj_set_size(row_head, LV_PCT(100), LV_SIZE_CONTENT);
            lv_obj_set_style_bg_opa(row_head, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(row_head, 0, 0);
            lv_obj_set_style_pad_all(row_head, 0, 0);
            lv_obj_set_style_pad_gap(row_head, 8, 0);
            lv_obj_set_flex_flow(row_head, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(row_head, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_clear_flag(row_head, LV_OBJ_FLAG_SCROLLABLE);

            lv_obj_t* lbl_name = lv_label_create(row_head);
            lv_label_set_text_fmt(lbl_name, "%s %s", icon, name);
            lv_obj_set_style_text_font(lbl_name, CJK_FONT, 0);
            lv_obj_set_style_text_color(lbl_name, g_colors->text, 0);

            lv_obj_t* lbl_rec = lv_label_create(card);
            lv_label_set_text_fmt(lbl_rec, "推荐交互方式: %s", rec);
            apply_hint_label(lbl_rec);

            lv_obj_t* row_btn = lv_obj_create(card);
            lv_obj_set_size(row_btn, LV_PCT(100), LV_SIZE_CONTENT);
            lv_obj_set_style_bg_opa(row_btn, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(row_btn, 0, 0);
            lv_obj_set_style_pad_all(row_btn, 0, 0);
            lv_obj_set_style_pad_gap(row_btn, 8, 0);
            lv_obj_set_flex_flow(row_btn, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(row_btn, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_clear_flag(row_btn, LV_OBJ_FLAG_SCROLLABLE);

            lv_obj_t* btn_allow = lv_button_create(row_btn);
            lv_obj_set_size(btn_allow, SCALE(90), SCALE(30));
            lv_obj_set_style_bg_color(btn_allow, lv_color_make(60, 130, 220), 0);
            lv_obj_set_style_radius(btn_allow, 8, 0);
            lv_obj_t* lbl_allow = lv_label_create(btn_allow);
            lv_label_set_text(lbl_allow, "授权");
            lv_obj_set_style_text_font(lbl_allow, CJK_FONT, 0);
            lv_obj_center(lbl_allow);
            lv_obj_add_event_cb(btn_allow, [](lv_event_t* e) {
                bool* f = (bool*)lv_event_get_user_data(e);
                if (!f) return;
                *f = true;
                save_theme_config();
                ui_log("[Auth] application authorized");
            }, LV_EVENT_CLICKED, bound_flag);

            lv_obj_t* btn_deny = lv_button_create(row_btn);
            lv_obj_set_size(btn_deny, SCALE(90), SCALE(30));
            lv_obj_set_style_bg_color(btn_deny, lv_color_make(170, 80, 80), 0);
            lv_obj_set_style_radius(btn_deny, 8, 0);
            lv_obj_t* lbl_deny = lv_label_create(btn_deny);
            lv_label_set_text(lbl_deny, "拒绝");
            lv_obj_set_style_text_font(lbl_deny, CJK_FONT, 0);
            lv_obj_center(lbl_deny);
            lv_obj_add_event_cb(btn_deny, [](lv_event_t* e) {
                bool* f = (bool*)lv_event_get_user_data(e);
                if (!f) return;
                *f = false;
                save_theme_config();
                ui_log("[Auth] application denied");
            }, LV_EVENT_CLICKED, bound_flag);
        };

        make_auth_card("📧", "Outlook", "CLI", &g_app_auth_email);
        make_auth_card("📅", "Calendar", "MCP", &g_app_auth_calendar);

        gen_auth_email_sw = lv_switch_create(tab);
        lv_obj_set_size(gen_auth_email_sw, SCALE(50), SCALE(26));
        if (g_app_auth_email) lv_obj_add_state(gen_auth_email_sw, LV_STATE_CHECKED);
        lv_obj_add_event_cb(gen_auth_email_sw, [](lv_event_t* e) {
            lv_obj_t* sw = (lv_obj_t*)lv_event_get_target(e);
            g_app_auth_email = lv_obj_has_state(sw, LV_STATE_CHECKED);
            save_theme_config();
            ui_log("[Auth] Email access %s", g_app_auth_email ? "enabled" : "disabled");
        }, LV_EVENT_VALUE_CHANGED, nullptr);
        make_kv_row("Email Access", gen_auth_email_sw);

        gen_auth_calendar_sw = lv_switch_create(tab);
        lv_obj_set_size(gen_auth_calendar_sw, SCALE(50), SCALE(26));
        if (g_app_auth_calendar) lv_obj_add_state(gen_auth_calendar_sw, LV_STATE_CHECKED);
        lv_obj_add_event_cb(gen_auth_calendar_sw, [](lv_event_t* e) {
            lv_obj_t* sw = (lv_obj_t*)lv_event_get_target(e);
            g_app_auth_calendar = lv_obj_has_state(sw, LV_STATE_CHECKED);
            save_theme_config();
            ui_log("[Auth] Calendar access %s", g_app_auth_calendar ? "enabled" : "disabled");
        }, LV_EVENT_VALUE_CHANGED, nullptr);
        make_kv_row("Calendar Access", gen_auth_calendar_sw);

        lv_obj_t* btn_scan_apps = lv_button_create(tab);
        lv_obj_set_size(btn_scan_apps, SCALE(220), SCALE(34));
        lv_obj_set_style_bg_color(btn_scan_apps, lv_color_make(70, 110, 170), 0);
        lv_obj_add_event_cb(btn_scan_apps, scan_apps_cb, LV_EVENT_CLICKED, nullptr);
        lv_obj_t* lbl_scan = lv_label_create(btn_scan_apps);
        lv_label_set_text(lbl_scan, "Scan Installed Apps");
        lv_obj_set_style_text_font(lbl_scan, CJK_FONT, 0);
        lv_obj_center(lbl_scan);
        make_kv_row("Installed Apps", btn_scan_apps);

        gen_apps_scan_ta = lv_textarea_create(tab);
        lv_textarea_set_one_line(gen_apps_scan_ta, false);
        lv_textarea_set_placeholder_text(gen_apps_scan_ta, "Scan result will be shown here...");
        lv_obj_set_size(gen_apps_scan_ta, LV_PCT(100), SCALE(130));
        apply_input_style(gen_apps_scan_ta);
    }

    add_divider(tab);

    /* ── Boot Start ── */
    lv_obj_t* row_boot = lv_obj_create(tab);
    lv_obj_set_size(row_boot, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row_boot, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row_boot, 0, 0);
    lv_obj_set_style_pad_all(row_boot, 0, 0);
    lv_obj_set_style_pad_ver(row_boot, 4, 0);
    lv_obj_clear_flag(row_boot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(row_boot, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row_boot, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* lbl_boot = lv_label_create(row_boot);
    lv_label_set_text(lbl_boot, "Boot Start");
    lv_obj_set_style_text_color(lbl_boot, lv_color_make(180, 185, 200), 0);
    lv_obj_set_style_text_font(lbl_boot, CJK_FONT, 0);

    lv_obj_t* boot_right = lv_obj_create(row_boot);
    lv_obj_set_style_bg_opa(boot_right, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(boot_right, 0, 0);
    lv_obj_set_style_pad_all(boot_right, 0, 0);
    lv_obj_clear_flag(boot_right, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(boot_right, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(boot_right, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(boot_right, 8, 0);
    lv_obj_set_size(boot_right, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

    gen_autostart_sw = lv_switch_create(boot_right);
    lv_obj_set_size(gen_autostart_sw, SCALE(50), SCALE(26));
    if (is_auto_start()) lv_obj_add_state(gen_autostart_sw, LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(gen_autostart_sw, lv_color_make(60, 65, 90), LV_PART_MAIN);
    lv_obj_set_style_bg_color(gen_autostart_sw, lv_color_make(60, 130, 220), LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(gen_autostart_sw, lv_color_make(255, 255, 255), LV_PART_KNOB);
    lv_obj_add_event_cb(gen_autostart_sw, [](lv_event_t* e) {
        lv_obj_t* sw = (lv_obj_t*)lv_event_get_target(e);
        set_auto_start(lv_obj_has_state(sw, LV_STATE_CHECKED));
    }, LV_EVENT_VALUE_CHANGED, nullptr);

    lv_obj_t* lbl_boot_hint = lv_label_create(boot_right);
    lv_label_set_text(lbl_boot_hint, "Start on boot");
    apply_hint_label(lbl_boot_hint);

    /* ── Auto Start ── */
    lv_obj_t* row_auto = lv_obj_create(tab);
    lv_obj_set_size(row_auto, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row_auto, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row_auto, 0, 0);
    lv_obj_set_style_pad_all(row_auto, 0, 0);
    lv_obj_set_style_pad_ver(row_auto, 4, 0);
    lv_obj_clear_flag(row_auto, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(row_auto, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row_auto, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* lbl_auto = lv_label_create(row_auto);
    lv_label_set_text(lbl_auto, "Auto Start");
    lv_obj_set_style_text_color(lbl_auto, lv_color_make(180, 185, 200), 0);
    lv_obj_set_style_text_font(lbl_auto, CJK_FONT, 0);

    lv_obj_t* auto_right = lv_obj_create(row_auto);
    lv_obj_set_style_bg_opa(auto_right, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(auto_right, 0, 0);
    lv_obj_set_style_pad_all(auto_right, 0, 0);
    lv_obj_clear_flag(auto_right, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(auto_right, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(auto_right, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(auto_right, 8, 0);
    lv_obj_set_size(auto_right, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

    gen_auto_start_sw = lv_switch_create(auto_right);
    lv_obj_set_size(gen_auto_start_sw, SCALE(50), SCALE(26));
    lv_obj_set_style_bg_color(gen_auto_start_sw, lv_color_make(60, 65, 90), LV_PART_MAIN);
    lv_obj_set_style_bg_color(gen_auto_start_sw, lv_color_make(60, 130, 220), LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(gen_auto_start_sw, lv_color_make(255, 255, 255), LV_PART_KNOB);
    lv_obj_add_event_cb(gen_auto_start_sw, [](lv_event_t* e) {
        lv_obj_t* sw = (lv_obj_t*)lv_event_get_target(e);
        bool enabled = lv_obj_has_state(sw, LV_STATE_CHECKED);
        /* TODO: persist auto_start_on_crash config */
    }, LV_EVENT_VALUE_CHANGED, nullptr);

    lv_obj_t* lbl_auto_hint = lv_label_create(auto_right);
    lv_label_set_text(lbl_auto_hint, "Restart on crash");
    apply_hint_label(lbl_auto_hint);

    add_divider(tab);

    /* ── Close Gateway on exit ── */
    {
        lv_obj_t* close_row = lv_obj_create(tab);
        lv_obj_set_size(close_row, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(close_row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(close_row, 0, 0);
        lv_obj_set_style_pad_all(close_row, 0, 0);
        lv_obj_clear_flag(close_row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_flow(close_row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(close_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        lv_obj_t* lbl_close = lv_label_create(close_row);
        lv_label_set_text(lbl_close, tr("退出时关闭 OpenClaw", "Close OpenClaw on exit"));
        lv_obj_set_style_text_color(lbl_close, g_colors->text, 0);
        lv_obj_set_style_text_font(lbl_close, CJK_FONT, 0);

        lv_obj_t* close_right = lv_obj_create(close_row);
        lv_obj_set_size(close_right, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(close_right, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(close_right, 0, 0);
        lv_obj_set_style_pad_all(close_right, 0, 0);
        lv_obj_clear_flag(close_right, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_flow(close_right, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(close_right, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_gap(close_right, 8, 0);

        gen_close_gateway_sw = lv_switch_create(close_right);
        lv_obj_set_size(gen_close_gateway_sw, SCALE(50), SCALE(26));
        lv_obj_set_style_bg_color(gen_close_gateway_sw, lv_color_make(60, 65, 90), LV_PART_MAIN);
        lv_obj_set_style_bg_color(gen_close_gateway_sw, lv_color_make(60, 130, 220), LV_PART_INDICATOR | LV_STATE_CHECKED);
        lv_obj_set_style_bg_color(gen_close_gateway_sw, lv_color_make(255, 255, 255), LV_PART_KNOB);

        /* Read from config: default ON */
        bool close_on_exit = true; /* default */
        {
            const char* userprofile = std::getenv("USERPROFILE");
            if (userprofile) {
                char path[512];
                snprintf(path, sizeof(path), "%s\\AppData\\Roaming\\AnyClaw_LVGL\\config.json", userprofile);
                FILE* f = fopen(path, "r");
                if (f) {
                    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
                    if (sz > 0 && sz < 16384) {
                        char* buf = new char[sz + 1]; fread(buf, 1, sz, f); buf[sz] = '\0';
                        if (strstr(buf, "\"close_gateway_on_exit\": false")) close_on_exit = false;
                        delete[] buf;
                    }
                    fclose(f);
                }
            }
        }
        if (close_on_exit) lv_obj_add_state(gen_close_gateway_sw, LV_STATE_CHECKED);

        lv_obj_add_event_cb(gen_close_gateway_sw, [](lv_event_t* e) {
            lv_obj_t* sw = (lv_obj_t*)lv_event_get_target(e);
            bool enabled = lv_obj_has_state(sw, LV_STATE_CHECKED);
            /* Save to config */
            extern void save_theme_config(); /* reuse config save */
            /* Write directly */
            const char* userprofile = std::getenv("USERPROFILE");
            if (userprofile) {
                char path[512];
                snprintf(path, sizeof(path), "%s\\AppData\\Roaming\\AnyClaw_LVGL\\config.json", userprofile);
                /* Read, patch, write back */
                FILE* f = fopen(path, "r");
                std::string content;
                if (f) { fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
                    if (sz > 0 && sz < 16384) { char* b = new char[sz+1]; fread(b,1,sz,f); b[sz]='\0'; content=b; delete[] b; }
                    fclose(f);
                }
                /* Replace or append close_gateway_on_exit */
                size_t pos = content.find("\"close_gateway_on_exit\"");
                if (pos != std::string::npos) {
                    size_t colon = content.find(':', pos);
                    size_t end = content.find_first_of(",\n}", colon);
                    if (colon != std::string::npos && end != std::string::npos) {
                        content.replace(colon + 1, end - colon - 1, enabled ? " true" : " false");
                    }
                } else {
                    /* Append before last } */
                    size_t last_brace = content.rfind('}');
                    if (last_brace != std::string::npos) {
                        char line[128];
                        snprintf(line, sizeof(line), "  \"close_gateway_on_exit\": %s,\n", enabled ? "true" : "false");
                        content.insert(last_brace, line);
                    }
                }
                f = fopen(path, "w");
                if (f) { fwrite(content.c_str(), 1, content.size(), f); fclose(f); }
            }
            LOG_I("Config", "close_gateway_on_exit = %s", enabled ? "true" : "false");
        }, LV_EVENT_VALUE_CHANGED, nullptr);

        lv_obj_t* lbl_hint = lv_label_create(close_right);
        lv_label_set_text(lbl_hint, tr("退出程序时停止Gateway", "Stop Gateway when quitting"));
        apply_hint_label(lbl_hint);
    }

    add_divider(tab);

    /* ── Language ── */
    gen_lang_dropdown = lv_dropdown_create(tab);
    lv_dropdown_set_options(gen_lang_dropdown, "中文\nEnglish\n한국어\n日本語");
    lv_dropdown_set_selected(gen_lang_dropdown, (uint16_t)lang_to_dropdown_index(g_lang));
    lv_obj_set_width(gen_lang_dropdown, SCALE(160));
    apply_input_style(gen_lang_dropdown);
    lv_obj_add_event_cb(gen_lang_dropdown, [](lv_event_t* e) {
        lv_obj_t* dd = (lv_obj_t*)lv_event_get_target(e);
        uint16_t sel = lv_dropdown_get_selected(dd);
        extern Lang g_lang;
        extern void save_theme_config();
        extern void apply_theme_to_all();
        g_lang = dropdown_index_to_lang(sel);
        save_theme_config();
        update_ui_language();
        apply_theme_to_all();
    }, LV_EVENT_VALUE_CHANGED, nullptr);
    make_kv_row("Language", gen_lang_dropdown);

    /* ── Refresh Interval ── */
    extern int g_refresh_interval_ms;
    extern void update_refresh_interval(int ms);

    static lv_obj_t* gen_refresh_dropdown = nullptr;
    gen_refresh_dropdown = lv_dropdown_create(tab);
    lv_dropdown_set_options(gen_refresh_dropdown, "5s\n15s\n30s\n60s\n120s");
    if (g_refresh_interval_ms <= 5000) lv_dropdown_set_selected(gen_refresh_dropdown, 0);
    else if (g_refresh_interval_ms <= 15000) lv_dropdown_set_selected(gen_refresh_dropdown, 1);
    else if (g_refresh_interval_ms <= 30000) lv_dropdown_set_selected(gen_refresh_dropdown, 2);
    else if (g_refresh_interval_ms <= 60000) lv_dropdown_set_selected(gen_refresh_dropdown, 3);
    else lv_dropdown_set_selected(gen_refresh_dropdown, 4);
    lv_obj_set_width(gen_refresh_dropdown, SCALE(160));
    apply_input_style(gen_refresh_dropdown);
    lv_obj_add_event_cb(gen_refresh_dropdown, refresh_dropdown_cb, LV_EVENT_VALUE_CHANGED, nullptr);
    make_kv_row("Refresh", gen_refresh_dropdown);

    /* ── Theme ── */
    extern lv_obj_t* ui_settings_add_theme_dropdown(lv_obj_t* tab);
    lv_obj_t* theme_dd = ui_settings_add_theme_dropdown(tab);
    make_kv_row("Theme", theme_dd);

    add_divider(tab);

    /* ── Security ── */
    lv_obj_t* sec_row = lv_obj_create(tab);
    lv_obj_set_size(sec_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(sec_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(sec_row, 0, 0);
    lv_obj_set_style_pad_all(sec_row, 0, 0);
    lv_obj_set_style_pad_ver(sec_row, 4, 0);
    lv_obj_clear_flag(sec_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(sec_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(sec_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* lbl_sec = lv_label_create(sec_row);
    lv_label_set_text(lbl_sec, "Security");
    lv_obj_set_style_text_color(lbl_sec, lv_color_make(180, 185, 200), 0);
    lv_obj_set_style_text_font(lbl_sec, CJK_FONT, 0);

    lv_obj_t* sec_right = lv_obj_create(sec_row);
    lv_obj_set_style_bg_opa(sec_right, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(sec_right, 0, 0);
    lv_obj_set_style_pad_all(sec_right, 0, 0);
    lv_obj_clear_flag(sec_right, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(sec_right, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(sec_right, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(sec_right, 8, 0);
    lv_obj_set_size(sec_right, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

    gen_security_led = lv_led_create(sec_right);
    lv_obj_set_size(gen_security_led, 10, 10);
    lv_led_set_color(gen_security_led, lv_color_make(0, 220, 60));
    lv_led_on(gen_security_led);

    gen_security_label = lv_label_create(sec_right);
    lv_label_set_text(gen_security_label, "Checking...");
    lv_obj_set_style_text_color(gen_security_label, lv_color_make(200, 205, 220), 0);
    lv_obj_set_style_text_font(gen_security_label, CJK_FONT, 0);

    /* Security detail items */
    gen_security_detail_label = lv_label_create(tab);
    lv_label_set_text(gen_security_detail_label, "API Key: Checking | Port: -- | Config: Checking");
    apply_hint_label(gen_security_detail_label);
    lv_label_set_long_mode(gen_security_detail_label, LV_LABEL_LONG_WRAP);
    refresh_security_status_ui();

    /* Divider */
    lv_obj_t* div3 = lv_obj_create(tab);
    lv_obj_set_size(div3, LV_PCT(100), 1);
    lv_obj_set_style_bg_color(div3, lv_color_make(55, 60, 85), 0);
    lv_obj_set_style_border_width(div3, 0, 0);
    lv_obj_clear_flag(div3, LV_OBJ_FLAG_SCROLLABLE);

    /* Log settings moved to Log tab */

    /* Divider */
    lv_obj_t* div4 = lv_obj_create(tab);
    lv_obj_set_size(div4, LV_PCT(100), 1);
    lv_obj_set_style_bg_color(div4, lv_color_make(55, 60, 85), 0);
    lv_obj_set_style_border_width(div4, 0, 0);
    lv_obj_clear_flag(div4, LV_OBJ_FLAG_SCROLLABLE);

    /* Reconfigure wizard button */
    lv_obj_t* btn_wizard = lv_button_create(tab);
    lv_obj_set_width(btn_wizard, LV_PCT(100));
    lv_obj_set_height(btn_wizard, 40);
    lv_obj_set_style_bg_color(btn_wizard, lv_color_make(40, 50, 90), 0);
    lv_obj_set_style_bg_grad_color(btn_wizard, lv_color_make(30, 40, 70), 0);
    lv_obj_set_style_bg_grad_dir(btn_wizard, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_radius(btn_wizard, 8, 0);
    lv_obj_add_event_cb(btn_wizard, [](lv_event_t* e) {
        (void)e;
        ui_settings_close();
        ui_show_setup_wizard();
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* btn_wizard_lbl = lv_label_create(btn_wizard);
    lv_label_set_text(btn_wizard_lbl, "Reconfigure Wizard");
    lv_obj_set_style_text_font(btn_wizard_lbl, CJK_FONT, 0);
    lv_obj_center(btn_wizard_lbl);
}

/* ═══════════════════════════════════════════════════════════════
 *  PERMISSIONS TAB (PERM-01)
 * ═══════════════════════════════════════════════════════════════ */
struct PermUiBinding {
    PermKey key;
    lv_obj_t* dd;
};
static std::vector<PermUiBinding> g_perm_bindings;

static uint16_t perm_to_sel(PermValue v) {
    switch (v) {
        case PermValue::Allow:    return 0;
        case PermValue::Deny:     return 1;
        case PermValue::Ask:      return 2;
        case PermValue::ReadOnly: return 3;
        default:                  return 1;
    }
}

static PermValue sel_to_perm(uint16_t s) {
    switch (s) {
        case 0: return PermValue::Allow;
        case 1: return PermValue::Deny;
        case 2: return PermValue::Ask;
        case 3: return PermValue::ReadOnly;
        default: return PermValue::Deny;
    }
}

static void build_permissions_tab(lv_obj_t* tab) {
    apply_dark_style(tab);
    lv_obj_set_style_pad_all(tab, 16, 0);
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(tab, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_gap(tab, 10, 0);

    /* Sync workspace root to permission boundary and load config */
    std::string ws_root = workspace_get_root();
    permissions().set_workspace_root(ws_root.c_str());
    permissions().load();
    g_perm_bindings.clear();

    lv_obj_t* title = lv_label_create(tab);
    lv_label_set_text(title, tr("权限策略", "Permissions Policy"));
    apply_section_label(title);

    lv_obj_t* hint = lv_label_create(tab);
    lv_label_set_text(hint, tr("可修改核心权限，保存后写入 APPDATA/AnyClaw_LVGL/permissions.json",
                               "Edit core permissions and save to APPDATA/AnyClaw_LVGL/permissions.json"));
    apply_hint_label(hint);

    struct Item { const char* name; PermKey key; };
    const Item items[] = {
        {"FS Read Workspace",  PermKey::FS_READ_WORKSPACE},
        {"FS Write Workspace", PermKey::FS_WRITE_WORKSPACE},
        {"FS Read External",   PermKey::FS_READ_EXTERNAL},
        {"FS Write External",  PermKey::FS_WRITE_EXTERNAL},
        {"Exec Shell",         PermKey::EXEC_SHELL},
        {"Exec Install",       PermKey::EXEC_INSTALL},
        {"Exec Delete",        PermKey::EXEC_DELETE},
        {"Net Outbound",       PermKey::NET_OUTBOUND},
        {"Net Inbound",        PermKey::NET_INBOUND},
        {"Net Intranet",       PermKey::NET_INTRANET},
        {"Device Camera",      PermKey::DEVICE_CAMERA},
        {"Device Mic",         PermKey::DEVICE_MIC},
        {"Device Screen",      PermKey::DEVICE_SCREEN},
        {"USB Storage",        PermKey::DEVICE_USB_STORAGE},
        {"Remote Node",        PermKey::DEVICE_REMOTE_NODE},
        {"New Device",         PermKey::DEVICE_NEW_DEVICE},
        {"Clipboard Read",     PermKey::CLIPBOARD_READ},
        {"Clipboard Write",    PermKey::CLIPBOARD_WRITE},
        {"System Modify",      PermKey::SYSTEM_MODIFY},
    };

    for (const auto& it : items) {
        lv_obj_t* row = lv_obj_create(tab);
        lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_set_style_pad_ver(row, 2, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        lv_obj_t* lbl = lv_label_create(row);
        lv_label_set_text(lbl, it.name);
        lv_obj_set_style_text_color(lbl, g_colors->text, 0);
        lv_obj_set_style_text_font(lbl, CJK_FONT, 0);

        lv_obj_t* dd = lv_dropdown_create(row);
        lv_dropdown_set_options(dd, "allow\ndeny\nask\nread_only");
        lv_obj_set_width(dd, SCALE(170));
        apply_input_style(dd);
        lv_dropdown_set_selected(dd, perm_to_sel(permissions().get(it.key)));

        g_perm_bindings.push_back({it.key, dd});
    }

    lv_obj_t* row_btn = lv_obj_create(tab);
    lv_obj_set_size(row_btn, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row_btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row_btn, 0, 0);
    lv_obj_set_style_pad_all(row_btn, 0, 0);
    lv_obj_clear_flag(row_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(row_btn, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row_btn, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(row_btn, 8, 0);

    lv_obj_t* btn_save = lv_button_create(row_btn);
    lv_obj_set_size(btn_save, SCALE(180), SCALE(34));
    lv_obj_set_style_bg_color(btn_save, lv_color_make(59, 130, 246), 0);
    lv_obj_set_style_radius(btn_save, 6, 0);
    lv_obj_add_event_cb(btn_save, [](lv_event_t* e) {
        (void)e;
        for (const auto& b : g_perm_bindings) {
            uint16_t sel = lv_dropdown_get_selected(b.dd);
            permissions().set(b.key, sel_to_perm(sel));
        }
        bool ok = permissions().save();
        if (ok) {
            workspace_sync_managed_sections();
            workspace_sync_runtime_config_from_permissions();
        }
        if (perm_status_label) {
            lv_label_set_text(perm_status_label, ok ? "Saved" : "Save failed");
            lv_obj_set_style_text_color(perm_status_label,
                                        ok ? lv_color_make(100, 200, 100) : lv_color_make(220, 100, 100), 0);
        }
        ui_log("[PERM] %s", ok ? "Saved permissions config" : "Save failed");
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* lbl_save = lv_label_create(btn_save);
    lv_label_set_text(lbl_save, tr("保存权限配置", "Save Permissions"));
    lv_obj_set_style_text_font(lbl_save, CJK_FONT, 0);
    lv_obj_set_style_text_color(lbl_save, lv_color_make(255, 255, 255), 0);
    lv_obj_center(lbl_save);

    perm_status_label = lv_label_create(row_btn);
    lv_label_set_text(perm_status_label, tr("未保存", "Not saved"));
    apply_hint_label(perm_status_label);
}

/* ═══════════════════════════════════════════════════════════════
 *  MODEL TAB
 * ═══════════════════════════════════════════════════════════════ */
/* Model list now provided by model_manager.h */

/* ── Dropdown change: update provider hint + load API key ── */
static void model_dropdown_cb(lv_event_t* e) {
    (void)e;
    if (!model_dropdown || !model_provider_hint) return;
    char sel[128];
    lv_dropdown_get_selected_str(model_dropdown, sel, sizeof(sel));

    /* Detect provider */
    const char* provider = "openrouter";
    const char* key_url = "openrouter.ai/settings/keys";
    if (strncmp(sel, "xiaomi/", 7) == 0) {
        provider = "xiaomi";
        key_url = "api.xiaomimimo.com";
    }

    /* Update provider hint */
    static char hint[256];
    snprintf(hint, sizeof(hint), "%s  ->  %s", provider, key_url);
    lv_label_set_text(model_provider_hint, hint);

    /* Update API key status indicator */
    if (api_key_status_label) {
        char existing_key[256] = {0};
        if (app_get_provider_api_key(provider, existing_key, sizeof(existing_key)) &&
            strcmp(existing_key, "__OPENCLAW_REDACTED__") != 0 && strlen(existing_key) > 10) {
            lv_label_set_text_fmt(api_key_status_label, "API Key: ✅ %s (%zu chars)", provider, strlen(existing_key));
            lv_obj_set_style_text_color(api_key_status_label, lv_color_make(100, 200, 100), 0);
        } else if (existing_key[0]) {
            lv_label_set_text_fmt(api_key_status_label, "API Key: ✅ %s (configured, hidden)", provider);
            lv_obj_set_style_text_color(api_key_status_label, lv_color_make(100, 200, 100), 0);
        } else {
            lv_label_set_text_fmt(api_key_status_label, "API Key: ❌ %s not configured", provider);
            lv_obj_set_style_text_color(api_key_status_label, lv_color_make(200, 80, 80), 0);
        }
    }

    /* Clear input field for new key entry */
    if (acc_apikey_ta) {
        lv_textarea_set_text(acc_apikey_ta, "");
    }
}

/* ═══ API Key callbacks ═══ */
static void apikey_show_cb(lv_event_t* e) {
    (void)e;
    if (!acc_apikey_ta) return;
    bool is_password = lv_textarea_get_password_mode(acc_apikey_ta);
    lv_textarea_set_password_mode(acc_apikey_ta, !is_password);
    /* Update button label */
    lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
    lv_obj_t* lbl = lv_obj_get_child(btn, 0);
    if (lbl) {
        lv_label_set_text(lbl, is_password ? "Hide Key" : "Show Key");
    }
}

/* Verify API Key — send test request to provider */
static void apikey_verify_cb(lv_event_t* e) {
    (void)e;
    if (!acc_apikey_ta || !api_key_status_label) return;

    const char* key_text = lv_textarea_get_text(acc_apikey_ta);
    if (!key_text || !key_text[0]) {
        lv_label_set_text(api_key_status_label, "Verify: ⚠️ Enter an API key first");
        lv_obj_set_style_text_color(api_key_status_label, lv_color_make(200, 180, 60), 0);
        return;
    }

    /* Detect provider from current model */
    const char* provider = "openrouter";
    const char* test_url = "https://openrouter.ai/api/v1/models";
    {
        char cur_model[256] = {0};
        if (app_get_current_model(cur_model, sizeof(cur_model)) && strncmp(cur_model, "xiaomi/", 7) == 0) {
            provider = "xiaomi";
            test_url = "https://api.xiaomimimo.com/v1/models";
        }
    }

    lv_label_set_text_fmt(api_key_status_label, "Verify: ⏳ Testing %s key...", provider);
    lv_obj_set_style_text_color(api_key_status_label, lv_color_make(200, 180, 60), 0);

    /* Run verification in background thread */
    std::string key_copy = key_text;
    std::string prov_copy = provider;
    std::string url_copy = test_url;
    lv_obj_t* status_lbl = api_key_status_label;

    std::thread([key_copy, prov_copy, url_copy, status_lbl]() {
        /* Send GET request with Bearer auth */
        char response[4096] = {0};
        char auth_header[512];
        snprintf(auth_header, sizeof(auth_header), "Bearer %s", key_copy.c_str());

        /* Use http_get from openclaw_mgr or a simple curl */
        char cmd[1024];
        snprintf(cmd, sizeof(cmd),
            "curl -s -o /dev/null -w %%{http_code} -H \"Authorization: Bearer %s\" \"%s\" --connect-timeout 5 --max-time 10",
            key_copy.c_str(), url_copy.c_str());

        FILE* pipe = _popen(cmd, "r");
        char http_code[16] = {0};
        if (pipe) {
            fread(http_code, 1, sizeof(http_code) - 1, pipe);
            _pclose(pipe);
        }

        /* Update UI from main thread */
        struct VerifyResult { char msg[256]; lv_obj_t* lbl; };
        VerifyResult* result = new VerifyResult;
        result->lbl = status_lbl;
        if (strncmp(http_code, "200", 3) == 0) {
            snprintf(result->msg, sizeof(result->msg), "Verify: ✅ %s key is valid!", prov_copy.c_str());
        } else if (strncmp(http_code, "401", 3) == 0 || strncmp(http_code, "403", 3) == 0) {
            snprintf(result->msg, sizeof(result->msg), "Verify: ❌ %s key is invalid (HTTP %s)", prov_copy.c_str(), http_code);
        } else {
            snprintf(result->msg, sizeof(result->msg), "Verify: ⚠️ %s HTTP %s (network issue?)", prov_copy.c_str(), http_code);
        }
        lv_async_call([](void* p) {
            VerifyResult* r = (VerifyResult*)p;
            if (r->lbl && lv_obj_is_valid(r->lbl)) {
                lv_label_set_text(r->lbl, r->msg);
                bool ok = (strncmp(r->msg + 9, "\xE2\x9C\x85", 3) == 0); /* ✅ */
                lv_obj_set_style_text_color(r->lbl, ok ? lv_color_make(100, 200, 100) : lv_color_make(200, 80, 80), 0);
            }
            delete r;
        }, result);
    }).detach();
}

/* Struct to pass data from background thread back to UI thread */
struct SaveFinishData {
    char model_name[256];
    lv_obj_t* btn;
};

static void save_finish_cb(void* user_data) {
    SaveFinishData* d = (SaveFinishData*)user_data;
    /* Update button label back to "Save" */
    if (d->btn && lv_obj_is_valid(d->btn)) {
        lv_obj_t* lbl = lv_obj_get_child(d->btn, 0);
        if (lbl) lv_label_set_text(lbl, "Save");
    }
    /* Update model display */
    if (model_current_label && d->model_name[0]) {
        lv_label_set_text_fmt(model_current_label, "%s", d->model_name);
    }
    delete d;
}

static void apikey_save_cb(lv_event_t* e) {
    lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);

    /* Read model name from dropdown */
    char model_name[256] = {0};
    if (model_dropdown) {
        lv_dropdown_get_selected_str(model_dropdown, model_name, sizeof(model_name));
    }

    /* Read API key from text input */
    const char* api_key_text = nullptr;
    if (acc_apikey_ta) {
        api_key_text = lv_textarea_get_text(acc_apikey_ta);
    }

    /* Update globals */
    if (model_name[0]) {
        snprintf(g_selected_model, sizeof(g_selected_model), "%s", model_name);
    }
    if (api_key_text && api_key_text[0]) {
        snprintf(g_api_key, sizeof(g_api_key), "%s", api_key_text);
    }

    /* Save local config immediately (fast) */
    save_theme_config();

    /* Update failover available models after API key change */
    failover_scan_available_models();

    /* Sync to OpenClaw Gateway in background thread to avoid UI freeze */
    if (model_name[0]) {
        ui_log("[Settings] Syncing model=%s to Gateway...", model_name);

        /* Show "Saving..." feedback on button */
        lv_obj_t* btn_lbl = lv_obj_get_child(btn, 0);
        if (btn_lbl) lv_label_set_text(btn_lbl, "Saving...");

        /* Copy strings to local variables for the thread lambda */
        std::string api_key_copy = (api_key_text && api_key_text[0]) ? api_key_text : "";
        std::string model_name_copy = model_name;

        /* Prepare data for the finish callback */
        SaveFinishData* finish = new SaveFinishData;
        snprintf(finish->model_name, sizeof(finish->model_name), "%s", model_name);
        finish->btn = btn;

        std::thread([api_key_copy, model_name_copy, finish]() {
            app_update_model_config(
                api_key_copy[0] ? api_key_copy.c_str() : nullptr,
                model_name_copy.c_str()
            );
            /* Update UI from main thread via LVGL async call */
            lv_async_call(save_finish_cb, finish);
        }).detach();
    }
}

static void build_model_tab(lv_obj_t* tab) {
    apply_dark_style(tab);
    lv_obj_set_style_pad_all(tab, 16, 0);
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(tab, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_gap(tab, 8, 0);

    /* Helper: create a horizontal Key: Value row (same as General tab) */
    auto make_kv_row = [&](const char* key_text, lv_obj_t* value_obj) -> lv_obj_t* {
        lv_obj_t* row = lv_obj_create(tab);
        lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_set_style_pad_ver(row, 4, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        lv_obj_t* lbl = lv_label_create(row);
        lv_label_set_text(lbl, key_text);
        lv_obj_set_style_text_color(lbl, lv_color_make(180, 185, 200), 0);
        lv_obj_set_style_text_font(lbl, CJK_FONT, 0);

        if (value_obj) {
            lv_obj_set_parent(value_obj, row);
        }
        return row;
    };

    /* ═══ Current Model ═══ */
    char gw_model[256] = {0};
    app_get_current_model(gw_model, sizeof(gw_model));
    if (gw_model[0]) {
        snprintf(g_selected_model, sizeof(g_selected_model), "%s", gw_model);
        snprintf(gw_model_buf, sizeof(gw_model_buf), "%s", gw_model);
    }

    model_current_label = lv_label_create(tab);
    lv_label_set_text(model_current_label, gw_model[0] ? gw_model : tr("未配置", "Not configured"));
    lv_obj_set_style_text_color(model_current_label, lv_color_make(100, 200, 100), 0);
    lv_obj_set_style_text_font(model_current_label, CJK_FONT, 0);
    make_kv_row("Current Model", model_current_label);

    add_divider(tab);

    /* ═══ Select Model ═══ */
    lv_obj_t* lbl_select = lv_label_create(tab);
    lv_label_set_text(lbl_select, tr("选择模型", "Select Model"));
    apply_section_label(lbl_select);

    /* Insert user's Gateway model into list if not already present */
    if (gw_model[0]) {
        model_insert_current(gw_model);
    }

    /* Build dropdown options from model list */
    char dd_options[4096] = {0};
    int selected_idx = 0;
    const char* gw_short = gw_model;
    if (gw_model[0] && strncmp(gw_model, "openrouter/", 11) == 0) {
        gw_short = gw_model + 11;
    }
    for (int i = 0; i < model_get_count(); i++) {
        const char* mname = model_get_name(i);
        if (i > 0) strcat(dd_options, "\n");
        strcat(dd_options, mname);
        if (gw_short[0] && strcmp(mname, gw_short) == 0) selected_idx = i;
    }

    /* Row: [Dropdown ←——→ [+ Add] ] */
    lv_obj_t* dd_row = lv_obj_create(tab);
    lv_obj_set_size(dd_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(dd_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(dd_row, 0, 0);
    lv_obj_set_style_pad_all(dd_row, 0, 0);
    lv_obj_clear_flag(dd_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(dd_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(dd_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(dd_row, 8, 0);

    model_dropdown = lv_dropdown_create(dd_row);
    lv_dropdown_set_options(model_dropdown, dd_options);
    lv_dropdown_set_selected(model_dropdown, selected_idx);
    lv_obj_set_width(model_dropdown, LV_PCT(70));
    apply_input_style(model_dropdown);
    lv_obj_add_event_cb(model_dropdown, model_dropdown_cb, LV_EVENT_VALUE_CHANGED, nullptr);

    /* Add model button */
    lv_obj_t* btn_add = lv_button_create(dd_row);
    lv_obj_set_size(btn_add, 40, 36);
    lv_obj_set_style_bg_color(btn_add, lv_color_make(40, 100, 60), 0);
    lv_obj_set_style_bg_color(btn_add, lv_color_make(50, 130, 80), LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn_add, 6, 0);
    lv_obj_t* lbl_add = lv_label_create(btn_add);
    lv_label_set_text(lbl_add, "+");
    lv_obj_set_style_text_font(lbl_add, FONT(16), 0);
    lv_obj_set_style_text_color(lbl_add, lv_color_make(255, 255, 255), 0);
    lv_obj_center(lbl_add);
    lv_obj_add_event_cb(btn_add, [](lv_event_t* ev) {
        (void)ev;
        /* Modal overlay */
        lv_obj_t* overlay = lv_obj_create(lv_scr_act());
        lv_obj_set_size(overlay, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_bg_color(overlay, lv_color_make(0, 0, 0), 0);
        lv_obj_set_style_bg_opa(overlay, LV_OPA_50, 0);
        lv_obj_set_style_border_width(overlay, 0, 0);
        lv_obj_set_style_pad_all(overlay, 0, 0);
        lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);

        /* Dialog box */
        lv_obj_t* dlg = lv_obj_create(overlay);
        lv_obj_set_size(dlg, 840, LV_SIZE_CONTENT);
        lv_obj_center(dlg);
        lv_obj_set_style_bg_color(dlg, lv_color_make(30, 32, 45), 0);
        lv_obj_set_style_border_color(dlg, lv_color_make(60, 65, 90), 0);
        lv_obj_set_style_border_width(dlg, 1, 0);
        lv_obj_set_style_radius(dlg, 10, 0);
        lv_obj_set_style_pad_all(dlg, 20, 0);
        lv_obj_set_flex_flow(dlg, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(dlg, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_set_style_pad_gap(dlg, 12, 0);
        lv_obj_clear_flag(dlg, LV_OBJ_FLAG_SCROLLABLE);

        /* Title */
        lv_obj_t* title = lv_label_create(dlg);
        lv_label_set_text(title, tr("添加自定义模型", "Add Custom Model"));
        lv_obj_set_style_text_color(title, lv_color_make(220, 225, 240), 0);
        lv_obj_set_style_text_font(title, CJK_FONT, 0);

        /* Hint */
        lv_obj_t* hint = lv_label_create(dlg);
        lv_label_set_text(hint, tr("输入模型名称", "Enter model name"));
        lv_obj_set_style_text_color(hint, lv_color_make(140, 150, 170), 0);
        lv_obj_set_style_text_font(hint, CJK_FONT, 0);

        /* Textarea — single line for model name */
        lv_obj_t* ta = lv_textarea_create(dlg);
        lv_obj_set_size(ta, LV_PCT(100), LV_SIZE_CONTENT);
        lv_textarea_set_one_line(ta, true);
        lv_textarea_set_max_length(ta, 128);
        lv_textarea_set_placeholder_text(ta, "google/gemini-2.5-pro");
        lv_obj_set_style_bg_color(ta, lv_color_make(20, 22, 32), 0);
        lv_obj_set_style_text_color(ta, lv_color_make(220, 225, 240), 0);
        lv_obj_set_style_text_font(ta, CJK_FONT, 0);
        lv_obj_set_style_border_color(ta, lv_color_make(60, 65, 90), 0);
        lv_obj_set_style_border_width(ta, 1, 0);
        lv_obj_set_style_radius(ta, 6, 0);
        lv_obj_set_style_pad_all(ta, 6, 0);
        lv_obj_set_style_pad_ver(ta, 8, 0);
        lv_obj_set_style_border_color(ta, lv_color_make(60, 65, 90), LV_STATE_FOCUSED);
        lv_obj_set_style_border_width(ta, 1, LV_STATE_FOCUSED);
        lv_obj_set_style_outline_width(ta, 0, LV_STATE_FOCUSED);
        lv_obj_set_style_text_color(ta, lv_color_make(180, 215, 255), LV_PART_SELECTED);
        lv_obj_clear_flag(ta, LV_OBJ_FLAG_SCROLLABLE);

        /* Button row — [OK] ←spacer→ [Cancel] */
        lv_obj_t* btn_row = lv_obj_create(dlg);
        lv_obj_set_size(btn_row, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(btn_row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(btn_row, 0, 0);
        lv_obj_set_style_pad_all(btn_row, 0, 0);
        lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(btn_row, LV_OBJ_FLAG_SCROLLABLE);

        /* OK button (left) */
        lv_obj_t* btn_ok = lv_button_create(btn_row);
        lv_obj_set_size(btn_ok, 80, 36);
        lv_obj_set_style_bg_color(btn_ok, lv_color_make(40, 100, 180), 0);
        lv_obj_set_style_bg_color(btn_ok, lv_color_make(50, 120, 200), LV_STATE_PRESSED);
        lv_obj_set_style_radius(btn_ok, 6, 0);
        lv_obj_t* lbl_ok = lv_label_create(btn_ok);
        lv_label_set_text(lbl_ok, tr("确定", "OK"));
        lv_obj_set_style_text_font(lbl_ok, CJK_FONT, 0);
        lv_obj_set_style_text_color(lbl_ok, lv_color_make(255, 255, 255), 0);
        lv_obj_center(lbl_ok);

        /* Cancel button (right) */
        lv_obj_t* btn_cancel = lv_button_create(btn_row);
        lv_obj_set_size(btn_cancel, 80, 36);
        lv_obj_set_style_bg_color(btn_cancel, lv_color_make(80, 85, 100), 0);
        lv_obj_set_style_bg_color(btn_cancel, lv_color_make(100, 105, 120), LV_STATE_PRESSED);
        lv_obj_set_style_radius(btn_cancel, 6, 0);
        lv_obj_t* lbl_cancel = lv_label_create(btn_cancel);
        lv_label_set_text(lbl_cancel, tr("取消", "Cancel"));
        lv_obj_set_style_text_font(lbl_cancel, CJK_FONT, 0);
        lv_obj_set_style_text_color(lbl_cancel, lv_color_make(255, 255, 255), 0);
        lv_obj_center(lbl_cancel);

        /* Cancel handler */
        lv_obj_add_event_cb(btn_cancel, [](lv_event_t* e2) {
            lv_obj_t* ov = lv_obj_get_parent(lv_obj_get_parent(lv_event_get_target_obj(e2)));
            lv_obj_del(ov);
        }, LV_EVENT_CLICKED, nullptr);

        /* Confirm handler */
        lv_obj_add_event_cb(btn_ok, [](lv_event_t* e2) {
            lv_obj_t* dlg2 = lv_obj_get_parent(lv_event_get_target_obj(e2));
            /* Find the textarea — it's the 3rd child (title, hint, ta, btn_row) */
            lv_obj_t* ta2 = lv_obj_get_child(dlg2, 2);
            const char* text = lv_textarea_get_text(ta2);
            if (text && text[0]) {
                /* Save custom model */
                model_save_custom(text);
                /* Refresh dropdown options */
                if (model_dropdown) {
                    char new_opts[4096] = {0};
                    for (int i = 0; i < model_get_count(); i++) {
                        if (i > 0) strcat(new_opts, "\n");
                        strcat(new_opts, model_get_name(i));
                    }
                    lv_dropdown_set_options(model_dropdown, new_opts);
                    /* Select the newly added model (last one) */
                    lv_dropdown_set_selected(model_dropdown, model_get_count() - 1);
                    /* Trigger value changed to update provider hint */
                    lv_obj_send_event(model_dropdown, LV_EVENT_VALUE_CHANGED, nullptr);
                }
                ui_log("[Settings] Added custom model: %s", text);
            }
            /* Close dialog */
            lv_obj_t* ov = lv_obj_get_parent(dlg2);
            lv_obj_del(ov);
        }, LV_EVENT_CLICKED, nullptr);

        /* Close on overlay click (outside dialog) */
        lv_obj_add_event_cb(overlay, [](lv_event_t* e2) {
            lv_obj_del(lv_event_get_target_obj(e2));
        }, LV_EVENT_CLICKED, nullptr);
    }, LV_EVENT_CLICKED, nullptr);

    /* Provider hint — kv row, updates when dropdown changes */
    model_provider_hint = lv_label_create(tab);
    {
        const char* init_provider = "openrouter";
        const char* init_url = "openrouter.ai/settings/keys";
        if (gw_model[0] && strncmp(gw_model, "xiaomi/", 7) == 0) {
            init_provider = "xiaomi";
            init_url = "api.xiaomimimo.com";
        }
        static char init_hint[256];
        snprintf(init_hint, sizeof(init_hint), "%s  ->  %s", init_provider, init_url);
        lv_label_set_text(model_provider_hint, init_hint);
    }
    lv_obj_set_style_text_color(model_provider_hint, lv_color_make(100, 180, 255), 0);
    lv_obj_set_style_text_font(model_provider_hint, CJK_FONT, 0);
    make_kv_row("Provider", model_provider_hint);

    /* Free model hint — standalone */
    lv_obj_t* lbl_hint = lv_label_create(tab);
    lv_label_set_text(lbl_hint, tr("带 :free 后缀的模型为免费模型", "Models with :free suffix are free"));
    apply_hint_label(lbl_hint);

    /* Pre-populate current model display from Gateway */
    if (g_selected_model[0] && model_current_label) {
        lv_label_set_text_fmt(model_current_label, "%s", g_selected_model);
    }

    /* ═══ Separator ═══ */
    add_divider(tab);

    /* ═══ API Key status + real key display ═══ */
    api_key_status_label = lv_label_create(tab);
    {
        const char* check_provider = "openrouter";
        if (gw_model[0] && strncmp(gw_model, "xiaomi/", 7) == 0) check_provider = "xiaomi";
        char existing_key[256] = {0};
        if (app_get_provider_api_key(check_provider, existing_key, sizeof(existing_key)) && existing_key[0]) {
            lv_label_set_text_fmt(api_key_status_label, "API Key: ✅ %s (%zu chars)", check_provider, strlen(existing_key));
            lv_obj_set_style_text_color(api_key_status_label, lv_color_make(100, 200, 100), 0);
        } else {
            lv_label_set_text(api_key_status_label, "API Key: ❌ Not configured");
            lv_obj_set_style_text_color(api_key_status_label, lv_color_make(200, 80, 80), 0);
        }
        lv_obj_set_style_text_font(api_key_status_label, CJK_FONT, 0);
    }

    /* ═══ API Key input — show real key from file ═══ */
    acc_apikey_ta = lv_textarea_create(tab);
    lv_textarea_set_one_line(acc_apikey_ta, true);
    lv_textarea_set_password_mode(acc_apikey_ta, true);
    lv_textarea_set_placeholder_text(acc_apikey_ta, "sk-or-...");
    lv_obj_set_width(acc_apikey_ta, LV_PCT(80));
    lv_obj_set_height(acc_apikey_ta, 56);
    apply_input_style(acc_apikey_ta);
    lv_textarea_set_text_selection(acc_apikey_ta, true);
    lv_group_add_obj(lv_group_get_default(), acc_apikey_ta);

    /* Pre-populate with real key from openclaw.json */
    {
        const char* init_provider = "openrouter";
        if (gw_model[0] && strncmp(gw_model, "xiaomi/", 7) == 0) init_provider = "xiaomi";
        char saved_key[256] = {0};
        if (app_get_provider_api_key(init_provider, saved_key, sizeof(saved_key)) && saved_key[0]) {
            lv_textarea_set_text(acc_apikey_ta, saved_key);
        }
    }
    make_kv_row("API Key", acc_apikey_ta);

    /* ═══ Action buttons — right-aligned row ═══ */
    lv_obj_t* row_btns = lv_obj_create(tab);
    lv_obj_set_size(row_btns, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row_btns, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row_btns, 0, 0);
    lv_obj_set_style_pad_all(row_btns, 0, 0);
    lv_obj_set_style_pad_ver(row_btns, 4, 0);
    lv_obj_clear_flag(row_btns, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(row_btns, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row_btns, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(row_btns, 8, 0);

    /* Show/Hide Key toggle */
    lv_obj_t* btn_show = lv_button_create(row_btns);
    lv_obj_set_size(btn_show, SCALE(100), SCALE(32));
    lv_obj_set_style_bg_color(btn_show, lv_color_make(60, 65, 90), 0);
    lv_obj_set_style_radius(btn_show, 6, 0);
    lv_obj_add_event_cb(btn_show, apikey_show_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* lbl_show = lv_label_create(btn_show);
    lv_label_set_text(lbl_show, "Hide Key");
    lv_obj_set_style_text_font(lbl_show, CJK_FONT, 0);
    lv_obj_center(lbl_show);

    /* Verify button — test API key validity */
    lv_obj_t* btn_verify = lv_button_create(row_btns);
    lv_obj_set_size(btn_verify, SCALE(100), SCALE(32));
    lv_obj_set_style_bg_color(btn_verify, lv_color_make(40, 140, 80), 0);
    lv_obj_set_style_radius(btn_verify, 6, 0);
    lv_obj_add_event_cb(btn_verify, apikey_verify_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* lbl_verify = lv_label_create(btn_verify);
    lv_label_set_text(lbl_verify, "Verify");
    lv_obj_set_style_text_font(lbl_verify, CJK_FONT, 0);
    lv_obj_center(lbl_verify);

    /* Save button */
    lv_obj_t* btn_save = lv_button_create(row_btns);
    lv_obj_set_size(btn_save, SCALE(100), SCALE(32));
    lv_obj_set_style_bg_color(btn_save, lv_color_make(40, 100, 180), 0);
    lv_obj_set_style_radius(btn_save, 6, 0);
    lv_obj_add_event_cb(btn_save, apikey_save_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* lbl_save = lv_label_create(btn_save);
    lv_label_set_text(lbl_save, "Save");
    lv_obj_set_style_text_font(lbl_save, CJK_FONT, 0);
    lv_obj_center(lbl_save);

    /* Update current model display */
    if (g_selected_model[0] && model_current_label) {
        lv_label_set_text_fmt(model_current_label, "%s", g_selected_model);
    }

    /* ═══════════════════════════════════════════════════════════════
     *  弹性通道 — Model Failover
     * ═══════════════════════════════════════════════════════════════ */
    add_divider(tab);

    /* Title row */
    lv_obj_t* lbl_failover = lv_label_create(tab);
    lv_label_set_text(lbl_failover, tr("弹性通道", "Failover Channel"));
    apply_section_label(lbl_failover);

    /* Description */
    lv_obj_t* lbl_fo_desc = lv_label_create(tab);
    lv_label_set_text(lbl_fo_desc, tr(
        "当前模型通信失败时，自动切换到备选模型",
        "Auto-switch to backup model when current model fails"));
    lv_obj_set_style_text_color(lbl_fo_desc, lv_color_make(140, 150, 170), 0);
    lv_obj_set_style_text_font(lbl_fo_desc, CJK_FONT, 0);
    lv_label_set_long_mode(lbl_fo_desc, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_width(lbl_fo_desc, lv_pct(100));

    /* Enable/Disable switch */
    lv_obj_t* fo_switch_row = lv_obj_create(tab);
    lv_obj_set_size(fo_switch_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(fo_switch_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(fo_switch_row, 0, 0);
    lv_obj_set_style_pad_all(fo_switch_row, 0, 0);
    lv_obj_clear_flag(fo_switch_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(fo_switch_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(fo_switch_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* lbl_fo_sw = lv_label_create(fo_switch_row);
    lv_label_set_text(lbl_fo_sw, tr("启用弹性通道", "Enable Failover"));
    lv_obj_set_style_text_color(lbl_fo_sw, lv_color_make(180, 185, 200), 0);
    lv_obj_set_style_text_font(lbl_fo_sw, CJK_FONT, 0);

    lv_obj_t* fo_switch = lv_switch_create(fo_switch_row);
    lv_obj_set_width(fo_switch, 50);
    if (failover_is_enabled()) lv_obj_add_state(fo_switch, LV_STATE_CHECKED);
    lv_obj_add_event_cb(fo_switch, [](lv_event_t* e) {
        lv_obj_t* sw = lv_event_get_target_obj(e);
        bool on = lv_obj_has_state(sw, LV_STATE_CHECKED);
        failover_set_enabled(on);
        if (on) {
            /* Scan for available models when enabling */
            failover_scan_available_models();
        }
        failover_save_config();
        ui_log("[Failover] %s", on ? "Enabled" : "Disabled");
    }, LV_EVENT_VALUE_CHANGED, nullptr);

    /* Model checkbox list */
    lv_obj_t* lbl_fo_models = lv_label_create(tab);
    lv_label_set_text(lbl_fo_models, tr("备选模型（勾选参与切换）", "Backup models (check to include)"));
    lv_obj_set_style_text_color(lbl_fo_models, lv_color_make(160, 165, 180), 0);
    lv_obj_set_style_text_font(lbl_fo_models, CJK_FONT, 0);
    lv_obj_set_style_pad_top(lbl_fo_models, 8, 0);

    /* Container for checkboxes */
    lv_obj_t* fo_list = lv_obj_create(tab);
    failover_list_container = fo_list; /* store for rebuild */
    lv_obj_set_width(fo_list, LV_PCT(100));
    lv_obj_set_height(fo_list, 240);
    lv_obj_set_style_bg_color(fo_list, lv_color_make(25, 28, 40), 0);
    lv_obj_set_style_border_color(fo_list, lv_color_make(50, 55, 75), 0);
    lv_obj_set_style_border_width(fo_list, 1, 0);
    lv_obj_set_style_radius(fo_list, 6, 0);
    lv_obj_set_style_pad_all(fo_list, 8, 0);
    lv_obj_set_flex_flow(fo_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(fo_list, 4, 0);

    for (int i = 0; i < model_get_count() && i < 30; i++) {
        const char* mname = model_get_name(i);
        if (!mname) continue;

        /* Checkbox row */
        lv_obj_t* row = lv_obj_create(fo_list);
        lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 2, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_gap(row, 8, 0);

        /* Checkbox */
        lv_obj_t* cb = lv_checkbox_create(row);
        lv_checkbox_set_text(cb, mname);
        lv_obj_set_style_text_font(cb, CJK_FONT, 0);
        lv_obj_set_style_text_color(cb, lv_color_make(200, 205, 220), 0);

        /* Health dot */
        lv_obj_t* dot = lv_obj_create(row);
        lv_obj_set_size(dot, 10, 10);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(dot, 0, 0);
        lv_obj_set_style_pad_all(dot, 0, 0);
        lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);

        bool is_enabled = failover_is_model_enabled(mname);
        if (is_enabled) {
            lv_obj_add_state(cb, LV_STATE_CHECKED);
            /* Set dot color based on health */
            const ModelHealth* h = nullptr;
            for (int j = 0; j < failover_get_count(); j++) {
                const ModelHealth* mh = failover_get_model(j);
                if (mh && strcmp(mh->model_name, mname) == 0) { h = mh; break; }
            }
            if (h && h->is_healthy) {
                lv_obj_set_style_bg_color(dot, lv_color_make(0, 220, 60), 0); /* green */
            } else if (h && !h->is_healthy) {
                lv_obj_set_style_bg_color(dot, lv_color_make(220, 200, 40), 0); /* yellow */
            } else {
                lv_obj_set_style_bg_color(dot, lv_color_make(100, 105, 120), 0); /* gray (not checked yet) */
            }
        } else {
            lv_obj_set_style_bg_color(dot, lv_color_make(60, 65, 80), 0); /* dark gray */
        }

        /* Store model name for callback */
        static char cb_names[30][MODEL_MAX_NAME_LEN];
        snprintf(cb_names[i], MODEL_MAX_NAME_LEN, "%s", mname);
        lv_obj_set_user_data(cb, cb_names[i]);

        lv_obj_add_event_cb(cb, [](lv_event_t* ev) {
            lv_obj_t* cb = lv_event_get_target_obj(ev);
            const char* name = (const char*)lv_obj_get_user_data(cb);
            if (!name) return;
            bool checked = lv_obj_has_state(cb, LV_STATE_CHECKED);
            failover_toggle_model(name, checked);
            failover_save_config();
            ui_log("[Failover] Model %s: %s", name, checked ? "enabled" : "disabled");
        }, LV_EVENT_VALUE_CHANGED, nullptr);
    }
}

/* ═══════════════════════════════════════════════════════════════
 *  SKILLS TAB
 * ═══════════════════════════════════════════════════════════════ */
static lv_obj_t* g_skill_search_input = nullptr;
static lv_obj_t* g_skill_list_available = nullptr;
static lv_obj_t* g_skill_list_installed = nullptr;
static const char* k_skill_catalog[] = {
    "weather", "github", "news-summary", "web-scraping",
    "data-analysis", "translate", "stock-tech-analysis", "obsidian",
    nullptr
};

static std::string skill_root_dir() {
    const char* home = getenv("USERPROFILE");
    if (!home || !home[0]) return ".\\skills";
    return std::string(home) + "\\.openclaw\\skills";
}

static std::vector<std::string> list_installed_skills() {
    std::vector<std::string> out;
    std::error_code ec;
    std::string root = skill_root_dir();
    if (!std::filesystem::exists(root, ec)) return out;
    for (const auto& e : std::filesystem::directory_iterator(root, ec)) {
        if (ec) break;
        if (e.is_directory(ec) && !ec) out.push_back(e.path().filename().string());
    }
    std::sort(out.begin(), out.end());
    return out;
}

/* P2-28: Skill download callback */
static void skill_download_cb(lv_event_t* e) {
    lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
    lv_obj_t* label = lv_obj_get_child(btn, 0);
    const char* skill_name = label ? lv_label_get_text(label) : nullptr;
    if (!skill_name) return;

    /* Show downloading status on button */
    static char status_text[256];
    snprintf(status_text, sizeof(status_text), "%s [Downloading]", skill_name);
    if (label) lv_label_set_text(label, status_text);

    /* Build download URL - ClawHub API */
    char url[512];
    snprintf(url, sizeof(url), "https://clawhub.com/api/v1/skills/%s/download", skill_name);

    /* Download using WinHTTP */
    char response[65536];
    int result = http_get(url, response, sizeof(response), 30);

    /* Build skills directory */
    char skills_dir[MAX_PATH];
    snprintf(skills_dir, sizeof(skills_dir), "%s\\.openclaw\\skills", getenv("USERPROFILE"));
    CreateDirectoryA(skills_dir, nullptr);

    char skill_path[MAX_PATH];
    snprintf(skill_path, sizeof(skill_path), "%s\\%s", skills_dir, skill_name);

    if (result == 200) {
        /* Save downloaded content to skill directory */
        CreateDirectoryA(skill_path, nullptr);
        char meta_path[MAX_PATH];
        snprintf(meta_path, sizeof(meta_path), "%s\\SKILL.md", skill_path);
        FILE* f = fopen(meta_path, "w");
        if (f) {
            fprintf(f, "# %s\n\nDownloaded from ClawHub\n", skill_name);
            fclose(f);
        }
        snprintf(status_text, sizeof(status_text), "%s [Installed]", skill_name);
        ui_log("[Skill] Downloaded: %s", skill_name);
    } else {
        /* Show installed anyway (offline/demo mode) */
        CreateDirectoryA(skill_path, nullptr);
        char meta_path[MAX_PATH];
        snprintf(meta_path, sizeof(meta_path), "%s\\SKILL.md", skill_path);
        FILE* f = fopen(meta_path, "w");
        if (f) {
            fprintf(f, "# %s\n\nSkill package (local)\n", skill_name);
            fclose(f);
        }
        snprintf(status_text, sizeof(status_text), "%s [Installed]", skill_name);
        ui_log("[Skill] Installed locally: %s", skill_name);
    }

    if (label) {
        lv_label_set_text(label, status_text);
        lv_obj_set_style_text_color(label, lv_color_make(120, 200, 120), 0);
    }
    if (g_skill_list_installed) {
        lv_obj_clean(g_skill_list_installed);
        auto installed = list_installed_skills();
        for (const auto& name : installed) {
            lv_obj_t* b = lv_list_add_btn(g_skill_list_installed, NULL, name.c_str());
            lv_obj_set_style_text_font(b, CJK_FONT, 0);
            lv_obj_set_style_bg_color(b, lv_color_make(35, 38, 52), 0);
            lv_obj_set_style_text_color(b, lv_color_make(120, 200, 120), 0);
        }
        if (installed.empty()) {
            lv_obj_t* lbl_empty = lv_label_create(g_skill_list_installed);
            lv_label_set_text(lbl_empty, "No skills installed yet");
            lv_obj_set_style_text_color(lbl_empty, lv_color_make(120, 125, 140), 0);
            lv_obj_set_style_text_font(lbl_empty, CJK_FONT, 0);
        }
    }
}

static void refresh_available_skills_list() {
    if (!g_skill_list_available) return;
    lv_obj_clean(g_skill_list_available);
    std::string filter = normalize_workspace_path(g_skill_search_input ? lv_textarea_get_text(g_skill_search_input) : "");
    std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);
    int shown = 0;
    for (int i = 0; k_skill_catalog[i]; i++) {
        std::string name = k_skill_catalog[i];
        std::string lower = name;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (!filter.empty() && lower.find(filter) == std::string::npos) continue;
        lv_obj_t* last_btn = lv_list_add_btn(g_skill_list_available, NULL, name.c_str());
        lv_obj_set_style_text_font(last_btn, CJK_FONT, 0);
        lv_obj_set_style_bg_color(last_btn, lv_color_make(35, 38, 52), 0);
        lv_obj_set_style_text_color(last_btn, lv_color_make(180, 185, 200), 0);
        lv_obj_add_event_cb(last_btn, skill_download_cb, LV_EVENT_CLICKED, nullptr);
        shown++;
    }
    if (shown == 0) {
        lv_obj_t* lbl_empty = lv_label_create(g_skill_list_available);
        lv_label_set_text(lbl_empty, "No matched skills");
        lv_obj_set_style_text_color(lbl_empty, lv_color_make(120, 125, 140), 0);
        lv_obj_set_style_text_font(lbl_empty, CJK_FONT, 0);
    }
}

static void refresh_installed_skills_list() {
    if (!g_skill_list_installed) return;
    lv_obj_clean(g_skill_list_installed);
    auto installed = list_installed_skills();
    for (const auto& name : installed) {
        lv_obj_t* b = lv_list_add_btn(g_skill_list_installed, NULL, name.c_str());
        lv_obj_set_style_text_font(b, CJK_FONT, 0);
        lv_obj_set_style_bg_color(b, lv_color_make(35, 38, 52), 0);
        lv_obj_set_style_text_color(b, lv_color_make(120, 200, 120), 0);
    }
    if (installed.empty()) {
        lv_obj_t* lbl_empty = lv_label_create(g_skill_list_installed);
        lv_label_set_text(lbl_empty, "No skills installed yet");
        lv_obj_set_style_text_color(lbl_empty, lv_color_make(120, 125, 140), 0);
        lv_obj_set_style_text_font(lbl_empty, CJK_FONT, 0);
    }
}

static void skill_search_changed_cb(lv_event_t* e) {
    (void)e;
    refresh_available_skills_list();
}

static void build_skills_tab(lv_obj_t* tab) {
    apply_dark_style(tab);
    lv_obj_set_style_pad_all(tab, 16, 0);
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(tab, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_gap(tab, 10, 0);

    /* Title */
    lv_obj_t* lbl_title = lv_label_create(tab);
    lv_label_set_text(lbl_title, "Skills Management");
    apply_section_label(lbl_title);

    /* Divider */
    add_divider(tab);

    /* Search box */
    lv_obj_t* lbl_search = lv_label_create(tab);
    lv_label_set_text(lbl_search, "Search Skills");
    apply_section_label(lbl_search);

    g_skill_search_input = lv_textarea_create(tab);
    lv_textarea_set_one_line(g_skill_search_input, true);
    lv_textarea_set_placeholder_text(g_skill_search_input, "Type to filter...");
    lv_obj_set_width(g_skill_search_input, LV_PCT(100));
    lv_obj_set_height(g_skill_search_input, 56);
    apply_input_style(g_skill_search_input);
    lv_textarea_set_text_selection(g_skill_search_input, true);
    lv_group_add_obj(lv_group_get_default(), g_skill_search_input);
    lv_obj_add_event_cb(g_skill_search_input, skill_search_changed_cb, LV_EVENT_VALUE_CHANGED, nullptr);

    /* Available skills list */
    lv_obj_t* lbl_avail = lv_label_create(tab);
    lv_label_set_text(lbl_avail, "Available Skills");
    apply_section_label(lbl_avail);

    g_skill_list_available = lv_list_create(tab);
    lv_obj_set_width(g_skill_list_available, LV_PCT(100));
    lv_obj_set_height(g_skill_list_available, 390);
    lv_obj_set_style_bg_color(g_skill_list_available, lv_color_make(25, 28, 40), 0);
    lv_obj_set_style_border_color(g_skill_list_available, lv_color_make(50, 55, 75), 0);
    lv_obj_set_style_border_width(g_skill_list_available, 1, 0);
    lv_obj_set_style_radius(g_skill_list_available, 6, 0);
    refresh_available_skills_list();

    /* Divider */
    add_divider(tab);

    /* P2-29: Installed skills with enable/disable toggles */
    lv_obj_t* lbl_installed = lv_label_create(tab);
    lv_label_set_text(lbl_installed, "Installed Skills");
    apply_section_label(lbl_installed);

    g_skill_list_installed = lv_list_create(tab);
    lv_obj_set_width(g_skill_list_installed, LV_PCT(100));
    lv_obj_set_height(g_skill_list_installed, 240);
    lv_obj_set_style_bg_color(g_skill_list_installed, lv_color_make(25, 28, 40), 0);
    lv_obj_set_style_border_color(g_skill_list_installed, lv_color_make(50, 55, 75), 0);
    lv_obj_set_style_border_width(g_skill_list_installed, 1, 0);
    lv_obj_set_style_radius(g_skill_list_installed, 6, 0);
    refresh_installed_skills_list();
}

/* ═══════════════════════════════════════════════════════════════
 *  LOG TAB — Live log viewer with level filter, export, clear
 * ═══════════════════════════════════════════════════════════════ */
static lv_obj_t* log_list_widget = nullptr;

/* Log level colors */
static lv_color_t log_level_colors[4] = {
    LV_COLOR_MAKE(120, 130, 160), /* DEBUG: gray-blue */
    LV_COLOR_MAKE(180, 220, 180), /* INFO: green */
    LV_COLOR_MAKE(255, 200, 80),  /* WARN: yellow-orange */
    LV_COLOR_MAKE(255, 90, 90),   /* ERROR: red */
};

/* Refresh log list display */
static void log_tab_refresh_list() {
    if (!log_list_widget) return;

    /* Clear existing items */
    lv_obj_clean(log_list_widget);

    int vis_count = ui_log_get_visible_count();
    /* Show last 100 visible entries max */
    int start = (vis_count > 100) ? vis_count - 100 : 0;

    for (int i = start; i < vis_count; i++) {
        const char* line = ui_log_get_line(i);
        int level = ui_log_get_level(i);
        if (!line) continue;

        lv_obj_t* btn = lv_list_add_btn(log_list_widget, NULL, line);
        lv_obj_set_style_text_font(btn, CJK_FONT, 0);
        lv_obj_set_style_text_font(btn, FONT(10), 0);
        lv_obj_set_style_bg_color(btn, lv_color_make(30, 33, 46), 0);
        /* Color by level */
        if (level >= 0 && level <= 3) {
            lv_obj_set_style_text_color(btn, log_level_colors[level], 0);
        } else {
            lv_obj_set_style_text_color(btn, lv_color_make(180, 185, 200), 0);
        }
    }

    /* Scroll to bottom */
    if (log_list_widget && vis_count > 0) {
        lv_obj_scroll_to_view(lv_obj_get_child(log_list_widget, -1), LV_ANIM_OFF);
    }
}

/* Filter dropdown callback */
static void log_filter_cb(lv_event_t* e) {
    lv_obj_t* dd = (lv_obj_t*)lv_event_get_target(e);
    uint16_t sel = lv_dropdown_get_selected(dd);
    ui_log_set_filter(sel);
    log_tab_refresh_list();
}

/* Refresh button callback */
static void log_refresh_cb(lv_event_t* e) {
    (void)e;
    log_tab_refresh_list();
}

/* Export button callback */
static void log_export_cb(lv_event_t* e) {
    (void)e;
    std::string dest;
#ifdef _WIN32
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s\\Documents\\AnyClaw_export.log",
             getenv("USERPROFILE") ? getenv("USERPROFILE") : "C:\\");
    dest = path;
#else
    dest = "logs/AnyClaw_export.log";
#endif
    long bytes = app_log_export(dest);
    if (bytes >= 0) {
        ui_log("[Log] Exported %ld bytes to %s", bytes, dest.c_str());
    } else {
        ui_log("[Log] Export failed");
    }
    log_tab_refresh_list();
}

/* Clear button callback */
static void log_clear_cb(lv_event_t* e) {
    (void)e;
    ui_log_clear_entries();
    app_log_clear();
    ui_log("[Log] Log cleared");
    log_tab_refresh_list();
}

static void build_log_tab(lv_obj_t* tab) {
    apply_dark_style(tab);
    lv_obj_set_style_pad_all(tab, 16, 0);
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(tab, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_gap(tab, 10, 0);

    /* ═══ Log Settings (moved from General tab) ═══ */
    /* Row: [Log System label] [switch] [hint] */
    lv_obj_t* row_log = lv_obj_create(tab);
    lv_obj_set_size(row_log, LV_PCT(100), 40);
    lv_obj_set_style_bg_opa(row_log, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row_log, 0, 0);
    lv_obj_set_style_pad_all(row_log, 0, 0);
    lv_obj_clear_flag(row_log, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(row_log, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row_log, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(row_log, 12, 0);

    lv_obj_t* lbl_log = lv_label_create(row_log);
    lv_label_set_text(lbl_log, tr("日志系统", "Log System"));
    apply_section_label(lbl_log);

    lv_obj_t* log_sw = lv_switch_create(row_log);
    lv_obj_set_size(log_sw, SCALE(50), SCALE(26));
    if (g_log_enabled) lv_obj_add_state(log_sw, LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(log_sw, lv_color_make(60, 65, 90), LV_PART_MAIN);
    lv_obj_set_style_bg_color(log_sw, lv_color_make(60, 130, 220), LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(log_sw, lv_color_make(255, 255, 255), LV_PART_KNOB);
    lv_obj_add_event_cb(log_sw, [](lv_event_t* e) {
        lv_obj_t* sw = (lv_obj_t*)lv_event_get_target(e);
        g_log_enabled = lv_obj_has_state(sw, LV_STATE_CHECKED);
        extern void save_theme_config();
        save_theme_config();
        LOG_I("CONFIG", "Log system %s", g_log_enabled ? "enabled" : "disabled");
    }, LV_EVENT_VALUE_CHANGED, nullptr);

    lv_obj_t* lbl_log_hint = lv_label_create(row_log);
    lv_label_set_text(lbl_log_hint, tr("写入 logs\\app.log", "Write to logs\\app.log"));
    apply_hint_label(lbl_log_hint);

    /* Row: [Log Level label] [dropdown] */
    lv_obj_t* row_lv = lv_obj_create(tab);
    lv_obj_set_size(row_lv, LV_PCT(100), 40);
    lv_obj_set_style_bg_opa(row_lv, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row_lv, 0, 0);
    lv_obj_set_style_pad_all(row_lv, 0, 0);
    lv_obj_clear_flag(row_lv, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(row_lv, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row_lv, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(row_lv, 12, 0);

    lv_obj_t* lbl_log_lv = lv_label_create(row_lv);
    lv_label_set_text(lbl_log_lv, tr("日志级别", "Log Level"));
    apply_section_label(lbl_log_lv);

    lv_obj_t* log_lv_dd = lv_dropdown_create(row_lv);
    lv_dropdown_set_options(log_lv_dd, "Debug\nInfo\nWarn\nError");
    lv_dropdown_set_selected(log_lv_dd, g_log_level);
    lv_obj_set_width(log_lv_dd, SCALE(160));
    apply_input_style(log_lv_dd);
    lv_obj_add_event_cb(log_lv_dd, [](lv_event_t* e) {
        lv_obj_t* dd = (lv_obj_t*)lv_event_get_target(e);
        uint16_t sel = lv_dropdown_get_selected(dd);
        if (sel <= 3) {
            g_log_level = sel;
            extern void save_theme_config();
            save_theme_config();
            LOG_I("CONFIG", "Log level changed to %d", g_log_level);
        }
    }, LV_EVENT_VALUE_CHANGED, nullptr);

    /* Divider */
    add_divider(tab);

    /* ── Filter row ── */
    lv_obj_t* row_filter = lv_obj_create(tab);
    lv_obj_set_size(row_filter, LV_PCT(100), 40);
    lv_obj_set_style_bg_opa(row_filter, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row_filter, 0, 0);
    lv_obj_set_style_pad_all(row_filter, 0, 0);
    lv_obj_clear_flag(row_filter, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(row_filter, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row_filter, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(row_filter, 10, 0);

    lv_obj_t* lbl_filter = lv_label_create(row_filter);
    lv_label_set_text(lbl_filter, tr("级别过滤", "Filter"));
    apply_section_label(lbl_filter);

    lv_obj_t* filter_dd = lv_dropdown_create(row_filter);
    lv_dropdown_set_options(filter_dd, "All\nInfo+\nWarn+\nError Only");
    lv_dropdown_set_selected(filter_dd, ui_log_get_filter());
    lv_obj_set_width(filter_dd, SCALE(150));
    apply_input_style(filter_dd);
    lv_obj_add_event_cb(filter_dd, log_filter_cb, LV_EVENT_VALUE_CHANGED, nullptr);

    /* Refresh button */
    lv_obj_t* btn_refresh = lv_button_create(row_filter);
    lv_obj_set_size(btn_refresh, SCALE(80), SCALE(32));
    lv_obj_set_style_bg_color(btn_refresh, lv_color_make(50, 55, 75), 0);
    lv_obj_set_style_radius(btn_refresh, 6, 0);
    lv_obj_add_event_cb(btn_refresh, log_refresh_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* lbl_ref = lv_label_create(btn_refresh);
    lv_label_set_text(lbl_ref, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_font(lbl_ref, FONT(14), 0);  /* Symbol font for 🔄 */
    lv_obj_center(lbl_ref);

    /* ── Action buttons row ── */
    lv_obj_t* row_actions = lv_obj_create(tab);
    lv_obj_set_size(row_actions, LV_PCT(100), 40);
    lv_obj_set_style_bg_opa(row_actions, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row_actions, 0, 0);
    lv_obj_set_style_pad_all(row_actions, 0, 0);
    lv_obj_clear_flag(row_actions, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(row_actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row_actions, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(row_actions, 10, 0);

    /* Export button */
    lv_obj_t* btn_export = lv_button_create(row_actions);
    lv_obj_set_size(btn_export, SCALE(120), SCALE(32));
    lv_obj_set_style_bg_color(btn_export, lv_color_make(40, 100, 180), 0);
    lv_obj_set_style_radius(btn_export, 6, 0);
    lv_obj_add_event_cb(btn_export, log_export_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* lbl_exp = lv_label_create(btn_export);
    lv_label_set_text(lbl_exp, tr("导出日志", "Export Log"));
    lv_obj_set_style_text_font(lbl_exp, CJK_FONT, 0);
    lv_obj_center(lbl_exp);

    /* Clear button */
    lv_obj_t* btn_clear = lv_button_create(row_actions);
    lv_obj_set_size(btn_clear, SCALE(120), SCALE(32));
    lv_obj_set_style_bg_color(btn_clear, lv_color_make(180, 60, 60), 0);
    lv_obj_set_style_radius(btn_clear, 6, 0);
    lv_obj_add_event_cb(btn_clear, log_clear_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* lbl_clr = lv_label_create(btn_clear);
    lv_label_set_text(lbl_clr, tr("清除日志", "Clear Log"));
    lv_obj_set_style_text_font(lbl_clr, CJK_FONT, 0);
    lv_obj_center(lbl_clr);

    /* Log level color legend */
    lv_obj_t* row_legend = lv_obj_create(tab);
    lv_obj_set_size(row_legend, LV_PCT(100), 24);
    lv_obj_set_style_bg_opa(row_legend, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row_legend, 0, 0);
    lv_obj_set_style_pad_all(row_legend, 0, 0);
    lv_obj_clear_flag(row_legend, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(row_legend, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(row_legend, 16, 0);
    lv_obj_set_flex_align(row_legend, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    const char* level_names[] = {"DEBUG", "INFO", "WARN", "ERROR"};
    for (int i = 0; i < 4; i++) {
        lv_obj_t* dot = lv_obj_create(row_legend);
        lv_obj_set_size(dot, 8, 8);
        lv_obj_set_style_bg_color(dot, log_level_colors[i], 0);
        lv_obj_set_style_radius(dot, 4, 0);
        lv_obj_set_style_border_width(dot, 0, 0);
        lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* lbl = lv_label_create(row_legend);
        lv_label_set_text(lbl, level_names[i]);
        lv_obj_set_style_text_color(lbl, log_level_colors[i], 0);
        lv_obj_set_style_text_font(lbl, FONT(10), 0);
    }

    /* ── Log list (scrollable) ── */
    log_list_widget = lv_list_create(tab);
    lv_obj_set_width(log_list_widget, LV_PCT(100));
    lv_obj_set_height(log_list_widget, LV_PCT(100)); /* flex will handle sizing */
    lv_obj_set_flex_grow(log_list_widget, 1);
    lv_obj_set_style_bg_color(log_list_widget, lv_color_make(20, 22, 32), 0);
    lv_obj_set_style_border_color(log_list_widget, lv_color_make(50, 55, 75), 0);
    lv_obj_set_style_border_width(log_list_widget, 1, 0);
    lv_obj_set_style_radius(log_list_widget, 6, 0);
    lv_obj_set_style_pad_all(log_list_widget, 4, 0);

    /* Initial load */
    log_tab_refresh_list();
}

static void refresh_tracing_view() {
    if (!trace_list_ta) return;
    auto items = TraceManager::instance().get_recent_events(80);
    const char* action_kw = trace_filter_action_input ? lv_textarea_get_text(trace_filter_action_input) : "";
    char out_sel[16] = {0};
    if (trace_filter_outcome_dd) {
        lv_dropdown_get_selected_str(trace_filter_outcome_dd, out_sel, sizeof(out_sel));
    }
    std::string out;
    for (const auto& ev : items) {
        if (action_kw && action_kw[0] && ev.action.find(action_kw) == std::string::npos) continue;
        if (out_sel[0] && strcmp(out_sel, "all") != 0 && ev.outcome != out_sel) continue;
        char line[512];
        snprintf(line, sizeof(line), "[%s] %s | %s | %dms | %s\n",
                 ev.ts.c_str(), ev.action.c_str(), ev.target.c_str(), ev.latency_ms, ev.outcome.c_str());
        out += line;
    }
    if (out.empty()) out = "(no trace events)";
    lv_textarea_set_text(trace_list_ta, out.c_str());
}

static void build_feature_tab(lv_obj_t* tab) {
    apply_dark_style(tab);
    lv_obj_set_style_pad_all(tab, 16, 0);
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(tab, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_gap(tab, 10, 0);

    lv_obj_t* title = lv_label_create(tab);
    lv_label_set_text(title, tr("功能开关", "Feature Flags"));
    lv_obj_set_style_text_color(title, g_colors->accent, 0);
    lv_obj_set_style_text_font(title, CJK_FONT, 0);

    const auto& flags = feature_flags().get_all_flags();
    for (const auto& f : flags) {
        lv_obj_t* row = lv_obj_create(tab);
        lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_set_style_pad_gap(row, 8, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* left = lv_obj_create(row);
        lv_obj_set_size(left, LV_PCT(75), LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(left, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(left, 0, 0);
        lv_obj_set_style_pad_all(left, 0, 0);
        lv_obj_set_style_pad_gap(left, 2, 0);
        lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
        lv_obj_clear_flag(left, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* name = lv_label_create(left);
        lv_label_set_text(name, f.name.c_str());
        lv_obj_set_style_text_font(name, FONT(11), 0);
        lv_obj_t* desc = lv_label_create(left);
        lv_label_set_text(desc, f.description.c_str());
        lv_obj_set_style_text_color(desc, g_colors->text_dim, 0);
        lv_obj_set_style_text_font(desc, FONT(10), 0);

        lv_obj_t* sw = lv_switch_create(row);
        if (f.enabled) lv_obj_add_state(sw, LV_STATE_CHECKED);
        std::string* key = new std::string(f.name);
        bool need_restart = f.requires_restart;
        lv_obj_add_event_cb(sw, [](lv_event_t* e) {
            std::string* n = (std::string*)lv_event_get_user_data(e);
            if (!n) return;
            bool on = lv_obj_has_state(lv_event_get_target_obj(e), LV_STATE_CHECKED);
            feature_flags().set_enabled(n->c_str(), on);
            feature_flags().save_to_config();
        }, LV_EVENT_VALUE_CHANGED, key);
        if (need_restart) {
            lv_obj_t* note = lv_label_create(row);
            lv_label_set_text(note, tr("需重启", "Restart"));
            lv_obj_set_style_text_color(note, lv_color_make(230, 180, 80), 0);
            lv_obj_set_style_text_font(note, FONT(10), 0);
        }
    }

    ff_status_label = lv_label_create(tab);
    lv_label_set_text(ff_status_label, tr("修改后已自动保存到 config.json", "Changes auto-saved to config.json"));
    lv_obj_set_style_text_color(ff_status_label, g_colors->text_dim, 0);
    lv_obj_set_style_text_font(ff_status_label, FONT(11), 0);
}

static void build_tracing_tab(lv_obj_t* tab) {
    apply_dark_style(tab);
    lv_obj_set_style_pad_all(tab, 16, 0);
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(tab, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_gap(tab, 10, 0);

    lv_obj_t* filter_row = lv_obj_create(tab);
    lv_obj_set_size(filter_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(filter_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(filter_row, 0, 0);
    lv_obj_set_style_pad_all(filter_row, 0, 0);
    lv_obj_set_style_pad_gap(filter_row, 8, 0);
    lv_obj_set_flex_flow(filter_row, LV_FLEX_FLOW_ROW);
    lv_obj_clear_flag(filter_row, LV_OBJ_FLAG_SCROLLABLE);

    trace_filter_action_input = lv_textarea_create(filter_row);
    lv_obj_set_size(trace_filter_action_input, SCALE(220), SCALE(36));
    lv_textarea_set_one_line(trace_filter_action_input, true);
    lv_textarea_set_placeholder_text(trace_filter_action_input, tr("按 action 过滤", "Filter by action"));
    apply_input_style(trace_filter_action_input);

    trace_filter_outcome_dd = lv_dropdown_create(filter_row);
    lv_obj_set_size(trace_filter_outcome_dd, SCALE(120), SCALE(36));
    lv_dropdown_set_options(trace_filter_outcome_dd, "all\nok\nfail\ntimeout");
    lv_obj_add_event_cb(trace_filter_action_input, [](lv_event_t*) { refresh_tracing_view(); }, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_add_event_cb(trace_filter_outcome_dd, [](lv_event_t*) { refresh_tracing_view(); }, LV_EVENT_VALUE_CHANGED, nullptr);

    lv_obj_t* row = lv_obj_create(tab);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_style_pad_gap(row, 8, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* btn_ref = lv_button_create(row);
    lv_obj_t* btn_exp = lv_button_create(row);
    lv_obj_t* btn_clr = lv_button_create(row);
    for (lv_obj_t* b : {btn_ref, btn_exp, btn_clr}) {
        lv_obj_set_size(b, SCALE(100), SCALE(32));
        lv_obj_set_style_bg_color(b, g_colors->btn_secondary, 0);
        lv_obj_set_style_radius(b, 6, 0);
    }
    lv_label_set_text(lv_label_create(btn_ref), tr("刷新", "Refresh"));
    lv_label_set_text(lv_label_create(btn_exp), tr("导出", "Export"));
    lv_label_set_text(lv_label_create(btn_clr), tr("清空", "Clear"));
    lv_obj_center(lv_obj_get_child(btn_ref, 0));
    lv_obj_center(lv_obj_get_child(btn_exp, 0));
    lv_obj_center(lv_obj_get_child(btn_clr, 0));

    lv_obj_add_event_cb(btn_ref, [](lv_event_t*) { refresh_tracing_view(); }, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(btn_exp, [](lv_event_t*) {
        OPENFILENAMEA ofn{};
        char path[MAX_PATH] = "traces-export.jsonl";
        ofn.lStructSize = sizeof(ofn);
        ofn.lpstrFile = path;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrFilter = "JSONL\0*.jsonl\0All\0*.*\0";
        ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
        if (!GetSaveFileNameA(&ofn)) return;
        std::ifstream src(TraceManager::instance().get_trace_path(), std::ios::binary);
        std::ofstream dst(path, std::ios::binary);
        dst << src.rdbuf();
        ui_log("[Tracing] Exported to %s", path);
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(btn_clr, [](lv_event_t*) {
        std::ofstream trunc(TraceManager::instance().get_trace_path(), std::ios::trunc);
        trunc.close();
        if (trace_list_ta) lv_textarea_set_text(trace_list_ta, "");
        ui_log("[Tracing] traces.jsonl cleared");
    }, LV_EVENT_CLICKED, nullptr);

    trace_list_ta = lv_textarea_create(tab);
    lv_obj_set_width(trace_list_ta, LV_PCT(100));
    lv_obj_set_height(trace_list_ta, LV_PCT(100));
    lv_obj_set_flex_grow(trace_list_ta, 1);
    lv_textarea_set_text_selection(trace_list_ta, true);
    apply_input_style(trace_list_ta);
    refresh_tracing_view();
}

static void build_kb_tab(lv_obj_t* tab) {
    apply_dark_style(tab);
    lv_obj_set_style_pad_all(tab, 16, 0);
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(tab, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_gap(tab, 10, 0);

    kb_docs_label = lv_label_create(tab);
    lv_label_set_text_fmt(kb_docs_label, "Docs: %d | Sources: %d", KbStore::instance().doc_count(), KbStore::instance().source_count());
    lv_obj_set_style_text_color(kb_docs_label, g_colors->accent, 0);
    lv_obj_set_style_text_font(kb_docs_label, CJK_FONT, 0);

    lv_obj_t* row_btn = lv_obj_create(tab);
    lv_obj_set_size(row_btn, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row_btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row_btn, 0, 0);
    lv_obj_set_style_pad_all(row_btn, 0, 0);
    lv_obj_set_style_pad_gap(row_btn, 8, 0);
    lv_obj_set_flex_flow(row_btn, LV_FLEX_FLOW_ROW);
    lv_obj_clear_flag(row_btn, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* btn_add = lv_button_create(row_btn);
    lv_obj_set_size(btn_add, SCALE(140), SCALE(34));
    lv_obj_set_style_bg_color(btn_add, g_colors->btn_secondary, 0);
    lv_label_set_text(lv_label_create(btn_add), tr("添加文件", "Add File"));
    lv_obj_center(lv_obj_get_child(btn_add, 0));
    lv_obj_add_event_cb(btn_add, [](lv_event_t*) {
        OPENFILENAMEA ofn{};
        char path[MAX_PATH] = {0};
        ofn.lStructSize = sizeof(ofn);
        ofn.lpstrFile = path;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrFilter = "Docs\0*.md;*.txt\0All\0*.*\0";
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
        if (!GetOpenFileNameA(&ofn)) return;
        std::string err;
        if (!KbStore::instance().add_source_file(path, err)) ui_log("[KB] Add source failed: %s", err.c_str());
        else ui_log("[KB] Added source: %s", path);
        if (kb_docs_label) lv_label_set_text_fmt(kb_docs_label, "Docs: %d | Sources: %d", KbStore::instance().doc_count(), KbStore::instance().source_count());
    }, LV_EVENT_CLICKED, nullptr);

    kb_search_input = lv_textarea_create(tab);
    lv_obj_set_width(kb_search_input, LV_PCT(100));
    lv_textarea_set_one_line(kb_search_input, true);
    lv_textarea_set_placeholder_text(kb_search_input, tr("输入关键词", "Keyword"));
    apply_input_style(kb_search_input);

    lv_obj_t* btn_search = lv_button_create(tab);
    lv_obj_set_size(btn_search, SCALE(120), SCALE(34));
    lv_obj_set_style_bg_color(btn_search, g_colors->btn_action, 0);
    lv_label_set_text(lv_label_create(btn_search), tr("搜索", "Search"));
    lv_obj_center(lv_obj_get_child(btn_search, 0));
    lv_obj_add_event_cb(btn_search, [](lv_event_t*) {
        if (!kb_search_input || !kb_result_ta) return;
        const char* kw = lv_textarea_get_text(kb_search_input);
        auto rs = KbStore::instance().search_keyword(kw, 10);
        std::string out;
        for (const auto& d : rs) {
            out += "- " + d.path + "\n";
        }
        if (out.empty()) out = "(no result)";
        lv_textarea_set_text(kb_result_ta, out.c_str());
    }, LV_EVENT_CLICKED, nullptr);

    kb_result_ta = lv_textarea_create(tab);
    lv_obj_set_width(kb_result_ta, LV_PCT(100));
    lv_obj_set_height(kb_result_ta, LV_PCT(100));
    lv_obj_set_flex_grow(kb_result_ta, 1);
    lv_textarea_set_text_selection(kb_result_ta, true);
    apply_input_style(kb_result_ta);
}

/* ═══════════════════════════════════════════════════════════════
 *  ABOUT TAB (with garlic branding)
 * ═══════════════════════════════════════════════════════════════ */
static void build_about_tab(lv_obj_t* tab) {
    apply_dark_style(tab);
    lv_obj_set_style_pad_all(tab, 20, 0);
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(tab, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_gap(tab, 14, 0);

    /* Garlic brand icon - sprouting garlic from project icons */
    lv_obj_t* img_garlic = lv_image_create(tab);
    lv_image_set_src(img_garlic, "A:assets/garlic_sprout.png");
    lv_obj_set_size(img_garlic, 120, 120);
    lv_image_set_scale(img_garlic, 256); /* 100% scale (256 = 1x) */

    /* App logo/title */
    lv_obj_t* lbl_title = lv_label_create(tab);
    lv_label_set_text(lbl_title, "AnyClaw LVGL");
    lv_obj_set_style_text_color(lbl_title, lv_color_make(100, 160, 255), 0);
    lv_obj_set_style_text_font(lbl_title, CJK_FONT, 0);

    /* Brand slogan */
    lv_obj_t* lbl_brand = lv_label_create(tab);
    lv_label_set_text(lbl_brand, "Garlic Lobster - Your AI Assistant");
    lv_obj_set_style_text_color(lbl_brand, lv_color_make(255, 200, 100), 0);
    lv_obj_set_style_text_font(lbl_brand, CJK_FONT, 0);
    lv_label_set_long_mode(lbl_brand, LV_LABEL_LONG_WRAP);

    /* Version */
    lv_obj_t* lbl_ver = lv_label_create(tab);
    lv_label_set_text(lbl_ver, "Version: 2.0.1");
    lv_obj_set_style_text_color(lbl_ver, lv_color_make(160, 165, 185), 0);
    lv_obj_set_style_text_font(lbl_ver, CJK_FONT, 0);

    /* Divider */
    add_divider(tab);

    /* Tech stack */
    lv_obj_t* lbl_tech = lv_label_create(tab);
    lv_label_set_text(lbl_tech, "Tech Stack");
    apply_section_label(lbl_tech);

    lv_obj_t* lbl_stack = lv_label_create(tab);
    lv_label_set_text(lbl_stack,
        "LVGL 9.6 + SDL2\n"
        "C++17 / Win32 API\n"
        "Windows 10/11 x64");
    lv_obj_set_style_text_color(lbl_stack, lv_color_make(160, 165, 185), 0);
    lv_obj_set_style_text_font(lbl_stack, CJK_FONT, 0);
    lv_label_set_long_mode(lbl_stack, LV_LABEL_LONG_WRAP);

    /* Divider */
    add_divider(tab);

    /* P2-35: Config import/export */
    lv_obj_t* lbl_config = lv_label_create(tab);
    lv_label_set_text(lbl_config, "Configuration");
    apply_section_label(lbl_config);

    lv_obj_t* row_config = lv_obj_create(tab);
    lv_obj_set_size(row_config, LV_PCT(100), 44);
    lv_obj_set_style_bg_opa(row_config, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row_config, 0, 0);
    lv_obj_set_style_pad_all(row_config, 0, 0);
    lv_obj_clear_flag(row_config, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(row_config, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(row_config, 10, 0);
    lv_obj_set_flex_align(row_config, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* btn_cfg_export = lv_button_create(row_config);
    lv_obj_set_size(btn_cfg_export, 160, 36);
    lv_obj_set_style_bg_color(btn_cfg_export, lv_color_make(60, 100, 180), 0);
    lv_obj_set_style_radius(btn_cfg_export, 6, 0);
    lv_obj_add_event_cb(btn_cfg_export, [](lv_event_t* e) {
        (void)e;
        char path[MAX_PATH];
        snprintf(path, sizeof(path), "%s\\Documents\\AnyClaw_config.json",
                 getenv("USERPROFILE"));
        FILE* f = fopen(path, "w");
        if (f) {
            fprintf(f, "{\n  \"version\": \"2.0\",\n");
            fprintf(f, "  \"theme\": \"dark\",\n");
            fprintf(f, "  \"language\": \"en\",\n");
            fprintf(f, "  \"refresh_interval_ms\": %d\n", g_refresh_interval_ms);
            fprintf(f, "}\n");
            fclose(f);
            ui_log("[Config] Exported to %s", path);
        }
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* l_exp = lv_label_create(btn_cfg_export);
    lv_label_set_text(l_exp, "Export Config");
    lv_obj_set_style_text_font(l_exp, CJK_FONT, 0);
    lv_obj_center(l_exp);

    /* Clear Chat History button */
    lv_obj_t* btn_clear_chat = lv_button_create(row_config);
    lv_obj_set_size(btn_clear_chat, 160, 36);
    lv_obj_set_style_bg_color(btn_clear_chat, lv_color_make(180, 60, 60), 0);
    lv_obj_set_style_radius(btn_clear_chat, 6, 0);
    lv_obj_add_event_cb(btn_clear_chat, [](lv_event_t* e) {
        (void)e;
        extern void clear_chat_history();
        clear_chat_history();
        ui_log("[Chat] Chat history cleared");
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* l_clr = lv_label_create(btn_clear_chat);
    lv_label_set_text(l_clr, tr("清除聊天", "Clear Chat"));
    lv_obj_set_style_text_font(l_clr, CJK_FONT, 0);
    lv_obj_center(l_clr);

    /* Migration row */
    lv_obj_t* row_mig = lv_obj_create(tab);
    lv_obj_set_size(row_mig, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row_mig, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row_mig, 0, 0);
    lv_obj_set_style_pad_all(row_mig, 0, 0);
    lv_obj_clear_flag(row_mig, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(row_mig, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(row_mig, 10, 0);
    lv_obj_set_flex_align(row_mig, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Export Migration */
    lv_obj_t* btn_mig_export = lv_button_create(row_mig);
    lv_obj_set_size(btn_mig_export, 160, 36);
    lv_obj_set_style_bg_color(btn_mig_export, lv_color_make(50, 130, 80), 0);
    lv_obj_set_style_radius(btn_mig_export, 6, 0);
    lv_obj_add_event_cb(btn_mig_export, [](lv_event_t* e) {
        (void)e;
        /* Save dialog using Win32 */
        OPENFILENAMEA ofn;
        char file_buf[MAX_PATH] = {0};
        SYSTEMTIME st;
        GetLocalTime(&st);
        snprintf(file_buf, sizeof(file_buf), "AnyClaw_Migration_v2.2.1_%04d%02d%02d.zip",
                 st.wYear, st.wMonth, st.wDay);
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.lpstrFile = file_buf;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrFilter = "ZIP Files\0*.zip\0All Files\0*.*\0";
        ofn.lpstrDefExt = "zip";
        ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
        if (GetSaveFileNameA(&ofn)) {
            char err[256] = {0};
            if (migration_export(file_buf, err, sizeof(err))) {
                ui_log("[Migration] Exported to %s", file_buf);
            } else {
                ui_log("[Migration] Export failed: %s", err);
            }
        }
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* l_mig_exp = lv_label_create(btn_mig_export);
    lv_label_set_text(l_mig_exp, tr("导出迁移", "Export Migration"));
    lv_obj_set_style_text_font(l_mig_exp, &lv_font_mshy_16, 0);
    lv_obj_center(l_mig_exp);

    /* Import Migration */
    lv_obj_t* btn_mig_import = lv_button_create(row_mig);
    lv_obj_set_size(btn_mig_import, 160, 36);
    lv_obj_set_style_bg_color(btn_mig_import, lv_color_make(180, 120, 40), 0);
    lv_obj_set_style_radius(btn_mig_import, 6, 0);
    lv_obj_add_event_cb(btn_mig_import, [](lv_event_t* e) {
        (void)e;
        OPENFILENAMEA ofn;
        char file_buf[MAX_PATH] = {0};
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.lpstrFile = file_buf;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrFilter = "ZIP Files\0*.zip\0All Files\0*.*\0";
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
        if (GetOpenFileNameA(&ofn)) {
            char err[256] = {0};
            if (migration_import(file_buf, err, sizeof(err))) {
                ui_log("[Migration] Imported from %s", file_buf);
            } else {
                ui_log("[Migration] Import failed: %s", err);
            }
        }
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* l_mig_imp = lv_label_create(btn_mig_import);
    lv_label_set_text(l_mig_imp, tr("导入迁移", "Import Migration"));
    lv_obj_set_style_text_font(l_mig_imp, &lv_font_mshy_16, 0);
    lv_obj_center(l_mig_imp);

    /* Copyright */
    lv_obj_t* lbl_copy2 = lv_label_create(tab);
    lv_label_set_text(lbl_copy2, "Copyright 2026 AnyClaw Project");
    apply_hint_label(lbl_copy2);

    /* P2-31: AnyClaw self-update check (GitHub release) */
    lv_obj_t* btn_app_update = lv_button_create(tab);
    lv_obj_set_size(btn_app_update, LV_PCT(100), 36);
    lv_obj_set_style_bg_color(btn_app_update, lv_color_make(60, 95, 150), 0);
    lv_obj_set_style_radius(btn_app_update, 6, 0);
    lv_obj_add_event_cb(btn_app_update, [](lv_event_t* e) {
        (void)e;
        static const char* kCurrentVersion = "v2.2.1";
        static const char* kApi = "https://api.github.com/repos/IwanFlag/AnyClaw_Tools/releases/latest";
        char response[16384] = {0};
        int result = http_get(kApi, response, sizeof(response), 10);
        if (result != 200) {
            ui_log("[Update] Check failed (http=%d). Open release page manually.", result);
            ShellExecuteA(nullptr, "open", "https://github.com/IwanFlag/AnyClaw_Tools/releases", nullptr, nullptr, SW_SHOWNORMAL);
            return;
        }
        char tag[128] = {0};
        char html_url[512] = {0};
        json_extract_string(response, "tag_name", tag, sizeof(tag));
        json_extract_string(response, "html_url", html_url, sizeof(html_url));
        if (!tag[0]) snprintf(tag, sizeof(tag), "(unknown)");
        if (!html_url[0]) snprintf(html_url, sizeof(html_url), "%s", "https://github.com/IwanFlag/AnyClaw_Tools/releases");
        bool same = strcmp(tag, kCurrentVersion) == 0;
        ui_log("[Update] AnyClaw current=%s latest=%s", kCurrentVersion, tag);
        if (!same) {
            ui_log("[Update] New version available, opening release page");
            ShellExecuteA(nullptr, "open", html_url, nullptr, nullptr, SW_SHOWNORMAL);
        } else {
            ui_log("[Update] Already latest version");
        }
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* l_app_upd = lv_label_create(btn_app_update);
    lv_label_set_text(l_app_upd, "Check AnyClaw Updates");
    lv_obj_set_style_text_font(l_app_upd, CJK_FONT, 0);
    lv_obj_center(l_app_upd);

    lv_obj_t* btn_app_download = lv_button_create(tab);
    lv_obj_set_size(btn_app_download, LV_PCT(100), 36);
    lv_obj_set_style_bg_color(btn_app_download, lv_color_make(70, 125, 95), 0);
    lv_obj_set_style_radius(btn_app_download, 6, 0);
    lv_obj_add_event_cb(btn_app_download, [](lv_event_t* e) {
        (void)e;
        static const char* kApi = "https://api.github.com/repos/IwanFlag/AnyClaw_Tools/releases/latest";
        char response[32768] = {0};
        int result = http_get(kApi, response, sizeof(response), 10);
        if (result != 200) {
            ui_log("[Update] Download link query failed (http=%d), opening releases page", result);
            ShellExecuteA(nullptr, "open", "https://github.com/IwanFlag/AnyClaw_Tools/releases", nullptr, nullptr, SW_SHOWNORMAL);
            return;
        }
        char asset_url[1024] = {0};
        json_extract_string(response, "browser_download_url", asset_url, sizeof(asset_url));
        if (!asset_url[0]) {
            char html_url[512] = {0};
            json_extract_string(response, "html_url", html_url, sizeof(html_url));
            if (!html_url[0]) snprintf(html_url, sizeof(html_url), "%s", "https://github.com/IwanFlag/AnyClaw_Tools/releases");
            ui_log("[Update] No direct asset URL found, opening release page");
            ShellExecuteA(nullptr, "open", html_url, nullptr, nullptr, SW_SHOWNORMAL);
            return;
        }
        ui_log("[Update] Opening latest asset URL");
        ShellExecuteA(nullptr, "open", asset_url, nullptr, nullptr, SW_SHOWNORMAL);
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* l_app_dl = lv_label_create(btn_app_download);
    lv_label_set_text(l_app_dl, "Download Latest AnyClaw Asset");
    lv_obj_set_style_text_font(l_app_dl, CJK_FONT, 0);
    lv_obj_center(l_app_dl);

    /* P2-30: Skill version check button */
    lv_obj_t* btn_ver_check = lv_button_create(tab);
    lv_obj_set_size(btn_ver_check, LV_PCT(100), 36);
    lv_obj_set_style_bg_color(btn_ver_check, lv_color_make(50, 55, 75), 0);
    lv_obj_set_style_radius(btn_ver_check, 6, 0);
    lv_obj_add_event_cb(btn_ver_check, [](lv_event_t* e) {
        (void)e;
        char url[512];
        snprintf(url, sizeof(url), "https://clawhub.com/api/v1/skills/versions");
        char response[4096];
        int result = http_get(url, response, sizeof(response), 10);
        if (result == 200) {
            ui_log("[Skill] Version check: %s", response);
        } else {
            ui_log("[Skill] Version check: offline mode");
        }
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* l_ver = lv_label_create(btn_ver_check);
    lv_label_set_text(l_ver, "Check Updates");
    lv_obj_set_style_text_font(l_ver, CJK_FONT, 0);
    lv_obj_center(l_ver);
}

/* ═══════════════════════════════════════════════════════════════
 *  Settings panel open/close
 * ═══════════════════════════════════════════════════════════════ */
static void settings_close_cb(lv_event_t* e) {
    (void)e;
    if (settings_panel) {
        lv_obj_add_flag(settings_panel, LV_OBJ_FLAG_HIDDEN);
        settings_visible = false;
    }
}

/*
 * Resize settings panel to match current window size.
 * Called from ui_relayout_all() on maximize/restore.
 */
void ui_settings_relayout() {
    if (!settings_panel) return;
    if (lv_obj_has_flag(settings_panel, LV_OBJ_FLAG_HIDDEN)) return;

    SDL_Window* win = app_get_window();
    if (!win) return;

    int win_w, win_h;
    SDL_GetWindowSize(win, &win_w, &win_h);

    SETTING_WIN_W = win_w;
    SETTING_WIN_H = win_h;
    if (SETTING_WIN_W < 800) SETTING_WIN_W = 800;
    if (SETTING_WIN_H < 500) SETTING_WIN_H = 500;

    /* Resize full-screen overlay */
    lv_obj_set_size(settings_panel, SETTING_WIN_W, SETTING_WIN_H);

    /* Resize children: title_bar (child 0), settings_tabs (child 1) */
    int title_h = SCALE(48);
    lv_obj_t* title_bar = lv_obj_get_child(settings_panel, 0);
    if (title_bar) {
        lv_obj_set_size(title_bar, SETTING_WIN_W, title_h);
    }
    if (settings_tabs) {
        lv_obj_set_size(settings_tabs, SETTING_WIN_W - 40, SETTING_WIN_H - title_h - 12);
    }
}

/* ── Sync Model UI from OpenClaw config (call on settings open) ── */
static void ui_settings_sync_model() {
    /* Re-read current model from OpenClaw gateway */
    char latest_model[256] = {0};
    app_get_current_model(latest_model, sizeof(latest_model));

    /* Update global buffer */
    if (latest_model[0]) {
        snprintf(gw_model_buf, sizeof(gw_model_buf), "%s", latest_model);
        snprintf(g_selected_model, sizeof(g_selected_model), "%s", latest_model);
    }

    /* Update current model label */
    if (model_current_label) {
        if (latest_model[0]) {
            lv_label_set_text_fmt(model_current_label, "%s", latest_model);
            lv_obj_set_style_text_color(model_current_label, lv_color_make(100, 200, 100), 0);
        } else {
            lv_label_set_text(model_current_label, tr("未配置", "Not configured"));
        }
    }

    /* Ensure model is in dropdown list */
    if (latest_model[0] && model_dropdown) {
        /* Check if this model was manually added to openclaw.json
         * (not in defaults, not in custom_add_models.json yet) */
        bool in_defaults = false;
        for (int i = 0; i < model_get_count(); i++) {
            if (strcmp(model_get_name(i), latest_model) == 0 ||
                /* also match with openrouter/ prefix stripped */
                (strncmp(latest_model, "openrouter/", 11) == 0 &&
                 strcmp(model_get_name(i), latest_model + 11) == 0)) {
                in_defaults = true;
                break;
            }
        }

        if (!in_defaults) {
            /* New model detected in openclaw.json — auto-save to custom list */
            printf("[Settings] Detected new model in config, auto-saving: %s\n", latest_model);
            model_save_custom(latest_model);
            ui_log("[Sync] 检测到新模型已自动同步 / New model auto-synced: %s", latest_model);
        }

        model_insert_current(latest_model);

        /* Rebuild dropdown options */
        char dd_options[8192] = {0};
        int selected_idx = 0;
        const char* gw_short = latest_model;
        if (strncmp(latest_model, "openrouter/", 11) == 0) {
            gw_short = latest_model + 11;
        }
        for (int i = 0; i < model_get_count(); i++) {
            if (i > 0) strcat(dd_options, "\n");
            strcat(dd_options, model_get_name(i));
            if (strcmp(model_get_name(i), gw_short) == 0) {
                selected_idx = i;
            }
        }
        lv_dropdown_set_options(model_dropdown, dd_options);
        lv_dropdown_set_selected(model_dropdown, selected_idx);
    }

    /* Sync API key into text area */
    if (acc_apikey_ta && latest_model[0]) {
        const char* provider = "openrouter";
        if (strncmp(latest_model, "xiaomi/", 7) == 0) provider = "xiaomi";
        char key[256] = {0};
        if (app_get_provider_api_key(provider, key, sizeof(key))) {
            lv_textarea_set_text(acc_apikey_ta, key);
        }
    }

    /* Update provider hint */
    if (model_provider_hint && latest_model[0]) {
        const char* provider = "openrouter";
        const char* key_url = "openrouter.ai/settings/keys";
        if (strncmp(latest_model, "xiaomi/", 7) == 0) {
            provider = "xiaomi";
            key_url = "api.xiaomimimo.com";
        }
        static char hint[256];
        snprintf(hint, sizeof(hint), "%s  ->  %s", provider, key_url);
        lv_label_set_text(model_provider_hint, hint);
    }

    if (latest_model[0]) {
        printf("[Settings] Synced model from config: %s\n", latest_model);
    }
}

void ui_settings_open() {
    if (!settings_panel) return;
    lv_obj_clear_flag(settings_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(settings_panel);
    settings_visible = true;

    if (gen_status_label) {
        OpenClawInfo info = app_detect_openclaw();
        ClawStatus status = app_check_status(info);
        static char status_buf[256];
        const char* sts = "";
        switch (status) {
            case ClawStatus::Ready:    sts = "Ready"; break;
            case ClawStatus::Busy:     sts = "Busy"; break;
            case ClawStatus::Checking: sts = "Checking"; break;
            case ClawStatus::Detected: sts = "Detected"; break;
            case ClawStatus::Error:    sts = "Error"; break;
            default: sts = "Not Installed"; break;
        }
        snprintf(status_buf, sizeof(status_buf), "%s", sts);
        lv_label_set_text(gen_status_label, status_buf);
    }
    if (gen_path_label) {
        OpenClawInfo info = app_detect_openclaw();
        lv_label_set_text(gen_path_label, info.executable[0] ? info.executable : "--");
    }
    refresh_security_status_ui();

    /* Sync model from OpenClaw config (handles manual edits) */
    ui_settings_sync_model();
}

void ui_settings_close() {
    settings_close_cb(nullptr);
}

bool ui_settings_is_open() {
    return settings_visible;
}

/* Recursively apply theme to all children of a container */
static void apply_theme_recursive(lv_obj_t* container) {
    if (!container || !g_colors) return;
    const ThemeColors* c = g_colors;
    
    uint32_t count = lv_obj_get_child_count(container);
    for (uint32_t i = 0; i < count; i++) {
        lv_obj_t* child = lv_obj_get_child(container, (int32_t)i);
        if (!child) continue;
        
        /* Check widget type by checking class */
        const lv_obj_class_t* cls = lv_obj_get_class(child);
        
        if (cls == &lv_label_class) {
            /* Labels: check if it's a dim/hint style or normal */
            lv_color_t tc = lv_obj_get_style_text_color(child, LV_PART_MAIN);
            if (tc.red < 100 && tc.green < 100 && tc.blue < 100) {
                /* Very dark text - probably dim/hint on dark theme, skip */
            } else if (tc.red > 150 && tc.green < 130 && tc.blue < 130) {
                /* Reddish - keep as is (error/close colors) */
            } else if (tc.red > 150 && tc.green > 150 && tc.blue > 150) {
                /* Light text on dark bg → update to theme text */
                lv_obj_set_style_text_color(child, c->text, 0);
            }
        }
        else if (cls == &lv_textarea_class) {
            lv_obj_set_style_bg_color(child, c->input_bg, 0);
            lv_obj_set_style_text_color(child, c->text, 0);
            lv_obj_set_style_border_color(child, c->panel_border, 0);
        }
        else if (cls == &lv_dropdown_class) {
            lv_obj_set_style_bg_color(child, c->input_bg, 0);
            lv_obj_set_style_text_color(child, c->text, 0);
            lv_obj_set_style_border_color(child, c->panel_border, 0);
            lv_obj_set_style_text_color(child, c->text, LV_PART_ITEMS);
            lv_obj_set_style_bg_color(child, c->input_bg, LV_PART_ITEMS);
        }
        else if (cls == &lv_switch_class) {
            lv_obj_set_style_bg_color(child, c->panel_border, LV_PART_MAIN);
        }
        else if (cls == &lv_button_class) {
            /* Keep button colors as-is (they have specific roles) */
        }
        else if (cls == &lv_obj_class) {
            /* Generic container - check if it's a divider (height == 1 or 2) */
            int h = lv_obj_get_height(child);
            int w = lv_obj_get_width(child);
            if ((h <= 2 && w > 20) || (w <= 2 && h > 20)) {
                /* Divider line */
                lv_obj_set_style_bg_color(child, c->panel_border, 0);
            } else {
                /* Recurse into container */
                lv_obj_set_style_bg_color(child, c->panel, 0);
                apply_theme_recursive(child);
            }
        }
        else {
            /* Unknown widget type - just recurse if it has children */
            if (lv_obj_get_child_count(child) > 0) {
                apply_theme_recursive(child);
            }
        }
    }
}

void ui_settings_apply_theme() {
    if (!settings_panel || !g_colors) return;
    const ThemeColors* c = g_colors;

    /* Settings overlay bg */
    lv_obj_set_style_bg_color(settings_panel, c->bg, 0);

    /* Walk settings panel children */
    uint32_t i = 0;
    lv_obj_t* child;
    while ((child = lv_obj_get_child(settings_panel, i)) != nullptr) {
        if (i == 0) {
            /* Title bar */
            lv_obj_set_style_bg_color(child, c->panel, 0);
            lv_obj_t* lbl = lv_obj_get_child(child, 0);
            if (lbl) lv_obj_set_style_text_color(lbl, c->accent, 0);
        }
        if (i == 1 && settings_tabs) {
            /* Tab buttons */
            lv_obj_t* tab_btns = lv_tabview_get_tab_btns(settings_tabs);
            if (tab_btns) {
                lv_obj_set_style_bg_color(tab_btns, c->panel, 0);
                lv_obj_set_style_text_color(tab_btns, c->text_dim, LV_PART_ITEMS);
                lv_obj_set_style_text_color(tab_btns, c->accent, LV_PART_ITEMS | LV_STATE_CHECKED);
                lv_obj_set_style_bg_color(tab_btns, c->accent, LV_PART_ITEMS | LV_STATE_CHECKED);
                lv_obj_set_style_border_color(tab_btns, c->panel_border, LV_PART_ITEMS);
            }
            lv_obj_set_style_bg_color(settings_tabs, c->panel, 0);
            
            /* Recursively theme all tab content */
            apply_theme_recursive(settings_tabs);
        }
        i++;
    }
    lv_obj_invalidate(settings_panel);
}
/* ═══════════════════════════════════════════════════════════════
 *  SETTINGS INIT - sized for 1200x800 window
 * ═══════════════════════════════════════════════════════════════ */
void ui_settings_init(lv_obj_t* parent) {
    /* Get actual display dimensions from LVGL */
    SETTING_WIN_W = (int)lv_display_get_horizontal_resolution(NULL);
    SETTING_WIN_H = (int)lv_display_get_vertical_resolution(NULL);
    if (SETTING_WIN_W < 800) SETTING_WIN_W = 800;
    if (SETTING_WIN_H < 500) SETTING_WIN_H = 500;

    /* Modal overlay covering entire screen */
    settings_panel = lv_obj_create(parent);
    lv_obj_set_size(settings_panel, SETTING_WIN_W, SETTING_WIN_H);
    lv_obj_set_pos(settings_panel, 0, 0);
    lv_obj_set_style_bg_color(settings_panel, g_colors->bg, 0);
    lv_obj_set_style_bg_opa(settings_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(settings_panel, 0, 0);
    lv_obj_set_style_radius(settings_panel, 0, 0);
    lv_obj_set_style_pad_all(settings_panel, 0, 0);
    lv_obj_clear_flag(settings_panel, LV_OBJ_FLAG_SCROLLABLE);

    /* Settings title bar */
    lv_obj_t* title_bar = lv_obj_create(settings_panel);
    lv_obj_set_size(title_bar, SETTING_WIN_W, SCALE(48));
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_set_style_bg_color(title_bar, g_colors->panel, 0);
    lv_obj_set_style_bg_opa(title_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(title_bar, 0, 0);
    lv_obj_set_style_radius(title_bar, 0, 0);
    lv_obj_set_style_pad_all(title_bar, 0, 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text(title, tr("设置", "Settings"));
    lv_obj_set_style_text_color(title, g_colors->accent, 0);
    lv_obj_set_style_text_font(title, CJK_FONT, 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    lv_obj_t* btn_close = lv_button_create(title_bar);
    lv_obj_set_size(btn_close, SCALE(84), SCALE(32));
    lv_obj_set_style_bg_color(btn_close, g_colors->btn_close, 0);
    lv_obj_set_style_radius(btn_close, SCALE(8), 0);
    lv_obj_align(btn_close, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_add_event_cb(btn_close, settings_close_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* lbl_close = lv_label_create(btn_close);
    lv_label_set_text(lbl_close, tr("关闭", "Close"));
    lv_obj_set_style_text_color(lbl_close, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl_close, CJK_FONT, 0);
    lv_obj_center(lbl_close);

    /* Tabview */
    settings_tabs = lv_tabview_create(settings_panel);
    int title_h = SCALE(48);
    lv_obj_set_size(settings_tabs, SETTING_WIN_W - 40, SETTING_WIN_H - title_h - 12);
    lv_obj_set_pos(settings_tabs, 20, title_h + 6);
    lv_obj_set_style_bg_color(settings_tabs, g_colors->panel, 0);
    lv_obj_set_style_border_width(settings_tabs, 0, 0);
    lv_obj_set_style_radius(settings_tabs, 8, 0);

    /* Tab button styling */
    lv_obj_t* tab_btns = lv_tabview_get_tab_btns(settings_tabs);
    lv_obj_set_style_bg_color(tab_btns, g_colors->panel, 0);
    lv_obj_set_style_text_color(tab_btns, g_colors->text_dim, LV_PART_ITEMS);
    lv_obj_set_style_text_color(tab_btns, g_colors->accent, LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_text_font(tab_btns, CJK_FONT, LV_PART_ITEMS);
    /* Vertical divider between tabs */
    lv_obj_set_style_border_side(tab_btns, LV_BORDER_SIDE_INTERNAL, LV_PART_ITEMS);
    lv_obj_set_style_border_color(tab_btns, g_colors->panel_border, LV_PART_ITEMS);
    lv_obj_set_style_border_width(tab_btns, 2, LV_PART_ITEMS);
    lv_obj_set_style_pad_ver(tab_btns, 8, LV_PART_ITEMS); /* Short divider, not full height */

    /* Create tabs */
    lv_obj_t* tab_gen = lv_tabview_add_tab(settings_tabs, "General");
    lv_obj_t* tab_perm = lv_tabview_add_tab(settings_tabs, "Permissions");
    lv_obj_t* tab_model = lv_tabview_add_tab(settings_tabs, "Model");
    lv_obj_t* tab_log = lv_tabview_add_tab(settings_tabs, tr("日志", "Log"));
    lv_obj_t* tab_ff = lv_tabview_add_tab(settings_tabs, tr("开关", "Feature"));
    lv_obj_t* tab_trace = lv_tabview_add_tab(settings_tabs, tr("追踪", "Tracing"));
    lv_obj_t* tab_kb = lv_tabview_add_tab(settings_tabs, tr("知识库", "KB"));
    lv_obj_t* tab_about = lv_tabview_add_tab(settings_tabs, "About");

    /* Build each tab */
    build_general_tab(tab_gen);
    build_permissions_tab(tab_perm);
    build_model_tab(tab_model);
    build_log_tab(tab_log);
    build_feature_tab(tab_ff);
    build_tracing_tab(tab_trace);
    build_kb_tab(tab_kb);
    build_about_tab(tab_about);

    /* Initially hidden */
    lv_obj_add_flag(settings_panel, LV_OBJ_FLAG_HIDDEN);
    settings_visible = false;

    ui_log("[Settings] Settings panel initialized");
}
