// Generate font with compression enabled
const { execSync } = require('child_process');
const fs = require('fs');

const FONT_TTF = 'C:/Users/thundersoft/.openclaw/workspace/AnyClaw_LVGL/tools/msyh_face0.ttf';
const OUTPUT = 'C:/Users/thundersoft/.openclaw/workspace/AnyClaw_LVGL/src/lv_font_mshy_16.c';

// Try with compression enabled (removes --no-compress flag)
const cmd = `npx lv_font_conv --font "${FONT_TTF}" --format lvgl --size 16 --bpp 4 --force-fast-kern-format -r 0x20-0x7E -r 0x3000-0x303F -r 0xFF00-0xFFEF -r 0x4E00-0x9FFF --lv-include lvgl/lvgl.h --lv-font-name lv_font_mshy_16 -o "${OUTPUT}"`;

console.log('Running with compression...');
try {
    execSync(cmd, { stdio: 'inherit', maxBuffer: 100 * 1024 * 1024 });
} catch (e) {
    console.error('Failed:', e.message);
    process.exit(1);
}

if (fs.existsSync(OUTPUT)) {
    const content = fs.readFileSync(OUTPUT, 'utf-8');
    const glyphCount = (content.match(/U\+/g) || []).length;
    const fileSize = fs.statSync(OUTPUT).size;
    console.log(`\nDone! Glyphs: ${glyphCount}, File size: ${(fileSize / 1024).toFixed(1)} KB (${(fileSize / 1024 / 1024).toFixed(1)} MB)`);
} else {
    console.error('Output file not created!');
    process.exit(1);
}
