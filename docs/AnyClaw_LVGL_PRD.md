# AnyClaw LVGL — 产品需求文档 (PRD)

> 版本：v2.0.3 | 最后更新：2026-04-11

---

## 1. 产品概述

### 1.1 产品定位

AnyClaw 是面向**学生**和**职场用户**的 AI 桌面管家。兼具 **AI 编程工作台**与 **AI 管家** 两大能力——让用户既能像 Cursor 一样高效编程，又能像拥有私人管家一样处理日常事务。

三个关键词：
- **简单** — 开箱即用，无需深入理解底层架构
- **安全** — 数据不离开本地，不依赖第三方平台
- **稳定** — 本地运行，不随平台策略变化而失效

**产品公式：**
```
Cursor（AI 编程）+ OpenClaw（AI 管家）= AnyClaw
```

| 模式 | 角色 | 类比 |
|------|------|------|
| Chat 模式 | AI 管家 — 聊天、提醒、查邮件、局域网协作、日常事务 | OpenClaw + 飞书 |
| Work 模式 | AI 编程工作台 — 代码编辑/查看、终端、Git、AI 辅助编程 | Cursor |

### 1.2 技术架构

**当前架构（v2.0）：**

```
AnyClaw (GUI) → OpenClaw Gateway (Agent) → LLM API
```

AnyClaw 是 GUI 壳子，通过 OpenClaw Gateway 与 LLM 交互。所有聊天、工具调用、会话管理经由 Gateway 中转。

**终局目标（v3.0+）：**

```
AnyClaw (Agent 管理器) → LLM API
                     ↘ 外部 Agent（OpenClaw 等）
```

AnyClaw 自身成为 Agent 平台——直接管理 prompt、工具调用、会话上下文，同时也能管理外部 Agent（如 OpenClaw、Hermes 等）。Direct API 模式为架构预留，当前仅支持 Gateway 模式。

---

## 2. 安装与首次启动

### 2.1 运行环境与部署

| 要求 | 说明 |
|------|------|
| 操作系统 | Windows 10/11 (x64) |
| 运行时依赖 | `SDL2.dll`（同目录） |
| 可选依赖 | OpenClaw Gateway（未安装时引导安装） |
| 磁盘空间 | < 20 MB |
| 内存 | < 200 MB（空闲状态） |

**部署方式：** 双文件部署 — `AnyClaw_LVGL.exe` + `SDL2.dll` 放入同一目录，双击运行。计划后续支持单文件部署（Launcher 嵌入 SDL2.dll）。

**跨平台编译：**
- Windows：MSYS2 原生编译，产物到 `artifacts/windows-native/`
- Linux：MinGW-w64 交叉编译，产物到 `artifacts/linux-to-windows/`；原生编译到 `artifacts/linux-native/`
- 构建脚本：`tools/windows/build-package.bat`、`tools/linux/build-win.sh`、`tools/linux/build-linux.sh`

### 2.2 实例互斥
- **功能编号：** INST-01 **优先级：** P0
- 通过 Windows Mutex (`Global\AnyClaw_LVGL_v2_SingleInstance`) 防止重复启动
- 检测到已有实例运行 → 弹出原生 MessageBox 提示 "Already Running"，退出程序
- 进程退出时自动释放 Mutex

### 2.3 启动自检
- **功能编号：** SC-01 **优先级：** P0

| 检查项 | 检测方式 | 自动修复 |
|--------|---------|---------|
| Node.js | `node --version` | 不可修复，弹窗提示安装 |
| npm | `npm --version` | 尝试 `npm cache clean --force` |
| 网络连通性 | HTTP GET google.com / openrouter.ai | 不可修复，提示检查网络 |
| 配置目录 | `%USERPROFILE%\.openclaw` 可写性 | 尝试 `create_directories` 创建 |

- 检查结果写入日志（`[SelfCheck]` 前缀）
- Node.js 缺失 → 显示 LVGL 弹窗阻断启动（`g_startup_error`）
- 当 `Node/npm/OpenClaw` 缺失时，即使 `wizard_completed=true` 也强制打开向导

### 2.4 首次启动向导
- **功能编号：** WIZ-01 **优先级：** P1 **状态：** ✅ 已实现
- 5 步引导（全屏遮罩 + 居中弹窗，深色风格 #1E2530 圆角 12）：
  1. **语言选择** — 中文/English 两个按钮，默认 EN
  2. **OpenClaw 检测** — 自动检测 Node.js/npm/Network/OpenClaw/Gateway，显示状态 LED + 版本号；支持向导内安装（Auto Install / 本地包）与进度显示
  3. **模型 & API 密钥** — 模型 dropdown（由 model_manager 动态加载）+ Provider 动态提示 + API Key 密码模式输入框
  4. **本地模型（可选）** — Gemma 4 本地安装开关 + 2B/9B/27B 勾选，可 Skip
  5. **个人信息 & 确认** — 昵称输入 + 时区 dropdown（26 个时区，默认 UTC+8）+ 配置摘要 + "Get Started" 按钮

**交互细节：**
- 顶部步骤指示器（Step 1/5），底部上一步/下一步按钮
- 首次启动自动弹出（检查 config.json `wizard_completed` 字段）
- 向导增加全程可见"退出向导"按钮，接入统一退出逻辑
- Gemma 增加显式 Skip 操作，跳过不影响正常使用
- **用户画像写入 USER.md**：路径 `%USERPROFILE%\.openclaw\workspace\USER.md`
- **General Tab 底部提供 "Reconfigure Wizard" 按钮**，可随时重新触发
- 默认 AI 交互模式 = 自主模式（不在此步暴露选择）

### 2.5 OpenClaw 安装

**OpenClaw 发现机制：**

1. config.json 中配置的路径
2. PATH 环境变量中的 `openclaw` 命令
3. npm 全局目录（`npm root -g`）
4. 常见安装位置扫描
5. 都找不到 → 引导用户安装或指定路径

**安装支持：**
- Auto Install：自动串联 Node.js + OpenClaw 安装
- 本地包：用户指定已安装路径
- 多源自动切换：下载/安装失败自动回退

### 2.6 工作区初始化

