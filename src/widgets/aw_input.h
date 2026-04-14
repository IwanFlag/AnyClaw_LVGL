#ifndef AW_INPUT_H
#define AW_INPUT_H

/*
 * ═══════════════════════════════════════════════════════════════
 *  AnyClaw Widgets — Standardized Input Creation
 * ═══════════════════════════════════════════════════════════════
 */

#include "aw_common.h"

/* Create a styled textarea */
inline lv_obj_t* aw_textarea_create(lv_obj_t* parent,
                                     const char* placeholder = nullptr,
                                     bool password_mode = false,
                                     int w = 0, int h = 0) {
    lv_obj_t* ta = lv_textarea_create(parent);
    if (w > 0) lv_obj_set_width(ta, w);
    else lv_obj_set_width(ta, LV_PCT(100));
    if (h > 0) lv_obj_set_height(ta, h);
    if (placeholder) lv_textarea_set_placeholder_text(ta, placeholder);
    if (password_mode) lv_textarea_set_password_mode(ta, true);

    /* Style */
    const lv_font_t* fnt = AW_CJK_FONT ? AW_CJK_FONT : aw_font(16);
    lv_obj_set_style_bg_color(ta, aw::dark_input(), 0);
    lv_obj_set_style_border_color(ta, aw::dark_border(), 0);
    lv_obj_set_style_border_width(ta, 1, 0);
    lv_obj_set_style_radius(ta, AW_RADIUS_MEDIUM, 0);
    lv_obj_set_style_pad_all(ta, AW_PAD_SMALL, 0);
    lv_obj_set_style_text_color(ta, aw::dark_text(), 0);
    lv_obj_set_style_text_font(ta, fnt, 0);

    /* Cursor color */
    lv_obj_set_style_border_color(ta, aw::color_primary(), LV_PART_CURSOR | LV_STATE_FOCUSED);

    /* Selection color */
    lv_obj_set_style_bg_color(ta, aw::color_sel_bg(), LV_PART_SELECTED);

    return ta;
}

/* Create a styled dropdown */
inline lv_obj_t* aw_dropdown_create(lv_obj_t* parent, const char* options, int w = 0) {
    lv_obj_t* dd = lv_dropdown_create(parent);
    if (w > 0) lv_obj_set_width(dd, w);
    else lv_obj_set_width(dd, LV_PCT(100));
    if (options) lv_dropdown_set_options(dd, options);

    /* Style */
    const lv_font_t* fnt = AW_CJK_FONT ? AW_CJK_FONT : aw_font(16);
    lv_obj_set_style_bg_color(dd, aw::dark_input(), 0);
    lv_obj_set_style_border_color(dd, aw::dark_border(), 0);
    lv_obj_set_style_border_width(dd, 1, 0);
    lv_obj_set_style_radius(dd, AW_RADIUS_MEDIUM, 0);
    lv_obj_set_style_text_color(dd, aw::dark_text(), 0);
    lv_obj_set_style_text_font(dd, fnt, 0);

    /* Indicator (dropdown arrow) */
    lv_obj_set_style_text_font(dd, &lv_font_montserrat_12, LV_PART_INDICATOR);
    lv_obj_set_style_text_font(dd, fnt, LV_PART_ITEMS | LV_STATE_DEFAULT);

    return dd;
}

/* Create a styled switch */
inline lv_obj_t* aw_switch_create(lv_obj_t* parent, bool initial = false) {
    lv_obj_t* sw = lv_switch_create(parent);
    if (initial) lv_obj_add_state(sw, LV_STATE_CHECKED);

    /* Style */
    lv_obj_set_style_bg_color(sw, g_colors ? g_colors->btn_secondary : lv_color_make(80, 85, 100), 0);
    lv_obj_set_style_bg_color(sw, aw::color_primary(), LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_radius(sw, AW_RADIUS_LARGE, 0);

    return sw;
}

#endif /* AW_INPUT_H */
