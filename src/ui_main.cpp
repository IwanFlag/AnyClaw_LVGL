#include "app.h"
#include <cstdio>
#include <cstring>

extern const lv_font_t lv_font_mshy_16;
#define CJK_FONT (&lv_font_mshy_16)

static lv_obj_t* status_label = nullptr;
static lv_obj_t* version_label = nullptr;
static lv_obj_t* path_label = nullptr;
static lv_obj_t* port_label = nullptr;
static lv_obj_t* btn_start = nullptr;
static lv_obj_t* btn_stop = nullptr;
static lv_obj_t* btn_refresh = nullptr;
static lv_obj_t* log_label = nullptr;
static lv_obj_t* led_ok = nullptr;
static lv_obj_t* led_warn = nullptr;

static OpenClawInfo g_info;
static ClawStatus g_status = ClawStatus::NotInstalled;

static void log_msg(const char* msg) {
    if (log_label) lv_label_set_text(log_label, msg);
}

static void btn_start_cb(lv_event_t* e) {
    (void)e;
    log_msg("[操作] 正在启动 Gateway...");
    if (app_start_gateway()) log_msg("[成功] Gateway 启动命令已发送");
    else log_msg("[失败] Gateway 启动失败");
}

static void btn_stop_cb(lv_event_t* e) {
    (void)e;
    log_msg("[操作] 正在停止 Gateway...");
    if (app_stop_gateway()) { log_msg("[成功] Gateway 已停止"); app_refresh_status(); }
    else log_msg("[失败] Gateway 停止失败");
}

static void btn_refresh_cb(lv_event_t* e) {
    (void)e;
    log_msg("[刷新] 正在检测...");
    app_refresh_status();
}

static const char* status_text(ClawStatus s) {
    switch (s) {
        case ClawStatus::NotInstalled: return "未安装";
        case ClawStatus::Detected:     return "已安装(未运行)";
        case ClawStatus::Running:      return "运行中";
        case ClawStatus::Error:        return "异常";
        default:                       return "未知";
    }
}

static lv_color_t status_color(ClawStatus s) {
    switch (s) {
        case ClawStatus::Running:  return lv_color_make(0, 200, 0);
        case ClawStatus::Error:    return lv_color_make(200, 0, 0);
        default:                   return lv_color_make(200, 200, 0);
    }
}

void app_refresh_status() {
    g_info = app_detect_openclaw();
    g_status = app_check_status(g_info);
    char buf[256];
    lv_label_set_text(status_label, status_text(g_status));
    lv_obj_set_style_text_color(status_label, status_color(g_status), 0);
    snprintf(buf, sizeof(buf), "版本: %s", g_info.version[0] ? g_info.version : "未知");
    lv_label_set_text(version_label, buf);
    snprintf(buf, sizeof(buf), "路径: %s", g_info.executable[0] ? g_info.executable : "未找到");
    lv_label_set_text(path_label, buf);
    snprintf(buf, sizeof(buf), "端口: %d", g_info.gateway_port);
    lv_label_set_text(port_label, buf);
    if (g_status == ClawStatus::Running) { lv_led_on(led_ok); lv_led_off(led_warn); }
    else if (g_status == ClawStatus::Error) { lv_led_off(led_ok); lv_led_on(led_warn); }
    else { lv_led_off(led_ok); lv_led_off(led_warn); }
    log_msg("[就绪] 状态已刷新");
}

