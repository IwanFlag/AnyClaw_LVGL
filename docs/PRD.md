# AnyClaw LVGL - 产品需求文档 (PRD)

> 版本：v2.2.1 | 最后更新：2026-04-12

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
通用 AI 工作台（Chat + Work）+ OpenClaw（AI 管家）= AnyClaw
```

| 模式 | 角色 | 内容 |
|------|------|------|
| Chat 模式 | AI 管家 | 对话、提醒、查邮件、IM 集成、日常事务 |
| Work 模式 | 通用 AI 工作台 | AI 执行任务的可视化工作区——编程、写文档、数据分析、做 PPT、搜索整理，执行过程实时可见 |

**Work 模式不限于编程。** 学生写论文、职场做报告、开发者写代码——任务类型不同，但 AI 干活并展示执行过程的模式是通用的。

### 1.2 AnyClaw 与 OpenClaw 的关系

AnyClaw 和 OpenClaw 是**两个独立组件**，职责不同：

| 组件 | 本质 | 类比 |
|------|------|------|
| **AnyClaw** | GUI 客户端（壳子） | 浏览器 |
| **OpenClaw** | Agent 平台（运行时框架） | 操作系统 |
| **Gateway** | OpenClaw 的守护进程 | 系统服务 |
| **Agent** | Gateway 管理的 AI 实例 | 应用程序 |
| **LLM** | Agent 调用的底层模型 | CPU |

**为什么需要两个？** AnyClaw 提供图形界面和用户体验，OpenClaw 提供 AI 能力和工具调用运行时。AnyClaw 不自己实现 Agent 逻辑，而是通过 Gateway 借助 OpenClaw 的能力。

**安装 AnyClaw 不代表自动安装了 OpenClaw。** 首次启动向导会引导用户安装 OpenClaw。

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

| 版本 | 核心内容 | 状态 |
|------|---------|------|
| v2.0 | Chat/Work 双模式、权限系统、本地模型、向导、Model Failover | ✅ 已发布 |
| v2.1 | Diff 可视化、数据表格、Ask 模式弹窗深化 | 🔧 规划中 |
| v2.2 | 搜索结果渲染、文件操作渲染、WebRenderer | ⏳ 规划中 |
| v3.0 | AnyClaw 自身成为 Agent 平台（Direct API）、全自主执行 | ⏳ 长期目标 |

### 1.5 明确排除项

以下功能在 v2.0~v2.2 范围内**明确不做**：

| 排除项 | 说明 | 预计版本 |
|--------|------|---------|
| Direct API 模式 | AnyClaw 直连 LLM，不经过 Gateway | v3.0 |
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
| 可选依赖 | OpenClaw Gateway（未安装时引导安装） |
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
| BootCheck | 全面环境体检 | 9 | 手动触发/Work 模式按钮 |

**SelfCheck 检查项：**

| 检查项 | 检测方式 | 自动修复 |
|--------|---------|---------|
| Node.js | `node --version` ≥ 22.14 | 不可修复，弹窗提示安装 |
| npm | `npm --version` | 尝试 `npm cache clean --force` |
| 网络连通性 | HTTP GET google.com / openrouter.ai | 不可修复，提示检查网络 |
| 配置目录 | `%USERPROFILE%\.openclaw` 可写性 | 尝试创建目录 |

**BootCheck 全面检查（9 项）：** Node.js 版本 / npm / OpenClaw / Gateway 端口 / 配置目录 / 工作区 / 磁盘空间 / 网络 / SDL2.dll

- Node.js 缺失 → 显示阻断弹窗，阻止启动
- 当 Node/npm/OpenClaw 缺失时，即使向导已完成也强制打开向导

> 设计 → UI-01, UI-03

### 2.4 首次启动向导
- **功能编号：** WIZ-01 **优先级：** P1 **状态：** ✅ 已实现（6步）

6 步引导（全屏遮罩 + 居中弹窗）：

| 步骤 | 内容 | 必填 | Skip 影响 |
|------|------|------|----------|
| Step 1 | 语言选择（中文/English） | 是 | — |
| Step 2 | OpenClaw 环境检测与安装 | 是 | 未安装则无法聊天 |
| Step 3 | 模型 & API Key | 是 | 未配置则无法聊天 |
| Step 4 | 本地模型（Gemma E4B/26B） | 否 | Skip 后仅使用云端模型 |
| Step 5 | 连接 IM（Telegram/Discord/WhatsApp） | 否 | Skip 后仅桌面端可用 |
| Step 6 | 个人信息 & 确认（昵称/时区） | 是 | — |

**Skip 降级矩阵：**

| 跳过步骤 | 可用功能 | 不可用功能 |
|---------|---------|-----------|
| Step 4（本地模型） | 云端模型聊天、Work 模式全部功能 | 离线推理、无网时 AI 不可用 |
| Step 5（IM 连接） | 桌面端 Chat/Work 全部功能 | 手机远程指挥 AnyClaw |

**IM 连接详情：**
- 支持渠道：Telegram（Bot Token）、Discord（Bot Token）、WhatsApp（扫码）
- 全部 Skip 不影响桌面端正常使用
- 价值：让手机成为 AnyClaw 的远程控制器

> 设计 → UI-04 ~ UI-09

### 2.5 OpenClaw 安装

**发现机制（优先级从高到低）：**
1. config.json 中配置的路径
2. PATH 环境变量中的 `openclaw` 命令
3. npm 全局目录（`npm root -g`）
4. 常见安装位置扫描
5. 都找不到 → 引导用户安装或指定路径

**安装方式：**
- Auto Install：自动串联 Node.js + OpenClaw 安装
- 本地包：用户指定已安装路径
- 离线包：`bundled/openclaw.tgz` 无网环境安装

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

- **功能编号：** WIN-01 **优先级：** P0

SDL + LVGL 官方驱动，UI 布局设计尺寸 1450×800（逻辑像素），实际窗口基于屏幕分辨率。

**布局结构：**

```
┌─────────────────────────────────────────────────────────┐
│ 标题栏 (48px)                                           │
├─────────────────────────────────────────────────────────┤ 1px 分隔
│ 左侧面板 (240px) │ 分隔条(6px) │ 右侧内容区            │
└─────────────────────────────────────────────────────────┘
```

| 区域 | 默认宽度 | 最小宽度 | 说明 |
|------|---------|---------|------|
| 标题栏 | 全宽 | — | 48px 高，品牌+模式切换+窗口控制 |
| 左侧面板 | 240px | 200px | Gateway 状态 + Session 列表 + Cron |
| 分隔条 | 6px | 6px | 可拖拽调整面板宽度 |
| 右侧内容区 | 剩余空间 | — | Chat/Work 模式内容 |

**窗口行为：**
- 关闭按钮 → 最小化到托盘（非退出）
- 双击标题栏 → 最大化/还原
- 标题栏拖拽移动窗口
- 位置记忆：重启后恢复上次位置
- 窗口最小尺寸 800×500

**响应式：**

| 窗口宽度 | 左面板 | Chat 面板（Work 模式） |
|---------|--------|----------------------|
| > 1000px | 240px 正常 | 320px |
| 800~1000px | 折叠为抽屉按钮 | 折叠为 40px |

> 设计 → UI-11, UI-12, UI-52~55

### 3.2 系统托盘
- **功能编号：** TRAY-01 **优先级：** P0

常驻托盘图标，四种状态（白色=空闲 / 绿色=运行中 / 红色=异常 / 黄色=检测中）。

**托盘菜单：**
- 状态行：Gateway 状态 + LED 颜色
- Start/Stop Gateway、Restart OpenClaw、Refresh Status
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
- 支持 **CN / EN / KR / JP** 四种语言
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

聊天走 OpenClaw Gateway `/v1/chat/completions`（OpenAI 兼容），不直连 LLM API。

**通信流程：**
```
AnyClaw → POST http://127.0.0.1:18789/v1/chat/completions（带 Gateway auth token）
        → Gateway → Agent → LLM
