#include "app.h"
#include "SDL.h"
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <string>
#include <windows.h>

extern const lv_font_t lv_font_mshy_16;
extern const lv_font_t lv_font_source_han_sans_sc_16_cjk;
#define CJK_FONT (&lv_font_source_han_sans_sc_16_cjk)

/* Layout constants for 1200x800 window */
#define WIN_W 1200
#define WIN_H 800
#define TITLE_H 48
#define LEFT_PANEL_W 520
#define RIGHT_PANEL_W (WIN_W - LEFT_PANEL_W - 30)
#define PANEL_TOP (TITLE_H + 8)
#define PANEL_H 450
#define FOOTER_H 30

/* Language mode: 0=CN only, 1=EN only, 2=Bilingual */
enum class Lang { CN, EN, BOTH };
static Lang g_lang = Lang::BOTH;  /* Default bilingual */

/* I18n string pair */
struct I18n { const char* en; const char* cn; };

/* Get translated string based on current language */
static const char* tr(const I18n& s) {
    switch (g_lang) {
        case Lang::CN:  return s.cn;
        case Lang::EN:  return s.en;
        case Lang::BOTH: {
            /* Return bilingual string - use static buffer */
            static char buf[512];
            snprintf(buf, sizeof(buf), "%s / %s", s.en, s.cn);
            return buf;
        }
    }
    return s.en;
}

/* Title uses special bilingual format */
static const char* tr_title(const I18n& s) {
    switch (g_lang) {
        case Lang::CN:  return s.cn;
        case Lang::EN:  return s.en;
        case Lang::BOTH: {
            static char buf[512];
            snprintf(buf, sizeof(buf), "[AnyClaw] %s / %s", s.en, s.cn);
            return buf;
        }
    }
    return s.en;
}

/* ── I18n Strings ── */
static const I18n STR_TITLE       = {"Desktop Manager", "桌面管理器"};
static const I18n STR_STATUS      = {"OpenClaw Status", "OpenClaw 状态"};
static const I18n STR_CONTROLS    = {"Controls", "操作面板"};
static const I18n STR_LOG         = {"Log", "日志"};
static const I18n STR_START       = {LV_SYMBOL_PLAY " Start", LV_SYMBOL_PLAY " 启动"};
static const I18n STR_STOP        = {LV_SYMBOL_STOP " Stop", LV_SYMBOL_STOP " 停止"};
static const I18n STR_REFRESH     = {LV_SYMBOL_REFRESH " Refresh", LV_SYMBOL_REFRESH " 刷新状态"};
static const I18n STR_SETTINGS    = {LV_SYMBOL_SETTINGS " Settings", LV_SYMBOL_SETTINGS " 设置"};
static const I18n STR_VERSION     = {"Version", "版本"};
static const I18n STR_PATH        = {"Path", "路径"};
static const I18n STR_PORT        = {"Port", "端口"};
static const I18n STR_FOOTER      = {"AnyClaw v2.0 | LVGL 9.6 + SDL2", "AnyClaw v2.0 | LVGL 9.6 + SDL2"};
static const I18n STR_AUTOREFRESH = {"Auto-refresh: 30s | Drag title bar to move", "自动刷新: 30s | 拖动标题栏移动窗口"};
static const I18n STR_CHECKING    = {"Checking...", "检查中..."};
static const I18n STR_NOTFOUND    = {"Not Found", "未找到"};
static const I18n STR_NOTINST     = {"Not Installed", "未安装"};
static const I18n STR_IDLE        = {"Idle", "空闲"};
static const I18n STR_RUNNING     = {"Running", "运行中"};
static const I18n STR_ERROR       = {"Error", "错误"};
static const I18n STR_UNKNOWN     = {"Unknown", "未知"};
static const I18n STR_TASK_LIST   = {LV_SYMBOL_LIST " Task List", LV_SYMBOL_LIST " 任务列表"};
static const I18n STR_CHAT        = {LV_SYMBOL_ENVELOPE " Chat", LV_SYMBOL_ENVELOPE " 聊天"};
static const I18n STR_WIFI        = {LV_SYMBOL_WIFI " Connected", LV_SYMBOL_WIFI " 已连接"};
static const I18n STR_BATTERY     = {LV_SYMBOL_BATTERY_FULL " Power", LV_SYMBOL_BATTERY_FULL " 电源"};

