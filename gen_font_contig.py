#!/usr/bin/env python3
"""Generate LVGL font with one contiguous CJK range for FORMAT0_TINY."""
import subprocess, os, re

cwd = r"C:\Users\thundersoft\.openclaw\workspace\AnyClaw_LVGL"
output = r"src\lv_font_mshy_16.c"

# Use a single contiguous CJK range from U+4E00 to U+9FFF
# This is about 20480 chars. Each char at 16px with bpp=4 = ~128 bytes
# Total: ~2.5MB bitmap - large but workable

cmd = [
    r"C:\Program Files\nodejs\npx.cmd", "lv_font_conv",
    "--format", "lvgl", "--size", "16",
    "--font", r"C:\Windows\Fonts\simhei.ttf",
    "--bpp", "4", "-o", output,
    "-r", "0x0020-0x007E",     # ASCII
    "-r", "0x4E00-0x9FFF",     # Full CJK range (includes all needed chars)
]

print("Generating font with full CJK range (U+4E00-U+9FFF)...")
print("This will take a while and produce a large file...")
result = subprocess.run(cmd, capture_output=True, text=True, cwd=cwd, timeout=600)

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
    CN = "中于件任作保信停储入关出列到刷前务动双取口号启器在地型基处天存安将已常开异当径待志态息户打找技拖择接搜操文新无日时显暂服未本术机板查标栈栏桌检模止正测源滤灰版状理电看知示移空窗端管索红绿置聊自色获行表装规言设词试语误账路输过运连退选配重错键闲面题黄"
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
