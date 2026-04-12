#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/*
 * ═══════════════════════════════════════════════════════════════
 *  AnyClaw LVGL — Centralized Configuration Constants
 *  All tunable parameters in one place. No more hardcoded values.
 * ═══════════════════════════════════════════════════════════════
 */

/* ── Window ─────────────────────────────────────────────────── */
#define WIN_DEFAULT_W           1450    /* Design width (logical px) */
#define WIN_DEFAULT_H           800     /* Design height (logical px) */
#define WIN_MIN_W               800     /* Minimum window width */
#define WIN_MIN_H               500     /* Minimum window height */

/* ── Title Bar ──────────────────────────────────────────────── */
#define TITLE_BAR_H_BASE        48      /* Title bar height (pre-SCALE) */
#define TITLE_BAR_ICON_PAD_L    8       /* Icon left padding */
#define TITLE_BAR_ICON_SIZE     48      /* Icon image size */
#define TITLE_BAR_LABEL_PAD_L   48      /* Title label left offset (after icon) */
#define TITLE_BAR_PAD_ALL       0       /* Title bar inner padding */
#define TITLE_BAR_BORDER_W      0       /* Title bar border width */

/* ── Left Navigation Bar ────────────────────────────────────── */
#define NAV_BAR_W_BASE          56      /* Left nav bar width (pre-SCALE) */
#define NAV_ICON_BTN_SIZE_BASE  40      /* Nav icon button size (pre-SCALE) */
#define NAV_ICON_BTN_RADIUS     10      /* Nav icon button corner radius */
#define NAV_ICON_GAP_BASE       4       /* Gap between nav buttons */
#define NAV_QUICK_AREA_H_BASE   96      /* Bottom quick-access area height (pre-SCALE) */

/* ── Title Bar Status Area ──────────────────────────────────── */
#define TITLE_LED_SIZE_BASE     10      /* Status LED size in title bar (pre-SCALE) */
#define TITLE_LED_GAP_BASE      16      /* Space before LED from right controls */
#define TITLE_MODEL_W_BASE      120     /* Model name label width in title bar (pre-SCALE) */

/* ── Panel Layout ───────────────────────────────────────────── */
#define LEFT_PANEL_DEFAULT_W    240     /* Left panel default width */
#define SPLITTER_W_BASE         6       /* Splitter bar width (pre-SCALE) */
#define PANEL_GAP_BASE          12      /* Gap between panels (pre-SCALE) */
#define PANEL_MIN_W             200     /* Minimum panel width */
#define PANEL_PAD_ALL           0       /* Panel inner padding */
#define PANEL_BORDER_W          1       /* Panel border width */
#define PANEL_RADIUS            12      /* Panel corner radius */

/* ── Spacing ────────────────────────────────────────────────── */
#define GAP_BASE                12      /* General spacing (pre-SCALE) */
#define CHAT_GAP_BASE           6       /* Chat area internal gap (pre-SCALE) */
#define CHAT_MSG_MARGIN_BASE    8       /* Chat message edge margin (pre-SCALE) */
#define TASK_ROW_GAP_BASE       12      /* Task list row gap (pre-SCALE) */

/* ── Left Panel Row ─────────────────────────────────────────── */
#define LP_ROW_H                30      /* Status row height */
#define LP_ROW_PAD_ALL          0       /* Row inner padding */
#define LP_ROW_BORDER_W         0       /* Row border width */
#define LP_ROW_GAP              6       /* Gap between items in a row */
#define LP_ROW_BG_OPA           LV_OPA_TRANSP  /* Row background opacity */

/* ── LED Indicator ──────────────────────────────────────────── */
#define LED_SIZE                14      /* LED indicator size (square) */
#define LED_DOT_SIZE            8       /* Task list status dot size */
#define LED_PAD_ALL             0       /* LED inner padding */
#define LED_BORDER_W            0       /* LED border width */

/* ── Task Item ──────────────────────────────────────────────── */
#define TASK_ITEM_H_BASE        32      /* Task item height (pre-SCALE) */
#define TASK_ITEM_PAD_ALL       4       /* Task item inner padding */
#define TASK_ITEM_BORDER_W      0       /* Task item border width */
#define TASK_ITEM_RADIUS        6       /* Task item corner radius */
#define TASK_ITEM_GAP           8       /* Gap between dot and name */
#define TASK_ADD_BTN_W          55      /* "+ Add" button width */
#define TASK_ADD_BTN_H          20      /* "+ Add" button height */
#define TASK_ADD_BTN_RADIUS     6       /* "+ Add" button radius */

/* ── Separator / Divider ────────────────────────────────────── */
#define SEP_H                   1       /* Horizontal separator height */
#define SEP_GAP                 12      /* Gap around separator */

