with open('src/lv_font_mshy_16.c') as f:
    c = f.read()

import re

start = c.find('static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[]')
brace_start = c.find('{', start)
depth = 0
for i, ch in enumerate(c[brace_start:], start=brace_start):
    if ch == '{': depth += 1
    elif ch == '}': depth -= 1
    if depth == 0:
        brace_end = i + 1
        break

dsc_text = c[brace_start:brace_end]
entries = re.findall(r'\{\.bitmap_index\s*=\s*(\d+),\s*\.adv_w\s*=\s*(\d+),\s*\.box_w\s*=\s*(\d+),\s*\.box_h\s*=\s*(\d+),\s*\.ofs_x\s*=\s*(-?\d+),\s*\.ofs_y\s*=\s*(-?\d+)\}', dsc_text)

bmp_start = c.find('static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[]')
bmp_data_start = c.find('{', bmp_start) + 1
bmp_data_end = c.find('};', bmp_data_start)
bmp_text = c[bmp_data_start:bmp_data_end]
hex_vals = re.findall(r'0x[0-9a-fA-F]+', bmp_text)
print('Total bitmap hex values:', len(hex_vals))

problematic = {'试': 15925, '务': 1281, '动': 1288, '列': 1143, '表': 15048, '服': 6509, '处': 2916, '理': 9830, '待': 4581}
working = {'运': 16944, '行': 15020, '中': 141, '本': 6540, '版': 9384, '端': 11599}

print('\n=== PROBLEMATIC ===')
for name, gid in problematic.items():
    e = entries[gid]
    print('  {} gid={}: idx={}, box={}x{}, adv={}'.format(name, gid, e[0], e[2], e[3], e[1]))

print('\n=== WORKING ===')
for name, gid in working.items():
    e = entries[gid]
    print('  {} gid={}: idx={}, box={}x{}, adv={}'.format(name, gid, e[0], e[2], e[3], e[1]))

print('\n=== DATA COMPARISON ===')
for name in ['务', '运', '中', '待']:
    gid = problematic.get(name) or working.get(name)
    e = entries[gid]
    bmp_idx = int(e[0])
    box_w = int(e[2])
    box_h = int(e[3])
    est_uncompressed = (box_w * box_h + 1) // 2
    data_bytes = len(hex_vals) - bmp_idx
    print('  {} ({}x{}): idx={}, est_min_bytes={}, avail_bytes={}'.format(name, box_w, box_h, bmp_idx, est_uncompressed, data_bytes))
