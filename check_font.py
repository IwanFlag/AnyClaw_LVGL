import re

with open('src/lv_font_mshy_16.c', 'r', encoding='utf-8') as f:
    content = f.read()

# Find all unicode entries
entries = re.findall(r'/\*\s*U\+([0-9a-fA-F]+)\s+"([^"]+)"\s*\*/', content)
print(f'Font contains {len(entries)} characters:')
for hexval, char in entries:
    codepoint = int(hexval, 16)
    print(f'  U+{hexval} "{char}" (decimal: {codepoint})')

# Now find the glyph_desc array to understand the cmap structure
desc_match = re.findall(r'lv_font_fmt_txt_glyph_dsc_t\s+\w+\s*\[\s*\]', content)
print(f'\nGlyph desc arrays found: {desc_match}')

# Find the font_dsc or cmaps
cmap_matches = re.findall(r'cmaps\[\].*?\.unicode\s*=\s*(\d+)', content, re.DOTALL)
print(f'\nCmap unicode entries: {cmap_matches}')

# Look for the font descriptor structure
font_dsc = re.search(r'lv_font_fmt_txt_dsc_t\s+\w+\s*=\s*\{', content)
if font_dsc:
    print(f'\nFont descriptor found at pos {font_dsc.start()}')
    snippet = content[font_dsc.start():font_dsc.start()+500]
    print(snippet)
