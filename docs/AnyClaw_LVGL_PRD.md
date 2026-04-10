## 1. 功能清单

### 1.1 项目目录结构

```
AnyClaw_LVGL/
├── CMakeLists.txt              # CMake 构建配置
├── .gitignore                  # Git 忽略规则
├── src/                        # 源代码
│   ├── main.cpp                # 程序入口、窗口创建、主循环
│   ├── ui_main.cpp             # 主界面 UI（聊天区、左侧面板、标题栏）
│   ├── ui_settings.cpp         # 设置页 UI（General/Model/Skills/Log/About）
│   ├── tray.cpp / tray.h       # 系统托盘（图标、菜单、状态）
│   ├── health.cpp / health.h   # 健康检查（进程/端口检测）
│   ├── openclaw_mgr.cpp        # OpenClaw Gateway 进程管理
│   ├── http_client.cpp         # HTTP 客户端封装
│   ├── autostart.cpp           # 开机自启动（注册表）
│   ├── selfcheck.cpp / .h      # 启动自检
│   ├── app.h                   # 公共声明（SCALE、DPI、窗口）
│   ├── app_log.h               # 日志宏
│   ├── lang.h                  # 国际化定义
│   ├── theme.h                 # 主题色定义
│   ├── markdown.h              # Markdown 渲染
│   ├── lv_conf.h               # LVGL 配置
│   ├── lv_font_mshy_16.c       # CJK 字体（微软雅黑 16px）
│   ├── lv_font_simhei_16.c     # 备用字体（黑体）
│   ├── AnyClaw_LVGL.rc         # Windows 资源（图标嵌入）
│   └── AnyClaw_LVGL.manifest   # Windows 应用清单
├── assets/                     # 运行时资源
│   ├── app_icon.png            # 应用图标（512x512）
│   ├── garlic_icon.png         # 大蒜角色图标（用户头像）
│   ├── garlic_32.png           # 标题栏图标（32x32）
│   ├── garlic_48.png           # 标题栏图标（48x48）
│   ├── garlic_sprout.png       # 关于页图标
│   ├── oc_icon_24.png          # OpenClaw 图标（备用）
│   ├── garlic_icon.ico         # EXE 嵌入图标
│   ├── icons/ai/               # AI 头像选项（多方案 × 多尺寸）
│   └── tray/                   # 托盘图标（灰/绿/黄/红 × 16/20/32/48）
├── thirdparty/                 # 第三方库
│   ├── lvgl/                   # LVGL 图形库源码
│   └── SDL2/                   # SDL2 头文件 + MinGW 库
├── tools/                      # 构建脚本
│   ├── linux/                  # Linux 交叉编译（MinGW + SDL2 + Wine）
│   │   ├── setup-build-env.sh  # 一键安装 MinGW、Wine、Xvfb
│   │   ├── setup-sdl2-mingw.sh # 交叉编译 SDL2
│   │   ├── toolchain-mingw64.cmake # CMake 工具链
│   │   ├── build.sh            # 一键编译 + 打包
│   │   └── run-wine.sh         # Wine 测试
│   └── windows/                # Windows 原生编译（MSYS2）
│       └── build.bat
└── docs/                       # 文档
    ├── AnyClaw_LVGL_PRD.md            # 产品需求文档（本文档）
    ├── AnyClaw_LVGL_build_guide.md    # 编译部署指南
    └── AnyClaw_LVGL_task_log.md       # 任务日志
```

---

## 2. 需求规格

---

### 2.1 窗口与界面

#### 2.1.1 主窗口
- **功能编号：** WIN-01 **优先级：** P0
- SDL + LVGL 官方驱动，UI 布局设计尺寸 1450×800（逻辑像素），实际窗口尺寸基于屏幕分辨率，默认屏幕 75%×75%，上限为屏幕分辨率减去边距
- 窗口关闭按钮 → 最小化到托盘（非退出）：`SDL_QUIT` 事件触发 `tray_show_window(false)`，标题栏关闭按钮同理
- 最后一次关闭时的位置，重启后恢复（从 config.json 读取 window_x/window_y）
- 窗口大小自适应 DPI 缩放（启动时检测，不可运行时修改）
- 窗口最小尺寸限制 800x500

#### 2.1.2 拖拽窗口
- **功能编号：** DD-01 **优先级：** P2
- 标题栏按住拖拽移动窗口

#### 2.1.3 窗口交互优化
- **功能编号：** WI-01 **优先级：** P2
- 双击标题栏最大化/还原（通过 Win32 `WM_NCLBUTTONDBLCLK` 子类化处理 + SDL 层 400ms 双击检测 fallback）
- 分隔条拖拽调整左右面板比例：
  - `PRESSED` → 记录起始 x 坐标，高亮分隔条颜色，切换鼠标为 resize 光标
  - `PRESSING` → 计算 dx 偏移，累加到 `LEFT_PANEL_W`，调用 `relayout_panels()` 实时刷新
  - `RELEASED` → 重置分隔条颜色和光标，写日志
  - `relayout_panels()` 同步更新左面板所有子控件的 x 坐标和宽度（version_label、port_label、task_items 等）

#### 2.1.4 窗口置顶与锁定
- **功能编号：** WP-01 **优先级：** P2
- 已移除托盘菜单入口，仅保留设置页开关

#### 2.1.5 系统托盘
- **功能编号：** TRAY-01 **优先级：** P0
- 常驻托盘图标，四种状态颜色（白色=空闲/绿色=运行中/红色=异常/黄色=检测中）
- 托盘菜单（Win32 `CreatePopupMenu` + `Shell_NotifyIcon`）：
  - 状态行：显示 OpenClaw 状态 + LED 颜色（自绘 Owner-Draw 菜单项）
  - Start/Stop Gateway
  - Restart OpenClaw
  - Refresh Status
  - Boot Start 开关
  - Open Settings
  - About
  - Exit
- 双击托盘图标：`tray_show_window(true)` 显示主窗口
- 退出确认：
  - 如 `is_exit_confirmation_enabled()` 为 true → 调用 `ui_show_exit_dialog()`（LVGL 模态弹窗）
  - 用户确认 → `tray_request_quit()` 设置 `g_shouldQuit = true`
  - 如关闭确认 → 直接设置 `g_shouldQuit = true`
- 主循环每帧检查 `tray_should_quit()`，退出时先 `save_theme_config()` 保存配置
- 托盘气泡通知：`tray_balloon(title, message, timeout_ms)` 用于状态变更通知

#### 2.1.6 图标系统
- **功能编号：** ICON-01 **优先级：** P1

| 功能 | LVGL 图标          |
| ---- | ------------------ |
| 启动 | LV_SYMBOL_PLAY     |
| 停止 | LV_SYMBOL_STOP     |
| 刷新 | LV_SYMBOL_REFRESH  |
| 设置 | LV_SYMBOL_SETTINGS |
| 日志 | LV_SYMBOL_FILE     |
| 首页 | LV_SYMBOL_HOME     |
| 编辑 | LV_SYMBOL_EDIT     |
| 关闭 | LV_SYMBOL_CLOSE    |

- 大蒜角色图标（用户头像、标题栏、关于页）
- AI 头像多方案（lobster 系列 + ai 系列，24/32/48 三尺寸）
- 托盘图标：预绘制合体 PNG（大蒜+LED），灰/绿/黄/红 × 16/20/32/48

#### 2.1.7 品牌形象
- **功能编号：** BR-01 **优先级：** P1
- 欢迎语：龙虾要吃蒜蓉的 🧄🦞 — 你的AI助手已就位！
- 关于对话框：品牌信息 / Tech Stack

#### 2.1.8 实例互斥
- **功能编号：** INST-01 **优先级：** P0
- 通过 Windows Mutex (`Global\AnyClaw_LVGL_v2_SingleInstance`) 防止重复启动
- 检测到已有实例运行 → 弹出原生 MessageBox 提示 "Already Running"，退出程序
- 进程退出时自动释放 Mutex

---

### 2.2 聊天功能

#### 2.2.1 聊天基础
- **功能编号：** CHAT-01 **优先级：** P0
- **架构：** 聊天走 OpenClaw Gateway `/v1/chat/completions`（OpenAI 兼容），不直连 LLM API
- **通信流程：** AnyClaw → `POST http://127.0.0.1:18789/v1/chat/completions`（带 Gateway auth token）→ Gateway → Agent → LLM
- Gateway token 自动从 `openclaw.json` 的 `gateway.auth.token` 读取
- model 固定为 `openclaw:main`，由 Gateway 路由到配置的 Agent
- 显示格式：`[HH:MM:SS] [角色] 内容`
- 发送消息：输入文字，回车或点击发送按钮触发 `chat_send_cb` → `chat_add_user_bubble(text)`
- 消息角色区分：用户消息靠右蓝色气泡，AI 回复靠左灰色气泡
- 气泡宽度自适应：短消息贴合文字宽度（`lv_text_get_size` 测量），长消息 70% 容器宽度换行
- 对话历史滚动：
  - 新消息发送 → `chat_force_scroll_bottom()` 强制滚到底
  - 流式输出中 → `chat_scroll_to_bottom()` 软滚（40px 容差，用户回看历史时不强制拉回）
  - 手动滚动 → 滚轮仅在 `chat_cont` 和 `chat_input` 上生效，方向为自然方向

#### 2.2.2 聊天输入框
- **功能编号：** CI-01 **优先级：** P0 **状态：** ✅ 已实现
- 支持多行输入（Shift+Enter 换行），Ctrl+V 粘贴
- 输入框自动增长：`chat_input_resize_cb` 监听 `LV_EVENT_VALUE_CHANGED`，追踪 `g_input_line_count`，仅行数真正变化时才调整高度
- 输入框默认高度 96px（3 行），最小高度 96px
- 高度变化时保存/恢复 `chat_cont` 的 `scroll_y`，防止布局跳动

#### 2.2.3 聊天输出区域
- **功能编号：** CO-01 **优先级：** P0
- 聊天区域顶部延伸至窗口顶部，最大化消息展示空间
- 滚动条仅作用于 chat_cont 和 chat_input

#### 2.2.4 消息气泡增强
- **功能编号：** MB-01 **优先级：** P2
- AI 回复流式显示（打字机效果）：
  - `stream_timer_cb` 每次追加 3 个字符到 `g_stream_buffer`
  - 调用 `render_markdown_to_label` 实时渲染 Markdown 格式
  - 流式过程中 `chat_scroll_to_bottom()` 软滚（用户回看历史时不强制拉回）
  - 流式完成后将内容追加到 `chat_history`
- 短消息气泡贴合文字宽度：`lv_text_get_size` 测量文字自然宽度，与 70% 容器宽度比较
  - 文字宽度 < 70%：`label = SIZE_CONTENT`，气泡贴合内容
  - 文字宽度 ≥ 70%：`label = LV_PCT(100)` + `inner = 70%`，气泡换行
- QQ 风格布局：消息自动上下堆叠，AI 靠左、用户靠右（`row_wrap` + `FLEX_ALIGN_START/END`）

#### 2.2.5 消息操作
- **功能编号：** MO-01 **优先级：** P1
- 消息文字可选中（鼠标拖拽选区）
- 选中后 Ctrl+C 复制到系统剪贴板（Win32 跨窗口可用）
- 未选中时 Ctrl+C 复制整条消息内容
- 消息时间戳显示

#### 2.2.6 Markdown 渲染
- **功能编号：** MD-01 **优先级：** P1
- 代码块、加粗、列表等基础格式
- CJK 逐字换行（LV_TEXT_FLAG_BREAK_ALL）

#### 2.2.7 多模态输入 & 文件上传
- **功能编号：** MM-01 **优先级：** P1
- 上传按钮（灰色默认，悬停蓝色）+ 弹出菜单（File / Dir）
- 菜单 hover 保持：80ms timer 检测鼠标是否在按钮或菜单上方，防止误关闭
- Win32 文件选择对话框：
  - 文件：`GetOpenFileNameA`（COMDLG32）
  - 目录：`SHBrowseForFolderA`（SHELL32 + OLE32）

#### 2.2.8 聊天历史持久化
- **功能编号：** CH-01 **优先级：** P1 **状态：** ✅ 已实现
- 最近 50 条消息持久化，重启后可查看
- 聊天历史清除功能

#### 2.2.9 会话管理
- **功能编号：** SM-01 **优先级：** P1 **状态：** ✅ 已实现
- 活跃会话列表查看（渠道/Agent/时长）
- 单个会话终止（Abort 按钮）
- 所有会话重置

#### 2.2.10 搜索功能
- **功能编号：** SF-01 **优先级：** P2 **状态：** ✅ 已实现
- 聊天记录搜索

---

### 2.3 OpenClaw 集成

#### 2.3.1 进程管理
- **功能编号：** PM-01 **优先级：** P0
- 一键启动、停止、重启 OpenClaw Gateway（`openclaw gateway start/stop`）
- 进程检测：`CreateToolhelp32Snapshot` 遍历进程列表，查找 `node.exe`
- OpenClaw 自动检测策略：
  - 策略 1：`where openclaw` PATH 查找 → 读取 `openclaw.json` 获取端口
  - 策略 2：npm 全局路径 `%APPDATA%\npm\node_modules\openclaw\openclaw.cmd`
- 状态判断：进程存在 + HTTP 200 = Running / 进程存在 + HTTP 非 200 = Error / 进程不存在 + 已检测到安装 = Detected / 未安装 = NotInstalled

#### 2.3.2 状态指示灯 LED
- **功能编号：** LED-01 **优先级：** P1

**三级统一规范**（LED / 托盘图标 / Task List 圆点 三者一致）：

LED 语义为**链路可达性**，反映用户能否正常与大模型交互，而非仅 Gateway 进程状态。

| 状态 | 颜色 | RGB | LVGL LED | 托盘图标 | 含义 | 用户理解 |
|------|------|-----|----------|---------|------|---------|
| Ready — 就绪 | 🟢 绿 | (0, 220, 60) | LED on | tray_green | Gateway 在 + HTTP 通 + Agent 空闲 | ✅ 可以发消息 |
| Busy — 处理中 | 🟡 黄 | (220, 200, 40) | LED on | tray_yellow | Gateway 在 + 有活跃 Session（ageMs < 5min） | ⏳ 模型在回复中 |
| Checking — 检测中 | 🟡 黄 | (220, 200, 40) | LED on | tray_yellow | 启动中 / 首次轮询 | 🔄 正在检测 |
| Error — 异常 | 🔴 红 | (220, 40, 40) | LED on | tray_red | Gateway 进程不在 / HTTP 不通 | ❌ 无法连接 |

**故障原因显示**：状态为 Error 时，状态文字显示具体原因：
- Gateway 进程未运行 → `"Gateway 未运行"`
- HTTP 端口无响应 → `"端口无响应"`
- 连接超时 → `"连接超时"`

状态文字支持 hover 显示完整原因（LVGL `LV_LABEL_LONG_MODE_DOTS` + tooltip）。

- 状态栏文字颜色与 LED 一致
- 托盘图标 LED 颜色同步变化
- Task List 中每个 Session/Cron 条目前的小圆点使用相同颜色（绿/黄/红/白）

#### 2.3.3 健康监控
- **功能编号：** HM-01 **优先级：** P1

**检测频率：** 每 30 秒（可配置 5s / 15s / 30s / 60s / 120s），通过 `g_refresh_interval_ms` 全局变量控制，Settings 下拉框修改后即时生效

**检测方式：** 后台线程 `_beginthreadex(health_thread)`，每轮 `Sleep(g_refresh_interval_ms)`

**检测内容：**
- 进程检测：`is_node_running()` — `CreateToolhelp32Snapshot` 遍历进程列表
- 端口检测：`http_get("http://127.0.0.1:18789/health")` — HTTP 端口是否响应
- Session 检测：HTTP 通时，调 `openclaw gateway call sessions.list --json`，检查是否有 ageMs < 5min 的活跃 Session

**LED 状态判定逻辑：**

```
Gateway 进程不在                  → 🔴 Error（"Gateway 未运行"）
Gateway 在 + HTTP 不通            → 🔴 Error（"端口无响应"）
Gateway 在 + HTTP 通 + 首次轮询   → 🟡 Checking
Gateway 在 + HTTP 通 + 有活跃Session → 🟡 Busy
Gateway 在 + HTTP 通 + 无活跃Session  → 🟢 Ready
```

**失败处理：**
- 节点不在 + HTTP 不通 → 尝试自动重启 1 次，托盘变黄
- 节点在 + HTTP 不通 → 失败计数 +1
  - 连续失败 < 3 次 → 视为正常，等待恢复
  - 连续失败 ≥ 3 次 → 托盘变红（Error）
- HTTP 恢复 → 计数清零，托盘变绿

**自动重启策略：**
- 进程不存在 + HTTP 无响应 → 自动执行 openclaw gateway start，仅重试 1 次
- 重启后 HTTP 恢复 → 正常
- 重启后仍失败 → 托盘变红，不再自动重启

#### 2.3.4 任务管理
- **功能编号：** TM-01 **优先级：** P1 **状态：** 🔧 实施中（v2.0）

AnyClaw 作为 OpenClaw 桌面管理器，提供任务可视化管理与用户干涉能力。

---

##### 2.3.4.1 架构概述

**核心约束**：Gateway 不提供 REST API，所有数据通过 CLI 子进程调用获取。

| 层 | 方式 | 频率 |
|----|------|------|
| 读（轮询） | `openclaw sessions --json` + `openclaw cron list --json` | 每 30s（跟随 `g_refresh_interval_ms`） |
| 写（用户触发） | `openclaw cron enable/disable/run/rm/add` + `gateway call sessions.reset` | 点击时 |

**数据获取方式**：

| 数据源 | 命令 | 输出格式 | 实测耗时 |
|--------|------|---------|---------|
| 健康状态 | `curl http://127.0.0.1:18789/health` | JSON `{"ok":true}` | ~50ms |
| 会话列表 | `openclaw gateway call sessions.list --json` | JSON | ~1.5s |
| 定时任务 | `openclaw cron list --json` | JSON | ~1.5s |

