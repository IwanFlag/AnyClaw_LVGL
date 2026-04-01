/* ═══════════════════════════════════════════════════════════════
 *  theme.h - Color theme definitions (P2-4)
 *  AnyClaw LVGL v2.0 - Desktop Manager
 * ═══════════════════════════════════════════════════════════════ */
#ifndef ANYCLAW_THEME_H
#define ANYCLAW_THEME_H

#include "lvgl.h"

struct ThemeColors {
    lv_color_t bg;           /* Main background */
    lv_color_t panel;        /* Panel background */
    lv_color_t panel_border; /* Panel border */
    lv_color_t text;         /* Primary text */
    lv_color_t text_dim;     /* Dim text */
    lv_color_t accent;       /* Accent/title */
    lv_color_t input_bg;     /* Input field background */
    lv_color_t log_bg;       /* Log background */
    lv_color_t log_text;     /* Log text */
    lv_color_t btn_start;    /* Start button */
    lv_color_t btn_stop;     /* Stop button */
    lv_color_t btn_action;   /* Action button (refresh/send) */
};

enum class Theme { Dark, Light, Classic };

extern const ThemeColors* g_colors;
extern Theme g_theme;

void save_theme_config();
void load_theme_config();
void apply_theme_to_all();

#endif /* ANYCLAW_THEME_H */
