#!/usr/bin/env python3
"""
1. Create a custom TTF subset with exactly the 124 needed Chinese chars + ASCII + symbols
2. Generate LVGL font from the subset
"""
import subprocess, os, re
from fontTools.ttLib import TTFont

CN = "中于件任作保信停储入关出列到刷前务动双取口号启器在地型基处天存安将已常开异当径待志态息户打找技拖择接搜操文新无日时显暂服未本术机板查标栈栏桌检模止正测源滤灰版状理电看知示移空窗端管索红绿置聊自色获行表装规言设词试语误账路输过运连退选配重错键闲面题黄"

# All characters we need
all_chars = set()
# ASCII printable
for i in range(0x20, 0x7F):
    all_chars.add(i)
# Chinese characters
for c in CN:
    all_chars.add(ord(c))

print(f"Total characters needed: {len(all_chars)}")

# Create subset TTF
print("Creating subset TTF from simhei.ttf...")
src_font = TTFont(r"C:\Windows\Fonts\simhei.ttf")

# Get all unicode cmap
cmap = src_font.getBestCmap()

# Create subset
subset_chars = sorted(all_chars)
subset_glyphs = set()
for cp in subset_chars:
    if cp in cmap:
        subset_glyphs.add(cmap[cp])
    else:
        print(f"  WARNING: U+{cp:04X} ({chr(cp)}) not in simhei.ttf!")

# Use fonttools subset
from fontTools.subset import Subsetter, Options

options = Options()
options.layout_features = []
subsetter = Subsetter(options=options)
subsetter.populate(unicodes=subset_chars)
subsetter.subset(src_font)

# Save subset
subset_path = r"thirdparty\cjk_subset_124.ttf"
src_font.save(subset_path)
print(f"Subset saved: {subset_path}")

# Count glyphs in subset
subset_cmap = src_font.getBestCmap()
subset_unicode_count = len(subset_cmap) if subset_cmap else 0
print(f"Subset has {subset_unicode_count} characters")

# Clean up
del src_font

# Now generate LVGL font from the subset
cwd = r"C:\Users\thundersoft\.openclaw\workspace\AnyClaw_LVGL"
output = r"src\lv_font_mshy_16.c"

# Since the subset has exactly our characters, generate with the full range
# The subset only contains characters in our range, so the cmap will be clean
cmd = [
    r"C:\Program Files\nodejs\npx.cmd", "lv_font_conv",
    "--format", "lvgl", "--size", "16",
    "--font", os.path.join(cwd, subset_path),
    "--bpp", "4", "-o", output,
    "-r", "0x0020-007E",       # ASCII
    "-r", "0x4E00-0x9FFF",     # CJK range (subset only has our 124 chars)
]

print(f"\nGenerating LVGL font from subset...")
result = subprocess.run(cmd, capture_output=True, text=True, cwd=cwd, timeout=300)

if result.stdout:
    print("STDOUT:", result.stdout[:300])
if result.stderr:
    print("STDERR:", result.stderr[:300])
print("Return code:", result.returncode)

if result.returncode == 0:
    fpath = os.path.join(cwd, output)
    fsize = os.path.getsize(fpath)
    print(f"\nGenerated file size: {fsize} bytes ({fsize/1024:.0f} KB)")
    
    with open(fpath, 'r') as f:
        content = f.read()
    
    cmaps = re.findall(r'\.type = (LV_FONT_FMT_TXT_CMAP_\w+)', content)
    print(f"CMAP types: {cmaps}")
    list_lengths = re.findall(r'\.list_length = (\d+)', content)
    print(f"List lengths: {list_lengths}")
    range_lengths = re.findall(r'\.range_length = (\d+)', content)
    print(f"Range lengths: {range_lengths}")
    cmap_count = content.count('.type = LV_FONT_FMT_TXT_CMAP_')
    print(f"Total cmaps: {cmap_count}")
    
    # Check coverage
    needed = set(ord(c) for c in CN)
    font_chars = set()
    for m in re.finditer(r'U\+([0-9A-F]+)', content):
        font_chars.add(int(m.group(1), 16))
    missing = needed - font_chars
    print(f"\nFont has {len(font_chars)} unique chars")
    print(f"Missing needed chars: {len(missing)}")
    if missing:
        for c in sorted(missing):
            print(f"  {chr(c)} U+{c:04X}")
    else:
        print("ALL needed characters are present!")
else:
    print("\nFont generation FAILED!")