首次启动（无 config.json）时：
- 默认建议工作区路径：`%USERPROFILE%\.openclaw\workspace\`
- 选择工作区模板（通用助手 / 开发者 / 写作者 / 数据分析）
- 自动创建目录结构和初始化文件（详见第 7 章）

---

## 3. 主界面

### 3.1 窗口与布局

#### 3.1.1 主窗口
- **功能编号：** WIN-01 **优先级：** P0
- SDL + LVGL 官方驱动，UI 布局设计尺寸 1450×800（逻辑像素），实际窗口尺寸基于屏幕分辨率，默认屏幕 75%×75%，上限为屏幕分辨率减去边距
- 窗口关闭按钮 → 最小化到托盘（非退出）：`SDL_QUIT` 事件触发 `tray_show_window(false)`
- 最后一次关闭时的位置，重启后恢复（从 config.json 读取 window_x/window_y）
- 窗口大小自适应 DPI 缩放（启动时检测，不可运行时修改）
- 窗口最小尺寸限制 800x500

#### 3.1.2 拖拽窗口
- **功能编号：** DD-01 **优先级：** P2
- 标题栏按住拖拽移动窗口

#### 3.1.3 窗口交互优化
- **功能编号：** WI-01 **优先级：** P2
- 双击标题栏最大化/还原（Win32 `WM_NCLBUTTONDBLCLK` 子类化 + SDL 层 400ms 双击检测 fallback）
- 分隔条拖拽调整左右面板比例：
  - `PRESSED` → 记录起始 x 坐标，高亮分隔条颜色，切换鼠标为 resize 光标
  - `PRESSING` → 计算 dx 偏移，累加到 `LEFT_PANEL_W`，调用 `relayout_panels()` 实时刷新
  - `RELEASED` → 重置分隔条颜色和光标
  - `relayout_panels()` 同步更新左面板所有子控件的 x 坐标和宽度

#### 3.1.4 窗口置顶与锁定
- **功能编号：** WP-01 **优先级：** P2
- 已移除托盘菜单入口，仅保留设置页开关

### 3.2 系统托盘
- **功能编号：** TRAY-01 **优先级：** P0
- 常驻托盘图标，四种状态颜色（白色=空闲/绿色=运行中/红色=异常/黄色=检测中）
- 托盘菜单：
  - 状态行：显示 OpenClaw 状态 + LED 颜色
  - Start/Stop Gateway、Restart OpenClaw、Refresh Status
  - Boot Start 开关、Open Settings、About、Exit
- 双击托盘图标：显示主窗口
- 退出确认：如 `is_exit_confirmation_enabled()` 为 true → 弹出 LVGL 模态确认弹窗
- 托盘气泡通知：`tray_balloon(title, message, timeout_ms)`

### 3.3 品牌与图标

#### 3.3.1 图标系统
- **功能编号：** ICON-01 **优先级：** P1

| 图标层级 | 尺寸 | 用途 |
|---------|------|------|
| ICON_TINY | 16px | 托盘最小、内联小图标 |
| ICON_SMALL | 20px | 托盘默认、菜单项 |
| ICON_MEDIUM | 24px | 聊天头像(AI)、按钮内图标 |
| ICON_LARGE | 32px | 聊天头像(用户)、标题栏 LVGL 图标 |
| ICON_XLARGE | 48px | 任务栏图标、关于页大图标 |

- LVGL 内置图标：LV_SYMBOL_PLAY/STOP/REFRESH/SETTINGS/FILE/HOME/EDIT/CLOSE
- 大蒜角色图标（用户头像、标题栏、关于页）
- AI 头像多方案（lobster 系列 + ai 系列，24/32/48 三尺寸）
- 托盘图标：预绘制合体 PNG（大蒜+LED），灰/绿/黄/红 × 16/20/32/48

#### 3.3.2 品牌形象
- **功能编号：** BR-01 **优先级：** P1
- 欢迎语：龙虾要吃蒜蓉的 🧄🦞 — 你的AI助手已就位！
- 关于对话框：品牌信息 / Tech Stack

### 3.4 国际化 (i18n)
- **功能编号：** I18N-01 **优先级：** P1
- 支持 **CN / EN / KR / JP** 切换，界面始终显示单一语言
- 首次启动自动检测系统语言（Windows API），不支持则默认英文
- 切换后立即生效，无需重启
- 所有 UI 文本通过 `I18n` 结构体定义，`tr()` 函数根据当前语言返回对应文本
- 配置持久化到 config.json（language 字段）

### 3.5 贴边大蒜头（Edge Snap Garlic Dock）
- **功能编号：** SNAP-01 **优先级：** P2 **状态：** ✅ 已实现
- 拖拽窗口到屏幕边缘（左/右/上/下）→ 隐藏主窗口 → 屏幕边缘显示浮动大蒜头
- 大蒜头规格：80×110 px，WS_EX_LAYERED 透明背景，始终置顶
- 悬停 → 葱头正弦波 ±12° 摇摆（PlgBlt 仿射变换，1.5Hz）
- 点击 → 恢复主窗口到原始位置
- WM_ACTIVATE 自动恢复（Alt+Tab/任务栏点击）

### 3.6 图标系统升级
- **功能编号：** ICON-02 **优先级：** P2 **状态：** ✅ 已实现
- 托盘图标尺寸升级到 96px（LANCZOS 上采样）
- 窗口标题栏图标升级到 96/64/32（WM_SETICON + SetClassLongPtr）

---

## 4. 聊天

### 4.1 聊天架构
- **功能编号：** CHAT-01 **优先级：** P0
- 聊天走 OpenClaw Gateway `/v1/chat/completions`（OpenAI 兼容），不直连 LLM API
- **通信流程：** AnyClaw → `POST http://127.0.0.1:18789/v1/chat/completions`（带 Gateway auth token）→ Gateway → Agent → LLM
- Gateway token 自动从 `openclaw.json` 的 `gateway.auth.token` 读取
- model 固定为 `openclaw:main`，由 Gateway 路由到配置的 Agent

### 4.2 输入框
- **功能编号：** CI-01 **优先级：** P0 **状态：** ✅ 已实现
- 支持多行输入（Shift+Enter 换行），Ctrl+V 粘贴
- 输入框自动增长：监听 `LV_EVENT_VALUE_CHANGED`，追踪 `g_input_line_count`，行数变化时调整高度
- 默认高度 96px（3 行），最小高度 96px
- 高度变化时保存/恢复 `chat_cont` 的 `scroll_y`，防止布局跳动

### 4.3 消息气泡与流式渲染
- **功能编号：** MB-01 **优先级：** P0
- 显示格式：`[HH:MM:SS] [角色] 内容`
- 消息角色区分：用户消息靠右蓝色气泡，AI 回复靠左灰色气泡
- 气泡宽度自适应：短消息贴合文字宽度（`lv_text_get_size` 测量），长消息 70% 容器宽度换行
- QQ 风格布局：消息自动上下堆叠（`row_wrap` + `FLEX_ALIGN_START/END`）
- AI 回复流式显示（打字机效果）：
  - `stream_timer_cb` 每次追加 3 个字符到 `g_stream_buffer`
  - 调用 `render_markdown_to_label` 实时渲染 Markdown 格式
  - 流式过程中 `chat_scroll_to_bottom()` 软滚（40px 容差，用户回看历史时不强制拉回）
- 对话历史滚动：
  - 新消息发送 → `chat_force_scroll_bottom()` 强制滚到底
  - 流式输出中 → 软滚
  - 手动滚动 → 滚轮仅在 `chat_cont` 和 `chat_input` 上生效

### 4.4 Markdown 渲染
- **功能编号：** MD-01 **优先级：** P1
- 代码块、加粗、列表等基础格式
- CJK 逐字换行（LV_TEXT_FLAG_BREAK_ALL）

### 4.5 消息操作
- **功能编号：** MO-01 **优先级：** P1 **状态：** ✅ 已实现
- 消息文字可选中（鼠标拖拽选区）—— `make_label_selectable(lbl)` 为 label 添加 CLICKABLE 标志 + 事件回调
- 选中后 Ctrl+C 复制到系统剪贴板（UTF-8 → CF_UNICODETEXT）
- 未选中时 Ctrl+C 复制整条消息内容
- 剪贴板 Unicode 支持：`clipboard_copy_to_win()` / `clipboard_paste_from_win()` 支持中文
- 消息时间戳显示

### 4.6 多模态输入与文件上传
- **功能编号：** MM-01 **优先级：** P0 **状态：** 🔧 实施中
- 上传按钮 + 弹出菜单（File / Dir）
- Win32 文件选择对话框：`GetOpenFileNameA`（文件）、`SHBrowseForFolderA`（目录）
- 聊天气泡中可视化显示附件卡片（类型/名称/大小/缩略图）
- 点击附件可直接打开（目录打开资源管理器，文件按系统默认程序）
- **功能编号：** SHARE-01 — 文件/图片/目录发送与可点击打开
- 路径必须通过权限系统校验（工作区边界与外部授权）

### 4.7 聊天历史持久化
- **功能编号：** CH-01 **优先级：** P1 **状态：** ✅ 已实现
- 最近 50 条消息持久化，重启后可查看
- 聊天历史清除功能

### 4.8 搜索
- **功能编号：** SF-01 **优先级：** P2 **状态：** ✅ 已实现
- 聊天记录搜索

### 4.9 SSE 流式超时保护
- **功能编号：** STREAM-01 **优先级：** P0 **状态：** ✅ 已实现
- SSE 流式请求添加超时看门狗：30 秒无新数据 或 总耗时 > 45 秒 → 强制结束
- 防止 Gateway 连接阻塞导致 UI 卡死
- 超时时显示"⚠️ AI 回复超时，请重试。"

### 4.10 会话管理
- **功能编号：** SM-01 **优先级：** P1 **状态：** ✅ 已实现
- 活跃会话列表查看（渠道/Agent/时长）
- 单个会话终止（Abort 按钮）
- 所有会话重置

---

## 5. 模型与 API 配置

### 5.1 云端模型管理
- **功能编号：** MODEL-01 **优先级：** P0 **状态：** ✅ 已实现
- **架构：** AnyClaw 是 OpenClaw 的 GUI 壳子，聊天走 Gateway `/v1/chat/completions`，模型配置同步到 `openclaw.json`
- **当前模型显示：** 绿色高亮从 Gateway 读取的实际配置模型
- **模型选择：** 下拉列表（lv_dropdown），按能力排序（405B → 轻量 → 付费）
- **添加自定义模型：** 下拉框右侧 "+" 按钮 → 弹出模态对话框 → 输入模型名称 → 确认后自动添加到列表并选中
  - 自定义模型保存到 `assets/custom_add_models.json`，持久化跨重启
- **免费模型：** OpenRouter 上 25 个 `:free` 模型，全部国内直连可达（实测 900-3500ms）

### 5.2 Provider 与 API Key
- **功能编号：** PROVIDER-01 **优先级：** P0 **状态：** ✅ 已实现
- **Provider 检测：** 下拉切换时自动检测 provider（openrouter/xiaomi），动态显示 "Provider: xxx → API Key: xxx"
- **API Key 联动：** 切换模型时自动从 Gateway 读取对应 provider 的 API Key 并填入输入框
- **所有 OpenRouter 模型共用一个 API Key**（openrouter.ai 创建一个 Key 即可调用全部模型）
- **Save 同步 Gateway：** 后台线程执行 `openclaw config set` 更新 provider + model + apiKey → 重启 Gateway（不阻塞 UI）