- Win32 实现：`CreateProcess` + 匿名管道捕获 stdout，JSON 解析
- 新建 `src/task_manager.cpp`：封装 CLI 调用、stdout 捕获、JSON 解析、数据缓存
- **不使用** `openclaw status`（文本表格格式不稳定，跨版本解析风险大）

---

##### 2.3.4.2 任务类型与生命周期

**任务类型：**

| 类型 | 来源 | 生命周期 | Session Key 前缀 |
|------|------|---------|-----------------|
| Gateway 进程 | 系统 | 常驻 | — |
| 会话 (Session) | 用户/AI 对话 | 活跃→休眠（不删除，ageMs 增长） | `agent:<agentId>:*` |
| 子代理 (Sub-Agent) | 主会话派生 | spawn→running→completed/failed | `subagent:*` |
| 定时任务 (Cron) | 用户配置 | 常驻（周期触发） | `cron:*` |

**Session 生命周期**：Session 不会"完成"，只会变冷。
```
用户发消息 → session 活跃 → AI 回复 → 用户离开 → session 休眠（ageMs 增长）
```

**Sub-Agent 生命周期**：有明确的开始和结束。
```
spawn → running → completed/failed → 变成历史记录
```

**Cron 生命周期**：永远重复，除非手动删除。
```
配置创建 → 每隔 N 时间触发 → 执行 → 等待 → 再次触发
```

---

##### 2.3.4.3 任务来源追溯

每条任务标注来源信息，用户可直观看到"这个任务是从哪里来的"。

**Session 数据中的来源字段**：

```json
{
  "key": "agent:main:feishu:grp_xxx",
  "agentId": "main",
  "origin": {
    "provider": "feishu",
    "surface": "feishu",
    "chatType": "group"
  },
  "lastChannel": "feishu",
  "deliveryContext": { "channel": "feishu" }
}
```

**来源信息对应关系**：

| 字段 | 含义 | 示例值 |
|------|------|--------|
| `agentId` | 哪个 Agent | `main`, `data-bot` |
| `origin.provider` | 哪个渠道 | `webchat`, `telegram`, `discord`, `feishu`, `wechat` |
| `origin.chatType` | 私聊/群聊 | `direct`, `group` |
| Session Key 前缀 | 任务类型 | `agent:` / `subagent:` / `cron:` |

**UI 来源展示格式**：

| Session Key | 渠道 | UI 显示 |
|-------------|------|---------|
| `agent:main:main` | webchat | main · WebChat · Direct |
| `agent:main:telegram:12345` | telegram | main · Telegram · Direct |
| `agent:main:feishu:grp_xxx` | feishu | main · 飞书 · Group |
| `subagent:xyz789` | webchat | 🤖 Sub-Agent · from WebChat |
| `cron:main:abc123` | — | 📋 Cron: 每日提醒 |

---

##### 2.3.4.4 Task List UI 设计

左侧面板 Task List 区域，替代原有 3 个硬编码条目。

**布局结构**：

```
┌─ Task List ───────────────────────────────────┐
│                                                │
│  Gateway Service                 🟢 Running    │
│  [Stop] [Restart]                              │
│                                                │
│  ─── Sessions ──────────────────────────       │
│  🟢 main · WebChat · Direct                    │
│     mimo-v2-pro · 137k tokens · 25s ago        │
│     [Abort]                                    │
│                                                │
│  ─── Cron Jobs ─────────────────────────       │
│  📋 每日提醒  0 9 * * *           ✅ Enabled   │
│     Next: tomorrow 09:00                       │
│     [Run Now] [Disable] [Delete]               │
│                                                │
│  📋 健康检查  */30 * * *          ✅ Enabled   │
│     Next: in 12 min                            │
│     [Run Now] [Disable] [Delete]               │
│                                                │
│  ── Completed (2) [Show] ──────                │
│  ✅ sub-agent: code-gen  3m ago                │
│  ✅ sub-agent: test-fix  12m ago               │
│                                                │
│  [+ Add Cron Job]                              │
│                                                │
└────────────────────────────────────────────────┘
```

**分区说明**：

| 区域 | 内容 | 数据来源 | 可操作 |
|------|------|---------|--------|
| Gateway 行 | 进程状态 + 操作按钮 | health.cpp（已有） | Stop / Restart |
| Sessions 区 | 活跃会话列表（ageMs < 30分钟） | `gateway call sessions.list --json` | Abort |
| Cron 区 | 定时任务列表（始终显示） | `cron list --json` | Run Now / Enable / Disable / Delete |
| Completed 区 | 已完成的子代理，默认折叠，30分钟后自动清理 | sessions 中 `subagent:*` 且不再活跃 | 仅查看 |

**无连接状态**：Gateway 进程不存在或 HTTP 不通时，Sessions/Cron 区显示 "Not Connected" 占位行。

---

##### 2.3.4.5 用户干涉操作

用户点击按钮 → 触发 CLI 命令 → 刷新列表。

**操作按钮 → CLI 映射**：

| 按钮 | 执行命令 | 耗时 | 反馈 |
|------|---------|------|------|
| Abort | `openclaw gateway call sessions.reset --json` | ~1.5s | 按钮 loading → 列表刷新 |
| Stop (Gateway) | `openclaw gateway stop` | ~2s | 状态灯变红 |
| Restart (Gateway) | `openclaw gateway restart` | ~3s | 状态灯闪烁→变绿 |
| Run Now | `openclaw cron run <id>` | ~1.5s | 按钮 loading → 列表刷新 |
| Enable | `openclaw cron enable <id>` | ~1.5s | 状态切换 ✅→✅ |
| Disable | `openclaw cron disable <id>` | ~1.5s | 状态切换 ✅→⏸️ |
| Delete | `openclaw cron rm <id>` | ~1.5s | 弹窗确认 → 从列表移除 |
| + Add Cron Job | 弹窗表单 → `openclaw cron add ...` | ~1.5s | 新条目出现在列表 |

**操作按钮交互流程**：

```
用户点击 [Disable]
  → 按钮变 "..." loading 状态
  → CreateProcess("openclaw cron disable abc123")
  → 等待进程完成
  → 成功：刷新 Task List（重新轮询 sessions + cron）
  → 失败：按钮恢复 + 红色提示 "Operation failed"
```

**新建 Cron 弹窗表单字段**：

| 字段 | 类型 | CLI 参数 |
|------|------|---------|
| 名称 | 文本输入 | `--name "..."` |
| 调度类型 | Dropdown（Cron / 间隔 / 一次性） | `--cron` / `--every` / `--at` |
| Cron 表达式 | 文本输入（5 字段） | `--cron "0 9 * * *"` |
| 间隔 | Dropdown（5m / 10m / 30m / 1h / ...） | `--every 30m` |
| 消息内容 | 多行文本 | `--message "..."` |
| Session 目标 | Dropdown（main / isolated） | `--session main` |
| 时区 | Dropdown（26 个时区） | `--tz "Asia/Shanghai"` |

---

##### 2.3.4.6 状态栏聚合灯

| 条件 | LED 颜色 | 托盘同步 |
|------|---------|---------|
| Gateway 进程不存在 | 🔴 红 | tray_red |
| Gateway 在 + HTTP 不通 | 🔴 红 | tray_red |
| Gateway 在 + 无活跃会话 | ⚪ 白 | tray_white |
| Gateway 在 + 有活跃会话（ageMs < 5 分钟） | 🟡 黄 | tray_yellow |
| 检测中 / 轮询进行中 | 🟡 黄 | tray_yellow |

---

##### 2.3.4.7 忙/空闲指示器

> **已合并到 §2.3.2 LED 指示灯。** LED 的三级语义（Ready/Busy/Error）已覆盖忙/闲感知，不再需要单独的指示器文字行。

LED 判断逻辑复用 §2.3.3 的 Session 检测结果：
- 有 ageMs < 5min 的活跃 Session → 🟡 Busy
- 无活跃 Session → 🟢 Ready

---

##### 2.3.4.8 任务完成与清理策略

**活跃任务**：始终可见，可操作。
- Session：ageMs < 30 分钟 → 正常显示
- Cron：始终显示（常驻配置）

**已完成任务**：显示 ✅ 标记，默认折叠。
- Sub-Agent 完成后 → 进入 Completed 区，保留 30 分钟
- 30 分钟后自动清理，不再显示
- 用户可点击 [Show] 展开查看

**冷 Session**：ageMs > 30 分钟 → 从 Sessions 区隐藏。
- 不显示在 Task List 中（session 数据保留在 Gateway 侧，AnyClaw 不负责清理）
- 用户如需查看历史，通过 WebChat 等原始渠道查看

**Cron 清理**：不由 AnyClaw 自动清理，用户通过 [Delete] 按钮手动删除。

---

##### 2.3.4.9 v2.1 扩展计划

| 功能 | 说明 |
|------|------|
| 会话历史查看 | 点击 Session 条目 → 右侧面板显示对话历史 |
| Cron 编辑 | 编辑已有 Cron 的调度和消息内容 |
| Cron 执行历史 | `openclaw cron runs --id <id> --json` 显示最近执行记录 |
| 任务通知 | 子代理完成/失败 → 托盘气泡 + 状态栏联动 |
| 如 Gateway 支持 `status --json` | 接入补充信息（版本号、心跳间隔等） |

---

##### 2.3.4.10 代码位置

| 模块 | 文件 | 状态 |
|------|------|------|
| 任务数据层 | `src/task_manager.cpp` | **新建**：CLI 调用、JSON 解析、数据缓存 |
| 任务列表 UI | `src/ui_main.cpp` | 改造：移除 3 行硬编码，改为动态列表 |
| 操作按钮回调 | `src/ui_main.cpp` | 新增：Abort / Cron Enable/Disable/Run/Delete 回调 |
| Cron 新建弹窗 | `src/ui_main.cpp` | 新增：表单 UI + `cron add` 调用 |
| 进程检测 | `src/health.cpp` | 复用：已有 Gateway 进程 + HTTP 检测 |
| HTTP 健康检查 | `src/http_client.cpp` | 复用：`curl /health` |

---

### 2.4 系统设置

#### 2.4.1 Settings Tab 结构

| Tab | 内容 |
|-----|------|
| General | Boot Start / Auto Start / Theme / Language / Exit Confirm |
| Model | Current Model / Search / Model List / API Key |
| Skills | Skill 列表 / 下载 / 安装 |
| Log | Log Enable / Log Level / Filter / Export / Clear |
| About | 品牌信息 / Tech Stack / Config Import/Export |

#### 2.4.2 首次启动向导
- **功能编号：** WIZ-01 **优先级：** P1 **状态：** ✅ 已实现
- 6步引导（全屏遮罩 + 居中弹窗，深色风格 #1E2530 圆角12，支持拖拽移动）：
  1. **语言选择** — 中文/English 两个按钮，默认 EN
  2. **OpenClaw 检测** — 自动检测，显示状态（Running/Detected/Not Found）+ LED + 版本号 + 端口
  3. **API 密钥** — 密码模式输入框（sk-or-...）
  4. **模型选择** — dropdown（10个预设模型：gemini/claude/gpt/deepseek/qwen/llama 等）
  5. **用户画像** — 昵称输入 + 时区 dropdown（26个时区，默认 UTC+8 Asia/Shanghai）
  6. **确认** — 配置摘要 + "Get Started" 绿色按钮
- 顶部步骤指示器（Step 1/6），底部上一步/下一步按钮
- 首次启动自动弹出（检查 config.json `wizard_completed` 字段，不存在或 false 则显示）
- 完成后标记 `wizard_completed: true` 写入 config.json，后续跳过
- `save_theme_config()` 持久化 wizard_completed 字段，`load_theme_config()` 启动时读取
- **用户画像写入 USER.md**：
  - 路径1：`Z:\root\.openclaw\workspace\USER.md`（测试环境）
  - 路径2：`%USERPROFILE%\.openclaw\workspace\USER.md`（生产环境）
  - 格式：Name/What to call them/Pronouns/Timezone/Notes + Context 空段
- **General Tab 底部提供 "Reconfigure Wizard" 按钮**，点击关闭设置面板 → 打开向导
- 向导可随时重新触发，重新填写会覆盖 USER.md
- 实现文件：`src/ui_main.cpp`（向导 UI + 配置读写）、`src/ui_settings.cpp`（按钮绑定）、`src/app.h`（声明）

#### 2.4.3 开机自启动 & 自动重启
- **功能编号：** AS-01 **优先级：** P1
- **Boot Start**：Windows 注册表 `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` 写入/删除 `AnyClaw_LVGL` 键值（`RegSetValueExW` / `RegDeleteValueW`），设置页/托盘菜单切换
- **Auto Start**：异常退出自动拉起 OpenClaw Gateway，独立开关控制（代码中通过 `g_autoRestarted` 标志，仅重启 1 次）

#### 2.4.4 通用设置（General Tab）
- 语言切换（CN/EN/KR/JP），立即生效
- 主题切换（默认暗色 / 默认亮色 / 经典暗色），立即生效
- 退出确认开关

#### 2.4.5 模型设置（Model Tab）
- **状态：** ✅ 已实现（下拉选择 + 自定义模型 + Gateway 同步）
- **架构：** AnyClaw 是 OpenClaw 的 GUI 壳子，聊天走 Gateway `/v1/chat/completions`，模型配置同步到 `openclaw.json`
- **当前模型显示：** 绿色高亮从 Gateway 读取的实际配置模型
- **模型选择：** 下拉列表（lv_dropdown），按能力排序（405B → 轻量 → 付费）
- **添加自定义模型：** 下拉框右侧 "+" 按钮 → 弹出模态对话框 → textarea 输入模型名称 → 确认后自动添加到列表并选中
  - textarea 样式与聊天输入框一致（深色背景、圆角、placeholder 提示）
  - 点击遮罩或取消按钮关闭弹框
  - 自定义模型保存到 `assets/custom_add_models.json`，持久化跨重启
- **免费模型：** OpenRouter 上 25 个 `:free` 模型，全部国内直连可达（实测 900-3500ms）
- **Provider 检测：** 下拉切换时自动检测 provider（openrouter/xiaomi），动态显示 "Provider: xxx → API Key: xxx"
- **API Key 联动：** 切换模型时自动从 Gateway 读取对应 provider 的 API Key 并填入输入框
- **Save 同步 Gateway：** 后台线程执行 `openclaw config set` 更新 provider + model + apiKey → 重启 Gateway（不阻塞 UI）
  - 按钮点击后立即显示 "Saving..."，后台完成后恢复 "Save"
- **用户配置兼容：**
  - 已有 openclaw.json 的用户：当前模型从配置读取，下拉显示 AnyClaw 推荐的免费模型
  - 新用户：引导通过 Settings 配置 API Key 和模型
- **所有 OpenRouter 模型共用一个 API Key**（openrouter.ai 创建一个 Key 即可调用全部模型）

#### 2.4.6 日志设置（Log Tab）
- 日志系统总开关（Enable / Disable），关闭后不写入文件
- 日志级别（Debug / Info / Warn / Error），低于此级别的日志不写入文件
- 级别过滤（All / Info+ / Warn+ / Error Only），仅影响 UI 显示
- 日志列表实时展示，按级别着色（Debug 灰 / Info 蓝 / Warn 黄 / Error 红）
- 刷新按钮重新加载日志
- **导出日志** — 复制当前日志到 `logs/AnyClaw_export.log`（Windows: `Documents\AnyClaw_export.log`）
- **清除日志** — 清空当前日志文件并写入清除时间标记
- 底部级别颜色图例（DEBUG / INFO / WARN / ERROR 彩色标识）

#### 2.4.7 关于（About Tab）
- 品牌信息、版本号、Tech Stack
- 配置导入/导出

---

### 2.5 外观与主题

#### 2.5.1 色彩方案
- **功能编号：** TH-01 **优先级：** P1

**预设主题：**

| 主题 | 背景 | 面板 | 文字 |
|------|------|------|------|
| 默认暗色 | #1A1E2E | #232838 | #E0E0E0 |
| 默认亮色 | #F5F5F5 | #FFFFFF | #333333 |
| 经典暗色 | #2D2D2D | #3C3C3C | #FFFFFF |

**暗色组件色：**
- 按钮：#2A3040 / Hover #353D50 / Active #1E2530
- 输入框：背景 #1E2530，边框 #3A4050，光标 #4A90D9
- 滚动条：轨道 #1A1E2E，滑块 #3A4050 / Hover #4A5060
- 分割线：#2A2E3E

**亮色组件色：**
- 按钮：#E8E8E8 / Hover #D0D0D0 / Active #C0C0C0
- 输入框：背景 #FFFFFF，边框 #CCCCCC，光标 #4A90D9
- 滚动条：轨道 #F0F0F0，滑块 #C0C0C0 / Hover #A0A0A0

#### 2.5.2 主题自定义
- **功能编号：** TH-CUSTOM-01 **优先级：** P2
- 用户自定义颜色方案

#### 2.5.3 字体大小调整
- **功能编号：** FS-01 **优先级：** P2

#### 2.5.4 字体管理

| 模块 | 字体                       | 字号     |
| ---- | -------------------------- | -------- |
| 中文 | 微软雅黑 (msyh.ttf)        | 16px     |
| 英文 | Montserrat                 | 10~20px  |
| 图标 | LVGL Symbols               | 对应字号 |

#### 2.5.5 通用控件体系
- **功能编号：** WIDGET-01 **优先级：** P1 **状态：** ✅ 已实现

- **目标**：统一控件创建方式，消除样式重复定义
- **当前问题**：
  - 25 个按钮各自独立设色/圆角/尺寸
  - 234 次 `set_style_*` 调用散落在 ui_main.cpp
  - Switch 样式（同样的 RGB 值）重复 3 遍
  - 分隔线有的用 `add_divider()`，有的手写 `lv_obj_create`
- **规划控件库**：