/* ── Chat ───────────────────────────────────────────────────── */
#define CHAT_INPUT_DEFAULT_H    96      /* Input box default height (3 lines) */
#define CHAT_INPUT_MIN_H        96      /* Input box minimum height */
#define CHAT_INPUT_PAD_ALL      8       /* Input box inner padding */
#define CHAT_INPUT_BORDER_W     1       /* Input box border width */
#define CHAT_INPUT_RADIUS       8       /* Input box corner radius */
#define CHAT_BUBBLE_MAX_PCT     70      /* Max bubble width as % of container */
#define CHAT_BUBBLE_PAD_H       8       /* Bubble horizontal padding */
#define CHAT_BUBBLE_PAD_V       6       /* Bubble vertical padding */
#define CHAT_BUBBLE_RADIUS      10      /* Bubble corner radius */
#define CHAT_BUBBLE_BORDER_W    0       /* Bubble border width */
#define CHAT_STREAM_CHUNK       3       /* Chars per stream tick */
#define CHAT_STREAM_INTERVAL_MS 50      /* Stream timer interval (ms) */
#define CHAT_AVATAR_SIZE        24      /* AI avatar size */
#define CHAT_AVATAR_SIZE_USER   32      /* User avatar size */
#define CHAT_META_GAP           6       /* Gap in meta row (timestamp, name, avatar) */

/* ── Buttons ────────────────────────────────────────────────── */
#define BTN_SEND_SIZE_BASE      24      /* Send button size (pre-SCALE) */
#define BTN_UPLOAD_SIZE_BASE    24      /* Upload button size (pre-SCALE) */
#define BTN_PAD_ALL             0       /* Generic button inner padding */
#define BTN_RADIUS              6       /* Generic button corner radius */
#define BTN_BORDER_W            0       /* Generic button border width */
#define WC_BTN_SIZE_BASE        28      /* Window control button size (pre-SCALE) */
#define WC_BTN_GAP              6       /* Window control button gap */
#define WC_BTN_MARGIN           10      /* Window control button margin from edge */

/* ── Scroll ─────────────────────────────────────────────────── */
#define SCROLL_SPEED_FACTOR     20      /* Scroll multiplier per wheel tick */
#define SCROLL_BOTTOM_TOLERANCE 40      /* px tolerance for "near bottom" */

/* ── Interaction ────────────────────────────────────────────── */
#define DOUBLE_CLICK_MS         400     /* Double-click detection threshold */
#define UPLOAD_HOVER_CHECK_MS   80      /* Upload menu hover keep-alive interval */

/* ── Health & Refresh ───────────────────────────────────────── */
#define HEALTH_CHECK_DEFAULT_MS 30000   /* Default health check interval */
#define HEALTH_HTTP_TIMEOUT     3       /* HTTP health check timeout (sec) */
#define HEALTH_FAIL_THRESHOLD   3       /* Consecutive failures before Error */
#define SESSION_ACTIVE_AGE_MS   300000  /* Session considered active if ageMs < this (5 min) */

/* ── Gateway ────────────────────────────────────────────────── */
#define GATEWAY_PORT            18789   /* Default OpenClaw Gateway port */
#define GATEWAY_HEALTH_URL      "http://127.0.0.1:18789/health"

/* ── Logging ────────────────────────────────────────────────── */
#define LOG_MAX_LINES           200     /* Max UI log entries */
#define LOG_FILE_MAX_SIZE_MB    1       /* Log file rotation threshold (MB) */
#define LOG_MAX_FILES           5       /* Max rotated log files (app.log + N-1) */
#define LOG_CLEANUP_DAYS        7       /* Delete rotated logs older than N days */

/* ── Crash ──────────────────────────────────────────────────── */
#define CRASH_LOG_MAX_COUNT     10      /* Max crash log files to keep */

/* ── Chat History ───────────────────────────────────────────── */
#define CHAT_HISTORY_MAX        4096    /* Chat history buffer size */
#define CHAT_PERSIST_MAX        50      /* Max messages to persist */

/* ── Settings ───────────────────────────────────────────────── */
#define SETTINGS_TAB_PAD_ALL    8       /* Settings tab inner padding */
#define SETTINGS_TAB_GAP        10      /* Gap between settings items */
#define SETTINGS_ROW_H          32      /* Settings row height */
#define SETTINGS_ROW_PAD_H      8       /* Settings row horizontal padding */
#define SETTINGS_ROW_RADIUS     6       /* Settings row corner radius */

/* ── Dialog / Modal ─────────────────────────────────────────── */
#define DIALOG_PAD_ALL          16      /* Dialog inner padding */
#define DIALOG_RADIUS           12      /* Dialog corner radius */
#define DIALOG_BORDER_W         1       /* Dialog border width */
#define DIALOG_MIN_W            300     /* Dialog minimum width */
#define DIALOG_MIN_H            200     /* Dialog minimum height */

/* ── Dropdown ───────────────────────────────────────────────── */
#define DROPDOWN_PAD_H          8       /* Dropdown horizontal padding */
#define DROPDOWN_PAD_V          4       /* Dropdown vertical padding */
#define DROPDOWN_RADIUS         6       /* Dropdown corner radius */
#define DROPDOWN_BORDER_W       1       /* Dropdown border width */

/* ── Tooltip ────────────────────────────────────────────────── */
#define TOOLTIP_PAD_H           6       /* Tooltip horizontal padding */
#define TOOLTIP_PAD_V           4       /* Tooltip vertical padding */
#define TOOLTIP_RADIUS          4       /* Tooltip corner radius */
#define TOOLTIP_BORDER_W        0       /* Tooltip border width */

#endif /* APP_CONFIG_H */
