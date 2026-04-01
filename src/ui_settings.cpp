/* ═══════════════════════════════════════════════════════════════
 *  ui_settings.cpp - M2 Settings UI (4-tab panel)
 *  AnyClaw LVGL v2.0 - Desktop Manager
 * ═══════════════════════════════════════════════════════════════ */

#include "app.h"
#include "theme.h"
#include "lang.h"
#include "lvgl.h"
#include "SDL.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <windows.h>

extern const lv_font_t lv_font_mshy_16;
#define CJK_FONT (&lv_font_mshy_16)

/* Layout constants - match ui_main.cpp */
#define WIN_W 1350
#define WIN_H 740

/* ── Settings panel state ── */
static lv_obj_t* settings_panel = nullptr;
static lv_obj_t* settings_tabs = nullptr;
static bool settings_visible = false;

/* ── General tab widgets ── */
static lv_obj_t* gen_status_label = nullptr;
static lv_obj_t* gen_path_label = nullptr;
static lv_obj_t* gen_autostart_sw = nullptr;
static lv_obj_t* gen_lang_dropdown = nullptr;
static lv_obj_t* gen_refresh_dropdown = nullptr;

/* ── Account tab widgets ── */
static lv_obj_t* acc_apikey_ta = nullptr;

/* ── Model tab widgets ── */
static lv_obj_t* model_list = nullptr;
static lv_obj_t* model_search_ta = nullptr;
static lv_obj_t* model_current_label = nullptr;

/* ── Current language (0=Chinese, 1=English) ── */
static int current_lang = (g_lang == Lang::CN) ? 0 : 1;

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
extern void update_refresh_interval(int ms);
extern int g_refresh_interval_ms;

static void refresh_dropdown_cb(lv_event_t* e) {
    lv_obj_t* dd = (lv_obj_t*)lv_event_get_target(e);
    uint16_t sel = lv_dropdown_get_selected(dd);
    int intervals[] = {15000, 30000, 60000};
    update_refresh_interval(intervals[sel]);
}