### 5.3 Gateway 同步
- **功能编号：** SYNC-01 **优先级：** P0 **状态：** ✅ 已实现
- AnyClaw 修改模型/API Key 后，通过 `openclaw config set` 同步到 Gateway
- Gateway 自动重启生效
- 按钮点击后立即显示 "Saving..."，后台完成后恢复 "Save"

### 5.4 Gemma 本地模型
- **功能编号：** GEMMA-01 **优先级：** P1 **状态：** 🔧 实施中
- 支持本地 Gemma 4 选装：小模型（2B）可内置，大模型（9B/27B）通过网络下载
- 用户可选择不安装本地模型（继续仅网关模式），多选安装项（2B/9B/27B）
- **推荐逻辑：** RAM < 16GB 推荐 2B / 16-32GB 推荐 9B / ≥32GB 推荐 27B
- 安装交互：总开关 + 模型清单多选 + 推荐模型自动勾选
- 下载链路支持三源自动回退与取消中断
- 配置持久化：`gemma_install_opt_in`（bool）、`gemma_model_mask`（bitmask）

**规划中：**
- 2B 小模型内置包策略
- 下载断点续传（`curl -C -`）
- 安装后模型可用性自检
- 文件完整性校验，下载失败自动清理残留

### 5.5 接入模式

**当前状态：** 仅支持 Gateway 模式。Direct API 模式为架构预留，当前暂不启用。

| 模式 | 架构 | 状态 |
|------|------|------|
| Gateway 模式 | AnyClaw → OpenClaw Gateway → LLM | ✅ 当前唯一路径 |
| Direct API 模式 | AnyClaw → LLM（AnyClaw 自身当 Agent） | ⏸️ 架构预留，详见附录 A.4 |

config.json 中若出现 Direct API 配置，启动时自动回落到 Gateway。

---

## 6. AnyClaw UI 控制权模式（用户 / AI）

- **功能编号：** CTRL-01 **优先级：** P0 **状态：** 🔧 实施中

### 6.1 模式定义

AnyClaw UI 控制权决定了**谁在操作 AnyClaw 的界面**。

| 模式 | 操作者 | 说明 |
|------|--------|------|
| **用户模式**（默认） | 人类用户 | 用户手动操作鼠标键盘，AI 仅建议与辅助 |
| **AI 模式** | Agent | Agent 模拟用户操作鼠标键盘来控制 AnyClaw UI |

**核心约束：** AI 拿到 UI 控制权后，必须模拟真实用户的鼠标键盘操作来操控 AnyClaw，没有捷径 API。AI 看到的界面和用户一样，操作方式也一样。

### 6.2 模式切换与持久化
- 控制模式在 Work 模式中显式可见可切换
- 模式值持久化到 config.json，启动时恢复
- 用户可随时一键回退到用户模式
- AI 模式下所有高风险动作仍走权限检查与审计

### 6.3 当前实现状态
- Work 面板已提供控制模式切换（User/AI）
- 模式值已写入 `config.json` 并在启动时恢复
- 当前为"控制权框架"阶段，后续补齐 AI 自动驱动执行策略

---

## 7. AI 交互模式（自主 / Ask / Plan）

- **功能编号：** AIMODE-01 **优先级：** P0 **状态：** 🆕 待实施

### 7.1 模式定义

AI 交互模式决定了 **Agent 执行任务时的行为方式**，与 UI 控制权模式独立——用户控制 AnyClaw 时也可以选择 AI 交互模式。

| 模式 | 含义 | 关键词 |
|------|------|--------|
| **自主模式**（默认） | AI 全自主判断与执行，不打断 | 自主 |
| **Ask 模式** | 自主执行，遇到决策点才暂停询问用户 | 临界询问 |
| **Plan 模式** | 先出完整方案与用户讨论，确认后自动执行 | 先谋后断 |

**自主模式**：AI 独立判断、独立执行，整个过程不向用户请求确认。

**Ask 模式**：AI 正常执行任务，但遇到需要决策的节点（比如选择方案、确认风险操作）时，暂停并询问用户，用户回答后继续。

**Plan 模式**：用户发布任务后，AI 先梳理完整的执行步骤和逻辑，与用户展开讨论。用户确认方案后，AI 按计划自动执行。

### 7.2 模式切换与持久化
- 模式切换入口：独立于 Chat/Work 面板，任何界面都可切换
- 默认模式：自主模式（首次启动默认，后续在设置中修改）
- 模式值持久化到 config.json
- 模式切换即时生效

### 7.3 与权限系统的关系

| 模式 | 权限需求 | 说明 |
|------|---------|------|
| 自主模式 | 完整权限体系兜底 | AI 全自主，需要权限系统严格约束 |
| Ask 模式 | 决策暂停提供安全保障 | 关键操作需用户确认，天然更安全 |
| Plan 模式 | 确认后执行 | 用户在执行前已审核方案，风险可控 |

### 7.4 与 UI 控制权模式的关系

两个维度正交组合：

|  | 用户控制 AnyClaw | AI 控制 AnyClaw |
|---|---|---|
| **自主模式** | 用户发任务 → AI 自主完成 | AI 自主操作 AnyClaw 执行任务 |
| **Ask 模式** | 用户发任务 → AI 遇到决策问用户 | AI 操作 AnyClaw → 遇到决策问用户 |
| **Plan 模式** | 用户发任务 → AI 出方案讨论 → 确认后执行 | AI 出方案讨论 → 确认后操作 AnyClaw 执行 |

---

## 8. 工作区管理

- **功能编号：** WS-01 **优先级：** P0 **状态：** 🔧 核心已实现

### 8.1 概念与目录结构

**工作区 (Workspace)** 是 OpenClaw Agent 的活动边界。Agent 所有文件操作限定在工作区根目录内。

**四个目录，各有归属：**

| 目录 | 归属 | 性质 |
|------|------|------|
| AnyClaw 安装目录 | AnyClaw 程序本体 (exe + dll + assets) | 只读 |
| AnyClaw 数据目录 | config.json、日志、缓存（`%APPDATA%\AnyClaw_LVGL\`） | 可读写 |
| OpenClaw 安装目录 | OpenClaw 运行时 (npm 全局) | 只读 |
| OpenClaw 工作区 | Agent 活动范围 (AGENTS.md、MEMORY.md、项目) | 可读写 |

**工作区目录结构：**

```
工作区根目录/
├── WORKSPACE.md          ← 工作区元信息（AnyClaw 管理）
├── AGENTS.md             ← Agent 行为规则
├── SOUL.md               ← Agent 人格
├── USER.md               ← 用户信息
├── MEMORY.md             ← 长期记忆
├── TOOLS.md              ← 工具配置
├── HEARTBEAT.md          ← 心跳任务
├── IDENTITY.md           ← Agent 身份
├── workspace.json        ← 权限配置（运行时读取）
├── .openclaw.lock        ← 实例锁文件
├── .backups/             ← 修改备份
├── memory/               ← 每日记忆文件
├── skills/               ← 已安装技能
└── projects/             ← 用户项目文件
```

### 8.2 多工作区支持

用户可创建多个工作区，每个工作区独立配置：

```
工作区列表:
├── 🏠 个人助手    D:\AI\personal\       SOUL.md=通用助手, 权限=基础
├── 💻 开发项目    D:\Projects\          SOUL.md=开发者, 权限=开放
└── 🔒 敏感项目    D:\Secret\            SOUL.md=谨慎模式, 权限=严格
```

每个工作区独立维护：AGENTS.md / SOUL.md / TOOLS.md、workspace.json、MEMORY.md / memory/、skills/

### 8.3 WORKSPACE.md

工作区身份证：

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

### 8.4 工作区模板

| 模板 | SOUL.md 风格 | 额外文件 | 默认权限 |
|------|-------------|---------|---------|
| 通用助手 | 友好、通用 | 无 | 基础（文件读写 + 网络） |
| 开发者 | 技术导向 | TOOLS.md（编译工具配置） | 开放（+ 命令执行） |
| 写作者 | 文学风格 | 素材目录 | 基础 |
| 数据分析 | 严谨、数据导向 | Python 环境配置 | 开放（+ 本地执行） |

### 8.5 健康检查

每次启动时执行：

| 检查项 | 未满足处理 |
|--------|-----------|
| 工作区目录是否存在 | 提示创建或更换目录 |
| 磁盘空间是否 > 100MB | 警告 |
| 目录是否有写入权限 | 降级为只读模式 |
| 是否被其他实例占用（lock 文件） | 检测进程存活，死亡则清理 lock，存活则拒绝启动 |
| 目录结构是否完整 | 引导创建缺失的核心文件 |
| OpenClaw 版本是否兼容 | 提示更新 |

### 8.6 工作区锁
- 启动时创建 `.openclaw.lock`，写入 PID
- 退出时删除 lock 文件
- 启动检测到已有 lock → 判断 PID 是否存活 → 存活则拒绝，死亡则覆盖

### 8.7 迁移（导出/导入）

**导出：**
- 打包内容：AnyClaw 配置 + OpenClaw 工作区全量 + manifest.json（版本号、来源主机名、导出时间、文件校验和）
- 输出：`AnyClaw_Migration_{version}_{timestamp}.zip`
- 安全：API Key 不导出明文，仅保留哈希；导入时需重新输入

**导入：**
- 前置检查：Node.js ≥ 18、磁盘空间 > 500MB、OpenClaw 可用
- 路径适配：两台机器用户名可能不同 → 导入时自动检测当前用户的路径
- 流程：解压 → 校验 manifest → 复制配置 → 安装 OpenClaw → 启动自检 → 重启

### 8.8 目录消失处理

工作区目录不可达时：

```
⚠️ 工作区目录不存在
D:\MyAIWorkspace\

