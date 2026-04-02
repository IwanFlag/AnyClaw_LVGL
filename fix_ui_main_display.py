import os

filepath = r'C:\Users\thundersoft\.openclaw\workspace\AnyClaw_LVGL\src\ui_main.cpp'
with open(filepath, 'r', encoding='utf-8') as f:
    content = f.read()

# Replace using the actual line endings (handle \r\n)
old_text = '/* P2-fix: Get actual display dimensions from LVGL (matches actual SDL window size) */'
new_comment = '/* P2-fix: Get actual window dimensions from main.cpp (actual SDL window size, NOT display resolution) */'

if old_text in content:
    # Replace the comment line
    content = content.replace(old_text, new_comment)
    
    # Replace the actual API calls
    content = content.replace(
        'WIN_W = (int)lv_display_get_horizontal_resolution(NULL);',
        'WIN_W = g_win_w > 800 ? g_win_w : 1088;'
    )
    content = content.replace(
        'WIN_H = (int)lv_display_get_vertical_resolution(NULL);',
        'WIN_H = g_win_h > 500 ? g_win_h : 680;'
    )
    
    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(content)
    print("Replacement successful!")
else:
    print("Old comment not found!")
