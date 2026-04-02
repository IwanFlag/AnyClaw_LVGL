#include "app.h"
#include "theme.h"
#include "lang.h"
#include "SDL.h"
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <string>
#include <fstream>
#include <filesystem>
#include <windows.h>

/* Extern for tray minimize */
#include "tray.h"

/* ═══ System Font: Load Windows 微软雅黑 at runtime ═══ */
#include "libs/tiny_ttf/lv_tiny_ttf.h"
lv_font_t* g_cjk_font = nullptr;
lv_font_t* g_cjk_font_small = nullptr;

static void init_system_font() {
    /* Try TrueType fonts from Windows system (stb_truetype: no .ttc support) */
    const char* font_paths[] = {
        "C:/Users/thundersoft/.openclaw/workspace/AnyClaw_LVGL/msyh.ttf",  /* Microsoft YaHei (extracted from .ttc) */
        "C:/Windows/Fonts/simhei.ttf",              /* SimHei - TrueType ✓ full CJK */
        nullptr
    };

    for (int i = 0; font_paths[i]; i++) {
        g_cjk_font = lv_tiny_ttf_create_file(font_paths[i], 16);
        if (g_cjk_font) {
            printf("[Font] Loaded %s, ptr=%p\n", font_paths[i], (void*)g_cjk_font); fflush(stdout);
            {
                FILE* f = fopen("font_debug.log", "w");
                if (f) { fprintf(f, "[Font] Loaded %s, ptr=%p\n", font_paths[i], (void*)g_cjk_font); fclose(f); }
            }
            g_cjk_font_small = lv_tiny_ttf_create_file(font_paths[i], 12);
            if (g_cjk_font_small) {
                printf("[Font] Loaded small (12px): %s\n", font_paths[i]);
            }
            return;
        } else {
            printf("[Font] Failed to load %s\n", font_paths[i]); fflush(stdout);
            {
                FILE* f = fopen("font_debug.log", "a");
                if (f) { fprintf(f, "[Font] Failed to load %s\n", font_paths[i]); fclose(f); }
            }
        }
    }

    printf("[Font] No CJK font found! Using fallback Montserrat\n"); fflush(stdout);
    {
        FILE* f = fopen("font_debug.log", "a");
        if (f) { fprintf(f, "[Font] No CJK font found! Using fallback Montserrat\n"); fclose(f); }
    }
    g_cjk_font = (lv_font_t*)&lv_font_montserrat_16;
    g_cjk_font_small = (lv_font_t*)&lv_font_montserrat_12;
}

#define CJK_FONT (g_cjk_font)
#define CJK_FONT_SMALL (g_cjk_font_small)

/* Layout constants - WIN_W/Win_H are dynamically set from actual display size */
static int WIN_W = 1088;    /* Updated at runtime in app_ui_init() */
static int WIN_H = 680;     /* Updated at runtime in app_ui_init() */
#define TITLE_H 48
#define PANEL_GAP 20        /* Gap between panels */
#define SPLITTER_W 6        /* Draggable splitter width */
static int LEFT_PANEL_W = 272;   /* Dynamic: default 1/4 of 1088 (1:3 ratio) */
static int RIGHT_PANEL_W = 816;  /* Dynamic: WIN_W - LEFT_PANEL_W - PANEL_GAP*2 */
#define PANEL_TOP (TITLE_H + 8)
static int PANEL_H = 450;    /* Updated at runtime: WIN_H - TITLE_H - FOOTER_H - 24 */
#define FOOTER_H 30

/* ═══ Theme System (P2-4) ═══ */
Theme g_theme = Theme::Dark;
static const ThemeColors THEME_DARK = {
    {0x1A, 0x1E, 0x2E},    /* bg */
    {0x23, 0x28, 0x38},    /* panel */
    {0x37, 0x3C, 0x55},    /* panel_border */
    {0xE0, 0xE0, 0xE0},    /* text */
    {0x90, 0x95, 0xB0},    /* text_dim */
    {0x82, 0xAA, 0xF0},    /* accent */
    {0x19, 0x1C, 0x26},    /* input_bg */
    {0x14, 0x14, 0x1C},    /* log_bg */
    {0x78, 0xFF, 0x78},    /* log_text */
    {0x19, 0x8C, 0x3C},    /* btn_start */
    {0xA0, 0x28, 0x28},    /* btn_stop */
    {0x40, 0x70, 0xC0},    /* btn_action - brighter for visibility */
};

static const ThemeColors THEME_LIGHT = {
    {0xF5, 0xF5, 0xF5},    /* bg */
    {0xFF, 0xFF, 0xFF},    /* panel */
    {0xDD, 0xDD, 0xDD},    /* panel_border */
    {0x33, 0x33, 0x33},    /* text */
    {0x88, 0x88, 0x88},    /* text_dim */
    {0x33, 0x66, 0xCC},    /* accent */
    {0xFA, 0xFA, 0xFA},    /* input_bg */
    {0xF0, 0xF0, 0xF0},    /* log_bg */
    {0x22, 0x88, 0x22},    /* log_text */
    {0x1B, 0x8C, 0x3C},    /* btn_start */
    {0xCC, 0x33, 0x33},    /* btn_stop */
    {0x40, 0x70, 0xC0},    /* btn_action - brighter for visibility */
};

static const ThemeColors THEME_CLASSIC = {
    {0x2D, 0x2D, 0x2D},    /* bg */
    {0x3C, 0x3C, 0x3C},    /* panel */
    {0x55, 0x55, 0x55},    /* panel_border */
    {0xFF, 0xFF, 0xFF},    /* text */
    {0xAA, 0xAA, 0xAA},    /* text_dim */
    {0x64, 0xA0, 0xFF},    /* accent */
    {0x28, 0x28, 0x28},    /* input_bg */
    {0x22, 0x22, 0x22},    /* log_bg */
    {0x78, 0xFF, 0x78},    /* log_text */
    {0x19, 0x8C, 0x3C},    /* btn_start */
    {0xA0, 0x28, 0x28},    /* btn_stop */
    {0x40, 0x70, 0xC0},    /* btn_action - brighter for visibility */
};

const ThemeColors* g_colors = &THEME_DARK;

/* P2-01: Language mode - PRD 2.1 single language only, auto-detect system */
static Lang detect_system_lang() {
    /* P2-01: Auto-detect system language via Windows API */
    wchar_t buf[LOCALE_NAME_MAX_LENGTH] = {};
    if (GetSystemDefaultLocaleName(buf, LOCALE_NAME_MAX_LENGTH)) {
        if (wcsncmp(buf, L"zh", 2) == 0) return Lang::CN;
    }
    LANGID lid = GetSystemDefaultUILanguage();
    if (PRIMARYLANGID(lid) == LANG_CHINESE) return Lang::CN;
    return Lang::EN;
}
Lang g_lang = Lang::EN  /* force EN for debugging */;

/* Forward declarations for P2-03/P2-04 persistence */
extern int g_refresh_interval_ms;
extern lv_timer_t* g_refresh_timer;

static std::string get_config_path() {
    const char* appdata = std::getenv("APPDATA");
    if (appdata) {
        std::string dir = std::string(appdata) + "\\AnyClaw_LVGL";
        std::filesystem::create_directories(dir);
        return dir + "\\config.json";
    }
    return "AnyClaw_config.json";
}

/* P2-03/P2-04: Full data persistence - theme, language, window position, refresh interval */
void save_theme_config() {
    std::ofstream f(get_config_path());
    if (f.is_open()) {
        int theme_idx = (int)g_theme;
        int lang_idx = (int)g_lang;

        /* Get window position and size */
        int wx = 0, wy = 0, ww = 1450, wh = 800;
        SDL_Window* win = app_get_window();
        if (win) {
            SDL_GetWindowPosition(win, &wx, &wy);
            SDL_GetWindowSize(win, &ww, &wh);
        }

        f << "{\n";
        f << "  \"theme\": " << theme_idx << ",\n";
        f << "  \"language\": " << lang_idx << ",\n";
        f << "  \"window_x\": " << wx << ",\n";
        f << "  \"window_y\": " << wy << ",\n";
        f << "  \"window_w\": " << ww << ",\n";
        f << "  \"window_h\": " << wh << ",\n";
        f << "  \"refresh_interval_ms\": " << g_refresh_interval_ms << "\n";
        f << "}\n";
        f.close();
    }
}

