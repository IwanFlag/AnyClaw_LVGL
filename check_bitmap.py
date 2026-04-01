with open('src/lv_font_mshy_16.c') as f:
    c = f.read()

test_chars = ['试', '务', '动', '列', '表', '服', '处', '理', '待', '运', '行', '中']
for ch in test_chars:
    code = hex(ord(ch))
    comment = '/* U+{} "{}" */'.format(code[2:].upper(), ch)
    pos = c.find(comment)
    if pos >= 0:
        data_start = pos + len(comment)
        data = c[data_start:data_start+500].strip()
        lines = data.split('\n')[:4]
        hex_str = ' '.join(l.strip() for l in lines)[:150]
        print('{} data: {}'.format(ch, hex_str))
    else:
        print('{}: COMMENT NOT FOUND'.format(ch))
