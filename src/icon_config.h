#ifndef ICON_CONFIG_H
#define ICON_CONFIG_H

/*
 * ═══════════════════════════════════════════════════════════════
 *  AnyClaw LVGL — Icon Size Definitions
 *  5-tier icon sizing for consistent visuals.
 * ═══════════════════════════════════════════════════════════════
 */

/* Icon size tiers (logical pixels, before SCALE) */
#define ICON_TINY       16      /* Tray minimum, inline small icons */
#define ICON_SMALL      20      /* Tray default, menu items */
#define ICON_MEDIUM     24      /* Chat avatar (AI), button icons */
#define ICON_LARGE      32      /* Chat avatar (user), title bar LVGL icon */
#define ICON_XLARGE     48      /* Taskbar icon, about page */

/* Chat avatars */
#define AVATAR_USER_SIZE    ICON_LARGE      /* 32px — user avatar */
#define AVATAR_AI_SIZE      ICON_MEDIUM     /* 24px — AI avatar */

/* Tray icons (file names reference these sizes) */
#define TRAY_ICON_DEFAULT   ICON_SMALL      /* 20px default tray */
#define TRAY_ICON_LARGE     ICON_LARGE      /* 32px large tray */

/* LED dot in task list — defined in app_config.h as LED_DOT_SIZE */

#endif /* ICON_CONFIG_H */