○ 重新创建此工作区
○ 选择其他目录
○ 创建新工作区
```

### 8.9 配置存储

| 文件 | 路径 | 管理方 | 说明 |
|------|------|--------|------|
| `config.json` | `%APPDATA%\AnyClaw_LVGL\` | AnyClaw | 主配置 |
| `workspace.json` | 工作区根目录 | AnyClaw | 运行时权限校验 |
| `WORKSPACE.md` | 工作区根目录 | AnyClaw | 工作区元信息 |
| `.openclaw.lock` | 工作区根目录 | 运行时 | 实例锁 |

---

## 9. 权限系统

- **功能编号：** PERM-01 **优先级：** P0 **状态：** 🔧 核心已实现

### 9.1 三层权限模型

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

### 9.2 第 1 层：工作区边界

Agent 的所有文件操作限定在工作区根目录内。

**路径校验逻辑：**
1. 检查是否在 `denied_paths` → 命中则拒绝
2. 检查是否在 `workspace_root` 下 → 命中则允许
3. 检查是否在 `extra_allowed_paths` → 命中则按 mode 处理
4. 都不命中 → 默认拒绝（白名单模式）

### 9.3 第 2 层：操作权限

每种操作类型独立授权，四种取值：`allow`（允许）、`deny`（禁止）、`ask`（每次询问）、`read_only`（仅读）。

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
| `net_outbound` | `allow` | 出站 HTTP/HTTPS |
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

### 9.4 第 3 层：资源上限

| 资源键 | 默认值 | 说明 |
|--------|--------|------|
| `max_disk_mb` | 2048 | 工作区最大磁盘占用 |
| `cmd_timeout_sec` | 30 | 单条命令超时（超时自动 kill） |
| `net_rpm` | 60 | 网络请求频率上限（次/分钟） |
| `max_subagents` | 5 | 最大并发子代理数 |
| `max_file_read_kb` | 500 | 单次文件读取上限 |
| `max_remote_connections` | 3 | 最大远程设备连接数 |

### 9.5 设备管理

**设备分类：**

```
设备 (Devices)
├── 本地外设：摄像头、麦克风、屏幕截图、扬声器
├── 本地存储：内置磁盘、USB 移动硬盘/U盘、SD 卡、网络映射驱动器
├── 远程设备：已配对 OpenClaw 节点、SSH 远程主机、云服务器实例、IoT 设备
└── 新插入设备：USB 存储/外设、蓝牙、Thunderbolt
```

**新插入设备默认策略：** 一律询问，不自动放行。

**设备信任表：** 用户选择"记住此设备"后，设备信息写入 `trusted_devices`，用序列号/硬件 ID 做唯一标识。

### 9.6 运行时强制执行

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
         ├── 操作权限 = ask？    → 🔔 弹窗 → 用户决定
         └── 不在任何列表？      → 默认拒绝（白名单模式）
```

### 9.7 权限升级（单次放行）

Agent 偶尔需要越权操作，弹窗确认：

```
🔒 Agent 请求临时权限
操作: 安装软件
命令: apt install ffmpeg
当前权限: ❌ 禁止

○ 允许（仅本次）  ○ 允许（永久开启此权限）  ○ 拒绝
```

- 单次放行不过夜——下次启动仍按 permissions.json 配置
- 永久开启 → 更新 permissions.json + 同步到 AGENTS.md/TOOLS.md 的 MANAGED 区域

### 9.8 AGENTS.md / TOOLS.md 双向同步

**MANAGED 区域机制：**

```markdown
## 用户自由区域（随便写，AnyClaw 不动）

<!-- ANYCLAW_MANAGED_START -->
## 工作区边界
- 根目录: D:\MyAIWorkspace\

## 权限规则
- 命令执行: ⚠️ 每次询问
<!-- ANYCLAW_MANAGED_END -->

## 更多自由区域
```

- `permissions.json` 是唯一权限真相（Source of Truth）
- AGENTS.md/TOOLS.md 的 MANAGED 区域是其"可读投影"
- 写入前自动备份到 `.backups/`，保留最近 10 份

### 9.9 审计日志

所有权限决策记录到 `%APPDATA%\AnyClaw_LVGL\audit.log`：

```
[2026-04-07 19:20:01] action=perm_load target=permissions.json decision=ok
[2026-04-07 19:20:05] action=exec_check target=openclaw gateway start decision=allow_once
[2026-04-07 19:20:07] action=path_check target=C:\Windows\system32\hosts decision=deny:external_default
```

**规划中：**
- 审计日志增加完整性字段（链式 hash）用于追溯校验
- 权限历史可视化面板

### 9.10 权限配置存储

**`permissions.json`**（`%APPDATA%\AnyClaw_LVGL\`）— Source of Truth：

```json
{
  "version": 1,
  "workspace_root": "D:\\MyAIWorkspace\\",
  "extra_allowed_paths": [],
  "denied_paths": ["C:\\Windows\\", "C:\\Program Files\\"],
  "permissions": { "fs_read_workspace": "allow", "exec_shell": "ask", "...": "..." },
  "trusted_devices": [],
  "limits": { "max_disk_mb": 2048, "cmd_timeout_sec": 30, "...": "..." }
}
```

**`workspace.json`**（工作区根目录）— 运行时投影。

**配置优先级：** `permissions.json`（主配置）→ `workspace.json`（运行时投影）→ AGENTS.md（可读投影）。启动时自动做一致性校验，不一致则重建 workspace.json。

### 9.11 Settings 权限管理面板

AnyClaw Settings 已新增 **Permissions** Tab（v1）：
- 核心权限项下拉编辑（allow/deny/ask/read_only）
- 保存后写入 `permissions.json`
- 启动时自动初始化权限（首启写默认配置）

**规划中：**
- 设备信任表管理
- 资源上限编辑
- 权限历史可视化与筛选
- 权限导入/导出

### 9.12 Skill 安装与权限交互

安装 Skill 时检查权限冲突：
- 权限充足 → 直接安装
- 权限不足 → 提示需开启的权限 → 用户确认后更新
- 用户拒绝 → 取消安装

---

## 10. OpenClaw 集成

### 10.1 进程管理
- **功能编号：** PM-01 **优先级：** P0
- 一键启动、停止、重启 OpenClaw Gateway（`openclaw gateway start/stop`）
- 进程检测：`CreateToolhelp32Snapshot` 遍历进程列表，查找 `node.exe`
- OpenClaw 自动检测策略：
  - 策略 1：`where openclaw` PATH 查找 → 读取 `openclaw.json` 获取端口
  - 策略 2：npm 全局路径 `%APPDATA%\npm\node_modules\openclaw\openclaw.cmd`

**状态判断：**

| 状态 | 条件 |
|------|------|
| Running | 进程存在 + HTTP 200 |
| Error | 进程存在 + HTTP 非 200 |
| Detected | 进程不存在 + 已检测到安装 |
| NotInstalled | 未安装 |

### 10.2 状态指示

**三级统一规范**（LED / 托盘图标 / Task List 圆点 三者一致）：

| 状态 | 颜色 | 含义 | 用户理解 |
|------|------|------|---------|
| Ready — 就绪 | 🟢 绿 | Gateway 在 + HTTP 通 + Agent 空闲 | ✅ 可以发消息 |
| Busy — 处理中 | 🟡 黄 | Gateway 在 + 有活跃 Session（ageMs < 5min） | ⏳ 模型在回复中 |
| Checking — 检测中 | 🟡 黄 | 启动中 / 首次轮询 | 🔄 正在检测 |
| Error — 异常 | 🔴 红 | Gateway 进程不在 / HTTP 不通 | ❌ 无法连接 |

**故障原因显示：** Error 时状态文字显示具体原因（Gateway 未运行 / 端口无响应 / 连接超时），支持 hover tooltip。

### 10.3 健康监控
- **功能编号：** HM-01 **优先级：** P1
- **检测频率：** 每 30 秒（可配置 5s / 15s / 30s / 60s / 120s），通过 `g_refresh_interval_ms` 控制
- **检测方式：** 后台线程 `health_thread`，每轮 `Sleep(g_refresh_interval_ms)`

**检测内容：**
- 进程检测：`is_node_running()`
- 端口检测：`http_get("http://127.0.0.1:18789/health")`
- Session 检测：HTTP 通时调 `gateway call sessions.list --json`，检查是否有 ageMs < 5min 的活跃 Session

**LED 状态判定逻辑：**

```
Gateway 进程不在                  → 🔴 Error
Gateway 在 + HTTP 不通            → 🔴 Error
Gateway 在 + HTTP 通 + 首次轮询   → 🟡 Checking
Gateway 在 + HTTP 通 + 有活跃Session → 🟡 Busy
Gateway 在 + HTTP 通 + 无活跃Session  → 🟢 Ready
```

**自动重启策略：**
- 进程不存在 + HTTP 无响应 → 自动执行 `openclaw gateway start`，仅重试 1 次
- 重启后仍失败 → 托盘变红，不再自动重启

### 10.4 会话与任务管理
- **功能编号：** TM-01 **优先级：** P1 **状态：** 🔧 实施中

**核心约束：** Gateway 不提供 REST API，所有数据通过 CLI 子进程调用获取。

| 数据源 | 命令 | 实测耗时 |
|--------|------|---------|
| 健康状态 | `curl http://127.0.0.1:18789/health` | ~50ms |
| 会话列表 | `openclaw gateway call sessions.list --json` | ~1.5s |
| 定时任务 | `openclaw cron list --json` | ~1.5s |