/* ── UI widgets ── */
static lv_obj_t* status_label = nullptr;
static lv_obj_t* version_label = nullptr;
static lv_obj_t* path_label = nullptr;
static lv_obj_t* port_label = nullptr;
static lv_obj_t* btn_start = nullptr;
static lv_obj_t* btn_stop = nullptr;
static lv_obj_t* btn_refresh = nullptr;
static lv_obj_t* btn_settings = nullptr;
static lv_obj_t* log_panel = nullptr;
static lv_obj_t* led_ok = nullptr;
static lv_obj_t* led_warn = nullptr;
static lv_obj_t* title_bar = nullptr;
static lv_obj_t* title_label = nullptr;

/* ── Log system ── */
#define LOG_MAX_LINES  50
#define LOG_LINE_LEN   256
static char log_lines[LOG_MAX_LINES][LOG_LINE_LEN];
static int  log_count = 0;
static int  log_scroll_user = -1;

static void log_refresh_display() {
    if (!log_panel) return;
    char full[LOG_MAX_LINES * LOG_LINE_LEN] = {0};
    int offset = 0;
    for (int i = 0; i < log_count && offset < (int)sizeof(full) - 2; i++) {
        int n = snprintf(full + offset, sizeof(full) - offset, "%s\n", log_lines[i]);
        if (n > 0) offset += n;
    }
    lv_obj_t* lbl = lv_obj_get_child(log_panel, 0);
    if (lbl) lv_label_set_text(lbl, full);
    if (log_scroll_user < 0 && lbl) {
        lv_obj_update_layout(log_panel);
        int lbl_h = (int)lv_obj_get_height(lbl);
        int panel_h = (int)lv_obj_get_height(log_panel);
        if (lbl_h > panel_h) lv_obj_scroll_to_y(log_panel, lbl_h - panel_h, LV_ANIM_OFF);
    }
}

void app_log(const char* fmt, ...) {
    char buf[LOG_LINE_LEN];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    char timed[LOG_LINE_LEN + 32];
    SYSTEMTIME st;
    GetLocalTime(&st);
    snprintf(timed, sizeof(timed), "[%02d:%02d:%02d] %s", st.wHour, st.wMinute, st.wSecond, buf);
    if (log_count >= LOG_MAX_LINES) {
        for (int i = 0; i < LOG_MAX_LINES - 1; i++) memcpy(log_lines[i], log_lines[i + 1], LOG_LINE_LEN);
        log_count = LOG_MAX_LINES - 1;
    }
    strncpy(log_lines[log_count], timed, LOG_LINE_LEN - 1);
    log_lines[log_count][LOG_LINE_LEN - 1] = 0;
    log_count++;
    log_refresh_display();
}

static void update_ui_language();

/* ── Window drag ── */
static void title_drag_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    static lv_point_t drag_start = {0, 0};
    static bool dragging = false;
    if (code == LV_EVENT_PRESSING) {
        lv_indev_t* indev = lv_indev_get_act();
        if (!indev) return;
        lv_point_t p;
        lv_indev_get_point(indev, &p);
        if (!dragging) { drag_start = p; dragging = true; }
        SDL_Window* win = app_get_window();
        if (win) {
            int wx, wy;
            SDL_GetWindowPosition(win, &wx, &wy);
            SDL_SetWindowPosition(win, wx + (p.x - drag_start.x), wy + (p.y - drag_start.y));
            drag_start = p;
        }
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_LEAVE) {
        dragging = false;
    }
}

static const char* status_text(ClawStatus s) {
    switch (s) {
        case ClawStatus::NotInstalled: return tr(STR_NOTINST);
        case ClawStatus::Detected:     return tr(STR_IDLE);
        case ClawStatus::Running:      return tr(STR_RUNNING);
        case ClawStatus::Error:        return tr(STR_ERROR);
        default:                       return tr(STR_UNKNOWN);
    }
}

static lv_color_t status_color(ClawStatus s) {
    switch (s) {
        case ClawStatus::Running:  return lv_color_make(0, 220, 60);
        case ClawStatus::Error:    return lv_color_make(220, 40, 40);
        default:                   return lv_color_make(220, 200, 40);
    }
}

static void auto_refresh_cb(lv_timer_t* timer) {
    (void)timer;
    app_log("[Timer] Auto-refreshing...");
    app_refresh_status();
}

