#ifndef AW_PANEL_H
#define AW_PANEL_H

/*
 * ═══════════════════════════════════════════════════════════════
 *  AnyClaw Widgets — Panel / Card / Row Creation
 * ═══════════════════════════════════════════════════════════════
 */

#include "aw_common.h"

/* Create a card container (rounded, bordered, padded) */
inline lv_obj_t* aw_card_create(lv_obj_t* parent, int w = 0, int h = 0) {
    lv_obj_t* card = lv_obj_create(parent);
    if (w > 0) lv_obj_set_size(card, w, h > 0 ? h : LV_SIZE_CONTENT);
    else lv_obj_set_size(card, LV_PCT(100), h > 0 ? h : LV_SIZE_CONTENT);

    lv_obj_set_style_bg_color(card, aw::dark_panel(), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, aw::dark_border(), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, AW_RADIUS_LARGE, 0);
    lv_obj_set_style_pad_all(card, AW_PAD_MEDIUM, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    return card;
}

/* Create a horizontal row container (for key:value settings rows) */
inline lv_obj_t* aw_row_create(lv_obj_t* parent, int height = 32) {
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_PCT(100), height);
    aw_make_container(row);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(row, AW_PAD_MEDIUM, 0);

    return row;
}

/* Create a key-value settings row with label and widget */
inline lv_obj_t* aw_setting_row(lv_obj_t* parent, const char* label_text,
                                 lv_obj_t** out_label = nullptr) {
    lv_obj_t* row = aw_row_create(parent);

    lv_obj_t* lbl = lv_label_create(row);
    lv_label_set_text(lbl, label_text);
    lv_obj_set_style_text_color(lbl, aw::dark_text(), 0);
    const lv_font_t* fnt = AW_CJK_FONT ? AW_CJK_FONT : aw_font(16);
    lv_obj_set_style_text_font(lbl, fnt, 0);

    if (out_label) *out_label = lbl;
    return row;
}

#endif /* AW_PANEL_H */
