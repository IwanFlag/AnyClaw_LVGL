#ifndef PATHS_H
#define PATHS_H

/*
 * ═══════════════════════════════════════════════════════════════
 *  AnyClaw LVGL — Path Definitions
 *  Three-zone separation: Deploy (read-only) / User (read-write) / OpenClaw (detect)
 * ═══════════════════════════════════════════════════════════════
 */

/* ── App Identity ───────────────────────────────────────────── */
#define APP_NAME                "AnyClaw_LVGL"
#define APP_MUTEX_NAME          "Global\\AnyClaw_LVGL_v2_SingleInstance"
#define APP_HTTP_AGENT          "AnyClaw_LVGL/1.0"
#define REG_AUTOSTART_VALUE     L"AnyClaw_LVGL"

/* ── User Data Directory (%APPDATA%\AnyClaw_LVGL\) ─────────── */
#define USERDATA_DIR_NAME       "AnyClaw_LVGL"

/* Config files */
#define FILE_CONFIG             "config.json"
#define FILE_ACCEPTED           "accepted.json"

/* Log directories */
#define DIR_LOGS                "logs"
#define DIR_CRASH_LOGS          "crash_logs"
#define FILE_APP_LOG            "anyclaw_app.log"

/* ── OpenClaw Detection ─────────────────────────────────────── */
#define OPENCLAW_CONFIG_DIR     "openclaw"
#define OPENCLAW_WORKSPACE      "workspace"

/* ── Asset Paths (relative to exe, with A: drive letter for LVGL FS) ── */
#define ASSET_APP_ICON          "A:assets/app_icon.png"
#define ASSET_GARLIC_ICON       "A:assets/garlic_icon.png"
#define ASSET_GARLIC_32         "A:assets/garlic_32.png"
#define ASSET_OC_ICON           "A:assets/oc_icon_24.png"
#define ASSET_DIR_TRAY          "A:assets/tray/"
#define ASSET_DIR_AI_ICONS      "A:assets/icons/ai/"

#endif /* PATHS_H */