```
src/widgets/
├── aw_button.h      // BTN_PRIMARY / BTN_SECONDARY / BTN_DANGER / BTN_GHOST
├── aw_input.h       // TextArea / Dropdown / Switch 统一样式
├── aw_panel.h       // Card / Dialog / Modal
├── aw_divider.h     // Horizontal / Vertical
├── aw_label.h       // Title / Body / Hint / Mono
└── aw_common.h      // 公共颜色/间距/圆角常量
```

- **示例**：`aw_btn_create(parent, "确定", BTN_PRIMARY)` 一行创建标准按钮

---

### 2.6 国际化 (i18n)
- **功能编号：** I18N-01 **优先级：** P1
- 支持 **CN / EN** 切换，界面始终显示单一语言（不会同时出现中英文）
- 首次启动自动检测系统语言（Windows API），不支持则默认英文
- 切换后立即生效，无需重启
- 所有 UI 文本通过 `I18n` 结构体定义，`tr()` 函数根据当前语言返回对应文本
- 配置持久化到 config.json（language 字段）

---

### 2.7 Skill 管理
- **功能编号：** SK-01 **优先级：** P1

**功能：**
- Skill 批量下载/安装（ClawHub 仓库）
- Skill 搜索与浏览
- 内置（Built-in）+ 第三方（ClawHub）分类展示
- Skill 启用/禁用、版本管理与更新
- Skill 自动更新开关

**Skill 列表：**

| 编号 | 技能名称                  | 来源     | 说明                                      |
| ---- | ------------------------- | -------- | ----------------------------------------- |
| S-01 | xiaolongxi-search         | ClawHub  | 小龙虾搜索 - 跨搜索引擎智能聚合搜索       |
| S-02 | xiaolongxi-convert        | ClawHub  | 格式转换大师 - 图片/PDF/文档转换          |
| S-03 | openclaw-audit            | Built-in | 安全审计 - 安全风险评估与漏洞检测         |
| S-04 | openclaw-code-interpreter | Built-in | 代码解释器 - 运行代码/分析数据/自动化任务 |
| S-05 | openclaw-file-converter   | Built-in | 文件格式转换工具                          |
| S-06 | openclaw-gemini           | Built-in | Gemini AI 交互                            |
| S-07 | openclaw-gradio           | Built-in | Gradio 应用调用                           |
| S-08 | openclaw-jina             | Built-in | Jina AI 工具 - 搜索/爬取/转换/RAG         |
| S-09 | openclaw-lab              | Built-in | 多模型研究 - AI 模型测试与对比            |
| S-10 | openclaw-perplexity       | Built-in | Perplexity 搜索                           |
| S-11 | openclaw-raptor           | Built-in | RAPTOR 高级检索                           |
| S-12 | openclaw-text-to-speech   | Built-in | 文本转语音                                |
| S-13 | openclaw-xurl             | Built-in | URL处理                                   |
| S-14 | web-scraper               | ClawHub  | 网页爬虫                                  |
| S-15 | openclaw-web-search       | Built-in | Web 搜索                                  |

---

### 2.8 系统管理

#### 2.8.1 通知系统
- **功能编号：** N-01 **优先级：** P1
- 托盘气泡通知
- 日志面板通知
- 状态栏颜色变化

#### 2.8.2 日志系统
- **功能编号：** LOG-01 **优先级：** P1
- 日志文件：`%APPDATA%\AnyClaw_LVGL\logs\app.log`
- 轮转规则：文件超过 1MB 时自动轮转（app.log → app.log.1 → .2 → .3 → .4）
- 最多保留 5 个文件（app.log + 4 个轮转文件）
- 启动时自动清理 7 天以上的轮转文件
- 日志级别：Debug / Info / Warn / Error
- 每条日志格式：`[YYYY-MM-DD HH:MM:SS.mmm] [LEVEL] [MODULE] message`
- 同时输出到 stdout

#### 2.8.3 错误处理与崩溃恢复
- **功能编号：** E-01 **优先级：** P1
- 崩溃日志：`%APPDATA%\AnyClaw_LVGL\crash_logs\crash_*.log`
- 保留最近 10 个崩溃日志
- SetUnhandledExceptionFilter 捕获异常
- 崩溃日志内容：异常代码、异常地址、异常类型（ACCESS_VIOLATION / STACK_OVERFLOW / DIVIDE_BY_ZERO 等）
- **Stack Trace** — 通过 dbghelp 符号解析，记录调用栈最多 64 帧
- 崩溃后生成 `last_crash.txt` 标记文件，记录最近一次崩溃日志路径

#### 2.8.4 更新机制
- **功能编号：** U-01 **优先级：** P0
- OpenClaw 更新：通过 `openclaw update` 命令
- AnyClaw 更新：仅支持本地安装覆盖

#### 2.8.5 更新日志
- **功能编号：** U-03 **优先级：** P2
- 版本更新时显示变更日志

#### 2.8.6 数据备份与恢复
- **功能编号：** B-01 **优先级：** P2
- **Config 导出** — About Tab "Export Config" 按钮，复制 config.json 到可选路径
- **Config 导入** — 计划中（暂未实现）

#### 2.8.7 版本号规范
- **功能编号：** VER-01 **优先级：** P1

**格式：** `v{主版本}.{次版本}.{修订号}`

| 字段 | 说明 | 示例 |
|------|------|------|
| 主版本 | 框架级变更（v1→v2） | v2.x.x |
| 次版本 | 功能新增或状态变更 | v2.1.x |
| 修订号 | Bug 修复、小改动 | v2.0.1 |

**当前版本：** `v2.0.1`

**显示位置：** 窗口标题栏、关于对话框、托盘气泡、日志文件头

**存储：** `#define ANYCLAW_VERSION "2.0.1"`，编译时嵌入 exe 资源段

#### 2.8.8 启动自检
- **功能编号：** SC-01 **优先级：** P0
- 应用启动时自动执行四项检查：

| 检查项 | 检测方式 | 自动修复 |
|--------|---------|---------|
| Node.js | `node --version` | 不可修复，弹窗提示用户安装 |
| npm | `npm --version` | 尝试 `npm cache clean --force` |
| 网络连通性 | HTTP GET google.com / openrouter.ai | 不可修复，提示检查网络 |
| 配置目录 | `%USERPROFILE%\.openclaw` 可写性 | 尝试 `create_directories` 创建 |

- 检查结果写入日志（`[SelfCheck]` 前缀）
- 修复后自动重新检查验证
- Node.js 缺失 → 显示 LVGL 弹窗阻断启动（`g_startup_error`）

#### 2.8.9 Security 状态面板
- **功能编号：** SEC-UI-01 **优先级：** P1
- General Tab 中显示安全状态区域
- 展示内容：API Key 状态、端口号、配置目录可写性
- 状态指示灯（绿色 LED + "Security: Good" 文字）
- 详细信息行：`API Key: OK | Port: 18789 | Config: Writable`

#### 2.8.10 一键迁移（环境搬家）
- **功能编号：** MIG-01 **优先级：** P1 **状态：** ✅ 已实现
- 将 AnyClaw + OpenClaw 全部环境打包，从一台电脑迁移到另一台

**导出（旧电脑）：**
- About Tab → "Export Migration" 按钮
- 打包内容：

| 内容 | 来源路径 | 说明 |
|------|---------|------|
| AnyClaw 配置 | `%APPDATA%\AnyClaw\config.json` | 语言、主题、窗口位置等 |
| 法律声明 | `%APPDATA%\AnyClaw\accepted.json` | 已接受标记 |
| OpenClaw 工作区 | `%USERPROFILE%\.openclaw\` 全量 | agents、skills、memory、MEMORY.md、SOUL.md 等 |
| OpenClaw 配置 | `openclaw.json` | Gateway 端口、模型配置等 |
| 迁移元数据 | 自动生成 `manifest.json` | 版本号、来源主机名、导出时间、文件校验和 |

- 输出：`AnyClaw_Migration_{version}_{timestamp}.zip`
- **安全**：API Key 不导出明文，仅保留哈希；导入时需重新输入

**导入（新电脑）：**
- About Tab → "Import Migration" 按钮
- 前置检查：

| 检查项 | 未满足处理 |
|--------|-----------|
| Node.js | 弹窗提示安装（附下载链接） |
| npm | 尝试自动修复 |
| OpenClaw | 自动执行 `npm install -g openclaw` |
| 磁盘空间 | 检查可用空间 > 500MB |

- 导入流程：
  1. 解压迁移包到临时目录
  2. 校验 `manifest.json` 版本兼容性
  3. 复制 AnyClaw 配置到 `%APPDATA%\AnyClaw\`
  4. 复制 OpenClaw 工作区到 `%USERPROFILE%\.openclaw\`
  5. 安装/更新 OpenClaw（如未安装）
  6. 启动自检验证环境完整性
  7. 重启生效

**路径适配：**
- 两台机器用户名可能不同 → 导入时自动检测当前用户的 `%APPDATA%` 路径
- OpenClaw 工作区中的绝对路径 → 跳过或提示手动修改

#### 2.8.11 离线授权（机器绑定）
- **功能编号：** LIC-01 **优先级：** P2 **状态：** ⏸️ 暂不实施
- 无服务器，基于硬件指纹绑定，仅授权机器可运行
- **决策（2026-04-06）**：暂不实施。产品走开源/免费路线，无需防盗版机制。后续如需商业化再启动。

**硬件指纹采集：**
- 采集多个硬件 ID 组合，防止单一 MAC 被伪造：

| 组件 | Win32 API | 说明 |
|------|-----------|------|
| MAC 地址 | `GetAdaptersAddresses` | 取第一个非回环网卡 |
| 磁盘序列号 | `GetVolumeInformation` | C: 卷序列号 |
| 主板 UUID | `GetSystemFirmwareTable(RSMB)` | SMBIOS UUID |

- 组合后 SHA-256 哈希 → 格式化为 8 段机器码：`ACWL-XXXX-XXXX-XXXX-XXXX-XXXX-XXXX-XXXX`

**授权流程：**

```
用户端                              开发者端
──────────                          ──────────
1. 首次启动
   → 无 license.key
   → 弹窗显示机器码
   → "请将机器码发送给开发者"

2. 把机器码发给开发者       ────→   3. 生成 License Key
                                      HMAC-SHA256(secret, machine_code)
                                      → 8 段格式化 Key

4. 收到 License Key        ←────

5. 粘贴到弹窗输入框
   → 本地验证
   → 通过 → 保存
     %APPDATA%\AnyClaw\license.key
   → 失败 → 提示"授权码无效"
```

**验证逻辑：**
- 启动时读取 `license.key`
- 重新采集当前机器硬件指纹
- `HMAC-SHA256(secret_key, current_fingerprint) == license_key`
- 通过 → 正常启动
- 失败/缺失 → 弹授权弹窗阻断，不进入主界面

**安全策略：**
- `secret_key` 在 exe 中混淆存储（分段拼接 + 异或，不以明文存在）
- 硬件指纹组合三类 ID，换网卡/磁盘/主板任一即失效
- license.key 仅存储签名值，不含硬件明文
- 授权弹窗不可跳过、不可关闭（无关闭按钮，仅验证通过才消失）

**开发者工具（独立脚本，不随发布）：**
- `tools/license_gen.py` — 输入机器码，输出 License Key
- 仅开发者持有，不打包到发布 zip

#### 2.8.12 部署区与用户工作区分离
- **功能编号：** PATH-01 **优先级：** P1 **状态：** ✅ 已实现

- **目标**：三区分离，路径统一管理

| 区域 | 路径 | 性质 |
|------|------|------|
| 部署区 | `AnyClaw_LVGL/`（安装目录） | 只读，不修改 |
| 用户工作区 | `%APPDATA%\AnyClaw\` | 可读写，用户数据 |
| OpenClaw 关联区 | `%USERPROFILE%\.openclaw\` | 只读检测，不管理 |

- **用户工作区结构**：

```
%APPDATA%\AnyClaw\
├── config.json           主配置
├── accepted.json         法律声明
├── license.key           授权文件
├── logs\                 日志
├── crash_logs\           崩溃日志
├── cache\                缓存
└── user\                 用户数据（聊天记录等）
```

- **实施方案**：新建 `src/paths.h`，所有路径用宏引用

---

### 2.9 数据与持久化

#### 2.9.1 配置文件

| 文件 | 路径 | 说明 |
|------|------|------|
| `config.json` | `%APPDATA%\AnyClaw_LVGL\` | 主配置（语言、主题、窗口位置等） |
| `accepted.json` | `%APPDATA%\AnyClaw_LVGL\` | 法律声明接受标记 |
| `app.log` | `%APPDATA%\AnyClaw_LVGL\logs\` | 应用日志 |
| `crash_*.log` | `%APPDATA%\AnyClaw_LVGL\crash_logs\` | 崩溃日志 |
| `task_history.json` | `%APPDATA%\AnyClaw_LVGL\` | 任务执行历史 |

#### 2.9.2 持久化数据清单
- 语言设置
- 窗口大小、位置（window_w / window_h / window_x / window_y）
- **窗口置顶状态**（always_top: 0/1）
- 健康检查时间间隔（refresh_interval_ms）
- 退出确认弹窗状态
- 首次启动向导完成标志（wizard_completed）
- 模型名称与 API Key 哈希
- 聊天历史记录（最近 50 条）
- 用户画像数据

#### 2.9.3 卸载
1. 退出 AnyClaw（托盘右键 → 退出）
2. 删除程序目录（exe + dll）
3. 可选：删除 `%APPDATA%\AnyClaw_LVGL\` 目录
4. 可选：删除注册表开机自启动项

---

### 2.10 技术规格

#### 2.10.1 运行环境

| 要求 | 说明 |
|------|------|
| 操作系统 | Windows 10/11 (x64) |
| 运行时依赖 | `SDL2.dll`（同目录） |
| 可选依赖 | OpenClaw Gateway（未安装时显示"未找到"） |
| 磁盘空间 | < 20 MB |
| 内存 | < 200 MB（空闲状态） |

#### 2.10.2 部署方式
- 双文件部署：`AnyClaw_LVGL.exe` + `SDL2.dll` 放入同一目录，双击运行
- 计划：单文件部署（Launcher 嵌入 SDL2.dll）

#### 2.10.3 快捷键
- **功能编号：** K-01 **优先级：** P1
- Ctrl+V 粘贴、Ctrl+C 复制、Ctrl+X 剪切、Ctrl+A 全选
- Enter 发送、Shift+Enter 换行
- 选中文字后按键直接替换（标准文本编辑行为）

#### 2.10.4 网络与代理
- **功能编号：** P-01 **优先级：** P1

#### 2.10.5 性能约束
- **功能编号：** PF-01 **优先级：** P1

#### 2.10.6 安全
- **功能编号：** SEC-01 **优先级：** P0

#### 2.10.7 DPI 自动检测
- **功能编号：** DPI-01 **优先级：** P1
- **应用设为 DPI-aware**：启动时调用 `SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)`，fallback 到 `SetProcessDPIAware()`
- 效果：Windows 不再对窗口做自动 DPI 缩放，SDL/LVGL 工作在物理像素空间
- 启动时通过 `GetDpiForSystem()`（Win32 API）检测系统 DPI 缩放比例
- fallback：`GetDeviceCaps(LOGPIXELSX)` 检测物理 DPI
- 窗口尺寸以实际屏幕分辨率为基准（`GetSystemMetrics(SM_CXSCREEN/SM_CYSCREEN)`），默认屏幕 75%×75%，上限为屏幕分辨率减去边距（任务栏、窗口边框）
- 布局常量按 DPI 缩放（`SCALE()` 宏：`px * dpi_scale / 100`）
- `app_set_dpi_scale()` 仅更新布局常量，**不再调用 `SDL_SetWindowSize()`**（该操作曾导致双重缩放：Windows 自动缩放 × 手动缩放 = 156% at 125% DPI）
- config 中保存 `dpi_scale` 作为快照，启动时对比：若与当前系统 DPI 不一致，忽略旧窗口尺寸，使用默认值

#### 2.10.8 IME 输入法支持
- **功能编号：** IME-01 **优先级：** P1
- SDL hints 启用 IME：`SDL_HINT_IME_SHOW_UI=1`、`SDL_HINT_IME_SUPPORT_EXTENDED_TEXT=1`
- 支持中文 / 日文输入法候选窗口显示
- 键盘缓冲区扩大到 256 字节（适配 IME 多字节序列）

#### 2.10.9 宏开关管理
- **功能编号：** MACRO-01 **优先级：** P1 **状态：** ✅ 已实现

- **目标**：所有可调参数统一用宏管理，消除硬编码散落
- **范围**：

| 类别 | 示例宏 | 当前散落位置 |
|------|--------|-------------|
| 窗口尺寸 | `WIN_DEFAULT_W`, `WIN_DEFAULT_H` | `ui_main.cpp:57-58` |
| 标题栏 | `TITLE_BAR_H` | `ui_main.cpp:59`, `main.cpp` |
| 面板布局 | `LEFT_PANEL_DEFAULT_W`, `SPLITTER_W` | `ui_main.cpp:65-66` |
| 间距 | `GAP`, `CHAT_GAP`, `CHAT_MSG_MARGIN` | `ui_main.cpp:62-64` |
| 聊天气泡 | `BUBBLE_MAX_WIDTH_PCT` | `ui_main.cpp` |
| 输入框 | `CHAT_INPUT_DEFAULT_H` | `ui_main.cpp:3074` |
| 按钮尺寸 | `BTN_SEND_SIZE`, `BTN_UPLOAD_SIZE` | `ui_main.cpp` |
| 滚动速度 | `SCROLL_SPEED_FACTOR` | `main.cpp` |
| 双击阈值 | `DOUBLE_CLICK_MS` | `main.cpp:400ms` |
| Health 间隔 | `HEALTH_CHECK_DEFAULT_MS` | `health.cpp` |
| 自动刷新 | `AUTO_REFRESH_DEFAULT_MS` | `ui_main.cpp:874` |
| Log 轮转 | `LOG_MAX_SIZE`, `LOG_MAX_FILES` | `app_log.h` |
| 崩溃日志 | `CRASH_LOG_MAX_COUNT` | `main.cpp:10个` |

- **实施方案**：新建 `src/app_config.h`，所有宏集中定义

---

### 2.11 任务执行协议

> AnyClaw 作为 OpenClaw 桌面管理器，其 AI Agent 在执行复杂任务时必须遵循以下协议。该协议同时定义了 UI 层任务状态的可视化要求。

#### 2.11.1 并行调度模型
- **功能编号：** EX-01 **优先级：** P0

```
┌─────────────────────────────────────┐
│           主任务（调度者）             │
│  职责：调度、监控、汇报、干预          │
└────┬────────┬────────┬──────────────┘
     │        │        │
  子代理A  子代理B  子代理C
  (任务1)  (任务2)  (任务3)
