with open('src/lv_font_mshy_16.c', 'r') as f:
    content = f.read()

problematic = ['试', '务', '动', '列', '表', '服', '处', '理', '待']
for ch in problematic:
    code = hex(ord(ch))
    pattern = f'/* U+{code[2:].upper()} "{ch}"'
    if pattern in content:
        print(f'Found {ch} ({code}) in font')
    else:
        print(f'MISSING {ch} ({code}) from font')
