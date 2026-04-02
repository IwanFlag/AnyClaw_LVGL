#include "lv_conf.h"
#include "lvgl.h"
#include "app.h"
#include "tray.h"
#include "health.h"
#include "SDL.h"
#include "SDL_syswm.h"
#include "drivers/sdl/lv_sdl_window.h"
#include "drivers/sdl/lv_sdl_mouse.h"
#include "drivers/sdl/lv_sdl_keyboard.h"
#include "drivers/sdl/lv_sdl_private.h"
#include <cstdio>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "comctl32.lib")

/* Expose for ui_main.cpp window drag */
static SDL_Window* g_window = nullptr;
SDL_Window* app_get_window() { return g_window; }

/* Expose runtime CJK font for ui_settings.cpp */
extern lv_font_t* g_cjk_font;
extern lv_font_t* g_cjk_font_small;
lv_font_t* app_get_cjk_font() { 
    static int printed = 0;
    if (!printed) {
        printf("[FONT] app_get_cjk_font() called, g_cjk_font=%p\n", (void*)g_cjk_font); fflush(stdout);
        printed = 1;
    }
    return g_cjk_font; 
}
lv_font_t* app_get_cjk_font_small() { 
    static int printed = 0;
    if (!printed) {
        printf("[FONT] app_get_cjk_font_small() called\n"); fflush(stdout);
        printed = 1;
    }
    return g_cjk_font_small; 
}

/* Get HWND from SDL_Window */
static HWND getNativeWindowHandle(SDL_Window* window) {
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (SDL_GetWindowWMInfo(window, &info)) {
        return info.info.win.window;
    }
    return nullptr;
}

/* Intercept SDL window close to minimize to tray */
static bool g_windowCloseIntercepted = false;

/* P2: Multi-instance protection via Windows Mutex */
static HANDLE g_instanceMutex = nullptr;

static bool acquire_instance_mutex() {
    g_instanceMutex = ::CreateMutexA(nullptr, TRUE, "Global\\AnyClaw_LVGL_v2_SingleInstance");
    if (::GetLastError() == ERROR_ALREADY_EXISTS) {
        return false;
    }
    return true;
}

static void release_instance_mutex() {
    if (g_instanceMutex) {
        ::ReleaseMutex(g_instanceMutex);
        ::CloseHandle(g_instanceMutex);
        g_instanceMutex = nullptr;
    }
}

/* ═══ Title bar drag via Win32 WM_NCHITTEST ═══ */
#define TITLE_BAR_HEIGHT 48
/* Window control buttons on title bar: right side, exclude from drag */
static RECT g_btn_exclude = {954, 0, 1078, TITLE_BAR_HEIGHT};

