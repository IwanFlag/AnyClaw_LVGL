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
static RECT g_btn_exclude = {1200, 0, 1340, TITLE_BAR_HEIGHT};

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

    /* P2-34: 发布版自动隐藏 Console 窗口，开发版保留 */
#ifdef _DEBUG
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
#else
    /* Release: no console window */
    FreeConsole();
#endif

    HINSTANCE hInstance = GetModuleHandle(nullptr);

    /* Init system tray */
    if (!tray_init(hInstance)) {
        printf("[MAIN] Warning: tray init failed\n");
    }

    /* Force software rendering for screenshot compatibility - must be before SDL_Init */
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");

    /* Init LVGL first (lv_sdl_window_create calls SDL_Init internally) */
    lv_init();

    /* Bug 2: Detect actual screen resolution and calculate 85% window size */
    int screen_w = 1920, screen_h = 1080;
    {
        SDL_DisplayMode dm;
        if (SDL_GetCurrentDisplayMode(0, &dm) == 0 && dm.w > 0 && dm.h > 0) {
            screen_w = dm.w;
            screen_h = dm.h;
        }
        printf("[INFO] Screen resolution: %dx%d\n", screen_w, screen_h);
    }
    int win_w = (int)(screen_w * 0.60);
    int win_h = (int)(screen_h * 0.75);
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

        /* Process SDL events */
        {
            SDL_Event ev;
            while (SDL_PollEvent(&ev)) {
                if (ev.type == SDL_QUIT) {
                    /* P2-03: Save window position before minimizing */
                    extern void save_theme_config();
                    save_theme_config();
                    /* Minimize to tray instead of quitting */
                    tray_show_window(false);
                    printf("[MAIN] Window minimized to tray\n");
                }
                /* Handle window maximize/restore events from OS */
                if (ev.type == SDL_WINDOWEVENT) {
                    if (ev.window.event == SDL_WINDOWEVENT_MAXIMIZED) {
                        extern void ui_on_window_maximized();
                        ui_on_window_maximized();
                    } else if (ev.window.event == SDL_WINDOWEVENT_RESTORED) {
                        extern void ui_on_window_restored();
                        ui_on_window_restored();
                    }
                }
                /* Keyboard shortcuts */
                if (ev.type == SDL_KEYDOWN) {
                    extern void ui_settings_open();
                    extern void ui_settings_close();
                    extern bool ui_settings_is_open();
                    switch (ev.key.keysym.sym) {
                        case SDLK_F8:
                            if (!ui_settings_is_open()) ui_settings_open();
                            break;
                        case SDLK_ESCAPE:
                            if (ui_settings_is_open()) ui_settings_close();
                            break;
                    }
                }
            }
        }

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