```

- 收到任务后立即拆解为子任务，全部以子代理并行执行
- 调度前分析依赖关系：有依赖的串行，无依赖的并行
- 最高优先级任务优先启动
- **UI 要求**：任务列表面板实时展示所有子代理的运行状态（LED + 名称 + 类型 + 运行时间）

#### 2.11.2 主动监控
- **功能编号：** EX-02 **优先级：** P0

- 主任务**主动轮询**子代理状态，不被动等汇报
- 定期检查子代理进度、输出、异常
- **UI 要求**：状态栏 LED 同步反映子代理聚合状态（全部绿 / 任一红 / 任一黄）

#### 2.11.3 实时汇报
- **功能编号：** EX-03 **优先级：** P1

- 子代理产出阶段性结果 → 主任务立即通知用户，不积攒，不等全部完成
- 关键里程碑、阻塞点、完成节点都主动通知
- **UI 要求**：托盘气泡通知 + 日志面板实时输出

#### 2.11.4 异常干预
- **功能编号：** EX-04 **优先级：** P0

- 子代理卡住（超时 / 死循环 / 报错）→ 主任务立即介入
- 处理策略：重试 → 换方案 → 拆分 → 手动接管
- 不干等，不踢皮球
- **UI 要求**：异常子代理红色高亮，托盘变红 + 气泡通知用户

#### 2.11.5 完成验证
- **功能编号：** EX-05 **优先级：** P1

- 子任务完成后，主任务必须进行二次验证
- 验证内容：功能是否正常、是否有遗漏、是否引入新问题
- 验证发现问题 → 指出具体问题 → 分配子任务修复 → 再次验证直到确认无误
- **截图验证**：每次代码修改编译后，必须截图验证 UI 效果
  - 截图命名：`AAA_{描述}_{YYYYMMDD_HHMM}.png`
  - 截图必须包含完整聊天区域，验证消息布局和文字显示

#### 2.11.6 开发流程
- **功能编号：** EX-06 **优先级：** P1

1. 功能 / Bug 任务 → 先理解需求，反馈给用户确认 → 确认后再执行
2. 执行完后自行运行测试，有问题自行修复
3. 每次代码修改，检查 `docs/` 目录文档是否需要同步更新
4. 所有修改（代码 + 文档）一并 commit 并 push 到 GitHub
5. 提交格式：`[AnyClaw][LVGL] 模块&模块`

---

### A.11 Agent、Sub-Agent 与 Session 的关系

#### A.11.1 基本概念

| 概念 | 说明 | 类比 |
|------|------|------|
| **Agent** | AI 大脑，定义 system prompt、工具、模型 | 职位（如"客服"） |
| **Session** | 一次对话实例，携带上下文和历史消息 | 一次具体的对话 |
| **Main Agent** | 与用户直接交互的主代理 | 总负责人 |
| **Sub-Agent** | 由主代理通过 `sessions_spawn` 派出的独立代理 | 外派执行者 |

#### A.11.2 关键区分

- 同一个 Agent 可以同时跑多个 Session（主会话 + cron 会话 + 子代理会话）
- Sub-Agent 运行在隔离 Session 中，不直接与用户交互
- 用户始终只与 Main Agent 对话，Main Agent 负责调度 Sub-Agent
- Sub-Agent 共享文件系统但不共享对话上下文
- **Sub-Agent 深度限制**：最多 1 层（depth 1/1），子代理不能再派子代理
- **Sub-Agent 结果通知**：完成时自动 push 结果到主代理（push-based completion），主代理不需要轮询
- 主代理可以 `subagents steer` 转达指令，或 `subagents kill` 终止

#### A.11.3 通信模型

```
用户 ↔ Main Agent（直接对话）
          │
          ├── sessions_spawn → Sub-Agent A（隔离会话，完成后汇报）
          ├── sessions_spawn → Sub-Agent B（隔离会话，完成后汇报）
          └── sessions_spawn → Sub-Agent C（隔离会话，完成后汇报）
```

- **用户 → Main Agent**：直接沟通，随时可以
- **Main Agent → Sub-Agent**：通过 `sessions_send` 或 spawn 时的 task 参数
- **Sub-Agent → Main Agent**：完成时自动 push 结果
- **用户 → Sub-Agent**：❌ 不可能，必须通过 Main Agent 中转
- **Sub-Agent → 用户**：❌ 不可能，必须通过 Main Agent 转述

#### A.11.4 子代理使用策略（Opt-In 模型）

Agent（无论主代理还是子代理）默认都是 **Opt-In**：所有任务自己干，不会主动派子代理。需要显式判断并执行 `sessions_spawn` 才会派出。

**适合派子代理的场景：**
1. **并行批量任务** — 多个独立任务可同时执行
2. **预估超过 1 分钟的耗时任务** — 编译、下载、克隆仓库、apt install 等。必须在执行前判断并派出，因为一旦自己开始执行就被阻塞，无法中途转交
3. **不想阻塞主线程** — 需要继续跟用户聊天的同时后台处理

**不适合派的场景：**
- 需要与用户频繁交互确认的任务（沟通链路太长）
- 单个简单任务（秒级完成，调度开销大于收益）
- 有复杂依赖关系的任务串

**关键限制：**
- 用户无法直接与子代理对话，必须通过主代理中转
- 子代理也无法主动联系用户
- 主代理可以 `subagents steer` 转达指令，或 `subagents kill` 终止

#### A.11.5 文件系统与 Context 的关系

**共享（磁盘文件）：** 所有 Session 实例共享同一个文件系统。主代理写入的文件，子代理或其他 session 实例可以读取。

**不共享（内存 Context）：** 每个 Session 有独立的对话历史和 context 窗口。session 之间不互通。context 有 token 上限，越早的消息越先被截断（头部丢失，尾部保留）。

**Context 窗口机制：**
- 窗口大小由模型决定（token 数限制）
- 新消息不断加入，旧消息从头部被挤出
- 心跳 poll 来时，Agent 看到的是窗口内剩余的历史，不是全部
- 这就是为什么"写下来的才算记得"——上下文是短期记忆，容量有限

**用户可见性：**
- ✅ 用户能看到：聊天记录（webchat/channel 界面直接可见）
- ❌ 用户看不到：系统提示词、注入文件内容（AGENTS.md/SOUL.md等）、工具定义、内部推理过程
- ✅ 用户能间接查看：`session_status` 查看用量/模型/费用
- ✅ 用户能改：编辑 AGENTS.md/SOUL.md 等文件，下次启动生效；对话中直接给指令实时生效

**持久化策略：** 重要信息必须写入文件，不要依赖 context 记住。

```
共享（持久化）              不共享（临时）
┌─────────────┐           ┌─────────────────┐
│ 文件系统     │           │ Session A context│
│ AGENTS.md   │ ←所有实例  │ Session B context│
│ SOUL.md     │  都能读    │ Session C context│
│ MEMORY.md   │           │ （各自独立）      │
│ memory/*.md │           └─────────────────┘
└─────────────┘
```

#### A.11.5b 记忆管理架构

Agent 每次 Session 启动都是"全新醒来"，通过文件系统恢复记忆连续性：

**两级记忆体系：**

| 层级 | 文件 | 性质 | 谁能读 |
|------|------|------|--------|
| 日志层 | `memory/YYYY-MM-DD.md` | 原始记录，每天一个文件 | 所有 Session |
| 精炼层 | `MEMORY.md` | 长期记忆，去重提炼 | ⚠️ 仅主会话 |

**MEMORY.md 安全规则：**
- **仅在主会话**（直接与用户对话的 Session）中加载
- **群聊、子代理 Session 中禁止加载** — 防止个人隐私泄露给陌生人
- 可自由读写更新，写入重要事件、决策、教训、偏好

**记忆持久化原则：**
- "写下来的才算记得" — 口头说的"记住这个"不会跨 Session 保留
- 重要的必须写文件，Context 窗口会截断旧消息
- 定期（通过心跳）将日志层有价值的内容提炼到 MEMORY.md

#### A.11.6 文件读取规则

Agent 在不同阶段读取不同文件，规则定义在 `AGENTS.md` 中：

**Session 启动时（每个实例执行）：**

OpenClaw 运行时自动将工作区根目录下的标准文件注入到 Agent 上下文中（system-level 行为，非 Agent 主动读取）：

| 文件 | 注入方式 | 说明 |
|------|---------|------|
| `AGENTS.md` | ✅ OpenClaw 自动注入 | 行为规则、工作流定义 |
| `SOUL.md` | ✅ OpenClaw 自动注入 | 人格、说话风格 |
| `USER.md` | ✅ OpenClaw 自动注入 | 用户信息（时区、称呼、偏好） |
| `TOOLS.md` | ✅ OpenClaw 自动注入 | 工具笔记（按需读取） |
| `IDENTITY.md` | ✅ OpenClaw 自动注入 | Agent 身份定义 |
| `HEARTBEAT.md` | ✅ OpenClaw 自动注入 | 心跳任务清单 |
| `memory/YYYY-MM-DD.md` | ⚠️ Agent 手动读 | 今天+昨天的笔记 |
| `MEMORY.md` | ⚠️ 仅主会话，Agent 手动读 | 长期记忆，群聊/子代理不读（防泄露） |

**运行中触发：**
| 触发条件 | 读取文件 | 说明 |
|----------|---------|------|
| 收到心跳 poll | `HEARTBEAT.md` | 检查周期性任务 |
| 回答历史问题 | `memory_search` 搜索 | 语义搜索记忆文件 |
| 使用特定工具 | `skills/*/SKILL.md` | 技能使用说明 |

**用户可通过编辑文件自定义 Agent 行为：**
- `AGENTS.md`：增删启动读取规则、子代理策略等
- `SOUL.md`：修改语气、人格
- `HEARTBEAT.md`：添加周期性检查任务

#### A.11.6b 群聊中 Agent 行为规则

在群聊（Discord、WhatsApp 等多人频道）中，Agent 接收所有消息但需要智能判断何时发言：

**应该发言：**
- 被直接 @提及 或提问
- 能提供有价值的信息/见解/帮助
- 有合适的幽默或有趣的内容
- 纠正重要的错误信息
- 被要求总结

**保持安静（HEARTBEAT_OK）：**
- 只是人类之间的闲聊
- 已经有人回答了问题
- 回复只会是"嗯"或"不错"
- 对话自然流畅不需要介入
- 发消息会打断当前氛围

**核心原则：** 人类在群聊中不会回复每一条消息，Agent 也不应该。质量 > 数量。

**社交反应（Reactions）：**
- 在支持 Reactions 的平台（Discord、Slack）上，用 emoji 表情做出轻量级社交信号
- 适用场景：👍 表示认可、😂 表示好笑、🤔 表示有意思、💡 表示启发
- 每条消息最多一个 Reaction，选最贴切的

#### A.11.7 Session 架构在 OpenClaw 中的位置

Session 与 Channel 的映射关系由 Gateway 管理，详见 A.10.1d（Session-Channel 映射）。

AnyClaw 作为桌面管理器，需要感知 Gateway 上的 Session 状态（忙/空闲），但不需要改 Gateway 代码。感知方式详见 A.10.4（可行方案）和 A.10.5（忙/空闲检测）。

---

### 2.12 工作区管理

> **功能编号：** WS-01 **优先级：** P0 **状态：** 🔧 核心已实现（健康检查/初始化/配置持久化/工作区锁）

#### 2.12.1 概念定义

**工作区 (Workspace)** 是 OpenClaw Agent 的活动边界。Agent 所有文件操作限定在工作区根目录内。每个工作区是一个独立的配置单元，包含 Agent 人格、记忆、工具配置和项目文件。

**四个目录，各有归属：**

| 目录 | 归属 | 由谁决定 | 性质 | 当前代码状态 |
|------|------|---------|------|-------------|
| AnyClaw 安装目录 | AnyClaw 程序本体 (exe + dll + assets) | 用户选择 | 只读 | ❌ 用户无选择，随便放 |
| AnyClaw 数据目录 | config.json、日志、缓存 | 系统固定 `%APPDATA%\AnyClaw_LVGL\` | 可读写 | ✅ 已实现（`paths.h`） |
| OpenClaw 安装目录 | OpenClaw 运行时 (npm 全局) | 自动检测 | 只读 | ⚠️ 硬编码 `%USERPROFILE%\.openclaw\` |
| OpenClaw 工作区 | Agent 活动范围 (AGENTS.md、MEMORY.md、项目) | 用户选择 | 可读写 | ✅ 已实现（`workspace_get_root()` + `config.json.workspace_root`） |

#### 2.12.2 工作区目录结构

每个工作区根目录包含以下文件（部分由 AnyClaw 管理，部分由用户/Agent 管理）：

```
工作区根目录/
├── WORKSPACE.md          ← 工作区元信息（AnyClaw 管理）
├── AGENTS.md             ← Agent 行为规则（混合管理，见 2.13.4）
├── SOUL.md               ← Agent 人格（用户/模板管理）
├── USER.md               ← 用户信息（Agent 管理）
├── MEMORY.md             ← 长期记忆（Agent 管理）
├── TOOLS.md              ← 工具配置（混合管理）
├── HEARTBEAT.md          ← 心跳任务（Agent 管理）
├── IDENTITY.md           ← Agent 身份（Agent 管理）
├── workspace.json        ← 权限配置（AnyClaw 管理，运行时读取）
├── .openclaw.lock        ← 实例锁文件（运行时生成）
├── .backups/             ← 修改备份（AnyClaw 管理）
├── memory/               ← 每日记忆文件（Agent 管理）
├── skills/               ← 已安装技能（Agent/ClawHub 管理）
└── projects/             ← 用户项目文件（用户/Agent 共用）
```

#### 2.12.3 多工作区支持

用户可以创建多个工作区，每个工作区独立配置：

```
工作区列表:
├── 🏠 个人助手    D:\AI\personal\       SOUL.md=通用助手, 权限=基础
├── 💻 开发项目    D:\Projects\          SOUL.md=开发者, 权限=开放
└── 🔒 敏感项目    D:\Secret\            SOUL.md=谨慎模式, 权限=严格
```

每个工作区独立维护：
- AGENTS.md / SOUL.md / TOOLS.md（不同人格和规则）
- workspace.json（不同权限配置）
- MEMORY.md / memory/（不同记忆）
- skills/（不同的已安装技能）

AnyClaw 启动时加载上次使用的工作区，Settings 中提供工作区切换和管理界面。

#### 2.12.4 WORKSPACE.md（新增文件）

工作区身份证，描述工作区的基本信息：

```markdown
# WORKSPACE.md

- 工作区名称: 个人助手
- 工作区根目录: D:\AI\personal\
- 创建时间: 2026-04-06
- 模板: 通用助手
- AnyClaw 版本: 2.0.1
- OpenClaw 版本: 0.x.x
- 工作区版本: 1
```

#### 2.12.5 安装向导（首次启动）

首次启动（无 config.json）时，弹出安装向导，步骤如下：

**步骤 1 - AnyClaw 安装位置**
- 用户选择 AnyClaw 程序文件存放目录
- 默认建议：`C:\Program Files\AnyClaw\`
- 需要约 20MB 空间

**步骤 2 - OpenClaw 安装**
- 自动检测：Node.js 是否已安装、版本是否 ≥ 22.14
- Node.js 缺失时自动下载安装（Windows MSI 静默安装）
- 自动执行 `npm install -g openclaw`（支持多源自动切换）
- 或用户指定已安装的 OpenClaw 路径

**步骤 3 - 工作区设置**
- 选择工作区根目录
- 默认建议：`%USERPROFILE%\.openclaw\workspace\`
- 选择工作区模板（通用助手 / 开发者 / 写作者 / 数据分析）
- 自动创建目录结构和初始化文件

**步骤 4 - 权限配置**（详见 2.13）

**步骤 5 - 确认并启动**
- 写入所有配置文件
- 创建 workspace.json、WORKSPACE.md、AGENTS.md（含 MANAGED 区域）等

**安装过程交互（本轮补充，已落地）**
- 向导安装采用异步执行，UI 不阻塞
- 左侧统一进度面板显示：任务名、当前步骤、进度条、结果
- 支持“退出安装”按钮，用户可中途取消安装任务
- 下载与安装链路统一支持多源回退（失败/超时自动切换）

#### 2.12.6 工作区健康检查

每次启动时执行以下检查：

| 检查项 | 未满足处理 |
|--------|-----------|
| 工作区目录是否存在 | 提示创建或更换目录 |
| 磁盘空间是否 > 100MB | 警告 |
| 目录是否有写入权限 | 降级为只读模式 |
| 是否被其他实例占用（lock 文件） | 检测进程存活，死亡则清理 lock，存活则拒绝启动 |
| 目录结构是否完整 | 引导创建缺失的核心文件 |
| OpenClaw 版本是否兼容 | 提示更新 |

#### 2.12.7 工作区锁（已实现）

防止多个 AnyClaw 实例同时操作同一工作区：

- 启动时创建 `.openclaw.lock`，当前写入 **PID**
- 退出时删除 lock 文件
- 启动检测到已有 lock → 判断 PID 是否存活 → 存活则拒绝获取锁，死亡则视为 stale lock 并覆盖
- 当前实现为单机互斥，不含 hostname 维度

#### 2.12.8 工作区迁移

**导出：** AnyClaw → 打包工作区为 zip（不含 API Key 明文，仅保留哈希）

导出内容：
- AnyClaw 配置（`%APPDATA%\AnyClaw_LVGL\`）
- OpenClaw 工作区全量（AGENTS.md、MEMORY.md、skills 等）
- 自动生成 `manifest.json`（版本号、来源主机名、导出时间、文件校验和）

**导入：** AnyClaw → 解压到新位置 → 自动适配路径

导入前置检查：
- Node.js ≥ 18
- 磁盘空间 > 500MB
- OpenClaw 可用（自动安装）

路径适配：两台机器用户名可能不同 → 导入时自动检测当前用户的 `%APPDATA%` 路径

#### 2.12.9 工作区目录消失处理

当工作区目录不可达（用户删除、USB 拔出、网络断开）：

```
⚠️ 工作区目录不存在
D:\MyAIWorkspace\