static LRESULT CALLBACK titlebar_subclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                          UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    (void)uIdSubclass; (void)dwRefData;
    if (msg == WM_NCHITTEST) {
        LRESULT hit = DefSubclassProc(hwnd, msg, wParam, lParam);
        if (hit == HTCLIENT) {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(hwnd, &pt);
            if (pt.y >= 0 && pt.y < TITLE_BAR_HEIGHT &&
                !(pt.x >= g_btn_exclude.left && pt.x < g_btn_exclude.right &&
                  pt.y >= g_btn_exclude.top && pt.y < g_btn_exclude.bottom)) {
                return HTCAPTION;
            }
        }
        return hit;
    }
    if (msg == WM_SETCURSOR) {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(hwnd, &pt);
        if (pt.y >= 0 && pt.y < TITLE_BAR_HEIGHT &&
            !(pt.x >= g_btn_exclude.left && pt.x < g_btn_exclude.right &&
              pt.y >= g_btn_exclude.top && pt.y < g_btn_exclude.bottom)) {
            SetCursor(LoadCursor(nullptr, IDC_ARROW));
            return TRUE;
        }
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

static void setup_title_drag(HWND hwnd) {
    if (hwnd) {
        SetWindowSubclass(hwnd, titlebar_subclass, 1001, 0);
        printf("[MAIN] Title bar drag subclass installed\n");
    }
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    /* P2: Prevent multiple instances */
    if (!acquire_instance_mutex()) {
        ::MessageBoxA(nullptr, "AnyClaw is already running.\n\n"
            "An instance of AnyClaw LVGL is already active in the system tray.",
            "AnyClaw - Already Running", MB_ICONINFORMATION | MB_OK);
        return 0;
    }

    /* Always show console for debug output */
    ::AllocConsole();
    ::freopen("CONOUT$", "w", stdout);
    ::freopen("CONOUT$", "w", stderr);

    /* Move console window to bottom-left */
    {
        HWND hCon = ::GetConsoleWindow();
        if (hCon) {
            ::MoveWindow(hCon, 0, 800, 500, 150, TRUE);
        }
    }

    HINSTANCE hInstance = GetModuleHandle(nullptr);

    /* Init system tray */
    if (!tray_init(hInstance)) {
        printf("[MAIN] Warning: tray init failed\n");
    }

    /* Force software rendering for screenshot compatibility - must be before SDL_Init */
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");

    /* Init LVGL first (lv_sdl_window_create calls SDL_Init internally) */
    lv_init();

    /* Detect actual screen resolution using Win32 (SDL may return wrong value) */
    int screen_w = GetSystemMetrics(SM_CXSCREEN);
    int screen_h = GetSystemMetrics(SM_CYSCREEN);
    if (screen_w <= 0) screen_w = 1280;
    if (screen_h <= 0) screen_h = 800;
    int win_w = (int)(screen_w * 0.85);
    int win_h = (int)(screen_h * 0.85);
    /* Clamp to reasonable minimum */
    if (win_w < 800) win_w = 800;
    if (win_h < 500) win_h = 500;

    /* Create SDL window using LVGL's official driver */
    lv_display_t* disp = lv_sdl_window_create(win_w, win_h);

    /* Bug 1: Enable window resizing so maximize/restore works */
    lv_sdl_window_set_resizeable(disp, true);

    /* Set window title and position */
    lv_sdl_window_set_title(disp, "AnyClaw LVGL v2.0 - Desktop Manager");
    g_window = lv_sdl_window_get_window(disp);
    /* Fix: get actual window size (may differ from requested if screen is smaller) */
    if (g_window) SDL_GetWindowSize(g_window, &win_w, &win_h);

    /* Left-align so right-side buttons are always visible on screen */
    if (g_window) {
        SDL_SetWindowPosition(g_window, 20, (screen_h - win_h) / 2 - 20);
        SDL_RaiseWindow(g_window);
        
        /* Fix DWM extended frame covering custom title bar */
        HWND hwnd = getNativeWindowHandle(g_window);
        if (hwnd) {
            MARGINS margins = {0, 0, 0, 0};  /* No extended frame */
            DwmExtendFrameIntoClientArea(hwnd, &margins);
        }
        
        int ww, wh;
        SDL_GetWindowSize(g_window, &ww, &wh);
        printf("[DEBUG] SDL window: %dx%d (screen: %dx%d)\n", ww, wh, screen_w, screen_h);
    }

    /* Create input devices */
    lv_indev_t* mouse = lv_sdl_mouse_create();
    lv_indev_t* keyboard = lv_sdl_keyboard_create();
    (void)mouse;
    (void)keyboard;

    /* Create input group for keyboard -> textarea focus */
    lv_group_t* grp = lv_group_create();
    lv_group_set_default(grp);
    lv_indev_set_group(keyboard, grp);

    /* Create UI */
    app_ui_init();

    /* Setup Win32 title bar drag after UI is created (buttons exist) */
    setup_title_drag(getNativeWindowHandle(g_window));

    printf("[INFO] GUI initialized. System tray active.\n");

    /* Start health monitoring */
    health_start();
    tray_set_state(TrayState::Yellow);  /* Start in checking state */
    tray_balloon("AnyClaw", "已启动，正在检测 OpenClaw 状态...", 3000);

    /* Main loop - process Windows messages for tray */
    while (!tray_should_quit()) {
        lv_timer_handler();
        tray_process_messages();

        /* SDL events handled by LVGL internal timer - no manual poll */

        SDL_Delay(5);
    }

    printf("[MAIN] Shutting down...\n");

    /* P2-03/P2-04: Save all config before exit */
    extern void save_theme_config();
    save_theme_config();

    /* Cleanup */
    health_stop();
    tray_cleanup();
    lv_sdl_quit();

    /* P2: Release the instance mutex */
    release_instance_mutex();

    return 0;
}
