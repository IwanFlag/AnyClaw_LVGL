#!/usr/bin/env python3
"""Generate LVGL font from Microsoft YaHei with full CJK coverage."""
import subprocess
import os
import sys

FONT_TTF = os.path.join(os.path.dirname(__file__), "msyh_face0.ttf")
OUTPUT = os.path.join(os.path.dirname(__file__), "..", "src", "lv_font_mshy_16.c")

# CJK ranges:
# 0x20-0x7E: ASCII
# 0x3000-0x303f: CJK Symbols and Punctuation
# 0x3040-0x309f: Hiragana (some UI may use)
# 0x30a0-0x30ff: Katakana
# 0xFF00-0xFFEF: Fullwidth Forms (！，．： etc.)
# 0x4E00-0x9FFF: CJK Unified Ideographs (main Chinese chars)
# 0xF900-0xFAFF: CJK Compatibility Ideographs

ranges = [
    "0x20-0x7E",
    "0x3000-0x303F",
    "0xFF00-0xFFEF",
    "0x4E00-0x9FFF",
    "0xF900-0xFAFF",
]

cmd = [
    "npx", "lv_font_conv",
    "--font", FONT_TTF,
    "--format", "lvgl",
    "--size", "16",
    "--bpp", "4",
    "--no-compress",
    "--no-prefilter",
    "--force-fast-kern-format",
]

for r in ranges:
    cmd.extend(["-r", r])

cmd.extend([
    "--lv-include", "lvgl/lvgl.h",
    "--lv-font-name", "lv_font_mshy_16",
    "-o", OUTPUT,
])

# Use shell=True with PowerShell on Windows since npx is a .ps1 script
print("Running:", " ".join(cmd))
result = subprocess.run(["powershell", "-Command", " ".join(cmd)], capture_output=True, text=True)
print("STDOUT:", result.stdout)
if result.stderr:
    print("STDERR:", result.stderr)

if result.returncode == 0:
    # Count glyphs
    with open(OUTPUT, 'r') as f:
        content = f.read()
    glyph_count = content.count("U+")
    file_size = os.path.getsize(OUTPUT)
    print(f"\nDone! Font file: {OUTPUT}")
    print(f"Glyphs: {glyph_count}")
    print(f"File size: {file_size / 1024:.1f} KB")
else:
    print(f"Failed with return code {result.returncode}")
    sys.exit(1)