void app_ui_init() {
    lv_obj_t* scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_make(30, 30, 40), 0);

    /* Title */
    lv_obj_t* title = lv_label_create(scr);
    lv_label_set_text(title, "[AnyClaw] 桌面管理器");
    lv_obj_set_style_text_color(title, lv_color_make(100, 149, 237), 0);
    lv_obj_set_style_text_font(title, CJK_FONT, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

    /* Left panel: Status (390 x 260) at x=8 */
    lv_obj_t* pl = lv_obj_create(scr);
    lv_obj_set_size(pl, 390, 260);
    lv_obj_set_pos(pl, 8, 40);
    lv_obj_set_style_bg_color(pl, lv_color_make(40, 40, 55), 0);
    lv_obj_set_style_border_width(pl, 1, 0);
    lv_obj_set_style_border_color(pl, lv_color_make(60, 60, 80), 0);
    lv_obj_set_style_radius(pl, 10, 0);
    lv_obj_set_style_pad_all(pl, 10, 0);

    lv_obj_t* pt = lv_label_create(pl);
    lv_label_set_text(pt, "OpenClaw 状态");
    lv_obj_set_style_text_color(pt, lv_color_make(180, 180, 200), 0);
    lv_obj_set_style_text_font(pt, CJK_FONT, 0);
    lv_obj_align(pt, LV_ALIGN_TOP_MID, 0, 5);

    lv_obj_t* sr = lv_obj_create(pl);
    lv_obj_set_size(sr, 360, 36);
    lv_obj_align(sr, LV_ALIGN_TOP_MID, 0, 35);
    lv_obj_set_style_bg_opa(sr, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(sr, 0, 0);
    lv_obj_set_style_pad_all(sr, 0, 0);

    led_ok = lv_led_create(sr);
    lv_obj_align(led_ok, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_set_size(led_ok, 12, 12);
    lv_led_set_color(led_ok, lv_color_make(0, 200, 0));

    led_warn = lv_led_create(sr);
    lv_obj_align(led_warn, LV_ALIGN_LEFT_MID, 38, 0);
    lv_obj_set_size(led_warn, 12, 12);
    lv_led_set_color(led_warn, lv_color_make(200, 0, 0));

    status_label = lv_label_create(sr);
    lv_label_set_text(status_label, "检测中...");
    lv_obj_set_style_text_color(status_label, lv_color_make(200, 200, 200), 0);
    lv_obj_set_style_text_font(status_label, CJK_FONT, 0);
    lv_obj_align(status_label, LV_ALIGN_LEFT_MID, 70, 0);

    version_label = lv_label_create(pl);
    lv_label_set_text(version_label, "版本: --");
    lv_obj_set_style_text_color(version_label, lv_color_make(160, 160, 180), 0);
    lv_obj_set_style_text_font(version_label, CJK_FONT, 0);
    lv_obj_align(version_label, LV_ALIGN_TOP_LEFT, 10, 80);

    path_label = lv_label_create(pl);
    lv_label_set_text(path_label, "路径: --");
    lv_obj_set_style_text_color(path_label, lv_color_make(160, 160, 180), 0);
    lv_obj_set_style_text_font(path_label, CJK_FONT, 0);  /* Must use CJK font for Chinese */
    lv_label_set_long_mode(path_label, LV_LABEL_LONG_MODE_DOTS);  /* Truncate with dots */
    lv_obj_set_width(path_label, 280);  /* Narrower to trigger DOTS truncation */
    lv_obj_align(path_label, LV_ALIGN_TOP_LEFT, 10, 110);

    port_label = lv_label_create(pl);
    lv_label_set_text(port_label, "端口: --");
    lv_obj_set_style_text_color(port_label, lv_color_make(160, 160, 180), 0);
    lv_obj_set_style_text_font(port_label, CJK_FONT, 0);
    lv_obj_align(port_label, LV_ALIGN_TOP_LEFT, 10, 220);

    /* Right panel: Controls (420 x 260) at x=410 */
    lv_obj_t* pr = lv_obj_create(scr);
    lv_obj_set_size(pr, 420, 260);
    lv_obj_set_pos(pr, 410, 40);
    lv_obj_set_style_bg_color(pr, lv_color_make(40, 45, 60), 0);
    lv_obj_set_style_border_width(pr, 2, 0);
    lv_obj_set_style_border_color(pr, lv_color_make(70, 80, 110), 0);
    lv_obj_set_style_radius(pr, 10, 0);
    lv_obj_set_style_pad_all(pr, 10, 0);

    lv_obj_t* rt = lv_label_create(pr);
    lv_label_set_text(rt, "操作面板");
    lv_obj_set_style_text_color(rt, lv_color_make(180, 180, 200), 0);
    lv_obj_set_style_text_font(rt, CJK_FONT, 0);
    lv_obj_align(rt, LV_ALIGN_TOP_MID, 0, 5);

    btn_start = lv_button_create(pr);
    lv_obj_set_size(btn_start, 170, 42);
    lv_obj_set_pos(btn_start, 12, 42);
    lv_obj_set_style_bg_color(btn_start, lv_color_make(0, 150, 50), 0);
    lv_obj_set_style_radius(btn_start, 8, 0);
    lv_obj_add_event_cb(btn_start, btn_start_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* ls = lv_label_create(btn_start);
    lv_label_set_text(ls, "启动");
    lv_obj_set_style_text_font(ls, CJK_FONT, 0);
    lv_obj_center(ls);

    btn_stop = lv_button_create(pr);
    lv_obj_set_size(btn_stop, 170, 42);
    lv_obj_set_pos(btn_stop, 210, 42);
    lv_obj_set_style_bg_color(btn_stop, lv_color_make(180, 50, 50), 0);
    lv_obj_set_style_radius(btn_stop, 8, 0);
    lv_obj_add_event_cb(btn_stop, btn_stop_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* lst = lv_label_create(btn_stop);
    lv_label_set_text(lst, "停止");
    lv_obj_set_style_text_font(lst, CJK_FONT, 0);
    lv_obj_center(lst);

    btn_refresh = lv_button_create(pr);
    lv_obj_set_size(btn_refresh, 360, 38);
    lv_obj_set_pos(btn_refresh, 18, 95);
    lv_obj_set_style_bg_color(btn_refresh, lv_color_make(50, 100, 180), 0);
    lv_obj_set_style_radius(btn_refresh, 8, 0);
    lv_obj_add_event_cb(btn_refresh, btn_refresh_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* lr = lv_label_create(btn_refresh);
    lv_label_set_text(lr, "刷新状态");
    lv_obj_set_style_text_font(lr, CJK_FONT, 0);
    lv_obj_center(lr);

    log_label = lv_label_create(pr);
    lv_label_set_text(log_label, "[就绪] 等待操作...");
    lv_obj_set_style_text_color(log_label, lv_color_make(100, 255, 100), 0);
    lv_obj_set_style_text_font(log_label, CJK_FONT, 0);
    lv_label_set_long_mode(log_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(log_label, 360);
    lv_obj_set_pos(log_label, 18, 170);

    /* Footer */
    lv_obj_t* footer = lv_label_create(scr);
    lv_label_set_text(footer, "AnyClaw LVGL v1.0");
    lv_obj_set_style_text_color(footer, lv_color_make(80, 80, 100), 0);
    lv_obj_set_style_text_font(footer, &lv_font_montserrat_10, 0);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -8);

    app_refresh_status();
}
