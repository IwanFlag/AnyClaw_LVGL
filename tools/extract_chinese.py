import os, re, sys
chinese_chars = set()
for root, dirs, files in os.walk(r'C:\Users\thundersoft\.openclaw\workspace\AnyClaw_LVGL'):
    # Skip build and tools dirs
    if 'build' in root or 'tools' in root:
        continue
    for f in files:
        if f.endswith(('.c', '.h', '.cpp')):
            path = os.path.join(root, f)
            try:
                with open(path, 'r', encoding='utf-8') as fh:
                    content = fh.read()
                for ch in content:
                    if '\u4e00' <= ch <= '\u9fff' or '\u3000' <= ch <= '\u303f' or '\uff00' <= ch <= '\uffef':
                        chinese_chars.add(ch)
            except:
                pass

sorted_chars = sorted(chinese_chars)
print(f"Total unique Chinese chars: {len(sorted_chars)}")
print("Chars:", ''.join(sorted_chars))

# Also output Unicode ranges needed
if sorted_chars:
    codepoints = [ord(c) for c in sorted_chars]
    print(f"\nCodepoint range: 0x{min(codepoints):04X} - 0x{max(codepoints):04X}")
