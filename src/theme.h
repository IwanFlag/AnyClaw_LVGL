/* ═══════════════════════════════════════════════════════════════
 *  theme.h - Color theme definitions (P2-4)
 *  AnyClaw LVGL v2.0 - Desktop Manager
 * ═══════════════════════════════════════════════════════════════ */
#ifndef ANYCLAW_THEME_H
#define ANYCLAW_THEME_H

#include "lvgl.h"

struct ThemeColors {
    /* Backgrounds */
    lv_color_t bg;              /* Main background */
    lv_color_t panel;           /* Panel background */
    lv_color_t panel_border;    /* Panel border */
    lv_color_t input_bg;        /* Input field background */

    /* Text — 白字黑底，黑字白底 */
    lv_color_t text;            /* Primary text (high contrast) */
    lv_color_t text_dim;        /* Secondary/dim text */
    lv_color_t text_hint;       /* Hint text (most subtle) */

    /* Accent & Section */
    lv_color_t accent;          /* Accent color, links, section titles */
    lv_color_t section_label;   /* Section title text */

    /* Buttons */
    lv_color_t btn_action;      /* Primary action button */
    lv_color_t btn_secondary;   /* Secondary/gray button */
    lv_color_t btn_close;       /* Close/danger button */
    lv_color_t btn_add;         /* Add/create button (green) */

    /* Log */
    lv_color_t log_bg;          /* Log background */
    lv_color_t log_text;        /* Log text */
};

enum class Theme { Dark, Light, Classic };

extern const ThemeColors* g_colors;
extern Theme g_theme;

void save_theme_config();
void load_theme_config();
void apply_theme_to_all();

#endif /* ANYCLAW_THEME_H */