**任务类型：**

| 类型 | 来源 | 生命周期 |
|------|------|---------|
| Gateway 进程 | 系统 | 常驻 |
| 会话 (Session) | 用户/AI 对话 | 活跃→休眠（ageMs 增长） |
| 子代理 (Sub-Agent) | 主会话派生 | spawn→running→completed |
| 定时任务 (Cron) | 用户配置 | 常驻（周期触发） |

**Task List UI：**

```
┌─ Task List ───────────────────────────────────┐
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
│     [Run Now] [Disable] [Delete]               │
│                                                │
│  [+ Add Cron Job]                              │
└────────────────────────────────────────────────┘
```

**用户操作 → CLI 映射：**

| 按钮 | 执行命令 | 反馈 |
|------|---------|------|
| Abort | `gateway call sessions.reset --json` | 按钮 loading → 列表刷新 |
| Run Now | `cron run <id>` | 按钮 loading → 列表刷新 |
| Enable/Disable | `cron enable/disable <id>` | 状态切换 |
| Delete | `cron rm <id>` | 弹窗确认 → 移除 |

**Session 计数：** "Task List"标题旁显示 `(N)` 计数 badge，30 秒自动刷新。

**清理策略：**
- Session：ageMs < 30 分钟正常显示，> 30 分钟隐藏
- Sub-Agent：完成后保留 30 分钟，自动清理
- Cron：手动删除

**规划中（v2.1）：**
- 会话历史查看
- Cron 编辑
- Cron 执行历史
- 任务通知（子代理完成/失败 → 托盘气泡）

### 10.5 Skill 管理
- **功能编号：** SK-01 **优先级：** P1
- Skill 批量下载/安装（ClawHub 仓库）
- Skill 搜索与浏览
- 内置（Built-in）+ 第三方（ClawHub）分类展示
- Skill 启用/禁用、版本管理与更新

---

## 11. AnyClaw UI 控制权与界面模式

### 11.1 界面模式（Chat / Work）
- **功能编号：** MODE-01 **优先级：** P1 **状态：** 🔧 实施中

AnyClaw 有两种核心模式，Voice 不是独立模式，而是 Chat 模式中的语音输入方式。

| 模式 | 角色 | 内容 |
|------|------|------|
| **Chat 模式** | AI 管家 | 文本对话 + 语音输入 + 文件发送 + 知识引用 + 局域网协作 + 日常事务 |
| **Work 模式** | AI 编程工作台 | 代码编辑/查看 + 终端 + Git + AI 辅助编程 + 任务管理 |

**Chat 模式（管家）：** 用户与 AI 管家对话，处理日常事务——查邮件、设提醒、搜文件、局域网聊天、文件传输。支持文字和语音两种输入方式。

**Work 模式（编程）：** 编程工作台布局，代码编辑/浏览 + 内置终端 + Git 状态 + AI 辅助写代码。对标 Cursor 的 Agent 编程体验。

**Voice 输入：** Chat 模式中的语音输入按钮（输入区上传按钮左侧），点击后语音转文字填入输入框，不是独立模式。

- 顶栏模式切换按钮：Chat ↔ Work 单按钮切换
- 各模式保留各自输入状态，切换不丢上下文
- 模式切换延迟目标：< 150ms

**当前实现状态：**
- 右侧主区已支持 Chat / Voice / Work 模式切换
- 聊天模式与输入状态可在模式切换后保持
- 语音与工作模式已具备独立容器
- **待调整：** Voice 从独立模式降级为 Chat 模式的输入方式；Work 模式从"管理面板"转型为"编程工作台"

---

## 12. 设置

### 12.1 Settings Tab 结构

| Tab | 内容 |
|-----|------|
| General | Boot Start / Auto Start / Theme / Language / Exit Confirm |
| Model | Current Model / Search / Model List / API Key |
| Skills | Skill 列表 / 下载 / 安装 |
| Log | Log Enable / Log Level / Filter / Export / Clear |
| Permissions | 权限编辑（allow/deny/ask/read_only） |
| About | 品牌信息 / Tech Stack / Config Import/Export / Migration |

### 12.2 General Tab
- **开机自启动：** Windows 注册表 `HKCU\...\Run` 写入/删除 `AnyClaw_LVGL` 键值
- **Auto Start：** 异常退出自动拉起 OpenClaw Gateway，独立开关控制
- 语言切换（CN/EN/KR/JP），立即生效
- 主题切换（默认暗色 / 默认亮色 / 经典暗色），立即生效
- 退出确认开关
- Security 状态面板：动态检测 API Key 状态、端口号、配置目录可写性
- **Reconfigure Wizard 按钮** — 关闭设置面板 → 打开向导

### 12.3 Model Tab
- 当前模型显示 + 下拉选择 + 自定义模型 + 免费模型
- Provider 检测 + API Key 联动
- Save 同步 Gateway（详见第 5 章）

### 12.4 Log Tab
- 日志系统总开关（Enable / Disable）
- 日志级别（Debug / Info / Warn / Error）
- 级别过滤（All / Info+ / Warn+ / Error Only），仅影响 UI 显示
- 日志列表实时展示，按级别着色
- 导出日志、清除日志
- 底部级别颜色图例

### 12.5 About Tab
- 品牌信息、版本号、Tech Stack
- 配置导入/导出
- 一键迁移（Export/Import Migration）

---

## 13. 外观与主题

### 13.1 色彩方案
- **功能编号：** TH-01 **优先级：** P1

| 主题 | 背景 | 面板 | 文字 |
|------|------|------|------|
| 默认暗色 | #1A1E2E | #232838 | #E0E0E0 |
| 默认亮色 | #F5F5F5 | #FFFFFF | #333333 |
| 经典暗色 | #2D2D2D | #3C3C3C | #FFFFFF |

### 13.2 字体管理

| 模块 | 字体 | 字号 |
|------|------|------|
| 中文 | 微软雅黑 (msyh.ttf) | 16px |
| 英文 | Montserrat | 10~20px |
| 图标 | LVGL Symbols | 对应字号 |

### 13.3 通用控件体系
- **功能编号：** WIDGET-01 **优先级：** P1 **状态：** ✅ 已实现

**目标：** 统一控件创建方式，消除样式重复定义。

**规划控件库：**

```
src/widgets/
├── aw_button.h      // BTN_PRIMARY / BTN_SECONDARY / BTN_DANGER / BTN_GHOST
├── aw_input.h       // TextArea / Dropdown / Switch 统一样式
├── aw_panel.h       // Card / Dialog / Modal
├── aw_divider.h     // Horizontal / Vertical
├── aw_label.h       // Title / Body / Hint / Mono
└── aw_common.h      // 公共颜色/间距/圆角常量
```

---

