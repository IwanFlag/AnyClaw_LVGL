#ifndef AW_BUTTON_H
#define AW_BUTTON_H

/*
 * ═══════════════════════════════════════════════════════════════
 *  AnyClaw Widgets — Standardized Button Creation
 * ═══════════════════════════════════════════════════════════════
 */

#include "aw_common.h"

enum AwBtnStyle {
    BTN_PRIMARY,    /* Blue accent button */
    BTN_SECONDARY,  /* Gray/neutral button */
    BTN_DANGER,     /* Red/warning button */
    BTN_SUCCESS,    /* Green/action button */
    BTN_GHOST,      /* Transparent with border */
};

/* Create a styled button with label text */
inline lv_obj_t* aw_btn_create(lv_obj_t* parent, const char* text,
                                AwBtnStyle style = BTN_PRIMARY,
                                int w = 0, int h = 36) {
    lv_obj_t* btn = lv_button_create(parent);
    if (w > 0) lv_obj_set_size(btn, w, h);
    else lv_obj_set_size(btn, LV_SIZE_CONTENT, h);
    lv_obj_set_style_radius(btn, AW_RADIUS_MEDIUM, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

    switch (style) {
        case BTN_PRIMARY:
            lv_obj_set_style_bg_color(btn, g_colors ? g_colors->btn_action : aw::color_primary(), 0);
            lv_obj_set_style_bg_color(btn, g_colors ? g_colors->accent_active : lv_color_make(90, 160, 255), LV_STATE_PRESSED);
            break;
        case BTN_SECONDARY:
            lv_obj_set_style_bg_color(btn, g_colors ? g_colors->btn_secondary : lv_color_make(80, 85, 100), 0);
            lv_obj_set_style_bg_color(btn, g_colors ? g_colors->active_overlay : lv_color_make(100, 105, 120), LV_STATE_PRESSED);
            break;
        case BTN_DANGER:
            lv_obj_set_style_bg_color(btn, g_colors ? g_colors->btn_close : aw::color_danger(), 0);
            lv_obj_set_style_bg_color(btn, g_colors ? g_colors->danger : lv_color_make(240, 100, 100), LV_STATE_PRESSED);
            break;
        case BTN_SUCCESS:
            lv_obj_set_style_bg_color(btn, g_colors ? g_colors->success : lv_color_make(40, 100, 60), 0);
            lv_obj_set_style_bg_color(btn, g_colors ? g_colors->success : lv_color_make(50, 130, 80), LV_STATE_PRESSED);
            break;
        case BTN_GHOST:
            lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_color(btn, aw::dark_border(), 0);
            lv_obj_set_style_border_width(btn, 1, 0);
            break;
    }

    lv_obj_t* lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, aw::color_white(), 0);
    if (AW_CJK_FONT) lv_obj_set_style_text_font(lbl, AW_CJK_FONT, 0);
    else lv_obj_set_style_text_font(lbl, aw_font(16), 0);
    lv_obj_center(lbl);

    return btn;
}

#endif /* AW_BUTTON_H */
