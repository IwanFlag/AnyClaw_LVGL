#include "lv_conf.h"
#include "lvgl.h"
#include "app.h"
#include "SDL.h"
#include <cstdio>
#include <windows.h>

static lv_display_t* disp = nullptr;
static lv_indev_t* indev = nullptr;

static SDL_Window* window = nullptr;
static SDL_Renderer* renderer = nullptr;
static SDL_Texture* texture = nullptr;

/* Flush callback */
static void sdl_flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* color_p) {
    int32_t w = lv_area_get_width(area);
    int32_t h = lv_area_get_height(area);

    SDL_Rect r = { area->x1, area->y1, w, h };
    int pitch = 1000 * 4; /* Full buffer pitch (LV_DISPLAY_RENDER_MODE_DIRECT) */
    SDL_UpdateTexture(texture, &r, color_p, pitch);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    lv_display_flush_ready(disp);
}

/* Read SDL mouse/touch input */
static void sdl_read_cb(lv_indev_t* indev, lv_indev_data_t* data) {
    (void)indev;
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            exit(0);
        }
    }

    int x, y;
    Uint32 state = SDL_GetMouseState(&x, &y);
    data->point.x = x;
    data->point.y = y;
    data->state = (state & SDL_BUTTON(1)) ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

#include <cstdio>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    /* Enable console for debug output */
    ::AllocConsole();
    ::freopen("CONOUT$", "w", stdout);
    ::freopen("CONOUT$", "w", stderr);

    /* Move console window to top-left corner to avoid blocking the LVGL window */
    {
        HWND hCon = ::GetConsoleWindow();
        if (hCon) {
            ::MoveWindow(hCon, 0, 600, 350, 80, TRUE);
        }
    }

    /* Init SDL */
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL init failed: %s\n", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow("AnyClaw LVGL",
        60, 15,
        1000, 650, SDL_WINDOW_BORDERLESS);
    if (!window) {
        printf("SDL window failed: %s\n", SDL_GetError());
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    texture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
        1000, 650);

    /* Verify actual window size */
    {
        int ww, wh;
        SDL_GetWindowSize(window, &ww, &wh);
        printf("[DEBUG] SDL window: %dx%d\n", ww, wh);
    }

    /* Init LVGL */
    lv_init();

    /* Create display */
    disp = lv_display_create(1000, 650);
    lv_display_set_flush_cb(disp, sdl_flush_cb);

    /* Allocate draw buffer */
    static uint8_t buf[1000 * 650 * 4]; /* 32-bit color */
    lv_display_set_buffers(disp, buf, NULL, sizeof(buf), LV_DISPLAY_RENDER_MODE_DIRECT);

    /* Create input device */
    indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, sdl_read_cb);

    /* Set tick source */
    lv_tick_set_cb([]() -> uint32_t { return SDL_GetTicks(); });

    /* Create UI */
    app_ui_init();

    /* Main loop */
    while (1) {
        lv_timer_handler();
        SDL_Delay(5);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