○ 重新创建此工作区
○ 选择其他目录
○ 创建新工作区
```

#### 2.12.10 工作区模板

不同用途的工作区有不同的初始文件结构：

| 模板 | SOUL.md 风格 | 额外文件 | 默认权限 |
|------|-------------|---------|---------|
| 通用助手 | 友好、通用 | 无 | 基础（文件读写 + 网络） |
| 开发者 | 技术导向 | TOOLS.md（编译工具配置） | 开放（+ 命令执行） |
| 写作者 | 文学风格 | 素材目录 | 基础 |
| 数据分析 | 严谨、数据导向 | Python 环境配置 | 开放（+ 本地执行） |

#### 2.12.11 OpenClaw 发现机制

AnyClaw 查找 OpenClaw 的顺序：

1. config.json 中配置的路径
2. PATH 环境变量中的 `openclaw` 命令
3. npm 全局目录（`npm root -g`）
4. 常见安装位置扫描
5. 都找不到 → 引导用户安装或指定路径

#### 2.12.12 工作区生命周期

```
┌─ 安装 ────────────────────────────────────────┐
│  用户选择 4 个目录                              │
│  初始化工作区结构（按模板）                      │
│  写入 config.json + workspace.json             │
│  生成 AGENTS.md（含 MANAGED 区域）等            │
└───────────────────────────────────────────────┘
          │
┌─ 启动 ────────────────────────────────────────┐
│  读 config → 健康检查 → 工作区锁               │
│  发现 OpenClaw → 传入 --workspace              │
│  Gateway 连接 → 就绪                           │
└───────────────────────────────────────────────┘
          │
┌─ 运行 ────────────────────────────────────────┐
│  每次操作前校验 workspace.json                  │
│  设备变更触发新设备策略（见 2.13）               │
│  AGENTS.md/TOOLS.md 修改前备份到 .backups/      │
│  权限决策记录到审计日志                          │
└───────────────────────────────────────────────┘
          │
┌─ 退出 ────────────────────────────────────────┐
│  删除 .openclaw.lock                           │
│  保存 config.json                              │
│  清理临时文件                                  │
└───────────────────────────────────────────────┘
          │
┌─ 卸载/迁移 ───────────────────────────────────┐
│  可选保留工作区                                │
│  导出/导入迁移包                               │
└───────────────────────────────────────────────┘
```

#### 2.12.13 配置存储

**新增文件：**

| 文件 | 路径 | 管理方 | 说明 |
|------|------|--------|------|
| `workspace.json` | 工作区根目录 | AnyClaw | 运行时权限校验用 |
| `WORKSPACE.md` | 工作区根目录 | AnyClaw | 工作区元信息 |
| `.openclaw.lock` | 工作区根目录 | 运行时 | 实例锁 |
| `permissions.json` | `%APPDATA%\AnyClaw_LVGL\` | AnyClaw | 权限配置源 of truth |
| `permissions.log` | `%APPDATA%\AnyClaw_LVGL\logs\` | 运行时 | 审计日志 |

**更新文件：**

| 文件 | 变更 |
|------|------|
| `config.json` | 新增 `workspace_root`、`openclaw_path` 字段 |
| `paths.h` | 用户工作区路径从硬编码改为从 config 动态读取 |
| `AGENTS.md` | 新增 `<!-- ANYCLAW_MANAGED_START/END -->` 管理区域 |
| `TOOLS.md` | 新增 `<!-- ANYCLAW_MANAGED_START/END -->` 管理区域 |

---

### 2.13 权限系统

> **功能编号：** PERM-01 **优先级：** P0 **状态：** 🔧 核心已实现（数据结构/路径校验/配置持久化/Settings UI/审计日志）

#### 2.13.1 设计目标

OpenClaw Agent 能执行高权限操作（读写文件、执行命令、访问网络、访问设备）。必须在安装时由用户划定边界，并在运行时强制执行。

**当前代码状态（2026-04-07）：** 已具备权限配置结构、路径边界校验、Settings 权限编辑页、审计日志；运行时拦截已在 OpenClaw 命令执行入口做首层接入（`exec_cmd`），设备信任表与交互式 ask 弹窗仍待补全。

#### 2.13.2 三层权限模型

```
┌──────────────────────────────────────────────┐
│  第 1 层：工作区边界 (Workspace Boundary)      │
│  → Agent 能在哪个目录树下活动                  │
├──────────────────────────────────────────────┤
│  第 2 层：操作权限 (Operation Permissions)     │
│  → 文件读/写、命令执行、网络、设备              │
├──────────────────────────────────────────────┤
│  第 3 层：资源上限 (Resource Limits)           │
│  → 磁盘占用、超时、请求频率、并发数             │
└──────────────────────────────────────────────┘
```

#### 2.13.3 第 1 层：工作区边界

Agent 的所有文件操作限定在工作区根目录内。

**规则：**
- 工作区根目录下的文件：默认可读写
- 工作区根目录之外的路径：默认禁止
- 用户可在安装时添加额外的"外部目录授权"（只读或读写）

**路径白名单/黑名单：**

```json
"workspace_root": "D:\\MyAIWorkspace\\",
"extra_allowed_paths": [
  {"path": "C:\\Users\\xxx\\Documents\\", "mode": "read"},
  {"path": "D:\\Shared\\", "mode": "read_write"}
],
"denied_paths": ["C:\\Windows\\", "C:\\Program Files\\"]
```

**路径校验逻辑：**
1. 检查是否在 `denied_paths` → 命中则拒绝
2. 检查是否在 `workspace_root` 下 → 命中则允许
3. 检查是否在 `extra_allowed_paths` → 命中则按 mode 处理
4. 都不命中 → 默认拒绝（白名单模式）

#### 2.13.4 第 2 层：操作权限

每种操作类型独立授权，四种取值：`allow`（允许）、`deny`（禁止）、`ask`（每次询问）、`read_only`（仅读，仅适用于文件类）。

**文件系统权限：**

| 权限键 | 默认值 | 说明 |
|--------|--------|------|
| `fs_read_workspace` | `allow` | 读取工作区内文件 |
| `fs_write_workspace` | `allow` | 写入工作区内文件 |
| `fs_read_external` | `deny` | 读取外部目录 |
| `fs_write_external` | `deny` | 写入外部目录 |

**命令执行权限：**

| 权限键 | 默认值 | 说明 |
|--------|--------|------|
| `exec_shell` | `ask` | 执行 shell 命令（高危） |
| `exec_install` | `deny` | 安装软件（apt/npm/pip） |
| `exec_delete` | `ask` | 删除文件（rm/del） |

**网络权限：**

| 权限键 | 默认值 | 说明 |
|--------|--------|------|
| `net_outbound` | `allow` | 出站 HTTP/HTTPS（API 调用需要） |
| `net_inbound` | `deny` | 入站连接（监听端口） |
| `net_intranet` | `deny` | 访问内网 IP 段 |

**设备权限：**

| 权限键 | 默认值 | 说明 |
|--------|--------|------|
| `device_camera` | `deny` | 摄像头 |
| `device_mic` | `deny` | 麦克风 |
| `device_screen` | `ask` | 屏幕截图/录屏 |
| `device_usb_storage` | `ask` | USB 存储设备 |
| `device_remote_node` | `ask` | 远程 OpenClaw 节点 |
| `device_ssh_host` | `ask` | SSH 远程主机 |
| `device_new_device` | `ask` | 新插入设备（默认策略） |

**系统权限：**

| 权限键 | 默认值 | 说明 |
|--------|--------|------|
| `clipboard_read` | `allow` | 读取剪贴板 |
| `clipboard_write` | `allow` | 写入剪贴板 |
| `system_modify` | `deny` | 修改系统设置 |

#### 2.13.5 第 3 层：资源上限

| 资源键 | 默认值 | 说明 |
|--------|--------|------|
| `max_disk_mb` | 2048 | 工作区最大磁盘占用 |
| `cmd_timeout_sec` | 30 | 单条命令超时（超时自动 kill） |
| `net_rpm` | 60 | 网络请求频率上限（次/分钟） |
| `max_subagents` | 5 | 最大并发子代理数 |
| `max_file_read_kb` | 500 | 单次文件读取上限 |
| `max_remote_connections` | 3 | 最大远程设备连接数 |

#### 2.13.6 设备管理

**设备分类：**

```
设备 (Devices)
├── 本地外设 (Local Peripherals)
│   ├── 摄像头、麦克风、屏幕截图、扬声器
│
├── 本地存储 (Local Storage)
│   ├── 内置磁盘（已授权工作区内）
│   ├── USB 移动硬盘 / U 盘
│   ├── SD 卡、网络映射驱动器
│
├── 远程设备 (Remote Nodes)
│   ├── 已配对的 OpenClaw 节点
│   ├── SSH 远程主机
│   ├── 云服务器实例
│   └── IoT 设备
│
└── 新插入设备 (Hot-Plug)
    ├── USB 存储（DBT_DEVICEARRIVAL + GUID_DEVINTERFACE_DISK）
    ├── USB 外设（摄像头、麦克风）
    ├── 蓝牙设备
    └── Thunderbolt 设备
```

**新插入设备默认策略：** 一律询问，不自动放行。

检测到新设备插入时弹窗：
```
🔌 新设备已连接
类型: USB 存储设备
名称: Kingston DataTraveler 32GB
盘符: F:\

允许 AnyClaw 访问？
○ 允许（读写）  ○ 允许（只读）  ○ 拒绝
☐ 记住此设备（下次不再询问）
```

**设备信任表：**

用户选择"记住此设备"后，设备信息写入 `trusted_devices`：

```json
"trusted_devices": [
  {
    "type": "usb_storage",
    "id": "SanDisk_Ultra_SERIAL123",
    "name": "我的U盘",
    "granted": "2026-04-06",
    "access": "read_write"
  },
  {
    "type": "remote_node",
    "id": "node_abc123",
    "name": "我的手机",
    "granted": "2026-04-06",
    "access": "read"
  }
]
```

**设备唯一标识：** 用序列号/硬件 ID 做唯一标识，防止伪造同名设备绕过信任。

**设备状态变更检测：**
- USB 设备拔出 → 停止对该设备的所有操作
- 远程节点离线 → 标记不可用，队列任务暂停
- 设备权限被外部修改（组策略等）→ 提示用户

#### 2.13.7 运行时强制执行（分阶段）

权限不只是文档，是代码级拦截。OpenClaw 工具调用层在每次操作前校验 workspace.json。

**校验流程：**

```
Agent 调用工具（read / write / exec / web_fetch ...）
         │
         ▼
OpenClaw Runtime 读取 workspace.json
         │
         ├── 路径在允许范围内？  → ✅ 放行
         ├── 路径在禁止列表？    → ❌ 拦截，返回错误
         ├── 操作权限 = allow？  → ✅ 放行
         ├── 操作权限 = deny？   → ❌ 拦截
         ├── 操作权限 = ask？    → 🔔 通知 AnyClaw → 弹窗 → 用户决定
         └── 不在任何列表？      → 默认拒绝（白名单模式）
```

**当前实现状态：**
- 已在 `openclaw_mgr.cpp` 的 `exec_cmd()` 前置权限校验（按 `exec_shell/exec_install/exec_delete` 分类）
- 已在 `session_manager.cpp`、`health.cpp` 的本地命令执行入口接入 `exec_shell` 校验
- `allow` 放行、`deny` 拦截并写审计、`ask` 进入交互决策（仅本次/永久/拒绝）
- 仍需把同类拦截扩展到其他命令执行入口（如 `selfcheck.cpp`）

#### 2.13.8 运行时权限升级（单次放行）

Agent 偶尔需要越权操作，但不想永久改权限。

弹窗确认：
```
🔒 Agent 请求临时权限
操作: 安装软件
命令: apt install ffmpeg
当前权限: ❌ 禁止

○ 允许（仅本次）  ○ 允许（永久开启此权限）  ○ 拒绝
```

- 单次放行不过夜——下次启动仍按 permissions.json 配置
- 永久开启 → 更新 permissions.json + 同步到 AGENTS.md/TOOLS.md 的 MANAGED 区域

#### 2.13.9 AGENTS.md / TOOLS.md 双向同步

**MANAGED 区域机制：**

- 当前实现（v1）：`permissions.json -> AGENTS.md/TOOLS.md` 单向投影已落地。
- 同步入口：启动加载权限后执行一次；Settings 保存权限后再同步一次。
- 文件策略：若存在 `ANYCLAW_MANAGED_START/END` 标记则替换标记区间；若不存在则自动追加标记区块。
- 待实现（v2.1）：`md -> json` 反向解析、冲突弹窗、写入前 `.backups` 备份轮转。

---

### 2.13.5 授权系统（License System）

> **功能编号：** LIC-02 **优先级：** P0 **状态：** 🔧 设计完成，待实施

#### 概述

AnyClaw 安装后提供免费试用期，到期后需要输入授权密钥继续使用。密钥基于时间递增机制，每把密钥的有效期是前一把的 2 倍。

#### 时间线

```
安装 ──→ 12h 免费 ──→ 需要密钥#0 ──→ 12h ──→ 需要密钥#1 ──→ 24h ──→ 需要密钥#2 ──→ 48h ...
         (试用期)      (续期 0)               (续期 1)              (续期 2)
```

| 序列号 | 有效期 | 累计（从安装算） | 说明 |
|--------|--------|-----------------|------|
| 试用期 | 12h | 0-12h | 无需密钥 |
| seq=0 | 12h | 12-24h | 首次续期 |
| seq=1 | 24h | 24-48h | 2倍 |
| seq=2 | 48h | 48-96h | 4倍 |
| seq=3 | 96h (4天) | 96-192h | 8倍 |
| seq=4 | 192h (8天) | 192-384h | 16倍 |
| seq=N | 12×2^N h | — | 指数增长 |

#### 密钥算法

```
secret   = "AnyClaw-2026-LOVE-GARLIC"  (编译时写入 exe，不公开)
duration = 12h × 2^seq
expiry   = now() + duration
payload  = seq(2B) + expiry(8B)         // 10 字节
mac      = HMAC-SHA256(secret, payload)[:8]  // 8 字节
key      = Base32(payload + mac)         // 18 字节 → XXXXX-XXXXX-XXXXX
```

**验证流程（AnyClaw 内部）：**
1. 解码 Base32 → payload(10B) + mac(8B)
2. 重新计算 HMAC，比对 mac → 不匹配则拒绝
3. 检查 expiry > now() → 过期则拒绝
4. 检查 seq ≥ current_seq → 序列号倒退则拒绝（防重放）
5. 通过 → 更新 current_seq = seq，写入 config.json

#### 密钥输入界面

- 过期弹窗：全屏遮罩 + 大蒜图标 + "试用期已结束" + 密钥输入框 + 激活按钮
- 激活成功 → 弹窗消失，显示剩余时间
- 底部状态栏显示：剩余 XXh（低时间时变红）

#### 存储

```json
// config.json
{
  "license_seq": 2,
  "license_expiry": 1743981600
}
```

#### 生成器

密钥生成工具：`tools/keygen.py`

```
python3 keygen.py 0      # 生成序列号 0 的密钥（12h）
python3 keygen.py 3      # 生成序列号 3 的密钥（96h）
python3 keygen.py --verify KEY  # 验证密钥
python3 keygen.py --list  # 显示所有序列号对应的时长
```

### 2.14 聊天区文字选中复制

- **功能编号：** SEL-01 **优先级：** P1 **状态：** ✅ 已实现
- 聊天区域（用户气泡、AI 气泡、流式输出 label）支持鼠标拖拽选中文字
- 选中后 Ctrl+C 复制到系统剪贴板（UTF-8 → CF_UNICODETEXT）
- 输入框（chat_input textarea）同样支持选中 + Ctrl+C/V/X
- 技术实现：
  - `make_label_selectable(lbl)` 为 label 添加 CLICKABLE 标志 + PRESSED/PRESSING/CLICKED/DEFOCUSED 事件回调
  - 动态创建的气泡在 `chat_add_user_bubble()` / `chat_add_ai_bubble()` 中调用
  - 全局 Ctrl+C 通过 SDL 事件监听 → `global_ctrl_c_copy()` 从 `g_selected_label` 提取选中文字
- **提交**: `7ad3515`

---

### 2.15 贴边大蒜头（Edge Snap Garlic Dock）

- **功能编号：** SNAP-01 **优先级：** P2 **状态：** ✅ 已实现（Win32 版）
- 拖拽窗口到屏幕边缘（左/右/上/下），松开鼠标后窗口隐藏，屏幕边缘显示浮动大蒜头
- 大蒜头规格：
  - 窗口尺寸 80×110 px，WS_EX_LAYERED 透明背景（洋红色色键）
  - 大蒜身体 `garlic_48.png`，葱头 `garlic_sprout.png`
  - 始终置顶（WS_EX_TOPMOST），不在任务栏显示（WS_EX_TOOLWINDOW）
- 交互：
  - 鼠标悬停 → 葱头以正弦波 ±12° 摇摆（PlgBlt 仿射变换，1.5Hz）
  - 鼠标点击（左键/右键）→ 恢复主窗口到原始位置
- 检测逻辑：
  - `WM_EXITSIZEMOVE`（Win32 子类化）触发 `garlic_snap_to_edge()`
  - 计算窗口四边与屏幕边缘的距离，最近边 ≤ 20px 则吸附
  - 非最大化状态才触发
- **已知问题**：首次触发正常，二次拖拽到边缘可能不触发（需调试 WM_EXITSIZEMOVE 重复注册）
- **提交**: `b1c7698`（初版 SDL 线程版）, `fbc982f`（重写为纯 Win32）

---

### 2.16 剪贴板 Unicode 支持

- **功能编号：** CLIP-01 **优先级：** P1 **状态：** ✅ 已实现
- 剪贴板操作从 CF_TEXT（ANSI）升级为 CF_UNICODETEXT（UTF-16），支持中文复制粘贴
- 涉及函数：
  - `clipboard_copy_to_win(text)`：UTF-8 → UTF-16（MultiByteToWideChar）→ SetClipboardData(CF_UNICODETEXT)
  - `clipboard_paste_from_win()`：GetClipboardData(CF_UNICODETEXT) → UTF-16 → UTF-8（WideCharToMultiByte）
- 影响范围：聊天区选中复制、输入框右键菜单复制/粘贴、Ctrl+V 粘贴
- **提交**: `b1c7698`

---

### 2.17 图标系统升级

- **功能编号：** ICON-02 **优先级：** P2 **状态：** ✅ 已实现
- 托盘图标尺寸：48px → 96px（新增 `tray_*.png` 96px 版本，LANCZOS 上采样）
- 窗口标题栏图标：48/32/16 → 96/64/32（WM_SETICON + SetClassLongPtr）
- `create_tray_icon()` 默认 size 参数从 48 改为 96
- **已知问题**：资源嵌入的图标（MakeIntResource(1)）加载失败（err=1813），依赖文件 fallback
- **提交**: `7ad3515`

AGENTS.md 和 TOOLS.md 中使用标记划分 AnyClaw 管理区域：

```markdown
## 用户自由区域（随便写，AnyClaw 不动）
用户的自定义规则、笔记、经验...

