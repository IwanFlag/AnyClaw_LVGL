# AnyClaw LVGL - 产品需求文档 (PRD)

> 版本：v2.2.1 | 最后更新：2026-04-12
> 合并来源：PRD v2.2.0 + 逐章补充 + UI 布局 Wireframe

---
## 1. 产品概述

### 1.1 产品定位

AnyClaw 是面向**学生**和**职场用户**的 AI 桌面管家。兼具 **通用 AI 工作台**与 **AI 管家** 两大能力--让 AI 不仅能陪聊，还能直接帮你干活，执行过程全程可见。

三个关键词：
- **简单** - 开箱即用，无需深入理解底层架构
- **安全** - 数据不离开本地，不依赖第三方平台
- **稳定** - 本地运行，不随平台策略变化而失效

**产品公式：**
```
通用 AI 工作台（Chat + Work）+ OpenClaw（AI 管家）= AnyClaw
```

| 模式 | 角色 | 内容 |
|------|------|------|
| Chat 模式 | AI 管家 | 对话、提醒、查邮件、IM 集成、日常事务 |
| Work 模式 | 通用 AI 工作台 | AI 执行任务的可视化工作区--编程、写文档、数据分析、做 PPT、搜索整理，执行过程实时可见 |

**Work 模式不限于编程。** 学生写论文、职场做报告、开发者写代码--任务类型不同，但 AI 干活并展示执行过程的模式是通用的。

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

AnyClaw 自身成为 Agent 平台--直接管理 prompt、工具调用、会话上下文，同时也能管理外部 Agent（如 OpenClaw、Hermes 等）。Direct API 模式为架构预留，当前仅支持 Gateway 模式。

### 1.3 PRD 执行修订（2026-04-11）

为避免"状态冲突/范围漂移"，本次执行补充以下约束：

- **范围约束：** 仅执行正文 2.x；附录 A.* 只做背景说明，不作为 v2.0 交付门槛。
- **状态口径：** `✅ 已实现`=可运行可验证；`🔧 实施中`=有可用骨架但未闭环；`⏸️ 暂不实施`=明确排除项。
- **冲突消解：** 同一能力不得同时出现"P0 待实施"和"v2.0 不做"；如需拆分，拆成"基础能力/深化能力"两个编号。
- **验收口径：**
  - AIMODE-01：Chat/Work 双入口切换 + 配置持久化 + 请求侧模式注入；
  - PROACTIVE-01：启动触发 + 空闲触发 + 日终摘要（默认不改用户项目文件）；
  - WORK-01：v2.0 要求"步骤流 + 终端输出 + 文档预览 + Chat 联动"（通用工作台，不限编程），不含 v2.1 diff/表格；
  - LIC-01：免费版无需密钥 + 专业版 HMAC-SHA256 时间递增密钥 + 序列号递增检查 + machine-id 展示 + keygen 工具。

### 1.4 竞品环境（2026-04-11 更新）

OpenClaw 桌面客户端赛道已有多个免费竞品，AnyClaw 需在差异化和商业模式上做出调整：

| 产品 | 价格 | IM 集成 | 编程能力 | 权限系统 | 本地模型 |
|------|------|---------|---------|---------|---------|
| nexu | 免费开源 | ✅ 微信/飞书/Slack/Discord | ❌ | ❌ | ❌ |
| QClaw（腾讯） | 免费闭源 | ✅ 微信/飞书/钉钉 | ❌ | ❌ | ❌ |
| 724Claw | 免费 | ❌ | ❌ | ❌ | ❌ |
| Cursor | Freemium | ❌ | ✅ | ❌ | ❌ |
| **AnyClaw** | **Freemium** | **🔧 规划中** | **✅** | **✅** | **✅** |

**AnyClaw 差异化定位：** 免费客户端 + 通用 AI 工作台（不限编程）+ 权限系统 + 原生 GUI + Agent 主动行为。详见 §18 分发与增长。

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

**部署方式：** 双文件部署 - `AnyClaw_LVGL.exe` + `SDL2.dll` 放入同一目录，双击运行。计划后续支持单文件部署（Launcher 嵌入 SDL2.dll）。

**跨平台编译：**
- Windows：MSYS2 原生编译，产物到 `artifacts/windows-native/`
- Linux：MinGW-w64 交叉编译，产物到 `artifacts/linux-to-windows/`；原生编译到 `artifacts/linux-native/`
- 构建脚本：`build/windows/build-package.bat`、`build/linux/build-win.sh`、`build/linux/build-linux.sh`

**补充细节（代码实际实现）：**
- DPI 感知采用 Win32 `PER_MONITOR_AWARE_V2` 级别，在任何 SDL/GDI 调用之前声明
- SDL 强制软件渲染 `SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software")`
- 窗口默认尺寸为屏幕 75%×75%，上限为屏幕分辨率减去边距（宽-60px，高-80px）
- IME 支持通过 SDL hints 启用：`SDL_HINT_IME_SHOW_UI=1`、`SDL_HINT_IME_SUPPORT_EXTENDED_TEXT=1`
- `KEYBOARD_BUFFER_SIZE` 设为 256 以支持中文输入法多字节序列

### 2.2 实例互斥
- **功能编号：** INST-01 **优先级：** P0
- 通过 Windows Mutex (`Global\AnyClaw_LVGL_v2_SingleInstance`) 防止重复启动
- 检测到已有实例运行 → 弹出原生 MessageBox 提示 "Already Running"，退出程序
- 进程退出时自动释放 Mutex

**补充细节（代码实际实现）：**
- `APP_MUTEX_NAME` 为编译时常量
- 检测到 `ERROR_ALREADY_EXISTS` 时弹出原生 MessageBox 并退出
- 退出时调用 `release_mutex()` 自动释放
- 互斥检查在日志初始化之后、所有 UI 初始化之前执行

### 2.3 启动自检
- **功能编号：** SC-01 **优先级：** P0

**代码实际实现中存在两个独立模块：**

| 模块 | 职责 | 检查项数 | 自动修复 | 调用时机 |
|------|------|---------|---------|---------|
| SelfCheck（`selfcheck.h/.cpp`） | 快速启动检查 | 4 | 有限（npm cache clean） | 每次启动 |
| BootCheck（`boot_check.h/.cpp`） | 全面环境体检 | 9 | 有（`auto_fix_all`） | 手动触发/Work模式按钮 |

**SelfCheck 检查项：**

| 检查项 | 检测方式 | 自动修复 |
|--------|---------|---------|
| Node.js | `node --version` | 不可修复，弹窗提示安装 |
| npm | `npm --version` | 尝试 `npm cache clean --force` |
| 网络连通性 | HTTP GET google.com / openrouter.ai | 不可修复，提示检查网络 |
| 配置目录 | `%USERPROFILE%\.openclaw` 可写性 | 尝试 `create_directories` 创建 |

**BootCheck 全面检查（9 项）：** Node.js 版本 / npm / OpenClaw / Gateway 端口 / 配置目录 / 工作区 / 磁盘空间 / 网络 / SDL2.dll

- `selfcheck_run_full()` 内部调用 `BootCheckManager::run_and_fix()` 并返回格式化报告
- `check_nodejs_version()` 要求 Node.js >= 22.14
- `check_sdl2dll()` 检查同目录下 SDL2.dll 是否存在
- 所有检查项均经过权限系统（`perm_check_exec`）前置验证
- 检查结果写入日志（`[SelfCheck]` 前缀）
- Node.js 缺失 → 显示 LVGL 弹窗阻断启动（`g_startup_error`）
- 当 `Node/npm/OpenClaw` 缺失时，即使 `wizard_completed=true` 也强制打开向导

### 2.4 首次启动向导
- **功能编号：** WIZ-01 **优先级：** P1 **状态：** ✅ 已实现（5步）/ 🔧 扩展中（6步）
- 6 步引导（全屏遮罩 + 居中弹窗，深色风格 #1E2530 圆角 12）：
  1. **语言选择** - 中文/English 两个按钮，默认 EN
  2. **OpenClaw 检测** - 自动检测 Node.js/npm/Network/OpenClaw/Gateway，显示状态 LED + 版本号；支持向导内安装（Auto Install / 本地包）与进度显示
  3. **模型 & API 密钥** - 模型 dropdown（由 model_manager 动态加载）+ Provider 动态提示 + API Key 密码模式输入框
  4. **本地模型（可选）** - Gemma 4 本地安装开关 + 2B/9B/27B 勾选，可 Skip
  5. **连接 IM（可选）** - 引导配置 OpenClaw 渠道，支持 Telegram / Discord / WhatsApp；Skip 后可在 Settings 中随时配置
  6. **个人信息 & 确认** - 昵称输入 + 时区 dropdown（26 个时区，默认 UTC+8）+ 配置摘要 + "Get Started" 按钮

**IM 连接步骤详情：**
- 渠道列表：Telegram（Bot Token）、Discord（Bot Token）、WhatsApp（扫码）
- 每个渠道显示"已连接"/"未配置"状态
- 点击"配置"弹出引导：输入 Token / 扫码绑定
- 全部 Skip 不影响正常使用，桌面端 Chat 模式照常可用
- **价值：** 让用户在手机上随时远程指挥 AnyClaw，这是 nexu/QClaw 的核心吸引力

**交互细节：**
- 顶部步骤指示器（Step 1/6），底部上一步/下一步按钮
- 首次启动自动弹出（检查 config.json `wizard_completed` 字段）
- 向导增加全程可见"退出向导"按钮，接入统一退出逻辑
- Gemma 和 IM 步骤均增加显式 Skip 操作，跳过不影响正常使用
- **用户画像写入 USER.md**：路径 `%USERPROFILE%\.openclaw\workspace\USER.md`
- **General Tab 底部提供 "Reconfigure Wizard" 按钮**，可随时重新触发
- 默认 AI 交互模式 = 自主模式（不在此步暴露选择）

**补充细节（代码实际实现）：**
- 向导每一步的 Skip 操作均不影响后续步骤
- Gemma 安装步骤中 `model_mask` 使用 bitmask：bit0=E4B, bit1=26B MoE
- 向导中模型 dropdown 由 `model_manager_init()` 动态加载，默认包含 21 个模型（含 OpenRouter 免费模型）
- `app_full_setup()` 串联 install → init → configure → verify 全流程
- 向导支持取消操作：`app_request_setup_cancel()` / `app_is_setup_cancelled()`

#### [§2.4] 向导每一步的 UI 布局

**通用框架**：
```
  ┌──────────────────────────────────────────────────────────────┐
  │                    全屏黑色遮罩 (深色背景)                     │
  │                                                              │
  │         ┌────────────────────────────────────────────┐       │
  │         │  ●──●──○──○──○  Step 1/5                  │       │ ← 步骤指示器
  │         │  ─────────────────────────────────────────  │       │
  │         │                                            │       │
  │         │         ┌────────────────────────┐         │       │
  │         │         │                        │         │       │ ← 内容区
  │         │         │    Step 内容区域        │         │       │
  │         │         │                        │         │       │
  │         │         └────────────────────────┘         │       │
  │         │                                            │       │
  │         │  ─────────────────────────────────────────  │       │
  │         │  [ 退出向导 ]      [ < Back ]  [ Next > ]  │       │ ← 底部按钮
  │         └────────────────────────────────────────────┘       │
  │                                                              │
  └──────────────────────────────────────────────────────────────┘
```

**通用框架详细说明**:
- **弹窗**: 居中, 背景 `#1E2530`, 圆角 12px, 内边距 20px
- **遮罩**: 全屏深色覆盖
- **步骤指示器**: 5 步圆点 (● 当前已完成, ○ 未完成) + "Step N/5" 文字
- **底部按钮行**: flex row, `LV_FLEX_ALIGN_SPACE_BETWEEN`
  - "退出向导": `btn_secondary`
  - "< Back": `btn_secondary`, 第一步时禁用
  - "Next >": `btn_action (#3B82F6)`, 最后一步变为绿色 "Get Started" `#22C55E`

**Step 1: Language** — 中文/English 两个按钮 140×44px，选中 `#3B82F6`，未选 `#32374B`

**Step 2: OpenClaw Detection** — 环境检测状态 LED（绿色 ✓ / 红色 ✗），未安装时提供下载/离线安装按钮

**Step 3: Model & API Key** — 模型 Dropdown + Provider 自动显示 + API Key 密码模式输入 + 获取链接提示

**Step 4: Local Models (Optional)** — Enable Switch + 2B/9B/27B Checkbox + Skip 按钮，`model_mask` 使用 bitmask

**Step 5: Profile & Confirm** — 昵称输入 + 时区 Dropdown + 配置摘要 + 绿色 "Get Started" 按钮

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
- 自动创建目录结构和初始化文件（详见第 9 章）

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

**补充细节（代码实际实现）：**
- 标题栏高度 `TITLE_BAR_H_BASE=48px`
- 左侧面板默认宽度 `LEFT_PANEL_DEFAULT_W=240px`
- 分隔条宽度 `SPLITTER_W_BASE=6px`
- 面板最小宽度 `PANEL_MIN_W=200px`
- 所有尺寸常量集中在 `app_config.h`，支持 DPI 缩放
- DPI 缩放比例启动时检测，保存到 `config.json` 的 `dpi_scale` 字段

