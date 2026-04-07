#ifndef AW_LABEL_H
#define AW_LABEL_H

/*
 * ═══════════════════════════════════════════════════════════════
 *  AnyClaw Widgets — Standardized Label Creation
 * ═══════════════════════════════════════════════════════════════
 */

#include "aw_common.h"

enum AwLabelStyle {
    LABEL_TITLE,    /* Large accent-colored heading */
    LABEL_BODY,     /* Normal text */
    LABEL_HINT,     /* Small dim text */
    LABEL_MONO,     /* Monospace/code text */
};

/* Create a styled label */
inline lv_obj_t* aw_label_create(lv_obj_t* parent, const char* text,
                                  AwLabelStyle style = LABEL_BODY) {
    lv_obj_t* lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_clear_flag(lbl, LV_OBJ_FLAG_SCROLLABLE);

    switch (style) {
        case LABEL_TITLE:
            lv_obj_set_style_text_color(lbl, aw::dark_accent(), 0);
            lv_obj_set_style_text_font(lbl, &lv_font_mshy_16, 0);
            break;
        case LABEL_BODY:
            lv_obj_set_style_text_color(lbl, aw::dark_text(), 0);
            lv_obj_set_style_text_font(lbl, &lv_font_mshy_16, 0);
            break;
        case LABEL_HINT:
            lv_obj_set_style_text_color(lbl, aw::dark_text_dim(), 0);
            lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
            break;
        case LABEL_MONO:
            lv_obj_set_style_text_color(lbl, lv_color_make(180, 220, 180), 0);
            lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
            break;
    }

    return lbl;
}

/* Create a wrapping label (for multi-line text) */
inline lv_obj_t* aw_label_wrap_create(lv_obj_t* parent, const char* text,
                                       AwLabelStyle style = LABEL_BODY,
                                       int max_width_pct = 100) {
    lv_obj_t* lbl = aw_label_create(parent, text, style);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_width(lbl, LV_PCT(max_width_pct));
    return lbl;
}

#endif /* AW_LABEL_H */