```

Gateway token 自动从 `openclaw.json` 的 `gateway.auth.token` 读取。
model 固定为 `openclaw:main`，由 Gateway 路由到配置的 Agent。

### 4.2 输入与消息
- **功能编号：** CI-01 **优先级：** P0 **状态：** ✅ 已实现

**输入框：**
- 多行输入，Shift+Enter 换行，Ctrl+V 粘贴
- 高度自动增长（监听行数变化），默认 96px，最小 96px
- 支持中文 IME 输入

**消息气泡与流式：**
- **功能编号：** MB-01 **优先级：** P0 **状态：** ✅ 已实现
- 格式：`[HH:MM:SS] [角色] 内容`
- 用户消息靠右，AI 回复靠左
- 气泡宽度自适应：短消息贴合文字宽度，长消息 70% 容器宽度换行

**AI 流式渲染：**
- 打字机效果，50ms 刷新间隔，每次追加 3 个字符
- 流式过程中软滚（40px 容差，用户回看历史时不强制拉回）
- 超时保护：30s 无数据 或 总耗时 > 45s → 强制终止，显示"⚠️ AI 回复超时，请重试。"

**上下文窗口管理：**
- 发送给 LLM 的上下文采用滑动窗口策略：取最近 N 条消息，按 token 数截断
- 截断阈值由模型 context window 决定（默认保留最近 20 条或 8000 tokens，取先到者）
- 被截断的历史保留在 chat_history buffer 中，用户仍可滚动查看
- 持久化上限 50 条，内存 buffer 上限 4096 条

**并发会话：**
- 支持多个 Session 同时存在（主聊天 + Work 任务 + Cron 触发的 Session）
- 用户同一时间只与一个 Session 交互（左侧点击切换）
- 切换 Session 时上下文保留（每个 Session 独立维护历史）
- 正在流式输出的 Session 切到后台后继续接收，不中断

> 设计 → UI-14, UI-15, UI-16, UI-17

### 4.3 文件与多模态
- **功能编号：** MM-01 **优先级：** P0 **状态：** 🔧 实施中

**上传方式：**
- 上传按钮 + 弹出菜单（File / Directory）
- Win32 原生文件选择对话框

**文件处理能力矩阵：**

| 文件类型 | AI 能做什么 |
|---------|-----------|
| 文本文件（.txt/.md/.csv/.json/.xml） | 读取内容、分析、总结、改写 |
| 代码文件（.py/.js/.c/.cpp/.java 等） | 读取、审查、修改、解释 |
| 图片（.png/.jpg/.gif） | 描述内容、OCR 识别（依赖模型能力） |
| PDF | 读取文本层、总结（依赖模型能力） |
| 目录 | 列出文件结构、批量操作 |

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

### 4.7 语音输入
- **功能编号：** VOICE-01 **优先级：** P1 **状态：** ⏸️ 待实施

语音是 Chat 模式中的一种输入方式，不是独立模式。

**交互流程：**
1. 用户点击 Chat 输入区的 🎤 按钮
2. 开始录音（按钮变为红色 ●，显示录音时长）
3. 用户再次点击或 30s 自动停止
4. STT 转文字 → 填入输入框
5. 用户可编辑后发送，或自动发送

**技术方案：**
- STT：本地 Whisper 模型（小模型，CPU 可跑）或云端 API
- 录音：Win32 `waveIn` API 或 SDL 音频
- 语言：跟随界面语言设置，支持中英文混合识别

**语音回复（TTS，可选）：**
- AI 回复完成后可选择朗读
- TTS 引擎：系统 SAPI 或云端 API
- 开关：Chat 输入区旁的 🔊 按钮

> 设计 → UI-17（输入区 🎤 按钮位置）

---

## 5. 工作台 Work

### 5.1 核心组件
- **功能编号：** WORK-01 **优先级：** P0 **状态：** 🔧 实施中

Work 模式是 **AI 执行任务的通用可视化工作区**。不限于编程，写文档、做数据分析、搜资料、处理文件全部覆盖。

**三大组件：**

| 组件 | 能力 | 说明 |
|------|------|------|
| 步骤流 | 实时展示 Agent 每步操作 | 解析 SSE 工具调用事件 |
| 输出区域 | 多类型渲染器 | 按任务类型自动选择渲染方式 |
| Chat 面板 | 对话 + 工具输出 | 缩小到侧面 |

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

| 模式 | 角色 | 侧重 |
|------|------|------|
| Chat | AI 管家 | 对话、日常事务、文件操作 |
| Work | AI 工作台 | 任务执行可视化、步骤流、输出渲染 |

**切换规则：**
- 顶栏胶囊按钮切换，< 150ms 延迟
- **一个 Agent，两种界面，上下文完全互通**
- 切换不中断正在运行的任务（Work 任务后台继续）
- Chat 和 Work 共享 chat_history、MEMORY.md、工具调用

> 设计 → UI-21

### 6.2 AI 交互模式（自主 / Ask / Plan）
- **功能编号：** AIMODE-01 **优先级：** P0 **状态：** ✅ 已实现（V1）

| 模式 | 含义 | 触发规则 |
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

> 设计 → UI-22, UI-29, UI-32

### 6.3 UI 控制权模式（用户 / AI）
- **功能编号：** CTRL-01 **优先级：** P0 **状态：** 🔧 实施中

| 模式 | 操作者 | 说明 |
|------|--------|------|
| **用户模式**（默认） | 人类用户 | 用户手动操作鼠标键盘，AI 仅建议 |
| **AI 模式** | Agent | Agent 模拟用户操作鼠标键盘来控制 AnyClaw UI |

**核心约束：** AI 拿到 UI 控制权后，必须模拟真实用户的鼠标键盘操作，没有捷径 API。

- 用户可随时一键回退到用户模式
- AI 模式下所有高风险动作仍走权限检查与审计

### 6.4 模式与 Agent 人格

| 模式 | Agent 侧重 | 暴露工具 |
|------|-----------|---------|
| Chat | 管家人格（友好、主动、关注日常事务） | 邮件/日历/文件搜索/IM/定时任务 |
| Work | 执行人格（严谨、高效、关注任务质量） | 代码/终端/文件操作/搜索/数据分析 |

Agent 人格由 SOUL.md 统一定义，模式只影响 prompt 侧重。MEMORY.md 跨模式共享。

---

## 7. 模型管理

### 7.1 模型架构概述

```
AnyClaw (GUI)
    │
    ▼
