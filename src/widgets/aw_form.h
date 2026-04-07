#ifndef AW_FORM_H
#define AW_FORM_H

/*
 * ═══════════════════════════════════════════════════════════════
 *  AnyClaw Widgets — Reusable Form Plugin
 *  Unified section/field builders for profile/settings panels.
 * ═══════════════════════════════════════════════════════════════
 */

#include "aw_panel.h"
#include "aw_label.h"
#include "aw_input.h"
#include "aw_button.h"

inline lv_obj_t* aw_form_section_create(lv_obj_t* parent, const char* title, int w = 0) {
    lv_obj_t* card = aw_card_create(parent, w > 0 ? w : LV_PCT(100), 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_gap(card, AW_PAD_SMALL, 0);
    aw_label_create(card, title ? title : "", LABEL_TITLE);
    return card;
}

inline lv_obj_t* aw_form_field_text(lv_obj_t* parent,
                                    const char* label,
                                    const char* value,
                                    lv_obj_t** out_ta,
                                    bool multiline = false,
                                    int w = 0) {
    lv_obj_t* row = lv_obj_create(parent);
    aw_make_container(row);
    lv_obj_set_width(row, w > 0 ? w : LV_PCT(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(row, AW_PAD_TINY, 0);

    aw_label_create(row, label ? label : "", LABEL_HINT);
    lv_obj_t* ta = aw_textarea_create(row, nullptr, false, w > 0 ? w : 0, multiline ? 48 : 32);
    lv_textarea_set_one_line(ta, multiline ? false : true);
    lv_textarea_set_text(ta, value ? value : "");
    if (out_ta) *out_ta = ta;
    return row;
}

inline lv_obj_t* aw_form_field_dropdown(lv_obj_t* parent,
                                        const char* label,
                                        const char* options,
                                        uint16_t selected,
                                        lv_obj_t** out_dd,
                                        int w = 0) {
    lv_obj_t* row = lv_obj_create(parent);
    aw_make_container(row);
    lv_obj_set_width(row, w > 0 ? w : LV_PCT(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(row, AW_PAD_TINY, 0);

    aw_label_create(row, label ? label : "", LABEL_HINT);
    lv_obj_t* dd = aw_dropdown_create(row, options, w > 0 ? w : 0);
    lv_dropdown_set_selected(dd, selected);
    if (out_dd) *out_dd = dd;
    return row;
}

#endif /* AW_FORM_H */
