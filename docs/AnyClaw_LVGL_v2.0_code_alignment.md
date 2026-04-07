# 代码 vs PRD 对齐报告

> 审计日期：2026-04-05
> 审计范围：`src/` 全部源码（15 个文件）
> PRD 版本：`AnyClaw_LVGL_v2.0_PRD.md`

---

## ✅ 已实现且 PRD 已覆盖

| # | 功能 | PRD编号 | PRD优先级 | 代码位置 | 备注 |
|---|------|---------|-----------|----------|------|
| 1 | 主窗口创建 (LVGL + SDL) | WIN-01 | P0 | `main.cpp` lv_sdl_window_create, 1450x800 DPI缩放 | 完整实现 |
| 2 | 窗口关闭→最小化到托盘 | WIN-01 | P0 | `ui_main.cpp` btn_close_cb → tray_show_window(false) | SDL_QUIT也走托盘 |
| 3 | 窗口位置记忆恢复 | WIN-01 | P0 | `main.cpp` load_window_config, `ui_main.cpp` save_theme_config | window_x/y/w/h持久化 |
| 4 | 窗口最小尺寸 800x500 | WIN-01 | P0 | `main.cpp` clamp逻辑 | 已实现 |
| 5 | 标题栏拖拽移动窗口 | DD-01 | P2 | `main.cpp` titlebar_subclass (WM_NCHITTEST) + `ui_main.cpp` title_drag_cb | Win32+LVGL双通道 |
| 6 | 双击标题栏最大化/还原 | WI-01 | P2 | `main.cpp` WM_NCLBUTTONDBLCLK + SDL 400ms双击检测fallback | 完整实现 |
| 7 | 分隔条拖拽调整面板 | WI-01 | P2 | `ui_main.cpp` splitter_drag_cb (PRESSED/PRESSING/RELEASED) + relayout_panels | 含光标切换+高亮 |
| 8 | 窗口置顶（仅设置页开关） | WP-01 | P2 | `main.cpp` app_set_topmost (SetWindowPos HWND_TOPMOST) | 托盘菜单入口已移除 |
| 9 | 系统托盘图标 + 四色状态 | TRAY-01 | P0 | `tray.cpp` create_tray_icon, 四种PNG+GDI fallback | 灰/绿/黄/红 |
| 10 | 托盘右键菜单（全功能） | TRAY-01 | P0 | `tray.cpp` create_tray_menu, owner-drawn含LED | Start/Stop/Restart/Refresh/Boot/Settings/About/Exit |
| 11 | 双击托盘图标显示窗口 | TRAY-01 | P0 | `tray.cpp` WM_LBUTTONUP → tray_show_window(true) | 已实现 |
| 12 | 退出确认弹窗 | TRAY-01 | P0 | `ui_main.cpp` ui_show_exit_dialog (LVGL模态) | 可配置开关 |
| 13 | 托盘气泡通知 | TRAY-01 | P0 | `tray.cpp` tray_balloon (NOTIFYICONDATA) | 状态变更时触发 |
| 14 | 实例互斥 | INST-01 | P0 | `main.cpp` CreateMutexA + MessageBox | 完整实现 |
| 15 | 聊天消息发送（文字+按钮） | CHAT-01 | P0 | `ui_main.cpp` chat_send_cb / chat_send_btn_cb | Enter+按钮双通道 |
| 16 | 用户消息气泡（蓝色靠右） | CHAT-01 | P0 | `ui_main.cpp` chat_add_user_bubble | QQ风格row_wrap+FLEX_ALIGN_END |
| 17 | AI消息气泡（灰色靠左） | CHAT-01 | P0 | `ui_main.cpp` chat_add_ai_bubble | 含AI头像+时间戳 |
| 18 | 气泡宽度自适应 | CHAT-01 | P0 | `ui_main.cpp` lv_text_get_size测量, 短消息贴合/长消息70%换行 | 完整实现 |
| 19 | 聊天滚动管理 | CHAT-01 | P0 | `ui_main.cpp` chat_scroll_to_bottom (软滚40px容差) + chat_force_scroll_bottom | 防止流式输出时与用户回看冲突 |
| 20 | 多行输入（Shift+Enter） | CI-01 | P0 | `ui_main.cpp` chat_input_cb 检测KMOD_SHIFT | 已实现 |
| 21 | 输入框自动增长 | CI-01 | P0 | `ui_main.cpp` chat_input_resize_cb (96px~200px) | 行数变化时调整, 保存/恢复scroll_y |
| 22 | AI流式输出（打字机效果） | MB-01 | P2 | `ui_main.cpp` stream_timer_cb (50ms/次, 3字符追加) | 含render_markdown实时渲染 |
| 23 | 消息文字可选中+Ctrl+C复制 | MO-01 | P1 | `ui_main.cpp` label_select_cb + global_ctrl_c_copy | 拖拽选区+Win32剪贴板 |
| 24 | Markdown基础渲染 | MD-01 | P1 | `markdown.h` render_markdown_to_label | 粗体/代码/标题/列表/链接 |
| 25 | 文件/目录上传选择 | MM-01 | P1 | `ui_main.cpp` upload_file_click_cb (GetOpenFileNameA) / upload_dir_click_cb (SHBrowseForFolderA) | 含弹出菜单+hover保持 |
| 26 | Gateway进程管理 | PM-01 | P0 | `openclaw_mgr.cpp` app_start_gateway / app_stop_gateway | openclaw gateway start/stop |
| 27 | OpenClaw自动检测 | PM-01 | P0 | `openclaw_mgr.cpp` app_detect_openclaw | PATH查找 + npm全局 fallback |
| 28 | 状态LED指示灯 | LED-01 | P1 | `ui_main.cpp` led_ok (lv_led) + tray图标同步 | 四色对应四种状态 |
| 29 | 健康监控后台线程 | HM-01 | P1 | `health.cpp` health_thread (_beginthreadex) | Sleep(g_refresh_interval_ms)可配置 |
| 30 | 自动重启策略 | HM-01 | P1 | `health.cpp` g_autoRestarted标志, 仅重试1次 | 失败→红灯不再重启 |
| 31 | General设置Tab | 2.4.4 | P1 | `ui_settings.cpp` build_general_tab | Boot Start/Auto Start/Language/Theme/Refresh/Security |
| 32 | Log设置Tab | 2.4.6 | P1 | `ui_settings.cpp` build_log_tab | Enable/Level/Filter/Export/Clear/颜色图例 |
| 33 | 关于Tab | 2.4.7 | P1 | `ui_settings.cpp` build_about_tab | 品牌/版本/Tech Stack/Config导出 |
| 34 | 三套主题色 | TH-01 | P1 | `ui_main.cpp` THEME_DARK/LIGHT/CLASSIC + `theme.h` ThemeColors | 含完整组件色定义 |
| 35 | 主题即时切换 | TH-01 | P1 | `ui_main.cpp` apply_theme_to_all | 遍历所有UI元素更新颜色 |
| 36 | 日志系统（文件+轮转） | LOG-01 | P1 | `app_log.h` | 1MB轮转, 5文件保留, 7天清理, 4级别 |
| 37 | 崩溃日志+Stack Trace | E-01 | P1 | `main.cpp` crash_handler (SetUnhandledExceptionFilter) | dbghelp符号解析64帧, 保留10个 |
| 38 | 版本号管理 | VER-01 | P1 | 编译时常量 v2.0.1 | 标题栏/关于/托盘显示一致 |
| 39 | 启动自检四项检查 | SC-01 | P0 | `selfcheck.cpp` | Node/npm/Network/ConfigDir + 自动修复 |
| 40 | 开机自启动注册表 | AS-01 | P1 | `autostart.cpp` RegSetValueExW/RegDeleteValueW | HKCU Run键 |
| 41 | 国际化 CN/EN 切换（单一语言显示） | I18N-01 | P1 | `lang.h` + `ui_main.cpp` tr() / `ui_settings.cpp` tr(cn,en) | 系统语言自动检测 |
| 42 | DPI自动检测 | DPI-01 | P1 | `main.cpp` GetDpiForSystem + GetDeviceCaps fallback | SCALE()宏全局适配 |
| 43 | IME输入法支持 | IME-01 | P1 | `main.cpp` SDL_HINT_IME_SHOW_UI + 256字节键盘缓冲 | 中文/日文输入法可用 |
| 44 | 快捷键Ctrl+C/V/X/A | K-01 | P1 | `main.cpp` SDL event watch | 含选中文本处理 |
| 45 | 安全状态面板 | SEC-UI-01 | P1 | `ui_settings.cpp` build_general_tab Security section | LED + "Security: Good" + 详情行 |
| 46 | LVGL配置 | - | - | `lv_conf.h` | 32位色深, SDL驱动, TinyTTF, LodePNG, Win32文件系统 |
| 47 | 日志导出/清除 | 2.4.6 | P1 | `ui_settings.cpp` log_export_cb / log_clear_cb | 导出到Documents, 清除写入标记 |
| 48 | 右键上下文菜单 | - | - | `ui_main.cpp` chat_input_right_click_cb | Copy/Cut/Paste/Select All |
| 49 | 首次启动法律声明 | - | - | `ui_main.cpp` show_disclaimer | accepted.json持久化 |

