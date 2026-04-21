/* ═══════════════════════════════════════════════════════════════
 *  theme.h - Color theme definitions (v2.2.1)
 *  AnyClaw LVGL — ThemeColors expanded to ~50 fields
 * ═══════════════════════════════════════════════════════════════ */
#ifndef ANYCLAW_THEME_H
#define ANYCLAW_THEME_H

#include "lvgl.h"

struct ThemeColors {
    /* ── 1. Backgrounds (5) ── */
    lv_color_t bg;                  /* Window root background */
    lv_color_t surface;             /* Input area, secondary surface */
    lv_color_t panel;               /* Panel / card background */
    lv_color_t raised;              /* Bubble, tooltip, floating layer */
    lv_color_t overlay;             /* Hover state, selected state */

    /* ── 2. Text (4) ── */
    lv_color_t text;                /* Primary text, headings */
    lv_color_t text_dim;            /* Secondary / dim text */
    lv_color_t text_hint;           /* Placeholder, disabled */
    lv_color_t text_inverse;        /* White text on dark bg */

    /* ── 3. Accent (5) ── */
    lv_color_t accent;              /* Primary accent, brand color */
    lv_color_t accent_hover;        /* Primary button hover */
    lv_color_t accent_active;       /* Primary button active */
    lv_color_t accent_subtle;       /* Background tint (low opacity) */
    lv_color_t accent_secondary;    /* Secondary accent / link blue */

    /* ── 4. Accent tertiary (1) ── */
    lv_color_t accent_tertiary;     /* Tertiary accent (peachy pink / mochi mauve) */

    /* ── 5. Semantic (4) ── */
    lv_color_t success;             /* Success, complete, Ready */
    lv_color_t warning;             /* Warning, Busy */
    lv_color_t danger;              /* Error, delete, Error */
    lv_color_t info;                /* Info, Checking */

    /* ── 6. Functional (8) ── */
    lv_color_t border;              /* Panel border */
    lv_color_t border_strong;       /* Input focus border */
    lv_color_t divider;             /* Divider line */
    lv_color_t focus_glow;          /* Focus glow color */
    lv_color_t hover_overlay;       /* Generic hover overlay */
    lv_color_t active_overlay;      /* Generic active overlay */
    lv_color_t disabled_bg;         /* Disabled state background */
    lv_color_t disabled_text;       /* Disabled state text */

    /* ── 7. Bubble (4) ── */
    lv_color_t bubble_user_bg;      /* User bubble base color */
    lv_color_t bubble_user_bg_end;  /* User bubble gradient end */
    lv_color_t bubble_ai_bg;        /* AI bubble base color */
    lv_color_t bubble_ai_accent_bar;/* AI bubble left accent bar */

    /* ── 8. Buttons (4) ── */
    lv_color_t btn_action;          /* Primary action button */
    lv_color_t btn_secondary;       /* Secondary / gray button */
    lv_color_t btn_close;           /* Close / danger button */
    lv_color_t btn_add;             /* Add / create button */

    /* ── 9. Log (2) ── */
    lv_color_t log_bg;              /* Log background */
    lv_color_t log_text;            /* Log text */

    /* ── 10. Shadow colors (4) ── */
    lv_color_t shadow_sm;           /* Card slight lift */
    lv_color_t shadow_md;           /* Bubble, tooltip */
    lv_color_t shadow_lg;           /* Dialog, dropdown */
    lv_color_t shadow_glow;         /* Focus / brand glow */

    /* ── 11. Opacity / Alpha (3, 0-255) ── */
    uint8_t    mask_opacity;        /* Modal mask opacity */
    uint8_t    toast_opacity;       /* Toast background opacity */
    uint8_t    loading_opacity;     /* Loading overlay opacity */

    /* ── 12. Structural + Gradient (4) ── */
    uint8_t    btn_grad_enable;     /* 1=gradient buttons, 0=solid */
    uint8_t    icon_stroke_width;   /* Icon stroke: 2=normal, 1=thin */
    lv_color_t btn_grad_start;     /* Button gradient start color */
    lv_color_t btn_grad_end;       /* Button gradient end color */

    /* ── 13. Radius (5, base px, apply SCALE() at use site) ── */
    uint8_t    radius_sm;           /* Small: badges, tags */
    uint8_t    radius_md;           /* Medium: buttons, inputs */
    uint8_t    radius_lg;           /* Large: cards, panels */
    uint8_t    radius_xl;           /* Extra: dialogs, popups */
    uint8_t    radius_2xl;          /* Max: floating elements */
};

/* ═══════════════════════════════════════════════════════════════
 *  ThemeFonts — per-theme font pointers (v2.2.1)
 *  9 English/general-purpose fonts + 2 CJK fonts
 * ═══════════════════════════════════════════════════════════════ */
#ifdef small
#undef small
#endif
struct ThemeFonts {
    /* English/general-purpose fonts (9 levels) */
    lv_font_t* display;       /* Brand title, 28px @800h, Bold */
    lv_font_t* h1;            /* Dialog title, 22px @800h, Bold */
    lv_font_t* h2;            /* Section title, 18px @800h, SemiBold */
    lv_font_t* h3;            /* Card title, 15px @800h, SemiBold */
    lv_font_t* body;          /* Body text, 13px @800h, Regular */
    lv_font_t* body_strong;   /* Emphasized body, 13px @800h, SemiBold */
    lv_font_t* small;         /* Secondary info, 11px @800h, Regular */
    lv_font_t* caption;       /* Tag/capsule, 10px @800h, Medium(500) */
    lv_font_t* code;          /* Code, 12px @800h, Regular */

    /* CJK fonts (same level as English, fallback chain) */
    lv_font_t* cjk_body;      /* CJK body */
    lv_font_t* cjk_title;     /* CJK title (Mochi only uses Source Han Serif) */

    /* FreeType vector fonts (high-DPI preferred, optional) */
    lv_font_t* freetype_body;  /* FreeType body font (may be NULL) */
    lv_font_t* freetype_title; /* FreeType title font (may be NULL) */
};

enum class Theme { Matcha = 0, Peachy = 1, Mochi = 2 };

extern const ThemeColors* g_colors;
extern ThemeFonts g_theme_fonts;
extern Theme g_theme;

void save_theme_config();
void load_theme_config();
void apply_theme_to_all();
void init_theme_fonts(Theme theme);

/* ═══════════════════════════════════════════════════════════════
 *  font_size — PCT-based dynamic font sizing
 *  pct: FONT_*_PCT constant (e.g. FONT_BODY_PCT=163)
 *  win_h: current window height
 *  min_px: minimum pixel value (FONT_MIN_*)
 *  Returns: clamp(win_h * pct / 10000, min_px)
 * ═══════════════════════════════════════════════════════════════ */
static inline int font_size(int pct, int win_h, int min_px) {
    int sz = win_h * pct / 10000;
    return (sz < min_px) ? min_px : sz;
}

#endif /* ANYCLAW_THEME_H */
