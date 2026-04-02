import os

filepath = r'C:\Users\thundersoft\.openclaw\workspace\AnyClaw_LVGL\src\ui_main.cpp'
with open(filepath, 'rb') as f:
    raw = f.read()

content = raw.decode('utf-8', errors='replace')
lines = content.split('\n')

# Find the start and end lines
start_line = None
end_line = None
for i, line in enumerate(lines):
    if 'wc_btn_size = 36' in line and start_line is None:
        start_line = i
    if start_line is not None and 'DIVIDER' in line.upper() and i > start_line + 10:
        end_line = i
        break

if start_line is None or end_line is None:
    print(f"Could not find section boundaries: start={start_line}, end={end_line}")
    exit(1)

print(f"Found section from line {start_line+1} to {end_line+1}")

# Build new section
new_section_lines = [
    '    /* P2-01: CN/EN language toggle button */\n',
    '    lv_obj_t* btn_lang_toggle = lv_button_create(title_bar);\n',
    '    lv_obj_set_size(btn_lang_toggle, 56, 30);\n',
    '    lv_obj_align(btn_lang_toggle, LV_ALIGN_RIGHT_MID, -(10 + (36 + 8) * 3 + 16), 0);\n',
    '    lv_obj_set_style_bg_color(btn_lang_toggle, lv_color_make(70, 90, 160), 0);\n',
    '    lv_obj_set_style_bg_opa(btn_lang_toggle, LV_OPA_COVER, 0);\n',
    '    lv_obj_set_style_radius(btn_lang_toggle, 6, 0);\n',
    '    lv_obj_set_style_border_width(btn_lang_toggle, 1, 0);\n',
    '    lv_obj_set_style_border_color(btn_lang_toggle, lv_color_make(100, 120, 180), 0);\n',
    '    lv_obj_add_event_cb(btn_lang_toggle, lang_toggle_cb, LV_EVENT_CLICKED, nullptr);\n',
    '    g_lang_toggle_label = lv_label_create(btn_lang_toggle);\n',
    '    lv_label_set_text(g_lang_toggle_label, (g_lang == Lang::CN) ? "CN" : "EN");\n',
    '    lv_obj_set_style_text_color(g_lang_toggle_label, lv_color_make(255, 255, 255), 0);\n',
    '    lv_obj_set_style_text_font(g_lang_toggle_label, CJK_FONT, 0);\n',
    '    lv_obj_center(g_lang_toggle_label);\n',
    '\n',
    '    /* Settings button */\n',
    '    btn_settings = lv_button_create(title_bar);\n',
    '    lv_obj_set_size(btn_settings, 80, 32);\n',
    '    lv_obj_align(btn_settings, LV_ALIGN_RIGHT_MID, -(10 + (36 + 8) * 3 + 16 + 80), 0);\n',
    '    lv_obj_set_style_bg_color(btn_settings, lv_color_make(70, 80, 120), 0);\n',
    '    lv_obj_set_style_bg_grad_color(btn_settings, lv_color_make(55, 65, 100), 0);\n',
    '    lv_obj_set_style_bg_grad_dir(btn_settings, LV_GRAD_DIR_VER, 0);\n',
    '    lv_obj_set_style_bg_opa(btn_settings, LV_OPA_COVER, 0);\n',
    '    lv_obj_set_style_radius(btn_settings, 8, 0);\n',
    '    lv_obj_set_style_border_width(btn_settings, 1, 0);\n',
    '    lv_obj_set_style_border_color(btn_settings, lv_color_make(100, 110, 150), 0);\n',
    '    lv_obj_add_event_cb(btn_settings, btn_settings_cb, LV_EVENT_CLICKED, nullptr);\n',
    '    lv_obj_t* lset = lv_label_create(btn_settings);\n',
    '    lv_label_set_text(lset, tr(STR_SETTINGS));\n',
    '    lv_obj_set_style_text_color(lset, lv_color_make(255, 255, 255), 0);\n',
    '    lv_obj_set_style_text_font(lset, CJK_FONT, 0);\n',
    '    lv_obj_center(lset);\n',
    '\n',
    '    /* \u2550\u2550 Window Control Buttons (Minimize / Maximize / Close) \u2550\u2550 */\n',
    '    int wc_btn_size = 36;\n',
    '    int wc_btn_gap = 8;\n',
    '    int wc_y_offset = 0;\n',
    '\n',
    '    /* Minimize button \u2014 */\n',
    '    btn_minimize = lv_button_create(title_bar);\n',
    '    lv_obj_set_size(btn_minimize, wc_btn_size, 30);\n',
    '    lv_obj_align(btn_minimize, LV_ALIGN_RIGHT_MID, -(10 + (wc_btn_size + wc_btn_gap) * 2 + wc_btn_size), wc_y_offset);\n',
    '    lv_obj_set_style_bg_color(btn_minimize, lv_color_make(70, 75, 100), 0);\n',
    '    lv_obj_set_style_bg_opa(btn_minimize, LV_OPA_COVER, 0);\n',
    '    lv_obj_set_style_radius(btn_minimize, 6, 0);\n',
    '    lv_obj_set_style_border_width(btn_minimize, 1, 0);\n',
    '    lv_obj_set_style_border_color(btn_minimize, lv_color_make(100, 105, 140), 0);\n',
    '    lv_obj_add_event_cb(btn_minimize, btn_minimize_cb, LV_EVENT_CLICKED, nullptr);\n',
    '    lv_obj_t* lbl_min = lv_label_create(btn_minimize);\n',
    '    lv_label_set_text(lbl_min, "\\xE2\\x80\\x94");  /* \u2014 */\n',
    '    lv_obj_set_style_text_font(lbl_min, CJK_FONT, 0);\n',
    '    lv_obj_center(lbl_min);\n',
    '\n',
    '    /* Maximize/Restore button \u25a1 / \u29c9 */\n',
    '    btn_maximize = lv_button_create(title_bar);\n',
    '    lv_obj_set_size(btn_maximize, wc_btn_size, 30);\n',
    '    lv_obj_align(btn_maximize, LV_ALIGN_RIGHT_MID, -(10 + (wc_btn_size + wc_btn_gap) * 2 + 2), wc_y_offset);\n',
    '    lv_obj_set_style_bg_color(btn_maximize, lv_color_make(70, 75, 100), 0);\n',
    '    lv_obj_set_style_bg_opa(btn_maximize, LV_OPA_COVER, 0);\n',
    '    lv_obj_set_style_radius(btn_maximize, 6, 0);\n',
    '    lv_obj_set_style_border_width(btn_maximize, 1, 0);\n',
    '    lv_obj_set_style_border_color(btn_maximize, lv_color_make(100, 105, 140), 0);\n',
    '    lv_obj_add_event_cb(btn_maximize, btn_maximize_cb, LV_EVENT_CLICKED, nullptr);\n',
    '    lbl_maximize = lv_label_create(btn_maximize);\n',
    '    lv_label_set_text(lbl_maximize, "\\xE2\\x96\\xA1");  /* \u25a1 */\n',
    '    lv_obj_set_style_text_font(lbl_maximize, CJK_FONT, 0);\n',
    '    lv_obj_center(lbl_maximize);\n',
    '\n',
    '    /* Close button (minimize to tray) \u2715 */\n',
    '    btn_close = lv_button_create(title_bar);\n',
    '    lv_obj_set_size(btn_close, wc_btn_size, 30);\n',
    '    lv_obj_align(btn_close, LV_ALIGN_RIGHT_MID, -10, wc_y_offset);\n',
    '    lv_obj_set_style_bg_color(btn_close, lv_color_make(180, 60, 60), 0);\n',
    '    lv_obj_set_style_bg_opa(btn_close, LV_OPA_COVER, 0);\n',
    '    lv_obj_set_style_radius(btn_close, 6, 0);\n',
    '    lv_obj_set_style_border_width(btn_close, 1, 0);\n',
    '    lv_obj_set_style_border_color(btn_close, lv_color_make(220, 80, 80), 0);\n',
    '    lv_obj_add_event_cb(btn_close, btn_close_cb, LV_EVENT_CLICKED, nullptr);\n',
    '    lv_obj_t* lbl_cls = lv_label_create(btn_close);\n',
    '    lv_label_set_text(lbl_cls, "\\xC3\\x97");  /* \u00d7 */\n',
    '    lv_obj_set_style_text_font(lbl_cls, CJK_FONT, 0);\n',
    '    lv_obj_center(lbl_cls);\n',
    '\n',
]

# Reconstruct file
new_lines = lines[:start_line] + new_section_lines + lines[end_line:]
new_content = '\n'.join(new_lines)

with open(filepath, 'w', encoding='utf-8') as f:
    f.write(new_content)

print(f"Successfully replaced {end_line - start_line} lines with {len(new_section_lines)} new lines")