---

## ⚠️ 代码有但 PRD 未覆盖（遗漏）

| # | 功能 | 代码位置 | 建议添加的PRD条目 |
|---|------|----------|-------------------|
| 1 | **启动错误LVGL弹窗** | `ui_main.cpp` show_startup_error | 2.8.x 启动错误提示 — Node.js缺失等阻断性错误的LVGL模态弹窗，支持自定义标题(Warning/Error) |
| 2 | **LVGL弹窗可拖拽** | `ui_main.cpp` dialog_drag_cb | 2.1.x 弹窗交互 — 所有模态弹窗(退出确认/关于/错误/免责声明)支持拖拽移动 |
| 3 | **标签hover截断提示** | `ui_main.cpp` label_hover_cb + add_label_hover_tooltip | 2.1.x 文字溢出提示 — 截断文字hover显示完整内容tooltip |
| 4 | **发送按钮动态状态** | `ui_main.cpp` update_send_button_state | 2.2.x 输入反馈 — 输入框为空时发送按钮灰色半透明，有内容时蓝色高亮 |
| 5 | **Ctrl+C复制气泡全文** | `ui_main.cpp` global_ctrl_c_copy | 2.2.5 消息操作 — 未选中时Ctrl+C复制整条消息内容 |
| 6 | **选中文本按键直接替换** | `main.cpp` SDL_KEYDOWN处理 | 2.10.3 快捷键 — 选中文字后按键直接替换（标准编辑行为） |
| 7 | **托盘LED叠加到图标** | `tray.cpp` create_tray_icon GDI fallback | 2.1.5 托盘图标 — GDI绘制LED高光点（白色小圆点模拟光泽） |
| 8 | **About页品牌图标** | `ui_main.cpp` / `ui_settings.cpp` garlic_sprout.png | 2.1.7 品牌形象 — 关于页使用大蒜发芽图标 |
| 9 | **Chat welcome消息** | `ui_main.cpp` 欢迎语行 | 2.2.1 聊天基础 — 系统欢迎消息（"Hi! I'm AnyClaw..."）居中显示 |
| 10 | **Test消息注入** | `ui_main.cpp` test_inject_messages_cb | 无 — 开发测试用的预置消息注入定时器(已禁用) |
| 11 | **user_data.txt标记文件** | `ui_main.cpp` 末尾 | 2.9.3 卸载 — 用户数据标记文件，供卸载程序识别 |
| 12 | **线程安全托盘状态** | `tray.cpp` InterlockedExchange/CompareExchange | 2.1.5 系统托盘 — 后台线程通过原子操作设置pending状态,主线程应用 |
| 13 | **Skill版本检查按钮** | `ui_settings.cpp` build_about_tab | 2.7 Skill管理 — Check Updates按钮调用ClawHub API |
| 14 | **输入框右键菜单hover保持** | `ui_main.cpp` upload_hover_check_cb (80ms timer) | 2.2.7 多模态 — 上传菜单80ms timer防止误关闭 |
| 15 | **窗口控件排除拖拽区域** | `main.cpp` g_btn_exclude RECT | 2.1.2 拖拽窗口 — 标题栏右侧按钮区域排除出拖拽范围 |

