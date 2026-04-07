/* ═══════════════════════════════════════════════════════════════
 *  markdown.h - Simple Markdown to LVGL styled text renderer
 *  AnyClaw LVGL v2.0 - P2-20 Markdown support
 * ═══════════════════════════════════════════════════════════════ */
#ifndef MARKDOWN_H
#define MARKDOWN_H

#include "lvgl.h"
#include <cstdio>
#include <cstring>

/* Render markdown into an LVGL label with proper styling */
static void render_markdown_to_label(lv_obj_t* label, const char* md, const lv_font_t* font) {
    if (!label || !md) return;

    char html[8192];
    int hi = 0;
    int len = (int)strlen(md);

    for (int i = 0; i < len && hi < (int)sizeof(html) - 64; ) {
        /* Bold: **text** */
        if (md[i] == '*' && md[i+1] == '*') {
            i += 2;
            while (i < len && !(md[i] == '*' && md[i+1] == '*') && hi < (int)sizeof(html) - 2) {
                html[hi++] = md[i++];
            }
            if (i + 1 < len) i += 2; /* skip closing ** */
        }
        /* Code: `code` */
        else if (md[i] == '`') {
            i++;
            /* Add monospace indicator */
            while (i < len && md[i] != '`' && hi < (int)sizeof(html) - 2) {
                html[hi++] = md[i++];
            }
            if (i < len) i++; /* skip closing ` */
        }
        /* Headers: # text */
        else if (md[i] == '#' && (i == 0 || md[i-1] == '\n')) {
            /* Skip # symbols, use larger font for heading */
            int hashes = 0;
            while (i < len && md[i] == '#') { hashes++; i++; }
            while (i < len && md[i] == ' ') i++;
            /* Render heading text */
            while (i < len && md[i] != '\n' && hi < (int)sizeof(html) - 2) {
                html[hi++] = md[i++];
            }
            if (i < len) i++; /* skip newline */
        }
        /* List: - item */
        else if (md[i] == '-' && (i == 0 || md[i-1] == '\n') && md[i+1] == ' ') {
            i += 2;
            html[hi++] = '*'; /* bullet character */
            html[hi++] = ' ';
            while (i < len && md[i] != '\n' && hi < (int)sizeof(html) - 2) {
                html[hi++] = md[i++];
            }
            if (i < len) i++;
        }
        /* Links: [text](url) */
        else if (md[i] == '[') {
            i++;
            while (i < len && md[i] != ']' && hi < (int)sizeof(html) - 2) {
                html[hi++] = md[i++];
            }
            if (i < len) i++; /* skip ] */
            /* Skip (url) */
            if (i < len && md[i] == '(') {
                i++;
                while (i < len && md[i] != ')') i++;
                if (i < len) i++;
            }
        }
        /* Regular character */
        else {
            html[hi++] = md[i++];
        }
    }
    html[hi] = '\0';

    lv_label_set_text(label, html);
    if (font) lv_obj_set_style_text_font(label, font, 0);
}

#endif /* MARKDOWN_H */