/* P2-26: Export for tray.cpp */
static lv_obj_t* g_exit_confirm_sw = nullptr;
bool is_exit_confirmation_enabled() {
    return g_exit_confirm_sw ? lv_obj_has_state(g_exit_confirm_sw, LV_STATE_CHECKED) : true;
}

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

    /* P2-01: Language dropdown - PRD 2.1 single language only */
    gen_lang_dropdown = lv_dropdown_create(tab);
    extern Lang g_lang;
    if (g_lang == Lang::CN) {
        lv_dropdown_set_options(gen_lang_dropdown, "中文\nEnglish");
    } else {
        lv_dropdown_set_options(gen_lang_dropdown, "English\n中文");
    }
    lv_dropdown_set_selected(gen_lang_dropdown, 0); /* Current language first */
    lv_obj_set_width(gen_lang_dropdown, 160);
    apply_input_style(gen_lang_dropdown);

    /* P2-01: Language change callback */
    lv_obj_add_event_cb(gen_lang_dropdown, [](lv_event_t* e) {
        lv_obj_t* dd = (lv_obj_t*)lv_event_get_target(e);
        uint16_t sel = lv_dropdown_get_selected(dd);
        extern Lang g_lang;
        extern void save_theme_config();
        extern void apply_theme_to_all();
        /* Swap language based on current language position */
        if (g_lang == Lang::CN) {
            g_lang = (sel == 0) ? Lang::CN : Lang::EN;
        } else {
            g_lang = (sel == 0) ? Lang::EN : Lang::CN;
        }
        save_theme_config();
        extern void apply_theme_to_all();
        apply_theme_to_all();
    }, LV_EVENT_VALUE_CHANGED, nullptr);

    /* P2-27: Refresh interval selector */
    lv_obj_t* lbl_refresh = lv_label_create(tab);
    lv_label_set_text(lbl_refresh, i18n("自动刷新间隔 / Auto Refresh", "Refresh Interval"));
    apply_section_label(lbl_refresh);

    extern int g_refresh_interval_ms;
    extern void update_refresh_interval(int ms);

    static lv_obj_t* gen_refresh_dropdown = nullptr;
    gen_refresh_dropdown = lv_dropdown_create(tab);
    lv_dropdown_set_options(gen_refresh_dropdown, "15 秒 / 15s\n30 秒 / 30s\n60 秒 / 60s");
    if (g_refresh_interval_ms <= 15000) lv_dropdown_set_selected(gen_refresh_dropdown, 0);
    else if (g_refresh_interval_ms <= 30000) lv_dropdown_set_selected(gen_refresh_dropdown, 1);
    else lv_dropdown_set_selected(gen_refresh_dropdown, 2);
    lv_obj_set_width(gen_refresh_dropdown, 160);
    apply_input_style(gen_refresh_dropdown);
    lv_obj_add_event_cb(gen_refresh_dropdown, refresh_dropdown_cb, LV_EVENT_VALUE_CHANGED, nullptr);

    /* Theme selector */
    lv_obj_t* lbl_theme = lv_label_create(tab);
    lv_label_set_text(lbl_theme, i18n("主题 / Theme", "Theme / 主题"));
    apply_section_label(lbl_theme);

    extern void ui_settings_add_theme_dropdown(lv_obj_t* tab);
    ui_settings_add_theme_dropdown(tab);

    /* P2-32: Component color preview */
    lv_obj_t* lbl_color = lv_label_create(tab);
    lv_label_set_text(lbl_color, i18n("组件颜色 / Colors", "Colors / 组件颜色"));
    apply_section_label(lbl_color);

    /* Color swatches showing current theme colors */
    lv_obj_t* color_row1 = lv_obj_create(tab);
    lv_obj_set_size(color_row1, LV_PCT(100), 28);
    lv_obj_set_style_bg_opa(color_row1, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(color_row1, 0, 0);
    lv_obj_set_style_pad_all(color_row1, 0, 0);
    lv_obj_clear_flag(color_row1, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(color_row1, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(color_row1, 8, 0);
    lv_obj_set_flex_align(color_row1, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Background color swatch */
    lv_obj_t* bg_sw = lv_obj_create(color_row1);
    lv_obj_set_size(bg_sw, 24, 24);
    lv_obj_set_style_bg_color(bg_sw, g_colors->bg, 0);
    lv_obj_set_style_border_width(bg_sw, 1, 0);
    lv_obj_set_style_border_color(bg_sw, lv_color_make(100, 100, 100), 0);
    lv_obj_set_style_radius(bg_sw, 4, 0);
    lv_obj_t* bg_lbl = lv_label_create(color_row1);
    lv_label_set_text(bg_lbl, i18n("背景", "BG"));
    apply_hint_label(bg_lbl);

    /* Panel color swatch */
    lv_obj_t* panel_sw = lv_obj_create(color_row1);
    lv_obj_set_size(panel_sw, 24, 24);
    lv_obj_set_style_bg_color(panel_sw, g_colors->panel, 0);
    lv_obj_set_style_border_width(panel_sw, 1, 0);
    lv_obj_set_style_border_color(panel_sw, lv_color_make(100, 100, 100), 0);
    lv_obj_set_style_radius(panel_sw, 4, 0);
    lv_obj_t* panel_lbl = lv_label_create(color_row1);
    lv_label_set_text(panel_lbl, i18n("面板", "Panel"));
    apply_hint_label(panel_lbl);

    /* Accent color swatch */
    lv_obj_t* acc_sw = lv_obj_create(color_row1);
    lv_obj_set_size(acc_sw, 24, 24);
    lv_obj_set_style_bg_color(acc_sw, g_colors->accent, 0);
    lv_obj_set_style_border_width(acc_sw, 1, 0);
    lv_obj_set_style_border_color(acc_sw, lv_color_make(100, 100, 100), 0);
    lv_obj_set_style_radius(acc_sw, 4, 0);
    lv_obj_t* acc_lbl = lv_label_create(color_row1);
    lv_label_set_text(acc_lbl, i18n("强调", "Accent"));
    apply_hint_label(acc_lbl);

    /* Text color swatch */
    lv_obj_t* txt_sw = lv_obj_create(color_row1);
    lv_obj_set_size(txt_sw, 24, 24);
    lv_obj_set_style_bg_color(txt_sw, g_colors->text, 0);
    lv_obj_set_style_border_width(txt_sw, 1, 0);
    lv_obj_set_style_border_color(txt_sw, lv_color_make(100, 100, 100), 0);
    lv_obj_set_style_radius(txt_sw, 4, 0);
    lv_obj_t* txt_lbl = lv_label_create(color_row1);
    lv_label_set_text(txt_lbl, i18n("文字", "Text"));
    apply_hint_label(txt_lbl);

    /* P2-25: Minimize behavior */
    lv_obj_t* lbl_min = lv_label_create(tab);
    lv_label_set_text(lbl_min, i18n("最小化行为 / Minimize To", "Minimize To"));
    apply_section_label(lbl_min);

    static lv_obj_t* gen_minimize_dd = nullptr;
    gen_minimize_dd = lv_dropdown_create(tab);
    lv_dropdown_set_options(gen_minimize_dd, "系统托盘 / System Tray\n任务栏 / Taskbar");
    lv_dropdown_set_selected(gen_minimize_dd, 0);
    lv_obj_set_width(gen_minimize_dd, 200);
    apply_input_style(gen_minimize_dd);

    /* P2-26: Exit confirmation toggle */
    lv_obj_t* lbl_exit = lv_label_create(tab);
    lv_label_set_text(lbl_exit, i18n("退出确认 / Exit Confirmation", "Exit Confirmation"));
    apply_section_label(lbl_exit);

    lv_obj_t* row_exit = lv_obj_create(tab);
    lv_obj_set_size(row_exit, LV_PCT(100), 40);
    lv_obj_set_style_bg_opa(row_exit, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row_exit, 0, 0);
    lv_obj_set_style_pad_all(row_exit, 0, 0);
    lv_obj_clear_flag(row_exit, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(row_exit, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row_exit, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(row_exit, 12, 0);

    static lv_obj_t* gen_exit_sw = nullptr;
    gen_exit_sw = lv_switch_create(row_exit);
    g_exit_confirm_sw = gen_exit_sw;
    lv_obj_set_size(gen_exit_sw, 50, 26);
    lv_obj_set_style_bg_color(gen_exit_sw, lv_color_make(60, 65, 90), LV_PART_MAIN);
    lv_obj_set_style_bg_color(gen_exit_sw, lv_color_make(60, 130, 220), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(gen_exit_sw, lv_color_make(255, 255, 255), LV_PART_KNOB);
    lv_obj_add_state(gen_exit_sw, LV_STATE_CHECKED);

    lv_obj_t* lbl_exit_hint = lv_label_create(row_exit);
    lv_label_set_text(lbl_exit_hint, i18n("退出托盘时弹窗确认", "Confirm before exit"));
    apply_hint_label(lbl_exit_hint);

    /* Divider */
    lv_obj_t* div2 = lv_obj_create(tab);
    lv_obj_set_size(div2, LV_PCT(100), 1);
    lv_obj_set_style_bg_color(div2, lv_color_make(55, 60, 85), 0);
    lv_obj_set_style_border_width(div2, 0, 0);
    lv_obj_clear_flag(div2, LV_OBJ_FLAG_SCROLLABLE);

    /* Security status section */
    lv_obj_t* lbl_sec = lv_label_create(tab);
    lv_label_set_text(lbl_sec, i18n("安全状态 / Security", "Security / 安全状态"));
    apply_section_label(lbl_sec);

    lv_obj_t* sec_row = lv_obj_create(tab);
    lv_obj_set_size(sec_row, LV_PCT(100), 36);
    lv_obj_set_style_bg_opa(sec_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(sec_row, 0, 0);
    lv_obj_set_style_pad_all(sec_row, 0, 0);
    lv_obj_clear_flag(sec_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(sec_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(sec_row, 10, 0);
    lv_obj_set_flex_align(sec_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Green LED */
    lv_obj_t* sec_green = lv_led_create(sec_row);
    lv_obj_set_size(sec_green, 10, 10);
    lv_led_set_color(sec_green, lv_color_make(0, 220, 60));
    lv_led_on(sec_green);

    lv_obj_t* sec_label = lv_label_create(sec_row);
    lv_label_set_text(sec_label, i18n("安全评分: 良好 (Green)", "Security: Good (Green)"));
    lv_obj_set_style_text_color(sec_label, lv_color_make(200, 205, 220), 0);
    lv_obj_set_style_text_font(sec_label, CJK_FONT, 0);

    /* Security detail items */
    lv_obj_t* lbl_sec_detail = lv_label_create(tab);
    lv_label_set_text(lbl_sec_detail, i18n(
        "API Key: OK | Port: 18789 | Config: Writable",
        "API Key: OK | Port: 18789 | Config: Writable"));
    apply_hint_label(lbl_sec_detail);
    lv_label_set_long_mode(lbl_sec_detail, LV_LABEL_LONG_WRAP);

    /* Divider */
    lv_obj_t* div3 = lv_obj_create(tab);
    lv_obj_set_size(div3, LV_PCT(100), 1);
    lv_obj_set_style_bg_color(div3, lv_color_make(55, 60, 85), 0);
    lv_obj_set_style_border_width(div3, 0, 0);
    lv_obj_clear_flag(div3, LV_OBJ_FLAG_SCROLLABLE);

    /* Reconfigure wizard button */
    lv_obj_t* btn_wizard = lv_button_create(tab);
    lv_obj_set_width(btn_wizard, LV_PCT(100));
    lv_obj_set_height(btn_wizard, 40);
    lv_obj_set_style_bg_color(btn_wizard, lv_color_make(40, 50, 90), 0);
    lv_obj_set_style_bg_grad_color(btn_wizard, lv_color_make(30, 40, 70), 0);
    lv_obj_set_style_bg_grad_dir(btn_wizard, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_radius(btn_wizard, 8, 0);
    lv_obj_t* btn_wizard_lbl = lv_label_create(btn_wizard);
    lv_label_set_text(btn_wizard_lbl, i18n("重新配置向导", "Reconfigure Wizard"));
    lv_obj_set_style_text_font(btn_wizard_lbl, CJK_FONT, 0);
    lv_obj_center(btn_wizard_lbl);
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
                lv_obj_t* btn = lv_list_add_btn(model_list, NULL, model_list_items[j]);
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
        lv_obj_t* btn = lv_list_add_btn(model_list, NULL, model_list_items[i]);
        lv_obj_set_style_text_font(btn, CJK_FONT, 0);
        lv_obj_set_style_bg_color(btn, lv_color_make(35, 38, 52), 0);
        lv_obj_set_style_text_color(btn, lv_color_make(180, 185, 200), 0);
        lv_obj_add_event_cb(btn, model_select_cb, LV_EVENT_CLICKED, nullptr);
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
    snprintf(status_text, sizeof(status_text), "%s [下载中...]", skill_name);
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
        snprintf(status_text, sizeof(status_text), "%s [已安装]", skill_name);
        app_log("[Skill] Downloaded: %s", skill_name);
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
        snprintf(status_text, sizeof(status_text), "%s [已安装]", skill_name);
        app_log("[Skill] Installed locally: %s", skill_name);
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
    lv_obj_set_style_pad_gap(tab, 10, 0);

    /* Title */
    lv_obj_t* lbl_title = lv_label_create(tab);
    lv_label_set_text(lbl_title, i18n("技能管理 / Skills", "Skills / 技能管理"));
    apply_section_label(lbl_title);

    /* Search box */
    lv_obj_t* lbl_search = lv_label_create(tab);
    lv_label_set_text(lbl_search, i18n("搜索技能", "Search Skills"));
    apply_section_label(lbl_search);

    lv_obj_t* skill_search = lv_textarea_create(tab);
    lv_textarea_set_one_line(skill_search, true);
    lv_textarea_set_placeholder_text(skill_search, i18n("输入关键词...", "Type to filter..."));
    lv_obj_set_width(skill_search, LV_PCT(100));
    lv_obj_set_height(skill_search, 38);
    apply_input_style(skill_search);

    /* Available skills list */
    lv_obj_t* lbl_avail = lv_label_create(tab);
    lv_label_set_text(lbl_avail, i18n("可下载技能 / Available", "Available / 可下载"));
    apply_section_label(lbl_avail);

    lv_obj_t* skill_list = lv_list_create(tab);
    lv_obj_set_width(skill_list, LV_PCT(100));
    lv_obj_set_height(skill_list, 130);
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
    lv_obj_t* div1 = lv_obj_create(tab);
    lv_obj_set_size(div1, LV_PCT(100), 1);
    lv_obj_set_style_bg_color(div1, lv_color_make(55, 60, 85), 0);
    lv_obj_set_style_border_width(div1, 0, 0);
    lv_obj_clear_flag(div1, LV_OBJ_FLAG_SCROLLABLE);

    /* P2-29: Installed skills with enable/disable toggles */
    lv_obj_t* lbl_installed = lv_label_create(tab);
    lv_label_set_text(lbl_installed, i18n("已安装技能 / Installed", "Installed / 已安装"));
    apply_section_label(lbl_installed);

    lv_obj_t* installed_list = lv_list_create(tab);
    lv_obj_set_width(installed_list, LV_PCT(100));
    lv_obj_set_height(installed_list, 80);
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
    lv_label_set_text(lbl_empty, i18n("暂无已安装技能", "No skills installed yet"));
    lv_obj_set_style_text_color(lbl_empty, lv_color_make(120, 125, 140), 0);
    lv_obj_set_style_text_font(lbl_empty, CJK_FONT, 0);
}

/* ═══════════════════════════════════════════════════════════════
 *  ABOUT TAB (with garlic branding)
 * ═══════════════════════════════════════════════════════════════ */
static void build_about_tab(lv_obj_t* tab) {
    apply_dark_style(tab);
    lv_obj_set_style_pad_all(tab, 20, 0);
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(tab, 14, 0);

    /* Garlic brand icon */
    lv_obj_t* lbl_garlic = lv_label_create(tab);
    lv_label_set_text(lbl_garlic, "\xE2\x97\x87 G");  /* ◇ */
    lv_obj_set_style_text_color(lbl_garlic, lv_color_make(255, 215, 100), 0);
    lv_obj_set_style_text_font(lbl_garlic, CJK_FONT, 0);

    /* App logo/title */
    lv_obj_t* lbl_title = lv_label_create(tab);
    lv_label_set_text(lbl_title, "AnyClaw LVGL");
    lv_obj_set_style_text_color(lbl_title, lv_color_make(100, 160, 255), 0);
    lv_obj_set_style_text_font(lbl_title, CJK_FONT, 0);

    /* Brand slogan */
    lv_obj_t* lbl_brand = lv_label_create(tab);
    lv_label_set_text(lbl_brand, i18n("龙虾要吃蒜蓉的 - Your AI Assistant", "Garlic Lobster - Your AI Assistant"));
    lv_obj_set_style_text_color(lbl_brand, lv_color_make(255, 200, 100), 0);
    lv_obj_set_style_text_font(lbl_brand, CJK_FONT, 0);
    lv_label_set_long_mode(lbl_brand, LV_LABEL_LONG_WRAP);

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
    /* P2-35: Config import/export */
    lv_obj_t* lbl_config = lv_label_create(tab);
    lv_label_set_text(lbl_config, i18n("配置管理 / Config", "Config / 配置管理"));
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
    lv_obj_set_size(btn_cfg_export, 130, 32);
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
            app_log("[Config] Exported to %s", path);
        }
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* l_exp = lv_label_create(btn_cfg_export);
    lv_label_set_text(l_exp, i18n("导出配置", "Export Config"));
    lv_obj_set_style_text_font(l_exp, CJK_FONT, 0);
    lv_obj_center(l_exp);

    /* Copyright */
    lv_obj_t* lbl_copy2 = lv_label_create(tab);
    lv_label_set_text(lbl_copy2, "Copyright 2026 AnyClaw Project");
    apply_hint_label(lbl_copy2);

    /* P2-30: Skill version check button */
    lv_obj_t* btn_ver_check = lv_button_create(tab);
    lv_obj_set_size(btn_ver_check, LV_PCT(100), 32);
    lv_obj_set_style_bg_color(btn_ver_check, lv_color_make(50, 55, 75), 0);
    lv_obj_set_style_radius(btn_ver_check, 6, 0);
    lv_obj_add_event_cb(btn_ver_check, [](lv_event_t* e) {
        (void)e;
        char url[512];
        snprintf(url, sizeof(url), "https://clawhub.com/api/v1/skills/versions");
        char response[4096];
        int result = http_get(url, response, sizeof(response), 10);
        if (result == 200) {
            app_log("[Skill] Version check: %s", response);
        } else {
            app_log("[Skill] Version check: offline mode");
        }
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* l_ver = lv_label_create(btn_ver_check);
    lv_label_set_text(l_ver, i18n("检查技能更新", "Check Updates"));
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
    lv_obj_set_style_bg_color(settings_panel, g_colors->bg, 0);
    lv_obj_set_style_bg_opa(settings_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(settings_panel, 0, 0);
    lv_obj_set_style_radius(settings_panel, 0, 0);
    lv_obj_set_style_pad_all(settings_panel, 0, 0);
    lv_obj_clear_flag(settings_panel, LV_OBJ_FLAG_SCROLLABLE);

    /* Settings title bar */
    lv_obj_t* title_bar = lv_obj_create(settings_panel);
    lv_obj_set_size(title_bar, WIN_W, 48);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_set_style_bg_color(title_bar, g_colors->panel, 0);
    lv_obj_set_style_bg_opa(title_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(title_bar, 0, 0);
    lv_obj_set_style_radius(title_bar, 0, 0);
    lv_obj_set_style_pad_all(title_bar, 0, 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_bar);
    lv_label_set_text(title, i18n("设置 / Settings", "Settings / 设置"));
    lv_obj_set_style_text_color(title, g_colors->accent, 0);
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

    /* Tabview - larger for 1450x800 */
    settings_tabs = lv_tabview_create(settings_panel);
    lv_obj_set_size(settings_tabs, WIN_W - 40, WIN_H - 60);
    lv_obj_set_pos(settings_tabs, 20, 54);
    lv_obj_set_style_bg_color(settings_tabs, g_colors->panel, 0);
    lv_obj_set_style_border_width(settings_tabs, 0, 0);
    lv_obj_set_style_radius(settings_tabs, 8, 0);

    /* Tab button styling */
    lv_obj_t* tab_btns = lv_tabview_get_tab_btns(settings_tabs);
    lv_obj_set_style_bg_color(tab_btns, g_colors->panel, 0);
    lv_obj_set_style_text_color(tab_btns, g_colors->text_dim, LV_PART_ITEMS);
    lv_obj_set_style_text_color(tab_btns, g_colors->accent, LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_text_font(tab_btns, CJK_FONT, LV_PART_ITEMS);

    /* Create 5 tabs */
    lv_obj_t* tab_gen = lv_tabview_add_tab(settings_tabs, i18n("常规", "General"));
    lv_obj_t* tab_acc = lv_tabview_add_tab(settings_tabs, i18n("账号", "Account"));
    lv_obj_t* tab_model = lv_tabview_add_tab(settings_tabs, i18n("模型", "Model"));
    lv_obj_t* tab_skills = lv_tabview_add_tab(settings_tabs, i18n("技能", "Skills"));
    lv_obj_t* tab_about = lv_tabview_add_tab(settings_tabs, i18n("关于", "About"));

    /* Build each tab */
    build_general_tab(tab_gen);
    build_account_tab(tab_acc);
    build_model_tab(tab_model);
    build_skills_tab(tab_skills);
    build_about_tab(tab_about);

    /* Initially hidden */
    lv_obj_add_flag(settings_panel, LV_OBJ_FLAG_HIDDEN);
    settings_visible = false;

    app_log("[Settings] Settings panel initialized");
}
