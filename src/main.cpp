#include "lv_conf.h"
#include "lvgl.h"
#include "app.h"
#include "tray.h"
#include "health.h"
#include "SDL.h"
#include "SDL_syswm.h"
#include "drivers/sdl/lv_sdl_window.h"
#include "drivers/sdl/lv_sdl_mouse.h"
#include <cstdio>
#include <windows.h>

/* Expose for ui_main.cpp window drag */
static SDL_Window* g_window = nullptr;
SDL_Window* app_get_window() { return g_window; }

/* Intercept SDL window close to minimize to tray */
static bool g_windowCloseIntercepted = false;

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    /* Enable console for debug output */
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

    /* Create SDL window using LVGL's official driver - 1200x800 */
    lv_display_t* disp = lv_sdl_window_create(1200, 800);

    /* Set window title and position */
    lv_sdl_window_set_title(disp, "AnyClaw LVGL v2.0 - Desktop Manager");
    g_window = lv_sdl_window_get_window(disp);

    /* Keep windowed with title bar for easier management */
    if (g_window) {
        SDL_SetWindowPosition(g_window, 100, 50);
        SDL_RaiseWindow(g_window);
        int ww, wh;
        SDL_GetWindowSize(g_window, &ww, &wh);
        printf("[DEBUG] SDL window: %dx%d\n", ww, wh);

        /* Hook into SDL to intercept close button (minimize to tray) */
        /* We'll handle this via the Windows message hook */
    }

    /* Create input device */
    lv_indev_t* indev = lv_sdl_mouse_create();
    (void)indev;

    /* Create UI */
    app_ui_init();

    printf("[INFO] GUI initialized. System tray active.\n");

    /* Start health monitoring */
    health_start();
    tray_set_state(TrayState::Yellow);  /* Start in checking state */
    tray_balloon("AnyClaw", "已启动，正在检测 OpenClaw 状态...", 3000);

    /* Main loop - process Windows messages for tray */
    while (!tray_should_quit()) {
        lv_timer_handler();
        tray_process_messages();

        /* Intercept SDL close: minimize to tray instead of quitting */
        {
            SDL_Event ev;
            while (SDL_PollEvent(&ev)) {
                if (ev.type == SDL_QUIT) {
                    /* Minimize to tray instead of quitting */
                    tray_show_window(false);
                    printf("[MAIN] Window minimized to tray\n");
                }
            }
        }

        SDL_Delay(5);
    }

    printf("[MAIN] Shutting down...\n");

    /* Cleanup */
    health_stop();
    tray_cleanup();
    lv_sdl_quit();

    return 0;
}
