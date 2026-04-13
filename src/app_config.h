#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/*
 * ═══════════════════════════════════════════════════════════════
 *  AnyClaw LVGL — 布局配置：百分比分层
 *
 *  规则：每个元素的尺寸 = 父容器尺寸 × 百分比
 *  不写绝对像素，只写百分比。
 *  SCALE() 仅用于 DPI 缩放（字号、圆角等视觉细节）。
 *
 *  容器层级树：
 *    窗口 (WIN_W × WIN_H)                        ← root
 *    ├── 标题栏     = 100% × TITLE_H_PCT%
 *    └── 内容区     = 100% × (100 - TITLE_H_PCT)%
 *        ├── 左导航 = NAV_W_PCT% × 100%
 *        ├── 左面板 = LEFT_PANEL_PCT% × 100%      (of 可用宽度)
 *        └── 右面板 = 剩余 × 100%
 *            ├── 消息区 = 100% × CHAT_FEED_H_PCT%
 *            │   ├── AI头像  = CHAT_AVATAR_PCT% (of 消息区宽)
 *            │   └── 气泡    = CHAT_BUBBLE_PCT% (of 消息区宽)
 *            └── 输入区 = 100% × (100 - CHAT_FEED_H_PCT)%
 *                ├── 输入框 = 100% × CHAT_INPUT_H_PCT%
 *                └── 发送钮 = BTN_SEND_PCT% (of 输入区高)
 *
 *  命名约定：
 *    *_PCT   = 占父容器百分比 (0-100)
 *    *_MIN   = 最小像素值 (防崩溃)
 *    *_MS    = 毫秒
 *    其他     = SCALE'd 像素或纯常量
 * ═══════════════════════════════════════════════════════════════
 */


/* ╔═══════════════════════════════════════════════════════════╗
 * ║  §1  窗口 (root container)                                ║
 * ╚═══════════════════════════════════════════════════════════╝ */
#define WIN_DEFAULT_W           1450    /* 设计基准宽 */
#define WIN_DEFAULT_H           800     /* 设计基准高 */
#define WIN_MIN_W               800     /* 最小窗口宽 */
#define WIN_MIN_H               500     /* 最小窗口高 */


/* ╔═══════════════════════════════════════════════════════════╗
 * ║  §2  容器层级 — 全部是父容器的百分比                        ║
 * ╚═══════════════════════════════════════════════════════════╝ */

/* ── 标题栏 (parent: 窗口) ── */
#define TITLE_H_PCT             6       /* 高 = 窗口高 × 6% */
#define TITLE_H_MIN             36      /* 最小高 */

/* ── 左导航栏 (parent: 窗口) ── */
#define NAV_W_PCT               4       /* 宽 = 窗口宽 × 4% */
#define NAV_W_MIN               40      /* 最小宽 */
#define NAV_ICON_BTN_PCT        70      /* 图标按钮 = 导航宽 × 70% */
#define NAV_ICON_GAP_PCT        8       /* 按钮间距 = 导航宽 × 8% */
#define NAV_ICON_BTN_RADIUS     10      /* 圆角 (SCALE'd) */
#define NAV_QUICK_H_PCT         25      /* 底部快捷区高 = 内容区高 × 25% */

/* ── 左面板 (parent: 可用宽度 = 窗口宽 - 导航宽) ── */
#define LEFT_PANEL_PCT          25      /* 宽 = 可用宽 × 25% */
#define LEFT_PANEL_MIN_W        160     /* 最小宽 */

/* ── 右面板 = 可用宽 - 左面板 - 间距 ── */
#define RIGHT_PANEL_MIN_W       200     /* 最小宽 */

/* ── 消息区 (parent: 右面板) ── */
#define CHAT_FEED_H_PCT         85      /* 高 = 右面板高 × 85% */

/* ── 输入区 (parent: 右面板) ── */
#define CHAT_INPUT_H_PCT        60      /* 高 = 输入区高 × 60% (多行) */
#define CHAT_INPUT_MIN_H        96      /* 最小高 */
#define CHAT_INPUT_RADIUS       8       /* 圆角 (SCALE'd) */

/* ── Splitter ── */
#define SPLITTER_W_PCT          0.5     /* 宽 = 窗口宽 × 0.5% */
#define SPLITTER_W_MIN          3       /* 最小宽 */

/* ── 安全间距 ── */
#define SAFE_PAD_PCT            1       /* 通用内边距 = 窗口宽 × 1% */


/* ╔═══════════════════════════════════════════════════════════╗
 * ║  §3  视觉元素 — 全部是容器的百分比                          ║
 * ╚═══════════════════════════════════════════════════════════╝ */