#### [§3] 主界面完整布局图

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│ ◉ garlic_48.png  AnyClaw LVGL      [  Chat  |  Work  ]  [⚙设置] [-] [□] [X] │  ← TITLE_H = 48px
├─────────────────────────────────────────────────────────────────────────────────┤  ← separator: 1px #373C55
│                                                                                 │
│  ┌─────────────────────┐   ╎   ┌─────────────────────────────────────────────┐  │
│  │ 🟢 Gateway: Ready   │   ╎   │                                             │  │
│  │    Port: 18789       │   ╎   │                                             │  │
│  ├─────────────────────┤   ╎   │                                             │  │
│  │ Sessions (2)    [+Add]│  ╎   │            右侧面板内容区                    │  │
│  │  ● Chat session      │   ╎   │        (Chat 模式 / Work 模式)              │  │
│  │    [✕ Abort]         │   ╎   │                                             │  │
│  │  ● Work task         │   ╎   │                                             │  │
│  │    [✕ Abort]         │   ╎   │                                             │  │
│  ├─────────────────────┤   ╎   │                                             │  │
│  │ Cron Tasks      [+] │   ╎   │                                             │  │
│  │  (empty placeholder) │   ╎   │                                             │  │
│  │                      │   ╎   │                                             │  │
│  └─────────────────────┘   ╎   └─────────────────────────────────────────────┘  │
│                             ╎                                                   │
│  ← LEFT_PANEL_W=240px →   6px  ← RIGHT_PANEL_W = WIN_W - 240 - 12*2 - 6 →    │
│                                                                                 │
└─────────────────────────────────────────────────────────────────────────────────┘
```

**标题栏 (Title Bar)**:
- 尺寸: `WIN_W × TITLE_H (48px)`, 背景色: `panel (#232838)`
- 支持拖拽移动窗口（通过 Win32 `WM_NCHITTEST` 子类化实现，返回 `HTCAPTION`）
- 子元素（从左到右）: 品牌图标 + 标题 + 模式切换栏(110px) + 设置按钮(68px) + 窗口控制按钮组
- 窗口控制按钮区域（右侧约 120px）从拖拽区域排除，由 `app_update_btn_exclude()` 动态更新
- 双击标题栏通过 `WM_NCLBUTTONDBLCLK` 消息处理最大化/还原，SDL 层另有 400ms 双击检测 fallback

**左侧面板 (Left Panel)**:
- 尺寸: `LEFT_PANEL_W (240px) × PANEL_H`, 背景色: `panel (#232838)`, 边框 1px
- 内部: 状态行 → 分隔线 → Session 列表(最多12项) → Cron 任务区

**分隔条 (Splitter)**:
- 尺寸: `SPLITTER_W (6px) × PANEL_H`, 支持拖拽调整面板宽度
- 悬停时变色为 `#82AAFF`, 鼠标变为水平调整光标

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

**补充细节（代码实际实现）：**
- `tray.h/.cpp` 使用 Win32 `NOTIFYICONDATAW` 实现，托盘菜单为 owner-drawn，支持深色主题
- 托盘图标从 EXE 内嵌资源 `IDI_ICON1` 加载，fallback 到 PNG 文件加载
- 托盘状态由后台健康线程通过 `tray_set_state()` 设置
- 主线程与后台线程间通过 `InterlockedCompareExchange` 原子操作同步托盘状态
- 菜单项使用 `MenuItemData` 结构体管理，LED 颜色指示器嵌入菜单文本

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
- 欢迎语：龙虾要吃蒜蓉的 🧄🦞 - 你的AI助手已就位！
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

**补充细节（代码实际实现）：**
- `garlic_dock.h/.cpp` 使用纯 Win32 实现（不用 LVGL/SDL）
- 大蒜头尺寸 80×110 px，身体 64px，嫩芽 72px
- 贴边检测阈值 `EDGE_SNAP_PX=20` 像素
- 通过 `stb_image` 加载 PNG 纹理（兼容 Wine，不依赖 WIC/GDI+）
- PNG 图片加载路径依次尝试 `assets\garlic_48.png`、`..\assets\garlic_48.png`、`..\..\assets\garlic_48.png`

### 3.6 图标系统升级
- **功能编号：** ICON-02 **优先级：** P2 **状态：** ✅ 已实现
- 托盘图标尺寸升级到 96px（LANCZOS 上采样）
- 窗口标题栏图标升级到 96/64/32（WM_SETICON + SetClassLongPtr）

---


## 4. Chat 模式

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
  - `stream_timer_cb` 使用 LVGL timer，每 50ms 刷新一次
  - 流式 buffer 原子操作：`InterlockedCompareExchange` 同步新数据标志
  - `CHAT_STREAM_CHUNK=3`：每次追加 3 个字符
  - 调用 `render_markdown_to_label` 实时渲染 Markdown 格式
  - 流式过程中 `chat_scroll_to_bottom()` 软滚（40px 容差，用户回看历史时不强制拉回）

**流式渲染完整状态机**：
```
用户发送 → chat_send_cb() → submit_prompt_to_chat()
    ↓
chat_start_stream() 启动 SSE 线程
    ↓
stream_timer_cb() 每 50ms 检查:
    ├── 新数据？ → stream_buf_copy → render_markdown_to_label → chat_scroll_to_bottom
    ├── 超时？（30s idle / 45s total） → 显示"⚠️ AI 回复超时，请重试。"
    └── 完成？ → 最终渲染 → 保存到 chat_history → chat_force_scroll_bottom()
```

- 对话历史滚动：
  - 新消息发送 → `chat_force_scroll_bottom()` 强制滚到底
  - 流式输出中 → 软滚（`SCROLL_BOTTOM_TOLERANCE=40px`）
  - 手动滚动 → 滚轮仅在 `chat_cont` 和 `chat_input` 上生效
- 流式结束时如果处于 Work 模式，追加 `work_append_md_block("AI", snapshot)`
- 聊天历史最大条目 `CHAT_HISTORY_MAX=4096`（buffer 大小）
- 持久化上限 `CHAT_PERSIST_MAX=50` 条

#### [§4] Chat 模式完整布局图

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│ ◉ AnyClaw LVGL              [  Chat  |  Work  ]  [⚙设置] [-] [□] [X]          │ 48px
├─────────────────────────────────────────────────────────────────────────────────┤ 1px
│                                                                                 │
│  ┌─────────────────────┐   ╎   ┌─────────────────────────────────────────────┐  │
│  │ 🟢 Gateway: Ready   │   ╎   │                                             │  │
│  │ 📁 个人助手          │   ╎   │  ┌───────────────────────────────────────┐  │  │
│  ├─────────────────────┤   ╎   │  │ 🔵 User                        12:30 │  │  │
│  │ Sessions (1)   [+Add]│  ╎   │  │ ┌─────────────────────────────┐     │  │  │
│  │  ● Chat session      │   ╎   │  │ │ 帮我查看一下系统状态          │     │  │  │
│  │    [✕]               │   ╎   │  │ └─────────────────────────────┘     │  │  │
│  ├─────────────────────┤   ╎   │  │                                     │  │  │
│  │ Cron Tasks      [+] │   ╎   │  │        ┌──────────────────────────┐ │  │  │
│  │  (empty)             │   ╎   │  │ 🦞 AI  │ 系统状态良好：           │ │  │  │
│  └─────────────────────┘   ╎   │  │ 12:30  │ · Gateway 运行中        │ │  │  │
│                             ╎   │  │        │ · 内存使用 45%          │ │  │  │
│                             ╎   │  │        └──────────────────────────┘ │  │  │
│                             ╎   │  └───────────────────────────────────────┘  │  │
│                             ╎   │                                             │  │
│                             ╎   │  ┌───────────────────────────────────────┐  │  │
│                             ╎   │  │ [🎤] [📎] │                      │ ▶ │  │  │
│                             ╎   │  │           │  输入消息...          │   │  │  │
│                             ╎   │  └───────────────────────────────────────┘  │  │
│                             ╎   └─────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

**左面板工作区信息行**：
- 显示当前工作区名称（从 `workspace_get_name()` 读取）
- 格式：📁 + 工作区名（如 "个人助手"）
- 工作区健康状态异常时图标变为 ⚠️（磁盘不足/目录不可达/被其他实例占用）
- 点击工作区名可弹出工作区详情（路径、磁盘空间、健康状态）

**消息气泡样式**:
- **用户气泡**: 靠右对齐, 最大宽度 70%, 背景色 `btn_action (#4070C0)`, 白色文字, 圆角 10px, 头像 32×32px
- **AI 气泡**: 靠左对齐, 最大宽度 70%, 背景色 `panel (#232838)`, 文字色 `text (#E0E0E0)`, 圆角 10px, 头像 24×24px
- 消息间距: `CHAT_GAP (6px)`, 消息边距: `CHAT_MSG_MARGIN (8px)`

**输入区**:
- 高度: `CHAT_INPUT_DEFAULT_H (96px)`, 最小 96px, 背景色 `input_bg (#191C26)`
- 子元素: 语音按钮(🎤) + 附件按钮(📎) + 多行输入框 + 发送按钮(▶)
- 输入框支持中文 IME 输入

### 4.4 Markdown 渲染
- **功能编号：** MD-01 **优先级：** P1
- 代码块、加粗、列表等基础格式
- CJK 逐字换行（LV_TEXT_FLAG_BREAK_ALL）

### 4.5 消息操作
- **功能编号：** MO-01 **优先级：** P1 **状态：** ✅ 已实现
- 消息文字可选中（鼠标拖拽选区）-- `make_label_selectable(lbl)` 为 label 添加 CLICKABLE 标志 + 事件回调
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
- **功能编号：** SHARE-01 - 文件/图片/目录发送与可点击打开
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
- 超时时如果 buffer 不为空，保留已接收的内容显示

**补充细节（代码实际实现）：**
- `g_stream_last_data_tick`：最后收到数据的时间戳
- `g_stream_start_tick`：流开始的时间戳
- 判断条件：`idle_ms > 30000 || total_ms > 45000`
- 超时时调用 `set_ai_next_step("Timeout, please retry")` 和 `append_ai_script_log("[error] stream timeout")`

### 4.10 会话管理
- **功能编号：** SM-01 **优先级：** P1 **状态：** ✅ 已实现
- 活跃会话列表查看（渠道/Agent/时长）
- 单个会话终止（Abort 按钮）
- 所有会话重置

**补充细节（代码实际实现）：**

**SessionInfo 数据结构**：
```cpp
struct SessionInfo {
    std::string key;           // "agent:main:webchat:main"
    std::string agentId;       // "main"
    std::string channel;       // "webchat", "telegram", "discord", etc.
    std::string chatType;      // "direct" or "group"
    long long ageMs;           // 毫秒级活跃间隔
    long long tokenCount;      // 近似 token 使用量
    bool isSubAgent;           // 是否子代理
    bool isCron;               // 是否定时任务触发
};
```

- 支持的渠道显示名称映射：webchat→WebChat, telegram→Telegram, discord→Discord, feishu→飞书, wechat→微信, signal→Signal, slack→Slack, whatsapp→WhatsApp, cli→CLI, line→LINE
- 活跃判定阈值：`SESSION_ACTIVE_AGE_MS=300000`（5 分钟）
- 单个会话终止命令：`openclaw gateway call sessions.reset --json -d '{"sessionKey":"..."}'`
- 所有会话重置命令：`openclaw gateway call sessions.reset --json`
- Session 刷新操作经过 `TraceSpan` 记录追踪

---
### 4.11 Agent 主动行为（管家的核心差异化）
- **功能编号：** PROACTIVE-01 **优先级：** P0 **状态：** ✅ 已实现（V1）

Chat 模式下的 AI 管家不应该"你问它才答"，而应该**主动帮用户做事**。

**基本原则：读和整理自己做，写和发送必须问。**

#### 能力分级

| 类别 | 示例 | 用户同意？ |
|------|------|----------|
| **整理型** | 整理工作区文件、清理过期记忆、更新 MEMORY.md、备份配置 | ❌ 自己做，做完汇报 |
| **监控型** | 检查文件变化、API Key 有效期、磁盘空间、配置一致性 | ❌ 自己查，有事才通知 |
| **应用操作型** | 读邮件、查日程、搜索文件（通过应用授权 §10.8） | ⚠️ 做之前问 |
| **执行型** | 发邮件、修改外部文件、执行命令 | ⚠️ 做之前问 |
| **破坏型** | 删除文件、修改系统配置、发送外部消息 | ✅ 必须问 |

#### 通知分级

| 级别 | 方式 | 示例 |
|------|------|------|
| **静默** | 不通知，写日志 | 整理了 MEMORY.md、清理了临时文件 |
| **摘要** | 固定时间一次性汇总 | "今天帮你做了 3 件事：整理了 X、清理了 Y" |
| **提醒** | 托盘气泡 | "日历提醒：20 分钟后有会议" |
| **警报** | 弹窗 + 声音 | "磁盘空间不足 100MB" |

#### 默认预设（开箱即用）

| 预设 | 触发 | 默认状态 | 前置条件 |
|------|------|---------|---------|
| 启动体检 | AnyClaw 启动时 | ✅ 默认开启 | 本地检测 |
| 空闲整理 | 用户 30 分钟无操作 | ✅ 默认开启 | 无 |
| 记忆维护 | 每天 18:00（OpenClaw Cron） | ✅ 默认开启 | MEMORY.md 存在 |
| 日终总结 | 每天 23:00（OpenClaw Cron） | ✅ 默认开启 | 无 |
| 日程提醒 | 每天 09:00 | ⏳ 应用授权后自动激活 | 日历应用已授权 |
| 邮件摘要 | 每天 12:00 | ⏳ 应用授权后自动激活 | 邮件应用已授权 |

#### 当前实现状态（V1）
- 启动触发已落地：启动后执行体检日志与主动行为初始化
- 空闲触发已落地：基于 Win32 空闲检测触发"自维护整理"
- 日终摘要已落地：23:00 触发一次性摘要日志
- 约束已落实：默认不修改用户项目文件，仅做自维护与状态汇报

#### 触发方式

| 触发 | 场景 | 实现 |
|------|------|------|
| **定时** | 记忆维护、日终总结 | v2.0 以 AnyClaw 本地调度先落地，Cron 深化在后续版本 |
| **空闲** | 用户 30 分钟没操作 → Agent 趁空闲整理 | AnyClaw Win32 空闲检测 |
| **启动** | AnyClaw 启动时自动体检 | AnyClaw 启动流程 |
| **事件** | 文件变化、应用授权变更、配置不一致 | v2.0 不做 |

**空闲整理范围（仅 AI 自身，不动用户文件）：**
- 整理/优化 MEMORY.md、memory 日志
- 自省缺陷、修正行为（参考 self-improvement skill 逻辑）
- 技能梳理与优化
- 不碰用户工作区文件


---

## 5. Work 模式——通用 AI 工作台
- **功能编号：** WORK-01 **优先级：** P0 **状态：** 🔧 实施中

Work 模式不限于编程，是 **AI 执行任务的通用可视化工作区**。用户在 Chat 里发任务，Work 面板展示 AI 每步做了什么、输出是什么--写代码、写文档、做数据分析、搜资料、处理文件，全部覆盖。

#### 5.1 核心组件

```
Work 面板
├── 步骤流（Step Stream）
│   └── 实时展示 Agent 每步操作：操作类型图标 + 描述 + 状态
│
├── 输出区域（Output Area）
│   └── 按任务类型自动选择渲染器（见 §5.2）
│
└── Chat 面板（已有）
    └── Agent 对话 + 工具输出
```

| 组件 | 能力 | 说明 |
|------|------|------|
| 步骤流 | 实时展示 Agent 每步操作 | 解析 SSE 工具调用事件，显示读了哪个文件、改了什么、跑了什么命令 |
| 输出区域 | 多类型渲染器 | 根据任务类型自动选择渲染方式（终端/预览/表格/图表/搜索结果） |
| Chat 面板 | 对话 + 工具输出 | Agent 读代码、提建议、跑命令 |

**补充细节（代码实际实现）：**
- Work 面板包含任务提示输入框（`mode_ta_work_prompt`）、发送按钮回调 `work_send_cb`、Boot Check 按钮 `work_boot_check_cb`
- 输出区域使用 `work_append_md_block` 追加内容
- Boot Check 运行在独立线程，通过 `ui_progress_begin/update/finish` 桥接进度到 UI
- Boot Check 结果导出为 `%APPDATA%\AnyClaw_LVGL\bootcheck-last.md`

**Work 模式发送流程**：
```
work_send_cb()
    ├── 保存 prompt 到 g_work_last_prompt
    ├── 设置 g_work_waiting_ai = true
    ├── work_append_md_block("User", text) 显示用户输入
    ├── set_ai_next_step("Work mode: executing task")
    ├── submit_prompt_to_chat(text) 发送到 Gateway
    └── 清空输入框
```

#### 5.2 任务类型与渲染映射

| 任务类型 | SSE 事件特征 | 渲染器 | 输出示例 | 版本 |
|----------|-------------|--------|---------|------|
| 终端 | `exec` 命令 | TerminalRenderer | 命令输出 | v2.0（已有） |
| 文档 | `write_file` .md/.txt | PreviewRenderer | 渲染后的 Markdown | v2.0 |
| 代码 | `write_file` .py/.js/.c 等 | PreviewRenderer → DiffRenderer | 代码高亮 + 修改对比 | v2.0 → v2.1 |
| 数据 | `exec` 含 python/pandas/csv | TableRenderer | 表格 + 统计摘要 | v2.1 |
| 搜索 | `web_fetch` / `browser` | WebRenderer | 结果卡片列表 | v2.2 |
| 文件 | 批量 `read` / `write_file` | FileRenderer | 操作日志 + 变更列表 | v2.2 |

| 阶段 | 新增渲染器 | 覆盖场景 |
|------|-----------|---------|
| v2.0 | TerminalRenderer（已有）+ PreviewRenderer | 终端输出 + Markdown/文档预览 |
| v2.1 | DiffRenderer + TableRenderer | 代码对比 + 数据表格 |
| v2.2 | ChartRenderer + WebRenderer + FileRenderer | 图表 + 搜索 + 文件操作 |

#### 5.3 渲染器接口设计

```c
typedef struct {
    const char* name;                          // "terminal", "preview", "diff", "table", "web", "file"
    bool (*can_handle)(SSEEvent* event);
    lv_obj_t* (*create)(lv_obj_t* parent);
    void (*update)(lv_obj_t* container, SSEEvent* event);
    void (*reset)(lv_obj_t* container);
} OutputRenderer;
```

#### 5.4 v2.0 布局

```
┌──────────────────────────────┬──────┐
│  [Chat] [Work]      ⚙️       │      │
├──────────────────────────────┤ Chat │
│                              │ 面板  │
│    AI 操作步骤流              │      │
│    （实时展示每步操作）        │      │
│                              │      │
├──────────────────────────────┤      │
│        输出面板               │      │
│   （终端/文档预览，自动切换）  │      │
└──────────────────────────────┴──────┘
```

#### [§5] Work 模式详细布局图

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│ ◉ AnyClaw LVGL              [ Chat  |  Work  ]  [⚙设置] [-] [□] [X]           │ 48px
├─────────────────────────────────────────────────────────────────────────────────┤ 1px
│                                                                                 │
│  ┌─────────────────────┐   ╎   ┌──────────────────────────┬──────────────────┐  │
│  │ 🟢 Gateway: Ready   │   ╎   │                          │                  │  │
│  │ 📁 开发项目          │   ╎   │   AI 操作步骤流          │   Chat 面板     │  │
│  ├─────────────────────┤   ╎   │   (Step Stream)          │   (缩小版)      │  │
│  │ Sessions (2)   [+Add]│  ╎   │                          │                  │  │
│  │  ● Main chat    [✕] │   ╎   │  📄 读取 config.json  ✓  │  🦞 正在帮你    │  │
│  │  ● Work task    [✕] │   ╎   │  ✏️ 写入 settings.js  ✓  │     修改配置...  │  │
│  ├─────────────────────┤   ╎   │  ⚡ npm install     ●    │                  │  │
│  │ Cron Tasks      [+] │   ╎   │     (运行中...)          │  🔵 已修改了    │  │
│  │  (empty)             │   ╎   │                          │     3 个文件     │  │
│  └─────────────────────┘   ╎   ├──────────────────────────┤                  │  │
│                             ╎   │                          │                  │  │
│                             ╎   │   输出面板               │  ┌────────────┐  │  │
│                             ╎   │   (Output Area)          │  │ 输入消息.. │  │  │
│                             ╎   │   自动切换渲染器          │  │        [▶] │  │  │
│                             ╎   │                          │  └────────────┘  │  │
│                             ╎   │  ┌────────────────────┐  │                  │  │
│                             ╎   │  │ $ npm install      │  │                  │  │
│                             ╎   │  │ added 234 packages │  │                  │  │
│                             ╎   │  │ in 12.3s           │  │                  │  │
│                             ╎   │  └────────────────────┘  │                  │  │
│                             ╎   └──────────────────────────┴──────────────────┘  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

**Work 模式布局结构**（主次分栏）：

| 区域 | 位置 | 尺寸 | 说明 |
|------|------|------|------|
| 左面板 | 左侧 | 240px（同 Chat 模式） | Gateway 状态 + 工作区信息 + Session 列表 + Cron |
| 步骤流 | 右侧上部 | 剩余宽度 × 55% | 实时展示 Agent 每步操作的 StepCard 列表 |
| 输出面板 | 右侧下部 | 剩余宽度 × 45% | 按任务类型自动切换渲染器（终端/预览/表格） |
| Chat 面板 | 右侧固定 | 320px（可折叠至 40px） | Agent 对话 + 工具输出摘要 + 输入框 |

**关键交互细节**：
- 步骤流和输出面板上下分栏，中间有可拖拽分隔条（6px）
- Chat 面板固定在右侧，宽度 320px，最小可折叠到 40px（只剩一个图标按钮）
- 折叠/展开：点击 Chat 面板顶部的 `◀` / `▶` 按钮
- 步骤流区域可垂直滚动，最新的 StepCard 自动滚到可见区域
- 输出面板在无任务时显示空状态引导文字："在 Chat 中发送任务，AI 的操作过程将在这里实时展示"
- 步骤流中的写操作 StepCard 带 [接受][拒绝][回退] 按钮，点击后输出面板同步更新
- 当前工作区路径在左面板顶部显示，Agent 操作的文件路径会与工作区路径做对比，超出工作区边界的路径标红警示

#### [§5] 步骤流 StepCard 样式

```
┌─────────────────────────────────────────────────────────┐
│ ┌─────────────────────────────────────────────────────┐ │
│ │  📄  读取 config.json              ● 进行中         │ │ ← 32px 高
│ │       输出摘要 (可折叠)                              │ │
│ └─────────────────────────────────────────────────────┘ │
│                                                         │
│ ┌─────────────────────────────────────────────────────┐ │
│ │  ✏️  写入 settings.json          ✓ 已完成           │ │
│ │       [ 接受 ] [ 拒绝 ] [ 回退 ]                     │ │ ← 写操作特有
│ └─────────────────────────────────────────────────────┘ │
│                                                         │
│ ┌─────────────────────────────────────────────────────┐ │
│ │  ⚡  执行 npm install              ✗ 失败           │ │
│ │       error: ENOENT package.json                    │ │
│ │       [ 重试 ]                                       │ │
│ └─────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────┘
```

**操作类型图标**: 读文件📄 / 写文件✏️ / 执行命令⚡ / 搜索🔍
**状态指示器**: 进行中(绿●) / 已完成(绿✓) / 失败(红✗) / 等待确认(蓝?) / 检测中(黄旋)
- 左侧面板 Task 列表最多 12 个任务项 (`MAX_TASK_WIDGETS=12`)
- 每项 32px 高 + 12px 间距, 支持 Hover 弹出详情 Tooltip (320px 宽)
- Abort 按钮: 28×22px, 背景 `#B43232`

#### 5.5 v2.1：Diff 可视化 + 数据表格

| 新增能力 | 说明 |
|---------|------|
| Agent 代码编辑 | Chat 面板里 Agent 直接给出代码修改 |
| Diff 可视化 | 修改前后对比，绿色（新增）红色（删除）高亮 |
| 接受/拒绝 | 用户点击"接受"→ 自动写入文件；点击"拒绝"→ 丢弃 |
| 编辑历史 | 撤销 Agent 的修改，回退到之前版本 |
| TableRenderer | CSV/Excel 数据表格展示 + 基础统计 |

#### 5.6 v3.0：全自主执行

| 新增能力 | 说明 |
|---------|------|
| 自主规划 | Agent 自主拆解任务步骤 |
| 执行步骤流 | 用户看到 Agent 每一步操作的实时展示 |
| 逐步批准 | 每步可单独批准/拒绝，或全部自动通过 |
| 终端验证 | Agent 自动跑测试/编译，根据输出迭代修改 |
| WebRenderer | 搜索结果可视化 |

**与 §7 AI 交互模式联动：** 自主模式 = Agent 自动执行全部步骤；Ask 模式 = 每步询问；Plan 模式 = 先出方案讨论再执行

---


## 6. 模式切换与 Agent 行为

### 6.1 界面模式（Chat / Work）
- **功能编号：** MODE-01 **优先级：** P1 **状态：** 🔧 实施中

AnyClaw 有两种核心模式，Voice 不是独立模式，而是 Chat 模式中的语音输入方式。

| 模式 | 角色 | 内容 |
|------|------|------|
| **Chat 模式** | AI 管家 | 文本对话 + 语音输入 + 文件发送 + IM 集成 + 知识引用 + 日常事务 |
| **Work 模式** | 通用 AI 工作台 | AI 执行任务的可视化工作区--步骤流 + 多类型输出渲染 + Chat |

**Voice 输入：** Chat 模式中的语音输入按钮（输入区上传按钮左侧），点击后语音转文字填入输入框，不是独立模式。

### 6.2 布局设计

**一个 Agent，两种模式界面，上下文完全互通。** Chat 和 Work 共享同一个 Agent 的记忆、会话、工具调用。

- Chat 模式：对话是主角，全屏展示
- Work 模式：AI 执行过程是主角，步骤流 + 输出渲染占主区域，Chat 面板缩到侧面
- 顶栏单按钮切换：Chat ↔ Work
- 切换不丢上下文
- 模式切换延迟目标：< 150ms

**补充细节（代码实际实现）：**
- Chat 和 Work 模式切换不丢失上下文，共享 `chat_history` buffer
- `set_ai_next_step()` 和 `append_ai_script_log()` 是 Work 模式的状态更新入口

### 6.3 模式与 Agent 行为

| 模式 | Agent 侧重 | 暴露工具 |
|------|-----------|---------|
| Chat | 管家人格（友好、主动、关注日常事务） | 邮件/日历/文件搜索/IM/定时任务 |
| Work | 执行人格（严谨、高效、关注任务质量） | 代码/终端/文件操作/搜索/数据分析 |

- Agent 人格由 SOUL.md 统一定义，模式只影响 prompt 侧重
- MEMORY.md 跨模式共享

---


## 7. AI 交互模式（自主 / Ask / Plan）

- **功能编号：** AIMODE-01 **优先级：** P0 **状态：** ✅ 已实现（V1）

### 7.1 模式定义

| 模式 | 含义 | 关键词 |
|------|------|--------|
| **自主模式**（默认） | AI 全自主判断与执行，不打断 | 自主 |
| **Ask 模式** | 自主执行，遇到决策点弹出 ask_feedback 弹窗 | 临界询问 |
| **Plan 模式** | 先出完整方案与用户讨论，确认后自动执行 | 先谋后断 |

### 7.2 模式切换与持久化
- 模式切换入口：Chat 面板和 Work 面板内均可快捷切换，即时生效
- 默认模式：自主模式
- 模式值持久化到 config.json（`ai_interaction_mode` 字段）
- 请求侧已按模式注入 system prompt（Ask/Plan 有不同约束）
- ask_feedback 模态弹窗与"操作拦截兜底"归入 v2.1 深化

### 7.3 与权限系统的关系

| 模式 | 权限需求 | 说明 |
|------|---------|------|
| 自主模式 | 完整权限体系兜底 | AI 全自主，需要权限系统严格约束 |
| Ask 模式 | 决策暂停提供安全保障 | 关键操作需用户确认，天然更安全 |
| Plan 模式 | 确认后执行 | 用户在执行前已审核方案，风险可控 |

### 7.4 与 UI 控制权模式的关系

|  | 用户控制 AnyClaw | AI 控制 AnyClaw |
|---|---|---|
| **自主模式** | 用户发任务 → AI 自主完成 | AI 自主操作 AnyClaw 执行任务 |
| **Ask 模式** | 用户发任务 → 遇到决策弹 ask_feedback 弹窗 | AI 操作 AnyClaw → 遇到决策弹 ask_feedback 弹窗 |
| **Plan 模式** | 用户发任务 → AI 出方案讨论 → 确认后执行 | AI 出方案讨论 → 确认后操作 AnyClaw 执行 |

### 7.5 决策交互机制（ask_feedback 弹窗，v2.1 深化）

**触发方式：双保险**

| 触发源 | 机制 | 说明 |
|--------|------|------|
| LLM 主动标记 | LLM 回复中嵌入结构化 JSON `{"type":"ask_feedback",...}` | Ask 模式下 system prompt 约束 |
| 操作拦截（兜底） | Agent 尝试 write_file/delete_file/exec/send_message 时自动拦截 | v2.1 实装 |

**弹窗 UI：** 模态弹窗，AI 建议高亮，预设选项按钮 + 自由输入框兜底

### 7.6 消息协议

**LLM → AnyClaw：**
```json
// Ask 模式
{"type":"ask_feedback","reason":"...","suggestion":"...","options":["..."]}

// Plan 模式
{"type":"plan","steps":["..."],"risk":"...","est_time":"..."}
```

**AnyClaw → LLM：**
```json
{"type":"feedback","answer":"..."}
{"type":"plan_confirm"}
{"type":"plan_modify","instruction":"..."}
```

---


## 8. AnyClaw UI 控制权模式（用户 / AI）

- **功能编号：** CTRL-01 **优先级：** P0 **状态：** 🔧 实施中

### 8.1 模式定义

| 模式 | 操作者 | 说明 |
|------|--------|------|
| **用户模式**（默认） | 人类用户 | 用户手动操作鼠标键盘，AI 仅建议与辅助 |
| **AI 模式** | Agent | Agent 模拟用户操作鼠标键盘来控制 AnyClaw UI |

**核心约束：** AI 拿到 UI 控制权后，必须模拟真实用户的鼠标键盘操作来操控 AnyClaw，没有捷径 API。

### 8.2 模式切换与持久化
- 控制模式在 Work 模式中显式可见可切换
- 模式值持久化到 config.json，启动时恢复
- 用户可随时一键回退到用户模式
- AI 模式下所有高风险动作仍走权限检查与审计

### 8.3 当前实现状态
- Work 面板已提供控制模式切换（User/AI）
- 模式值已写入 `config.json` 并在启动时恢复
- 当前为"控制权框架"阶段，后续补齐 AI 自动驱动执行策略

---


## 9. 工作区管理

- **功能编号：** WS-01 **优先级：** P0 **状态：** 🔧 核心已实现

### 9.1 概念与目录结构

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

**补充细节 - 默认模板文件内容摘要：**

| 文件 | 默认内容要点 |
|------|-------------|
| AGENTS.md | Session 启动流程：读 SOUL.md → USER.md → memory 日志 |
| SOUL.md | 核心原则：真诚帮助、有观点、先自救再提问 |
| USER.md | 用户信息模板（Name/Timezone/Notes） |
| TOOLS.md | 空白，供用户自定义工具配置 |
| HEARTBEAT.md | 空文件（注释提示），保持空则跳过心跳 API 调用 |
| IDENTITY.md | 默认 Name/Creature/Vibe/Emoji 模板 |
| MEMORY.md | 长期记忆文件，从 daily 记忆文件中定期整理 |
| BOOTSTRAP.md | 首次运行配置，完成后删除 |

**`workspace.json` 初始内容**：
```json
{
  "version": 1,
  "workspace_root": "<path>",
  "denied_paths": ["C:\\Windows\\", "C:\\Program Files\\"],
  "permissions": {
    "fs_read": "allow", "fs_write": "allow",
    "exec": "ask", "net_outbound": "allow", "device": "deny"
  },
  "limits": { "cmd_timeout_sec": 30, "max_subagents": 5, "net_rpm": 60 }
}
```

### 9.2 多工作区支持

用户可创建多个工作区，每个工作区独立配置。

### 9.3 WORKSPACE.md

工作区身份证：包含名称、根目录、创建时间、模板、版本信息。

### 9.4 工作区模板

| 模板 | SOUL.md 风格 | 额外文件 | 默认权限 |
|------|-------------|---------|---------|
| 通用助手 | 友好、通用 | 无 | 基础 |
| 开发者 | 技术导向 | TOOLS.md（编译工具配置） | 开放 |
| 写作者 | 文学风格 | 素材目录 | 基础 |
| 数据分析 | 严谨、数据导向 | Python 环境配置 | 开放 |

### 9.5 健康检查

| 检查项 | 未满足处理 |
|--------|-----------|
| 工作区目录是否存在 | 提示创建或更换目录 |
| 磁盘空间是否 > 100MB | 警告 |
| 目录是否有写入权限 | 降级为只读模式 |
| 是否被其他实例占用（lock 文件） | 检测进程存活，死亡则清理 lock，存活则拒绝启动 |
| 目录结构是否完整 | 引导创建缺失的核心文件 |
| OpenClaw 版本是否兼容 | 提示更新 |

**补充细节（代码实际实现）：**
- `WorkspaceHealth` 结构体包含 exists/writable/has_agents_md/has_soul_md/has_memory_dir/free_mb/error_msg 七个字段
- 磁盘空间检查通过 `std::filesystem::space()` 获取可用空间
- 写入权限检查通过创建临时文件 `.anyclaw_test` 验证，完成后自动删除

### 9.6 工作区锁
- 启动时创建 `.openclaw.lock`，写入 PID
- 退出时删除 lock 文件
- 启动检测到已有 lock → 判断 PID 是否存活 → 存活则拒绝，死亡则覆盖

**补充细节（代码实际实现）：**
- PID 检测使用 `OpenProcess(SYNCHRONIZE, FALSE, pid)` + `WaitForSingleObject(h, 0)`
- `WAIT_OBJECT_0` 判断进程是否已退出
- 死进程的 lock 文件会被自动覆盖
- `g_workspace_lock_held` 全局标志追踪锁状态

### 9.7 迁移（导出/导入）

**导出：**
- 打包内容：AnyClaw 配置 + OpenClaw 工作区全量 + manifest.json
- 输出：`AnyClaw_Migration_{version}_{timestamp}.zip`
- 安全：API Key 不导出明文

**补充细节（代码实际实现）：**
- `config.json` 中的 `api_key` 字段被替换为空字符串 `""`
- `openclaw.json` 中的 `token` 字段被替换为 `"__REDACTED__"`
- `openclaw.json` 中所有 `apiKey` 字段被替换为 `"__REDACTED__"`
- `manifest.json` 包含 hostname、export_time、version 信息

**导入：**
- 前置检查：Node.js ≥ 18、磁盘空间 > 500MB、OpenClaw 可用
- 路径适配：两台机器用户名可能不同 → 导入时自动检测当前用户的路径

### 9.8 目录消失处理

工作区目录不可达时提示用户重新创建、选择其他目录或创建新工作区。

### 9.9 AGENTS.md / TOOLS.md 双向同步

**MANAGED 区域机制：**
- `workspace_sync_managed_sections()` 在 AGENTS.md 和 TOOLS.md 中维护 `<!-- ANYCLAW_MANAGED_START/END -->` 区域
- 每次权限变更时自动同步
- 写入前自动备份到 `.backups/`，保留最近 10 份
- 文件名格式：`AGENTS.md.20260407_192005.bak`

**MANAGED 区域内容示例**：
```markdown
## AnyClaw Managed Policy
- generated_at: 2026-04-07 19:20:05
- workspace_root: D:\MyAIWorkspace\
- denied_paths: C:\\Windows\\, C:\\Program Files\\

## Permission Snapshot
- exec_shell: allow
- fs_write_workspace: allow
```

### 9.10 配置存储

| 文件 | 路径 | 管理方 | 说明 |
|------|------|--------|------|
| `config.json` | `%APPDATA%\AnyClaw_LVGL\` | AnyClaw | 主配置 |
| `permissions.json` | `%APPDATA%\AnyClaw_LVGL\` | AnyClaw | 权限 Source of Truth |
| `workspace.json` | 工作区根目录 | AnyClaw | 运行时权限投影 |
| `WORKSPACE.md` | 工作区根目录 | AnyClaw | 工作区元信息 |
| `.openclaw.lock` | 工作区根目录 | 运行时 | 实例锁 |

---


## 10. 权限系统

- **功能编号：** PERM-01 **优先级：** P0 **状态：** 🔧 核心已实现

### 10.1 权限模型（细粒度逐项控制）

> ⚠️ v2.2.1 修正：代码实际采用细粒度逐项控制（19 项权限键），而非 PRD v2.2.0 描述的两档模式。所有权限默认值为 Allow。

**实际权限键列表（19 项，代码中已定义）：**

| 分类 | 权限键 | 默认值 | 说明 |
|------|--------|--------|------|
| 文件系统 | `fs_read_workspace` | Allow | 读工作区文件 |
| | `fs_write_workspace` | Allow | 写工作区文件 |
| | `fs_read_external` | Allow | 读外部文件 |
| | `fs_write_external` | Allow | 写外部文件 |
| 命令执行 | `exec_shell` | Allow | 执行 shell 命令 |
| | `exec_install` | Allow | 安装软件 |
| | `exec_delete` | Allow | 删除文件 |
| 网络 | `net_outbound` | Allow | 出站网络 |
| | `net_inbound` | Allow | 入站网络 |
| | `net_intranet` | Allow | 局域网 |
| 设备 | `device_camera` | Allow | 摄像头 |
| | `device_mic` | Allow | 麦克风 |
| | `device_screen` | Allow | 屏幕 |
| | `device_usb_storage` | Allow | USB 存储 |
| | `device_remote_node` | Allow | 远程节点 |
| | `device_new_device` | Allow | 新设备 |
| 系统 | `clipboard_read` | Allow | 剪贴板读取 |
| | `clipboard_write` | Allow | 剪贴板写入 |
| | `system_modify` | Allow | 系统修改 |

每个权限键独立设置为 Allow/Deny/Ask/ReadOnly。

### 10.2 工作区边界（始终生效）

无论哪种模式，工作区边界始终强制：

1. Agent 文件操作限定在工作区根目录内
2. 路径穿越（`../`）拒绝
3. 系统目录（`C:\Windows\`、`C:\Program Files\`）始终禁止写入

**补充细节（代码实际实现）：**
- `is_path_allowed()` 按以下顺序检查：denied_paths → workspace_root → extra_allowed → 默认拒绝
- denied_paths 默认包含 `C:\Windows\` 和 `C:\Program Files\`
- 路径比较使用前缀匹配，先做正斜杠→反斜杠规范化
- 所有路径决策均写入审计日志

### 10.3 弹窗确认机制

当 Agent 请求超出当前权限设置的操作时：

**`ui_permission_confirm()` 返回值**：
- `-1`：UI 不可用（fallback 到 Win32 MessageBox）
- `0`：用户拒绝
- `1`：用户允许（仅本次）
- `2`：用户允许（持久化，自动修改权限值为 Allow 并保存）

**弹窗文本格式**：
```
Agent 请求执行命令：

{命令内容}

权限项: {权限键名}

是: 仅本次允许
取消: 永久允许
否: 拒绝
```

**Fallback 机制**：当 `ui_permission_confirm()` 返回 -1 时（非 UI 线程或 UI 未初始化），使用 Win32 `MessageBoxA` 原生弹窗。

#### [§10.3] 权限确认弹窗 Wireframe

```
  ┌──────────────────────────────────────────────────────────────┐
  │                    全屏黑色遮罩 (50% opacity)                 │
  │         ┌────────────────────────────────────────────┐       │
  │         │  AnyClaw 权限确认                          │       │ ← accent 色, 可拖拽
  │         │                                            │       │
  │         │  Agent 请求执行命令：                       │       │
  │         │  读取文件 config.json                      │       │
  │         │  权限项：FS_READ_WORKSPACE                 │       │
  │         │                                            │       │
  │         │  ┌──────────┐ ┌───────────┐ ┌───────────┐  │       │
  │         │  │   拒绝   │ │ 此次授权  │ │ 永久授权  │  │       │
  │         │  │  红色    │ │  蓝色     │ │  绿色     │  │       │
  │         │  └──────────┘ └───────────┘ └───────────┘  │       │
  │         │                                            │       │
  │         │  ⏱ 60秒超时自动拒绝                         │       │
  │         └────────────────────────────────────────────┘       │
  └──────────────────────────────────────────────────────────────┘
```

- 弹窗容器: 620px 宽, 背景 `panel (#232838)`, 圆角 12px, 内边距 16px
- 按钮: 拒绝(`btn_close #A02828`) / 此次授权(`btn_action #4070C0`) / 永久授权(`btn_add #288C46`)
- 超时机制: 60000ms 自动选择 deny

### 10.4 AGENTS.md / TOOLS.md 双向同步

**MANAGED 区域机制：** 模式切换时自动更新，写入前自动备份。

### 10.5 审计日志

权限决策记录到 `%APPDATA%\AnyClaw_LVGL\audit.log`：

```
[2026-04-07 19:20:05] action=exec_check target=pip install pandas decision=allow_once
[2026-04-07 19:20:07] action=path_check target=C:\Windows\system32\hosts decision=deny:denied_path
[2026-04-07 19:20:10] action=perm_set target=exec_shell decision=allow
```

`perm_audit_verify_chain()` 函数存在但为 v3.0 高级功能。

### 10.6 资源上限

**`ResourceLimits` 结构体已在 v2.0 代码中定义：**

```cpp
struct ResourceLimits {
    int max_disk_mb = 2048;
    int cmd_timeout_sec = 30;
    int net_rpm = 60;
    int max_subagents = 5;
    int max_file_read_kb = 500;
    int max_remote_connections = 3;
};
```

- 资源上限随 `permissions.json` 持久化
- 运行时同步到 `workspace.json` 的 `limits` 字段

### 10.7 workspace.json 运行时投影

`workspace_sync_runtime_config_from_permissions()` 从 `permissions.json` 生成 `workspace.json` 运行时投影，供 Agent 侧读取。

权限值映射：Allow/ReadOnly → "allow"，Ask → "ask"，Deny → "deny"。

### 10.8 应用授权（App Authorization）

Agent 需要操作用户已有的桌面应用（邮件、日历、编辑器等）来提供管家服务。

**自动检测与推荐：** 扫描系统已安装应用，自动检测可用交互方式（CLI > MCP > COM/API > Skill > UI 自动化），推荐最安全的方案。

**用户分层：** 普通用户看到授权/拒绝按钮；进阶用户可展开选择方式；开发者可完整配置。

### 10.9 v3.0+ 三层权限模型（高级模式）

v2.0 的细粒度逐项控制是基础版。v3.0 引入完整的三层权限模型作为"高级模式"：

```
第 1 层：工作区边界 (Workspace Boundary)
第 2 层：操作权限 (Operation Permissions) - 19+ 权限键
第 3 层：资源上限 (Resource Limits) - 用户可自定义
```

新增设备管理（设备信任表 + 新设备默认策略）、审计链式 hash 完整性校验、权限导入/导出。

---


## 11. OpenClaw 集成

### 11.1 进程管理
- **功能编号：** PM-01 **优先级：** P0
- 一键启动、停止、重启 OpenClaw Gateway（`openclaw gateway start/stop`）
- 进程检测：`CreateToolhelp32Snapshot` 遍历进程列表，查找 `node.exe`

**OpenClaw 自动检测策略：**
1. `where openclaw` PATH 查找 → 读取 `openclaw.json` 获取端口
2. npm 全局路径 `%APPDATA%\npm\node_modules\openclaw\openclaw.cmd`

### 11.2 状态指示

**三级统一规范**（LED / 托盘图标 / Task List 圆点 三者一致）：

| 状态 | 颜色 | 含义 |
|------|------|------|
| Ready | 🟢 绿 | Gateway 在 + HTTP 通 + Agent 空闲 |
| Busy | 🟡 黄 | Gateway 在 + 有活跃 Session（ageMs < 5min） |
| Checking | 🟡 黄 | 启动中 / 首次轮询 |
| Error | 🔴 红 | Gateway 进程不在 / HTTP 不通 |

### 11.3 健康监控
- **功能编号：** HM-01 **优先级：** P1
- **检测频率：** 每 30 秒（可配置 5s / 15s / 30s / 60s / 120s），通过 `g_refresh_interval_ms` 控制

**健康检查状态机（完整逻辑，代码实际实现）**：
```
Gateway 进程不存在 + HTTP 无响应:
    ├── 从未自动重启过 → 执行 app_start_gateway()，状态变为 Checking
    └── 已自动重启过 → 状态变为 Error，不再重试

Gateway 进程存在 + HTTP 无响应:
    ├── 连续失败次数 < 3 → 状态保持 Checking
    └── 连续失败次数 >= 3 → 状态变为 Error，重置自动重启标记

Gateway 进程存在 + HTTP 响应 200:
    ├── 首次轮询 → 状态为 Checking
    ├── 有活跃 Session → 状态为 Busy
    └── 无活跃 Session → 状态为 Ready
```

- 自动重启仅执行一次（`g_autoRestarted` 标志），重启失败后不再尝试
- 健康检查经过 `TraceSpan("health_check_cycle")` 记录追踪

### 11.4 会话与任务管理
- **功能编号：** TM-01 **优先级：** P1 **状态：** 🔧 实施中

**核心约束：** Gateway 不提供 REST API，所有数据通过 CLI 子进程调用获取。

| 数据源 | 命令 | 实测耗时 |
|--------|------|---------|
| 健康状态 | `curl http://127.0.0.1:18789/health` | ~50ms |
| 会话列表 | `openclaw gateway call sessions.list --json` | ~1.5s |
| 定时任务 | `openclaw cron list --json` | ~1.5s |

**Session 解析逻辑（代码实际实现）：**
- JSON 解析使用轻量级自定义解析器（`json_str` / `json_ll`），不依赖外部库
- Sub-Agent 检测：key 中包含 `subagent:` 则标记为子代理
- Cron 检测：key 中包含 `cron:` 则标记为定时任务
- token 计数依次尝试 `approximateTokens` 和 `totalTokens` 字段

**Task List UI** — 包含 Gateway Service 状态/控制、Session 列表（Abort 按钮）、Cron 任务列表（Run Now/Enable/Disable/Delete）。

**清理策略：** Session ageMs > 30 分钟隐藏；Sub-Agent 完成后保留 30 分钟自动清理。

### 11.5 Skill 管理
- **功能编号：** SK-01 **优先级：** P1
- Skill 批量下载/安装（ClawHub 仓库）、搜索与浏览
- 内置（Built-in）+ 第三方（ClawHub）分类展示
- Skill 启用/禁用、版本管理与更新

---


## 12. 模型与 API 配置

### 12.1 云端模型管理
- **功能编号：** MODEL-01 **优先级：** P0 **状态：** ✅ 已实现
- **架构：** AnyClaw 是 OpenClaw 的 GUI 壳子，聊天走 Gateway `/v1/chat/completions`，模型配置同步到 `openclaw.json`
- **当前模型显示：** 绿色高亮从 Gateway 读取的实际配置模型
- **模型选择：** 下拉列表（lv_dropdown），按能力排序
- **添加自定义模型：** 下拉框右侧 "+" 按钮 → 弹出模态对话框 → 输入模型名称 → 确认后自动添加到列表并选中
- 自定义模型保存到 `assets/custom_add_models.json`，持久化跨重启
- **免费模型：** OpenRouter 上 25 个 `:free` 模型，全部国内直连可达

**补充细节 - 模型加载优先级（代码实际实现）：**
1. 默认静态列表（`default_models[]`，21 个模型）
2. 用户自定义模型（`assets/custom_add_models.json`）
3. openclaw.json providers 配置中的模型
4. Gateway 当前配置的模型（插入列表顶部）

`model_ensure_default_config()` 在首次使用时自动设置 `openrouter/google/gemma-3-4b-it:free` 作为默认模型。

### 12.2 Provider 与 API Key
- **功能编号：** PROVIDER-01 **优先级：** P0 **状态：** ✅ 已实现
- **Provider 检测：** 下拉切换时自动检测 provider（openrouter/xiaomi），动态显示
- **API Key 联动：** 切换模型时自动从 Gateway 读取对应 provider 的 API Key 并填入输入框
- **所有 OpenRouter 模型共用一个 API Key**
- **Save 同步 Gateway：** 后台线程执行 `openclaw config set` 更新 provider + model + apiKey → 重启 Gateway（不阻塞 UI）

### 12.3 Gateway 同步
- **功能编号：** SYNC-01 **优先级：** P0 **状态：** ✅ 已实现
- AnyClaw 修改模型/API Key 后，通过 `openclaw config set` 同步到 Gateway
- Gateway 自动重启生效

### 12.4 Gemma 本地模型
- **功能编号：** GEMMA-01 **优先级：** P1 **状态：** 🔧 实施中
- 支持本地 Gemma 4 选装：E4B（入门，8GB RAM）/ 26B MoE（中档，16GB RAM），GGUF 格式
- 推理引擎：内嵌 llama.cpp 后台进程（`llama-server.exe`），CREATE_NO_WINDOW 隐藏运行
- 推理进程与 UI 进程隔离，大模型加载不卡界面
- 下载链路支持三源自动回退与取消中断
- 配置持久化：`gemma_install_opt_in`（bool）、`gemma_model_mask`（bitmask）

### 12.5 接入模式

| 模式 | 架构 | 状态 |
|------|------|------|
| Gateway 模式 | AnyClaw → OpenClaw Gateway → LLM | ✅ 当前唯一路径 |

Direct API 模式为架构预留，暂不启用。

### 12.6 Model Failover 弹性通道（新增）

- **功能编号：** FAILOVER-01 **优先级：** P1 **状态：** ✅ 已实现

Model Failover 是模型弹性通道系统，当当前模型不可用时自动切换到健康的备份模型。

#### 评分公式
```
score = 0.6 × stability + 0.25 × speed + 0.15 × capability
```

| 维度 | 计算方式 | 说明 |
|------|---------|------|
| stability | 1.0 - (consec_fail / 3) → clamp 0~1 | 连续失败越多分越低 |
| speed | 1.0 - (latency / 15000) → clamp 0~1 | 延迟越低分越高 |
| capability | 查表（405B→1.0, 7B→0.25 等） | 按模型参数量打分 |

#### 健康检查机制

| 参数 | 值 | 说明 |
|------|-----|------|
| 检查间隔 | 5 分钟 | `FAILOVER_CHECK_INTERVAL_MS=300000` |
| 探针超时 | 12 秒 | `FAILOVER_PROBE_TIMEOUT_SEC=12` |
| 连续失败阈值 | 3 次 | 触发冷却 |
| 冷却时间 | 10 分钟 | `FAILOVER_COOLDOWN_MS=600000` |
| 计数衰减 | 每 20 次检查 | 重置 success/fail 计数器 |
| 最大模型数 | 16 | `FAILOVER_MAX_MODELS=16` |

#### 状态转换
```
正常 → 连续失败 >= 3 → 冷却（10 分钟内不参与选择）
  → 冷却结束 → 恢复参与
  → 探针成功 → 健康状态恢复
```

#### 核心函数

| 函数 | 说明 |
|------|------|
| `failover_init()` | 从 config.json 加载 failover 配置 |
| `failover_probe_model(name)` | 直接调用 OpenRouter API 探测单个模型（最小请求，1 token） |
| `failover_probe_all()` | 对所有启用模型执行一轮探针 |
| `failover_get_next_healthy(current)` | 返回除当前模型外评分最高的健康模型 |
| `failover_record_result(name, success, latency)` | 记录模型调用结果，更新健康评分 |
| `failover_start_health_thread()` | 启动后台健康检查线程 |
| `failover_scan_available_models()` | 扫描有 API Key 的模型，加入 failover 列表 |

#### 探针机制
- `failover_probe_model()` 直接调用 OpenRouter API（不经 Gateway）
- 发送最小请求（1 token），HTTP 超时 12 秒
- 不重启 Gateway、不修改配置

#### Failover UI

Model Tab 中提供 Failover 区域：
- Switch: 启用自动切换
- 备用模型 Dropdown

#### [§13.3] Model Tab 布局图

```
┌──────────────────────────────────────────────────────────┐
│ General  │ Permissions │ Model  │  Log  │  About         │
├──────────────────────────────────────────────────────────┤
│  Current Model    ● openrouter/anthropic/claude-sonnet-4  │
│  ─────────────────────────────────────────────────────── │
│  Select Model                                            │
│  ┌──────────────────────────────────────────┐  ┌──────┐  │
│  │ claude-sonnet-4                    ▾     │  │  +   │  │
│  └──────────────────────────────────────────┘  └──────┘  │
│  Provider: OpenRouter                                    │
│  ─────────────────────────────────────────────────────── │
│  API Key                                                 │
│  ┌──────────────────────────────────────────────┐        │
│  │ ••••••••••••••••••••                   [Save] │        │
│  └──────────────────────────────────────────────┘        │
│  💡 Free model available                                 │
│  [使用免费模型]                                          │
│  ─────────────────────────────────────────────────────── │
│  Failover                                                │
│  [====] 启用自动切换                                      │
│  备用模型: [gemini-2.5-flash ▾]                          │
└──────────────────────────────────────────────────────────┘
```

---


## 13. 设置

### 13.1 Settings Tab 结构

| Tab | 内容 |
|-----|------|
| General | Boot Start / Auto Start / Theme / Language / Exit Confirm / Reconfigure Wizard |
| Permissions | 权限编辑（allow/deny/ask/read_only）19 项 |
| Model | Current Model / Search / Model List / API Key / Failover |
| Log | Log Enable / Log Level / Filter / Export / Clear |
| About | 品牌信息 / Tech Stack / Config Import/Export / Migration |

### 13.2 General Tab
- 开机自启动 / Auto Start / 语言切换（CN/EN/KR/JP）/ 主题切换 / 退出确认
- Security 状态面板：动态检测 API Key 状态、端口号、配置目录可写性
- **Reconfigure Wizard 按钮**

#### [§13.2] General Tab 布局图

```
┌──────────────────────────────────────────────────────────┐
│ General  │ Permissions │ Model  │  Log  │  About         │
├──────────────────────────────────────────────────────────┤
│  Gateway Status    ● Ready                               │
│  Install Path      C:\Users\xxx\AppData\Roaming\...      │
│  Workspace         D:\Projects\AnyClaw                   │
│  ─────────────────────────────────────────────────────── │
│  Boot Start        [====]  Start on boot                 │
│  Auto Start        [    ]  Restart on crash              │
│  ─────────────────────────────────────────────────────── │
│  Theme             [■ Dark] [□ Light] [□ Classic]        │
│  Language          [CN / EN / KR / JP ▾]                 │
│  Exit Confirm      [====]  弹出退出确认                  │
│  Close on Exit     [====]  Close OpenClaw on exit        │
│  ─────────────────────────────────────────────────────── │
│  Security Status                                           │
│    API Key: ● 已配置 / ○ 未配置                           │
│    Gateway Port: 18789                                    │
│    Config Dir: 可写 ✓ / 不可写 ✗                          │
│  ─────────────────────────────────────────────────────── │
│  ┌────────────────────────────────────────────────────┐  │
│  │           [ 🔧 Reconfigure Wizard ]                │  │
│  └────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────┘
```

### 13.3 Model Tab
- 当前模型显示 + 下拉选择 + 自定义模型 + 免费模型 + API Key + Failover
- 详见 §12 各节

### 13.4 Log Tab
- 日志系统总开关 + 日志级别 + 级别过滤 + 实时日志列表（按级别着色）+ 导出/清除

#### [§13.4] Log Tab 布局图

```
┌──────────────────────────────────────────────────────────┐
│ General  │ Permissions │ Model  │  Log  │  About         │
├──────────────────────────────────────────────────────────┤
│  Log System   [====]  Write to logs\app.log              │
│  Log Level    [Debug ▾]                                  │
│  ─────────────────────────────────────────────────────── │
│  Filter       [All ▾]  [🔄 Refresh]                      │
│  ┌────────────────────────────────────────────────────┐  │
│  │ [12:30:01 INFO] Gateway health check OK            │  │
│  │ [12:30:02 WARN] Session timeout approaching        │  │
│  │ [12:30:03 ERROR] Failed to connect to model        │  │
│  │ [12:30:04 DEBUG] Retry attempt 2/3                 │  │
│  │ ...                                                │  │
│  └────────────────────────────────────────────────────┘  │
│  ● DEBUG   ● INFO   ● WARN   ● ERROR   [Export] [Clear] │
└──────────────────────────────────────────────────────────┘
```

- 日志列表: `lv_list_create`, `flex_grow=1`, 背景 `#141420`, 最大条目 200
- 级别颜色: DEBUG `#808080` / INFO `#4090E0` / WARN `#E0C020` / ERROR `#E03030`

### 13.5 Permissions Tab
- 9 项权限编辑，每项 Dropdown（allow/deny/ask/read_only）
- Save 按钮 + 状态反馈

#### [§13.5] Permissions Tab 布局图

```
┌──────────────────────────────────────────────────────────┐
│ General  │ Permissions │ Model  │  Log  │  About         │
├──────────────────────────────────────────────────────────┤
│  权限策略                                                │
│  可修改核心权限，保存后写入 APPDATA/AnyClaw_LVGL/...      │
│  ── 文件系统 ──────────────────────────────────────────── │
│  FS Read Workspace       [allow ▾]                       │
│  FS Write Workspace      [allow ▾]                       │
│  FS Read External        [ask ▾]                         │
│  FS Write External       [deny ▾]                        │
│  ── 命令执行 ──────────────────────────────────────────── │
│  Exec Shell              [ask ▾]                         │
│  Exec Install            [deny ▾]                        │
│  ── 网络 ──────────────────────────────────────────────── │
│  Net Outbound            [allow ▾]                       │
│  Net Intranet            [ask ▾]                         │
│  ── 系统 ──────────────────────────────────────────────── │
│  System Modify           [deny ▾]                        │
│  ──────────────────────────────────────────────────────── │
│  [ 保存权限配置 ]    未保存 / Saved / Save failed         │
└──────────────────────────────────────────────────────────┘
```

### 13.6 About Tab
- 品牌信息、版本号、Tech Stack
- 配置导入/导出、一键迁移（Export/Import Migration）
- Check Skill Versions 按钮

#### [§13.6] About Tab 布局图

```
┌──────────────────────────────────────────────────────────┐
│ General  │ Permissions │ Model  │  Log  │  About         │
├──────────────────────────────────────────────────────────┤
│              ┌──────────────┐                            │
│              │ garlic_sprout│                            │ ← 120×120
│              │    .png      │                            │
│              └──────────────┘                            │
│           AnyClaw LVGL                                   │
│       Garlic Lobster - Your AI Assistant                 │
│              Version: 2.2.1                              │
│  ─────────────────────────────────────────────────────── │
│  Tech Stack: LVGL 9.6 + SDL2 / C++17 / Win32 API        │
│  ─────────────────────────────────────────────────────── │
│  Configuration                                           │
│  ┌────────────────┐  ┌────────────────┐                  │
│  │  Export Config  │  │   Clear Chat   │                  │
│  └────────────────┘  └────────────────┘                  │
│  Migration                                               │
│  ┌────────────────┐  ┌────────────────┐                  │
│  │ Export Migration│  │Import Migration│                  │
│  └────────────────┘  └────────────────┘                  │
│        Copyright 2026 AnyClaw Project                    │
│  ┌────────────────────────────────────────────────────┐  │
│  │         Check Skill Versions                       │  │
│  └────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────┘
```

---


## 14. 外观与主题

### 14.1 色彩方案
- **功能编号：** TH-01 **优先级：** P1

| 主题 | 背景 | 面板 | 文字 |
|------|------|------|------|
| 默认暗色 | #1A1E2E | #232838 | #E0E0E0 |
| 默认亮色 | #F5F5F5 | #FFFFFF | #333333 |
| 经典暗色 | #2D2D2D | #3C3C3C | #FFFFFF |

#### 全局主题色板（Dark 主题）

| 语义 | 颜色值 | 用途 |
|------|--------|------|
| `bg` | `#1A1E2E` | 窗口背景 |
| `panel` | `#232838` | 面板/卡片背景 |
| `panel_border` | `#373C55` | 面板边框 |
| `input_bg` | `#191C26` | 输入框/下拉背景 |
| `text` | `#E0E0E0` | 主文字 |
| `text_dim` | `#9095B0` | 次要文字 |
| `accent` | `#82AAF0` | 强调色/标题 |
| `btn_action` | `#4070C0` | 主操作按钮 |
| `btn_secondary` | `#505568` | 次要按钮 |
| `btn_close` | `#A02828` | 危险/关闭按钮 |
| `btn_add` | `#288C46` | 添加/确认按钮 |

### 14.2 字体管理

| 模块 | 字体 | 字号 |
|------|------|------|
| 中文 | 微软雅黑 (msyh.ttf) | 16px |
| 英文 | Montserrat | 10~20px |
| 图标 | LVGL Symbols | 对应字号 |

### 14.3 通用控件体系
- **功能编号：** WIDGET-01 **优先级：** P1 **状态：** ✅ 已实现

**控件尺寸常量（`app_config.h`）：**

| 类别 | 常量 | 值 |
|------|------|-----|
| 聊天气泡 | `CHAT_BUBBLE_MAX_PCT` | 70% |
| 聊天气泡 | `CHAT_BUBBLE_PAD_H/V` | 8/6 px |
| 聊天气泡 | `CHAT_BUBBLE_RADIUS` | 10 |
| 输入框 | `CHAT_INPUT_DEFAULT_H` | 96 px |
| 按钮 | `BTN_SEND_SIZE_BASE` | 24 px |
| 弹窗 | `DIALOG_PAD_ALL` | 16 |
| 弹窗 | `DIALOG_RADIUS` | 12 |
| 设置行 | `SETTINGS_ROW_H` | 32 px |
| Dropdown | `DROPDOWN_RADIUS` | 6 |

**弹窗系统通用样式（`create_dialog()`）：**
- 遮罩: 全屏黑色, LV_OPA_50
- 容器: 居中, bg=panel, border=panel_border (1px), radius=12
- 内边距: 16px, flex: COLUMN, gap=12
- 支持拖拽移动 (dialog_drag_cb)

---


## 15. 系统管理

### 15.1 日志系统
- **功能编号：** LOG-01 **优先级：** P1
- 日志文件：`%APPDATA%\AnyClaw_LVGL\logs\app.log`
- 轮转规则：文件超过 1MB 自动轮转，最多保留 5 个文件
- 启动时自动清理 7 天以上的轮转文件
- UI 日志使用 `LOG_MAX_LINES=200` 限制内存中的日志条目数

### 15.2 错误处理与崩溃恢复
- **功能编号：** E-01 **优先级：** P1
- 崩溃日志：`%APPDATA%\AnyClaw_LVGL\crash_logs\crash_*.log`，保留最近 10 个
- SetUnhandledExceptionFilter 捕获异常
- Stack Trace 通过 dbghelp 符号解析，最多 64 帧
- 崩溃后生成 `last_crash.txt` 标记文件

### 15.3 通知系统
- **功能编号：** N-01 **优先级：** P1
- 托盘气泡通知 / 日志面板通知 / 状态栏颜色变化

### 15.4 更新机制
- **功能编号：** U-01 **优先级：** P0
- OpenClaw 更新：通过 `openclaw update` 命令
- AnyClaw 更新：GitHub Releases 自动检测 + 一键下载覆盖
- 托管仓库：`IwanFlag/AnyClaw_LVGL`

### 15.5 版本号规范
- **功能编号：** VER-01 **优先级：** P1
- 格式：`v{主版本}.{次版本}.{修订号}`
- 当前版本：`v2.2.1`
- 存储：`#define ANYCLAW_VERSION "2.2.1"`

### 15.6 商业模式与授权系统
- **功能编号：** LIC-01 **优先级：** P0 **状态：** ✅ 已实现（V1）

**商业模式：Freemium（免费 + 增值付费）**

| 层级 | 内容 | 价格 |
|------|------|------|
| **免费版** | Chat 模式 + 基础 Work 模式 + 单工作区 + 基础权限 | 免费 |
| **专业版** | 多工作区 + 完整权限 + DiffRenderer + TableRenderer + 本地模型 + IM 多渠道 + Agent 主动行为完整版 | 付费 |
| **企业版** | 权限审计 + 多实例管理 + 部署支持 | 按席位收费 |

**授权系统（HMAC-SHA256 时间递增密钥方案）：**

> ⚠️ v2.2.1 修正：代码实际使用 HMAC-SHA256 + Base32 编码的时间递增密钥方案，非 PRD v2.2.0 描述的 HW/TM 双密钥。

**密钥格式：**
```
Payload (10 bytes):
  [0-1]: seq (2 bytes, big-endian) — 序列号
  [2-9]: expiry (8 bytes, big-endian) — Unix 过期时间戳

完整 = Base32(payload[10] + HMAC-SHA256(payload)[0:8])
密钥 = 18 bytes raw → Base32 编码 → 分组显示
```

**HMAC 密钥**：`"AnyClaw-2026-LOVE-GARLIC"`（硬编码在 `license.cpp`）
**基础时长**：`BASE_DURATION_H=12` 小时

**验证流程：**
1. 用户输入密钥字符串 → 去除空格和连字符，转大写
2. Base32 解码 → 得到 18 字节 raw
3. 分离 payload(10B) + mac(8B)
4. 用 LICENSE_SECRET 对 payload 计算 HMAC-SHA256
5. 比对前 8 字节 MAC → 不匹配则拒绝
6. 从 payload 提取 seq(2B) + expiry(8B)
7. 检查 expiry > now → 过期则拒绝
8. 检查 seq >= g_license_seq → 序列号太低则拒绝（防回退）
9. 通过 → 更新 config.json 中的 license_seq + license_expiry

**License 状态判断：**
- `license_is_valid()`：expiry <= 0 时视为试用期（始终有效），否则检查 `now < expiry`
- `license_remaining_seconds()`：返回剩余秒数，expiry <= 0 时返回 9999999（无限试用）
- `license_get_remaining_str()`：格式化为 "Xd remaining" / "X.Xh remaining" / "Xm remaining"

**配置存储（config.json）：**
```json
{
  "license_seq": 1,
  "license_expiry": 1743981600
}
```
- 无 `hw_key` / `tm_key` 字段
- 免费版无需密钥，`license_seq=0` 且 `license_expiry=0` 视为试用期

#### [§15.6] License 激活弹窗 Wireframe

```
  ┌──────────────────────────────────────────────────────────────┐
  │                    全屏黑色遮罩 (50% opacity)                 │
  │         ┌────────────────────────────────────────────┐       │
  │         │  License Activation                        │       │
  │         │  ⚠ Trial Period Ended                      │       │ ← 红色
  │         │  Enter a license key to continue           │       │
  │         │  Current sequence: 3 (next: seq 4, 240h)  │       │
  │         │  ┌──────────────────────────────────────┐  │       │
  │         │  │ XXXXX-XXXXX-XXXXX                    │  │       │
  │         │  └──────────────────────────────────────┘  │       │
  │         │  ┌──────────────────┐ ┌─────────────────┐  │       │
  │         │  │     Activate     │  │     Close      │  │       │
  │         │  │   绿色 #00B43C   │  │ btn_secondary  │  │       │
  │         │  └──────────────────┘ └─────────────────┘  │       │
  │         └────────────────────────────────────────────┘       │
  └──────────────────────────────────────────────────────────────┘
```

---


## 16. 技术规格

### 16.1 快捷键
- **功能编号：** K-01 **优先级：** P1
- Ctrl+V 粘贴、Ctrl+C 复制、Ctrl+X 剪切、Ctrl+A 全选
- Enter 发送、Shift+Enter 换行
- 选中文字后按键直接替换

### 16.2 网络通信架构

```
AnyClaw (GUI)
  ├→ Chat/SSE    → HTTP POST + SSE  → Gateway :18789  → OpenClaw Agent → LLM API
  ├→ Health      → HTTP GET /health → Gateway :18789
  ├→ Sessions    → CLI 子进程调用   → openclaw gateway call sessions.list --json
  ├→ Config      → CLI 子进程调用   → openclaw config set/get
  ├→ License     → 本地内存验证     → 不走 Gateway
  ├→ KB 检索     → 本地文件搜索    → 不走网络
  ├→ Failover 探针 → HTTP POST     → OpenRouter API（直连，不经 Gateway）
  └→ 本地模型    → HTTP POST + SSE  → llama-server :18800 → GGUF 文件
```

| 场景 | 目标 | 协议 | 超时 |
|------|------|------|------|
| 聊天（云端） | Gateway :18789 | HTTP POST + SSE, OpenAI 格式 | 流式 45s |
| 健康检查 | Gateway :18789 | HTTP GET /health | 3s |
| 会话管理 | openclaw CLI | 子进程 + JSON | 12s |
| Failover 探针 | OpenRouter API | HTTP POST, OpenAI 格式 | 12s |
| 配置更新 | openclaw CLI | `openclaw config set` | 8s |
| 更新检查 | GitHub API | HTTP GET | 10s |

**Gateway 通信协议（OpenAI 兼容）：**
```http
POST http://127.0.0.1:18789/v1/chat/completions
Content-Type: application/json
Authorization: Bearer <gateway_token>

{
  "model": "openclaw:main",
  "messages": [...],
  "stream": true
}
```

### 16.3 性能约束与硬件要求

**最低配置（云端模型）：** Windows 10 x64, 双核 2GHz+, 4GB RAM, 500MB 磁盘
**推荐配置（本地 E4B）：** 四核 2.5GHz+, 8GB RAM, 5GB 磁盘
**高配（本地 26B MoE）：** 八核 3GHz+, 16GB RAM, 20GB+ 磁盘

**关键性能常量：**
- `CHAT_STREAM_INTERVAL_MS=50`：流式渲染刷新间隔
- `CHAT_STREAM_CHUNK=3`：每次追加字符数
- `HEALTH_CHECK_DEFAULT_MS=30000`：健康检查默认间隔
- Ring buffer：Trace 模块 1000 事件、Chat 历史 buffer 4096 字节

### 16.4 安全

#### S.1 内存安全
- `secure_zero()` 函数用于安全擦除敏感内存
- API Key 通过 `g_api_key[256]` 全局变量存储，退出前应清零

#### S.2 进程隔离
- 所有外部命令执行通过 `CreateProcessA` + `CREATE_NO_WINDOW`，不继承句柄
- Pipe 使用 `SECURITY_ATTRIBUTES` + `SetHandleInformation` 防止子进程继承读端
- 命令执行超时强制杀死进程

#### S.3 数据脱敏
- 迁移导出时自动脱敏：`api_key` → `""`、`token` → `"__REDACTED__"`、`apiKey` → `"__REDACTED__"`
- 审计日志不记录密钥内容，仅记录操作和决策

#### S.4 工作区沙箱
- 路径穿越（`../`）检测：`is_path_allowed()` 通过前缀匹配阻止
- 系统目录写入永久禁止：`denied_paths_` 默认包含 `C:\Windows\`、`C:\Program Files\`

#### S.5 License 验证
- HMAC-SHA256 签名验证防止伪造密钥
- 序列号递增检查防止密钥回退攻击

#### S.6 崩溃处理
- `SetUnhandledExceptionFilter` 全局异常捕获
- 崩溃日志包含异常码、异常地址、操作类型、堆栈帧（最多 64 帧）
- dbghelp 符号解析用于堆栈追踪

#### S.7 单实例保护
- Windows Mutex 全局命名空间防止重复启动
- 工作区锁文件 `.openclaw.lock` 防止多实例操作同一工作区

### 16.5 DPI 自动检测
- **功能编号：** DPI-01 **优先级：** P1
- 启动时调用 `SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)`
- 布局常量按 DPI 缩放（`SCALE()` 宏）

### 16.6 IME 输入法支持
- **功能编号：** IME-01 **优先级：** P1
- SDL hints 启用 IME：`SDL_HINT_IME_SHOW_UI=1`、`SDL_HINT_IME_SUPPORT_EXTENDED_TEXT=1`

### 16.7 宏开关管理
- **功能编号：** MACRO-01 **优先级：** P1 **状态：** ✅ 已实现
- 所有可调参数用宏集中管理在 `src/app_config.h`

---


## 17. 远程协作与高级功能

### 17.1 远程协作

- **功能编号：** REMOTE-01 **优先级：** P1 **状态：** 🔧 基础层已实现 / 连接层待实施

不自建远程桌面，全部基于 Windows 内置方案（Quick Assist / RDP）。

**RemoteProtocolManager 实现细节（代码实际实现）：**

**会话状态机**：
```
pending → requesting → pending_accept → connected → closed
```

**安全机制：**
- 会话 ID 格式：`rp-{tick32}-{seq4}`
- 认证 Token 格式：`auth-{rand8}-{rand8}-{seed}`
- Token 旋转：`pending_accept` 状态验证成功后自动更换 token（防重放攻击）
- 过期会话自动清理，最近 64 条记录保留用于审计

**核心 API：** `create_session()` / `verify_token()` / `try_update_state()` / `close_session()` / `recent_records()`

**远程协作路线图：**

| 阶段 | 内容 | 状态 |
|------|------|------|
| v2.0 基础层 | 请求/接收/拒绝/断开状态机 UI + 安全闸门 | ✅ 已实现 |
| v2.0 连接层 | Quick Assist 启动 + 安全码读取 + IM 发送 | 🔧 待实施 |
| v2.1 | RDP 支持 + 信任设备管理 + 历史连接记录 | 🔧 规划中 |
| v2.2 | 局域网自动发现 + 双向语音 | ⏸️ 移至附录候选 |

### 17.2 本地知识库
- **功能编号：** KB-01 **优先级：** P1 **状态：** 🔧 V1 已实现

**KbStore V1 实现细节（代码实际实现）：**

**存储：** 索引文件 `%APPDATA%\AnyClaw_LVGL\kb_index.txt`，文档内容在内存中缓存，启动时懒加载。

**搜索算法**：
```
score = 100 (base)
  + min(120, frequency × 15)     // 关键词出现频次
  + max(0, 40 - position/120)    // 首次命中位置
  + 80 (if filename matches)     // 文件名命中加分
```

**上下文构建：** `build_context_snippet(keyword, max_docs=3, max_chars=1200)` 返回格式化上下文，命中片段截取以关键词为中心约 260 字符，片段去重基于内容哈希。

**核心 API：** `ensure_loaded()` / `add_source_file()` / `search_keyword()` / `build_context_snippet()` / `doc_count()`

---


## 18. 分发与增长

- **功能编号：** GROW-01 **优先级：** P0 **状态：** 🆕 待实施

### 18.1 目标用户触达

| 用户群 | 来源渠道 | 核心诉求 |
|--------|---------|---------|
| OpenClaw 现有用户 | GitHub / OpenClaw Discord | 更好的 GUI 体验 |
| 学生 | V2EX / 掘金 / 校园社群 | 免费 + 写论文/做作业 |
| 职场用户 | 小红书 / 知乎 / 公众号 | 效率工具 + 简单好用 |
| 开发者 | GitHub / Hacker News / Product Hunt | 编程工作台 + 开源 |

### 18.2 渠道策略

| 渠道 | 动作 | 成本 | 优先级 |
|------|------|------|--------|
| ai-bot.cn | 提交收录（AI智能体 + AI编程工具 两个分类） | 免费 | P0 |
| GitHub | 开源客户端核心 / Releases + README | 免费 | P0 |
| OpenClaw 社区 | Discord / GitHub Discussions 推广 | 免费 | P0 |
| V2EX / 掘金 | 产品介绍 + 使用体验文章 | 免费 | P1 |
| Product Hunt | 海外发布 | 免费 | P1 |

### 18.3 差异化传播点

| 渠道 | 主打卖点 |
|------|---------|
| ai-bot.cn | "不只是聊天窗口，AI 帮你干活还能看到它干了什么" |
| OpenClaw 社区 | "免费客户端，权限系统 + 双模式 + IM 集成" |
| 学生渠道 | "零成本 AI 助手，帮写论文、做 PPT、分析数据" |
| 开发者渠道 | "20MB 原生 GUI，不是 300MB 的 Electron 套壳" |


---


## 新增章节：Feature Flags 功能开关

- **功能编号：** FF-01 **优先级：** P1 **状态：** ✅ 已实现

功能开关系统支持低风险灰度发布，允许在不重新编译的情况下启用/禁用特定功能。

### 预定义开关（6 个）

| 开关名 | 描述 | 默认值 | 需要重启 |
|--------|------|--------|---------|
| `sbx_sandbox_exec` | 沙箱执行 | OFF | 否 |
| `obs_tracing` | 可观测性追踪 | OFF | **是** |
| `boot_self_heal` | 启动自修复 | OFF | 否 |
| `mem_long_term` | 长期记忆 | OFF | 否 |
| `kb_knowledge_base` | 知识库 | OFF | 否 |
| `remote_collab` | 远程协作 | OFF | **是** |

### 配置存储
`config.json` 中的 `feature_flags` 对象：
```json
{
  "feature_flags": {
    "sbx_sandbox_exec": false,
    "obs_tracing": false,
    "boot_self_heal": false,
    "mem_long_term": false,
    "kb_knowledge_base": false,
    "remote_collab": false
  }
}
```

### 核心 API

| 方法 | 说明 |
|------|------|
| `feature_flags_init()` | 启动时调用，从 config.json 加载 |
| `feature_flags().is_enabled(name)` | 检查开关状态 |
| `feature_flags().set_enabled(name, true/false)` | 设置开关 |
| `feature_flags().save_to_config()` | 持久化到 config.json |
| `feature_flags().reset_to_defaults()` | 重置全部为默认值 |
| `feature_flags().get_all_flags()` | 获取所有开关列表（供 UI 显示） |

- `feature_flags_init()` 在 `main()` 中早期调用
- 标记 `requires_restart=true` 的开关变更需要重启应用才能生效


---


## 新增章节：Tracing 可观测性追踪

- **功能编号：** OBS-01 **优先级：** P1 **状态：** ✅ 已实现

追踪系统记录应用内关键操作的性能和结果数据，用于排障和性能分析。

### 架构

```
TraceSpan (RAII)  →  TraceManager (singleton)
                          ├── Ring Buffer (1000 events, in-memory)
                          └── traces.jsonl (persistent, APPDATA)
```

### 数据结构
```cpp
struct TraceEvent {
    std::string ts;          // ISO-8601 时间戳（含毫秒）
    std::string trace_id;    // "tr_{tick32}_{counter}"
    std::string action;      // 操作名（如 "http_post_stream"）
    std::string target;      // 目标资源（如 URL、session key）
    int latency_ms;          // 耗时毫秒
    std::string outcome;     // "ok" / "fail" / "timeout"
};
```

### Ring Buffer
- 容量：1000 事件，满时循环覆盖
- 线程安全：`CRITICAL_SECTION` 保护
- 每 50 个事件自动 flush 到文件（防崩溃丢失）

### 文件输出
- 路径：`%APPDATA%\AnyClaw_LVGL\traces.jsonl`
- 格式：JSON Lines（每行一个 JSON 对象），追加写入模式

### RAII TraceSpan
```cpp
{ TraceSpan span("http_post_stream", url); ... }  // 自动记录 outcome=ok
TraceSpan span("perm_check", cmd);
if (!ok) span.set_fail();                          // 手动控制 outcome
```

### 便捷宏
```cpp
TRACE("action", "target");                    // RAII span
TRACE_RECORD("action", "target", ms, "ok");   // 手动记录
```

### 已集成追踪的操作
- `health_check_cycle`：每轮健康检查
- `session_refresh` / `session_abort` / `session_abort_all`：会话管理
- `boot_check_run_and_fix`：启动健康检查
- `http_post_stream`：聊天 SSE 流式请求


---


## 新增章节：LAN Chat 局域网聊天

- **功能编号：** LAN-CHAT-01 **状态：** ⏸️ 暂不实施（已迁附录候选）
- 本功能不纳入当前版本正文交付，详细设计见附录 `A.6`。


---


## 新增章节：FTP 传输

- **功能编号：** FTP-01 **状态：** ⏸️ 暂不实施（已迁附录候选）
- 本功能不纳入当前版本正文交付，详细设计见附录 `A.6`。


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
│   ├── boot_check.cpp / .h     # 启动健康检查（9 项 + 自动修复）
│   ├── model_manager.cpp / .h  # 模型管理 + Failover
│   ├── permissions.cpp / .h    # 权限系统（19 项细粒度控制）
│   ├── license.cpp / .h        # 授权验证（HMAC-SHA256）
│   ├── feature_flags.cpp / .h  # 功能开关系统
│   ├── tracing.cpp / .h        # 可观测性追踪
│   ├── session_manager.cpp/.h  # 会话管理
│   ├── workspace.cpp / .h      # 工作区管理
│   ├── migration.cpp / .h      # 迁移导出/导入
│   ├── remote_protocol.cpp/.h  # 远程会话协议
│   ├── remote_tcp_channel.cpp/.h # 远程 TCP 通道
│   ├── kb_store.cpp / .h       # 本地知识库
│   ├── garlic_dock.cpp / .h    # 贴边大蒜头
│   ├── lan_chat_client.cpp/.h  # 局域网聊天客户端
│   ├── lan_chat_types.h        # LAN 聊天类型定义
│   ├── lan_chat_protocol.cpp/.h # LAN 聊天协议
│   ├── ftp_client.cpp / .h     # FTP 传输
│   ├── app_config.h            # 宏开关集中管理
│   ├── app.h                   # 公共声明
│   ├── app_log.h               # 日志宏
│   ├── lang.h                  # 国际化定义
│   ├── theme.h                 # 主题色定义
│   ├── markdown.h              # Markdown 渲染
│   ├── icon_config.h           # 图标配置
│   ├── json_util.cpp           # JSON 工具
│   ├── utf8_util.cpp           # UTF-8 工具
│   ├── cjk_font_data.h         # CJK 字体数据
│   ├── lv_conf.h               # LVGL 配置
│   ├── resource.h              # Windows 资源 ID
│   └── widgets/                # 通用控件
│       ├── aw_button.h
│       ├── aw_input.h
│       ├── aw_panel.h
│       ├── aw_label.h
│       ├── aw_common.h
│       └── aw_form.h
├── assets/                     # 运行时资源
├── thirdparty/                 # LVGL + SDL2
├── tools/                      # 构建脚本
├── artifacts/                  # 构建产物
└── docs/                       # 文档
```

### A.2 Gateway 与 Session 架构

**Gateway 是什么：** OpenClaw 的守护进程，跑在本机 `localhost:18789`，是整个系统的"总机"。

| 概念 | 说明 |
|------|------|
| **Agent** | AI 大脑，定义 system prompt、工具、模型 |
| **Session** | 一次对话实例，携带上下文和历史消息 |
| **Gateway** | 守护进程，路由消息、管理 Session 生命周期 |

**通信架构总览：**
```
AnyClaw (GUI)
  ├→ Chat UI → HTTP POST + SSE → Gateway :18789 → OpenClaw Agent → LLM API
  ├→ Work UI → SSE 步骤流解析 ← Gateway
  ├→ Settings → CLI 子进程调用（openclaw config set/get）
  ├→ License → 本地内存验证（不走 Gateway）
  └→ 本地模型 → HTTP POST + SSE → llama-server :18800 → GGUF 文件
```

### A.3 Agent / Sub-Agent / Session 关系

| 概念 | 说明 |
|------|------|
| **Main Agent** | 与用户直接交互的主代理 |
| **Sub-Agent** | 由主代理通过 `sessions_spawn` 派出的独立代理 |
| **Sub-Agent 深度限制** | 最多 1 层，子代理不能再派子代理 |
| **结果通知** | 完成时自动 push 结果到主代理（push-based） |

### A.4 AnyClaw Agent 管理器架构（Direct API 终局目标）

**当前架构：** AnyClaw (GUI) → OpenClaw Gateway (Agent) → LLM
**目标架构：** AnyClaw 自身成为 Agent 管理器，直接管理 prompt、工具调用、会话上下文。

### A.5 竞品功能吸收

对 Ollama、LM Studio、Cursor、Claude、Hermes Agent 五个产品的横向对比，筛选 AnyClaw 应当吸收的功能方向：

| # | 功能方向 | 来源 | 优先级 |
|---|---------|------|--------|
| 1 | MCP Client | LM Studio | 🔴 高 |
| 2 | 本地模型推理引擎 | Ollama + LM Studio | 🔴 高 |
| 3 | OpenAI 兼容 API 服务端 | LM Studio + Ollama | 🔴 高 |
| 4 | RAG 文档聊天体验 | LM Studio | 🔴 高 |
| 12 | Docker 安全沙箱 | Hermes Agent | 🔴 高（v3.0） |
| 13 | 自动技能创建 + 开放标准 | Hermes Agent | 🟡 中 |
| 14 | 一键安装体验 | Hermes Agent | 🟡 中 |

### A.6 LAN/FTP 候选设计（暂不实施）

> 说明：以下内容保留为后续候选方案，不纳入当前版本正文交付范围。

#### A.6.1 LAN Chat（LAN-CHAT-01）

**定位：** 同局域网内 AnyClaw 用户之间直接通信，不走 Gateway，不走公网。

**架构：**
- 基于 TCP/UDP 的 Client-Host 模型
- TCP 用于聊天消息传输
- UDP 用于局域网自动发现

**Host 模式：**
- 监听 TCP 端口接收客户端连接（backlog=8）
- 监听 UDP 端口响应发现请求
- 发现协议：收到 `ANYCLAW_DISCOVER` UDP 包 → 回复 `ANYCLAW_HOST|{port}`

**Client 模式：**
- TCP 连接到指定 host:port
- 连接后发送 `HELLO|nickname` 注册
- 独立接收线程处理消息

**消息类型：**

| 类型 | 协议命令 | 说明 |
|------|---------|------|
| 注册 | `HELLO\|nick` | 连接时发送 |
| 群聊 | `BROADCAST\|from\|\|\|text` | 广播给所有人 |
| 私聊 | `PRIVATE\|from\|to\|\|text` | 点对点消息 |
| 建群 | `GROUP_CREATE\|from\|\|group\|` | 创建主题群 |
| 加群 | `GROUP_JOIN\|from\|\|group\|` | 加入群 |
| 群消息 | `GROUP_MSG\|from\|\|group\|text` | 群内消息 |

**事件回调：**
```cpp
using EventCallback = std::function<void(const LanChatEvent&)>;
```

**身份绑定模型：**
```
device_id = SHA256(MAC + salt) 前 16 字符
signature = HMAC(MAC, nickname)
广播：{ nickname, device_id, signature }
```

#### A.6.2 FTP 传输（FTP-01）

基于 WinINet 的 FTP 传输候选方案。

**功能：**
- 单文件上传/下载
- 目录递归上传（`recursive_upload=true`）
- 进度回调（百分比 + 步骤文本）
- 支持取消（`std::atomic<bool>& cancel_flag`）
- FTPS 支持（`use_ftps=true`，使用 `INTERNET_FLAG_SECURE`）
- 断点续传（`resume_download=true`）

**核心 API：**
```cpp
bool ftp_transfer_file(const FtpTransferConfig& cfg,
                       std::atomic<bool>& cancel_flag,
                       std::string& error_out,
                       FtpProgressCallback on_progress);
```

**配置结构：**
```cpp
struct FtpTransferConfig {
    std::string host;
    int port = 21;
    std::string username, password;
    std::string remote_path, local_path;
    bool upload = false;
    bool use_ftps = false;
    bool recursive_upload = false;
    bool resume_download = true;
};
```

**聊天能力（历史讨论保留）：**
| 能力 | 说明 |
|------|------|
| 群聊 | 同 LAN 内公开频道 |
| 私聊 | 点对点消息 |
| 建群 | 用户自建主题群 |
| 文件传输 | 局域网高速传输，断点续传 |
| AI 群助手 | 群内共享 Agent（规划中） |

### A.7 暂不实施项

| 功能编号 | 状态 | 说明 |
|---------|------|------|
| DIRECT-API | ⏸️ | 架构预留，当前仅 Gateway 模式 |
| LIC-02 | ⏸️ | 离线硬件指纹授权，当前方案已满足 |
| SANDBOX-01 | ⏸️ | Docker 安全沙箱，v2.0 靠权限系统兜底 |
| STREAM-01 | ⏸️ | 自建流媒体通道，当前依赖 Windows 原生方案 |
| LAN-CHAT-01 | ⏸️ | 局域网聊天迁入附录候选，暂不实施 |
| FTP-01 | ⏸️ | FTP 传输迁入附录候选，暂不实施 |

---


> **文档完成。** PRD v2.2.1 合并了原始 PRD v2.2.0（2568 行）、逐章补充（1147 行）和 UI 布局 Wireframe（970 行），修正了权限系统和 License 系统的描述错误，新增了 Feature Flags、Tracing、KB Store V1、Model Failover 等章节，并将 LAN Chat/FTP 迁入附录候选。