## 14. 系统管理

### 14.1 日志系统
- **功能编号：** LOG-01 **优先级：** P1
- 日志文件：`%APPDATA%\AnyClaw_LVGL\logs\app.log`
- 轮转规则：文件超过 1MB 自动轮转，最多保留 5 个文件
- 启动时自动清理 7 天以上的轮转文件
- 格式：`[YYYY-MM-DD HH:MM:SS.mmm] [LEVEL] [MODULE] message`

### 14.2 错误处理与崩溃恢复
- **功能编号：** E-01 **优先级：** P1
- 崩溃日志：`%APPDATA%\AnyClaw_LVGL\crash_logs\crash_*.log`，保留最近 10 个
- SetUnhandledExceptionFilter 捕获异常
- Stack Trace 通过 dbghelp 符号解析，最多 64 帧
- 崩溃后生成 `last_crash.txt` 标记文件

### 14.3 通知系统
- **功能编号：** N-01 **优先级：** P1
- 托盘气泡通知
- 日志面板通知
- 状态栏颜色变化

### 14.4 更新机制
- **功能编号：** U-01 **优先级：** P0
- OpenClaw 更新：通过 `openclaw update` 命令
- AnyClaw 更新：仅支持本地安装覆盖

### 14.5 版本号规范
- **功能编号：** VER-01 **优先级：** P1
- 格式：`v{主版本}.{次版本}.{修订号}`
- 当前版本：`v2.0.1`
- 存储：`#define ANYCLAW_VERSION "2.0.1"`，编译时嵌入 exe 资源段

### 14.6 License 授权系统
- **功能编号：** LIC-01 **优先级：** P0 **状态：** 🔧 设计完成，待实施

**两种解锁方式（OR 逻辑，任意一种有效即可使用）：**

| 密钥类型 | 前缀 | 生成方式 | 有效期 |
|----------|------|---------|--------|
| HW 密钥 | `HW-` | `keygen.py hw <MAC>` | 永久（绑定该 MAC 地址） |
| TM 密钥 | `TM-` | `keygen.py tm <hours>` | 指定小时数（到期锁死） |

**密钥格式：** `HW-XXXXX-XXXXX-XXXXX` / `TM-XXXXX-XXXXX-XXXXX`，前缀区分类型，AnyClaw 自动识别验证逻辑。

**HW 密钥（硬件绑定）：**
- 用户获取 MAC 地址：AnyClaw 弹窗自动显示（`GetAdaptersAddresses` 取第一个非回环网卡），格式 `XX:XX:XX:XX:XX:XX`
- 用户把 MAC 地址发给开发者
- 开发者用独立脚本生成 HW 密钥：`keygen.py hw <MAC>`
- 用户输入 HW 密钥 → 验证通过 → 永久解锁该机器
- HW 密钥优先级高于 TM 密钥

**TM 密钥（时间激活）：**
- 密钥内编码允许使用时长（小时），生成时指定：`keygen.py tm 720`（30 天）
- AnyClaw 读取密钥内的时间信息，到期完全锁死
- 到期后弹窗提醒用户获取新密钥
- 无时间上限

**验证流程：**
1. 读取密钥前缀 → 判断类型（HW/TM）
2. HW：解码 → 重新采集当前 MAC → HMAC 比对 → 通过则永久解锁
3. TM：解码 → 提取过期时间 → 检查是否过期 → 未过期则临时解锁
4. 两种密钥独立存储，任意一种有效即可使用

**密钥存储：**
```json
// config.json
{
  "hw_key": "HW-XXXXX-XXXXX-XXXXX",
  "tm_key": "TM-XXXXX-XXXXX-XXXXX",
  "tm_expiry": 1743981600
}
```

**密钥输入界面：**
- 过期弹窗：全屏遮罩 + 大蒜图标 + "试用期已结束" + 密钥输入框 + 激活按钮
- 底部状态栏显示：剩余 XXh（低时间时变红）

**密钥生成工具：** `tools/keygen.py`（独立脚本，不随发布包）
```
python3 keygen.py hw AA:BB:CC:DD:EE:FF   # 生成 HW 密钥
python3 keygen.py tm 720                  # 生成 720h TM 密钥
python3 keygen.py --verify KEY            # 验证密钥
```

### 14.7 离线授权（硬件指纹 — 已暂停）
- **功能编号：** LIC-02 **优先级：** P2 **状态：** ⏸️ 暂不实施
- 基于 MAC + 磁盘序列号 + 主板 UUID 的组合指纹方案
- 详见附录 A.5

---

## 15. 技术规格

### 15.1 快捷键
- **功能编号：** K-01 **优先级：** P1
- Ctrl+V 粘贴、Ctrl+C 复制、Ctrl+X 剪切、Ctrl+A 全选
- Enter 发送、Shift+Enter 换行
- 选中文字后按键直接替换

### 15.2 网络与代理
- **功能编号：** P-01 **优先级：** P1

### 15.3 性能约束
- **功能编号：** PF-01 **优先级：** P1

### 15.4 安全
- **功能编号：** SEC-01 **优先级：** P0

### 15.5 DPI 自动检测
- **功能编号：** DPI-01 **优先级：** P1
- 启动时调用 `SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)`
- 布局常量按 DPI 缩放（`SCALE()` 宏）
- config 中保存 `dpi_scale` 作为快照，启动时对比系统 DPI 一致性

### 15.6 IME 输入法支持
- **功能编号：** IME-01 **优先级：** P1
- SDL hints 启用 IME：`SDL_HINT_IME_SHOW_UI=1`、`SDL_HINT_IME_SUPPORT_EXTENDED_TEXT=1`
- 支持中文 / 日文输入法候选窗口显示

### 15.7 宏开关管理
- **功能编号：** MACRO-01 **优先级：** P1 **状态：** ✅ 已实现

所有可调参数用宏集中管理，消除硬编码散落。新建 `src/app_config.h` 集中定义。

| 类别 | 示例宏 |
|------|--------|
| 窗口尺寸 | `WIN_DEFAULT_W`, `WIN_DEFAULT_H` |
| 标题栏 | `TITLE_BAR_H` |
| 面板布局 | `LEFT_PANEL_DEFAULT_W`, `SPLITTER_W` |
| 聊天气泡 | `BUBBLE_MAX_WIDTH_PCT` |
| 输入框 | `CHAT_INPUT_DEFAULT_H` |
| Health 间隔 | `HEALTH_CHECK_DEFAULT_MS` |
| Log 轮转 | `LOG_MAX_SIZE`, `LOG_MAX_FILES` |

---

## 16. 远程协作与高级功能

- **功能编号：** REMOTE-01 **优先级：** P0 **状态：** 🆕 待实施

### 16.1 远程协作（远程桌面 + 语音 + 控制）

**能力范围：**
- 两个 AnyClaw 用户点对点会话
- 远程桌面画面共享
- 双向语音通话
- 经授权后远程控制对方输入（鼠标/键盘）

**安全设计（强制）：**
- 会话必须双向确认（发起方 + 接收方）
- 控制权限分级：仅看屏 / 语音 / 完整控制
- 每次控制会话必须可随时一键中止（双方都可中止）
- 全链路加密 + 审计日志记录（开始/结束/授权变更）

**交互原则：**
- 默认最小权限（仅看屏）
- UI 显示"正在被远程控制"持续提示，不可隐藏
- 高风险操作二次确认（文件删除、命令执行、系统设置）

### 16.2 局域网群聊（LAN Chat）
- **功能编号：** LAN-CHAT-01 **状态：** 🔧 实施中（MVP）
- 同一 LAN 内可由一台 AnyClaw 启动 Host，其它客户端连接
- 支持能力：全局群聊、私聊、建群、加群、群消息发送
- 协议：轻量文本帧（cmd/from/to/group/payload），TCP 传输

**规划中：** 发现协议（mDNS/广播）、离线消息持久化

### 16.3 FTP 传输
- **功能编号：** FTP-01 **状态：** 🔧 实施中（基础版）
- 单文件 FTP 上传与下载，支持进度展示与取消
- 传输层采用 WinINet

**规划中：**
- FTP 断点续传（下载侧优先）：检测本地已下载大小后尝试 `REST`
- Retry Last 按钮：失败后一键复用上次参数重试
- FTPS 支持
- 目录递归同步

### 16.4 本地知识库
- **功能编号：** KB-01 **优先级：** P1 **状态：** 🔧 实施中（V1）

**V1 已实现：**
- 发送聊天时按关键词检索本地 KB，Top-N 文档片段自动拼接到请求上下文
- KB 索引文件持久化（`%APPDATA%\AnyClaw_LVGL\kb_index.txt`）

