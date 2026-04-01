import os
import re

# Check LVGL version
for line in open('CMakeLists.txt', encoding='utf-8'):
    if 'lvgl' in line.lower() or 'version' in line.lower():
        print(f'CMakeLists: {line.strip()}')

# Check if any files use LVGL version macros
for root, dirs, files in os.walk('src'):
    for fname in files:
        fpath = os.path.join(root, fname)
        with open(fpath, 'r', encoding='utf-8', errors='replace') as f:
            content = f.read()
        if 'LVGL_VERSION' in content or 'LV_VERSION' in content:
            matches = re.findall(r'LV(?:GL)?_VERSION[_A-Z]*\s*[= ]+\d+', content)
            for m in matches:
                print(f'{fpath}: {m}')

# Check thirdparty for LVGL headers
for root, dirs, files in os.walk('.'):
    for fname in files:
        if 'lv_version' in fname.lower() or 'lv_conf' in fname.lower():
            print(f'Found: {os.path.join(root, fname)}')