static void load_theme_config() {
    std::ifstream f(get_config_path());
    if (!f.is_open()) return;
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    f.close();

    /* Simple JSON parsing */
    auto extract_int = [&](const char* key) -> int {
        std::string search = std::string("\"") + key + "\"";
        size_t pos = content.find(search);
        if (pos == std::string::npos) return -1;
        pos = content.find(':', pos);
        if (pos == std::string::npos) return -1;
        pos++;
        while (pos < content.size() && (content[pos] == ' ' || content[pos] == '\t')) pos++;
        int val = 0;
        while (pos < content.size() && content[pos] >= '0' && content[pos] <= '9') {
            val = val * 10 + (content[pos] - '0');
            pos++;
        }
        return val;
    };

    int theme = extract_int("theme");
    if (theme >= 0 && theme <= 2) {
        g_theme = (Theme)theme;
        switch (g_theme) {
            case Theme::Dark:    g_colors = &THEME_DARK; break;
            case Theme::Light:   g_colors = &THEME_LIGHT; break;
            case Theme::Classic: g_colors = &THEME_CLASSIC; break;
        }
    }
    int lang = extract_int("language");
    if (lang >= 0 && lang <= 2) {
        g_lang = (Lang)lang;
    }

    /* P2-03: Restore window position */
    int wx = extract_int("window_x");
    int wy = extract_int("window_y");
    if (wx >= 0 && wy >= 0 && wx < 3000 && wy < 2000) {
        /* Will be applied after SDL window is created */
        SDL_Window* win = app_get_window();
        if (win) {
            SDL_SetWindowPosition(win, wx, wy);
        }
    }

    /* P2-07: Restore refresh interval */
    int ri = extract_int("refresh_interval_ms");
    if (ri >= 5000 && ri <= 300000) {
        g_refresh_interval_ms = ri;
        if (g_refresh_timer) {
            lv_timer_set_period(g_refresh_timer, ri);
        }
    }
}

/* Forward declaration for title bar creation (called last for z-order) */
static void create_title_bar(lv_obj_t* scr);

/* Theme change callback - forward declaration */
void apply_theme_to_all();

static void theme_dropdown_cb(lv_event_t* e) {
    lv_obj_t* dropdown = (lv_obj_t*)lv_event_get_target(e);
    uint16_t sel = lv_dropdown_get_selected(dropdown);
    if (sel == (uint16_t)g_theme) return;
    g_theme = (Theme)sel;
    switch (g_theme) {
        case Theme::Dark:    g_colors = &THEME_DARK; break;
        case Theme::Light:   g_colors = &THEME_LIGHT; break;
        case Theme::Classic: g_colors = &THEME_CLASSIC; break;
    }
    save_theme_config();
    apply_theme_to_all();
    app_log("[Theme] Switched to %s", sel == 0 ? "Dark" : (sel == 1 ? "Light" : "Classic"));
}

/* Called from settings UI to add theme dropdown */
void ui_settings_add_theme_dropdown(lv_obj_t* tab) {
    lv_obj_t* dd = lv_dropdown_create(tab);
    lv_dropdown_set_options(dd, "Dark\nLight\nClassic Dark");
    lv_dropdown_set_selected(dd, (uint16_t)g_theme);
    lv_obj_set_width(dd, 200);
    lv_obj_set_style_bg_color(dd, lv_color_make(40, 44, 60), 0);
    lv_obj_set_style_text_color(dd, lv_color_make(200, 205, 220), 0);
    lv_obj_set_style_border_color(dd, lv_color_make(60, 65, 90), 0);
    lv_obj_add_event_cb(dd, theme_dropdown_cb, LV_EVENT_VALUE_CHANGED, nullptr);
}

/* I18n string pair */
struct I18n { const char* en; const char* cn; };

/* Get translated string based on current language */
static const char* tr(const I18n& s) {
    /* P2-01: Single language display per PRD 2.1 */
    /* English only */ return s.en; }

/* P2-01: Single language title per PRD 2.1 */
static const char* tr_title(const I18n& s) {
    /* English only */ return s.en; }

/* ── I18n Strings ── */
static const I18n STR_TITLE       = {"Desktop Manager", "桌面管理器"};
static const I18n STR_STATUS      = {"OpenClaw Status", "OpenClaw 状态"};
static const I18n STR_CONTROLS    = {"Controls", "操作面板"};
static const I18n STR_LOG         = {"Log", "日志"};
static const I18n STR_START       = {"Start", "启动"};
static const I18n STR_STOP        = {"Stop", "停止"};
static const I18n STR_REFRESH     = {"Refresh", "刷新状态"};
static const I18n STR_SETTINGS    = {"Settings", "设置"};
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
static const I18n STR_TASK_LIST   = {"Task List", "任务列表"};
static const I18n STR_CHAT        = {"Chat", "聊天"};
static const I18n STR_WIFI        = {"Connected", "已连接"};
static const I18n STR_BATTERY     = {"Power", "电源"};
static const I18n STR_NO_TASKS    = {"No pending tasks", "暂无待处理任务"};
static const I18n STR_GW_RUNNING  = {"Gateway Service", "Gateway 服务"};
static const I18n STR_CHAT_INPUT  = {"Type message...", "输入消息..."};
static const I18n STR_SEND        = {"Send", "发送"};

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
static lv_obj_t* g_lang_toggle_label = nullptr;  /* P2-01: language toggle button label */
static lv_obj_t* title_label = nullptr;

/* Window control buttons */
static lv_obj_t* btn_minimize = nullptr;
static lv_obj_t* btn_maximize = nullptr;
static lv_obj_t* btn_close = nullptr;
static lv_obj_t* lbl_maximize = nullptr;  /* Label inside maximize button, for icon swap */
static bool g_maximized = false;          /* Track maximize state */

/* Splitter / Resizable panels */
static lv_obj_t* splitter = nullptr;
static lv_obj_t* left_panel = nullptr;
static lv_obj_t* right_panel = nullptr;
static lv_obj_t* chat_cont = nullptr;      /* Chat outer container */
static lv_obj_t* chat_cont2 = nullptr;     /* Chat inner flex container */
static lv_obj_t* btn_send_widget = nullptr; /* Send button */

/* P2: Task list dynamic widgets */
static lv_obj_t* task_panel = nullptr;
#define MAX_TASK_WIDGETS 5
static lv_obj_t* task_labels[MAX_TASK_WIDGETS] = {nullptr};
static int task_count = 0;

/* P2: Chat area widgets */
static lv_obj_t* chat_display = nullptr;
static lv_obj_t* chat_input = nullptr;
static char chat_history[4096] = {0};

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

/* ── Window control button callbacks ── */
static void btn_minimize_cb(lv_event_t* e) {
    (void)e;
    SDL_Window* win = app_get_window();
    if (win) {
        SDL_MinimizeWindow(win);
        app_log("[Window] Minimized");
    }
}

static void btn_maximize_cb(lv_event_t* e) {
    (void)e;
    SDL_Window* win = app_get_window();
    if (!win) return;

    if (g_maximized) {
        /* Restore to normal size */
        SDL_RestoreWindow(win);
        g_maximized = false;
        if (lbl_maximize) lv_label_set_text(lbl_maximize, "\xE2\x96\xA1");  /* □ */
        app_log("[Window] Restored to normal size");
    } else {
        /* Maximize */
        SDL_MaximizeWindow(win);
        g_maximized = true;
        if (lbl_maximize) lv_label_set_text(lbl_maximize, "\xE2\xA7\xA9");  /* ⧉ */
        app_log("[Window] Maximized");
    }
}

static void btn_close_cb(lv_event_t* e) {
    (void)e;
    /* Save config before minimizing to tray */
    extern void save_theme_config();
    save_theme_config();
    /* Minimize to tray instead of quitting */
    tray_show_window(false);
    app_log("[Window] Minimized to tray");
}

/* Called from main.cpp when OS triggers maximize (double-click title bar, Win+Up, etc.) */
void ui_on_window_maximized() {
    g_maximized = true;
    if (lbl_maximize) lv_label_set_text(lbl_maximize, "\xE2\xA7\xA9");  /* ⧉ */
}

