/* ═══════════════════════════════════════════════════════════════
 *  ui_settings.cpp - M2 Settings UI (4-tab panel)
 *  AnyClaw LVGL v2.0 - Desktop Manager
 * ═══════════════════════════════════════════════════════════════ */

#include "app.h"
#include "SDL.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <windows.h>

extern const lv_font_t lv_font_mshy_16;
extern const lv_font_t lv_font_source_han_sans_sc_16_cjk;
#define CJK_FONT (&lv_font_source_han_sans_sc_16_cjk)

/* Layout constants - match ui_main.cpp */
#define WIN_W 1200
#define WIN_H 800

/* ── Settings panel state ── */
static lv_obj_t* settings_panel = nullptr;
static lv_obj_t* settings_tabs = nullptr;
static bool settings_visible = false;

/* ── General tab widgets ── */
static lv_obj_t* gen_status_label = nullptr;
static lv_obj_t* gen_path_label = nullptr;
static lv_obj_t* gen_autostart_sw = nullptr;
static lv_obj_t* gen_lang_dropdown = nullptr;

/* ── Account tab widgets ── */
static lv_obj_t* acc_apikey_ta = nullptr;

/* ── Model tab widgets ── */
static lv_obj_t* model_list = nullptr;
static lv_obj_t* model_search_ta = nullptr;
static lv_obj_t* model_current_label = nullptr;

/* ── Current language (0=Chinese, 1=English) ── */
static int current_lang = 0;

/* ── Forward declarations ── */
static void settings_close_cb(lv_event_t* e);
static void settings_tab_change_cb(lv_event_t* e);

/* ═══════════════════════════════════════════════════════════════
 *  i18n helpers
 * ═══════════════════════════════════════════════════════════════ */
static const char* i18n(const char* zh, const char* en) {
    return current_lang == 0 ? zh : en;
}

/* ═══════════════════════════════════════════════════════════════
 *  Style helpers
 * ═══════════════════════════════════════════════════════════════ */