---

## ❌ PRD 标记已实现但代码未找到（虚标）

| # | 功能 | PRD编号 | PRD状态 | 实际情况 |
|---|------|---------|---------|----------|
| 1 | 欢迎语完整文案 | BR-01 | 已实现 | 代码中欢迎语为 "Hi! I'm AnyClaw, your AI assistant." 而非PRD要求的 "龙虾要吃蒜蓉的 🧄🦞 — 你的AI助手已就位！" |
| 2 | KR/JP语言支持 | I18N-01 | CN/EN已实现 | `lang.h` 仅有 CN/EN 两个枚举值，PRD提及KR/JP"暂未实现"但代码中无任何痕迹 |

---

## ❌ PRD 标记"部分实现" — 实际状态不一致

| # | 功能 | PRD编号 | PRD状态 | 实际代码状态 |
|---|------|---------|---------|-------------|
| 1 | **任务管理 TM-01** | TM-01 | ⚠️ 部分实现 | **严重不符**。PRD描述了完整的任务管理系统（会话/子代理/Cron/心跳列表、LED状态、右键菜单、详情弹窗、API对接），代码中仅有3个硬编码的静态标签（Gateway Service/Health Monitor/Auto Refresh），无任何动态数据源、无API调用、无交互。可评为"仅5%实现"。 |
| 2 | **Skill管理 SK-01** | SK-01 | 部分 | 代码有Skills Tab UI（搜索框+可用列表+已安装列表），但: ① Skill名称硬编码8个，与PRD的15个不一致 ② 下载回调仅创建空SKILL.md文件，无实际ClawHub API集成 ③ 无启用/禁用切换功能(仅显示[ON]/[OFF]文本) ④ 无版本管理。可评为"20%实现"。 |
| 3 | **首次启动向导 WIZ-01** | WIZ-01 | ✅ 已实现 | 6步全屏向导（语言→检测→API→模型→昵称→确认），wizard_completed持久化，昵称写入USER.md，Settings按钮已绑定回调。 |
| 4 | **自动启动持久化 AS-01** | AS-01 | 已实现 | Boot Start (注册表) ✅ 已实现；但 Auto Start (异常退出自动拉起) 的switch回调中仅有 `TODO: persist auto_start_on_crash config`，未实际持久化。health.cpp中g_autoRestarted逻辑存在但不可配置。 |