<!-- ANYCLAW_MANAGED_START -->
## 工作区边界
- 根目录: D:\MyAIWorkspace\
- 禁止访问: C:\Windows\

## 权限规则
- 命令执行: ⚠️ 每次询问
<!-- ANYCLAW_MANAGED_END -->

## 更多自由区域（随便写）
补充内容...
```

**规则：**
- `<!-- ANYCLAW_MANAGED_START -->` 到 `<!-- ANYCLAW_MANAGED_END -->` 之间 = AnyClaw 管理
- 标记之外 = 用户自由区域，AnyClaw **只读不写**
- AnyClaw 写入时只替换标记之间的内容，不动标记之外的任何内容

**Source of Truth：** `permissions.json` 是唯一权限真相。AGENTS.md/TOOLS.md 的 MANAGED 区域是其"可读投影"。

**双向同步场景：**

| 场景 | 处理 |
|------|------|
| 用户在 AnyClaw UI 改权限 | 更新 permissions.json → 替换 MANAGED 区域 → 更新 workspace.json |
| 用户手动改了 MANAGED 区域 | AnyClaw 启动时检测差异 → 弹窗选择：以文件为准 / 以 json 为准 / 跳过 |
| 用户改了自由区域 | AnyClaw 不动，正常加载 |
| 用户删除了 MANAGED 标记 | 启动时检测 → 弹窗选择：重新插入管理区域 / 仅用 json / 忽略 |
| Skill 安装需要额外权限 | 安装时提示需要开启哪些权限 → 用户确认后自动更新 |
| 其他工具改了 AGENTS.md | 启动时检测 → 同"用户手动改了 MANAGED 区域"处理 |

**文件修改备份：** AnyClaw 每次修改 AGENTS.md/TOOLS.md 前，备份到 `工作区\.backups\`，最多保留 10 个备份。

#### 2.13.10 权限配置存储

**`permissions.json`**（`%APPDATA%\AnyClaw_LVGL\`）— AnyClaw 源 of truth：

```json
{
  "version": 1,
  "workspace_root": "D:\\MyAIWorkspace\\",
  "extra_allowed_paths": [
    {"path": "C:\\Users\\xxx\\Documents\\", "mode": "read"}
  ],
  "denied_paths": ["C:\\Windows\\", "C:\\Program Files\\"],
  "permissions": {
    "fs_read_workspace": "allow",
    "fs_write_workspace": "allow",
    "fs_read_external": "deny",
    "fs_write_external": "deny",
    "exec_shell": "ask",
    "exec_install": "deny",
    "exec_delete": "ask",
    "net_outbound": "allow",
    "net_inbound": "deny",
    "net_intranet": "deny",
    "device_camera": "deny",
    "device_mic": "deny",
    "device_screen": "ask",
    "device_usb_storage": "ask",
    "device_remote_node": "ask",
    "device_ssh_host": "ask",
    "device_new_device": "ask",
    "clipboard_read": "allow",
    "clipboard_write": "allow",
    "system_modify": "deny"
  },
  "trusted_devices": [],
  "limits": {
    "max_disk_mb": 2048,
    "cmd_timeout_sec": 30,
    "net_rpm": 60,
    "max_subagents": 5,
    "max_file_read_kb": 500,
    "max_remote_connections": 3
  }
}
```

**`workspace.json`**（工作区根目录）— 运行时直接读取：

```json
{
  "version": 1,
  "workspace_root": "D:\\MyAIWorkspace\\",
  "denied_paths": ["C:\\Windows\\"],
  "permissions": {
    "fs_read": "allow",
    "fs_write": "allow",
    "exec": "ask",
    "net_outbound": "allow",
    "device": "deny"
  },
  "limits": {
    "cmd_timeout_sec": 30,
    "max_subagents": 5,
    "net_rpm": 60
  },
  "trusted_devices": []
}
```

**配置冲突优先级：** workspace.json > permissions.json > AGENTS.md

理由：workspace.json 是运行时直接读取的，最权威最快。

#### 2.13.11 审计日志（已实现基础版）

所有权限决策记录到 `%APPDATA%\AnyClaw_LVGL\audit.log`：

```
[2026-04-07 19:20:01] action=perm_load target=permissions.json decision=ok
[2026-04-07 19:20:03] action=perm_set target=exec_shell decision=ask
[2026-04-07 19:20:05] action=exec_check target=openclaw gateway start decision=allow_once
[2026-04-07 19:20:07] action=path_check target=C:\Windows\system32\hosts decision=deny:external_default
```

AnyClaw 当前已提供日志落盘；"权限历史"可视化面板仍待实现。

#### 2.13.12 Skill 安装与权限交互

用户通过 ClawHub 安装 Skill 时，检查 Skill 所需权限是否与当前配置冲突：

- 权限充足 → 直接安装
- 权限不足 → 提示需要开启哪些权限 → 用户确认后自动更新 permissions.json
- 用户拒绝 → 取消安装

#### 2.13.13 Settings 权限管理面板（已实现 v1）

AnyClaw Settings 已新增 **Permissions** Tab（v1）：

- 9 个核心权限项下拉编辑（allow/deny/ask/read_only）
- 保存后写入 `permissions.json`
- 打开设置时自动加载并绑定工作区根目录
- 启动时自动初始化权限（首启写默认配置）

**待实现（v2.1）：**
- 设备信任表管理
- 资源上限编辑
- 权限历史可视化与筛选
- 权限导入/导出

---

### 2.18 任务列表Session计数

- **功能编号：** TASK-01 **优先级：** P1 **状态：** ✅ 已实现
- "Task List"标题旁显示 `(N)` 计数badge，反映当前活跃会话数
- 数据来源：`openclaw gateway call sessions.list --json`，解析 `ageMs` < 5分钟的会话
- Busy状态时列出每个session的渠道来源（webchat/telegram/discord等）
- 30秒健康检查周期自动刷新计数
- **提交**: `852d717`

### 2.19 SSE流式超时保护

- **功能编号：** STREAM-01 **优先级：** P0 **状态：** ✅ 已实现
- SSE流式请求添加超时看门狗：30秒无新数据 或 总耗时>45秒 → 强制结束
- 防止Gateway连接阻塞导致UI卡死（新消息无法发送）
- 超时时显示"⚠️ AI 回复超时，请重试。"
- **提交**: `73117ad`

### 2.20 大蒜贴边Dock（最终版）

- **功能编号：** SNAP-02 **优先级：** P2 **状态：** ✅ 已实现
- 拖拽窗口到屏幕边缘 → 隐藏主窗口 → 显示大蒜头图片
- 悬停大蒜头 → 葱头摇摆动画（PlgBlt仿射变换，±12°，1.5Hz）
- 点击大蒜头 → 恢复主窗口
- WM_ACTIVATE自动恢复（Alt+Tab/任务栏点击）
- stb_image加载PNG（Wine兼容）
- 四边吸附 + 屏幕边界clamp
- **提交**: `fbc982f`, `b273c61`, `3be4575`

---

### 2.21 PRD 风险复核与补强（本轮新增）

#### 2.21.1 已识别风险（按优先级）

| 风险ID | 风险描述 | 影响 | 处置优先级 |
|------|------|------|------|
| RISK-PERM-ASK | ask 交互闭环已实现，但跨线程/无界面场景的交互可用性仍需压测 | 极端场景可能出现确认体验不一致 | P1 |
| RISK-CONFIG-SOURCE | 文档中 `workspace.json > permissions.json` 的优先级与当前实现不一致（当前以 `permissions.json` 为主） | 运行行为与文档不一致，易误配 | P0 |
| RISK-MANAGED-BACKUP | MANAGED 区域同步已具备写前备份（保留10份），但缺少策略配置化 | 备份数量与保留策略不可按场景调整 | P2 |
| RISK-AUDIT-IMMUTABLE | 审计日志可写但不可验签，缺少防篡改能力 | 安全追溯可信度不足 | P1 |
| RISK-POLICY-COVERAGE | 运行时命令拦截已覆盖部分入口，仍有漏网路径 | 策略执行不完全 | P1 |

#### 2.21.2 需求补充（强制）

1. `ask` 必须进入交互决策：`仅本次允许 / 永久允许 / 拒绝`，禁止静默自动放行。  
2. 统一配置源：以 `permissions.json` 为 source of truth，`workspace.json` 为运行时投影；启动时自动做一致性校验。  
3. MANAGED 同步写前备份：写入 `AGENTS.md/TOOLS.md` 前自动备份至 `.backups`，保留最近 N=10。  
4. 审计日志增加完整性字段（链式 hash 或签名摘要）用于追溯校验。  
5. 对所有本地命令执行入口建立统一拦截层，禁止散落式绕过。  

#### 2.21.3 关键需求逻辑（可执行）

**A. ask 权限决策状态机**

```
请求到达 -> 读取权限值
  -> allow      => 放行 + 审计(action=exec_check, decision=allow)
  -> deny       => 拒绝 + 审计(decision=deny)
  -> ask        => 弹窗决策
                   -> 仅本次允许  => 放行 + 审计(decision=allow_once)
                   -> 永久允许    => 修改 permissions.json + 同步 MANAGED + 放行
                   -> 拒绝        => 拒绝 + 审计(decision=deny_by_user)
