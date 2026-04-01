#!/usr/bin/env python3
"""Check characters in cjk_subset.ttf"""
try:
    from fontTools.ttLib import TTFont
    font = TTFont('thirdparty/cjk_subset.ttf')
    cmap = font.getBestCmap()
    print(f"cjk_subset.ttf contains {len(cmap)} characters:")
    for cp in sorted(cmap.keys()):
        char = chr(cp)
        print(f"  U+{cp:04X} = {cp} '{char}'")
except ImportError:
    print("fontTools not installed, trying alternative...")
    import subprocess
    # Try using ttfdump or other tools
    import os
    print(f"File size: {os.path.getsize('thirdparty/cjk_subset.ttf')} bytes")
