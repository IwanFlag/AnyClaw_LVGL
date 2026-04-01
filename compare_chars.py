with open('src/lv_font_mshy_16.c') as f:
    c = f.read()

chars = {'测': 'work', '试': 'bad', '务': 'bad', '动': 'bad', '列': 'bad', '表': 'bad', '服': 'bad', '处': 'bad', '理': 'work'}
for name, status in chars.items():
    code = hex(ord(name))
    comment = '/* U+{} "{}" */'.format(code[2:].upper(), name)
    pos = c.find(comment)
    if pos >= 0:
        data = c[pos+50:pos+300].strip()
        first_bytes = data[:80]
        print('{} ({}): {}'.format(name, status, first_bytes))