```

**B. 配置一致性逻辑（启动时）**

1. 读取 `permissions.json`（主配置）  
2. 计算 `workspace.json` 投影并比较哈希  
3. 不一致时：自动重建 `workspace.json`，并写审计 `config_sync=rebuilt`  
4. 重建失败时：降级只读并提示用户（不进入高危操作路径）  

当前实现状态：
- 已实现 `permissions.json -> workspace.json` 投影同步
- 启动与设置保存路径均会触发一致性同步
- 已写审计：`up_to_date / created / rebuilt / write_failed`

**C. MANAGED 区域写入逻辑**

1. 写入前备份目标文件到 `.backups/`  
2. 检测标记存在性：有标记则替换区间，无标记则追加区块  
3. 写入后回读校验（标记完整、UTF-8 可解析）  
4. 失败回滚：使用最新备份恢复并告警  

#### 2.21.4 验收标准（DoD）

| 编号 | 验收项 | 通过标准 |
|------|------|------|
| AC-PERM-ASK-01 | ask 必须弹窗 | 任一 `ask` 命令触发可见交互，不允许静默放行 |
| AC-PERM-ASK-02 | 永久授权生效 | 选择“永久允许”后，`permissions.json` 与 MANAGED 区域同步更新 |
| AC-CONFIG-01 | 配置一致性 | 删除或篡改 `workspace.json` 后重启可自动恢复且有审计记录 |
| AC-MANAGED-01 | 安全写入 | 同步前生成备份；故障注入时可自动回滚 |
| AC-AUDIT-01 | 审计可追溯 | 审计日志包含时间、动作、目标、决策、链式校验字段 |

#### 2.21.5 数据结构补充（字段级）

**permissions.json 新增字段：**

```json
{
  "version": 2,
  "policy_hash": "sha256:...",
  "last_sync_at": "2026-04-07T20:10:00+08:00",
  "ask_policy": {
    "default_ttl_sec": 0,
    "allow_once_cache_sec": 0
  }
}
```

**audit.log 建议结构化格式（可落盘为 jsonl）：**

```json
{"ts":"2026-04-07T20:10:00+08:00","trace_id":"...","action":"exec_check","target":"openclaw gateway start","decision":"allow","chain_prev":"...","chain_curr":"..."}
```

字段说明：
- `policy_hash`：对权限关键字段计算哈希，检测配置漂移  
- `trace_id`：一次用户动作在多模块中的关联 ID  
- `chain_prev/chain_curr`：链式完整性校验字段  

#### 2.21.6 异常与回退策略

| 场景 | 系统行为 | 用户可见反馈 |
|------|------|------|
| ask 弹窗超时/关闭 | 默认拒绝（fail-closed） | toast + 审计记录 `deny:timeout` |
| permissions.json 解析失败 | 载入最小安全默认策略（deny 高危） | 启动告警 + 引导修复 |
| MANAGED 写入失败 | 自动回滚最近备份 | 弹窗提示并保留错误日志 |
| audit 写入失败 | 主流程不中断，但写 stderr + 内存环形缓冲 | 设置页显示“审计异常”状态 |
| 配置重建失败 | 进入只读降级模式 | 状态栏红色告警 |

#### 2.21.7 测试矩阵（最小集）

| 测试ID | 类型 | 前置 | 预期 |
|------|------|------|------|
| T-PERM-ASK-001 | 功能 | `exec_shell=ask` | 弹窗出现，三分支都正确 |
| T-PERM-ASK-002 | 回归 | `exec_shell=deny` | 命令被拒绝，审计有记录 |
| T-CONF-001 | 健壮性 | 删除 workspace.json | 启动后自动重建成功 |
| T-MANAGED-001 | 健壮性 | 人工破坏 MANAGED 区块 | 同步后恢复合法结构 |
| T-AUDIT-001 | 安全 | 连续执行 10 次高危命令 | 审计链完整无断裂 |
| T-PERF-001 | 性能 | 100 次快速权限校验 | p95 < 5ms（本地） |

---

### 2.22 参考项目对标后的必要新增功能（CCB / DeerFlow）

> 参考：`claude-code-best/claude-code` 与 `bytedance/deer-flow`

#### 2.22.1 必要新增（建议纳入 v2.1）

| 功能编号 | 功能 | 价值 | 优先级 |
|------|------|------|------|
| OBS-01 | 可观测性追踪（会话链路、工具耗时、错误聚合） | 降低排障成本，支持线上稳定性治理 | P1 |
| SBX-01 | 沙箱执行模式（高危命令/脚本隔离运行） | 显著降低主机风险面 | P0 |
| BOOT-01 | 一键环境体检与自修复（依赖、端口、配置） | 降低安装/升级失败率 | P1 |
| FLAG-01 | Feature Flag 开关体系 | 低风险灰度发布与快速回滚 | P1 |
| MEM-01 | 长记忆整理任务（周期压缩/索引） | 长会话性能与上下文质量提升 | P2 |

当前实现状态（2026-04-07）：
- `SBX-01` 已完成 v1：高危命令进入 sandbox 工作目录 + Job Object 内存/进程数约束 + 超时终止。
- `SBX-01` v2.1 待补：CPU 限速与网络隔离能力。

#### 2.22.2 需求逻辑

- `SBX-01` 与 `PERM-01` 绑定：权限策略决定是否进入沙箱、沙箱配置决定执行上限。  
- `OBS-01` 与审计日志联动：审计记录安全事件，追踪记录性能与链路，两者合并可定位“谁触发了什么慢/错操作”。  
- `FLAG-01` 作为发布门控：新能力默认关闭，仅在灰度环境启用。  
- `BOOT-01` 作为安装入口：首次启动先体检再引导，失败时给出可执行修复命令。  

#### 2.22.3 里程碑拆分（建议）

| 里程碑 | 时间窗 | 交付 |
|------|------|------|
| M1 | v2.1-alpha | ask 交互闭环 + 统一拦截层 + MANAGED 双向同步基础 |
| M2 | v2.1-beta | 沙箱最小可用（命令隔离+超时+资源限制）+ 观测链路基础指标 |
| M3 | v2.1-rc | 一键体检自修复 + Feature Flag 灰度 + 关键回归测试 |

#### 2.22.4 非功能性需求（新增）

- **安全性**：高危命令默认拒绝或 ask，禁止默认开放。  
- **可靠性**：配置重建与文件回滚具备幂等性；崩溃后可恢复到最近稳定状态。  
- **可观测性**：每次关键路径操作都要可追踪（trace_id / action / latency / outcome）。  
- **可维护性**：策略判断集中在统一模块，避免多处重复实现导致策略漂移。  
- **兼容性**：Windows 优先，Linux 路径规则与进程探活逻辑需等价实现。  

#### 2.22.5 与现有代码映射（开发入口）

| 需求 | 主要文件 | 说明 |
|------|------|------|
| ask 交互闭环 | `ui_main.cpp`, `openclaw_mgr.cpp`, `session_manager.cpp`, `health.cpp` | 弹窗决策 + 执行前拦截 |
| 配置一致性重建 | `permissions.cpp`, `workspace.cpp`, `main.cpp` | 启动阶段一致性检查与修复 |
| MANAGED 双向同步 | `workspace.cpp`, `ui_settings.cpp` | 标记区替换/解析 + 保存触发 |
| 审计链 | `permissions.cpp` | 统一写审计与完整性字段 |
| 沙箱执行 | `openclaw_mgr.cpp`（后续抽象 `executor` 模块） | 高危命令隔离入口 |

---

### 2.23 本地知识库（Knowledge Base）

- **功能编号：** KB-01 **优先级：** P1 **状态：** 🆕 待实施

#### 2.23.1 用户价值

- 用户可把常用文档、图片、语音、代码目录沉淀为“可检索知识”
- 聊天时可引用知识片段，避免反复上传同一资料

#### 2.23.2 核心逻辑

1. 用户添加本地目录（白名单）作为知识源  
2. 后台建立索引（文件元数据 + 文本内容 + 图片/OCR摘要 + 音频ASR摘要）  
3. 聊天检索时先命中知识库，再拼接到上下文  
4. 变更检测（新增/修改/删除）增量更新索引  

#### 2.23.3 最小数据模型

- `kb_sources.json`：知识源路径、启用状态、上次扫描时间
- `kb_index.db`：倒排索引/向量索引（后续可切换实现）
- `kb_meta`：文件类型、hash、mtime、摘要

---

### 2.24 界面模式（聊天 / 语音 / 工作）

- **功能编号：** MODE-01 **优先级：** P1 **状态：** 🔧 实施中（V1 已落地）

#### 2.24.1 模式定义

- **聊天模式**：文本对话 + 文件发送 + 知识引用  
- **语音模式**：实时语音输入/播放，转写与回复可见  
- **工作模式**：任务面板、会话管理、工具执行、日志与状态一屏联动  

#### 2.24.2 UI 设计要求

- 顶部统一模式切换条，支持快捷键切换
- 各模式保留各自输入状态，切换不丢上下文
- 模式切换延迟目标：< 150ms（UI 可交互）

#### 2.24.3 当前实现状态（V1）

- 右侧主区已支持 `Chat / Voice / Work` 模式切换
- 聊天模式与输入状态可在模式切换后保持
- 语音与工作模式已具备独立容器，便于后续增量扩展

---

### 2.25 文件/图片/目录发送与可点击打开

- **功能编号：** SHARE-01 **优先级：** P0 **状态：** 🔧 实施中（V1 已落地）

#### 2.25.1 交互目标

- 聊天输入区支持：文件、图片、目录拖拽/选择发送
- 聊天气泡中可视化显示附件卡片（类型/名称/大小/缩略图）
- 点击附件可直接打开（目录打开资源管理器，文件按系统默认程序）

#### 2.25.2 关键约束

- 路径必须通过权限系统校验（工作区边界与外部授权）
- 目录发送默认仅发送目录树摘要；用户可选择递归深度
- 图片消息支持缩略图预览，点击查看原图

#### 2.25.3 当前实现状态（V1）

- 已支持选择文件/目录后在聊天区生成附件卡片
- 附件卡片支持点击打开本地文件/目录
- 图片附件卡片已支持基础预览能力（后续补全稳定性与错误态）

---

### 2.26 头像与角色配置（AI / 用户双侧）

- **功能编号：** PROFILE-01 **优先级：** P1 **状态：** 🆕 待实施

#### 2.26.1 AI 侧属性

- 头像、名字、性格、技能标签、角色
- 与 OpenClaw 工作区属性文件映射（AGENTS/SOUL/TOOLS/IDENTITY）

#### 2.26.2 用户侧属性

- 头像、名字、性格、角色
- 多用户配置隔离（工作区级）

#### 2.26.3 同步逻辑

- UI 修改 -> 本地 profile 配置 -> 受管文档投影（MANAGED 区）  
- 手工修改受管文档 -> 启动检测冲突 -> 选择以 UI 或文件为准  

---

### 2.27 AnyClaw 双端远程协作（远程桌面 + 语音 + 控制）

- **功能编号：** REMOTE-01 **优先级：** P0 **状态：** 🆕 待实施（高安全风险）

#### 2.27.1 能力范围

- 两个 AnyClaw 用户点对点会话
- 远程桌面画面共享
- 双向语音通话
- 经授权后远程控制对方输入（鼠标/键盘）

#### 2.27.2 安全设计（强制）

- 会话必须双向确认（发起方 + 接收方）
- 控制权限分级：仅看屏 / 语音 / 完整控制
- 每次控制会话必须可随时一键中止（双方都可中止）
- 全链路加密 + 审计日志记录（开始/结束/授权变更）

#### 2.27.3 交互原则

- 默认最小权限（仅看屏）
- UI 显示“正在被远程控制”持续提示，不可隐藏
- 高风险操作二次确认（文件删除、命令执行、系统设置）

---

### 2.28 控制权模式（AI 控制 AnyClaw / 用户控制 AnyClaw）

- **功能编号：** CTRL-01 **优先级：** P0 **状态：** 🔧 实施中（配置与 UI 已落地）

#### 2.28.1 模式定义

- **用户模式（默认）**：用户手动发起主要动作，AI 仅建议与辅助
- **AI 模式**：AI 可按策略自动驱动 AnyClaw 执行动作（需受权限与审计约束）

#### 2.28.2 需求逻辑

1. 控制模式在工作模式中显式可见可切换  
2. 模式切换必须持久化到本地配置  
3. AI 模式下所有高风险动作仍走权限检查与审计  
4. 用户可随时一键回退到用户模式  

#### 2.28.3 当前实现状态（V1）

- Work 面板已提供控制模式切换（User/AI）
- 模式值已写入 `config.json` 并在启动时恢复
- 当前为“控制权框架”阶段，后续补齐 AI 自动驱动执行策略

---

### 2.29 大模型接入模式（网关模式 / 直连 API 模式）

- **功能编号：** LLM-ACCESS-01 **优先级：** P0 **状态：** 🔧 实施中（当前固定 Gateway，Direct API 暂缓）

#### 2.29.1 模式定义

- **网关模式**：AnyClaw 通过 OpenClaw Gateway 调用模型（统一鉴权与会话）
- **API 模式**：AnyClaw 直接调用模型厂商 API（绕过网关）

#### 2.29.2 需求逻辑

1. 用户可在工作模式中切换接入模式  
2. 切换结果持久化并在重启后恢复  
3. 网关模式优先用于稳定会话与统一观测  
4. API 模式需补齐各厂商兼容层与失败回退策略  

#### 2.29.3 当前实现状态（V1）

- Work 面板保留接入模式入口，但产品策略暂定只启用 Gateway
- `config.json` 中若出现 Direct API 配置，启动时自动回落到 Gateway
- 运行时调用链当前以网关链路为唯一正式路径

#### 2.29.4 新增：Gemma 4 本地模型安装选项（LLM-LOCAL-GEMMA4-01）

- **目标：** 在安装/工作配置中支持本地 Gemma 4 选装，兼顾轻量开箱与高配扩展
- **范围：**
  1. 小模型（如 Gemma 4 2B）可内置到 AnyClaw 安装包  
  2. 大模型（如 Gemma 4 9B/27B）不内置，通过网络下载  
  3. 用户可选择不安装本地模型（继续仅网关模式）  
  4. 用户可多选本地模型安装项（2B/9B/27B）  
  5. 系统根据本机配置给出一个“推荐模型”（内存优先）
- **推荐逻辑（首版）：**
  - RAM < 16GB：推荐 2B
  - 16GB <= RAM < 32GB：推荐 9B
  - RAM >= 32GB：推荐 27B
- **安装交互：**
  - 提供“启用本地 Gemma 4 安装”总开关
  - 开启后显示模型清单（多选）
  - 至少选择一个模型；若用户未选，默认回填推荐模型
- **配置持久化：**
  - `gemma_install_opt_in`（bool）
  - `gemma_model_mask`（bitmask，多选）
- **异常与回退：**
  - 下载失败不阻塞主程序，记录失败项并允许重试
  - 本地模型不可用时自动回退网关模式
- **当前实现状态（V2）：**
  - Work 面板已支持 Gemma 安装总开关、2B/9B/27B 多选、基于内存的推荐模型
  - 向导流程已集成本地安装链路，安装中可见步骤化进度与结果
  - 下载链路已支持三源自动回退与取消中断
  - 待补充：2B 小模型内置包策略、下载断点续传、安装后模型可用性自检

---

### 2.30 用户视角反馈与界面合理性结论（当前版本）

#### 2.30.1 直接问题（用户视角）

- 模式切换已可用，但语音/工作的“可执行能力”还弱于聊天
- 附件发送已可用，但仍缺上传队列、失败重试、批量反馈
- 图片预览有基础能力，仍需统一解码与错误占位策略
- 远程互控属高风险场景，必须默认开启强安全闸门

#### 2.30.2 界面设计结论

- **结论：基础结构合理**（左状态 + 右主工作区 + 顶部模式切换）
- **结论：产品化程度需继续提升**（语音/工作模式需补关键操作入口）
- **结论：交互逻辑总体通顺**，但高风险动作确认与失败反馈需更强提示


## 附录：需求讨论草稿

> 以下内容为产品方向讨论记录，后续整理为正式需求。

### A.1 产品定位

AnyClaw 的核心价值：

1. **简单** — 帮助客户低门槛使用 OpenClaw，无需深入理解底层架构
2. **安全** — 客户数据不离开本地，不依赖第三方平台
3. **长期稳定** — 本地运行，不随平台策略变化而失效

### A.2 知识库体系

- 客户可借助 AnyClaw 在本地建立自己的知识库体系
- 脱离集体平台，数据自主可控
- 不依赖任何云服务或第三方 SaaS

### A.3 跨平台编译要求

- **目标**：同时支持 Linux 和 Windows 运行环境
- **Linux 编译**：需有完整的 Linux 编译环境，产出 Linux 原生可执行文件
- **Windows 编译**：通过 MinGW-w64 交叉编译，产出 Windows x64 可执行文件 + SDL2.dll
- **构建产物**：每次构建须同时产出两个平台的可用产物
- **CI/CD**：后续考虑 GitHub Actions 自动化双平台构建

### A.5 图标统一管理（待实施）

- **目标**：消除图标尺寸散落，统一层级定义

| 图标层级 | 尺寸 | 用途 |
|---------|------|------|
| `ICON_TINY` | 16px | 托盘最小、内联小图标 |
| `ICON_SMALL` | 20px | 托盘默认、菜单项 |
| `ICON_MEDIUM` | 24px | 聊天头像(AI)、按钮内图标 |
| `ICON_LARGE` | 32px | 聊天头像(用户)、标题栏LVGL图标 |
| `ICON_XLARGE` | 48px | 任务栏图标、关于页大图标 |

- **当前混乱**：
  - 托盘：16/20/32/48 四种尺寸共存
  - 聊天头像：用户 32px / AI 24px，代码各写各的
  - 窗口图标：main.cpp LoadImageW 同时加载 48/32/16
- **实施方案**：新建 `src/icon_config.h`，各场景默认尺寸用宏引用

### A.9 离线授权（已写入 2.8.11，待实施）

- 硬件指纹：MAC + 磁盘序列号 + 主板 UUID → SHA-256 → 8 段机器码
- 授权流程：机器码 → 开发者生成 HMAC-SHA256 Key → 本地验证
- 详见 2.8.11 节

### A.10 OpenClaw Gateway 与 Session 架构分析

> 2026-04-05 讨论记录：AnyClaw 如何感知 OpenClaw 实时状态。

#### A.10.1 Gateway 是什么

Gateway 是 OpenClaw 的守护进程（`openclaw gateway`），跑在本机 `localhost:18789`，是整个系统的"总机"：

- **收消息**：从各个渠道（WebChat、Discord、Telegram、CLI 等）收用户输入
- **调度 Agent**：把消息路由给对应的 Session，管理 Session 生命周期
- **暴露 API**：给外部客户端提供 HTTP/WebSocket 接口查状态、发消息
- **管理心跳**：定时 Poll Agent 看它还活着没

#### A.10.1b Gateway 心跳机制

Gateway 定时向 Session 发送心跳 poll（默认提示词），Agent 收到后检查周期性任务并回复。这是 Gateway 主动感知 Agent 存活的方式。

**默认心跳提示词：**
> Read HEARTBEAT.md if it exists (workspace context). Follow it strictly. Do not infer or repeat old tasks from prior chats. If nothing needs attention, reply HEARTBEAT_OK.

**Agent 心跳行为：**
- 收到心跳后检查 `HEARTBEAT.md` 中定义的周期性任务
- 可以利用心跳做有用的后台工作：整理记忆文件、检查项目状态、提交代码、更新 MEMORY.md
- 没有需要处理的事情时回复 `HEARTBEAT_OK`

**主动检查内容（Agent 可自行配置）：**

| 检查类型 | 场景 |
|----------|------|
| 邮件 | 未读紧急邮件 |
| 日历 | 未来 24-48h 事件 |
| 提及 | Twitter/社交通知 |
| 天气 | 用户可能外出时 |

**何时主动联系用户 vs 保持安静：**

| 主动联系 | 保持安静 (HEARTBEAT_OK) |
|----------|------------------------|
| 重要邮件到达 | 深夜 (23:00-08:00) 除非紧急 |
| 日历事件临近 (<2h) | 用户明显忙碌 |
| 发现有趣的内容 | 上次检查 <30 分钟前 |
| 超过 8h 没说话 | 没有新情况 |

**心跳中的记忆维护（周期性，每几天一次）：**
1. 读取近期 `memory/YYYY-MM-DD.md` 文件
2. 提取重要事件、教训、洞察
3. 更新 `MEMORY.md`（去重、提炼）
4. 清理 MEMORY.md 中过时的信息

#### A.10.1c Heartbeat vs Cron 对比

| 维度 | Heartbeat | Cron |
|------|-----------|------|
| 适用场景 | 多项检查可合并（邮件+日历+通知一轮搞定） | 精确时间要求（每周一 9:00 准时执行） |
| 上下文 | 携带近期对话上下文 | 隔离会话，不依赖主会话历史 |
| 时间精度 | 允许 ~30min 漂移 | 精确到秒 |
| 模型 | 与主会话共用模型和思考级别 | 可指定不同模型/思考级别 |
| 输出 | 需要时主动联系用户 | 直接投递到指定 channel |
| API 调用 | 可减少（合并检查项） | 独立调用 |

**最佳实践：** 将多项周期性检查合并到 `HEARTBEAT.md`（减少 API 调用），精确调度用 Cron。

#### A.10.1d Session-Channel 映射

Gateway 维护 Session 与 Channel 的映射关系，AnyClaw 通过 Gateway API 查询：

```
Gateway
├── Session: webchat:main       → Agent: main（主会话，与用户交互）
├── Session: discord:general    → Agent: main
├── Session: telegram:xxx       → Agent: main
├── Session: cron:main:abc123   → Agent: main（定时任务）
├── Session: subagent:xyz789    → Agent: main（子代理，隔离会话）
└── Session: webchat:data-bot   → Agent: data-bot（不同 Agent）
```

每个 Channel（WebChat、Discord、Telegram 等）对应独立的 Session 实例，但可以共享同一个 Agent。AnyClaw 作为桌面管理器需要感知 Gateway 上各 Session 的状态（忙/空闲），但不需要改 Gateway 代码。

#### A.10.1e 物理通信流程

从用户视角看完整通信链路：

```
用户设备（浏览器/App）
    │
    ↓ HTTP/WebSocket
Gateway（本机 daemon，端口 18789）
    │
    ↓ 路由到 Session
Agent（LLM 推理 + 工具调用）
    │
    ↓ 结果返回
Gateway
    │
    ↓ 推送到 Channel
