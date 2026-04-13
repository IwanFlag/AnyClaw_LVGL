/* ═══════════════════════════════════════════════════════════════
 *  lang.h - Language management for AnyClaw LVGL
 *  P2-01: Single language mode per PRD 2.1
 * ═══════════════════════════════════════════════════════════════ */
#ifndef LANG_H
#define LANG_H

/* Language mode */
enum class Lang { CN, EN };

/* Global language variable */
extern Lang g_lang;

#endif /* LANG_H */