OpenClaw Gateway → Agent → LLM API（云端）
                         → llama-server（本地）
```

| 层 | 职责 | 配置位置 |
|------|------|---------|
| AnyClaw | 提供模型选择 UI、API Key 输入 | `config.json` |
| Gateway | 路由到配置的 Provider 和模型 | `openclaw.json` |
| Agent | system prompt、工具、行为规则 | SOUL.md + AGENTS.md |
| LLM | token in → token out | Provider API / 本地推理 |

### 7.2 云端模型
- **功能编号：** MODEL-01 **优先级：** P0 **状态：** ✅ 已实现

**模型选择：**
- 下拉列表，按能力排序，默认 21 个模型
- 支持添加自定义模型（"+" 按钮 → 输入模型名称）
- OpenRouter 上 25 个 `:free` 模型，国内直连可达

**Provider 与 API Key：**
- 切换模型自动检测 Provider（openrouter/xiaomi）
- 所有 OpenRouter 模型共用一个 API Key
- API Key 存储：`config.json` 明文存储（当前实现），建议用户不在公共设备上使用
- Save 同步 Gateway：`openclaw config set` → 重启 Gateway

**Gateway 同步流程：**
```
用户修改模型/API Key → AnyClaw → openclaw config set → Gateway 自动重启
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
| 更新后 | 自动重启 Gateway | 不阻塞 UI |

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
| **对话记忆** | Session 上下文 | Gateway 内存 | Session 级 | 当前对话的上下文 |
| **长期记忆** | MEMORY.md + memory/ 日志 | 工作区文件 | 跨 Session | 用户偏好、决策记录、经验教训 |
| **知识库** | KB 索引 | 本地文件 | 持久化 | 用户导入的参考资料 |