/* Called from main.cpp when OS triggers restore (double-click title bar, Win+Down, etc.) */
void ui_on_window_restored() {
    g_maximized = false;
    if (lbl_maximize) lv_label_set_text(lbl_maximize, "\xE2\x96\xA1");  /* □ */
}

/* ── Window drag ── */
static void title_drag_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    static lv_point_t drag_start = {0, 0};
    static bool dragging = false;
    if (code == LV_EVENT_PRESSING) {
        /* Don't drag when maximized */
        if (g_maximized) return;
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

/* ── Splitter relayout: update all panels and children ── */
static void relayout_panels() {
    if (!left_panel || !right_panel || !splitter) return;

    RIGHT_PANEL_W = WIN_W - LEFT_PANEL_W - PANEL_GAP * 2 - SPLITTER_W;
    if (RIGHT_PANEL_W < 200) { RIGHT_PANEL_W = 200; LEFT_PANEL_W = WIN_W - RIGHT_PANEL_W - PANEL_GAP * 2 - SPLITTER_W; }
    if (LEFT_PANEL_W < 150) { LEFT_PANEL_W = 150; RIGHT_PANEL_W = WIN_W - LEFT_PANEL_W - PANEL_GAP * 2 - SPLITTER_W; }

    /* Move & resize left panel */
    lv_obj_set_size(left_panel, LEFT_PANEL_W, PANEL_H);
    lv_obj_set_pos(left_panel, 10, PANEL_TOP);

    /* Move & resize splitter */
    int split_x = 10 + LEFT_PANEL_W;
    lv_obj_set_size(splitter, SPLITTER_W, PANEL_H);
    lv_obj_set_pos(splitter, split_x, PANEL_TOP);

    /* Move & resize right panel */
    int right_x = split_x + SPLITTER_W + 10;
    lv_obj_set_size(right_panel, RIGHT_PANEL_W, PANEL_H);
    lv_obj_set_pos(right_panel, right_x, PANEL_TOP);

    /* Re-layout right panel children that have explicit sizes */
    int input_h = 66;
    int log_h = 3 * 16 + 14;
    int chat_y = 115;
    int chat_h = PANEL_H - chat_y - input_h - log_h - 36;
    int input_y = PANEL_H - input_h - log_h - 28;
    int log_title_y = PANEL_H - log_h - 20;
    int log_y = log_title_y + 20;

    /* Use global widget pointers for relayout */
    if (chat_cont) lv_obj_set_size(chat_cont, RIGHT_PANEL_W - 24, chat_h);
    if (chat_cont2) lv_obj_set_size(chat_cont2, RIGHT_PANEL_W - 28, chat_h - 4);
    if (chat_display) lv_obj_set_width(chat_display, RIGHT_PANEL_W - 40);
    if (chat_input) {
        lv_obj_set_size(chat_input, RIGHT_PANEL_W - 124, input_h);
        lv_obj_set_pos(chat_input, 10, input_y);
    }
    if (btn_send_widget) {
        lv_obj_set_size(btn_send_widget, 60, input_h);
        lv_obj_set_pos(btn_send_widget, RIGHT_PANEL_W - 108, input_y);
    }
    if (log_panel) {
        lv_obj_set_size(log_panel, RIGHT_PANEL_W - 24, log_h);
        lv_obj_set_pos(log_panel, 10, log_y);
    }

    /* Update left panel children widths */
    if (version_label) lv_obj_set_width(version_label, LEFT_PANEL_W + 20);
    if (path_label) lv_obj_set_width(path_label, LEFT_PANEL_W + 20);
    if (port_label) lv_obj_set_width(port_label, LEFT_PANEL_W + 20);

    lv_obj_invalidate(lv_screen_active());
}

/* ── Splitter drag handler ── */
static void splitter_drag_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    static int drag_start_x = 0;
    static bool is_dragging = false;

    if (code == LV_EVENT_PRESSED) {
        is_dragging = true;
        lv_indev_t* indev = lv_indev_get_act();
        if (indev) {
            lv_point_t p;
            lv_indev_get_point(indev, &p);
            drag_start_x = p.x;
        }
        /* Highlight splitter on press */
        lv_obj_set_style_bg_color(splitter, lv_color_make(130, 170, 255), 0);
    } else if (code == LV_EVENT_PRESSING && is_dragging) {
        lv_indev_t* indev = lv_indev_get_act();
        if (!indev) return;
        lv_point_t p;
        lv_indev_get_point(indev, &p);
        int dx = p.x - drag_start_x;
        drag_start_x = p.x;

        LEFT_PANEL_W += dx;
        relayout_panels();
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_LEAVE) {
        is_dragging = false;
        /* Reset splitter color */
        const ThemeColors* c = g_colors;
        lv_obj_set_style_bg_color(splitter, c->panel_border, 0);
        app_log("[Splitter] Left panel width: %d, Right: %d", LEFT_PANEL_W, RIGHT_PANEL_W);
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

/* P2-27: 自动刷新频率配置（可配置：15s / 30s / 60s） */
int g_refresh_interval_ms = 30000;
lv_timer_t* g_refresh_timer = nullptr;

void update_refresh_interval(int new_ms) {
    g_refresh_interval_ms = new_ms;
    if (g_refresh_timer) {
        lv_timer_set_period(g_refresh_timer, new_ms);
    }
    /* Update hint text */
    char hint_text[128];
    snprintf(hint_text, sizeof(hint_text), "自动刷新: %ds | 拖动标题栏移动窗口",
             new_ms / 1000);
    app_log("[Settings] Refresh interval changed to %ds", new_ms / 1000);
}

/* ── P2: Chat area ── */

/* P2-21: 消息气泡 - 用户消息靠右蓝色，AI消息靠左灰色 */
static void chat_add_user_bubble(const char* text) {
    lv_obj_t* chat_cont = lv_obj_get_parent(chat_display);
    if (!chat_cont) return;

    const ThemeColors* c = g_colors;

    /* 用户消息容器 - 靠右 */
    lv_obj_t* bubble = lv_obj_create(chat_cont);
    lv_obj_set_size(bubble, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_width(bubble, RIGHT_PANEL_W - 60);
    lv_obj_set_style_bg_color(bubble, lv_color_make(59, 130, 246), 0); /* 蓝色 */
    lv_obj_set_style_bg_opa(bubble, LV_OPA_80, 0);
    lv_obj_set_style_radius(bubble, 10, 0);
    lv_obj_set_style_pad_all(bubble, 6, 0);
    lv_obj_set_style_border_width(bubble, 0, 0);
    lv_obj_set_style_margin_bottom(bubble, 4, 0);
    /* Align right */
    lv_obj_set_flex_align(chat_cont, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);

    lv_obj_t* lbl = lv_label_create(bubble);
    char entry[512];
    SYSTEMTIME st;
    GetLocalTime(&st);
    snprintf(entry, sizeof(entry), "[%02d:%02d:%02d] %s",
             st.wHour, st.wMinute, st.wSecond, text);
    lv_label_set_text(lbl, entry);
    lv_obj_set_style_text_color(lbl, lv_color_make(255, 255, 255), 0);
    lv_obj_set_style_text_font(lbl, CJK_FONT, 0);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lv_obj_get_parent(lbl), RIGHT_PANEL_W - 80);

    /* Reset flex align */
    lv_obj_set_flex_align(chat_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    /* Save to chat_history for persistence */
    size_t hlen = strlen(chat_history);
    size_t elen = strlen(entry);
    if (hlen + elen < sizeof(chat_history) - 1) {
        strcat(chat_history, entry);
        strcat(chat_history, "\n");
    }
}

/* P2-20: Include markdown renderer */
#include "markdown.h"

/* P2-19: AI streaming state */
static lv_obj_t* g_stream_bubble = nullptr;
static lv_obj_t* g_stream_label = nullptr;
static char g_stream_buffer[4096] = {0};
static int g_stream_pos = 0;
static lv_timer_t* g_stream_timer = nullptr;
static bool g_streaming = false;

static void stream_timer_cb(lv_timer_t* timer) {
    (void)timer;
    if (!g_stream_label || !g_streaming) return;

    /* Simulate incremental text arrival (for demo) */
    /* In production, this would be fed from HTTP streaming response */
    static const char* demo_text = "\n正在思考...\n\n这是AI的回复示例。**粗体文字** 和普通文字。\n\n- 项目1\n- 项目2\n\n`代码示例` 在这里。";
    int demo_len = (int)strlen(demo_text);

    if (g_stream_pos < demo_len) {
        int chunk = 1; /* One character at a time for typewriter effect */
        if (g_stream_pos + chunk > demo_len) chunk = demo_len - g_stream_pos;

        strncat(g_stream_buffer, demo_text + g_stream_pos, chunk);
        g_stream_pos += chunk;

        /* Update label with markdown rendering */
        render_markdown_to_label(g_stream_label, g_stream_buffer, CJK_FONT);

        /* Auto-scroll */
        lv_obj_t* chat_cont = lv_obj_get_parent(chat_display);
        if (chat_cont) {
            lv_obj_update_layout(chat_cont);
            int h = (int)lv_obj_get_height(chat_cont);
            lv_obj_scroll_to_y(chat_cont, h, LV_ANIM_OFF);
        }
    } else {
        /* Streaming complete */
        g_streaming = false;
        lv_timer_delete(g_stream_timer);
        g_stream_timer = nullptr;

        /* Save to chat_history */
        size_t hlen = strlen(chat_history);
        size_t slen = strlen(g_stream_buffer);
        if (hlen + slen < sizeof(chat_history) - 1) {
            strcat(chat_history, g_stream_buffer);
            strcat(chat_history, "\n");
        }

        app_log("[Chat] Streaming complete");
    }
}

static void chat_start_stream() {
    if (g_streaming) return; /* Already streaming */

    lv_obj_t* chat_cont = lv_obj_get_parent(chat_display);
    if (!chat_cont) return;

    /* Create AI bubble for streaming */
    g_stream_bubble = lv_obj_create(chat_cont);
    lv_obj_set_size(g_stream_bubble, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_width(g_stream_bubble, RIGHT_PANEL_W - 60);
    lv_obj_set_style_bg_color(g_stream_bubble, lv_color_make(55, 65, 81), 0);
    lv_obj_set_style_bg_opa(g_stream_bubble, LV_OPA_60, 0);
    lv_obj_set_style_radius(g_stream_bubble, 10, 0);
    lv_obj_set_style_pad_all(g_stream_bubble, 6, 0);
    lv_obj_set_style_border_width(g_stream_bubble, 0, 0);
    lv_obj_set_style_margin_bottom(g_stream_bubble, 4, 0);
    lv_obj_set_flex_align(chat_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    g_stream_label = lv_label_create(g_stream_bubble);
    lv_obj_set_style_text_color(g_stream_label, lv_color_make(230, 230, 230), 0);
    lv_label_set_long_mode(g_stream_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(g_stream_bubble, RIGHT_PANEL_W - 80);

    /* Initialize stream buffer with header */
    SYSTEMTIME st;
    GetLocalTime(&st);
    snprintf(g_stream_buffer, sizeof(g_stream_buffer), "[%02d:%02d:%02d] [AI]",
             st.wHour, st.wMinute, st.wSecond);
    g_stream_pos = 0;
    g_streaming = true;

    /* Start streaming timer (50ms per character for typewriter effect) */
    g_stream_timer = lv_timer_create(stream_timer_cb, 50, nullptr);

    app_log("[Chat] Streaming started");
}

static void chat_add_ai_bubble(const char* text) {
    lv_obj_t* chat_cont = lv_obj_get_parent(chat_display);
    if (!chat_cont) return;

    /* AI消息容器 - 靠左 */
    lv_obj_t* bubble = lv_obj_create(chat_cont);
    lv_obj_set_size(bubble, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_width(bubble, RIGHT_PANEL_W - 60);
    lv_obj_set_style_bg_color(bubble, lv_color_make(55, 65, 81), 0); /* 灰色 */
    lv_obj_set_style_bg_opa(bubble, LV_OPA_60, 0);
    lv_obj_set_style_radius(bubble, 10, 0);
    lv_obj_set_style_pad_all(bubble, 6, 0);
    lv_obj_set_style_border_width(bubble, 0, 0);
    lv_obj_set_style_margin_bottom(bubble, 4, 0);
    lv_obj_set_flex_align(chat_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t* lbl = lv_label_create(bubble);
    char entry[2048];
    SYSTEMTIME st;
    GetLocalTime(&st);
    snprintf(entry, sizeof(entry), "[%02d:%02d:%02d] [AI]\n",
             st.wHour, st.wMinute, st.wSecond);

    /* P2-20: Render markdown in AI response */
    size_t elen = strlen(entry);
    size_t tlen = strlen(text);
    if (elen + tlen < sizeof(entry) - 1) {
        strncat(entry, text, sizeof(entry) - elen - 1);
    }

    render_markdown_to_label(lbl, entry, CJK_FONT);
    lv_obj_set_style_text_color(lbl, lv_color_make(230, 230, 230), 0);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(bubble, RIGHT_PANEL_W - 80);

    /* Save to chat_history */
    size_t hlen = strlen(chat_history);
    size_t elen2 = strlen(entry);
    if (hlen + elen2 < sizeof(chat_history) - 1) {
        strcat(chat_history, entry);
        strcat(chat_history, "\n");
    }
}

static void chat_send_cb(lv_event_t* e) {
    (void)e;
    if (!chat_input || !chat_display) return;
    const char* text = lv_textarea_get_text(chat_input);
    if (!text || !text[0]) return;

    /* P2-21: 创建用户消息气泡 */
    chat_add_user_bubble(text);
    lv_textarea_set_text(chat_input, "");

    /* P2-19: Start AI streaming response */
    chat_start_stream();

    app_log("[Chat] Message sent, streaming response...");
}

static void chat_input_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_READY) {
        chat_send_cb(e);
    }
    /* 回车键发送消息 */
    if (code == LV_EVENT_KEY) {
        uint32_t key = lv_event_get_key(e);
        if (key == LV_KEY_ENTER) {
            chat_send_cb(e);
        }
    }
}

/* ── P2: Update task list dynamically ── */
static void update_task_list(ClawStatus status) {
    /* P2: Instead of creating new labels that overlap with existing ones,
     * just update the existing task placeholder labels if they exist.
     * The static labels from app_ui_init() handle the display.
     * No dynamic label creation needed - text is updated in app_refresh_status(). */
    (void)status;
}

static void lang_cn_cb(lv_event_t* e) {
    (void)e;
    if (g_lang != Lang::CN) { g_lang = Lang::CN; update_ui_language(); app_log("[Lang] Chinese mode"); }
}
static void lang_en_cb(lv_event_t* e) {
    (void)e;
    if (g_lang != Lang::EN) { g_lang = Lang::EN; update_ui_language(); app_log("[Lang] English mode"); }
}
/* P2-01: CN/EN toggle button - single language per PRD 2.1 */
static void lang_toggle_cb(lv_event_t* e) {
    (void)e;
    /* English only mode - toggle does nothing useful, just show EN */
    if (g_lang_toggle_label) {
        lv_label_set_text(g_lang_toggle_label, "EN");
        lv_obj_center(g_lang_toggle_label);
    }
    app_log("[Lang] English mode (bilingual disabled)");
}

void app_refresh_status() {
    OpenClawInfo info = app_detect_openclaw();
    ClawStatus status = app_check_status(info);
    static char ver_buf[512], path_buf[512], port_buf[64];

    lv_label_set_text(status_label, status_text(status));
    lv_obj_set_style_text_color(status_label, status_color(status), 0);
    snprintf(ver_buf, sizeof(ver_buf), "%s: %s", STR_VERSION.en, info.version[0] ? info.version : STR_NOTFOUND.en);
    lv_label_set_text(version_label, ver_buf);
    if (info.executable[0]) snprintf(path_buf, sizeof(path_buf), "%s: %s", STR_PATH.en, info.executable);
    else snprintf(path_buf, sizeof(path_buf), "%s: %s", STR_PATH.en, STR_NOTFOUND.en);
    lv_label_set_text(path_label, path_buf);
    snprintf(port_buf, sizeof(port_buf), "%s: %d", STR_PORT.en, info.gateway_port);
    lv_label_set_text(port_label, port_buf);
    if (status == ClawStatus::Running) { lv_led_on(led_ok); lv_led_off(led_warn); }
    else if (status == ClawStatus::Error) { lv_led_off(led_ok); lv_led_on(led_warn); }
    else { lv_led_off(led_ok); lv_led_off(led_warn); }

    /* P2: Update dynamic task list */
    update_task_list(status);

    lv_obj_invalidate(lv_screen_active());
}

static void update_ui_language() {
    /* Hardcoded English - no more bilingual mode */
    lv_label_set_text(title_label, STR_TITLE.en);
    lv_obj_t* lbl;
    lbl = lv_obj_get_child(btn_start, 0); if (lbl) lv_label_set_text(lbl, STR_START.en);
    lbl = lv_obj_get_child(btn_stop, 0); if (lbl) lv_label_set_text(lbl, STR_STOP.en);
    lbl = lv_obj_get_child(btn_refresh, 0); if (lbl) lv_label_set_text(lbl, STR_REFRESH.en);
    lbl = lv_obj_get_child(btn_settings, 0); if (lbl) lv_label_set_text(lbl, STR_SETTINGS.en);
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

/* ═══ Legal Disclaimer (first launch) ═══ */
static bool g_disclaimer_accepted = false;

static std::string get_disclaimer_path() {
    const char* appdata = std::getenv("APPDATA");
    if (appdata) {
        std::string dir = std::string(appdata) + "\\AnyClaw_LVGL";
        std::filesystem::create_directories(dir);
        return dir + "\\accepted.json";
    }
    return "accepted.json";
}

static bool is_disclaimer_accepted() {
    std::ifstream f(get_disclaimer_path());
    return f.good();
}

static void save_disclaimer_accepted() {
    std::ofstream f(get_disclaimer_path());
    if (f.is_open()) {
        f << "{\n  \"disclaimer\": true,\n  \"timestamp\": \"2026-04-01\"\n}\n";
        f.close();
    }
}

static void disclaimer_ok_cb(lv_event_t* e) {
    lv_obj_t* checkbox = (lv_obj_t*)lv_event_get_user_data(e);
    if (checkbox && lv_obj_has_state(checkbox, LV_STATE_CHECKED)) {
        g_disclaimer_accepted = true;
        save_disclaimer_accepted();
        /* Hide disclaimer panel */
        lv_obj_t* panel = (lv_obj_t*)lv_event_get_target(e);
        panel = lv_obj_get_parent(lv_obj_get_parent(panel)); /* go up to modal */
        if (panel) {
            lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN);
            lv_obj_del(panel);
        }
        app_log("[Disclaimer] Accepted by user");
    }
}

static void show_disclaimer(lv_obj_t* parent) {
    if (is_disclaimer_accepted()) {
        g_disclaimer_accepted = true;
        return;
    }

    /* Create modal overlay */
    lv_obj_t* modal = lv_obj_create(parent);
    lv_obj_set_size(modal, WIN_W, WIN_H);
    lv_obj_set_pos(modal, 0, 0);
    lv_obj_set_style_bg_color(modal, lv_color_make(20, 20, 30), 0);
    lv_obj_set_style_bg_opa(modal, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(modal, 0, 0);
    lv_obj_set_style_radius(modal, 0, 0);
    lv_obj_set_style_pad_all(modal, 0, 0);
    lv_obj_clear_flag(modal, LV_OBJ_FLAG_SCROLLABLE);

    /* Disclaimer box */
    lv_obj_t* box = lv_obj_create(modal);
    lv_obj_set_size(box, 600, 400);
    lv_obj_center(box);
    lv_obj_set_style_bg_color(box, lv_color_make(30, 35, 50), 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(box, 1, 0);
    lv_obj_set_style_border_color(box, lv_color_make(80, 90, 130), 0);
    lv_obj_set_style_radius(box, 12, 0);
    lv_obj_set_style_pad_all(box, 20, 0);
    lv_obj_set_flex_flow(box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(box, 12, 0);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

    /* Title */
    lv_obj_t* lbl_title = lv_label_create(box);
    lv_label_set_text(lbl_title, tr({"Legal Disclaimer / 法律免责声明",
        "Legal Disclaimer / 法律免责声明"}));
    lv_obj_set_style_text_color(lbl_title, lv_color_make(255, 200, 100), 0);
    lv_obj_set_style_text_font(lbl_title, CJK_FONT, 0);

    /* Content */
    lv_obj_t* lbl_content = lv_label_create(box);
    lv_label_set_text(lbl_content,
        tr({
            "AnyClaw is an open-source desktop assistant.\n"
            "By using this software, you agree that:\n"
            "1. Skill downloads are third-party content\n"
            "2. The developer is not liable for data loss\n"
            "3. API keys are stored locally at your risk\n"
            "4. No warranty is provided\n\n"
            "Please read the full license at GitHub.",
            "AnyClaw 是一款开源桌面助手。\n"
            "使用本软件即表示您同意：\n"
            "1. 技能下载内容来自第三方\n"
            "2. 开发者不对数据丢失承担责任\n"
            "3. API密钥本地存储，风险自担\n"
            "4. 本软件不提供任何担保\n\n"
            "请在GitHub查阅完整许可协议。"
        }));
    lv_obj_set_style_text_color(lbl_content, lv_color_make(200, 205, 220), 0);
    lv_obj_set_style_text_font(lbl_content, CJK_FONT, 0);
    lv_label_set_long_mode(lbl_content, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl_content, LV_PCT(100));

    /* Spacer */
    lv_obj_t* spacer = lv_obj_create(box);
    lv_obj_set_size(spacer, LV_PCT(100), 1);
    lv_obj_set_style_bg_color(spacer, lv_color_make(60, 65, 85), 0);
    lv_obj_set_style_border_width(spacer, 0, 0);
    lv_obj_clear_flag(spacer, LV_OBJ_FLAG_SCROLLABLE);

    /* Checkbox */
    lv_obj_t* cb = lv_checkbox_create(box);
    lv_checkbox_set_text(cb, tr({"I have read and agree", "我已阅读并同意"}));
    lv_obj_set_style_text_color(cb, lv_color_make(200, 205, 220), 0);
    lv_obj_set_style_text_font(cb, CJK_FONT, 0);
    lv_obj_set_width(cb, LV_PCT(100));

    /* OK button */
    lv_obj_t* btn_ok = lv_button_create(box);
    lv_obj_set_width(btn_ok, LV_PCT(100));
    lv_obj_set_height(btn_ok, 40);
    lv_obj_set_style_bg_color(btn_ok, lv_color_make(40, 100, 180), 0);
    lv_obj_set_style_radius(btn_ok, 8, 0);
    lv_obj_add_event_cb(btn_ok, disclaimer_ok_cb, LV_EVENT_CLICKED, cb);
    lv_obj_t* lbl_ok = lv_label_create(btn_ok);
    lv_label_set_text(lbl_ok, tr({"Accept / 同意", "Accept / 同意"}));
    lv_obj_set_style_text_font(lbl_ok, CJK_FONT, 0);
    lv_obj_center(lbl_ok);

    app_log("[Disclaimer] Showing legal disclaimer");
}

/* ═══ Apply theme colors to all UI elements ═══ */
void apply_theme_to_all() {
    const ThemeColors* c = g_colors;
    lv_obj_t* scr = lv_screen_active();

    /* Screen background */
    lv_obj_set_style_bg_color(scr, c->bg, 0);

    /* Title bar - stored as user data, iterate children */
    if (title_bar) {
        lv_obj_set_style_bg_color(title_bar, c->panel, 0);
        lv_obj_set_style_border_color(title_bar, c->panel_border, 0);
    }

    /* Find left panel (child 3 of screen typically) */
    lv_obj_t* child;
    uint32_t i = 0;
    while ((child = lv_obj_get_child(scr, i)) != nullptr) {
        /* Skip title bar (child 0) and divider (child 1) */
        if (i == 0 || i == 1) { i++; continue; }

        /* Check if this is a panel (has bg color close to old panel) */
        /* Apply panel theme to all main panels */
        lv_color_t bg = lv_obj_get_style_bg_color(child, LV_PART_MAIN);
        lv_opa_t opa = lv_obj_get_style_bg_opa(child, LV_PART_MAIN);
        if (opa == LV_OPA_COVER) {
            /* Check if it's a panel by size */
            int w = lv_obj_get_width(child);
            int h = lv_obj_get_height(child);
            if (h == PANEL_H || h == WIN_H) {
                /* Left or right panel, or settings overlay */
                if (w == WIN_W) {
                    /* Settings overlay */
                    lv_obj_set_style_bg_color(child, c->bg, 0);
                } else {
                    lv_obj_set_style_bg_color(child, c->panel, 0);
                    lv_obj_set_style_border_color(child, c->panel_border, 0);
                }
            }
        }
        i++;
    }

    /* Status label colors */
    if (status_label) lv_obj_set_style_text_color(status_label, c->text, 0);
    if (version_label) lv_obj_set_style_text_color(version_label, c->text_dim, 0);
    if (path_label) lv_obj_set_style_text_color(path_label, c->text_dim, 0);
    if (port_label) lv_obj_set_style_text_color(port_label, c->text_dim, 0);

    /* Chat area */
    if (chat_display) {
        lv_obj_set_style_text_color(chat_display, c->text, 0);
        /* Update chat container bg */
        lv_obj_t* chat_cont = lv_obj_get_parent(chat_display);
        if (chat_cont) {
            lv_obj_set_style_bg_color(chat_cont, c->input_bg, 0);
            lv_obj_set_style_border_color(chat_cont, c->panel_border, 0);
        }
    }
    if (chat_input) {
        lv_obj_set_style_bg_color(chat_input, c->input_bg, 0);
        lv_obj_set_style_text_color(chat_input, c->text, 0);
        lv_obj_set_style_border_color(chat_input, c->panel_border, 0);
    }

    /* Log panel */
    if (log_panel) {
        lv_obj_set_style_bg_color(log_panel, c->log_bg, 0);
        lv_obj_set_style_border_color(log_panel, c->panel_border, 0);
        /* Update log text color - first child label */
        lv_obj_t* log_lbl = lv_obj_get_child(log_panel, 0);
        if (log_lbl) lv_obj_set_style_text_color(log_lbl, c->log_text, 0);
    }

    /* Button colors */
    if (btn_start) {
        lv_obj_set_style_bg_color(btn_start, c->btn_start, 0);
        lv_obj_set_style_shadow_color(btn_start, c->panel, 0);
    }
    if (btn_stop) {
        lv_obj_set_style_bg_color(btn_stop, c->btn_stop, 0);
        lv_obj_set_style_shadow_color(btn_stop, c->panel, 0);
    }
    if (btn_refresh) {
        lv_obj_set_style_bg_color(btn_refresh, c->btn_action, 0);
        lv_obj_set_style_shadow_color(btn_refresh, c->panel, 0);
    }

    lv_obj_invalidate(scr);
}

/* ═══ Create title bar LAST so it's above all panels (z-order fix) ═══ */
static void create_title_bar(lv_obj_t* scr) {
    const ThemeColors* c = g_colors;

    title_bar = lv_obj_create(scr);
    lv_obj_set_size(title_bar, lv_obj_get_width(scr), TITLE_H);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_set_style_bg_color(title_bar, c->panel, 0);
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

    /* EN language toggle button removed - default language is English */

    /* ═══ Window control buttons (top-right of title bar) ═══ */
    {
        int wc_btn_size = 36;
        int wc_btn_gap = 8;
        int wc_btn_right_margin = 10;
        int wc_btn_y = (TITLE_H - 30) / 2;  /* vertical center in title bar */
        int wc_base_x = WIN_W - wc_btn_size * 3 - wc_btn_gap * 2 - wc_btn_right_margin;
        /* Clamp: ensure buttons fit within title bar */
        {
            int title_w = (int)lv_display_get_horizontal_resolution(NULL);
            if (title_w > 0 && wc_base_x + wc_btn_size * 3 + wc_btn_gap * 2 + wc_btn_right_margin > title_w) {
                wc_base_x = title_w - wc_btn_size * 3 - wc_btn_gap * 2 - wc_btn_right_margin;
            }
        }

        /* Minimize button - 灰色 */
        btn_minimize = lv_button_create(title_bar);
        lv_obj_set_size(btn_minimize, wc_btn_size, 30);
        lv_obj_set_pos(btn_minimize, wc_base_x, wc_btn_y);
        lv_obj_set_style_bg_color(btn_minimize, lv_color_make(120, 120, 140), 0);
        lv_obj_set_style_bg_opa(btn_minimize, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(btn_minimize, 6, 0);
        lv_obj_set_style_border_width(btn_minimize, 1, 0);
        lv_obj_set_style_border_color(btn_minimize, lv_color_make(200, 200, 220), 0);
        /* Fix: Clear CLICK_FOCUSABLE so first click triggers the callback immediately */
        lv_obj_clear_flag(btn_minimize, LV_OBJ_FLAG_CLICK_FOCUSABLE);
        lv_obj_add_event_cb(btn_minimize, btn_minimize_cb, LV_EVENT_CLICKED, nullptr);
        lv_obj_t* lbl_min = lv_label_create(btn_minimize);
        lv_label_set_text(lbl_min, "-");
        lv_obj_set_style_text_font(lbl_min, CJK_FONT, 0);
        lv_obj_center(lbl_min);

        /* Maximize/Restore button - 蓝色 */
        btn_maximize = lv_button_create(title_bar);
        lv_obj_set_size(btn_maximize, wc_btn_size, 30);
        lv_obj_set_pos(btn_maximize, wc_base_x + wc_btn_size + wc_btn_gap, wc_btn_y);
        lv_obj_set_style_bg_color(btn_maximize, lv_color_make(70, 130, 220), 0);
        lv_obj_set_style_bg_opa(btn_maximize, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(btn_maximize, 6, 0);
        lv_obj_set_style_border_width(btn_maximize, 1, 0);
        lv_obj_set_style_border_color(btn_maximize, lv_color_make(100, 160, 255), 0);
        lv_obj_clear_flag(btn_maximize, LV_OBJ_FLAG_CLICK_FOCUSABLE);
        lv_obj_add_event_cb(btn_maximize, btn_maximize_cb, LV_EVENT_CLICKED, nullptr);
        lbl_maximize = lv_label_create(btn_maximize);
        lv_label_set_text(lbl_maximize, "[]");
        lv_obj_set_style_text_font(lbl_maximize, CJK_FONT, 0);
        lv_obj_center(lbl_maximize);

        /* Close button - 红色 */
        btn_close = lv_button_create(title_bar);
        lv_obj_set_size(btn_close, wc_btn_size, 30);
        lv_obj_set_pos(btn_close, wc_base_x + (wc_btn_size + wc_btn_gap) * 2, wc_btn_y);
        lv_obj_set_style_bg_color(btn_close, lv_color_make(220, 60, 60), 0);
        lv_obj_set_style_bg_opa(btn_close, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(btn_close, 6, 0);
        lv_obj_set_style_border_width(btn_close, 1, 0);
        lv_obj_set_style_border_color(btn_close, lv_color_make(255, 120, 120), 0);
        lv_obj_clear_flag(btn_close, LV_OBJ_FLAG_CLICK_FOCUSABLE);
        lv_obj_add_event_cb(btn_close, btn_close_cb, LV_EVENT_CLICKED, nullptr);
        lv_obj_t* lbl_cls = lv_label_create(btn_close);
        lv_label_set_text(lbl_cls, "X");
        lv_obj_set_style_text_font(lbl_cls, CJK_FONT, 0);
        lv_obj_center(lbl_cls);

        printf("[UI] Window controls at x=%d, y=%d (title bar)\n", wc_base_x, wc_btn_y);
    }

    app_log("[UI] Title bar created (z-order: top, width: %d)", WIN_W - 14);
}

void app_ui_init() {
    /* Load Windows system font (微软雅黑) for CJK text */
    init_system_font();

    /* Load saved config first (sets g_theme, g_lang, g_colors) */
    load_theme_config();
    g_lang = Lang::EN;  /* Force English as default */
    {
        SDL_Window* win = app_get_window();
        if (win) {
            SDL_GetWindowSize(win, &WIN_W, &WIN_H);
        } else {
            WIN_W = (int)lv_display_get_horizontal_resolution(NULL);
            WIN_H = (int)lv_display_get_vertical_resolution(NULL);
        }
    }
    if (WIN_W < 800) WIN_W = 800;   /* Clamp to reasonable minimum */
    if (WIN_H < 500) WIN_H = 500;
    LEFT_PANEL_W = WIN_W / 4;   /* Default 1:3 ratio */
    RIGHT_PANEL_W = WIN_W - LEFT_PANEL_W - PANEL_GAP * 2 - SPLITTER_W;
    PANEL_H = WIN_H - TITLE_H - FOOTER_H - 24;
    if (PANEL_H < 300) PANEL_H = 300;
    printf("[UI] Layout: window=%dx%d, left=%d, right=%d, panel_h=%d\n",
           WIN_W, WIN_H, LEFT_PANEL_W, RIGHT_PANEL_W, PANEL_H);

    const ThemeColors* c = g_colors;
    lv_obj_t* scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, c->bg, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

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
    left_panel = pl;  /* Store reference for splitter */
    lv_obj_set_size(pl, LEFT_PANEL_W, PANEL_H);
    lv_obj_set_pos(pl, 10, PANEL_TOP);
    lv_obj_set_style_bg_color(pl, c->panel, 0);
    lv_obj_set_style_bg_opa(pl, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(pl, 1, 0);
    lv_obj_set_style_border_color(pl, c->panel_border, 0);
    lv_obj_set_style_radius(pl, 12, 0);
    lv_obj_set_style_pad_all(pl, 12, 0);
    lv_obj_clear_flag(pl, LV_OBJ_FLAG_SCROLLABLE);

    /* Panel title with garlic icon */
    /* Panel title */
    lv_obj_t* pt = lv_label_create(pl);
    lv_label_set_text(pt, tr({"Garlic Status", "蒜蓉状态"}));
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
    lv_obj_set_style_text_color(status_label, c->text, 0);
    lv_obj_set_style_text_font(status_label, CJK_FONT, 0);
    lv_obj_align(status_label, LV_ALIGN_LEFT_MID, 55, 0);

    /* Version, Path, Port labels - use small 10px font for English-only info */
    version_label = create_styled_label(pl, "Version: --", c->text_dim, 15, 80, LEFT_PANEL_W + 20);
    path_label = create_styled_label(pl, "Path: --", c->text_dim, 15, 100, LEFT_PANEL_W + 20);
    port_label = create_styled_label(pl, "Port: --", c->text_dim, 15, 120, LEFT_PANEL_W + 20);
    lv_obj_set_style_text_font(version_label, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_font(path_label, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_font(port_label, &lv_font_montserrat_10, 0);

    /* Separator (BUG-011: moved down to 200) */
    lv_obj_t* sep1 = lv_obj_create(pl);
    lv_obj_set_size(sep1, LEFT_PANEL_W - 30, 1);
    lv_obj_set_pos(sep1, 15, 200);
    lv_obj_set_style_bg_color(sep1, c->panel_border, 0);
    lv_obj_set_style_bg_opa(sep1, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sep1, 0, 0);
    lv_obj_set_style_pad_all(sep1, 0, 0);
    lv_obj_clear_flag(sep1, LV_OBJ_FLAG_SCROLLABLE);

    /* Task list area title with icon (BUG-011: moved down to y=205) */
    lv_obj_t* task_title = lv_label_create(pl);
    lv_label_set_text(task_title, tr({"Task List", "任务列表"}));
    lv_obj_set_style_text_color(task_title, c->accent, 0);
    lv_obj_set_style_text_font(task_title, CJK_FONT, 0);
    lv_obj_set_width(task_title, LEFT_PANEL_W - 80);
    lv_obj_set_pos(task_title, 15, 205);

    /* Manual task add button */
    lv_obj_t* btn_add_task = lv_button_create(pl);
    lv_obj_set_size(btn_add_task, 55, 26);
    lv_obj_set_pos(btn_add_task, LEFT_PANEL_W - 70, 205);
    lv_obj_set_style_bg_color(btn_add_task, c->btn_action, 0);
    lv_obj_set_style_radius(btn_add_task, 6, 0);
    lv_obj_t* lbl_add = lv_label_create(btn_add_task);
    lv_label_set_text(lbl_add, tr({"+ Add", "+ 添加"}));
    lv_obj_set_style_text_font(lbl_add, CJK_FONT, 0);
    lv_obj_center(lbl_add);

    /* Hint at bottom */
    lv_obj_t* hint = create_styled_label(pl, tr(STR_AUTOREFRESH), c->text_dim, 15, PANEL_H - 50, LEFT_PANEL_W - 30);
    lv_obj_set_style_text_font(hint, CJK_FONT_SMALL, 0);

    /* ═══ SPLITTER (draggable divider between panels) ═══ */
    splitter = lv_obj_create(scr);
    int split_x = 10 + LEFT_PANEL_W;
    lv_obj_set_size(splitter, SPLITTER_W, PANEL_H);
    lv_obj_set_pos(splitter, split_x, PANEL_TOP);
    lv_obj_set_style_bg_color(splitter, c->panel_border, 0);
    lv_obj_set_style_bg_opa(splitter, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(splitter, 0, 0);
    lv_obj_set_style_radius(splitter, 3, 0);
    lv_obj_set_style_pad_all(splitter, 0, 0);
    lv_obj_clear_flag(splitter, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(splitter, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(splitter, LV_OBJ_FLAG_CHECKABLE);  /* For press visual */
    lv_obj_add_event_cb(splitter, splitter_drag_cb, LV_EVENT_PRESSED, nullptr);
    lv_obj_add_event_cb(splitter, splitter_drag_cb, LV_EVENT_PRESSING, nullptr);
    lv_obj_add_event_cb(splitter, splitter_drag_cb, LV_EVENT_RELEASED, nullptr);
    lv_obj_add_event_cb(splitter, splitter_drag_cb, LV_EVENT_LEAVE, nullptr);

    /* ═══ RIGHT PANEL: Controls ═══ */
    lv_obj_t* pr = lv_obj_create(scr);
    right_panel = pr;  /* Store reference for splitter */
    lv_obj_set_size(pr, RIGHT_PANEL_W, PANEL_H);
    lv_obj_set_pos(pr, split_x + SPLITTER_W + 10, PANEL_TOP);
    lv_obj_set_style_bg_color(pr, c->panel, 0);
    lv_obj_set_style_bg_opa(pr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(pr, 1, 0);
    lv_obj_set_style_border_color(pr, c->panel_border, 0);
    lv_obj_set_style_radius(pr, 12, 0);
    lv_obj_set_style_pad_all(pr, 12, 0);
    lv_obj_clear_flag(pr, LV_OBJ_FLAG_SCROLLABLE);

    /* Panel title */
    lv_obj_t* rt = lv_label_create(pr);
    lv_label_set_text(rt, tr(STR_CONTROLS));
    lv_obj_set_style_text_color(rt, lv_color_make(130, 170, 240), 0);
    lv_obj_set_style_text_font(rt, CJK_FONT, 0);
    lv_obj_set_pos(rt, 8, 8);

    /* Buttons - 4 in a row, evenly spaced */
    int btn_h = 42;
    int btn_y = 48;
    int btn_margin = 10;
    int btn_w = (RIGHT_PANEL_W - 2 * btn_margin - 3 * 12) / 4;  // 4 buttons + 3 gaps of 12px
    int btn_gap = 12;

    btn_start = create_styled_button(pr, tr(STR_START), btn_margin, btn_y, btn_w, btn_h,
        c->btn_start, c->panel, btn_start_cb);

    btn_stop = create_styled_button(pr, tr(STR_STOP), btn_margin + (btn_w + btn_gap), btn_y, btn_w, btn_h,
        c->btn_stop, c->panel, btn_stop_cb);

    btn_refresh = create_styled_button(pr, tr(STR_REFRESH), btn_margin + (btn_w + btn_gap) * 2, btn_y, btn_w, btn_h,
        c->btn_action, c->panel, btn_refresh_cb);

    btn_settings = create_styled_button(pr, tr(STR_SETTINGS), btn_margin + (btn_w + btn_gap) * 3, btn_y, btn_w, btn_h,
        lv_color_make(240, 160, 60), c->panel, btn_settings_cb);

    /* Chat bubble area title with icon */
    lv_obj_t* chat_title = create_styled_label(pr, tr(STR_CHAT), lv_color_make(130, 170, 240), 8, 95, 200);

    /* Layout: adaptive sizing — chat fills space, log pinned to bottom (3 lines) */
    int input_h = 66;   /* 3行高度输入框 */
    int log_h = 3 * 16 + 14;  /* 3行日志 */
    int chat_y = 160;
    /* Chat fills from chat_y down to just above input+log at bottom */
    int chat_h = PANEL_H - chat_y - input_h - log_h - 36;  /* 36 = gaps + titles */
    chat_cont = lv_obj_create(pr);
    lv_obj_set_size(chat_cont, RIGHT_PANEL_W - 24, chat_h);
    lv_obj_set_pos(chat_cont, 10, chat_y);
    lv_obj_set_style_bg_color(chat_cont, c->input_bg, 0);
    lv_obj_set_style_bg_opa(chat_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(chat_cont, 1, 0);
    lv_obj_set_style_border_color(chat_cont, c->panel_border, 0);
    lv_obj_set_style_radius(chat_cont, 6, 0);
    lv_obj_set_style_pad_all(chat_cont, 6, 0);

    /* P2-21: 消息气泡 - 使用滚动容器替代单一label */
    chat_cont2 = lv_obj_create(chat_cont);
    lv_obj_set_size(chat_cont2, RIGHT_PANEL_W - 20, chat_h);
    lv_obj_set_pos(chat_cont2, 10, chat_y);
    lv_obj_set_style_bg_opa(chat_cont2, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(chat_cont2, 0, 0);
    lv_obj_set_style_pad_all(chat_cont2, 4, 0);
    lv_obj_set_flex_flow(chat_cont2, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(chat_cont2, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    chat_display = lv_label_create(chat_cont2);
    lv_label_set_text(chat_display, "");
    /* P2-17: 品牌形象 - 欢迎语 */
    snprintf(chat_history, sizeof(chat_history),
        "[System] LongXia Yao Chi SuanRong De - Your AI assistant is ready!\n");
    lv_label_set_text(chat_display, chat_history);
    lv_obj_set_style_text_color(chat_display, c->text, 0);
    lv_obj_set_style_text_font(chat_display, CJK_FONT, 0);
    lv_label_set_long_mode(chat_display, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(chat_display, RIGHT_PANEL_W - 40);
    lv_obj_set_style_bg_color(chat_display, lv_color_make(40, 44, 52), 0);
    lv_obj_set_style_bg_opa(chat_display, LV_OPA_30, 0);
    lv_obj_set_style_radius(chat_display, 8, 0);
    lv_obj_set_style_pad_all(chat_display, 6, 0);

    /* Chat input area — right below chat, just above log */
    int input_y = PANEL_H - input_h - log_h - 28;  /* above log panel + gap */

    chat_input = lv_textarea_create(pr);
    lv_obj_set_size(chat_input, RIGHT_PANEL_W - 124, input_h);
    lv_obj_set_pos(chat_input, 10, input_y);
    lv_textarea_set_placeholder_text(chat_input, tr(STR_CHAT_INPUT));
    /* P2-22: 多行输入 - 3行高度 */
    lv_textarea_set_one_line(chat_input, false);
    lv_textarea_set_max_length(chat_input, 2000);
    lv_obj_set_style_bg_color(chat_input, c->input_bg, 0);
    lv_obj_set_style_text_color(chat_input, c->text, 0);
    lv_obj_set_style_text_font(chat_input, CJK_FONT, 0);
    lv_obj_set_style_border_color(chat_input, c->panel_border, 0);
    lv_obj_set_style_border_width(chat_input, 1, 0);
    lv_obj_set_style_radius(chat_input, 6, 0);
    lv_obj_set_size(chat_input, RIGHT_PANEL_W - 124, input_h);  /* room for Send(60)+Clear(32)+gaps(2*8) */
    lv_obj_add_event_cb(chat_input, chat_input_cb, LV_EVENT_READY, nullptr);
    lv_obj_add_event_cb(chat_input, chat_input_cb, LV_EVENT_KEY, nullptr);
    /* Add textarea to default group for keyboard input */
    lv_group_add_obj(lv_group_get_default(), chat_input);

    /* Send button */
    btn_send_widget = lv_button_create(pr);
    lv_obj_set_size(btn_send_widget, 60, input_h);
    lv_obj_set_pos(btn_send_widget, RIGHT_PANEL_W - 108, input_y);
    lv_obj_set_style_bg_color(btn_send_widget, lv_color_make(70, 130, 220), 0);
    lv_obj_set_style_bg_opa(btn_send_widget, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn_send_widget, 6, 0);
    lv_obj_set_style_border_width(btn_send_widget, 1, 0);
    lv_obj_set_style_border_color(btn_send_widget, lv_color_make(100, 160, 255), 0);
    lv_obj_add_event_cb(btn_send_widget, chat_send_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* lsend = lv_label_create(btn_send_widget);
    lv_label_set_text(lsend, "Send");
    lv_obj_set_pos(btn_send_widget, RIGHT_PANEL_W - 108, input_y);
    lv_obj_set_style_text_font(lsend, CJK_FONT, 0);
    lv_obj_center(lsend);

    /* Log area — pinned to bottom */
    int log_title_y = PANEL_H - log_h - 20;
    lv_obj_t* log_title = create_styled_label(pr, tr({"Log", "日志"}), lv_color_make(130, 170, 240), 8, log_title_y, 200);

    /* Log panel - 3行日志, 贴底部 */
    int log_y = log_title_y + 20;
    log_panel = lv_obj_create(pr);
    lv_obj_set_size(log_panel, RIGHT_PANEL_W - 24, log_h);
    lv_obj_set_pos(log_panel, 10, log_y);
    lv_obj_set_style_bg_color(log_panel, c->log_bg, 0);
    lv_obj_set_style_bg_opa(log_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(log_panel, 1, 0);
    lv_obj_set_style_border_color(log_panel, c->panel_border, 0);
    lv_obj_set_style_radius(log_panel, 6, 0);
    lv_obj_set_style_pad_all(log_panel, 6, 0);
    lv_obj_set_flex_flow(log_panel, LV_FLEX_FLOW_COLUMN);

    lv_obj_t* log_lbl = lv_label_create(log_panel);
    lv_label_set_text(log_lbl, "");
    lv_obj_set_style_text_color(log_lbl, c->log_text, 0);
    lv_obj_set_style_text_font(log_lbl, &lv_font_montserrat_10, 0);
    lv_label_set_long_mode(log_lbl, LV_LABEL_LONG_WRAP);

    /* ═══ FOOTER ═══ */
    lv_obj_t* footer = lv_label_create(scr);
    lv_label_set_text(footer, tr(STR_FOOTER));
    lv_obj_set_style_text_color(footer, c->text_dim, 0);
    lv_obj_set_style_text_font(footer, CJK_FONT, 0);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -8);

    /* ═══ Settings UI ═══ */
    ui_settings_init(scr);

    /* Initial refresh */
    app_refresh_status();

    /* P2-27: Auto-refresh timer (configurable: 15s / 30s / 60s) */
    g_refresh_timer = lv_timer_create(auto_refresh_cb, g_refresh_interval_ms, nullptr);

    app_log("[Ready] AnyClaw LVGL v2.0 - Bilingual mode");
    app_log("[Garlic] LongXia Yao Chi SuanRong De - Your AI assistant is ready!");

    /* ═══ Create title bar LAST - ensures it's above all panels ═══ */
    create_title_bar(scr);

    /* Show legal disclaimer on first launch */
    show_disclaimer(scr);

    /* Create user_data.txt marker for uninstaller */
    {
        const char* appdata = std::getenv("APPDATA");
        if (appdata) {
            std::string marker = std::string(appdata) + "\\AnyClaw_LVGL\\user_data.txt";
            std::ofstream f(marker);
            if (f.is_open()) {
                f << "AnyClaw LVGL User Data Marker\n";
                f << "Keep this file to preserve user data during uninstall.\n";
                f.close();
            }
        }
    }
}

