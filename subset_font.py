#!/usr/bin/env python3
"""Extract only needed CJK characters from simhei.ttf into a tiny subset font."""
from fontTools.subset import Subsetter
from fontTools.ttLib import TTFont
import os

INPUT = r"C:\Windows\Fonts\simhei.ttf"
OUTPUT = r"thirdparty\cjk_subset.ttf"

chars = (
    "中于件作保停入关列到刷前动双取口号启器在地型处天存安将已常开当径待志态息户找技拖择接搜操文新无日时显暂服未本术机板查标栈栏桌检模止正测源滤灰版状理电知示移空窗端管索红绿置聊自色获行表装规言设词语误账路输过运连选配错键闲面题黄"
    "任务处理列表"
)

print(f"Extracting {len(chars)} characters from simhei.ttf...")
font = TTFont(INPUT)

subsetter = Subsetter()
subsetter.populate(text=chars)
subsetter.subset(font)

font.save(OUTPUT)
sz = os.path.getsize(OUTPUT)
print(f"Saved {OUTPUT} ({sz} bytes)")
