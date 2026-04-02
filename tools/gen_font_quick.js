// Generate font with common CJK range - shorter command line
const { execSync } = require('child_process');
const fs = require('fs');
const path = require('path');

const FONT_TTF = 'C:/Users/thundersoft/.openclaw/workspace/AnyClaw_LVGL/tools/msyh_face0.ttf';
const OUTPUT = 'C:/Users/thundersoft/.openclaw/workspace/AnyClaw_LVGL/src/lv_font_mshy_16.c';

// Use CJK common character range - covers ~3755 Level 1 GB2312 chars
// GB2312 Level 1: 0x4E00-0x67FF (about 6000 chars, covers most common Chinese)
// Smaller range: 0x4E00-0x5FFF (about 8000 chars) 
// Even smaller: 0x4E00-0x56FF (about 5800 chars) - this is commonly used subset

const cmd = `npx lv_font_conv --font "${FONT_TTF}" --format lvgl --size 16 --bpp 4 --no-compress --no-prefilter --force-fast-kern-format -r 0x20-0x7E -r 0x3000-0x303F -r 0xFF00-0xFFEF -r 0x4E00-0x56FF --lv-include lvgl/lvgl.h --lv-font-name lv_font_mshy_16 -o "${OUTPUT}"`;

console.log('Running font generation with range 0x4E00-0x56FF...');
try {
    execSync(cmd, { stdio: 'inherit', cwd: __dirname, maxBuffer: 50 * 1024 * 1024 });
} catch (e) {
    console.error('Failed:', e.message);
    process.exit(1);
}

if (fs.existsSync(OUTPUT)) {
    const content = fs.readFileSync(OUTPUT, 'utf-8');
    const glyphCount = (content.match(/U\+/g) || []).length;
    const fileSize = fs.statSync(OUTPUT).size;
    console.log(`\nDone! Glyphs: ${glyphCount}, File size: ${(fileSize / 1024).toFixed(1)} KB`);
} else {
    console.error('Output file not created!');
    process.exit(1);
}
