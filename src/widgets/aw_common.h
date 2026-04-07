#ifndef AW_COMMON_H
#define AW_COMMON_H

/*
 * ═══════════════════════════════════════════════════════════════
 *  AnyClaw Widgets — Common Constants & Helpers
 * ═══════════════════════════════════════════════════════════════
 */

#include "lvgl.h"
#include "app_config.h"

/* ── Color Palette ──────────────────────────────────────────── */
namespace aw {
    /* Dark theme */
    inline lv_color_t dark_bg()       { return lv_color_make(26, 30, 46); }
    inline lv_color_t dark_panel()    { return lv_color_make(35, 40, 56); }
    inline lv_color_t dark_text()     { return lv_color_make(224, 224, 224); }
    inline lv_color_t dark_text_dim() { return lv_color_make(130, 135, 150); }
    inline lv_color_t dark_accent()   { return lv_color_make(100, 160, 255); }
    inline lv_color_t dark_border()   { return lv_color_make(42, 46, 62); }
    inline lv_color_t dark_input()    { return lv_color_make(30, 37, 48); }

    /* Functional colors */
    inline lv_color_t color_primary()  { return lv_color_make(59, 130, 246); }
    inline lv_color_t color_success()  { return lv_color_make(0, 220, 60); }
    inline lv_color_t color_warning()  { return lv_color_make(220, 200, 40); }
    inline lv_color_t color_danger()   { return lv_color_make(220, 80, 80); }
    inline lv_color_t color_white()    { return lv_color_make(255, 255, 255); }

    /* Selection */
    inline lv_color_t color_sel_bg()   { return lv_color_make(180, 215, 255); }
    inline lv_color_t color_sel_text() { return lv_color_make(30, 30, 40); }
}

/* ── Spacing Constants ──────────────────────────────────────── */
#define AW_PAD_TINY     2
#define AW_PAD_SMALL    4
#define AW_PAD_MEDIUM   8
#define AW_PAD_LARGE    12
#define AW_PAD_XLARGE   16

#define AW_RADIUS_SMALL  4
#define AW_RADIUS_MEDIUM 8
#define AW_RADIUS_LARGE  12

/* ── Utility: clear scrollable flags + set transparent bg ───── */
inline void aw_make_container(lv_obj_t* obj) {
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_radius(obj, 0, 0);
}

/* ── Utility: separator line ────────────────────────────────── */
inline lv_obj_t* aw_divider_create(lv_obj_t* parent, lv_color_t color) {
    lv_obj_t* sep = lv_obj_create(parent);
    lv_obj_set_size(sep, LV_PCT(100), 1);
    aw_make_container(sep);
    lv_obj_set_style_bg_color(sep, color, 0);
    lv_obj_set_style_bg_opa(sep, LV_OPA_COVER, 0);
    return sep;
}

/* ── Utility: vertical divider ──────────────────────────────── */
inline lv_obj_t* aw_vdivider_create(lv_obj_t* parent, lv_color_t color, int height = 20) {
    lv_obj_t* sep = lv_obj_create(parent);
    lv_obj_set_size(sep, 2, height);
    aw_make_container(sep);
    lv_obj_set_style_bg_color(sep, color, 0);
    lv_obj_set_style_bg_opa(sep, LV_OPA_COVER, 0);
    return sep;
}

#endif /* AW_COMMON_H */