static void btn_settings_cb(lv_event_t* e) { (void)e; ui_settings_open(); }
static void btn_start_cb(lv_event_t* e) {
    (void)e;
    app_log(tr({"[Op] Starting Gateway...", "[操作] 正在启动 Gateway..."}));
    if (app_start_gateway()) app_log("[OK] Gateway start command sent");
    else app_log("[Fail] Gateway start failed");
}
static void btn_stop_cb(lv_event_t* e) {
    (void)e;
    app_log(tr({"[Op] Stopping Gateway...", "[操作] 正在停止 Gateway..."}));
    if (app_stop_gateway()) { app_log("[OK] Gateway stopped"); app_refresh_status(); }
    else app_log("[Fail] Gateway stop failed");
}
static void btn_refresh_cb(lv_event_t* e) { (void)e; app_log(tr({"[Refresh] Checking...", "[刷新] 正在检测..."})); app_refresh_status(); }

static void lang_cn_cb(lv_event_t* e) {
    (void)e;
    if (g_lang != Lang::CN) { g_lang = Lang::CN; update_ui_language(); app_log("[Lang] Chinese mode"); }
}
static void lang_en_cb(lv_event_t* e) {
    (void)e;
    if (g_lang != Lang::EN) { g_lang = Lang::EN; update_ui_language(); app_log("[Lang] English mode"); }
}
static void lang_both_cb(lv_event_t* e) {
    (void)e;
    if (g_lang != Lang::BOTH) { g_lang = Lang::BOTH; update_ui_language(); app_log("[Lang] Bilingual mode"); }
}

void app_refresh_status() {
    OpenClawInfo info = app_detect_openclaw();
    ClawStatus status = app_check_status(info);
    static char ver_buf[512], path_buf[512], port_buf[64];

    lv_label_set_text(status_label, status_text(status));
    lv_obj_set_style_text_color(status_label, status_color(status), 0);
    snprintf(ver_buf, sizeof(ver_buf), "%s: %s", tr(STR_VERSION), info.version[0] ? info.version : tr(STR_NOTFOUND));
    lv_label_set_text(version_label, ver_buf);
    if (info.executable[0]) snprintf(path_buf, sizeof(path_buf), "%s: %s", tr(STR_PATH), info.executable);
    else snprintf(path_buf, sizeof(path_buf), "%s: %s", tr(STR_PATH), tr(STR_NOTFOUND));
    lv_label_set_text(path_label, path_buf);
    snprintf(port_buf, sizeof(port_buf), "%s: %d", tr(STR_PORT), info.gateway_port);
    lv_label_set_text(port_label, port_buf);
    if (status == ClawStatus::Running) { lv_led_on(led_ok); lv_led_off(led_warn); }
    else if (status == ClawStatus::Error) { lv_led_off(led_ok); lv_led_on(led_warn); }
    else { lv_led_off(led_ok); lv_led_off(led_warn); }

    lv_obj_invalidate(lv_screen_active());
}

static void update_ui_language() {
    lv_label_set_text(title_label, tr_title(STR_TITLE));
    lv_obj_t* lbl;
    lbl = lv_obj_get_child(btn_start, 0); if (lbl) lv_label_set_text(lbl, tr(STR_START));
    lbl = lv_obj_get_child(btn_stop, 0); if (lbl) lv_label_set_text(lbl, tr(STR_STOP));
    lbl = lv_obj_get_child(btn_refresh, 0); if (lbl) lv_label_set_text(lbl, tr(STR_REFRESH));
    lbl = lv_obj_get_child(btn_settings, 0); if (lbl) lv_label_set_text(lbl, tr(STR_SETTINGS));
    app_refresh_status();
}

/* ── Helper: create styled label ── */
static lv_obj_t* create_styled_label(lv_obj_t* parent, const char* text, lv_color_t color, int x, int y, int w) {
    lv_obj_t* lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, color, 0);
    lv_obj_set_style_text_font(lbl, CJK_FONT, 0);
    if (w > 0) {
        lv_obj_set_width(lbl, w);
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_MODE_DOTS);
    }
    lv_obj_set_pos(lbl, x, y);
    return lbl;
}

/* ── Helper: create button ── */
static lv_obj_t* create_styled_button(lv_obj_t* parent, const char* text,
    int x, int y, int w, int h, lv_color_t bg, lv_color_t shadow,
    lv_event_cb_t cb) {
    lv_obj_t* btn = lv_button_create(parent);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_style_bg_color(btn, bg, 0);
    lv_obj_set_style_radius(btn, 10, 0);
    lv_obj_set_style_shadow_width(btn, 4, 0);
    lv_obj_set_style_shadow_color(btn, shadow, 0);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, CJK_FONT, 0);
    lv_obj_center(lbl);
    return btn;
}