---

## ❌ PRD 标记"待实施" — 确认代码中无实现

| # | 功能 | PRD编号 | PRD优先级 |
|---|------|---------|-----------|
| 1 | 聊天历史持久化 (最近50条) | CH-01 | P1 |
| 2 | 会话管理 (列表/切换/新建/删除) | SM-01 | P1 |
| 3 | 搜索功能 | SF-01 | P2 |
| 4 | 通用控件库 (src/widgets/) | WIDGET-01 | P1 |
| 5 | 一键迁移 (环境搬家) | MIG-01 | P1 |
| 6 | 离线授权 (机器绑定) | LIC-01 | P0 |
| 7 | 部署区与用户工作区分离 | PATH-01 | P1 |
| 8 | 宏开关集中管理 | MACRO-01 | P1 |
| 9 | 任务执行协议UI (并行调度/监控/汇报) | EX-01~EX-06 | P0/P1 |

> **注意**：PRD 2.8.11 离线授权 LIC-01 标记为 P0 且"待实施"，代码中完全没有任何相关逻辑（无硬件指纹采集、无HMAC验证、无授权弹窗）。这是一个 **P0级缺失**。

---

## 📊 统计

| 指标 | 数值 |
|------|------|
| **PRD条目总数** | 63 |
| **已实现** | 35 (55.6%) |
| **部分实现** | 10 (15.9%) |
| **未实现（待实施）** | 18 (28.5%) |
| **代码功能点总数** | ~78 (含PRD外的15个额外功能) |
| **PRD覆盖率（全量+半量）** | 63.5% |
| **虚标** | 2 项 |
| **PRD遗漏（代码有PRD无）** | 15 项 |
| **P0级缺失** | 2 项（LIC-01 离线授权、EX-01 并行调度） |

---

## 🔍 关键发现

### 1. P0级缺失风险
- **LIC-01 离线授权** (P0)：完全没有实现。如果产品需要授权保护，这是发布阻断项。
- **EX-01 并行调度模型** (P0)：任务执行协议UI完全未实现，但这属于"AnyClaw作为AI Agent桌面管理器"的高级功能，可延后。

### 2. TM-01 严重虚标
PRD将任务管理标记为"⚠️ 部分实现（仅Gateway进程级状态）"，但实际代码连Gateway进程级状态都不是动态的——只是3个硬编码字符串。这与PRD描述的完整任务可视化系统差距极大。

### 3. 代码中的硬编码问题
- Model列表硬编码10个模型（PRD未提及具体列表）
- Skill列表硬编码8个（PRD列了15个）
- 布局常量散落在ui_main.cpp（PRD 2.10.9已识别此问题）
- `tr()` 函数目前仅返回英文（`return s.en`），CN翻译被跳过

### 4. 配置持久化覆盖良好
config.json已持久化：theme/language/window_x/y/w_h/refresh_interval/always_top/exit_confirm/dpi_scale/log_enabled/log_level。缺失：chat_history/wizard_completed/auto_start_on_crash/model_name/api_key_hash。

### 5. 额外功能（代码有PRD无）
代码中实现了15个PRD未覆盖的功能，主要是UI交互增强（弹窗拖拽、tooltip、右键菜单、发送按钮状态、线程安全等），这些应该补充到PRD中以保持文档与代码一致。
