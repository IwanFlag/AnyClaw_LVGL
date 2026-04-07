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
    ├── AnyClaw_LVGL_v2.0_PRD.md        # 产品需求文档（本文档）
    ├── AnyClaw_LVGL_v2.0_build_guide.md # 编译部署指南
    └── AnyClaw_LVGL_v2.0_task_test.md  # 任务测试记录
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
- **功能编号：** SF-01 **优先级：** P2 **状态：** ❌ 待实现
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
- **功能编号：** MIG-01 **优先级：** P1 **状态：** ❌ 待实现
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

> **功能编号：** WS-01 **优先级：** P0 **状态：** ❌ 待实施

#### 2.12.1 概念定义

**工作区 (Workspace)** 是 OpenClaw Agent 的活动边界。Agent 所有文件操作限定在工作区根目录内。每个工作区是一个独立的配置单元，包含 Agent 人格、记忆、工具配置和项目文件。

**四个目录，各有归属：**

| 目录 | 归属 | 由谁决定 | 性质 | 当前代码状态 |
|------|------|---------|------|-------------|
| AnyClaw 安装目录 | AnyClaw 程序本体 (exe + dll + assets) | 用户选择 | 只读 | ❌ 用户无选择，随便放 |
| AnyClaw 数据目录 | config.json、日志、缓存 | 系统固定 `%APPDATA%\AnyClaw_LVGL\` | 可读写 | ✅ 已实现（`paths.h`） |
| OpenClaw 安装目录 | OpenClaw 运行时 (npm 全局) | 自动检测 | 只读 | ⚠️ 硬编码 `%USERPROFILE%\.openclaw\` |
| OpenClaw 工作区 | Agent 活动范围 (AGENTS.md、MEMORY.md、项目) | 用户选择 | 可读写 | ❌ 未实现，硬编码 |

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
- 自动检测：Node.js 是否已安装、版本是否 ≥ 18
- 自动执行 `npm install -g openclaw`
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

#### 2.12.7 工作区锁

防止多个 AnyClaw 实例同时操作同一工作区：

- 启动时创建 `.openclaw.lock`，内容包含 PID、hostname、启动时间
- 退出时删除 lock 文件
- 启动检测到已有 lock → 判断 PID 是否存活（同 hostname）→ 存活则拒绝，死亡则清理
- 跨机器不互锁（hostname 不同）

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

> **功能编号：** PERM-01 **优先级：** P0 **状态：** ❌ 待实施

#### 2.13.1 设计目标

OpenClaw Agent 能执行高权限操作（读写文件、执行命令、访问网络、访问设备）。必须在安装时由用户划定边界，并在运行时强制执行。

**当前代码状态：无任何权限控制。** Agent 可以自由访问整个文件系统、执行任意命令、访问所有设备。本章节定义从零开始的权限体系。

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

#### 2.13.7 运行时强制执行

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

**当前代码缺口：** 现有 `openclaw_mgr.cpp` 中的 `exec_cmd()` 直接调用 `CreateProcessA`，无任何权限校验。需要在工具调用层增加拦截点。

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

#### 2.13.11 审计日志

所有权限决策记录到 `%APPDATA%\AnyClaw_LVGL\logs\permissions.log`：

```
[2026-04-06 15:47:01] ALLOW  fs_read  D:\MyAIWorkspace\AGENTS.md
[2026-04-06 15:47:02] DENY   fs_read  C:\Windows\system32\hosts
[2026-04-06 15:47:03] ASK    exec     rm -rf /tmp/cache  → 用户允许(单次)
[2026-04-06 15:47:04] ALLOW  net_out  https://api.openai.com/v1
[2026-04-06 15:47:05] DENY   device   USB Kingston F:\ (未授权)
```

AnyClaw Settings 提供"权限历史"面板，用户可查看所有允许、拒绝、询问记录。

#### 2.13.12 Skill 安装与权限交互

用户通过 ClawHub 安装 Skill 时，检查 Skill 所需权限是否与当前配置冲突：

- 权限充足 → 直接安装
- 权限不足 → 提示需要开启哪些权限 → 用户确认后自动更新 permissions.json
- 用户拒绝 → 取消安装

#### 2.13.13 Settings 权限管理面板

AnyClaw Settings 新增 **Permissions** Tab：

- 工作区根目录（修改后触发健康检查）
- 操作权限开关（文件/命令/网络/设备各子项）
- 设备信任表管理（查看/删除已信任设备）
- 资源上限调整
- 权限历史（审计日志查看器）
- 权限导入/导出（随工作区迁移包一起打包）

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
