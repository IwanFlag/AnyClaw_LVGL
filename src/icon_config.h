#ifndef ICON_CONFIG_H
#define ICON_CONFIG_H

/*
 * ═══════════════════════════════════════════════════════════════
 *  AnyClaw LVGL — Icon Size Definitions
 *  5-tier icon sizing for consistent visuals.
 * ═══════════════════════════════════════════════════════════════
 */

/* Icon size tiers (logical pixels, before SCALE) */
#define ANYCLAW_ICON_TINY       16      /* Tray minimum, inline small icons */
#define ANYCLAW_ICON_SMALL      20      /* Tray default, menu items */
#define ANYCLAW_ICON_MEDIUM     24      /* Chat avatar (AI), button icons */
#define ANYCLAW_ICON_LARGE      32      /* Chat avatar (user), title bar LVGL icon */
#define ANYCLAW_ICON_XLARGE     48      /* Taskbar icon, about page */

/* Chat avatars */
#define AVATAR_USER_SIZE    ANYCLAW_ICON_LARGE      /* 32px — user avatar */
#define AVATAR_AI_SIZE      ANYCLAW_ICON_MEDIUM     /* 24px — AI avatar */

/* Tray icons (file names reference these sizes) */
#define TRAY_ICON_DEFAULT   ANYCLAW_ICON_SMALL      /* 20px default tray */
#define TRAY_ICON_LARGE     ANYCLAW_ICON_LARGE      /* 32px large tray */

/* LED dot in task list — defined in app_config.h as LED_DOT_SIZE */

#endif /* ICON_CONFIG_H */
