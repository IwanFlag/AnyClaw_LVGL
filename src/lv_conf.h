#ifndef LV_CONF_H
#define LV_CONF_H

/*====================
   COLOR SETTINGS
 *====================*/
#define LV_COLOR_DEPTH 32
#define LV_COLOR_16_SWAP 0

/*=========================
   MEMORY SETTINGS
 *=========================*/
#define LV_MEM_CUSTOM 1
#define LV_MEM_SIZE (512U * 1024U)

/*====================
   HAL SETTINGS
 *====================*/
#define LV_TICK_CUSTOM 0
#define LV_DPI_DEF 130

/*=================
   DRAW SETTINGS
 *=================*/
#define LV_DRAW_COMPLEX 1

/*===================
   FEATURES
 *===================*/
#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR 0

/*=================
   THEMES
 *=================*/
#define LV_USE_THEME_DEFAULT 1
#if LV_USE_THEME_DEFAULT
    #define LV_THEME_DEFAULT_DARK 1
    #define LV_THEME_DEFAULT_GROW 1
    #define LV_THEME_DEFAULT_TRANSITION_TIME 80
#endif

/*=================
   FONT USAGE
 *=================*/
/* Enable large font support (>4096 glyphs) for expanded CJK font */
#define LV_FONT_FMT_TXT_LARGE 1

#define LV_FONT_MONTSERRAT_10 1
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_22 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_26 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_30 1
#define LV_FONT_MONTSERRAT_32 1
#define LV_FONT_MONTSERRAT_36 1
#define LV_FONT_MONTSERRAT_40 1
#define LV_FONT_MONTSERRAT_48 1
#define LV_FONT_SOURCE_HAN_SANS_SC_16_CJK 0
#define LV_FONT_SOURCE_HAN_SANS_SC_14_CJK 1
#define LV_FONT_MSHY_16 1
typedef struct _lv_font_t lv_font_t;
#ifdef __cplusplus
extern "C" {
#endif
extern const lv_font_t lv_font_mshy_16;
extern const lv_font_t lv_font_source_han_sans_sc_14_cjk;
#ifdef __cplusplus
}
#endif
#define LV_FONT_DEFAULT (&lv_font_montserrat_16)

/*=================
   WIDGETS
 *=================*/
#define LV_USE_ARC        1
#define LV_USE_BAR        1
#define LV_USE_BTN        1
#define LV_USE_BTNMATRIX  1
#define LV_USE_CHECKBOX   1
#define LV_USE_DROPDOWN   1
#define LV_USE_IMG        1
#define LV_USE_LABEL      1
#define LV_USE_LINE       1
#define LV_USE_ROLLER     1
#define LV_USE_SLIDER     1
#define LV_USE_SWITCH      1
#define LV_USE_TABLE      1
#define LV_USE_TEXTAREA   1
#define LV_LABEL_TEXT_SELECTION  1   /* Enable text selection in labels/textareas */
#define LV_USE_LED        1
#define LV_USE_LIST       1
#define LV_USE_MSGBOX     1
#define LV_USE_TABVIEW    1
#define LV_USE_WIN        1
#define LV_USE_ANIMIMG    0
#define LV_USE_CALENDAR   0
#define LV_USE_CHART      0
#define LV_USE_COLORWHEEL 0
#define LV_USE_IMGBTN     0
#define LV_USE_KEYBOARD   0
#define LV_USE_MENU       0
#define LV_USE_METER      0
#define LV_USE_SPAN       0
#define LV_USE_SPINBOX    0
#define LV_USE_SPINNER    0
#define LV_USE_TILEVIEW   0
#define LV_USE_CANVAS     1
#define LV_USE_SNAPSHOT   0

/*=================
   EXAMPLES
 *=================*/
#define LV_BUILD_EXAMPLES 0

/*=================
   IMAGE DECODERS
 *=================*/
#define LV_USE_LODEPNG    1

/*=================
   TINY_TTF (runtime font loading)
 *=================*/
#define LV_USE_TINY_TTF          1
#define LV_TINY_TTF_FILE_SUPPORT 1
#define LV_TINY_TTF_CACHE_GLYPH_CNT 512

/*=================
   FREETYPE (vector font rendering — high-DPI clarity)
 *=================*/
#define LV_USE_FREETYPE         1
#if LV_USE_FREETYPE
    /* FreeType requires 32KB+ draw thread stack */
    #define LV_DRAW_THREAD_STACK_SIZE (32 * 1024)
    /* Cache: higher = more memory, better perf */
    #define LV_FREETYPE_CACHE_FT_GLYPH_CNT 256
    /* Use LVGL's built-in memory allocator */
    #define LV_FREETYPE_USE_LVGL_PORT  0
    /* Outline mode for scalable fonts (not used for emoji bitmap) */
    #define LV_FREETYPE_F26DOT6_SHIFT  0
#endif

/*=================
   SDL DRIVER
 *=================*/
#define LV_USE_SDL               1
#define LV_SDL_RENDER_MODE       LV_DISPLAY_RENDER_MODE_DIRECT
#define LV_SDL_FULLSCREEN        0
#define LV_SDL_DIRECT_EXIT       1
#define LV_SDL_ACCELERATED       1
#define LV_SDL_MOUSEWHEEL_MODE_ENCODER  0
#define LV_SDL_MOUSEWHEEL_MODE_CROWN    1
#define LV_SDL_MOUSEWHEEL_MODE          LV_SDL_MOUSEWHEEL_MODE_CROWN
#define LV_SDL_INCLUDE_PATH      <SDL.h>

/*=================
   FILE SYSTEM (Win32)
 *=================*/
#define LV_USE_FS_WIN32 1
#if LV_USE_FS_WIN32
    #define LV_FS_WIN32_LETTER 'A'     /* Drive letter for LVGL paths like "A:assets/..." */
    #define LV_FS_WIN32_PATH ""         /* Base path (empty = exe directory / CWD) */
    #define LV_FS_WIN32_CACHE_SIZE 0    /* No cache */
#endif

#define LV_USE_OS LV_OS_WINDOWS

#endif /*LV_CONF_H*/
