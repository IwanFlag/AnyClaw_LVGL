#!/usr/bin/env python3
"""Generate LVGL font from Microsoft YaHei with UI-used Chinese characters + ASCII."""
import subprocess
import os

FONT_TTF = os.path.join(os.path.dirname(__file__), "msyh_face0.ttf")
OUTPUT = os.path.join(os.path.dirname(__file__), "..", "src", "lv_font_mshy_16.c")
SYMBOLS_FILE = os.path.join(os.path.dirname(__file__), "symbols.txt")

# Extract Chinese chars actually used in the UI source
chinese_chars = set()
project_root = r'C:\Users\thundersoft\.openclaw\workspace\AnyClaw_LVGL'
for root, dirs, files in os.walk(project_root):
    if 'build' in root or 'tools' in root:
        continue
    for f in files:
        if f.endswith(('.c', '.h', '.cpp')):
            path = os.path.join(root, f)
            try:
                with open(path, 'r', encoding='utf-8') as fh:
                    content = fh.read()
                for ch in content:
                    if '\u4e00' <= ch <= '\u9fff' or '\u3000' <= ch <= '\u303f' or '\uff00' <= ch <= '\uffef':
                        chinese_chars.add(ch)
            except:
                pass

symbols = ''.join(sorted(chinese_chars))
print(f"Total unique chars: {len(symbols)}")

# Write symbols to a file
with open(SYMBOLS_FILE, 'w', encoding='utf-8') as f:
    f.write(symbols)

cmd = (
    f'npx lv_font_conv --font "{FONT_TTF}" --format lvgl --size 16 --bpp 4'
    f' --no-compress --no-prefilter --force-fast-kern-format'
    f' -r 0x20-0x7E'
    f' -r 0x3000-0x303F'
    f' -r 0xFF00-0xFFEF'
    f' --symbols-file "{SYMBOLS_FILE}"'
    f' --lv-include lvgl/lvgl.h --lv-font-name lv_font_mshy_16'
    f' -o "{OUTPUT}"'
)

print("Running font generation...")
result = subprocess.run(["powershell", "-Command", cmd], capture_output=True, text=True)
print("STDOUT:", result.stdout)
if result.stderr:
    print("STDERR:", result.stderr[:1000])

if result.returncode == 0:
    with open(OUTPUT, 'r') as f:
        content = f.read()
    glyph_count = content.count("U+")
    file_size = os.path.getsize(OUTPUT)
    print(f"\nDone! Glyphs: {glyph_count}, File size: {file_size / 1024:.1f} KB")
else:
    print(f"Failed with return code {result.returncode}")