/* ── 字号 (parent: 窗口高) ── */
/* 基准: 窗口高 800px → body 13px → 比率 1.625% */
#define FONT_DISPLAY_PCT       350      /* 品牌标题 = 窗口高 × 3.5% /100 */
#define FONT_H1_PCT            275      /* 弹窗标题 = 窗口高 × 2.75% /100 */
#define FONT_H2_PCT            225      /* 区域标题 = 窗口高 × 2.25% /100 */
#define FONT_H3_PCT            188      /* 卡片标题 = 窗口高 × 1.88% /100 */
#define FONT_BODY_PCT          163      /* 正文     = 窗口高 × 1.63% /100 */
#define FONT_SMALL_PCT         138      /* 次要信息 = 窗口高 × 1.38% /100 */
#define FONT_CAPTION_PCT       125      /* 标签     = 窗口高 × 1.25% /100 */
#define FONT_CODE_PCT          150      /* 代码     = 窗口高 × 1.5% /100 */

/* ── 字号最小值 (px) ── */
#define FONT_MIN_DISPLAY       20
#define FONT_MIN_H1            16
#define FONT_MIN_H2            14
#define FONT_MIN_H3            12
#define FONT_MIN_BODY          11
#define FONT_MIN_SMALL         10
#define FONT_MIN_CAPTION        9
#define FONT_MIN_CODE          10

/* ── 标题栏元素 (parent: 标题栏) ── */
#define TITLE_ICON_PCT         80       /* 品牌图标 = 标题栏高 × 80% */
#define TITLE_LED_PCT          18       /* LED = 标题栏高 × 18% */
#define TITLE_MODEL_W_PCT      10       /* 模型名宽 = 窗口宽 × 10% */
#define TITLE_ICON_PAD_L       8        /* 图标左内边距 (SCALE'd) */

/* ── 窗口控制按钮 (parent: 标题栏) ── */
#define WC_BTN_H_PCT           58       /* 按钮高 = 标题栏高 × 58% */
#define WC_BTN_ASPECT          100      /* 按钮宽 = 高 × 100% (正方形) */
#define MODE_BAR_W_PCT         12       /* 模式切换栏宽 = 窗口宽 × 12% */
#define MODE_BAR_W_MIN         120      /* 模式切换栏最小宽 */
#define SIDE_BTN_W_PCT         5        /* 设置按钮宽 = 窗口宽 × 5% */
#define SIDE_BTN_W_MIN         48       /* 设置按钮最小宽 */
#define WC_BTN_GAP_PCT         15       /* 按钮间距 = 按钮宽 × 15% */
#define WC_BTN_MARGIN_PCT      20       /* 右边距 = 按钮宽 × 20% */
#define WC_BTN_RADIUS          6        /* 圆角 (SCALE'd) */

/* ── Chat 气泡 (parent: 消息区) ── */
#define CHAT_BUBBLE_PCT        70       /* 最大宽 = 消息区宽 × 70% */
#define CHAT_BUBBLE_RADIUS     10       /* 圆角 (SCALE'd) */
#define CHAT_AVATAR_PCT        30       /* 头像 = 消息区宽 × 3% (计算后) */
#define CHAT_AVATAR_MIN        20       /* 头像最小 */
#define CHAT_AVATAR_MAX        32       /* 头像最大 */
#define CHAT_STREAM_CHUNK      3        /* 流式追加字数 */
#define CHAT_STREAM_INTERVAL_MS 50      /* 流式刷新间隔 (ms) */

/* ── Chat 输入区控件 (parent: 输入区) ── */
#define BTN_SEND_PCT           45       /* 发送按钮 = 输入区高 × 45% */
#define BTN_SEND_MIN           32       /* 发送按钮最小 */
#define BTN_UPLOAD_PCT         30       /* 上传按钮 = 输入区高 × 30% */
#define BTN_UPLOAD_MIN         24       /* 上传按钮最小 */

/* ── 左面板元素 (parent: 左面板) ── */
#define LP_ROW_H_PCT           8        /* 状态行高 = 左面板高 × 8% */
#define LP_ROW_H_MIN           24       /* 状态行最小高 */
#define LED_SIZE_PCT           40       /* LED = 行高 × 40% */
#define LED_DOT_PCT            25       /* 任务状态点 = 行高 × 25% */

/* ── 任务项 (parent: 左面板) ── */
#define TASK_ITEM_H_PCT        8        /* 任务项高 = 左面板高 × 8% */
#define TASK_ITEM_H_MIN        24       /* 任务项最小高 */
#define TASK_ITEM_RADIUS       6        /* 圆角 (SCALE'd) */


/* ╔═══════════════════════════════════════════════════════════╗
 * ║  §4  通用间距 — SCALE'd 或窗口百分比                        ║
 * ╚═══════════════════════════════════════════════════════════╝ */
#define GAP_PCT                 1       /* 通用间距 = 窗口宽 × 1% */
#define GAP_MIN                 4       /* 最小间距 */
#define CHAT_GAP_PCT            0.5     /* 消息区间距 = 消息区宽 × 0.5% */
#define CHAT_GAP_MIN            3
#define PANEL_BORDER_W          1       /* 面板边框 */
#define PANEL_RADIUS            12      /* 面板圆角 (SCALE'd) */


/* ╔═══════════════════════════════════════════════════════════╗
 * ║  §5  控件库 — 容器百分比                                    ║
 * ╚═══════════════════════════════════════════════════════════╝ */

