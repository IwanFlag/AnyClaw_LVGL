import re

with open('thirdparty/lvgl/src/font/lv_font_source_han_sans_sc_16_cjk.c', 'r', encoding='utf-8') as f:
    content = f.read()

entries = re.findall(r'/\*\s*U\+([0-9a-fA-F]+)\s+"([^"]+)"\s*\*/', content)
print(f'Source Han font contains {len(entries)} characters:')

# Check if problematic chars are in this font
problematic = {'务': 0x52A1, '动': 0x52A8, '列': 0x5217, '表': 0x8868, 
               '服': 0x670D, '处': 0x5904, '理': 0x7406, '本': 0x672C, '路': 0x8DEF}

font_codepoints = set()
for hexval, char in entries:
    cp = int(hexval, 16)
    font_codepoints.add(cp)
    if cp in problematic.values():
        print(f'  FOUND: U+{hexval} "{char}"')

for name, cp in problematic.items():
    if cp not in font_codepoints:
        print(f'  MISSING: "{name}" (U+{cp:04X})')
