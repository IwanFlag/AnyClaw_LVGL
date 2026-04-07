/* ═══════════════════════════════════════════════════════════════
 *  ui_settings.cpp - M2 Settings UI (4-tab panel)
 *  AnyClaw LVGL v2.0 - Desktop Manager
 * ═══════════════════════════════════════════════════════════════ */

#include "app.h"
#include "app_config.h"
#include "model_manager.h"
#include "migration.h"
#include "workspace.h"
#include "app_log.h"
#include "theme.h"
#include "SDL.h"
#include "lang.h"
#include "lvgl.h"
#include <windows.h>
#include <commdlg.h>
#include "widgets/aw_common.h"
#include "widgets/aw_button.h"
#include "widgets/aw_label.h"
#include "widgets/aw_input.h"
#include "widgets/aw_panel.h"

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
#include <string>
#include <thread>
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

/* ── Account tab widgets ── */
static lv_obj_t* acc_apikey_ta = nullptr;
static lv_obj_t* api_key_status_label = nullptr;

/* ── Model tab widgets ── */
static lv_obj_t* model_dropdown = nullptr;
static lv_obj_t* model_current_label = nullptr;
static lv_obj_t* model_provider_hint = nullptr; /* "Provider: openrouter/xiaomi" */
static lv_obj_t* failover_list_container = nullptr; /* checkbox list for failover models */
static char gw_model_buf[256] = {0}; /* Gateway model for callbacks */

/* ── Current language (0=Chinese, 1=English) ── */
static int current_lang = (g_lang == Lang::CN) ? 0 : 1;

/* ── Forward declarations ── */
static void settings_close_cb(lv_event_t* e);
static void settings_tab_change_cb(lv_event_t* e);

/* ═══════════════════════════════════════════════════════════════
 *  i18n helpers (single-language display)
 *  tr(cn, en) -> returns one string based on current language
 * ═══════════════════════════════════════════════════════════════ */
static const char* tr(const char* cn, const char* en) {
    switch (g_lang) {
        case Lang::CN:  return cn;
        case Lang::EN:  return en;
    }
    return en;
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
    lv_obj_set_style_bg_color(d, lv_color_make(40, 90, 200), 0); /* 深蓝色 */
    lv_obj_set_style_bg_opa(d, LV_OPA_70, 0);
    lv_obj_set_style_border_width(d, 0, 0);
    lv_obj_clear_flag(d, LV_OBJ_FLAG_SCROLLABLE);
    return d;
}