**规划中（V2）：**
- 检索排序升级为评分模型（频次、首次命中位置、文件名命中）
- 上下文构建升级为"命中片段截取"而非固定前缀截断
- 片段去重与总长度预算控制

---

## 附录

### A.1 项目目录结构

```
AnyClaw_LVGL/
├── CMakeLists.txt              # CMake 构建配置
├── src/                        # 源代码
│   ├── main.cpp                # 程序入口、窗口创建、主循环
│   ├── ui_main.cpp             # 主界面 UI
│   ├── ui_settings.cpp         # 设置页 UI
│   ├── tray.cpp / tray.h       # 系统托盘
│   ├── health.cpp / health.h   # 健康检查
│   ├── openclaw_mgr.cpp        # OpenClaw Gateway 进程管理
│   ├── http_client.cpp         # HTTP 客户端封装
│   ├── autostart.cpp           # 开机自启动
│   ├── selfcheck.cpp / .h      # 启动自检
│   ├── app.h                   # 公共声明
│   ├── app_log.h               # 日志宏
│   ├── lang.h                  # 国际化定义
│   ├── theme.h                 # 主题色定义
│   ├── markdown.h              # Markdown 渲染
│   └── ...
├── assets/                     # 运行时资源
├── thirdparty/                 # LVGL + SDL2
├── tools/                      # 构建脚本
├── artifacts/                  # 构建产物
└── docs/                       # 文档
```

### A.2 Gateway 与 Session 架构

**Gateway 是什么：** OpenClaw 的守护进程，跑在本机 `localhost:18789`，是整个系统的"总机"：
- 收消息：从各个渠道（WebChat、Discord、Telegram、CLI 等）收用户输入
- 调度 Agent：把消息路由给对应的 Session，管理 Session 生命周期
- 暴露 API：HTTP/WebSocket 接口
- 管理心跳：定时 Poll Agent 确认存活

**核心概念区分：**

| 概念 | 说明 |
|------|------|
| **Agent** | AI 大脑，定义 system prompt、工具、模型 |
| **Session** | 一次对话实例，携带上下文和历史消息 |
| **Gateway** | 守护进程，路由消息、管理 Session 生命周期 |

同一个 Agent 可以同时跑多个 Session。Gateway 路由消息到 Session，Session 决定用哪个 Agent。

**Session-Channel 映射：**

```
Gateway
├── Session: webchat:main       → Agent: main（主会话）
├── Session: discord:general    → Agent: main
├── Session: telegram:xxx       → Agent: main
├── Session: cron:main:abc123   → Agent: main（定时任务）
├── Session: subagent:xyz789    → Agent: main（子代理）
└── Session: webchat:data-bot   → Agent: data-bot
```

**Heartbeat vs Cron：**

| 维度 | Heartbeat | Cron |
|------|-----------|------|
| 适用场景 | 多项检查可合并 | 精确时间要求 |
| 上下文 | 携带近期对话历史 | 隔离会话，从零开始 |
| 时间精度 | 允许 ~30min 漂移 | 精确到秒 |
| 输出 | 需要时主动联系用户 | 直接投递到指定 channel |

**Gateway 故障影响：** Gateway 挂掉后所有 Session 断开、心跳停止、Cron 停止触发。恢复后自动恢复能力（心跳、Cron），Session 需用户重新连接。

**AnyClaw 感知状态方式：** 进程检测 + HTTP 端口检测，不依赖 Gateway，本地直连诊断。

### A.3 Agent / Sub-Agent / Session 关系

| 概念 | 说明 |
|------|------|
| **Main Agent** | 与用户直接交互的主代理 |
| **Sub-Agent** | 由主代理通过 `sessions_spawn` 派出的独立代理 |
| **Sub-Agent 深度限制** | 最多 1 层，子代理不能再派子代理 |
| **结果通知** | 完成时自动 push 结果到主代理（push-based） |

**通信模型：**
```
用户 ↔ Main Agent
          ├── sessions_spawn → Sub-Agent A
          ├── sessions_spawn → Sub-Agent B
          └── sessions_spawn → Sub-Agent C
```

**文件系统与 Context：**
- 共享：所有 Session 实例共享同一个文件系统
- 不共享：每个 Session 有独立的对话历史和 context 窗口
- "写下来的才算记得" — context 窗口会截断旧消息

### A.4 AnyClaw Agent 管理器架构（Direct API 终局目标）

**当前架构：** AnyClaw (GUI) → OpenClaw Gateway (Agent) → LLM

**目标架构：** AnyClaw 自身成为 Agent 管理器，直接管理 prompt、工具调用、会话上下文，同时也能管理外部 Agent。

**Direct API 模式含义（理解 B）：**

```
AnyClaw (Agent 管理器) → LLM API
                     ↘ 外部 Agent（OpenClaw、Hermes 等）
```

AnyClaw 自己实现 prompt 管理、工具调用、会话上下文。参考 OpenClaw 开源架构和 Claude 等开源代码结合设计。

**当前状态：** 仅支持 Gateway 模式，Direct API 为架构预留。

### A.5 离线授权（硬件指纹 — 详细设计）

基于 MAC + 磁盘序列号 + 主板 UUID 组合指纹（SHA-256 → 8 段机器码）。独立密钥生成工具 `tools/license_gen.py`。详见 §14.7。

### A.6 需求讨论草稿

**产品核心价值：** 简单、安全、长期稳定。

**知识库方向：** 用户可在本地建立知识库体系，脱离集体平台，数据自主可控，不依赖任何云服务。

**参考项目对标（CCB / DeerFlow）：**

| 功能 | 价值 | 优先级 |
|------|------|--------|
| 可观测性追踪 | 降低排障成本 | P1 |
| 沙箱执行模式 | 降低主机风险面 | P0（v1 已完成） |
| 一键环境体检自修复 | 降低安装失败率 | P1 |
| Feature Flag 开关体系 | 低风险灰度发布 | P1 |
| 长记忆整理任务 | 长会话性能提升 | P2 |

### A.7 LAN Chat 设计讨论（2026-04-11）

#### 定位

同局域网内 AnyClaw 用户之间直接通信，不走 Gateway，不走公网。面向学生（实验室/宿舍）和职场用户（办公室），用于日常协作、文件传输、AI 协作。

#### 身份绑定模型

昵称与设备硬件 ID 绑定，防冒充、防伪装。

```
device_id = SHA256(MAC + salt) 前 16 字符
signature = HMAC(MAC, nickname)
广播：{ nickname, device_id, signature }
```

- 对外只暴露 device_id（哈希），真实 MAC 不明文传输
- 签名用真实 MAC 生成，防篡改和冒充
- salt 可用 AnyClaw 版本号或固定字符串
- 别人改昵称 → 签名不匹配 → 拒绝
- 别人用自己 MAC + 你的昵称 → 签名不一致 → 拒绝

#### 自动发现

- **主方案**：UDP 广播或 mDNS，启动 AnyClaw 后自动发现同网段实例，显示对方昵称
- **Fallback**：企业网络可能屏蔽 UDP 广播/mDNS → 支持手动输入 IP + 端口
- **隐身模式**：不想被发现时关闭广播

#### 聊天能力

| 能力 | 说明 |
|------|------|
| 群聊 | 同 LAN 内公开频道，所有在线用户可见 |
| 私聊 | 点对点消息 |
| 建群 | 用户自建主题群（"前端组""论文组"） |
| 离线消息 | 对方不在线时暂存发送方本地，上线后推送 |

#### 文件传输

LAN 最大优势——同一局域网内传输不走公网，速度快。

- 拖拽发送文件/文件夹
- 断点续传
- 传输进度条
- 接收方确认（防止垃圾文件）
- 大文件后台传输 + 限速（避免卡死聊天）

#### AI 协作（规划中）

AnyClaw 区别于普通局域网聊天工具的差异化能力。

- **AI 群助手**：群里加一个共享 Agent，所有人 @它就能用
- **协同任务**：用户 A 发起任务 → Agent 拆分 → 分配给多个用户的 Agent 并行执行
- **共享 Agent 权限**：需确定群内共享 Agent 能访问谁的文件（只读 vs 读写）

#### 待定问题

| 问题 | 备注 |
|------|------|
| 群组管理 | 谁是群主？创建者离线后群怎么办？邀请制 vs 公开加入？踢人/禁言？ |
| 消息存储 | 本地 SQLite？纯内存？保留多久？离线消息暂存在发送方还是？ |
| AI 群助手权限 | 共享 Agent 能访问谁的文件？只读的话价值有限 |
| 网络环境 | 不同子网/VLAN 之间能否发现？需要手动 IP fallback |
| 跨平台 | 当前仅 Windows，Linux AnyClaw 能否与 Windows 互通？ |
| 大文件传输 | 2GB 视频会不会卡死聊天？需要限速或后台传输 |
| 消息持久化 | 消息存本地还是纯内存？是否永久保存？ |

