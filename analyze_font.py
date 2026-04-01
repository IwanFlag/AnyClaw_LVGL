import re

with open('src/lv_font_mshy_16.c', 'r', encoding='utf-8') as f:
    content = f.read()

# Find the font_dsc_t structure
# Look for the font descriptor
print("=== Font Descriptor Structure ===")
# Find the main font descriptor
matches = list(re.finditer(r'lv_font_fmt_txt_dsc_t\s+(\w+)\s*=', content))
for m in matches:
    print(f"\nFont descriptor: {m.group(1)} at pos {m.start()}")
    # Get surrounding 1000 chars
    snippet = content[m.start():m.start()+1500]
    print(snippet)

# Find cmaps
print("\n\n=== CMAPS ===")
cmap_matches = list(re.finditer(r'lv_font_fmt_txt_cmap_t\s+\w+\s*\[\s*\]', content))
for m in cmap_matches:
    print(f"\nCMAP at pos {m.start()}")
    snippet = content[m.start():m.start()+500]
    print(snippet)
