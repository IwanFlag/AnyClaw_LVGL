#include "lv_conf.h"
#include "lvgl.h"
#include "app.h"
#include "SDL.h"
#include "drivers/sdl/lv_sdl_window.h"
#include "drivers/sdl/lv_sdl_mouse.h"
#include <cstdio>
#include <windows.h>

/* Expose for ui_main.cpp window drag */
static SDL_Window* g_window = nullptr;
SDL_Window* app_get_window() { return g_window; }

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
    }

    /* Create input device */
    lv_indev_t* indev = lv_sdl_mouse_create();
    (void)indev;

    /* Create UI */
    app_ui_init();

    printf("[INFO] GUI initialized. Use title bar to drag window.\n");

    /* Main loop */
    while (1) {
        lv_timer_handler();
        SDL_Delay(5);
    }

    lv_sdl_quit();
    return 0;
}