### A.8 竞品功能吸收（2026-04-11）

对 Ollama、LM Studio、Cursor、Claude 四个产品的横向对比，筛选 AnyClaw 应当吸收的功能方向。

#### 总览

| # | 功能方向 | 来源 | 优先级 | 对标产品 |
|---|---------|------|--------|---------|
| 1 | MCP Client | LM Studio | 🔴 高 | LM Studio |
| 2 | 本地模型推理引擎 | Ollama + LM Studio | 🔴 高 | Ollama / LM Studio |
| 3 | OpenAI 兼容 API 服务端 | LM Studio + Ollama | 🔴 高 | LM Studio / Ollama |
| 4 | RAG 文档聊天体验 | LM Studio | 🔴 高 | LM Studio |
| 5 | SDK / API 文档 | LM Studio | 🟡 中 | LM Studio |
| 6 | Headless 模式 | LM Studio | 🟡 中 | LM Studio |
| 7 | 跨平台 | LM Studio + Ollama | 🟡 中 | LM Studio / Ollama |
| 8 | 竞品参考——Cursor Composer | Cursor | 🟡 中 | Cursor |
| 9 | 模型格式兼容（GGUF/MLX） | LM Studio | 🟢 低 | LM Studio |
| 10 | 多模态能力 | Claude | 🟢 低 | Claude |
| 11 | 模型社区生态 | Ollama | 🟢 低 | Ollama |

#### 1. MCP Client（Model Context Protocol）

**来源：** LM Studio 0.4.x 已支持 MCP Client，用户可安装 MCP Server 并与本地模型配合使用。

**现状：** AnyClaw 的工具调用完全依赖 OpenClaw Gateway 内置的 Skill 系统，没有接入 MCP 协议。

**吸收价值：**
- MCP 正在成为 LLM 工具调用的事实标准协议（Claude Desktop、Cursor、LM Studio 均已支持）
- 接入 MCP 后，AnyClaw 可直接复用社区已有的 MCP Server（数据库查询、浏览器控制、文件系统等），无需逐个开发 Skill
- 降低 AnyClaw 工具生态的建设成本

**设计方向：**
- AnyClaw 内置 MCP Client，支持配置 MCP Server 列表
- MCP Server 的工具注册到 Agent 工具池，与现有 Skill 并行
- MCP Server 的权限纳入 AnyClaw 三层权限体系管控（§9）
- 安装/卸载 MCP Server 可通过 GUI 完成

#### 2. 本地模型推理引擎

**来源：** Ollama 是纯本地推理引擎，一行命令完成模型下载+运行；LM Studio 内置 llama.cpp + MLX 运行时，GUI 内完成模型管理和推理。

**现状：** AnyClaw §5.4 规划了 Gemma 本地模型安装（2B/9B/27B），但只覆盖"下载安装"，不包含推理运行时。用户装完模型后仍需外部推理服务。

**吸收价值：**
- 当前架构下，本地模型 = 安装 Gemma + 依赖外部推理服务 → 门槛高
- 对标 Ollama/LM Studio，用户期望"装完就能跑"
- 本地推理是"数据不离开本地"核心价值的最后一环

**设计方向：**
- 集成轻量推理引擎（如 llama.cpp 作为库内嵌，非外部进程）
- GUI 内完成：模型搜索 → 下载 → 加载 → 聊天，全流程不离开 AnyClaw
- 推理引擎的选择需考虑体积（嵌入 exe 后总体积 < 100MB）
- 与 Gateway 模式并行：本地推理走本地通道，云端推理走 Gateway

#### 3. OpenAI 兼容 API 服务端

**来源：** LM Studio（端口 1234）和 Ollama（端口 11434）均提供 OpenAI 兼容 REST API，使得 VSCode 插件、其他 App 可直连本地模型。

**现状：** AnyClaw 只做 Client（消费 Gateway API），不对外暴露服务端接口。

**吸收价值：**
- 暴露本地 API 端口 → VSCode 插件、浏览器扩展、其他桌面 App 可消费 AnyClaw 的 Agent 能力
- 生态扩展的关键基础设施——没有 API 就没有插件生态
- 与"本地推理引擎"配合：本地模型 + 本地 API = 完全离线的 AI 服务能力

**设计方向：**
- 可选开启本地 OpenAI 兼容 API（默认关闭，Settings 中开启）
- 端口可配置（默认 18800 以上，避免与 Gateway 18789 冲突）
- API 认证：本地 token，防止局域网内其他设备未授权访问
- 两种后端路由：本地推理引擎 / Gateway Agent → 用户选择

#### 4. RAG 文档聊天体验

**来源：** LM Studio 支持拖入文档直接对话（离线 RAG），体验流畅。

**现状：** AnyClaw §16.4 有本地知识库 V1（关键词检索 + 固定前缀截断拼接到上下文），但检索排序粗糙、上下文构建原始。

**吸收价值：**
- LM Studio 的文档 RAG 体验是当前本地 AI 的标杆——拖入即问，无需配置
- AnyClaw 的知识库 V1 还需要用户手动建立索引，门槛远高于 LM Studio
- RAG 是非编程用户（学生/职场）的核心使用场景之一

**设计方向（V2 升级）：**
- 聊天窗口支持拖入文档（PDF/Word/TXT/Markdown），自动解析并建立向量索引
- 检索排序升级：频次 + 首次命中位置 + 文件名命中 + 语义相似度综合评分
- 上下文构建升级：命中片段截取（而非固定前缀截断）
- 片段去重 + 总长度预算控制
- 文档管理面板：已导入文档列表、重建索引、删除

#### 5. SDK / API 文档

**来源：** LM Studio 提供 `lmstudio-python` 和 `lmstudio-js` 两个官方 SDK。

**现状：** AnyClaw 纯 GUI 应用，无 SDK、无 API 文档。

**吸收价值：**
- 有 SDK 意味着开发者可以二次集成 AnyClaw 的 Agent 能力
- 为未来生态扩展打基础（插件、脚本、自动化）

**设计方向：**
- 短期：完善 §3 OpenAI 兼容 API 的文档
- 中期：提供 Python/JS 薄封装 SDK
- 长期：建立开发者社区和插件市场

#### 6. Headless 模式

**来源：** LM Studio 提供 `llmster`（无 GUI 版本），适用于服务器/CI 环境。

**现状：** AnyClaw 是纯 GUI 应用，无服务端模式。

**吸收价值：**
- 适配 NAS、树莓派、云服务器等无显示器场景
- 与远程协作功能（§16）互补——Headless 实例作为远程 Agent 节点
- CI/CD 集成场景（开发者的自动化流水线）

**设计方向：**
- 提供命令行模式：`AnyClaw_LVGL.exe --headless --port 18800`
- 无 GUI 时提供 HTTP API + WebSocket
- 与远程协作功能结合：Headless 实例可通过 LM Link 类协议被 GUI 实例发现和连接

#### 7. 跨平台

**来源：** LM Studio（macOS/Windows/Linux）、Ollama（macOS/Windows/Linux）均三平台。

**现状：** AnyClaw 仅支持 Windows（MSYS2 编译）。虽有交叉编译（MinGW → Windows），但无 Linux/macOS 原生版本。

**吸收价值：**
- 用户群扩展：macOS 开发者、Linux 运维人员
- 与 LAN Chat 跨平台互通需求一致（§A.7 待定问题）
- LVGL 本身支持多平台，技术上可行

**设计方向：**
- 短期：Linux 原生编译支持（已有的 `build-linux.sh` 需完善）
- 中期：macOS 原生编译（需替换 SDL2 + Win32 API 调用）
- 注意：托盘、文件对话框、进程管理等模块有平台差异，需要抽象层

#### 11. 模型社区生态

**来源：** Ollama 的 `ollama pull` 一行命令完成模型下载+可用，模型库丰富。

**现状：** AnyClaw 的模型管理是下拉框选择 + 手动填 API Key，没有"搜索→下载→可用"的一键流程。

**吸收价值：**
- Ollama 的模型生态吸引力在于"开箱即用"——用户不需要理解 API/Provider/Key 的概念
- AnyClaw 当前的模型选择对新手不友好（需要理解 OpenRouter、Provider、API Key 的关系）
- 降低模型使用门槛是"简单"核心价值的直接体现

**设计方向：**
- 模型浏览器：GUI 内搜索/浏览可用模型（含 OpenRouter 云端 + 本地可下载模型）
- 一键流程：选中模型 → 自动检测 Provider → 引导填 Key → 可用
- 本地模型：类似 Ollama 的体验，选中即下载+加载+可用
- 模型推荐：根据用户硬件（RAM/GPU）推荐合适模型