**对话记忆管理：**
- 滑动窗口截断（见 §4.2 上下文窗口管理）
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
| openclaw.json（Gateway 配置） | 全局共享 |

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
| General | 开机自启动 / 语言 / 主题 / 退出确认 / Reconfigure Wizard |
| Permissions | 19 项权限编辑（Allow/Deny/Ask/ReadOnly） |
| Model | 当前模型 / 模型列表 / API Key / Failover |
| Log | 日志开关 / 级别 / 过滤 / 导出 / 清除 |
| About | 品牌信息 / 版本 / Tech Stack / 配置导入导出 / 迁移 |

> 设计 → UI-45, UI-46, UI-47, UI-48, UI-49

### 11.2 配置存储

| 文件 | 路径 | 管理方 | 说明 |
|------|------|--------|------|
| `config.json` | `%APPDATA%\AnyClaw_LVGL\` | AnyClaw | 主配置 |
| `permissions.json` | `%APPDATA%\AnyClaw_LVGL\` | AnyClaw | 权限 Source of Truth |
| `workspace.json` | 工作区根目录 | AnyClaw | 运行时权限投影 |
| `openclaw.json` | `%USERPROFILE%\.openclaw\` | OpenClaw | Gateway 配置 |

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

```
AnyClaw (GUI)
  ├→ Chat/SSE    → HTTP POST + SSE  → Gateway :18789  → Agent → LLM API
  ├→ Health      → HTTP GET /health → Gateway :18789
  ├→ Sessions    → CLI 子进程调用   → openclaw gateway call
  ├→ Config      → CLI 子进程调用   → openclaw config set/get
  ├→ License     → 本地内存验证     → 不走 Gateway
  ├→ KB 检索     → 本地文件搜索    → 不走网络
  ├→ Failover    → HTTP POST       → OpenRouter API（直连）
  └→ 本地模型    → HTTP POST + SSE  → llama-server :18800 → GGUF