用户设备
```

**各层职责：**

| 层 | 物理位置 | 职责 |
|---|---------|------|
| 用户终端 | 用户设备（手机/电脑） | 发消息、看结果 |
| Channel | 平台（WebChat/Discord/Telegram） | 消息传输通道 |
| Gateway | 服务器本机 daemon | 收消息→路由→调度→管理 Session 生命周期 |
| Session | 服务器内存 | 一次对话实例，携带上下文 |
| Agent | LLM 服务 | AI 推理、工具调用、决策 |

**Gateway 的角色：**
- 收消息：从各个 Channel 接收用户输入
- 调度 Agent：将消息路由到对应的 Session
- 暴露 API：给外部客户端提供 HTTP/WebSocket 接口
- 管理心跳：定时 Poll Agent 确认存活

**Heartbeat vs Cron 在物理层面的区别：**

| 维度 | Heartbeat | Cron |
|------|-----------|------|
| 谁触发 | Gateway 按配置间隔 poll | Gateway 按 cron 表达式触发 |
| 跑在哪 | 主会话内（共享上下文） | 独立隔离会话（干净上下文） |
| 上下文 | 携带当前对话历史 | 无历史，从零开始 |
| 时间精度 | 允许漂移（~30min） | 精确到秒 |
| Agent 控制 | 收到后自行决定做什么 | task 在创建时指定 |
| 用在哪 | 批量检查（邮件+日历+通知一轮搞定） | 精确定时（每周一9点执行） |

#### A.10.1f Gateway 故障场景

Gateway 是系统的总开关，挂掉后整个系统停摆。

**Gateway 挂掉时丢失的能力：**

| 能力 | 状态 | 说明 |
|------|------|------|
| 心跳 | ❌ 停止 | Gateway 不再 poll，Agent 不会被唤醒 |
| Cron 定时触发 | ❌ 停止 | Gateway 不再按调度触发任务 |
| 所有 Session | ❌ 断开 | 包括主会话、子代理、cron 隔离会话 |
| 消息收发 | ❌ 中断 | 用户无法与 Agent 通信 |

**Gateway 挂掉后保留的数据：**

| 数据 | 状态 | 说明 |
|------|------|------|
| Cron 任务配置 | ✅ 持久化 | 配置保存在 Gateway 配置文件中，重启后恢复 |
| 文件系统 | ✅ 不受影响 | 工作区文件、memory 文件完好 |
| Agent 状态 | ✅ 无状态 | Agent 本身无记忆，不影响 |

**Gateway 恢复后自动恢复的能力：**

| 能力 | 行为 |
|------|------|
| 心跳 | 自动恢复，Gateway 重启后继续按间隔 poll |
| Cron | 自动恢复，错过的时间段不补发（skip-missed） |
| Session | 需用户重新连接（webchat 刷新页面等） |

**AnyClaw 的作用：**
- 健康监控线程检测 Gateway 存活（进程 + HTTP 端口）
- 检测到挂掉后自动执行 `openclaw gateway start` 拉起
- 仅重试 1 次，失败后托盘变红不再自动重启
- 详见 §2.3.3 健康监控

#### A.10.2 正确的架构模型

```
用户 → 渠道(Channel) → Gateway → Session(→ Agent)
```

关键区分：
- **Agent** = AI 大脑（定义 system prompt、工具、模型）
- **Session** = 一次对话（携带上下文、历史消息）
- 同一个 Agent 可以同时跑多个 Session
- Gateway 路由消息到 Session，Session 决定用哪个 Agent

示例：

```
Gateway
├── Session: webchat:main       → Agent: main
├── Session: discord:general    → Agent: main
├── Session: telegram:xxx       → Agent: main
├── Session: cron:main:abc123   → Agent: main
├── Session: webchat:data-bot   → Agent: data-bot
└── Session: cron:data-bot:xyz  → Agent: data-bot
```

**AnyClaw 关心的是：Gateway 上有没有 Session 正在忙？**
不是 "Agent 有没有在忙"，因为同一个 Agent 的不同 Session 可能一个忙一个闲。

#### A.10.3 WebChat vs AnyClaw 状态感知机制对比

| 维度 | WebChat | AnyClaw |
|------|---------|---------|
| 通信方式 | Gateway WebSocket/HTTP API 中转 | 本地直连（进程检测 + HTTP） |
| 实时性 | 依赖 Gateway 存活 | 不依赖 Gateway，直接本地检测 |
| Gateway 挂了 | 瞎了 | 照样能检测到，还能拉起来 |
| 远程能力 | 可以（跨网络） | 仅限本机 |
| 耦合度 | 解耦（统一入口） | 强耦合本机 |

**结论：两者互补，不是竞争关系。** 更合理的架构是：
- 健康检测走 AnyClaw 的直连方式（本地 Agent 自己诊断自己）
- 状态上报走 Gateway API（统一出口给所有客户端）
- 如果打通（AnyClaw 检测 → 上报 Gateway → WebChat 也能看到），两全其美

#### A.10.4 AnyClaw 感知 Agent 状态的可行方案

**不需要改 Gateway，全部在 AnyClaw 侧做。**

**方式 A：调 CLI（最简单）**

```
AnyClaw → exec("openclaw sessions --json") → 解析结果
AnyClaw → exec("openclaw status --json")    → 解析结果
```

- 优点：一行命令搞定，不需要额外依赖
- 缺点：每次调 CLI 都要启动一个进程，有延迟；没法实时推送
- 判断忙/空闲：`openclaw sessions --json --active 2`，有返回 = 有东西在忙

**方式 B：连 WebSocket（更实时）**

```
AnyClaw → ws://localhost:18789 → Gateway
         ← 心跳/状态推送 ←
         → sessions.reset (中断)
         → agent.message (发任务)
```

- 优点：实时推送，低延迟，能做打断
- 缺点：C++ 里要实现 WebSocket 客户端 + Gateway 协议解析

**现有可用的 Gateway/CLI 能力：**

| 能力 | 现有方式 | 需要改 Gateway？ |
|------|---------|----------------|
| 查 session 列表 | `openclaw sessions --json` | 不用 |
| 查 agent 是否在线 | `openclaw status` | 不用 |
| 中断当前任务 | Gateway WebSocket: `sessions.reset` | 不用 |
| 发送消息 | Gateway WebSocket | 不用 |

#### A.10.5 忙/空闲检测 + 打断 + 任务排队设计

**核心能力：感知状态、打断能力、任务调度。**

**1. 感知：Agent 是忙还是空闲**

```json
// 通过 CLI 查询最近活跃 session
openclaw sessions --json --active 2
→ 有结果 = 有东西在忙（🔴）
→ 无结果 = 全部空闲（🟢）
```

UI 反映：
- 🟢 空闲、🔴 忙碌、🟡 思考中
- 聊天框上方显示状态指示器

**2. 打断：忙的时候可以中断当前任务**

- AnyClaw 右侧面板加 Stop 按钮（忙时高亮，空闲时灰）
- 点击后通过 Gateway WebSocket 发 `sessions.reset`

**3. 派任务：空闲时自动派发，忙时排队**

本地任务队列：

```cpp
struct PendingTask {
    std::string message;
    int priority;
    bool waiting;
};
std::queue<PendingTask> task_queue;
```

流程：
1. 用户输入消息 → 检测 agent 状态
2. 空闲 → 直接发消息
3. 忙碌 → 入队，UI 显示 "排队中 (#2)"
4. 轮询检测到 agent 变空闲 → 自动派队列第一条
5. 用户可拖拽调整队列顺序，或取消排队任务

#### A.10.6 实施路径

**最小可行版本（MVP）：**

1. AnyClaw 调 `openclaw sessions --json --active 5`，轮询（5秒一次）
2. 聊天框上方显示 "🟢 空闲" 或 "🔴 处理中"
3. 忙时发送按钮变 "排队" 模式，消息入队
4. 空闲时自动发送

不涉及 Gateway 改动，AnyClaw 侧约 200 行代码。

**完整版本：**

| 层 | 改动 | 优先级 |
|---|---|---|
| AnyClaw `health.cpp` | 新增 session 轮询线程 | 高 |
| AnyClaw UI | 状态指示器（灯 + 文字） | 高 |
| AnyClaw UI | Stop 按钮逻辑 | 高 |
| AnyClaw Chat | 发送前状态检查 + 排队 | 中 |
| AnyClaw WS | WebSocket 客户端连 Gateway | 低（MVP 不需要） |

### A.12 第三方凭据管理（讨论中）

> **决策（2026-04-06）**：暂不实施。核心功能（聊天、任务管理、会话管理）稳定后再评估。
> **优先级：** P2 以下
> **状态：** ⏸️ 讨论草稿

#### 需求背景

AnyClaw 作为 AI Agent 桌面管理器，用户可能需要授权 Agent 操作需要登录的网站（查邮件、社交通知等）。需要一种安全的凭据提供机制。

#### 方案对比

| 方案 | 安全性 | 复杂度 | 是否存储密码 |
|------|--------|--------|-------------|
| **A. 内置密码管理** | ❌ 低 | 高 | 是 — 不推荐 |
| **B. OAuth Token 存储** | ✅ 高 | 中 | 否 — 只存 token |
| **C. 对接已有密码管理器** | ✅ 高 | 低 | 否 — 读取第三方 |
| **D. 浏览器 Cookie 代理** | ⚠️ 中 | 中 | 否 — 复用会话 |

**推荐方案 C**（如需实施）：对接 Bitwarden CLI

#### Bitwarden 对接方案（如采用）

| 项目 | 说明 |
|------|------|
| 工具 | Bitwarden CLI (`bw`) |
| 费用 | 个人免费，开源 |
| 命令 | `bw get item example.com` → 返回 JSON（含 username/password/uri） |
| 集成方式 | AnyClaw 调 `bw` CLI 子进程，解密后注入 HTTP 客户端 |
| 安全模型 | 用户主密码由用户保管，AnyClaw 不存储；session key 仅内存中 |
| 前置条件 | 用户已安装 Bitwarden CLI 并完成 `bw login` |

#### 核心原则

- **AnyClaw 不应该知道或存储用户密码明文**
- 凭据提供应由用户主动授权，非自动注入
- 仅支持有成熟 CLI/API 的密码管理器，不做自研

#### 待定问题

1. 是否需要 UI 面板管理已授权网站列表？
2. Token/Session 过期后的自动刷新策略？
3. 是否支持多账号（同一网站多个凭据）？

---

### 2.31 本轮新增需求落地（2026-04-09）

#### 2.31.1 向导流程修复（P0）
- 向导增加全程可见“退出向导”按钮，并接入统一退出逻辑（安装中触发取消）。
- 检测页增加向导内安装进度区（任务/步骤/进度条/结果），不再仅依赖左侧全局状态栏。
- 检测页“安装入口”调整为 `OpenClaw missing` 即可触发，支持 `Auto Install`（自动串联 Node.js + OpenClaw）。
- 修复 Back 返回后 Next 失效问题：Next 按钮状态由检测结果实时恢复/禁用，不保留旧状态。
- Gemma 增加显式 `Skip` 操作，跳过不影响 AnyClaw 正常使用。

#### 2.31.2 局域网群聊（LAN-CHAT-01，MVP）
- 新增局域网中继模式：同一 LAN 内可由一台 AnyClaw 启动 Host，其它客户端连接。
- 支持能力：全局群聊、私聊、建群、加群、群消息发送。
- Work 模式新增 LAN Chat 面板（Host/Connect、目标用户、群组、消息输入、事件流）。
- 协议采用轻量文本帧（cmd/from/to/group/payload）并通过 TCP 传输。

#### 2.31.3 FTP 上传下载（FTP-01，基础版）
- 新增单文件 FTP 上传与下载（基础版），支持进度展示与取消。
- Work 模式新增 FTP 面板：host/port/user/pass/remote/local 配置，上传/下载/取消按钮。
- 传输层采用 WinINet，上传按文件总大小给出真实进度，下载提供分段进度。

#### 2.31.4 当前实现边界
- LAN 当前为中继 MVP，不含发现协议（mDNS/广播）与离线消息持久化。
- FTP 当前为基础 FTP，不含 FTPS 与目录递归同步。

### 2.32 跨平台构建与产物规范（BUILD-TOOLCHAIN-01）

#### 2.32.1 目录与命名规范（已落地）
- 工具按平台分层：
  - `tools/common/`：通用打包脚本
  - `tools/windows/`：Windows 环境检测/依赖准备/一键编译打包
  - `tools/linux/`：Linux 环境检测/Linux 原生构建/Linux 交叉构建 Windows
- 产物按运行平台落盘到固定目录：
  - `artifacts/windows-native/`：Windows 运行包
  - `artifacts/linux-native/`：Linux 运行包
  - `artifacts/linux-to-windows/`：Linux 交叉生成的 Windows 运行包

#### 2.32.2 一键环境与构建入口（已落地）
- Windows
  - 环境检查：`tools/windows/setup-env.bat`
  - 依赖缓存：`tools/windows/fetch-runtime-deps.bat`（SDL2 缓存到 `tools/cache/`，避免每次临时下载）
  - 一键编译打包：`tools/windows/build-package.bat`
- Linux
  - 环境检查：`tools/linux/setup-env.sh`
  - Linux -> Windows：`tools/linux/build-win.sh`
  - Linux -> Linux：`tools/linux/build-linux.sh`

#### 2.32.3 UI 交互修复（本轮）
- 主界面右上角改为：模式切换（Chat/Voice/Work）→ 设备 → 设置 → 最小化/最大化/关闭。
- Settings 页面恢复显式关闭按钮。
- API Key 输入框高度统一为单行输入高度。
- 长任务防重复点击：FTP 传输进行中禁用重复操作按钮，仅保留取消。
- 恢复聊天上传按钮。

#### 2.32.4 安装与健康检查容错（本轮）
- 当 `Node/npm/OpenClaw` 缺失时，即使 `wizard_completed=true` 也强制打开向导。
- 向导网络检测超时收敛（降低阻塞体感）。

### 2.33 模式切换与输入区交互优化（UI-MODE-20260409）

- 顶栏模式改为单按钮切换 `Chat <-> Work`（减少顶部拥挤与误触）。
- `Voice` 入口移至聊天输入区，位置在上传按钮左侧，风格与输入区功能按钮一致。
- 移除顶栏“设备”按钮，仅保留“设置”入口，避免功能重复与歧义。
- 长任务按钮防重入策略继续收敛（执行中禁用重复点击，保留必要取消入口）。

### 2.34 KB 上下文接入与健康线程稳定性加固（2026-04-09）

#### 2.34.1 本地知识库接入聊天上下文（KB-CHAT-V1）
- 在发送聊天请求时，按用户输入关键词从本地 KB 检索 Top-N 文档片段，并自动拼接为请求上下文。
- UI 展示仍保持用户原始输入，KB 片段仅进入发送 payload，不污染聊天气泡显示。
- 新增 KB 索引文件持久化（`%APPDATA%\\AnyClaw_LVGL\\kb_index.txt`），重启后自动加载已登记来源。
- 目标：降低“知识已导入但聊天未利用”的割裂感，形成最小闭环 RAG 体验（V1）。

#### 2.34.2 健康检查线程稳定性加固（STABILITY-HEALTH-V1）
- 健康线程由长时间整段 `Sleep(interval)` 改为可中断分片等待，提升停止/退出响应速度。
- 当 `node.exe` 不存在时不再执行 HTTP 健康探测，减少无效网络调用和阻塞抖动。
- 目标：降低“退出卡住/周期性卡顿”概率，减少日志噪音并稳定后台轮询行为。

### 2.35 下一轮落地（稳定性/FTP/KB/REMOTE，2026-04-09）

#### 2.35.1 稳定性专项（STABILITY-V2）
- 聊天流式期间统一防重入：`Chat` 与 `Work` 发送入口均在流式进行中拒绝新提交。
- 发送区按钮与流状态联动：`Send/Upload/Voice` 在流式期间禁用，流结束自动恢复，避免并发交互拥塞。
- 健康线程停止路径增强：停止等待超时将记录告警并清理回调，降低退出阶段悬空回调风险。

#### 2.35.2 FTP 断点续传（下载侧优先，FTP-RESUME-V1）
- 下载任务新增续传能力：检测本地已下载大小后尝试 `REST`，失败时回退为跳过已下载字节继续拉取。
- 当本地文件已完成（本地大小 >= 远端大小）时直接判定成功，避免重复下载。
- Work 面板新增 `Retry Last` 按钮：失败后可一键复用上次参数重试，减少重复输入成本。

#### 2.35.3 KB V2（检索打分与上下文裁剪）
- 检索排序升级为评分模型：频次、首次命中位置、文件名命中共同参与排序，优先返回高相关文档。
- 上下文构建升级为“命中片段截取”而非固定前缀截断，减少无关噪声。
- 新增片段去重与总长度预算控制，降低重复上下文对回复质量与token预算的影响。

#### 2.35.4 REMOTE 真通道前置加固（REMOTE-HARDEN-V2）
- 协议层新增状态迁移守卫：仅允许合法迁移（如 `pending_accept -> connected`），非法迁移直接拒绝。
- UI 侧接入失败原因回显（如 `invalid_transition`），提升故障可观测性与调试效率。
- 会话鉴权链路继续强化：随机令牌、一次性轮换、过期清理共同降低重放与陈旧会话风险。

### 2.36 PRD 对照补齐与缺陷修复（2026-04-09）

#### 2.36.1 Security 状态面板动态化（SEC-UI-01）
- General Tab 的 Security 面板从静态文案升级为动态检测结果：
  - API Key 状态（按当前模型 provider 推断）
  - Gateway 端口状态
  - 配置目录可写性
- 安全灯态改为 `Good/Warning/Risk` 实时反映，不再固定显示绿色。

#### 2.36.2 Task List 调度可视化补强（TM-01）
- Task List 增加 `Scheduler` 汇总项，显示：
  - Active sessions
  - Active cron sessions
- 用于补足 PRD 中“会话/调度状态可视化”的最小闭环展示。

#### 2.36.3 本轮 Bug 扫描与修复结果
- 修复 Security 行文案覆盖问题（文本颜色被重复赋值导致状态色失效）。
- 修复发送入口并发问题：Chat/Work 在流式回复期间统一禁止重入发送，避免状态错乱。

### 2.37 本轮继续落地（REMOTE / GEMMA / MODE02，2026-04-09）

#### 2.37.1 REMOTE 请求超时自动重试（REMOTE-01 补强）
- 在 `pending_accept` 超时时增加自动重试（最多 2 次），并在失败后回落到 `idle`。
- 请求链路统一到 `remote_begin_request()`，减少状态分叉。
- 安全闸门（guard）仍为前置条件，不会绕过。
- 新增 `remote_tcp_channel` 通道骨架：TCP 心跳、断线重连、失败回退与状态上报（MVP 控制信令通道）。

#### 2.37.2 Gemma 下载续传与安装后校验（GEMMA-01 补强）
- Gemma 多源下载支持断点续传：目标文件已存在时使用 `curl -C -`。
- 下载阶段完成后新增文件完整性校验，所有勾选模型校验通过才判定安装成功。
- 下载失败自动清理目标残留文件，避免半包污染后续安装判断。

#### 2.37.3 Voice -> Work 路径补全（MODE-02 补强）
- Voice 区新增 `Send To Work`，将转写内容直接进入 Work Prompt 并自动切换模式。
- 保持 `Send To Chat` 原链路，实现语音输入到 Chat/Work 的双通道闭环。

#### 2.37.4 运行期日志噪声治理（稳定性补充）
- 对 WinHTTP 重复错误（如 `12029`）引入节流输出：
  - 首次保留 Error 级别；
  - 1 分钟内相同错误降级为重复告警，避免日志刷屏影响排障。
