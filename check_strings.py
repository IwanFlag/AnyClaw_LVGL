import re

# Check font characters
with open('src/lv_font_mshy_16.c', 'r', encoding='utf-8') as f:
    font_content = f.read()

entries = re.findall(r'/\*\s*U\+([0-9a-fA-F]+)\s+"([^"]+)"\s*\*/', font_content)
font_chars = set()
font_chars_str = []
for hexval, char in entries:
    codepoint = int(hexval, 16)
    font_chars.add(codepoint)
    font_chars_str.append(f'U+{hexval} "{char}"')

print(f'Font contains {len(entries)} characters:')
for s in font_chars_str:
    print(f'  {s}')

# Check all string definitions in source files
import os
src_dir = 'src'
problematic = ['试', '务', '动', '列', '表', '服', '处', '理', '本', '路', '检', '测', '查', '看', '配', '置']

print('\n\n=== Searching for problematic chars in source files ===')
for root, dirs, files in os.walk(src_dir):
    for fname in files:
        if not fname.endswith(('.c', '.cpp', '.h')):
            continue
        fpath = os.path.join(root, fname)
        with open(fpath, 'rb') as f:
            raw = f.read()
        
        # Check for problematic chars
        for char in problematic:
            char_bytes = char.encode('utf-8')
            if char_bytes in raw:
                # Find the context
                with open(fpath, 'r', encoding='utf-8') as ft:
                    for i, line in enumerate(ft, 1):
                        if char in line:
                            # Check if char is in the font
                            cp = ord(char)
                            in_font = "IN FONT" if cp in font_chars else "NOT IN FONT"
                            print(f'{fpath}:{i}: [{in_font}] {line.rstrip()}')
