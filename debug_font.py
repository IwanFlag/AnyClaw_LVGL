with open('src/lv_font_mshy_16.c', 'r') as f:
    content = f.read()

import re

# Find all cmap blocks
blocks = [(int(s), int(l), int(g)) for s, l, g in re.findall(
    r'\.range_start\s*=\s*(\d+)\s*,\s*\.range_length\s*=\s*(\d+)\s*,\s*\.glyph_id_start\s*=\s*(\d+)',
    content
)]

# Problematic chars from screenshot
problematic = {
    '试': 0x8BD5, '务': 0x52A1, '动': 0x52A8, '列': 0x5217,
    '表': 0x8868, '服': 0x670D, '处': 0x5904, '理': 0x7406,
    '待': 0x5F85,
}

# Working chars from screenshot  
working = {
    '运': 0x8FD0, '行': 0x884C, '中': 0x4E2D,
    '版': 0x7248, '本': 0x672C, '路': 0x8DEF, '径': 0x5F84,
    '端': 0x7AEF, '口': 0x53E3, '服': 0x670D, '务': 0x52A1,
    '已': 0x5DF2, '连': 0x8FDE, '接': 0x63A5,
    '状': 0x72B6, '态': 0x6001,
}

print('=== Character to Block Mapping ===')
all_chars = {**problematic, **working}
for name, code in sorted(all_chars.items(), key=lambda x: x[1]):
    found_block = None
    for start, length, gid in blocks:
        end = start + length
        if start <= code < end:
            offset = code - start
            glyph_id = gid + offset
            found_block = (start, length, gid, offset, glyph_id)
            break
    if found_block:
        s, l, g, off, gid = found_block
        status = 'BAD' if name in problematic else 'ok'
        print(f'  {name} U+{code:04X} -> Block [{s}-{s+l-1}] offset={off} glyph_id={gid} [{status}]')
    else:
        print(f'  {name} U+{code:04X} -> NO BLOCK FOUND!')

# Check glyph_dsc count
dsc_match = re.search(r'\.cmap_num\s*=\s*(\d+)', content)
if dsc_match:
    print(f'\nCmap num: {dsc_match.group(1)}')
    
# Check total glyph_dsc entries
dsc_entries = re.findall(r'\{\.adv_w\s*=', content)
print(f'Total glyph_dsc entries: {len(dsc_entries)}')
