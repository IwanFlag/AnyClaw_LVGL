const fs = require('fs');
const path = require('path');

// We need to use lv_font_conv programmatically
// First, let's find the lv_font_conv installation
const lvFontConvPath = require.resolve('lv_font_conv');
console.log('lv_font_conv found at:', lvFontConvPath);

// Check if it exports a programmatic API
const lfc = require('lv_font_conv');
console.log('lv_font_conv exports:', Object.keys(lfc));