/* ── 按钮 ── */
#define BTN_RADIUS              6       /* 通用按钮圆角 (SCALE'd) */
#define BTN_BORDER_W            0

/* ── 输入框 ── */
#define CHAT_INPUT_PAD_ALL      8       /* 输入框内边距 (SCALE'd) */
#define CHAT_INPUT_BORDER_W     1

/* ── 加载遮罩 (parent: 窗口) ── */
#define LOADING_OVERLAY_W_PCT   32      /* 宽 = 窗口宽 × 32% */
#define LOADING_OVERLAY_H_PCT   37      /* 高 = 窗口高 × 37% */
#define LOADING_ICON_PCT        21      /* 图标 = 遮罩高 × 21% */

/* ── 右键菜单项 (parent: 右键菜单) ── */
#define CTX_ITEM_H_PCT          8       /* 项高 = 左面板高 × 8% */
#define CTX_ITEM_H_MIN          28      /* 项高最小 */

/* ── 弹窗 (parent: 窗口) ── */
#define DIALOG_W_PCT            40      /* 宽 = 窗口宽 × 40% */
#define DIALOG_H_PCT            55      /* 高 = 窗口高 × 55% */
#define DIALOG_W_MIN            300
#define DIALOG_H_MIN            200
#define DIALOG_RADIUS           12      /* 圆角 (SCALE'd) */
#define DIALOG_PAD_ALL          16      /* 内边距 (SCALE'd) */
#define DIALOG_BORDER_W         1

/* ── 向导弹窗 (parent: 窗口) ── */
#define WIZARD_W_PCT            38      /* 宽 = 窗口宽 × 38% */
#define WIZARD_H_PCT            60      /* 高 = 窗口高 × 60% */
#define WIZARD_W_MIN            480
#define WIZARD_H_MIN            400

/* ── Dropdown ── */
#define DROPDOWN_RADIUS         6
#define DROPDOWN_BORDER_W       1

/* ── Tooltip ── */
#define TOOLTIP_RADIUS          4
#define TOOLTIP_BORDER_W        0


/* ╔═══════════════════════════════════════════════════════════╗
 * ║  §6  非视觉常量 (不随窗口变化)                              ║
 * ╚═══════════════════════════════════════════════════════════╝ */

/* ── 交互 ── */
#define DOUBLE_CLICK_MS         400
#define UPLOAD_HOVER_CHECK_MS   80

/* ── 健康检查 ── */
#define HEALTH_CHECK_DEFAULT_MS 30000
#define HEALTH_HTTP_TIMEOUT     3
#define HEALTH_FAIL_THRESHOLD   3
#define SESSION_ACTIVE_AGE_MS   300000

/* ── Gateway ── */
#define GATEWAY_PORT            18789
#define GATEWAY_HEALTH_URL      "http://127.0.0.1:18789/health"

/* ── 日志 ── */
#define LOG_MAX_LINES           200
#define LOG_FILE_MAX_SIZE_MB    1
#define LOG_MAX_FILES           5
#define LOG_CLEANUP_DAYS        7

/* ── 崩溃 ── */
#define CRASH_LOG_MAX_COUNT     10

/* ── 聊天记录 ── */
#define CHAT_HISTORY_MAX        4096
#define CHAT_PERSIST_MAX        50

/* ── 设置页 ── */
#define SETTINGS_TAB_PAD_ALL    8
#define SETTINGS_TAB_GAP        10
#define SETTINGS_ROW_H_PCT      4       /* 行高 = 设置面板高 × 4% */
#define SETTINGS_ROW_H_MIN      28
#define SETTINGS_ROW_RADIUS     6

/* ── 滚动 ── */
#define SCROLL_SPEED_FACTOR     20
#define SCROLL_BOTTOM_TOLERANCE_PCT  5  /* "接近底部"容差 = 容器高 × 5% */


/* ╔═══════════════════════════════════════════════════════════╗
 * ║  Backward compatibility aliases (_BASE → _MIN)            ║
 * ╚═══════════════════════════════════════════════════════════╝ */
#define TITLE_BAR_H_BASE    TITLE_H_MIN
#define PANEL_GAP_BASE      GAP_MIN
#define SPLITTER_W_BASE     SPLITTER_W_MIN
#define GAP_BASE            GAP_MIN
#define CHAT_GAP_BASE       CHAT_GAP_MIN
#define CHAT_MSG_MARGIN_BASE 8      /* chat message side margin (SCALE'd) */
#define LED_DOT_SIZE        6       /* task status dot px (SCALE'd) */
#define LP_ROW_H            LP_ROW_H_MIN
#define LP_ROW_BG_OPA       LV_OPA_TRANSP
#define LP_ROW_BORDER_W     0
#define LP_ROW_PAD_ALL      0
#define LP_ROW_GAP          4
#define NAV_QUICK_AREA_H_PCT NAV_QUICK_H_PCT

#endif /* APP_CONFIG_H */