static void apply_dark_style(lv_obj_t* obj) {
    lv_obj_set_style_bg_color(obj, lv_color_make(30, 33, 48), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(obj, lv_color_make(200, 205, 220), 0);
    lv_obj_set_style_text_font(obj, CJK_FONT, 0);
}

static void apply_input_style(lv_obj_t* obj) {
    lv_obj_set_style_bg_color(obj, lv_color_make(20, 22, 35), 0);
    lv_obj_set_style_border_color(obj, lv_color_make(60, 65, 90), 0);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_radius(obj, 6, 0);
    lv_obj_set_style_text_color(obj, lv_color_make(200, 205, 220), 0);
    lv_obj_set_style_text_font(obj, CJK_FONT, 0);
    lv_obj_set_style_pad_all(obj, 8, 0);
}

static void apply_section_label(lv_obj_t* obj) {
    lv_obj_set_style_text_color(obj, lv_color_make(130, 170, 240), 0);
    lv_obj_set_style_text_font(obj, CJK_FONT, 0);
}

static void apply_hint_label(lv_obj_t* obj) {
    lv_obj_set_style_text_color(obj, lv_color_make(90, 95, 120), 0);
    lv_obj_set_style_text_font(obj, CJK_FONT, 0);
}

/* ═══════════════════════════════════════════════════════════════
 *  GENERAL TAB
 * ═══════════════════════════════════════════════════════════════ */
static void build_general_tab(lv_obj_t* tab) {
    apply_dark_style(tab);
    lv_obj_set_style_pad_all(tab, 16, 0);
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(tab, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_gap(tab, 12, 0);

    /* Status section title */
    lv_obj_t* lbl_status = lv_label_create(tab);
    lv_label_set_text(lbl_status, i18n("OpenClaw 状态", "OpenClaw Status"));
    apply_section_label(lbl_status);

    /* Status value */
    gen_status_label = lv_label_create(tab);
    lv_label_set_text(gen_status_label, "--");
    lv_obj_set_style_text_color(gen_status_label, lv_color_make(200, 205, 220), 0);
    lv_obj_set_style_text_font(gen_status_label, CJK_FONT, 0);

    /* Path section title */
    lv_obj_t* lbl_path = lv_label_create(tab);
    lv_label_set_text(lbl_path, i18n("安装路径", "Install Path"));
    apply_section_label(lbl_path);

    /* Path value */
    gen_path_label = lv_label_create(tab);
    lv_label_set_text(gen_path_label, "--");
    lv_label_set_long_mode(gen_path_label, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_width(gen_path_label, LV_SIZE_CONTENT);
    lv_obj_set_style_text_color(gen_path_label, lv_color_make(160, 165, 185), 0);
    lv_obj_set_style_text_font(gen_path_label, CJK_FONT, 0);

    /* Divider */
    lv_obj_t* div1 = lv_obj_create(tab);
    lv_obj_set_size(div1, LV_PCT(100), 1);
    lv_obj_set_style_bg_color(div1, lv_color_make(55, 60, 85), 0);
    lv_obj_set_style_border_width(div1, 0, 0);
    lv_obj_clear_flag(div1, LV_OBJ_FLAG_SCROLLABLE);

    /* Auto-start toggle */
    lv_obj_t* lbl_auto = lv_label_create(tab);
    lv_label_set_text(lbl_auto, i18n("开机自启", "Auto Start"));
    apply_section_label(lbl_auto);

    lv_obj_t* row_auto = lv_obj_create(tab);
    lv_obj_set_size(row_auto, LV_PCT(100), 40);
    lv_obj_set_style_bg_opa(row_auto, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row_auto, 0, 0);
    lv_obj_set_style_pad_all(row_auto, 0, 0);
    lv_obj_clear_flag(row_auto, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(row_auto, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row_auto, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(row_auto, 12, 0);

    gen_autostart_sw = lv_switch_create(row_auto);
    lv_obj_set_size(gen_autostart_sw, 50, 26);
    lv_obj_set_style_bg_color(gen_autostart_sw, lv_color_make(60, 65, 90), LV_PART_MAIN);
    lv_obj_set_style_bg_color(gen_autostart_sw, lv_color_make(60, 130, 220), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(gen_autostart_sw, lv_color_make(255, 255, 255), LV_PART_KNOB);

    lv_obj_t* lbl_auto_hint = lv_label_create(row_auto);
    lv_label_set_text(lbl_auto_hint, i18n("启动时自动运行 Gateway", "Start Gateway on boot"));
    apply_hint_label(lbl_auto_hint);

    /* Language selector */
    lv_obj_t* lbl_lang = lv_label_create(tab);
    lv_label_set_text(lbl_lang, i18n("语言 / Language", "Language / 语言"));
    apply_section_label(lbl_lang);

    gen_lang_dropdown = lv_dropdown_create(tab);
    lv_dropdown_set_options(gen_lang_dropdown, "中文\nEnglish\n双语 / Bilingual");
    lv_dropdown_set_selected(gen_lang_dropdown, current_lang);
    lv_obj_set_width(gen_lang_dropdown, 160);
    apply_input_style(gen_lang_dropdown);
}

/* ═══════════════════════════════════════════════════════════════
 *  ACCOUNT TAB
 * ═══════════════════════════════════════════════════════════════ */
static void build_account_tab(lv_obj_t* tab) {
    apply_dark_style(tab);
    lv_obj_set_style_pad_all(tab, 16, 0);
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(tab, 12, 0);

    /* API Key section */
    lv_obj_t* lbl_key = lv_label_create(tab);
    lv_label_set_text(lbl_key, "OpenRouter API Key");
    apply_section_label(lbl_key);

    lv_obj_t* lbl_hint = lv_label_create(tab);
    lv_label_set_text(lbl_hint, i18n("在 openrouter.ai 获取 API Key", "Get your API key at openrouter.ai"));
    apply_hint_label(lbl_hint);

    /* API Key input */
    acc_apikey_ta = lv_textarea_create(tab);
    lv_textarea_set_one_line(acc_apikey_ta, true);
    lv_textarea_set_placeholder_text(acc_apikey_ta, "sk-or-...");
    lv_obj_set_width(acc_apikey_ta, LV_PCT(100));
    lv_obj_set_height(acc_apikey_ta, 44);
    apply_input_style(acc_apikey_ta);

    /* Show/Hide toggle */
    lv_obj_t* row_vis = lv_obj_create(tab);
    lv_obj_set_size(row_vis, LV_PCT(100), 36);
    lv_obj_set_style_bg_opa(row_vis, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row_vis, 0, 0);
    lv_obj_set_style_pad_all(row_vis, 0, 0);
    lv_obj_clear_flag(row_vis, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(row_vis, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(row_vis, 8, 0);

    lv_obj_t* btn_show = lv_button_create(row_vis);
    lv_obj_set_size(btn_show, 100, 32);
    lv_obj_set_style_bg_color(btn_show, lv_color_make(60, 65, 90), 0);
    lv_obj_set_style_radius(btn_show, 6, 0);
    lv_obj_t* btn_show_lbl = lv_label_create(btn_show);
    lv_label_set_text(btn_show_lbl, i18n("显示 Key", "Show Key"));
    lv_obj_set_style_text_font(btn_show_lbl, CJK_FONT, 0);
    lv_obj_center(btn_show_lbl);

    lv_obj_t* btn_save = lv_button_create(row_vis);
    lv_obj_set_size(btn_save, 100, 32);
    lv_obj_set_style_bg_color(btn_save, lv_color_make(40, 100, 180), 0);
    lv_obj_set_style_radius(btn_save, 6, 0);
    lv_obj_t* btn_save_lbl = lv_label_create(btn_save);
    lv_label_set_text(btn_save_lbl, i18n("保存", "Save"));
    lv_obj_set_style_text_font(btn_save_lbl, CJK_FONT, 0);
    lv_obj_center(btn_save_lbl);

    /* Separator */
    lv_obj_t* div1 = lv_obj_create(tab);
    lv_obj_set_size(div1, LV_PCT(100), 1);
    lv_obj_set_style_bg_color(div1, lv_color_make(55, 60, 85), 0);
    lv_obj_set_style_border_width(div1, 0, 0);
    lv_obj_clear_flag(div1, LV_OBJ_FLAG_SCROLLABLE);

    /* Account info */
    lv_obj_t* lbl_info = lv_label_create(tab);
    lv_label_set_text(lbl_info, i18n("账户信息将存储在本地配置文件中", "Account info stored in local config"));
    apply_hint_label(lbl_info);
}

/* ═══════════════════════════════════════════════════════════════
 *  MODEL TAB
 * ═══════════════════════════════════════════════════════════════ */
static const char* model_list_items[] = {
    "google/gemini-2.5-pro-preview",
    "google/gemini-2.0-flash",
    "anthropic/claude-3.5-sonnet",
    "anthropic/claude-3-haiku",
    "openai/gpt-4o",
    "openai/gpt-4o-mini",
    "xai/grok-2",
    "deepseek/deepseek-chat",
    "qwen/qwen-2.5-72b",
    "meta-llama/llama-3.1-405b",
    nullptr
};

static void model_search_cb(lv_event_t* e) {
    lv_obj_t* ta = (lv_obj_t*)lv_event_get_target(e);
    const char* text = lv_textarea_get_text(ta);
    if (model_list) {
        lv_obj_t* list_btn = nullptr;
        uint32_t i = 0;
        while ((list_btn = lv_obj_get_child(model_list, i)) != nullptr) {
            lv_obj_delete(list_btn);
        }
        for (int j = 0; model_list_items[j]; j++) {
            if (text[0] == '\0' || strstr(model_list_items[j], text)) {
                lv_obj_t* btn = lv_list_add_btn(model_list, LV_SYMBOL_FILE, model_list_items[j]);
                lv_obj_set_style_text_font(btn, CJK_FONT, 0);
                lv_obj_set_style_bg_color(btn, lv_color_make(35, 38, 52), 0);
                lv_obj_set_style_text_color(btn, lv_color_make(180, 185, 200), 0);
            }
        }
    }
}

static void model_select_cb(lv_event_t* e) {
    lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
    lv_obj_t* label = lv_obj_get_child(btn, 1);
    if (label && model_current_label) {
        const char* text = lv_label_get_text(label);
        static char buf[256];
        snprintf(buf, sizeof(buf), "%s: %s", i18n("当前模型", "Current Model"), text);
        lv_label_set_text(model_current_label, buf);
    }
}

static void build_model_tab(lv_obj_t* tab) {
    apply_dark_style(tab);
    lv_obj_set_style_pad_all(tab, 16, 0);
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(tab, 10, 0);

    /* Current model display */
    lv_obj_t* lbl_current = lv_label_create(tab);
    lv_label_set_text(lbl_current, i18n("当前选择", "Current Selection"));
    apply_section_label(lbl_current);

    model_current_label = lv_label_create(tab);
    lv_label_set_text(model_current_label, i18n("当前模型: 未选择", "Current Model: None"));
    lv_obj_set_style_text_color(model_current_label, lv_color_make(220, 220, 230), 0);
    lv_obj_set_style_text_font(model_current_label, CJK_FONT, 0);

    /* Search box */
    lv_obj_t* lbl_search = lv_label_create(tab);
    lv_label_set_text(lbl_search, i18n("搜索模型", "Search Models"));
    apply_section_label(lbl_search);

    model_search_ta = lv_textarea_create(tab);
    lv_textarea_set_one_line(model_search_ta, true);
    lv_textarea_set_placeholder_text(model_search_ta, i18n("输入关键词过滤...", "Type to filter..."));
    lv_obj_set_width(model_search_ta, LV_PCT(100));
    lv_obj_set_height(model_search_ta, 38);
    apply_input_style(model_search_ta);
    lv_obj_add_event_cb(model_search_ta, model_search_cb, LV_EVENT_VALUE_CHANGED, nullptr);

    /* Model list */
    model_list = lv_list_create(tab);
    lv_obj_set_width(model_list, LV_PCT(100));
    lv_obj_set_height(model_list, 180);
    lv_obj_set_style_bg_color(model_list, lv_color_make(25, 28, 40), 0);
    lv_obj_set_style_border_color(model_list, lv_color_make(50, 55, 75), 0);
    lv_obj_set_style_border_width(model_list, 1, 0);
    lv_obj_set_style_radius(model_list, 6, 0);

    for (int i = 0; model_list_items[i]; i++) {
        lv_obj_t* btn = lv_list_add_btn(model_list, LV_SYMBOL_FILE, model_list_items[i]);
        lv_obj_set_style_text_font(btn, CJK_FONT, 0);
        lv_obj_set_style_bg_color(btn, lv_color_make(35, 38, 52), 0);
        lv_obj_set_style_text_color(btn, lv_color_make(180, 185, 200), 0);
        lv_obj_add_event_cb(btn, model_select_cb, LV_EVENT_CLICKED, nullptr);
    }
}

/* ═══════════════════════════════════════════════════════════════
 *  ABOUT TAB
 * ═══════════════════════════════════════════════════════════════ */
static void build_about_tab(lv_obj_t* tab) {
    apply_dark_style(tab);
    lv_obj_set_style_pad_all(tab, 20, 0);
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(tab, 14, 0);

    /* App logo/title */
    lv_obj_t* lbl_title = lv_label_create(tab);
    lv_label_set_text(lbl_title, "AnyClaw LVGL");
    lv_obj_set_style_text_color(lbl_title, lv_color_make(100, 160, 255), 0);
    lv_obj_set_style_text_font(lbl_title, CJK_FONT, 0);

    /* Version */
    lv_obj_t* lbl_ver = lv_label_create(tab);
    lv_label_set_text(lbl_ver, "Version: 2.0.0");
    lv_obj_set_style_text_color(lbl_ver, lv_color_make(160, 165, 185), 0);
    lv_obj_set_style_text_font(lbl_ver, CJK_FONT, 0);

    /* Divider */
    lv_obj_t* div1 = lv_obj_create(tab);
    lv_obj_set_size(div1, LV_PCT(100), 1);
    lv_obj_set_style_bg_color(div1, lv_color_make(55, 60, 85), 0);
    lv_obj_set_style_border_width(div1, 0, 0);
    lv_obj_clear_flag(div1, LV_OBJ_FLAG_SCROLLABLE);

    /* Tech stack */
    lv_obj_t* lbl_tech = lv_label_create(tab);
    lv_label_set_text(lbl_tech, i18n("技术栈", "Tech Stack"));
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
    lv_obj_t* div2 = lv_obj_create(tab);
    lv_obj_set_size(div2, LV_PCT(100), 1);
    lv_obj_set_style_bg_color(div2, lv_color_make(55, 60, 85), 0);
    lv_obj_set_style_border_width(div2, 0, 0);
    lv_obj_clear_flag(div2, LV_OBJ_FLAG_SCROLLABLE);

    /* Copyright */
    lv_obj_t* lbl_copy = lv_label_create(tab);
    lv_label_set_text(lbl_copy, "Copyright 2026 AnyClaw Project");
    apply_hint_label(lbl_copy);
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
            case ClawStatus::Running: sts = i18n("运行中 (绿色)", "Running (Green)"); break;
            case ClawStatus::Detected: sts = i18n("已安装 (黄色)", "Detected (Yellow)"); break;
            case ClawStatus::Error: sts = i18n("错误 (红色)", "Error (Red)"); break;
            default: sts = i18n("未安装 (灰色)", "Not Installed (Gray)"); break;
        }
        snprintf(status_buf, sizeof(status_buf), "%s", sts);
        lv_label_set_text(gen_status_label, status_buf);
    }
    if (gen_path_label) {
        OpenClawInfo info = app_detect_openclaw();
        lv_label_set_text(gen_path_label, info.executable[0] ? info.executable : "--");
    }
}

void ui_settings_close() {
    settings_close_cb(nullptr);
}

bool ui_settings_is_open() {
    return settings_visible;
}

/* ═══════════════════════════════════════════════════════════════
 *  SETTINGS INIT - sized for 1200x800 window
 * ═══════════════════════════════════════════════════════════════ */
void ui_settings_init(lv_obj_t* parent) {
    /* Modal overlay covering entire 1200x800 screen */
    settings_panel = lv_obj_create(parent);
    lv_obj_set_size(settings_panel, WIN_W, WIN_H);
    lv_obj_set_pos(settings_panel, 0, 0);
    lv_obj_set_style_bg_color(settings_panel, lv_color_make(20, 22, 35), 0);
    lv_obj_set_style_bg_opa(settings_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(settings_panel, 0, 0);
    lv_obj_set_style_radius(settings_panel, 0, 0);
    lv_obj_set_style_pad_all(settings_panel, 0, 0);
    lv_obj_clear_flag(settings_panel, LV_OBJ_FLAG_SCROLLABLE);

    /* Settings title bar */
    lv_obj_t* title_bar = lv_obj_create(settings_panel);
    lv_obj_set_size(title_bar, WIN_W, 48);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_set_style_bg_color(title_bar, lv_color_make(40, 44, 60), 0);
    lv_obj_set_style_bg_opa(title_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(title_bar, 0, 0);
    lv_obj_set_style_radius(title_bar, 0, 0);
    lv_obj_set_style_pad_all(title_bar, 0, 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text(title, i18n("设置 / Settings", "Settings / 设置"));
    lv_obj_set_style_text_color(title, lv_color_make(100, 160, 255), 0);
    lv_obj_set_style_text_font(title, CJK_FONT, 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    /* Close button */
    lv_obj_t* btn_close = lv_button_create(title_bar);
    lv_obj_set_size(btn_close, 36, 36);
    lv_obj_align(btn_close, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_bg_color(btn_close, lv_color_make(180, 50, 50), 0);
    lv_obj_set_style_bg_color(btn_close, lv_color_make(220, 70, 70), LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn_close, 8, 0);
    lv_obj_add_event_cb(btn_close, settings_close_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* lbl_x = lv_label_create(btn_close);
    lv_label_set_text(lbl_x, "X");
    lv_obj_set_style_text_color(lbl_x, lv_color_make(255, 255, 255), 0);
    lv_obj_center(lbl_x);

    /* Tabview - larger for 1200x800 */
    settings_tabs = lv_tabview_create(settings_panel);
    lv_obj_set_size(settings_tabs, WIN_W - 40, WIN_H - 60);
    lv_obj_set_pos(settings_tabs, 20, 54);
    lv_obj_set_style_bg_color(settings_tabs, lv_color_make(28, 31, 45), 0);
    lv_obj_set_style_border_width(settings_tabs, 0, 0);
    lv_obj_set_style_radius(settings_tabs, 8, 0);

    /* Tab button styling */
    lv_obj_t* tab_btns = lv_tabview_get_tab_btns(settings_tabs);
    lv_obj_set_style_bg_color(tab_btns, lv_color_make(35, 38, 52), 0);
    lv_obj_set_style_text_color(tab_btns, lv_color_make(140, 145, 165), LV_PART_ITEMS);
    lv_obj_set_style_text_color(tab_btns, lv_color_make(100, 160, 255), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(tab_btns, lv_color_make(50, 55, 80), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_text_font(tab_btns, CJK_FONT, LV_PART_ITEMS);

    /* Create 4 tabs */
    lv_obj_t* tab_gen = lv_tabview_add_tab(settings_tabs, i18n("常规", "General"));
    lv_obj_t* tab_acc = lv_tabview_add_tab(settings_tabs, i18n("账号", "Account"));
    lv_obj_t* tab_model = lv_tabview_add_tab(settings_tabs, i18n("模型", "Model"));
    lv_obj_t* tab_about = lv_tabview_add_tab(settings_tabs, i18n("关于", "About"));

    /* Build each tab */
    build_general_tab(tab_gen);
    build_account_tab(tab_acc);
    build_model_tab(tab_model);
    build_about_tab(tab_about);

    /* Initially hidden */
    lv_obj_add_flag(settings_panel, LV_OBJ_FLAG_HIDDEN);
    settings_visible = false;

    app_log("[Settings] Settings panel initialized");
}
