with open('src/lv_font_mshy_16.c', 'r') as f:
    content = f.read()

import re

# Find all cmap blocks and extract their bitmap_index values
blocks = re.findall(r'\.bitmap_index\s*=\s*(\d+),\s*\.range_length\s*=\s*(\d+)', content)
print(f'Found {len(blocks)} cmap blocks')
for i, (idx, length) in enumerate(blocks):
    print(f'  Block {i}: index={idx}, length={length}')

# Find the unicode_list arrays
unicode_lists = re.findall(r'static const uint16_t unicode_list_\d+\[\]', content)
print(f'\nFound {len(unicode_lists)} unicode_list arrays')
for ul in unicode_lists:
    print(f'  {ul}')
