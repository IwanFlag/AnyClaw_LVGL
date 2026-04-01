#!/usr/bin/env python3
"""Generate LVGL font with multiple -r args to get FORMAT0_TINY cmaps."""
import subprocess, os, re

CN = "中于件任作保信停储入关出列到刷前务动双取口号启器在地型基处天存安将已常开异当径待志态息户打找技拖择接搜操文新无日时显暂服未本术机板查标栈栏桌检模止正测源滤灰版状理电看知示移空窗端管索红绿置聊自色获行表装规言设词试语误账路输过运连退选配重错键闲面题黄"

# Build contiguous ranges that include all needed Chinese characters
cn_codepoints = sorted(set(ord(c) for c in CN))
ranges = []
start = cn_codepoints[0]; end = cn_codepoints[0]
for cp in cn_codepoints[1:]:
    if cp - end <= 1 and (cp - start + 1) <= 256:
        end = cp
    else:
        ranges.append((start, end))
        start = cp; end = cp
ranges.append((start, end))

# Split ranges > 256
final_ranges = []
for s, e in ranges:
    while e - s + 1 > 256:
        final_ranges.append((s, s + 255))
        s += 256
    final_ranges.append((s, e))

print(f"CJK ranges ({len(final_ranges)}):")
total_cjk = 0
for s, e in final_ranges:
    n = e - s + 1
    total_cjk += n
    print(f"  0x{s:04X}-0x{e:04X} ({n} chars)")
print(f"Total CJK positions: {total_cjk}")

# Build command with separate -r arguments
cwd = r"C:\Users\thundersoft\.openclaw\workspace\AnyClaw_LVGL"
output = r"src\lv_font_mshy_16.c"

cmd = [
    r"C:\Program Files\nodejs\npx.cmd", "lv_font_conv",
    "--format", "lvgl", "--size", "16",
    "--font", r"C:\Windows\Fonts\simhei.ttf",
    "--bpp", "4", "-o", output,
]

# Add each range as separate -r
for s, e in final_ranges:
    cmd.extend(["-r", f"0x{s:04X}-0x{e:04X}"])

# Add ASCII range
cmd.extend(["-r", "0x0020-0x007E"])

# Note: LVGL symbols (U+F000+) are not in simhei.ttf
# They were also missing in the original font

print(f"\nTotal -r arguments: {len(cmd) - 7}")
print("Running lv_font_conv...")

result = subprocess.run(cmd, capture_output=True, text=True, cwd=cwd, timeout=180)
if result.stdout:
    print("STDOUT:", result.stdout[:300])
if result.stderr:
    print("STDERR:", result.stderr[:300])
print("Return code:", result.returncode)

if result.returncode == 0:
    fpath = os.path.join(cwd, output)
    fsize = os.path.getsize(fpath)
    print(f"\nGenerated file size: {fsize} bytes")
    
    with open(fpath, 'r') as f:
        content = f.read()
    
    cmaps = re.findall(r'\.type = (LV_FONT_FMT_TXT_CMAP_\w+)', content)
    print(f"CMAP types: {cmaps}")
    list_lengths = re.findall(r'\.list_length = (\d+)', content)
    print(f"List lengths: {list_lengths}")
    cmap_count = content.count('.type = LV_FONT_FMT_TXT_CMAP_')
    print(f"Total cmaps: {cmap_count}")
    
    # Check coverage
    font_chars = set()
    for m in re.finditer(r'U\+([0-9A-F]+)', content):
        font_chars.add(int(m.group(1), 16))
    needed = set(ord(c) for c in CN)
    missing = needed - font_chars
    print(f"\nFont has {len(font_chars)} unique chars")
    print(f"App needs {len(needed)} Chinese chars")
    print(f"Missing: {len(missing)}")
    if missing:
        for c in sorted(missing):
            print(f"  {chr(c)} U+{c:04X}")
else:
    print("\nFont generation FAILED!")
