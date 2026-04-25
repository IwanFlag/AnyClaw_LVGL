# AnyClaw LVGL - 产品需求文档 (PRD)

> 版本：v2.2.11 | 最后更新：2026-04-21

---

## 目录

- [§1 产品概览](#1-产品概览)
- [§2 安装与首次启动](#2-安装与首次启动)
- [§3 主界面](#3-主界面)
- [§4 对话 Chat](#4-对话-chat)
- [§5 工作台 Work](#5-工作台-work)
- [§6 模式体系](#6-模式体系)
- [§7 模型管理](#7-模型管理)
- [§8 Agent 能力](#8-agent-能力)
- [§9 权限系统](#9-权限系统)
- [§10 工作区管理](#10-工作区管理)
- [§11 设置](#11-设置)
- [§12 技术架构](#12-技术架构)
- [§13 系统管理](#13-系统管理)
- [附录](#附录)

---

## 1. 产品概览

### 1.1 产品定位

AnyClaw 是面向**学生**和**职场用户**的 AI 桌面管家。兼具 **通用 AI 工作台**与 **AI 管家** 两大能力——让 AI 不仅能陪聊，还能直接帮你干活，执行过程全程可见。

三个关键词：
- **简单** — 开箱即用，无需深入理解底层架构
- **安全** — 数据不离开本地，不依赖第三方平台
- **稳定** — 本地运行，不随平台策略变化而失效

**产品公式：**
```
通用 AI 工作台（Chat + Work）+ Agent 运行时（OpenClaw / Hermes）= AnyClaw
```

| 模式 | 角色 | 内容 |
|------|------|------|
| Chat 模式 | AI 管家 | 对话、提醒、查邮件、IM 集成、日常事务 |
| Work 模式 | 通用 AI 工作台 | AI 执行任务的可视化工作区——编程、写文档、数据分析、做 PPT、搜索整理，执行过程实时可见 |

**Work 模式不限于编程。** 学生写论文、职场做报告、开发者写代码——任务类型不同，但 AI 干活并展示执行过程的模式是通用的。

### 1.2 AnyClaw 与 Agent 运行时的关系

AnyClaw 是 **GUI 客户端（壳子）**，不自己实现 Agent 逻辑。它通过 Agent 运行时来获得 AI 能力。

**当前支持两种运行时，用户可在设置中切换：**

| 运行时 | 本质 | 端口 | 默认 |
|--------|------|------|------|
| **OpenClaw Agent** | 成熟的 Agent 平台（守护进程 + 内置 Agent） | :18789 | ✅ 默认 |
| **Hermes Agent** | Nous Research 开源 Agent 框架（自进化技能 + 持久记忆） | :18790 | 可选 |

**组件关系：**

| 组件 | 本质 | 类比 |
|------|------|------|
| **AnyClaw** | GUI 客户端 | 浏览器 |
| **OpenClaw** | Agent 平台（运行时 A） | 操作系统 A |
| **Hermes Agent** | Agent 框架（运行时 B） | 操作系统 B |
| **守护进程** | OpenClaw 的常驻服务 | 系统服务 |
| **AI 实例** | 运行时管理的对话实体 | 应用程序 |
| **LLM** | 底层模型 | CPU |

**为什么要两个运行时？**
- AnyClaw 提供统一的图形界面和用户体验
- 用户根据需求选择：OpenClaw 成熟稳定（权限/Cron/Heartbeat 完善），Hermes 更智能（自进化技能/持久记忆/复杂工作流）
- 两个运行时独立运行，Session 不互通，配置各自管理

**安装 AnyClaw 不代表自动安装了运行时。** 首次启动向导会引导用户安装 OpenClaw（默认运行时），并可选安装 Hermes / Claude Code CLI Worker。

**主题策略（v2.2.11）：**
- 仅保留三套主打主题：`matcha`（默认）/ `peachy` / `mochi`
- 主题切换在 Settings General 中即时生效，无需重启

> 设计 → UI-05（环境检测）

### 1.3 差异化定位

| 产品 | 价格 | IM 集成 | 编程能力 | 权限系统 | 本地模型 |
|------|------|---------|---------|---------|---------|
| nexu | 免费开源 | ✅ 微信/飞书/Slack/Discord | ❌ | ❌ | ❌ |
| QClaw（腾讯） | 免费闭源 | ✅ 微信/飞书/钉钉 | ❌ | ❌ | ❌ |
| 724Claw | 免费 | ❌ | ❌ | ❌ | ❌ |
| Cursor | Freemium | ❌ | ✅ | ❌ | ❌ |
| **AnyClaw** | **Freemium** | **🔧 规划中** | **✅** | **✅** | **✅** |

**AnyClaw 差异化：** 免费客户端 + 通用 AI 工作台（不限编程）+ 权限系统 + 原生 GUI + Agent 主动行为。

### 1.4 版本规划路线图

|| 版本 | 核心内容 | 状态 |
|------|---------|------|
| v2.0 | Chat/Work 双模式、权限系统、本地模型、向导、Model Failover | ✅ 已发布 |
| v2.1 | Diff 可视化、数据表格、Ask 模式弹窗深化 | 🔧 规划中 |
| v2.2 | 搜索结果渲染、文件操作渲染、WebRenderer | ⏳ 规划中 |
| v2.3 | 三 Agent 并行（OpenClaw Leader + Hermes + Claude Code）完整编排能力 | 🔧 规划中 |
| v3.0 | AnyClaw 自身成为 Agent 平台（Direct API）、全自主执行 | ⏳ 长期目标 |

### 1.5 明确排除项

以下功能在 v2.0~v2.2 范围内**明确不做**：

| 排除项 | 说明 | 预计版本 |
|--------|------|---------|
| Direct API 模式 | AnyClaw 直连 LLM，不经过 Agent | v3.0 |
| 离线硬件指纹授权 | 不联网的 License 验证方案 | v3.0 |
| Docker 安全沙箱 | 容器化 Agent 执行环境 | v3.0 |
| 自建流媒体通道 | 不走 Windows 原生方案的远程桌面 | v3.0 |
| 局域网聊天 | 同网络内 AnyClaw 用户直接通信 | 待定 |
| FTP 传输 | 基于 FTP 的文件传输 | 待定 |

---

## 2. 安装与首次启动

### 2.1 运行环境与部署

| 要求 | 说明 |
|------|------|
| 操作系统 | Windows 10/11 (x64) |
| 运行时依赖 | `SDL2.dll`（同目录） |
| 可选依赖 | OpenClaw Agent（未安装时引导安装） |
| 磁盘空间 | < 20 MB（不含本地模型） |
| 内存 | < 200 MB（空闲状态） |

**部署方式：** 双文件部署 — `AnyClaw_LVGL.exe` + `SDL2.dll` 放入同一目录，双击运行。

**跨平台编译：**
- Windows：MSYS2 原生编译
- Linux：MinGW-w64 交叉编译
- 构建脚本：见 `build/README.md`

**补充细节（代码实际实现）：**
- DPI 感知采用 Win32 `PER_MONITOR_AWARE_V2` 级别
- SDL 强制软件渲染
- 窗口默认尺寸为屏幕 75%×75%，最小 800×500
- IME 支持中文输入法

### 2.2 实例互斥
- **功能编号：** INST-01 **优先级：** P0
- 通过 Windows Mutex 防止重复启动
- 检测到已有实例运行 → 弹出原生 MessageBox 提示，退出程序

> 设计 → UI-02

### 2.3 启动自检
- **功能编号：** SC-01 **优先级：** P0

**两个独立模块：**

| 模块 | 职责 | 检查项数 | 调用时机 |
|------|------|---------|---------|
| SelfCheck | 快速启动检查 | 4 | 每次启动 |
| BootCheck | 全面环境体检 | 9 | 启动时自动运行（无需手动触发） |

**启动页面形态（UI-01）：**
- 启动时进入 Boot Check 页面，页面尺寸与主界面一致（同窗口尺寸），不是全屏独占系统桌面。
- 页面展示统一状态：`Pass / Fail / Unknown`，并展示关键运行时版本与可用性。
- 检查项最少包含：Node.js、npm、Network、OpenClaw、Agent（端口18789）、Hermes、Claude Code CLI。

**启动分流规则：**
- 全部 `Pass`：显示成功状态后短暂停留（>= 2.5 秒）自动进入主界面。
- 存在 `Fail`：停留在 Boot Check 页面，提供两个按钮：`进入主界面（受限）`、`进入向导修复`。
- 存在 `Unknown` 且无 `Fail`：允许进入主界面，同时在页面标记为可复检项。

**首次安装与已安装用户规则：**
- 首次安装（`wizard_completed=false`）：Boot Check 页面完成后直接进入向导流程。
- 已安装用户（`wizard_completed=true`）：Boot Check 页面完成后进入主界面；若关键项 `Fail`，允许用户改走向导。

**SelfCheck 检查项：**

| 检查项 | 检测方式 | 自动修复 |
|--------|---------|---------|
| Node.js | `node --version` ≥ 22.14 | 检查 bundled/node-win-x64.zip → 解压安装 |
| npm | `npm --version` | 尝试 `npm cache clean --force` |
| 网络连通性 | HTTP GET google.com / openrouter.ai | 不可修复，标记为离线模式 |
| 配置目录 | `%USERPROFILE%\.openclaw` 可写性 | 尝试创建目录 |

- Node.js 缺失且 bundled/ 有离线包 → 自动静默安装便携版
- Node.js 缺失且 bundled/ 无离线包 → 显示阻断弹窗，引导安装
- 无网络但 bundled/ 有 openclaw.tgz → 自动离线安装 OpenClaw
- 当 Node/npm/OpenClaw 缺失时，即使向导已完成也应在 Boot Check 页面给出明确失败，并提供一键进入向导修复

> 设计 → UI-01, UI-03

### 2.4 首次启动向导
- **功能编号：** WIZ-01 **优先级：** P1 **状态：** 🔧 规划中（双阶段）

首次体验采用双阶段模型：

- 阶段 A（首次可用闭环，强引导）：目标是在 3 分钟内让用户达到“可聊天、可执行、可见反馈”。
- 阶段 B（进阶能力引导，非阻断）：将高认知成本能力后置为推荐任务清单。

**阶段 A：4 步可用闭环（全屏遮罩 + 居中弹窗）**

|| 步骤 | 内容 | 必填 | Skip 影响 |
|------|------|------|----------|
| Step 1 | 语言选择（中文/English） | 是 | — |
| Step 2 | 环境与默认能力就绪（自动检测 + 一键修复） | 是 | 关键依赖未就绪则不可继续 |
| Step 3 | 模型 & API Key（最小闭环） | 是 | 未配置则无法聊天 |
| Step 4 | 完成页（进入主界面 + 进阶能力推荐） | 是 | — |

**阶段 B：进阶能力引导（推荐任务清单，非阻断）**

- 本地模型（Gemma）
- 连接 IM（飞书/Telegram/Discord）
- Leader 模式
- 可选运行时安装（Hermes / Claude Code CLI）

> 阶段 B 任意项可跳过，不影响基础聊天和任务执行。

**Agent 安装详情：**

|| Agent | 角色 | 安装方式 | 必须 |
|-------|------|---------|------|
| OpenClaw | Leader | npm 全局 / bundled 离线包 | ✅ |
| Hermes | Worker | 独立安装包 | 否 |
| Claude Code CLI | Worker | Anthropic 官方安装 | 否 |

**阶段 A Step 2 交互细节（环境就绪卡片）：**
- 展示统一状态：`就绪 / 修复中 / 需处理 / 失败可重试`
- 仅保留一个主操作按钮：`自动修复`
- 二级操作按钮：`稍后手动处理`
- 失败时必须展示错误摘要，并提供 `重试`

**阶段 B 运行时安装卡片（进阶能力）：**
- OpenClaw（基础运行时）卡片：状态 + 版本号 + `[在线安装（推荐）] [bundled/离线安装] [手动指定路径]`
- Hermes 卡片（可选）：`未安装/已安装` + `[安装] [卸载]`
- Claude Code CLI 卡片（可选）：路径检测 + 版本检测 + `[安装指引] [指定路径] [卸载]`
- 任一安装任务失败时在卡片内显示错误摘要，并提供 `[重试]` 按钮

**阶段 B Skip 降级矩阵：**

|| 跳过步骤 | 可用功能 | 不可用功能 |
|---------|---------|-----------|
| Step 4（本地模型） | 云端模型聊天、Work 模式全部功能 | 离线推理、无网时 AI 不可用 |
| Step 5（IM 连接） | 桌面端 Chat/Work 全部功能 | 手机远程指挥 AnyClaw |
| Step 6（Leader 模式） | 单 Agent 模式运行（仅 OpenClaw） | 多 Agent 并行编排 |

**Leader 模式（默认开启）：**
- OpenClaw 作为入口接收消息，自动拆分任务并行分发给 Hermes 和 Claude
- Skip 后切换为单 Agent 模式，可后续在 `[Agent ▼]` 中重新开启

**IM 连接详情：**
- 支持渠道：飞书（Bot AppID + App Secret）、Telegram（Bot Token）、Discord（Bot Token）
- 全部 Skip 不影响桌面端正常使用
- 价值：让手机成为 AnyClaw 的远程控制器

**可验收标准（WIZ-01）：**
1. 新用户在 3 分钟内完成阶段 A，并成功发起至少一次对话。
2. 阶段 A 不出现 Runtime/Leader 的强制决策。
3. Step 3 API Key 非法或为空时，必须显示明确错误原因，且 Next 保持禁用。
4. 任一步骤失败时必须可重试，不允许进入不可前进且不可恢复状态。
5. 阶段 B 推荐任务可在设置页复访，不影响主流程可用性。

> 设计 → UI-04 ~ UI-09

### 2.5 OpenClaw 安装

**发现机制（优先级从高到低）：**
1. config.json 中配置的路径
2. PATH 环境变量中的 `openclaw` 命令
3. npm 全局目录（`npm root -g`）
4. 常见安装位置扫描
5. 都找不到 → 引导用户安装或指定路径

**安装方式：**
- 在线安装（推荐）：自动串联 Node.js + OpenClaw 安装（需要网络）
- 离线安装：从 `bundled/` 目录安装，无需网络
- 手动指定路径：兜底方案，用户指定已安装路径（仅自动扫描全部失败时显示）

**离线安装资源（`bundled/` 目录）：**

| 文件 | 用途 | 大小 |
|------|------|------|
| `node-win-x64.zip` | Node.js v22.14.0 Windows 便携版 | ~34 MB |
| `openclaw.tgz` | OpenClaw npm 包（含全部依赖） | ~45 MB |

**离线安装流程：**

```
1. 检测到无网络 → 自动进入离线模式
2. 检查 bundled/node-win-x64.zip → 解压到 %APPDATA%\AnyClaw_LVGL\node-portable\
3. 将 node-portable\ 添加到当前进程 PATH
4. 验证 node --version ≥ 22.14
5. 检查 bundled/openclaw.tgz → npm install -g bundled/openclaw.tgz（使用便携版 node）
6. 验证 openclaw --version
7. 任一步骤失败 → 显示错误 + 提供手动指定路径选项
```

**在线安装流程（在线安装（推荐））：**

```
1. 下载 Node.js v22.x LTS .msi → 静默安装
2. npm install -g openclaw
3. 验证 node + openclaw 版本
4. 失败 → 回退到离线检测 bundled/
```

**兜底策略：**

| 场景 | 处理 |
|------|------|
| 有网 + 在线安装（推荐） 成功 | 正常流程 |
| 有网 + 在线安装（推荐） 失败 | 回退到 bundled/ 离线安装 |
| 无网 + bundled/ 存在 | 自动离线安装 |
| 无网 + bundled/ 缺失 | 引导用户手动安装或指定已安装路径 |
| 用户已有安装 | 自动扫描 PATH/npm/常见位置，找到直接标 ✅ 跳过安装 |

### 2.6 工作区初始化

首次启动（无 config.json）时：
- 默认工作区路径：`%USERPROFILE%\.openclaw\workspace\`
- 选择工作区模板（通用助手 / 开发者 / 写作者 / 数据分析）
- 自动创建目录结构和初始化文件

> 设计 → UI-10

### 2.7 新手引导（向导之后）

向导完成后进入主界面，提供渐进式引导：

| 阶段 | 触发时机 | 引导内容 |
|------|---------|---------|
| 首次进入 | 向导完成 | Chat 空状态快捷建议气泡（"试试问我：帮我查一下天气"） |
| 首次切换 Work | 用户点击 Work 标签 | Work 空状态引导（"在 Chat 中发送任务，AI 的操作将在这里展示"） |
| 首次遇到权限弹窗 | Agent 首次请求权限 | 权限弹窗旁标注说明 |
| 首次无网 | 健康检查失败 | 引导检查网络或切换本地模型 |

---

## 3. 主界面

### 3.1 窗口与布局

> 详细布局规范见 Design.md §12 主界面窗口规则。

**布局语义：**

|| 区域 | 说明 |
|--|------|------|
| 左导航栏 | 图标-only 模块切换：Bot / 文件，底部 Chat/Work 切换 + 设置 |
| 标题栏 | 品牌 + 模块名 + 窗口控制，48px 高 |
| 左侧面板 | Bot 模块独有，可用宽 × 25%（min 160px），显示 Agent 状态 / Session 列表 / Cron 列表 |
| 右侧面板 | 其余可用空间，Chat/Work 共用，显示输出区 + 控制行 + 输入区 |

**Bot 模块左右面板：**

|| 左侧面板 | 右侧面板 |
|--|---------|---------|
| Agent 状态 + 当前模型 + Session 列表 + Cron 列表 | 控制行 + 输出区 + 输入区 |

**窗口行为：**
- 关闭 → 最小化到托盘
- 双击标题栏 → 最大化/还原
- 位置记忆：重启后恢复
- 最小尺寸 800×500

> 设计 → UI-11, UI-12, UI-52~55

### 3.1.1 C2 对话与任务中心（统一入口）

为降低模式学习成本，C2 采用“一个输入入口，双结果视图”策略：

1. 输入统一
- 用户在同一输入框输入问题或任务。
- 系统自动判定请求类型：问答（Q&A）或执行任务（Task）。

2. 结果分流
- Q&A：仅展示对话结果。
- Task：展示对话摘要 + 执行轨迹（步骤、进度、产出）。

3. 模式职责调整
- Chat/Work 保留为显示层切换，不再作为用户必须先选的入口。

**可执行验收：**
1. 用户无需先手动切换 Chat/Work，即可发起并完成一次任务请求。
2. Task 类型请求必须出现可见执行进度与最终结果摘要。
3. Q&A 请求不得出现冗余执行面板。
4. 显示层切换不丢失同一会话上下文。

### 3.2 系统托盘
- **功能编号：** TRAY-01 **优先级：** P0

常驻托盘图标，四种状态（白色=空闲 / 绿色=运行中 / 红色=异常 / 黄色=检测中）。

**托盘菜单：**
- 状态行：Agent 状态 + LED 颜色
- Start/Stop Agent、Restart OpenClaw、Refresh Status
- Boot Start 开关、Open Settings、About、Exit
- 双击托盘图标：显示主窗口
- 退出确认弹窗（可配置）

> 设计 → UI-63

### 3.3 品牌与国际化

**品牌形象：**
- 欢迎语：龙虾要吃蒜蓉的 🧄🦞 — 你的AI助手已就位！
- 大蒜角色图标贯穿 UI（用户头像、标题栏、托盘）

**国际化：**
- **功能编号：** I18N-01 **优先级：** P1
- 支持 **CN / EN** 两种语言
- 首次启动自动检测系统语言，不支持则默认英文
- 切换后立即生效，无需重启
- 翻译范围：UI 文本（按钮/标签/提示/菜单）；AI 回复内容跟随用户配置的模型语言
- 日期格式：跟随语言设置（CN→YYYY-MM-DD, EN→MM/DD/YYYY）
- 数字格式：跟随语言设置

### 3.4 贴边大蒜头
- **功能编号：** SNAP-01 **优先级：** P2 **状态：** ✅ 已实现

拖拽窗口到屏幕边缘 → 隐藏主窗口 → 屏幕边缘显示浮动大蒜头（80×110px）。
悬停 → 葱头正弦波摇摆。点击 → 恢复主窗口。

> 设计 → UI-13

---

## 4. 对话 Chat

### 4.1 聊天架构
- **功能编号：** CHAT-01 **优先级：** P0

聊天走 OpenClaw Agent `/v1/chat/completions`（OpenAI 兼容），不直连 LLM API。

**通信流程：**
```
AnyClaw → POST http://127.0.0.1:18789/v1/chat/completions（带 Agent auth token）
        → OpenClaw Agent → LLM
```

Agent token 自动从 `openclaw.json` 的 `gateway.auth.token` 读取。
model 由用户在 UI 选择（默认 MiniMax M2.7-highspeed），通过 OpenClaw Agent 路由。

### 4.2 输入与消息
- **功能编号：** CI-01 **优先级：** P0 **状态：** ✅ 已实现

**输入框：**
- 多行输入，Shift+Enter 换行，Ctrl+V 粘贴
- 高度自动增长（监听行数变化），默认 96px，最小 96px
- 支持中文 IME 输入

**消息气泡与流式：**
- **功能编号：** MB-01 **优先级：** P0 **状态：** ✅ 已实现
- **布局：** Chat 模式对话像 QQ 那样，AI 消息靠左，用户消息靠右，气泡样式区分身份
- 格式：`[HH:MM:SS] [角色] 内容`
- 用户消息靠右，AI 回复靠左
- 气泡宽度自适应：短消息贴合文字宽度，长消息 70% 容器宽度换行

**AI 流式渲染：**
- 打字机效果，50ms 刷新间隔，每次追加 3 个字符
- 流式过程中软滚（40px 容差，用户回看历史时不强制拉回）
- 超时保护：30s 无数据 或 总耗时 > 45s → 强制终止，显示"⚠️ AI 回复超时，请重试。"

**上下文窗口管理：**
- AnyClaw 每次请求仅发送当前消息（+ system prompt），上下文维护由 OpenClaw/Hermes 守护进程负责
- AnyClaw 侧的 chat_history 仅用于 UI 显示气泡和磁盘持久化（`chat_history.json`），不参与构建发给 Agent 的 messages 数组
- 持久化上限 50 条，内存 buffer 上限 4096 字节
- 用户消息和 system prompt 通过 `std::string` 动态拼接，无固定长度限制

**并发会话：**
- 支持多个 Session 同时存在（主聊天 + Work 任务 + Cron 触发的 Session）
- 用户同一时间只与一个 Session 交互（左侧点击切换）
- 切换 Session 时上下文保留（每个 Session 独立维护历史）
- 正在流式输出的 Session 切到后台后继续接收，不中断

> 设计 → UI-14, UI-15, UI-16, UI-17

### 4.3 文件与多模态
- **功能编号：** MM-01 **优先级：** P0 **状态：** 🔧 实施中

**上传方式：**
- 点击输入框左侧 📎 图标 → 弹出浮层菜单（两个选项：选择文件 / 选择目录）
- 选择文件：Win32 原生多选文件对话框（`GetOpenFileNameA`，支持 `OFN_ALLOWMULTISELECT`）
- 选择目录：Win32 原生目录对话框（`SHBrowseForFolderA`）
- Ask 模式：上传前弹窗确认（`ask_mode_confirm_action`）
- 选定后路径以标签形式插入输入框，可编辑或删除后再发送

**文件处理能力矩阵：**

| 文件类型 | AI 能做什么 |
|---------|-----------|
| 文本文件（.txt/.md/.csv/.json/.xml） | 读取内容、分析、总结、改写 |
| 代码文件（.py/.js/.c/.cpp/.java 等） | 读取、审查、修改、解释 |
| 图片（.png/.jpg/.gif） | 描述内容、OCR 识别（依赖模型能力） |
| PDF | 读取文本层、总结（依赖模型能力） |
| 目录 | 列出文件结构、批量操作 |

**附件队列机制：**
- 选定文件/目录后，加入全局附件队列（`g_attachment_queue`）
- 输入框内显示附件卡片（类型图标 + 文件名/目录名 + 大小或文件数量）
- 目录附件自动统计内部文件数量
- 发送消息时附件随请求一起发送

**显示：**
- 聊天气泡中显示附件卡片（类型/名称/大小/缩略图）
- 点击可直接打开（目录→资源管理器，文件→系统默认程序）
- 路径必须通过权限系统校验

> 设计 → UI-18, UI-36

### 4.4 消息操作
- **功能编号：** MO-01 **优先级：** P1 **状态：** ✅ 已实现

- 消息文字可选中（鼠标拖拽选区）
- Ctrl+C 复制选中内容（未选中时复制整条消息）
- 聊天记录搜索

> 设计 → UI-20

### 4.5 会话管理
- **功能编号：** SM-01 **优先级：** P1 **状态：** ✅ 已实现

- 活跃会话列表（渠道/Agent/时长/Token 量）
- 单个会话终止（Abort 按钮）
- 所有会话重置
- Session 活跃阈值：5 分钟（ageMs < 300000）
- 完成的 Sub-Agent Session 保留 30 分钟后自动清理

### 4.6 Agent 主动行为
- **功能编号：** PROACTIVE-01 **优先级：** P0 **状态：** ✅ 已实现（V1）

Chat 模式下的 AI 管家不应该"你问它才答"，而应该**主动帮用户做事**。

**基本原则：读和整理自己做，写和发送必须问。**

**能力分级：**

| 类别 | 示例 | 需要用户同意？ |
|------|------|--------------|
| 整理型 | 整理 MEMORY.md、清理过期记忆、备份配置 | ❌ 自己做，做完汇报 |
| 监控型 | 检查文件变化、磁盘空间、配置一致性 | ❌ 自己查，有事才通知 |
| 应用操作型 | 读邮件、查日程、搜索文件 | ⚠️ 做之前问 |
| 执行型 | 发邮件、修改外部文件、执行命令 | ⚠️ 做之前问 |
| 破坏型 | 删除文件、修改系统配置、发送外部消息 | ✅ 必须问 |

**空闲整理范围（仅 AI 自身，不动用户文件）：**
- 整理/优化 MEMORY.md、memory 日志
- 自省缺陷、修正行为
- 技能梳理与优化
- **不碰**用户工作区文件

**触发方式：**

| 触发 | 场景 | 默认状态 |
|------|------|---------|
| 启动 | AnyClaw 启动时体检 | ✅ 默认开启 |
| 空闲 | 用户 30 分钟无操作 | ✅ 默认开启 |
| 定时 | 18:00 记忆维护 / 23:00 日终总结 | ✅ 默认开启 |
| 应用授权 | 日历/邮件授权后自动激活 | ⏳ 授权后激活 |

**通知分级：**

| 级别 | 方式 | 示例 |
|------|------|------|
| 静默 | 不通知，写日志 | 整理了 MEMORY.md |
| 摘要 | 固定时间一次性汇总 | "今天帮你做了 3 件事" |
| 提醒 | 托盘气泡 | "日历提醒：20 分钟后有会议" |
| 警报 | 弹窗 + 声音 | "磁盘空间不足 100MB" |

### 4.7 语音模式（实时语音交互）
- **功能编号：** VOICE-01 **优先级：** P1 **状态：** ⏸️ 待实施

> ⚠️ Text|Voice 切换按钮已在 v2.2.11 之前的版本中移除，控制行已精简为 `[Agent ▼] [Report] [AI行为 ▼]`。语音模式作为独立功能模块待后续实现。

---

## 5. 工作台 Work

### 5.1 核心组件
- **功能编号：** WORK-01 **优先级：** P0 **状态：** 🔧 实施中

Work 模式是 **AI 执行任务的通用可视化工作区**。不限于编程，写文档、做数据分析、搜资料、处理文件全部覆盖。

**三大组件：**

|| 组件 | 能力 | 说明 |
|------|------|------|
| 步骤流 | 实时展示 Agent 每步操作 | 解析 SSE 工具调用事件 |
| 输出区域 | 多类型渲染器 | 按任务类型自动选择渲染方式 |
| 控制行 | Agent 行为控制 | `[Agent ▼]  [Report]  [AI行为 ▼]` |

> 设计 → UI-12, UI-30

### 5.2 任务类型与渲染映射

| 任务类型 | SSE 事件特征 | 渲染器 | 版本 |
|----------|-------------|--------|------|
| 终端 | `exec` 命令 | TerminalRenderer | v2.0 |
| 文档 | `write_file` .md/.txt | PreviewRenderer | v2.0 |
| 代码 | `write_file` .py/.js/.c 等 | PreviewRenderer → DiffRenderer | v2.0 → v2.1 |
| 数据 | `exec` 含 python/pandas/csv | TableRenderer | v2.1 |
| 搜索 | `web_fetch` / `browser` | WebRenderer | v2.2 |
| 文件 | 批量 `read` / `write_file` | FileRenderer | v2.2 |

### 5.3 StepCard 交互

每步操作显示为一个 StepCard，包含操作类型图标 + 描述 + 状态。

**操作类型：** 读文件📄 / 写文件✏️ / 执行命令⚡ / 搜索🔍

**状态指示：** 进行中(绿●) / 已完成(绿✓) / 失败(红✗) / 等待确认(蓝?) / 检测中(黄旋)

**写操作特有：** [接受] [拒绝] [回退] 按钮
- 接受 → 自动写入文件
- 拒绝 → 丢弃修改
- 回退 → 恢复到之前版本

**执行失败：** 显示错误输出 + [重试] 按钮

> 设计 → UI-23, UI-24, UI-25

### 5.4 任务生命周期

```
用户发送任务 → 任务开始 → 步骤执行中 → 任务完成/失败/取消
```

| 状态 | 触发条件 | UI 表现 |
|------|---------|---------|
| 执行中 | Agent 正在操作 | 步骤流实时更新，最新 StepCard 自动滚到可见 |
| 完成 | Agent 回复"完成"或工具调用结束 | 最后一个 StepCard 标记 ✓ |
| 失败 | 工具调用报错且无法恢复 | 对应 StepCard 标记 ✗ + 错误信息 + 重试按钮 |
| 取消 | 用户点击 Abort | 停止当前执行，标记已取消 |

**结果导出：**
- Work 产出内容自动保存到工作区 `projects/` 目录
- 用户可通过 Chat 面板对话获取结果摘要
- 输出面板内容可选中复制

**多任务：**
- 同一时间只运行一个 Work 任务
- 新任务排队或提示"当前有任务运行中"

> 设计 → UI-27

### 5.5 v2.1：Diff 可视化 + 数据表格

| 新增能力 | 说明 |
|---------|------|
| Diff 可视化 | 修改前后对比，绿色新增红色删除高亮 |
| 接受/拒绝 | 点击"接受"→自动写入；"拒绝"→丢弃 |
| TableRenderer | CSV/Excel 数据表格展示 + 基础统计 |

> 设计 → UI-28

### 5.6 v3.0：全自主执行

| 新增能力 | 说明 |
|---------|------|
| 自主规划 | Agent 自主拆解任务步骤 |
| 逐步批准 | 每步可单独批准/拒绝，或全部自动通过 |
| 终端验证 | Agent 自动跑测试/编译，根据输出迭代修改 |

---

## 6. 模式体系

AnyClaw 有三套模式维度，各自独立，组合使用。

### 6.1 界面模式（Chat / Work）
- **功能编号：** MODE-01 **优先级：** P1 **状态：** 🔧 实施中

|| 模式 | 角色 | 侧重 |
|------|------|------|
| Chat | AI 管家 | 对话、日常事务、文件操作 |
| Work | AI 工作台 | 任务执行可视化、步骤流、输出渲染 |

**Chat/Work 切换：** 单按钮，放置于左导航栏底部，图标切换。切换延迟 < 150ms，共享上下文。

**切换规则：**
- 上下文完全互通
- 切换不中断正在运行的任务（Work 任务后台继续）
- Chat 和 Work 共享 chat_history、MEMORY.md、工具调用

> 设计 → UI-21

### 6.2 AI 交互模式（自主 / Ask / Plan）
- **功能编号：** AIMODE-01 **优先级：** P0 **状态：** ✅ 已实现（V1）

**控制行（Chat / Work 通用）：**

```
┌──────────────────────────────────────────────────────────┐
│  [Agent ▼]  [AI行为 ▼]  [Report]                [状态标签]│
└──────────────────────────────────────────────────────────┘
```

**Agent 下拉：** 切换运行时，支持 OpenClaw（默认）/ Hermes / Claude Code。切换前检测安装状态，未安装则弹 Toast 提示并回退到 OpenClaw。即时生效，持久化到 config.json。

**Report 按钮：** 点击打开上次 BootCheck 诊断报告（新窗口或面板）。

**AI行为 下拉：** 三个选项，含义和触发规则如下：

|| 模式 | 含义 | 触发规则 |
|------|------|---------|
| **自主模式**（默认） | AI 全自主判断与执行，不打断 | — |
| **Ask 模式** | 自主执行，遇到决策点弹窗询问 | 遇到以下操作时弹窗：写文件、删除文件、执行命令、发送外部消息、修改系统配置 |
| **Plan 模式** | 先出完整方案与用户讨论，确认后自动执行 | 收到任务后先输出步骤清单，等待用户确认 |

**决策点定义（Ask 模式触发规则）：**

| 操作类型 | 是否弹窗 | 说明 |
|---------|---------|------|
| 读取工作区文件 | ❌ | 自主执行 |
| 写入工作区文件 | ✅ | 需确认内容和目标路径 |
| 删除文件 | ✅ | 需确认文件名 |
| 执行 shell 命令 | ✅ | 需确认命令内容 |
| 发送外部消息（邮件/IM） | ✅ | 需确认收件人和内容 |
| 修改系统配置 | ✅ | 需确认修改项 |
| 搜索/网络请求 | ❌ | 自主执行 |

**Plan 模式输出格式：**
```json
{
  "type": "plan",
  "steps": ["步骤1描述", "步骤2描述", "..."],
  "risk": "low/medium/high",
  "est_time": "~30s"
}
```

**模式切换：** Chat 面板和 Work 面板内均可切换，即时生效，持久化到 config.json。

**Agent 人格：** Chat 和 Work 决定 Agent 的侧重和暴露工具集合——Chat 为管家人格（友好、主动），暴露邮件/日历/搜索/IM/定时任务；Work 为执行人格（严谨、高效），暴露代码/终端/文件操作/搜索/数据分析。Agent 人格由 SOUL.md 统一定义，MEMORY.md 跨模式共享。

> 设计 → UI-22, UI-29, UI-32

### 6.3 控制权模式（用户 / AI）

|| 模式 | 操作者 | 说明 |
|------|--------|------|
| **用户控制**（默认） | 人类用户 | 用户手动操作鼠标键盘，AI 仅建议 |
| **AI 托管** | Agent | Agent 模拟用户操作鼠标键盘来控制 AnyClaw UI（如通过 Mano/CUA） |

**核心约束：** AI 拿到 UI 控制权后，必须模拟真实用户的鼠标键盘操作，没有捷径 API。

- 用户可随时一键回退到用户模式
- AI 模式下所有高风险动作仍走权限检查与审计

### 6.4 键盘快捷键
- **功能编号：** KB-01  **优先级：** P1  **状态：** ✅ 已实现

全局快捷键，任意界面均有效（文本编辑中性质冲突的除外）。

**导航快捷键：**

| 快捷键 | 功能 | 生效范围 | 冲突场景 |
|--------|------|---------|---------|
| `Ctrl+1` | 切换到 Bot 模块 | 全局 | 数字输入框中禁用 |
| `Ctrl+2` | 切换到 Files 模块 | 全局 | 数字输入框中禁用 |
| `Ctrl+Tab` | 循环切换导航模块 | 全局 | Tab 切换控件中禁用 |
| `Ctrl+T` | 切换 Chat/Work 模式 | 仅 Bot 模块 | 其他模块不响应 |

**界面快捷键：**

| 快捷键 | 功能 | 生效范围 | 冲突场景 |
|--------|------|---------|---------|
| `Ctrl+,` | 打开设置界面 | 全局 | 设置页本身禁用 |
| `Ctrl+Shift+T` | 循环切换主题（Matcha→Peachy→Mochi→Matcha） | 全局 | 无冲突 |
| `Ctrl+N` | 新建对话（Bot·Chat 模式，清空输入框） | 仅 Chat 模式 | 文本编辑时转为发送（Ctrl+Enter） |

**窗口快捷键：**

| 快捷键 | 功能 | 生效范围 | 冲突场景 |
|--------|------|---------|---------|
| `Ctrl+M` | 最小化窗口 | 全局 | 无冲突 |
| `F11` | 最大化/还原窗口 | 全局 | 无冲突 |
| `Ctrl+W` | 最小化到系统托盘（不退出进程） | 全局 | 无冲突 |

**文本编辑快捷键（已有）：**

| 快捷键 | 功能 |
|--------|------|
| `Ctrl+C` | 复制选中文本 |
| `Ctrl+V` | 粘贴文本 |
| `Ctrl+X` | 剪切选中文本 |
| `Ctrl+A` | 全选文本 |
| `Ctrl+Enter` | 发送消息 |

**交互细节：**
- 快捷键事件优先于应用逻辑处理
- 主题切换后即时渲染，无需重启
- 托盘关闭后进程保持运行，可通过托盘图标恢复
- 状态标签（`mode_lbl_chat_status`）显示当前 Agent 运行时状态

> 设计 → UI-KS

---

## 7. 模型管理

### 7.1 模型架构概述

```
AnyClaw (GUI)
    │
    ▼
OpenClaw Agent（路由层）
    │
    ├── MiniMax M2.7-highspeed（默认）
    ├── OpenRouter（可选，25+ 模型）
    └── llama-server（本地模型）
```

| 层 | 职责 | 配置位置 |
|------|------|---------|
| AnyClaw | 提供模型选择 UI、API Key 输入 | `config.json` |
| 守护进程 | 路由到配置的 Provider 和模型 | `openclaw.json` |
| AI 实例 | system prompt、工具、行为规则 | SOUL.md + AGENTS.md |
| LLM | token in → token out | Provider API / 本地推理 |

### 7.2 云端模型
- **功能编号：** MODEL-01 **优先级：** P0 **状态：** ✅ 已实现

**模型选择：**
- 下拉列表带搜索框，输入即可过滤模型，按能力排序，默认 21 个模型，**默认模型为 MiniMax M2.7-highspeed**
- 支持添加自定义模型（"+" 按钮 → 输入模型名称）
- OpenRouter 上 25 个 `:free` 模型，国内直连可达

**Provider 与 API Key：**
- 切换模型自动检测 Provider（openrouter/xiaomi）
- 所有 OpenRouter 模型共用一个 API Key
- API Key 存储：`config.json` 明文存储（当前实现），建议用户不在公共设备上使用
- Save 同步 Agent：`openclaw config set` → 重启 Agent

**Agent 同步流程：**
```
用户修改模型/API Key → AnyClaw → openclaw config set → Agent 自动重启
```

> 设计 → UI-47

### 7.3 本地模型
- **功能编号：** LOCAL-01 **优先级：** P1 **状态：** 🔧 实施中

**支持的模型：**

| 模型 | 参数量 | 内存要求 | 适用场景 |
|------|--------|---------|---------|
| Gemma 3 E4B | ~4B | 8 GB RAM | 入门级，聊天、简单任务 |
| Gemma 3 26B MoE | ~26B | 16 GB RAM | 中档，代码、分析 |

格式：GGUF，推理引擎：内嵌 llama-server 后台进程（`llama-server.exe`，端口 18800）。

**下载管理：**

| 能力 | 说明 |
|------|------|
| 多源回退 | HuggingFace → ModelScope → 镜像站，自动切换 |
| 下载进度 | 速度、已下载/总大小、百分比、预计剩余时间 |
| 断点续传 | 支持中断后继续下载 |
| 取消中断 | 用户可随时取消 |
| 完整性校验 | 下载完成后 sha256 校验 |
| 磁盘预估 | 选装前显示所需空间和剩余空间 |

**已安装模型管理：**

| 能力 | 说明 |
|------|------|
| 已安装列表 | 模型名、版本号、大小、安装时间 |
| 模型切换 | 切换当前激活的本地模型 |
| 推理状态监控 | llama-server 进程存活、内存占用、推理速度(tokens/s) |
| 卸载/清理 | 删除 GGUF 文件，释放磁盘空间 |

**云端 ↔ 本地统一切换：**

| 能力 | 说明 |
|------|------|
| 统一模型列表 | 云端 + 本地合并展示在一个下拉列表中 |
| 一键切换 | 点击即切换当前使用的模型 |
| 状态指示 | 云端=在线连接 / 本地=离线推理中 |
| 自动降级 | 云端不可用时 fallback 到本地（如果启用 Failover） |

**配置持久化：**
- `gemma_install_opt_in`（bool）— 是否启用本地模型
- `gemma_model_mask`（bitmask）— bit0=E4B, bit1=26B

> 设计 → UI-07, UI-42

### 7.4 Model Failover 弹性通道
- **功能编号：** FAILOVER-01 **优先级：** P1 **状态：** ✅ 已实现

当前模型不可用时自动切换到健康的备份模型。

**评分公式：**
```
score = 0.6 × stability + 0.25 × speed + 0.15 × capability
```

| 参数 | 值 |
|------|-----|
| 检查间隔 | 5 分钟 |
| 探针超时 | 12 秒 |
| 连续失败阈值 | 3 次触发冷却 |
| 冷却时间 | 10 分钟 |

> 设计 → UI-47（Failover 区域）

### 7.5 平台更新

| 更新对象 | 方式 | 说明 |
|---------|------|------|
| AnyClaw | GitHub Releases 自动检测 + 一键下载覆盖 | 托管仓库 `IwanFlag/AnyClaw_LVGL` |
| OpenClaw | `openclaw update` 命令 | 通过 CLI 子进程调用 |
| 更新后 | 自动重启 Agent | 不阻塞 UI |

> 设计 → UI-57

---

## 8. Agent 能力

### 8.1 内置能力

Agent 默认具备以下能力（通过 OpenClaw 工具系统实现）：

| 能力 | 工具 | 说明 |
|------|------|------|
| 文件读写 | `read`, `write`, `edit` | 工作区内文件操作 |
| 命令执行 | `exec` | 运行 shell 命令 |
| 网络搜索 | `mimo_web_search` | 实时搜索 |
| 网页抓取 | `web_fetch` | 获取 URL 内容 |
| 浏览器控制 | `browser` | 自动化网页操作 |
| 定时任务 | `cron` | 创建和管理定时任务 |
| 消息发送 | `message` | 通过配置的渠道发送消息 |
| 记忆管理 | `memory_search`, `memory_get` | 查询和管理长期记忆 |
| 子代理 | `sessions_spawn` | 派出独立子代理执行任务 |
| 画布 | `canvas` | 展示 UI 内容 |

### 8.2 Skill 体系
- **功能编号：** SK-01 **优先级：** P1

**Skill 是什么：** 扩展 Agent 能力的模块，包含指令文件（SKILL.md）和可选的脚本/工具。

**来源：**

| 来源 | 说明 | 安全性 |
|------|------|--------|
| 内置（Built-in） | 随 AnyClaw 预装 | ✅ 官方维护 |
| ClawHub | 社区仓库（clawhub.com） | ⚠️ 社区提交，有审核 |

**管理能力：**
- 搜索与浏览（ClawHub 仓库）
- 批量安装 / 更新 / 卸载
- 启用 / 禁用（不卸载）
- 版本管理

**安全机制：**
- ClawHub Skill 安装前显示权限声明
- 安装到 `skills/` 目录，受工作区权限系统约束
- 不允许 Skill 修改 AGENTS.md/SOUL.md 的 MANAGED 区域

> 设计 → UI-65

### 8.3 外部应用集成

Agent 需要操作用户已有的桌面应用来提供管家服务。

**自动检测与推荐：** 扫描系统已安装应用，自动检测可用交互方式，推荐最安全方案。

**交互方式优先级：** CLI > MCP > COM/API > Skill > UI 自动化

**已规划集成：**

| 应用 | 能力 | 交互方式 | 状态 |
|------|------|---------|------|
| 邮件（Outlook/Thunderbird） | 读邮件、摘要、发送 | CLI / COM | ⏳ 待实施 |
| 日历（Google Calendar/Outlook） | 查日程、提醒 | API / MCP | ⏳ 待实施 |
| 文件搜索 | 全局搜索文件内容 | CLI | ⏳ 待实施 |
| VS Code | 打开文件、跳转行号 | COM | ⏳ 待实施 |

**用户分层：** 普通用户看到授权/拒绝按钮；进阶用户可展开选择方式；开发者可完整配置。

> 设计 → UI-43

### 8.4 记忆与知识

Agent 有三层记忆机制，各自边界清晰：

| 层级 | 机制 | 存储位置 | 生命周期 | 用途 |
|------|------|---------|---------|------|
| **对话记忆** | Session 上下文 | Agent 内存 | Session 级 | 当前对话的上下文 |
| **长期记忆** | MEMORY.md + memory/ 日志 | 工作区文件 | 跨 Session | 用户偏好、决策记录、经验教训 |
| **知识库** | KB 索引 | 本地文件 | 持久化 | 用户导入的参考资料 |

**对话记忆管理：**
- 上下文由 Agent/Agent 运行时管理，AnyClaw 仅发送当前消息（见 §4.2）
- Session 结束后不自动持久化到文件（除非 Agent 主动写入 MEMORY.md）

**长期记忆维护：**
- 每天 18:00（OpenClaw Cron）自动整理 memory/ 日志 → 更新 MEMORY.md
- 空闲时（30 分钟无操作）Agent 自主整理 MEMORY.md
- MEMORY.md 只保留精炼信息，原始日志在 memory/ 目录

**知识库（KB）：**
- 用户添加源文件 → 本地索引构建
- Agent 对话时自动检索相关上下文（关键词匹配 + 评分排序）
- 评分公式：`score = 100 + frequency×15 + position_bonus + filename_bonus`

**记忆边界规则：**
- MEMORY.md **不在共享/群聊上下文中加载**（安全考虑）
- 对话记忆不跨 Session 共享
- 知识库内容对所有 Session 可见

> 设计 → UI-60

### 8.5 子代理与多任务

| 概念 | 说明 |
|------|------|
| 主 Agent | 与用户直接交互的代理 |
| 子代理 | 由主 Agent 通过 `sessions_spawn` 派出的独立代理 |
| 深度限制 | 最多 1 层，子代理不能再派子代理 |
| 结果通知 | 完成时自动 push 结果到主 Agent |

**用户可见性：**
- 左侧面板 Session 列表中标记子代理（isSubAgent 标记）
- 子代理运行时显示状态（运行中/已完成）
- 用户可终止子代理（Abort 按钮）

---

## 9. 权限系统

### 9.1 权限模型
- **功能编号：** PERM-01 **优先级：** P0 **状态：** 🔧 核心已实现

细粒度逐项控制，19 项权限键，每项独立设置为 Allow/Deny/Ask/ReadOnly。

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

### 9.2 工作区边界（始终生效）

无论哪种模式，工作区边界始终强制：
1. Agent 文件操作限定在工作区根目录内
2. 路径穿越（`../`）拒绝
3. 系统目录（`C:\Windows\`、`C:\Program Files\`）始终禁止写入

### 9.3 弹窗确认机制

当 Agent 请求超出当前权限设置的操作时：

**弹窗返回值：**
- 用户拒绝 → 本次操作取消
- 此次授权 → 仅本次允许，不修改权限配置
- 永久授权 → 修改权限值为 Allow 并保存

**超时：** 60 秒无响应自动拒绝

**降级：** UI 不可用时 fallback 到 Win32 MessageBox

> 设计 → UI-31

### 9.4 Ask 权限交互流程

当权限值为 Ask 时，Agent 请求操作的完整流程：

```
Agent 请求操作
    → 检查权限值
    → Ask → 弹出权限确认弹窗
         → 用户拒绝 → 操作取消
         → 用户允许（本次） → 执行，不改配置
         → 用户允许（永久） → 改权限为 Allow，执行
    → Allow → 直接执行
    → Deny → 拒绝，通知 Agent
```

与 §6.2 Ask 模式的区别：
- **权限 Ask**：权限系统层面的拦截，针对特定权限键
- **Ask 模式**：Agent 行为层面的主动询问，针对决策点
- 两者可叠加：Ask 模式下 + Ask 权限 = 双重确认

### 9.5 审计日志

权限决策记录到 `%APPDATA%\AnyClaw_LVGL\audit.log`：
```
[2026-04-07 19:20:05] action=exec_check target=pip install pandas decision=allow_once
[2026-04-07 19:20:07] action=path_check target=C:\Windows\system32\hosts decision=deny:denied_path
```

### 9.6 资源上限

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

### 9.7 权限继承

| 场景 | 权限来源 |
|------|---------|
| 主 Agent 操作 | 用户在 Permissions Tab 配置的权限 |
| 子代理操作 | 继承主 Agent 的权限配置 |
| Cron 触发的任务 | 继承主 Agent 的权限配置 |
| IM 渠道远程操作 | 与桌面端相同权限（同一 Agent） |

### 9.8 免费版 vs 专业版权限差异

| 能力 | 免费版 | 专业版 |
|------|--------|--------|
| 权限配置 | 基础项（文件/命令/网络） | 全部 19 项 |
| 审计日志 | ❌ | ✅ |
| 资源上限调整 | 固定默认值 | 可自定义 |
| 权限导入/导出 | ❌ | ✅ |

> 设计 → UI-46

---

## 10. 工作区管理

### 10.1 概念与目录结构
- **功能编号：** WS-01 **优先级：** P0 **状态：** 🔧 核心已实现

**工作区 (Workspace)** 是 OpenClaw Agent 的活动边界。Agent 所有文件操作限定在工作区根目录内。

**四个目录，各有归属：**

| 目录 | 归属 | 性质 |
|------|------|------|
| AnyClaw 安装目录 | AnyClaw 程序本体 | 只读 |
| AnyClaw 数据目录 | config.json、日志、缓存（`%APPDATA%\AnyClaw_LVGL\`） | 可读写 |
| OpenClaw 安装目录 | OpenClaw 运行时 | 只读 |
| OpenClaw 工作区 | Agent 活动范围 | 可读写 |

**工作区目录结构：**
```
工作区根目录/
├── WORKSPACE.md          ← 工作区元信息
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

各模板文件默认内容见 `bundled/workspace/` 目录。

### 10.2 多工作区支持

用户可创建多个工作区，每个工作区独立配置。

**工作区隔离：**

| 资源 | 隔离方式 |
|------|---------|
| MEMORY.md | 每个工作区独立 |
| memory/ 日志 | 每个工作区独立 |
| skills/ | 每个工作区独立 |
| workspace.json | 每个工作区独立 |
| config.json（AnyClaw 配置） | 全局共享 |
| openclaw.json（Agent 配置） | 全局共享 |

### 10.3 健康检查

| 检查项 | 未满足处理 |
|--------|-----------|
| 工作区目录是否存在 | 提示创建或更换 |
| 磁盘空间 > 100MB | 警告 |
| 目录可写 | 降级为只读模式 |
| 是否被其他实例占用（lock 文件） | 检测进程存活，死亡则清理 |
| 目录结构完整 | 引导创建缺失的核心文件 |

### 10.4 工作区锁

- 启动时创建 `.openclaw.lock`，写入 PID
- 退出时删除 lock 文件
- 检测到已有 lock → 判断 PID 是否存活 → 存活则拒绝，死亡则覆盖

> 设计 → UI-39

### 10.5 迁移（导出/导入）

**导出：** AnyClaw 配置 + OpenClaw 工作区全量 → `AnyClaw_Migration_{version}_{timestamp}.zip`
**安全：** API Key 不导出明文（`api_key` → `""`, `token` → `"__REDACTED__"`）
**导入：** 路径自动适配（检测当前用户路径）

### 10.6 数据管理

| 能力 | 说明 |
|------|------|
| 聊天记录导出 | 将 chat_history 导出为 JSON/Markdown |
| 工作区备份 | 手动/定时备份工作区到指定位置 |
| 迁移 | 完整的导出/导入（见 §10.5） |

**备份策略：**
- `.backups/` 目录自动保留 AGENTS.md/TOOLS.md 最近 10 份修改备份
- 文件名格式：`AGENTS.md.20260407_192005.bak`

### 10.7 目录消失处理

工作区目录不可达时：提示用户重新创建、选择其他目录或创建新工作区。

> 设计 → UI-40, UI-41, UI-64

---

## 11. 设置

### 11.1 Settings 结构

| Tab | 内容 |
|-----|------|
| General | 开机自启动(boot_start) / 崩溃自重启(auto_start) / 语言 / 主题 / 退出确认 / Reconfigure Wizard |
| Agent | Runtime/Leader/可选 Agent 安装状态、启用开关、路径与端口配置 |
| Permissions | 19 项权限编辑（Allow/Deny/Ask/ReadOnly） |
| Model | 当前模型 / 模型列表 / API Key / Failover |
| Skill | 扩展能力模块管理（ClawHub / 内置） |
| KB | 知识库入口（Agent 中心 C3 二级 Tab） |
| Log | 日志开关 / 级别 / 过滤 / 导出 / 清除 |
| Feature | Feature Flags 开关（实验项） |
| Tracing | 追踪与观测配置 |
| About | 品牌信息 / 版本 / Tech Stack / 配置导入导出 / 迁移 |

> 设计 → UI-45, UI-46, UI-47, UI-48, UI-48A, UI-49

### 11.1.1 用户视角 IA（4 大中心）

为降低新用户认知成本，设置页从“功能平铺”重构为“用户任务分区”：

1. 开始使用中心（C1）
- 范围：General + Model
- 目标：快速可用（语言/主题/基础模型/API）

2. 对话与任务中心（C2）
- 范围：主界面入口，不在设置页首层承载
- 目标：统一输入与任务反馈，减少模式切换心智负担

3. Agent 中心（C3）
- 范围：Agent + Permissions + Skill + KB（二级 Tab）
- 目标：运行时与执行策略治理

4. 系统与诊断中心（C4）
- 范围：Log + Feature + Tracing + About
- 目标：稳定性与诊断能力集中

**曝光策略：**
- C1/C2 面向高频用户路径，优先展示。
- C3/C4 面向进阶与诊断，默认低曝光。

### 11.1.2 C1（开始使用中心）可执行验收

**范围：** General + Model 合并视图。

**必备能力：**
1. 一键完成环境就绪检查与修复。
2. 完成推荐模型选择与 API Key 配置。
3. 保存后立即执行连接测试并返回结果。

**硬性规则：**
1. API Key 为空或格式不合法时，保存按钮必须禁用。
2. 修复任务运行中，保存与重复修复按钮必须禁用。
3. 失败场景必须出现错误摘要与重试入口。
4. C1 首层不得出现 Runtime/Leader 强制决策。

**通过标准：**
1. 新用户在 C1 内 2 分钟完成设置并触发一次成功测试。
2. 用户可在不进入 Agent/诊断中心的情况下完成基础可用配置。
3. 所有失败流程均可恢复，不出现不可前进状态。

### 11.1.3 C3（Agent 中心）可执行验收

**范围：** Agent + Permissions + Skill + KB（二级 Tab）。

**必备能力：**
1. 运行时切换（OpenClaw/Hermes/Claude）可保存、可感知、可回滚。
2. Leader 与权限策略统一在同中心治理。
3. 可选 Agent（Hermes/Claude）支持安装、重检、失败重试。

**硬性规则：**
1. 运行时切换后必须提示副作用：仅影响新会话，当前会话不被强制中断。
2. 保存前后必须有状态反馈（成功/失败/冲突），且具备重试入口。
3. 当配置冲突（如选 Claude 但路径/可执行不可用）时，保存按钮禁用。
4. 回滚操作必须一键恢复到上一个已提交运行时。

**通过标准：**
1. 用户可在 1 分钟内完成运行时切换并看到明确生效提示。
2. 任一安装失败场景可重试，不需要重启应用。
3. 配置冲突时不会写入错误配置。

### 11.1.4 C4（系统与诊断中心）可执行验收

**范围：** Log + Feature + Tracing + About。

**必备能力：**
1. 日志查看与过滤可在同中心完成。
2. Feature/Tracing 开关可保存并立即反馈状态。
3. About 提供版本、导出、迁移入口。

**硬性规则：**
1. 诊断能力默认低曝光，不干扰 C1/C2 主路径。
2. 日志清空、导出等高影响操作必须有确认反馈。
3. Tracing/Feature 的实验项需标注风险级别（实验/调试）。

**通过标准：**
1. 普通用户不进入 C4 也不影响基础使用。
2. 高级用户可在 C4 完成完整排障闭环（观察 -> 调参 -> 验证）。

### 11.2 配置存储

| 文件 | 路径 | 管理方 | 说明 |
|------|------|--------|------|
| `config.json` | `%APPDATA%\AnyClaw_LVGL\` | AnyClaw | 主配置 |
| `permissions.json` | `%APPDATA%\AnyClaw_LVGL\` | AnyClaw | 权限 Source of Truth |
| `workspace.json` | 工作区根目录 | AnyClaw | 运行时权限投影 |
| `openclaw.json` | `%USERPROFILE%\.openclaw\` | OpenClaw | Agent 配置 |

### 11.3 config.json 完整字段清单

```json
{
  "wizard_completed": false,
  "language": "en",
  "theme": "matcha",
  "window_x": 100,
  "window_y": 100,
  "dpi_scale": 1.0,
  "boot_start": false,
  "auto_start": false,
  "exit_confirmation": true,
  "close_openclaw_on_exit": false,
  "ai_interaction_mode": "autonomous",
  "license_seq": 0,
  "license_expiry": 0,
  "gemma_install_opt_in": false,
  "gemma_model_mask": 0,
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

---

## 12. 技术架构

### 12.1 平台概述

AnyClaw 支持三种 Agent 运行时并行，由 OpenClaw 担任 Leader 角色统一编排：

```
AnyClaw (GUI)
  ├→ Chat/SSE    → HTTP POST + SSE  → OpenClaw (Leader) → Agent → LLM API
  │                                        ├→ Hermes Agent (Worker)
  │                                        └→ Claude Code CLI (Worker)
  ├→ Health      → HTTP GET /health → OpenClaw :18789
  ├→ Sessions    → CLI 子进程调用   → openclaw gateway call
  ├→ Config      → CLI 子进程调用   → openclaw config set/get
  ├→ License     → 本地内存验证     → 不走运行时
  ├→ KB 检索     → 本地文件搜索    → 不走网络
  ├→ Failover    → HTTP POST       → OpenRouter API（直连）
  └→ 本地模型    → HTTP POST + SSE  → llama-server :18800 → GGUF
```

**三 Agent 架构：**

```
                    ┌── OpenClaw :18789（Leader，默认）
                    │       │
AnyClaw → 统一入口 ─┤       ├──→ Hermes Agent :18790（Worker）
                    │       │
                    │       └──→ Claude Code CLI（Worker，stdio）
                    │
                    └── 单 Agent 模式（OpenClaw / Hermes / Claude 独立运行）
```

**Leader 模式（默认）：**
- OpenClaw 作为入口，接收用户消息后分析任务复杂度
- 轻量任务（闲聊、简单问答）→ OpenClaw 独立完成，直接回复
- 复杂任务（编程、搜索、多步骤）→ 并行分发给 Hermes 和 Claude Code
- 汇总结果后返回给用户
- 支持任务分工策略：按能力分配（Hermes → 日常/搜索，Claude → 编程/代码）

**单 Agent 模式：**
- 仅运行选定的 Agent，不做编排
- 由 `[Agent ▼]` 下拉框切换（OpenClaw / Hermes / Claude）

### 12.1.5 运行时系统

**功能编号：** RT-01 **优先级：** P1 **状态：** 🔧 规划中

AnyClaw 通过运行时抽象层管理 Agent 生命周期。用户选择运行时后，所有聊天/工具调用/Session 管理均走对应运行时。

**运行时对比：**

|| 维度 | OpenClaw（Leader） | Hermes Agent | Claude Code CLI |
|------|------|-----------------|--------------|
| 角色 | Leader（入口+编排） | Worker | Worker |
| 开源协议 | Apache 2.0 | MIT | Anthropic EULA |
| Agent 人格 | SOUL.md / AGENTS.md 文件驱动 | 内置人格系统 + 可配置 | 内置（Anthropic） |
| 工具系统 | exec/read/write/web_fetch/browser/cron/message 等 | 自有工具 + 可扩展 | 已有工具集 |
| 权限系统 | AnyClaw 19 项权限 + 工作区沙箱 | Hermes 原生权限 | Claude Code 内置 |
| Session | Agent 内存管理，支持多 Session | Hermes Session 管理 | 独立进程 |
| Sub-Agent | sessions_spawn，深度 1 层 | Hermes 原生子代理 | 不支持 |
| 记忆 | MEMORY.md + memory/ 日志 | 持久记忆 + 自动技能生成 | 临时 Session |
| 定时任务 | Cron 系统（cron list/add/remove） | 待定 | 不支持 |
| IM 集成 | 飞书/Telegram/Discord | 待定 | 不支持 |
| 推荐模型 | 任意（OpenRouter/xiaomi 等） | MiMo-V2（已官方集成） | Claude（官方） |
| 成熟度 | ✅ 稳定 | 🔧 早期集成 | ✅ 稳定 |

**切换逻辑：**

```
用户在 [Agent ▼] 切换
  → Leader 模式：OpenClaw 保持运行，动态连接/断开 Hermes & Claude
  → 单 Agent 模式：停止编排，仅运行选定的 Agent
  → 更新 config.json 的 active_runtime 字段
  → Session 不迁移（各自独立）
```

**配置字段（config.json）：**

```json
{
  "active_runtime": "openclaw",
  "agent_mode": "leader",           // "leader" | "single"
  "single_agent": "openclaw",       // "openclaw" | "hermes" | "claude"
  "hermes_port": 18790,
  "hermes_enabled": false,
  "claude_code_path": ""            // claude.exe 路径，空则未配置
}
```

**首次启动向导更新：**
- Step 2（环境检测）增加 Agent 安装：OpenClaw（必须）+ Hermes（可选）+ Claude Code CLI（可选）
- 向导完成后默认启用 Leader 模式

### 12.2 运行时守护进程

当前活跃运行时由 `config.json` 的 `active_runtime` 字段决定。

OpenClaw Agent 作为 Leader 运行，负责接收用户消息、拆分任务、并行分发给 Hermes 和 Claude Code，最后汇总结果。

#### 12.2.1 OpenClaw Agent（Leader）

**进程管理：**
- 一键启动、停止、重启（`openclaw gateway start/stop`）
- 进程检测：`CreateToolhelp32Snapshot` 遍历进程列表

**健康监控：**
- 检测频率：每 30 秒（可配置 5s/15s/30s/60s/120s）
- 自动重启：Agent 进程不存在 + HTTP 无响应 → 自动重启一次，失败后不再重试
- 三级状态：Ready(🟢) / Busy(🟡) / Error(🔴)

**状态机：**
```
Agent 进程不存在 + HTTP 无响应:
  ├── 从未自动重启 → 执行 app_start_gateway()，状态变 Checking
  └── 已自动重启 → 状态变 Error，不再重试

Agent 进程存在 + HTTP 无响应:
  ├── 连续失败 < 3 → 状态保持 Checking
  └── 连续失败 ≥ 3 → 状态变 Error

Agent 进程存在 + HTTP 200:
  ├── 有活跃 Session → Busy
  └── 无活跃 Session → Ready
```

### 12.3 网络通信

|| 场景 | 目标（Leader 模式） | 目标（单 Agent 模式） | 协议 | 超时 |
|------|----------------------|-----------------------|------|------|
| 聊天（主入口） | OpenClaw :18789（统一入口） | 各自 Agent 端口 | HTTP POST + SSE, OpenAI 格式 | 流式 45s |
| Worker 连接 | Hermes :18790 / Claude stdio | — | HTTP / Stdio | — |
| 健康检查 | OpenClaw :18789 | 各自 Agent 端口 | HTTP GET /health | 3s |
| 会话管理 | openclaw CLI | openclaw CLI | 子进程 + JSON | 12s |
| Failover 探针 | OpenRouter API | OpenRouter API | HTTP POST, OpenAI 格式 | 12s |
| 配置更新 | openclaw CLI | openclaw CLI | `openclaw config set` | 8s |

**Leader 模式通信流程：**
```
用户消息 → AnyClaw → OpenClaw :18789
                         ↓ 分析任务复杂度
              ┌────────────┴────────────┐
          轻量任务                      复杂任务
          OpenClaw 独立完成              并行分发给 Worker
               ↓                           ↓
          直接回复                     ┌───┴───┐
               ↓                  Hermes  Claude
               ↓                     ↓       ↓
               └────── 汇总结果 ←───┘
                         ↓
                    返回 AnyClaw → 用户
```

**Agent 通信协议（OpenAI 兼容）：**
```http
POST http://127.0.0.1:18789/v1/chat/completions
Content-Type: application/json
Authorization: Bearer ***

{
  "model": "openclaw:main",
  "messages": [...],
  "stream": true
}
```

### 12.4 安全与隐私

**用户可理解的隐私说明：**

| 数据 | 存在哪 | 传到哪 | 谁能看到 |
|------|--------|--------|---------|
| 对话内容 | Agent 内存 + chat_history buffer | LLM API（OpenRouter/xiaomi 等） | 用户 + LLM Provider |
| 配置文件 | 本地 config.json | 不上传 | 仅用户 |
| API Key | 本地 config.json（明文） | Agent 中转到 Provider | 仅用户 |
| 工作区文件 | 本地磁盘 | Agent 操作时可能发给 LLM | 用户 + Agent（受权限约束） |
| MEMORY.md | 本地磁盘 | **不上传**（不在群聊上下文中加载） | 仅用户 |
| 审计日志 | 本地 audit.log | 不上传 | 仅用户 |

**安全机制：**
- API Key 通过 `secure_zero()` 在退出前擦除内存
- 迁移导出时自动脱敏
- 审计日志不记录密钥内容
- 工作区沙箱：路径穿越检测 + 系统目录写入永久禁止
- 进程隔离：外部命令通过 `CreateProcessA` + `CREATE_NO_WINDOW`

### 12.5 性能约束

| 配置 | 最低（云端） | 推荐（本地 E4B） | 高配（本地 26B） |
|------|------------|----------------|-----------------|
| CPU | 双核 2GHz+ | 四核 2.5GHz+ | 八核 3GHz+ |
| 内存 | 4 GB | 8 GB | 16 GB |
| 磁盘 | 500 MB | 5 GB | 20 GB+ |

**关键性能常量：**
- 流式渲染：50ms 刷新，3 字符/次
- 健康检查：默认 30 秒
- 空闲内存：< 200 MB
- 启动到可交互：< 3 秒

### 12.6 错误恢复

**故障场景分级：**

| 场景 | 检测方式 | 严重级别 | 恢复方式 |
|------|---------|---------|---------|
| Agent 未启动 | HTTP 无响应 + 进程不存在 | 🔴 Critical | 自动重启一次 / 手动启动 |
| Agent 运行但 HTTP 不通 | HTTP 超时 | 🟠 Error | 健康检查 30s 重试 |
| LLM API 超时 | SSE idle > 30s | 🟡 Warning | 终止流式，显示超时提示 |
| LLM API 429 限流 | HTTP 429 | 🟡 Warning | Retry-After 倒计时 / Failover 切换 |
| 网络断开 | 连续失败 ≥ 3 | 🔴 Critical | 健康检查 30s 重试 / 切本地模型 |
| 本地模型文件损坏 | llama-server 启动失败 | 🟠 Error | 提示重新下载 |
| config.json 损坏 | JSON 解析失败 | 🟠 Error | 用默认值重建 + 提示用户 |
| 工作区磁盘满 | 磁盘 < 100MB | 🟡 Warning | 警报弹窗 + 清理建议 |
| OpenClaw 版本不兼容 | CLI 返回错误 | 🟠 Error | 提示更新 OpenClaw |

**离线降级时 UI 状态：**

| 功能 | Agent 断开 | LLM 超时 | LLM 429 | 网络断开 |
|------|-------------|----------|---------|---------|
| 发送消息 | ❌ 禁用 | ⏸️ 当前阻塞 | ⏸️ 当前阻塞 | ❌ 禁用 |
| 历史查看 | ✅ | ✅ | ✅ | ✅ |
| 设置 | ✅ | ✅ | ✅ | ✅ |
| Boot Check | ✅ | ✅ | ✅ | ✅ |
| Failover | — | ✅ 自动切换 | ✅ 自动切换 | — |

> 设计 → UI-37, UI-38, UI-56

---

## 13. 系统管理

### 13.1 日志
- **功能编号：** LOG-01 **优先级：** P1
- 日志文件：`%APPDATA%\AnyClaw_LVGL\logs\app.log`
- 轮转：文件超过 1MB 自动轮转，最多 5 个文件，7 天以上自动清理

### 13.2 崩溃恢复
- **功能编号：** E-01 **优先级：** P1
- 崩溃日志：`%APPDATA%\AnyClaw_LVGL\crash_logs\crash_*.log`，保留最近 10 个
- `SetUnhandledExceptionFilter` 捕获异常，dbghelp 符号解析（最多 64 帧）

**崩溃恢复 UI（UI-56）：**
- 启动时检测 `last_crash.txt` 是否存在
- 存在则弹出"💥 AnyClaw 异常退出"弹窗，显示崩溃摘要
- 用户可选择 [查看日志] 或 [继续启动]
- 日志内容分级：FATAL ERROR（红色）、Exception（橙色）、Call Stack（灰色）

> 设计 → UI-56

### 13.3 通知体系

统一通知策略，用户可配置接收级别：

| 级别 | 方式 | 示例 | 默认 |
|------|------|------|------|
| 静默 | 仅写日志 | 整理了 MEMORY.md | ✅ |
| 摘要 | 日终一次性汇总 | "今天做了 3 件事" | ✅ |
| 提醒 | 托盘气泡 | "日历提醒：20 分钟后有会议" | ✅ |
| 警报 | 弹窗 + 声音 | "磁盘空间不足 100MB" | ✅ |

**通知渠道：**
- 托盘气泡通知（`tray_balloon`）— Windows 系统托盘区域弹出气泡，带 title + message + timeout
- Toast 通知（右下角滑入，3 秒自动消失，最多同时 3 条，向上堆叠）
- 模态弹窗（警报级，需用户操作）
- 日志记录（所有级别）

> 设计 → UI-37, UI-38, UI-58

### 13.4 更新管理
|| 对象 | 方式 |
|------|------|
| AnyClaw | GitHub Releases 自动检测 + 一键下载覆盖 |
| OpenClaw | `openclaw update` 命令 |

**更新检测 UI（UI-57）：**
- 启动时检测 GitHub Releases 最新版本，与当前版本比较
- 发现新版本时弹出更新提示弹窗，显示"当前版本 → 新版本"
- 下载进度条实时显示百分比、速度、剩余时间
- 三源自动回退：HuggingFace → ModelScope → 镜像站
- 用户可选择 [立即更新] 或 [稍后提醒]

> 设计 → UI-57

### 13.5 Feature Flags
- **功能编号：** FF-01 **优先级：** P1 **状态：** ✅ 已实现

| 开关 | 描述 | 默认 | 需重启 |
|------|------|------|--------|
| `sbx_sandbox_exec` | 沙箱执行 | OFF | 否 |
| `obs_tracing` | 可观测性追踪 | OFF | 是 |
| `boot_self_heal` | 启动自修复 | OFF | 否 |
| `mem_long_term` | 长期记忆 | OFF | 否 |
| `kb_knowledge_base` | 知识库 | OFF | 否 |
| `remote_collab` | 远程协作 | OFF | 是 |

> 设计 → UI-50

### 13.6 Tracing
- **功能编号：** OBS-01 **优先级：** P1 **状态：** ✅ 已实现

追踪系统记录关键操作的性能和结果数据，用于排障。

- Ring Buffer：1000 事件，每 50 个事件自动 flush 到文件
- 持久化：`%APPDATA%\AnyClaw_LVGL\traces.jsonl`（JSON Lines 格式）

> 设计 → UI-51

### 13.7 商业授权
- **功能编号：** LIC-01 **优先级：** P0 **状态：** ✅ 已实现（V1）

**商业模式：Freemium**

| 层级 | 内容 | 价格 |
|------|------|------|
| 免费版 | Chat + 基础 Work + 单工作区 + 基础权限 | 免费 |
| 专业版 | 多工作区 + 完整权限 + Diff/Table + 本地模型 + IM 多渠道 + Agent 主动行为完整版 | 付费 |
| 企业版 | 权限审计 + 多实例管理 + 部署支持 | 按席位 |

**授权系统：** HMAC-SHA256 + Base32 编码的时间递增密钥方案。
- 免费版无需密钥，`license_seq=0` 且 `license_expiry=0` 视为试用期（始终有效）

**License 激活弹窗（UI-33）交互逻辑：**
- 试用期结束时首次启动 AnyClaw 时弹出，阻断主界面
- 显示内容：标题"License Activation"、警告"Trial Period Ended"、当前序列号(next seq)提示、密钥输入框
- 输入密钥后点击 [Activate]：本地验证 HMAC 签名 → 写入 `license_seq` 和 `license_expiry` → 弹窗关闭
- 验证失败：输入框下方显示错误原因，允许重试
- 点击 [Close]：关闭弹窗并退出程序（不能跳过）

> 设计 → UI-33

### 13.8 版本号规范
- 格式：`v{主版本}.{次版本}.{修订号}`
- 当前：`v2.2.11`
- 存储：`#define ANYCLAW_VERSION "2.2.11"`

---

## 附录

### A. LAN/FTP 候选设计（暂不实施）

#### A.1 LAN Chat

同局域网内 AnyClaw 用户直接通信，基于 TCP/UDP 的 Client-Host 模型。TCP 用于消息传输，UDP 用于自动发现。

#### A.2 FTP 传输

基于 WinINet 的 FTP 传输，支持单文件/目录递归上传下载、FTPS、断点续传。

---

> **文档完成。** 13 章正文 + 1 节附录。
>
> | 对应关系 | 见 Design.md 开头「编号索引表」 |
> |----------|-------------------------------|
> | 设计规范 | `docs/Design.md` |
> | 文档索引 | `docs/README.md` |
