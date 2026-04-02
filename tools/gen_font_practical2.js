// Generate practical CJK font - most common characters only
const { execSync } = require('child_process');
const fs = require('fs');

const FONT_TTF = 'C:/Users/thundersoft/.openclaw/workspace/AnyClaw_LVGL/tools/msyh_face0.ttf';
const OUTPUT = 'C:/Users/thundersoft/.openclaw/workspace/AnyClaw_LVGL/src/lv_font_mshy_16.c';

// Try multiple ranges to find optimal coverage vs size
const ranges = [
    { name: 'Level 1 (0x4E00-0x5FFF)', range: '0x4E00-0x5FFF' },
    { name: 'Extended (0x4E00-0x67FF)', range: '0x4E00-0x67FF' },
];

// Start with smaller range
const range = ranges[0];
const cmd = `npx lv_font_conv --font "${FONT_TTF}" --format lvgl --size 16 --bpp 4 --no-compress --no-prefilter --force-fast-kern-format -r 0x20-0x7E -r 0x3000-0x303F -r 0xFF00-0xFFEF -r ${range.range} --lv-include lvgl/lvgl.h --lv-font-name lv_font_mshy_16 -o "${OUTPUT}"`;

console.log(`Running with range: ${range.name}...`);
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
    console.log(`\nDone! Glyphs: ${glyphCount}, File size: ${(fileSize / 1024).toFixed(1)} KB (${(fileSize / 1024 / 1024).toFixed(2)} MB)`);
    
    // Check specific characters we need
    const neededChars = ['题', '频', '风', '颜', '龙', '默', '色', '获', '虾', '蓝', '蒜', '蓉', '聊', '绿', '置'];
    const missing = neededChars.filter(ch => !content.includes(`"${ch.codePointAt(0).toString(16).toUpperCase().padStart(4, '0')}"`));
    if (missing.length > 0) {
        console.log('\nNote: Some UI chars may be missing:', missing.join(', '));
    } else {
        console.log('\nAll key UI characters appear to be covered!');
    }
} else {
    console.error('Output file not created!');
    process.exit(1);
}
