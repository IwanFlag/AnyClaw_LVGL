#!/usr/bin/env python3
"""Generate LVGL font with practical Chinese character coverage."""
import subprocess
import os

SCRIPT_DIR = os.path.dirname(__file__)
FONT_TTF = os.path.join(SCRIPT_DIR, "msyh_face0.ttf")
OUTPUT = os.path.join(SCRIPT_DIR, "..", "src", "lv_font_mshy_16.c")

# Generate font in chunks to avoid command line length limits
# First: ASCII + common punctuation
ranges_basic = "0x20-0x7E 0x3000-0x303F 0xFF00-0xFFEF"

# Chinese chars actually used in UI - extract them
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
                        chinese_chars.add(ord(ch))
            except:
                pass

# Build ranges string for used chars
symbols_list = ''.join(chr(c) for c in sorted(chinese_chars))
print(f"Total unique Chinese chars needed: {len(symbols_list)}")

# Write symbols to temp file, then use Python to call lv_font_conv module directly
# Since CLI has length limits, let's use a different approach
# Use a reasonable CJK range that covers most common chars
# GB2312 Level 1 + Level 2 + common extended = ~8000 chars
# But to keep file size manageable, let's use a curated list

# Approach: Use node.js script to call lv_font_conv programmatically
node_script = f"""
const {{ execSync }} = require('child_process');
const fs = require('fs');
const path = require('path');

// Build the symbols string
const symbols = `{symbols_list}`;

const args = [
    '--font', '{FONT_TTF.replace(chr(92), '/')}',
    '--format', 'lvgl',
    '--size', '16',
    '--bpp', '4',
    '--no-compress',
    '--no-prefilter',
    '--force-fast-kern-format',
    '-r', '0x20-0x7E',
    '-r', '0x3000-0x303F',
    '-r', '0xFF00-0xFFEF',
    '--symbols', symbols,
    '--lv-include', 'lvgl/lvgl.h',
    '--lv-font-name', 'lv_font_mshy_16',
    '-o', '{OUTPUT.replace(chr(92), '/')}'
];

console.log('Running lv_font_conv with', symbols.length, 'Chinese chars...');
const cmd = `npx lv_font_conv ${{args.join(' ')}}`;
execSync(cmd, {{ stdio: 'inherit', shell: true }});
console.log('Done!');
"""

script_path = os.path.join(SCRIPT_DIR, 'gen_font_node.js')
with open(script_path, 'w', encoding='utf-8') as f:
    f.write(node_script)

print("Running via node...")
result = subprocess.run(['node', script_path], capture_output=True, text=True, cwd=SCRIPT_DIR)
print("STDOUT:", result.stdout)
if result.stderr:
    print("STDERR:", result.stderr[:1000])

if os.path.exists(OUTPUT):
    with open(OUTPUT, 'r') as f:
        content = f.read()
    glyph_count = content.count("U+")
    file_size = os.path.getsize(OUTPUT)
    print(f"\nDone! Glyphs: {glyph_count}, File size: {file_size / 1024:.1f} KB")
else:
    print("Output file not created!")
    if result.returncode != 0:
        print(f"Return code: {result.returncode}")