```

### 12.2 Gateway 守护进程

**进程管理：**
- 一键启动、停止、重启（`openclaw gateway start/stop`）
- 进程检测：`CreateToolhelp32Snapshot` 遍历进程列表

**健康监控：**
- 检测频率：每 30 秒（可配置 5s/15s/30s/60s/120s）
- 自动重启：Gateway 进程不存在 + HTTP 无响应 → 自动重启一次，失败后不再重试
- 三级状态：Ready(🟢) / Busy(🟡) / Error(🔴)

**状态机：**
```
Gateway 进程不存在 + HTTP 无响应:
  ├── 从未自动重启 → 执行 app_start_gateway()，状态变 Checking
  └── 已自动重启 → 状态变 Error，不再重试

Gateway 进程存在 + HTTP 无响应:
  ├── 连续失败 < 3 → 状态保持 Checking
  └── 连续失败 ≥ 3 → 状态变 Error

Gateway 进程存在 + HTTP 200:
  ├── 有活跃 Session → Busy
  └── 无活跃 Session → Ready
```

### 12.3 网络通信

| 场景 | 目标 | 协议 | 超时 |
|------|------|------|------|
| 聊天（云端） | Gateway :18789 | HTTP POST + SSE, OpenAI 格式 | 流式 45s |
| 健康检查 | Gateway :18789 | HTTP GET /health | 3s |
| 会话管理 | openclaw CLI | 子进程 + JSON | 12s |
| Failover 探针 | OpenRouter API | HTTP POST, OpenAI 格式 | 12s |
| 配置更新 | openclaw CLI | `openclaw config set` | 8s |

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

### 12.4 安全与隐私

**用户可理解的隐私说明：**

| 数据 | 存在哪 | 传到哪 | 谁能看到 |
|------|--------|--------|---------|
| 对话内容 | Gateway 内存 + chat_history buffer | LLM API（OpenRouter/xiaomi 等） | 用户 + LLM Provider |
| 配置文件 | 本地 config.json | 不上传 | 仅用户 |
| API Key | 本地 config.json（明文） | Gateway 中转到 Provider | 仅用户 |
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
| Gateway 未启动 | HTTP 无响应 + 进程不存在 | 🔴 Critical | 自动重启一次 / 手动启动 |
| Gateway 运行但 HTTP 不通 | HTTP 超时 | 🟠 Error | 健康检查 30s 重试 |
| LLM API 超时 | SSE idle > 30s | 🟡 Warning | 终止流式，显示超时提示 |
| LLM API 429 限流 | HTTP 429 | 🟡 Warning | Retry-After 倒计时 / Failover 切换 |
| 网络断开 | 连续失败 ≥ 3 | 🔴 Critical | 健康检查 30s 重试 / 切本地模型 |
| 本地模型文件损坏 | llama-server 启动失败 | 🟠 Error | 提示重新下载 |
| config.json 损坏 | JSON 解析失败 | 🟠 Error | 用默认值重建 + 提示用户 |
| 工作区磁盘满 | 磁盘 < 100MB | 🟡 Warning | 警报弹窗 + 清理建议 |
| OpenClaw 版本不兼容 | CLI 返回错误 | 🟠 Error | 提示更新 OpenClaw |

**离线降级时 UI 状态：**

| 功能 | Gateway 断开 | LLM 超时 | LLM 429 | 网络断开 |
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
- 托盘气泡通知（`tray_balloon`）
- Toast 通知（右下角滑入，3 秒自动消失）
- 模态弹窗（警报级）
- 日志记录（所有级别）

> 设计 → UI-37, UI-38, UI-58

### 13.4 更新管理

| 对象 | 方式 |
|------|------|
| AnyClaw | GitHub Releases 自动检测 + 一键下载覆盖 |
| OpenClaw | `openclaw update` 命令 |

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
- License 激活弹窗：试用期结束时阻断，要求输入密钥

> 设计 → UI-33

### 13.8 版本号规范
- 格式：`v{主版本}.{次版本}.{修订号}`
- 当前：`v2.2.1`
- 存储：`#define ANYCLAW_VERSION "2.2.1"`

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
