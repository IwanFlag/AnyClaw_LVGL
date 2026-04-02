import os

filepath = r'C:\Users\thundersoft\.openclaw\workspace\AnyClaw_LVGL\src\ui_settings.cpp'
with open(filepath, 'r', encoding='utf-8') as f:
    content = f.read()

# Replace the display resolution API calls with globals
old_text = 'SETTING_WIN_W = (int)lv_display_get_horizontal_resolution(NULL);'
new_text = 'SETTING_WIN_W = g_win_w > 800 ? g_win_w : 1088;'
if old_text in content:
    content = content.replace(old_text, new_text)
    print("WIN_W replaced")
else:
    print("WIN_W not found!")

old_text = 'SETTING_WIN_H = (int)lv_display_get_vertical_resolution(NULL);'
new_text = 'SETTING_WIN_H = g_win_h > 500 ? g_win_h : 680;'
if old_text in content:
    content = content.replace(old_text, new_text)
    print("WIN_H replaced")
else:
    print("WIN_H not found!")

with open(filepath, 'w', encoding='utf-8') as f:
    f.write(content)
print("Done!")
