const { execSync } = require('child_process');
const fs = require('fs');
const path = require('path');

const SCRIPT_DIR = __dirname;
const FONT_TTF = path.join(SCRIPT_DIR, 'msyh_face0.ttf').replace(/\\/g, '/');
const OUTPUT = path.join(SCRIPT_DIR, '..', 'src', 'lv_font_mshy_16.c').replace(/\\/g, '/');

// Extract Chinese chars from source files
const projectRoot = path.join(SCRIPT_DIR, '..');
const chineseSet = new Set();

function walkDir(dir) {
    const entries = fs.readdirSync(dir, { withFileTypes: true });
    for (const entry of entries) {
        const fullPath = path.join(dir, entry.name);
        if (entry.isDirectory()) {
            if (entry.name !== 'build' && entry.name !== 'tools') {
                walkDir(fullPath);
            }
        } else if (/\.(c|h|cpp)$/i.test(entry.name)) {
            const content = fs.readFileSync(fullPath, 'utf-8');
            for (const ch of content) {
                const code = ch.codePointAt(0);
                if ((code >= 0x4E00 && code <= 0x9FFF) ||
                    (code >= 0x3000 && code <= 0x303F) ||
                    (code >= 0xFF00 && code <= 0xFFEF)) {
                    chineseSet.add(ch);
                }
            }
        }
    }
}

walkDir(projectRoot);
const symbols = Array.from(chineseSet).sort().join('');
console.log(`Total unique Chinese chars needed: ${symbols.length}`);

// Call lv_font_conv via npx
const args = [
    '--font', FONT_TTF,
    '--format', 'lvgl',
    '--size', '16',
    '--bpp', '4',
    '--no-compress',
    '--no-prefilter',
    '--force-fast-kern-format',
    '-r', '0x20-0x7E',
    '-r', '0x3000-0x303F',
    '-r', '0xFF00-0xFFEF',
    '--symbols', symbols,
    '--lv-include', 'lvgl/lvgl.h',
    '--lv-font-name', 'lv_font_mshy_16',
    '-o', OUTPUT
];

console.log('Running lv_font_conv...');
try {
    execSync(`npx lv_font_conv ${args.map(a => `"${a}"`).join(' ')}`, {
        stdio: 'inherit',
        cwd: SCRIPT_DIR
    });
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
