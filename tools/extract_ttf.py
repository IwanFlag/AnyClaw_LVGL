from fontTools.ttLib import TTCollection, TTFont
import os

ttc_path = r"C:\Windows\Fonts\msyh.ttc"
ttc = TTCollection(ttc_path)
for i, font in enumerate(ttc.fonts):
    name = font["name"].getDebugName(4)
    print(f"{i}: {name}")
    out = f"C:\\Users\\thundersoft\\.openclaw\\workspace\\AnyClaw_LVGL\\tools\\msyh_face{i}.ttf"
    try:
        font.save(out)
        print(f"  -> saved to {out}")
    except Exception as e:
        print(f"  -> error: {e}")