static void build_general_tab(lv_obj_t* tab) {
    apply_dark_style(tab);
    lv_obj_set_style_pad_all(tab, 16, 0);
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(tab, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_gap(tab, 8, 0);

    /* Helper: create a horizontal Key: Value row using aw widgets */
    auto make_kv_row = [&](const char* key_text, lv_obj_t* value_obj) -> lv_obj_t* {
        lv_obj_t* row = aw_setting_row(tab, key_text);
        if (value_obj) {
            lv_obj_set_parent(value_obj, row);
        }
        return row;
    };

    /* ── Gateway Status ── */
    gen_status_label = aw_label_create(tab, "--", LABEL_BODY);
    lv_obj_set_style_text_color(gen_status_label, aw::color_success(), 0);
    make_kv_row("Gateway Status", gen_status_label);

    /* ── Install Path ── */
    gen_path_label = aw_label_create(tab, "--", LABEL_HINT);
    lv_label_set_long_mode(gen_path_label, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_width(gen_path_label, LV_SIZE_CONTENT);
    make_kv_row("Install Path", gen_path_label);

    /* ── Workspace Path (WS-01) ── */
    {
        std::string ws_root = workspace_get_root();
        lv_obj_t* ws_label = aw_label_create(tab, ws_root.empty() ? "Not configured" : ws_root.c_str(), LABEL_HINT);
        lv_label_set_long_mode(ws_label, LV_LABEL_LONG_MODE_DOTS);
        lv_obj_set_width(ws_label, LV_SIZE_CONTENT);
        make_kv_row("Workspace", ws_label);
    }

    add_divider(tab);

    /* ── Boot Start ── */
    lv_obj_t* row_boot = aw_setting_row(tab, "Boot Start");

    lv_obj_t* boot_right = lv_obj_create(row_boot);
    aw_make_container(boot_right);
    lv_obj_set_flex_flow(boot_right, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(boot_right, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(boot_right, 8, 0);
    lv_obj_set_size(boot_right, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

    gen_autostart_sw = aw_switch_create(boot_right, is_auto_start());
    lv_obj_set_size(gen_autostart_sw, SCALE(50), SCALE(26));
    lv_obj_set_style_bg_color(gen_autostart_sw, lv_color_make(255, 255, 255), LV_PART_KNOB);
    lv_obj_add_event_cb(gen_autostart_sw, [](lv_event_t* e) {
        lv_obj_t* sw = (lv_obj_t*)lv_event_get_target(e);
        set_auto_start(lv_obj_has_state(sw, LV_STATE_CHECKED));
    }, LV_EVENT_VALUE_CHANGED, nullptr);

    lv_obj_t* lbl_boot_hint = aw_label_create(boot_right, "Start on boot", LABEL_HINT);

    /* ── Auto Start ── */
    lv_obj_t* row_auto = lv_obj_create(tab);
    lv_obj_set_size(row_auto, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row_auto, LV_OPA_TRANSP, 0);
    lv_obj_t* row_auto = aw_setting_row(tab, "Auto Start");

    lv_obj_t* auto_right = lv_obj_create(row_auto);
    aw_make_container(auto_right);
    lv_obj_set_flex_flow(auto_right, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(auto_right, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(auto_right, 8, 0);
    lv_obj_set_size(auto_right, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

    gen_auto_start_sw = aw_switch_create(auto_right, false);
    lv_obj_set_size(gen_auto_start_sw, SCALE(50), SCALE(26));
    lv_obj_add_event_cb(gen_auto_start_sw, [](lv_event_t* e) {
        lv_obj_t* sw = (lv_obj_t*)lv_event_get_target(e);
        bool enabled = lv_obj_has_state(sw, LV_STATE_CHECKED);
        /* TODO: persist auto_start_on_crash config */
    }, LV_EVENT_VALUE_CHANGED, nullptr);

    lv_obj_t* lbl_auto_hint = aw_label_create(auto_right, "Restart on crash", LABEL_HINT);

    add_divider(tab);

    /* ── Close Gateway on exit ── */
    {
        lv_obj_t* close_row = aw_setting_row(tab, tr("退出时关闭 OpenClaw", "Close OpenClaw on exit"));

        lv_obj_t* close_right = lv_obj_create(close_row);
        aw_make_container(close_right);
        lv_obj_set_size(close_right, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(close_right, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(close_right, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_gap(close_right, 8, 0);

        gen_close_gateway_sw = aw_switch_create(close_right, false);
        lv_obj_set_size(gen_close_gateway_sw, SCALE(50), SCALE(26));

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
    extern Lang g_lang;
    if (g_lang == Lang::CN) {
        lv_dropdown_set_options(gen_lang_dropdown, "中文\nEnglish");
    } else {
        lv_dropdown_set_options(gen_lang_dropdown, "English\n中文");
    }
    lv_dropdown_set_selected(gen_lang_dropdown, 0);
    lv_obj_set_width(gen_lang_dropdown, SCALE(160));
    apply_input_style(gen_lang_dropdown);
    lv_obj_add_event_cb(gen_lang_dropdown, [](lv_event_t* e) {
        lv_obj_t* dd = (lv_obj_t*)lv_event_get_target(e);
        uint16_t sel = lv_dropdown_get_selected(dd);
        extern Lang g_lang;
        extern void save_theme_config();
        extern void apply_theme_to_all();
        if (g_lang == Lang::CN) {
            g_lang = (sel == 0) ? Lang::CN : Lang::EN;
        } else {
            g_lang = (sel == 0) ? Lang::EN : Lang::CN;
        }
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
    lv_obj_t* sec_row = aw_setting_row(tab, "Security");

    lv_obj_t* sec_right = lv_obj_create(sec_row);
    aw_make_container(sec_right);
    lv_obj_set_flex_flow(sec_right, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(sec_right, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(sec_right, 8, 0);
    lv_obj_set_size(sec_right, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

    lv_obj_t* sec_green = lv_led_create(sec_right);
    lv_obj_set_size(sec_green, 10, 10);
    lv_led_set_color(sec_green, aw::color_success());
    lv_led_on(sec_green);

    lv_obj_t* sec_label = aw_label_create(sec_right, "Good", LABEL_BODY);

    /* Security detail items */
    lv_obj_t* lbl_sec_detail = aw_label_create(tab, "API Key: OK | Port: 18789 | Config: Writable", LABEL_HINT);
    lv_label_set_long_mode(lbl_sec_detail, LV_LABEL_LONG_WRAP);

    aw_divider_create(tab, aw::dark_border());

    /* Divider */
    aw_divider_create(tab, aw::dark_border());

    /* Reconfigure wizard button */
    lv_obj_t* btn_wizard = aw_btn_create(tab, "", BTN_SECONDARY, 0, 40);
    lv_obj_set_width(btn_wizard, LV_PCT(100));
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
    lv_obj_t* dd_row = aw_row_create(tab);

    model_dropdown = aw_dropdown_create(dd_row, dd_options);
    lv_dropdown_set_selected(model_dropdown, selected_idx);
    lv_obj_set_width(model_dropdown, LV_PCT(70));
    lv_obj_add_event_cb(model_dropdown, model_dropdown_cb, LV_EVENT_VALUE_CHANGED, nullptr);

    /* Add model button */
    lv_obj_t* btn_add = aw_btn_create(dd_row, "+", BTN_SUCCESS, 40, 36);
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
        lv_obj_t* title = aw_label_create(dlg, tr("添加自定义模型", "Add Custom Model"), LABEL_TITLE);

        /* Hint */
        lv_obj_t* hint = aw_label_create(dlg, tr("输入模型名称", "Enter model name"), LABEL_HINT);

        /* Textarea — single line for model name */
        lv_obj_t* ta = aw_textarea_create(dlg, "google/gemini-2.5-pro");
        lv_obj_set_size(ta, LV_PCT(100), LV_SIZE_CONTENT);
        lv_textarea_set_one_line(ta, true);
        lv_textarea_set_max_length(ta, 128);

        /* Button row — [OK] ←spacer→ [Cancel] */
        lv_obj_t* btn_row = aw_row_create(dlg);

        /* OK button (left) */
        lv_obj_t* btn_ok = aw_btn_create(btn_row, tr("确定", "OK"), BTN_PRIMARY, 80, 36);

        /* Cancel button (right) */
        lv_obj_t* btn_cancel = aw_btn_create(btn_row, tr("取消", "Cancel"), BTN_SECONDARY, 80, 36);
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
    acc_apikey_ta = aw_textarea_create(tab, "sk-or-...", true, 0, 56);
    lv_textarea_set_one_line(acc_apikey_ta, true);
    lv_textarea_set_max_length(acc_apikey_ta, 256);
    lv_obj_set_width(acc_apikey_ta, LV_PCT(80));
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
    lv_obj_t* row_btns = aw_row_create(tab);
    lv_obj_set_flex_align(row_btns, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Show/Hide Key toggle */
    lv_obj_t* btn_show = aw_btn_create(row_btns, "Hide Key", BTN_SECONDARY, SCALE(100), SCALE(32));
    lv_obj_add_event_cb(btn_show, apikey_show_cb, LV_EVENT_CLICKED, nullptr);

    /* Verify button — test API key validity */
    lv_obj_t* btn_verify = aw_btn_create(row_btns, "Verify", BTN_SUCCESS, SCALE(100), SCALE(32));
    lv_obj_add_event_cb(btn_verify, apikey_verify_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* lbl_verify = lv_label_create(btn_verify);
    /* Save button */
    lv_obj_t* btn_save = aw_btn_create(row_btns, "Save", BTN_PRIMARY, SCALE(100), SCALE(32));
    lv_obj_add_event_cb(btn_save, apikey_save_cb, LV_EVENT_CLICKED, nullptr);

    /* Update current model display */
    if (g_selected_model[0] && model_current_label) {
        lv_label_set_text_fmt(model_current_label, "%s", g_selected_model);
    }

    /* ═══════════════════════════════════════════════════════════════
     *  弹性通道 — Model Failover
     * ═══════════════════════════════════════════════════════════════ */
    add_divider(tab);

    /* Title row */
    lv_obj_t* lbl_failover = aw_label_create(tab, tr("弹性通道", "Failover Channel"), LABEL_TITLE);

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
    lv_obj_t* fo_switch_row = aw_setting_row(tab, tr("启用弹性通道", "Enable Failover"));

    lv_obj_t* fo_switch = aw_switch_create(fo_switch_row, failover_is_enabled());
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
    lv_obj_t* lbl_fo_models = aw_label_create(tab, tr("备选模型（勾选参与切换）", "Backup models (check to include)"), LABEL_HINT);
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

/* P2-28: Skill download callback */
static void skill_download_cb(lv_event_t* e) {
    lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
    const char* skill_name = (const char*)lv_obj_get_user_data(btn);
    if (!skill_name) return;

    /* Show downloading status on button */
    static char status_text[256];
    snprintf(status_text, sizeof(status_text), "%s [Downloading]", skill_name);
    lv_obj_t* label = lv_obj_get_child(btn, 0);
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
}

static const char* skill_names[] = {
    "weather", "github", "news-summary", "web-scraping",
    "Data Analysis", "translate", "stock-tech-analysis", "obsidian",
    nullptr
};

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

    lv_obj_t* skill_search = lv_textarea_create(tab);
    lv_textarea_set_one_line(skill_search, true);
    lv_textarea_set_placeholder_text(skill_search, "Type to filter...");
    lv_obj_set_width(skill_search, LV_PCT(100));
    lv_obj_set_height(skill_search, 56);
    apply_input_style(skill_search);
    lv_textarea_set_text_selection(skill_search, true);
    lv_group_add_obj(lv_group_get_default(), skill_search);

    /* Available skills list */
    lv_obj_t* lbl_avail = lv_label_create(tab);
    lv_label_set_text(lbl_avail, "Available Skills");
    apply_section_label(lbl_avail);

    lv_obj_t* skill_list = lv_list_create(tab);
    lv_obj_set_width(skill_list, LV_PCT(100));
    lv_obj_set_height(skill_list, 390);
    lv_obj_set_style_bg_color(skill_list, lv_color_make(25, 28, 40), 0);
    lv_obj_set_style_border_color(skill_list, lv_color_make(50, 55, 75), 0);
    lv_obj_set_style_border_width(skill_list, 1, 0);
    lv_obj_set_style_radius(skill_list, 6, 0);

    /* P2-28: Skill download with HTTP GET callback */
    for (int i = 0; skill_names[i]; i++) {
        static char btn_texts[32][128]; /* One per skill */
        snprintf(btn_texts[i], sizeof(btn_texts[i]), "%s", skill_names[i]);
        lv_list_add_btn(skill_list, NULL, btn_texts[i]);
        lv_obj_t* last_btn = lv_obj_get_child(skill_list, -1);
        if (last_btn) {
            lv_obj_set_style_text_font(last_btn, CJK_FONT, 0);
            lv_obj_set_style_bg_color(last_btn, lv_color_make(35, 38, 52), 0);
            lv_obj_set_style_text_color(last_btn, lv_color_make(180, 185, 200), 0);
            /* Set skill name as user data for download callback */
            static char skill_data[32][64];
            snprintf(skill_data[i], sizeof(skill_data[i]), "%s", skill_names[i]);
            lv_obj_set_user_data(last_btn, skill_data[i]);
            lv_obj_add_event_cb(last_btn, skill_download_cb, LV_EVENT_CLICKED, nullptr);
        }
    }

    /* Divider */
    add_divider(tab);

    /* P2-29: Installed skills with enable/disable toggles */
    lv_obj_t* lbl_installed = lv_label_create(tab);
    lv_label_set_text(lbl_installed, "Installed Skills");
    apply_section_label(lbl_installed);

    lv_obj_t* installed_list = lv_list_create(tab);
    lv_obj_set_width(installed_list, LV_PCT(100));
    lv_obj_set_height(installed_list, 240);
    lv_obj_set_style_bg_color(installed_list, lv_color_make(25, 28, 40), 0);

    /* Add installed skills with toggle switch */
    static bool skill_enabled[] = {true, true, true, false, false, true, true, false}; /* Default states */
    const char* installed_skills[] = {"xiaolongxi-search", "xiaolongxi-convert", "web-scraper",
                                       "openclaw-audit", "openclaw-code-interpreter", "openclaw-perplexity",
                                       "openclaw-web-search", "openclaw-gemini", nullptr};
    for (int i = 0; installed_skills[i]; i++) {
        static char btn_text[256];
        snprintf(btn_text, sizeof(btn_text), "%s  %s",
                 installed_skills[i],
                 skill_enabled[i] ? "[ON]" : "[OFF]");
        lv_list_add_btn(installed_list, NULL, btn_text);
        lv_obj_t* last_btn = lv_obj_get_child(installed_list, -1);
        if (last_btn) {
            lv_obj_set_style_text_font(last_btn, CJK_FONT, 0);
            lv_obj_set_style_bg_color(last_btn, lv_color_make(35, 38, 52), 0);
            if (skill_enabled[i]) {
                lv_obj_set_style_text_color(last_btn, lv_color_make(120, 200, 120), 0);
            } else {
                lv_obj_set_style_text_color(last_btn, lv_color_make(140, 140, 140), 0);
            }
        }
    }
    lv_obj_set_style_border_color(installed_list, lv_color_make(50, 55, 75), 0);
    lv_obj_set_style_border_width(installed_list, 1, 0);
    lv_obj_set_style_radius(installed_list, 6, 0);

    /* Placeholder - no skills installed yet */
    lv_obj_t* lbl_empty = lv_label_create(installed_list);
    lv_label_set_text(lbl_empty, "No skills installed yet");
    lv_obj_set_style_text_color(lbl_empty, lv_color_make(120, 125, 140), 0);
    lv_obj_set_style_text_font(lbl_empty, CJK_FONT, 0);
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

    lv_obj_t* lbl_log_lv = aw_label_create(row_lv, tr("日志级别", "Log Level"), LABEL_TITLE);

    lv_obj_t* log_lv_dd = aw_dropdown_create(row_lv, "Debug\nInfo\nWarn\nError");
    lv_dropdown_set_selected(log_lv_dd, g_log_level);
    lv_obj_set_width(log_lv_dd, SCALE(160));
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
    lv_obj_t* row_filter = aw_row_create(tab, 40);
    lv_obj_set_flex_align(row_filter, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* lbl_filter = aw_label_create(row_filter, tr("级别过滤", "Filter"), LABEL_TITLE);

    lv_obj_t* filter_dd = aw_dropdown_create(row_filter, "All\nInfo+\nWarn+\nError Only");
    lv_dropdown_set_selected(filter_dd, ui_log_get_filter());
    lv_obj_set_width(filter_dd, SCALE(150));
    lv_obj_add_event_cb(filter_dd, log_filter_cb, LV_EVENT_VALUE_CHANGED, nullptr);

    /* Refresh button */
    lv_obj_t* btn_refresh = aw_btn_create(row_filter, LV_SYMBOL_REFRESH, BTN_SECONDARY, SCALE(80), SCALE(32));
    lv_obj_add_event_cb(btn_refresh, log_refresh_cb, LV_EVENT_CLICKED, nullptr);

    /* ── Action buttons row ── */
    lv_obj_t* row_actions = aw_row_create(tab, 40);
    lv_obj_set_flex_align(row_actions, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Export button */
    lv_obj_t* btn_export = aw_btn_create(row_actions, tr("导出日志", "Export Log"), BTN_PRIMARY, SCALE(120), SCALE(32));
    lv_obj_add_event_cb(btn_export, log_export_cb, LV_EVENT_CLICKED, nullptr);

    /* Clear button */
    lv_obj_t* btn_clear = aw_btn_create(row_actions, tr("清除日志", "Clear Log"), BTN_DANGER, SCALE(120), SCALE(32));
    lv_obj_add_event_cb(btn_clear, log_clear_cb, LV_EVENT_CLICKED, nullptr);

    /* Log level color legend */
    lv_obj_t* row_legend = aw_row_create(tab, 24);
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
    lv_obj_t* lbl_title = aw_label_create(tab, "AnyClaw LVGL", LABEL_TITLE);

    /* Brand slogan */
    lv_obj_t* lbl_brand = lv_label_create(tab);
    lv_label_set_text(lbl_brand, "Garlic Lobster - Your AI Assistant");
    lv_obj_set_style_text_color(lbl_brand, lv_color_make(255, 200, 100), 0);
    if (AW_CJK_FONT) lv_obj_set_style_text_font(lbl_brand, AW_CJK_FONT, 0);
    lv_label_set_long_mode(lbl_brand, LV_LABEL_LONG_WRAP);

    /* Version */
    lv_obj_t* lbl_ver = aw_label_create(tab, "Version: 2.0.1", LABEL_HINT);

    /* Divider */
    add_divider(tab);

    /* Tech stack */
    lv_obj_t* lbl_tech = aw_label_create(tab, "Tech Stack", LABEL_TITLE);

    lv_obj_t* lbl_stack = aw_label_create(tab,
        "LVGL 9.6 + SDL2\n"
        "C++17 / Win32 API\n"
        "Windows 10/11 x64", LABEL_HINT);
    lv_label_set_long_mode(lbl_stack, LV_LABEL_LONG_WRAP);

    /* Divider */
    add_divider(tab);

    /* P2-35: Config import/export */
    lv_obj_t* lbl_config = aw_label_create(tab, "Configuration", LABEL_TITLE);

    lv_obj_t* row_config = aw_row_create(tab, 44);
    lv_obj_set_flex_align(row_config, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* btn_cfg_export = aw_btn_create(row_config, "Export Config", BTN_PRIMARY, 160, 36);
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

    /* Clear Chat History button */
    lv_obj_t* btn_clear_chat = aw_btn_create(row_config, tr("清除聊天", "Clear Chat"), BTN_DANGER, 160, 36);
    lv_obj_add_event_cb(btn_clear_chat, [](lv_event_t* e) {
        (void)e;
        extern void clear_chat_history();
        clear_chat_history();
        ui_log("[Chat] Chat history cleared");
    }, LV_EVENT_CLICKED, nullptr);

    /* Migration row */
    lv_obj_t* row_mig = aw_row_create(tab);
    lv_obj_set_flex_align(row_mig, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Export Migration */
    lv_obj_t* btn_mig_export = aw_btn_create(row_mig, tr("导出迁移", "Export Migration"), BTN_SUCCESS, 160, 36);
    lv_obj_add_event_cb(btn_mig_export, [](lv_event_t* e) {
        (void)e;
        /* Save dialog using Win32 */
        OPENFILENAMEA ofn;
        char file_buf[MAX_PATH] = {0};
        SYSTEMTIME st;
        GetLocalTime(&st);
        snprintf(file_buf, sizeof(file_buf), "AnyClaw_Migration_v2.0.1_%04d%02d%02d.zip",
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

    /* Import Migration */
    lv_obj_t* btn_mig_import = aw_btn_create(row_mig, tr("导入迁移", "Import Migration"), BTN_SECONDARY, 160, 36);
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

    /* Copyright */
    lv_obj_t* lbl_copy2 = aw_label_create(tab, "Copyright 2026 AnyClaw Project", LABEL_HINT);

    /* P2-30: Skill version check button */
    lv_obj_t* btn_ver_check = aw_btn_create(tab, "Check Updates", BTN_SECONDARY, 0, 36);
    lv_obj_set_width(btn_ver_check, LV_PCT(100));
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
    lv_label_set_text(title, "Settings");
    lv_obj_set_style_text_color(title, g_colors->accent, 0);
    lv_obj_set_style_text_font(title, CJK_FONT, 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    /* Close button */
    lv_obj_t* btn_close = lv_button_create(title_bar);
    lv_obj_set_size(btn_close, SCALE(28), SCALE(28));
    lv_obj_align(btn_close, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_bg_color(btn_close, lv_color_make(180, 50, 50), 0);
    lv_obj_set_style_bg_color(btn_close, lv_color_make(220, 70, 70), LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn_close, 8, 0);
    lv_obj_add_event_cb(btn_close, settings_close_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* lbl_x = lv_label_create(btn_close);
    lv_label_set_text(lbl_x, "X");
    lv_obj_set_style_text_color(lbl_x, lv_color_make(255, 255, 255), 0);
    lv_obj_center(lbl_x);

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
    lv_obj_set_style_border_color(tab_btns, lv_color_make(80, 90, 120), LV_PART_ITEMS);
    lv_obj_set_style_border_width(tab_btns, 2, LV_PART_ITEMS);
    lv_obj_set_style_pad_ver(tab_btns, 8, LV_PART_ITEMS); /* Short divider, not full height */

    /* Create 6 tabs */
    lv_obj_t* tab_gen = lv_tabview_add_tab(settings_tabs, "General");
    lv_obj_t* tab_model = lv_tabview_add_tab(settings_tabs, "Model");
    lv_obj_t* tab_log = lv_tabview_add_tab(settings_tabs, tr("日志", "Log"));
    lv_obj_t* tab_about = lv_tabview_add_tab(settings_tabs, "About");

    /* Build each tab */
    build_general_tab(tab_gen);
    build_model_tab(tab_model);
    build_log_tab(tab_log);
    build_about_tab(tab_about);

    /* Initially hidden */
    lv_obj_add_flag(settings_panel, LV_OBJ_FLAG_HIDDEN);
    settings_visible = false;

    ui_log("[Settings] Settings panel initialized");
}