void app_ui_init() {
    lv_obj_t* scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_make(30, 30, 40), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    /* ═══ TITLE BAR ═══ */
    title_bar = lv_obj_create(scr);
    lv_obj_set_size(title_bar, WIN_W, TITLE_H);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_set_style_bg_color(title_bar, lv_color_make(35, 35, 50), 0);
    lv_obj_set_style_bg_opa(title_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(title_bar, 0, 0);
    lv_obj_set_style_radius(title_bar, 0, 0);
    lv_obj_set_style_pad_all(title_bar, 0, 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(title_bar, title_drag_cb, LV_EVENT_ALL, nullptr);

    title_label = lv_label_create(title_bar);
    lv_label_set_text(title_label, tr_title(STR_TITLE));
    lv_obj_set_style_text_color(title_label, lv_color_make(100, 160, 255), 0);
    lv_obj_set_style_text_font(title_label, CJK_FONT, 0);
    lv_obj_align(title_label, LV_ALIGN_LEFT_MID, 15, 0);

    /* Language buttons on title bar */
    lv_obj_t* btn_lang_cn = lv_button_create(title_bar);
    lv_obj_set_size(btn_lang_cn, 36, 30);
    lv_obj_align(btn_lang_cn, LV_ALIGN_RIGHT_MID, -180, 0);
    lv_obj_set_style_bg_color(btn_lang_cn, lv_color_make(50, 60, 90), 0);
    lv_obj_set_style_radius(btn_lang_cn, 6, 0);
    lv_obj_add_event_cb(btn_lang_cn, lang_cn_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* lcn = lv_label_create(btn_lang_cn);
    lv_label_set_text(lcn, "CN");
    lv_obj_set_style_text_font(lcn, &lv_font_montserrat_12, 0);
    lv_obj_center(lcn);

    lv_obj_t* btn_lang_both = lv_button_create(title_bar);
    lv_obj_set_size(btn_lang_both, 46, 30);
    lv_obj_align(btn_lang_both, LV_ALIGN_RIGHT_MID, -138, 0);
    lv_obj_set_style_bg_color(btn_lang_both, lv_color_make(60, 80, 140), 0);
    lv_obj_set_style_radius(btn_lang_both, 6, 0);
    lv_obj_add_event_cb(btn_lang_both, lang_both_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* lboth = lv_label_create(btn_lang_both);
    lv_label_set_text(lboth, "C+E");
    lv_obj_set_style_text_font(lboth, &lv_font_montserrat_12, 0);
    lv_obj_center(lboth);

    lv_obj_t* btn_lang_en = lv_button_create(title_bar);
    lv_obj_set_size(btn_lang_en, 36, 30);
    lv_obj_align(btn_lang_en, LV_ALIGN_RIGHT_MID, -96, 0);
    lv_obj_set_style_bg_color(btn_lang_en, lv_color_make(50, 60, 90), 0);
    lv_obj_set_style_radius(btn_lang_en, 6, 0);
    lv_obj_add_event_cb(btn_lang_en, lang_en_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* len = lv_label_create(btn_lang_en);
    lv_label_set_text(len, "EN");
    lv_obj_set_style_text_font(len, &lv_font_montserrat_12, 0);
    lv_obj_center(len);

    /* Settings button */
    btn_settings = lv_button_create(title_bar);
    lv_obj_set_size(btn_settings, 80, 32);
    lv_obj_align(btn_settings, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_bg_color(btn_settings, lv_color_make(60, 70, 110), 0);
    lv_obj_set_style_bg_grad_color(btn_settings, lv_color_make(45, 55, 90), 0);
    lv_obj_set_style_bg_grad_dir(btn_settings, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_radius(btn_settings, 8, 0);
    lv_obj_add_event_cb(btn_settings, btn_settings_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* lset = lv_label_create(btn_settings);
    lv_label_set_text(lset, tr(STR_SETTINGS));
    lv_obj_set_style_text_font(lset, CJK_FONT, 0);
    lv_obj_center(lset);

    /* ═══ DIVIDER ═══ */
    lv_obj_t* div1 = lv_obj_create(scr);
    lv_obj_set_size(div1, WIN_W, 1);
    lv_obj_set_pos(div1, 0, TITLE_H);
    lv_obj_set_style_bg_color(div1, lv_color_make(60, 70, 100), 0);
    lv_obj_set_style_bg_opa(div1, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(div1, 0, 0);
    lv_obj_set_style_pad_all(div1, 0, 0);
    lv_obj_clear_flag(div1, LV_OBJ_FLAG_SCROLLABLE);

    /* ═══ LEFT PANEL: Status ═══ */
    lv_obj_t* pl = lv_obj_create(scr);
    lv_obj_set_size(pl, LEFT_PANEL_W, PANEL_H);
    lv_obj_set_pos(pl, 10, PANEL_TOP);
    lv_obj_set_style_bg_color(pl, lv_color_make(35, 38, 52), 0);
    lv_obj_set_style_bg_opa(pl, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(pl, 1, 0);
    lv_obj_set_style_border_color(pl, lv_color_make(55, 60, 85), 0);
    lv_obj_set_style_radius(pl, 12, 0);
    lv_obj_set_style_pad_all(pl, 12, 0);
    lv_obj_clear_flag(pl, LV_OBJ_FLAG_SCROLLABLE);

    /* Panel title */
    lv_obj_t* pt = lv_label_create(pl);
    lv_label_set_text(pt, tr(STR_STATUS));
    lv_obj_set_style_text_color(pt, lv_color_make(130, 170, 240), 0);
    lv_obj_set_style_text_font(pt, CJK_FONT, 0);
    lv_obj_set_pos(pt, 8, 8);

    /* Status row with LEDs */
    lv_obj_t* sr = lv_obj_create(pl);
    lv_obj_set_size(sr, LEFT_PANEL_W - 24, 34);
    lv_obj_set_pos(sr, 12, 42);
    lv_obj_set_style_bg_opa(sr, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(sr, 0, 0);
    lv_obj_set_style_pad_all(sr, 0, 0);
    lv_obj_clear_flag(sr, LV_OBJ_FLAG_SCROLLABLE);

    led_ok = lv_led_create(sr);
    lv_obj_align(led_ok, LV_ALIGN_LEFT_MID, 8, 0);
    lv_obj_set_size(led_ok, 10, 10);
    lv_led_set_color(led_ok, lv_color_make(0, 220, 60));
    lv_led_off(led_ok);

    led_warn = lv_led_create(sr);
    lv_obj_align(led_warn, LV_ALIGN_LEFT_MID, 30, 0);
    lv_obj_set_size(led_warn, 10, 10);
    lv_led_set_color(led_warn, lv_color_make(220, 40, 40));
    lv_led_off(led_warn);

    status_label = lv_label_create(sr);
    lv_label_set_text(status_label, tr(STR_CHECKING));
    lv_obj_set_style_text_color(status_label, lv_color_make(220, 220, 230), 0);
    lv_obj_set_style_text_font(status_label, CJK_FONT, 0);
    lv_obj_align(status_label, LV_ALIGN_LEFT_MID, 55, 0);

    /* Version, Path, Port labels */
    version_label = create_styled_label(pl, "Version: --", lv_color_make(160, 165, 185), 15, 85, LEFT_PANEL_W - 30);
    path_label = create_styled_label(pl, "Path: --", lv_color_make(160, 165, 185), 15, 120, LEFT_PANEL_W - 30);
    port_label = create_styled_label(pl, "Port: --", lv_color_make(160, 165, 185), 15, 155, LEFT_PANEL_W - 30);

    /* Separator */
    lv_obj_t* sep1 = lv_obj_create(pl);
    lv_obj_set_size(sep1, LEFT_PANEL_W - 30, 1);
    lv_obj_set_pos(sep1, 15, 190);
    lv_obj_set_style_bg_color(sep1, lv_color_make(55, 60, 85), 0);
    lv_obj_set_style_bg_opa(sep1, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sep1, 0, 0);
    lv_obj_set_style_pad_all(sep1, 0, 0);
    lv_obj_clear_flag(sep1, LV_OBJ_FLAG_SCROLLABLE);

    /* Task list area title with icon */
    lv_obj_t* task_title = create_styled_label(pl, tr(STR_TASK_LIST), lv_color_make(130, 170, 240), 15, 200, 200);

    /* Task placeholder items */
    lv_obj_t* task1 = create_styled_label(pl, tr({"  [Running] Gateway Service", "  [运行中] Gateway 服务"}), lv_color_make(0, 220, 60), 15, 230, LEFT_PANEL_W - 30);
    lv_obj_t* task2 = create_styled_label(pl, tr({"  [Idle] No tasks pending", "  [空闲] 暂无待处理任务"}), lv_color_make(220, 200, 40), 15, 255, LEFT_PANEL_W - 30);

    /* Hint at bottom */
    lv_obj_t* hint = create_styled_label(pl, tr(STR_AUTOREFRESH), lv_color_make(90, 95, 120), 15, PANEL_H - 60, LEFT_PANEL_W - 30);

    /* ═══ RIGHT PANEL: Controls ═══ */
    lv_obj_t* pr = lv_obj_create(scr);
    lv_obj_set_size(pr, RIGHT_PANEL_W, PANEL_H);
    lv_obj_set_pos(pr, LEFT_PANEL_W + 20, PANEL_TOP);
    lv_obj_set_style_bg_color(pr, lv_color_make(38, 42, 58), 0);
    lv_obj_set_style_bg_opa(pr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(pr, 1, 0);
    lv_obj_set_style_border_color(pr, lv_color_make(55, 65, 95), 0);
    lv_obj_set_style_radius(pr, 12, 0);
    lv_obj_set_style_pad_all(pr, 12, 0);
    lv_obj_clear_flag(pr, LV_OBJ_FLAG_SCROLLABLE);

    /* Panel title */
    lv_obj_t* rt = lv_label_create(pr);
    lv_label_set_text(rt, tr(STR_CONTROLS));
    lv_obj_set_style_text_color(rt, lv_color_make(130, 170, 240), 0);
    lv_obj_set_style_text_font(rt, CJK_FONT, 0);
    lv_obj_set_pos(rt, 8, 8);

    /* Buttons - 3 in a row */
    int btn_w = 160;
    int btn_h = 42;
    int btn_y = 45;
    int btn_gap = 15;

    btn_start = create_styled_button(pr, tr(STR_START), 10, btn_y, btn_w, btn_h,
        lv_color_make(25, 140, 60), lv_color_make(0, 100, 40), btn_start_cb);

    btn_stop = create_styled_button(pr, tr(STR_STOP), 10 + btn_w + btn_gap, btn_y, btn_w, btn_h,
        lv_color_make(160, 40, 40), lv_color_make(100, 20, 20), btn_stop_cb);

    btn_refresh = create_styled_button(pr, tr(STR_REFRESH), 10 + (btn_w + btn_gap) * 2, btn_y, btn_w, btn_h,
        lv_color_make(40, 80, 160), lv_color_make(25, 50, 120), btn_refresh_cb);

    /* Chat bubble area title with icon */
    lv_obj_t* chat_title = create_styled_label(pr, tr(STR_CHAT), lv_color_make(130, 170, 240), 8, 95, 200);

    /* Log area title with icon */
    lv_obj_t* log_title = create_styled_label(pr, tr({LV_SYMBOL_FILE " Log", LV_SYMBOL_FILE " 日志"}), lv_color_make(130, 170, 240), 8, 115, 200);

    /* Log panel */
    int log_y = 140;
    int log_h = PANEL_H - log_y - 15;
    log_panel = lv_obj_create(pr);
    lv_obj_set_size(log_panel, RIGHT_PANEL_W - 24, log_h);
    lv_obj_set_pos(log_panel, 10, log_y);
    lv_obj_set_style_bg_color(log_panel, lv_color_make(20, 20, 28), 0);
    lv_obj_set_style_bg_opa(log_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(log_panel, 1, 0);
    lv_obj_set_style_border_color(log_panel, lv_color_make(50, 55, 75), 0);
    lv_obj_set_style_radius(log_panel, 6, 0);
    lv_obj_set_style_pad_all(log_panel, 6, 0);
    lv_obj_set_flex_flow(log_panel, LV_FLEX_FLOW_COLUMN);

    lv_obj_t* log_lbl = lv_label_create(log_panel);
    lv_label_set_text(log_lbl, "");
    lv_obj_set_style_text_color(log_lbl, lv_color_make(120, 255, 120), 0);
    lv_obj_set_style_text_font(log_lbl, &lv_font_montserrat_10, 0);
    lv_label_set_long_mode(log_lbl, LV_LABEL_LONG_WRAP);

    /* ═══ FOOTER ═══ */
    lv_obj_t* footer = lv_label_create(scr);
    lv_label_set_text(footer, tr(STR_FOOTER));
    lv_obj_set_style_text_color(footer, lv_color_make(60, 65, 85), 0);
    lv_obj_set_style_text_font(footer, &lv_font_montserrat_10, 0);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -8);

    /* ═══ Settings UI ═══ */
    ui_settings_init(scr);

    /* Initial refresh */
    app_refresh_status();

    /* Auto-refresh timer (30s) */
    lv_timer_create(auto_refresh_cb, 30000, nullptr);

    app_log("[Ready] AnyClaw LVGL v2.0 - Bilingual mode");
}
