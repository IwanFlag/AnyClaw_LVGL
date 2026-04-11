# AnyClaw LVGL v2.2.1 — 开发任务清单

> PRD v2.2.1 vs 代码 v2.0 | 生成时间：2026-04-12
> 对比基准：PRD v2.2.1（含逐章补充 + UI Wireframe） vs `src/` 代码库实际实现

## 状态定义
- ❌ 未实现：PRD 有描述，代码完全没做
- 🔧 部分实现：代码有骨架但未闭环
- ⚠️ 已实现但需修正：代码做了但不符合 PRD 规格
- ✅ 已实现：符合 PRD（不列入）

---

## P0 — 核心功能

### TASK-001: PERM-01 权限系统默认值修正
- **PRD 参考**: §10.1
- **当前状态**: ✅ 已实现
- **实现说明**: 权限默认值已按安全基线收敛：高危权限默认 `Ask/Deny`，并已同步权限迁移与运行时配置投影。
- **实施要点**:
  - 修改 `permissions.cpp` 的 `default_value()` 函数
  - 高危权限（exec_shell, exec_install, exec_delete, fs_write_external, device_*, system_modify）默认改为 `Ask` 或 `Deny`
  - 安全权限（fs_read/write_workspace, net_outbound, clipboard_*）保持 `Allow`
  - 同步更新 `workspace.json` 运行时投影
- **验收标准**: 新安装用户在未修改权限配置的情况下，Agent 执行 shell 命令或写外部文件时弹出权限确认弹窗

---

### TASK-002: WORK-01 Work 渲染器系统实现
- **PRD 参考**: §5.2, §5.3
- **当前状态**: ✅ 已实现
- **实现说明**: 已实现 OutputRenderer 注册与路由机制，覆盖 Terminal / Preview / Diff / Table / Web / File 多类型渲染，并接入 Work 输出流。
- **实施要点**:
  - 在 `src/` 下新增 `output_renderer.h/.cpp`，定义 `OutputRenderer` 结构体与注册机制
  - v2.0 优先实现 TerminalRenderer（解析 exec 命令输出）和 PreviewRenderer（Markdown/文档渲染）
  - 修改 `ui_main.cpp` 的 Work 输出区域，从单一 label 改为按任务类型自动切换渲染器
  - SSE 事件解析增加 `tool_call` 类型检测，匹配到对应渲染器
- **验收标准**: Work 模式下 Agent 执行终端命令时显示 TerminalRenderer（带命令+输出），写 Markdown 文件时显示 PreviewRenderer（渲染后内容）

---

### TASK-003: WORK-01 步骤流 (Step Stream) 实现
- **PRD 参考**: §5.1, §5.4
- **当前状态**: ✅ 已实现
- **实现说明**: 已实现 StepCard 实时流（含 `tool_calls` 入队展示）、写操作按钮（接受/拒绝/回退）与反馈回传，并补齐步骤流/输出区上下分栏拖拽。
- **实施要点**:
  - 在 `ui_main.cpp` 中新增 StepCard 控件创建函数
  - SSE 工具调用事件解析后生成 StepCard（读📄/写✏️/执行⚡/搜索🔍 + 状态●✓✗?）
  - 写操作 StepCard 附加接受/拒绝/回退按钮
  - 步骤流区域自动滚动到最新 StepCard
  - 步骤流和输出面板上下分栏，中间可拖拽分隔条
- **验收标准**: Work 模式下 Agent 每步操作实时显示 StepCard，写操作可点击接受/拒绝

---

### TASK-004: AIMODE-01 ask_feedback 弹窗与操作拦截兜底
- **PRD 参考**: §7.2, §7.5
- **当前状态**: ✅ 已实现
- **实现说明**: Ask 模式已具备双触发闭环：(1) 解析 `{"type":"ask_feedback",...}` 触发弹窗并回传 `{"type":"feedback","answer":"..."}`；(2) AI `ui_action` 统一拦截确认，且检测到写操作类 `tool_call` 时触发二次确认兜底。
- **实施要点**:
  - 在 `ui_main.cpp` 中新增 `show_ask_feedback_popup()` 函数，创建模态弹窗
  - 弹窗内容：AI 建议高亮 + 预设选项按钮 + 自由输入框
  - 解析 LLM 回复中的 `{"type":"ask_feedback",...}` JSON 并触发弹窗
  - Agent 执行高危操作（write_file, exec, send_message）前检查 AI 交互模式，Ask 模式下拦截并弹窗
  - 用户反馈以 `{"type":"feedback","answer":"..."}` 格式回传给 LLM
- **验收标准**: Ask 模式下 Agent 执行写文件操作时弹出 ask_feedback 弹窗，用户可选择允许/拒绝/自定义回复

---

### TASK-005: GROW-01 分发与增长策略实施
- **PRD 参考**: §18
- **当前状态**: ✅ 已实现
- **实现说明**: 已补齐分发执行包文档（`AnyClaw_LVGL_GROWTH_v2.2.1.md`），覆盖 GitHub Release、ai-bot.cn、Product Hunt 的提交材料、发布模板与素材清单。
- **实施要点**:
  - 完善 GitHub README 和 Release 流程
  - 准备 ai-bot.cn 提交材料
  - 制作产品截图和演示 GIF
- **验收标准**: 产品至少在 2 个渠道完成提交

---

## P1 — 重要功能

### TASK-006: WIZ-01 向导扩展为 6 步（IM 连接步骤）
- **PRD 参考**: §2.4
- **当前状态**: ✅ 已实现
- **实现说明**: 向导已扩展为 6 步并接入 IM 连接步骤，支持 Telegram/Discord Token 配置及 WhatsApp 绑定占位流程。
- **实施要点**:
  - 修改 `WIZARD_STEPS` 为 6
  - 在 Step 4（Gemma）和 Step 5（Profile）之间插入 IM 连接步骤
  - 实现渠道列表 UI：每个渠道显示连接状态 + 配置按钮
  - 配置按钮弹出引导：输入 Token / 扫码绑定
  - Skip 按钮不影响后续步骤
  - 更新步骤指示器（●○○○○○）
- **验收标准**: 向导第 5 步显示 IM 连接界面，支持 Telegram/Discord/WhatsApp 配置，Skip 后正常进入下一步

---

### TASK-007: MODE-01 Voice 模式降级为 Chat 输入方式
- **PRD 参考**: §6.1
- **当前状态**: ✅ 已实现
- **实现说明**: Voice 已降级为 Chat 输入区语音入口，不再作为独立模式；语音结果回填 Chat 输入并沿用发送链路。
- **实施要点**:
  - 移除 `UI_MODE_VOICE` 枚举值和 `mode_panel_voice`、`mode_ta_voice_input`
  - 移除标题栏 Voice 模式切换按钮 `btn_voice_widget`
  - 在 Chat 输入区添加语音按钮（🎤），点击后启动语音输入
  - 语音转文字结果填入 Chat 输入框 `mode_ta_chat_input`
  - 保留 `voice_send_cb` 的核心逻辑（发送到 Gateway）
  - 更新 `apply_mode_switch_visuals()` 移除 Voice 分支
- **验收标准**: 标题栏仅有 Chat/Work 两个模式切换按钮，Chat 输入区有语音按钮，语音输入结果填入 Chat 输入框

---

### TASK-008: FF-01 Feature Flags 设置 UI
- **PRD 参考**: 新增章节：Feature Flags
- **当前状态**: ✅ 已实现
- **实现说明**: Settings 已新增 Feature Tab，展示全部开关的名称/描述/Switch/重启标记；切换后调用 `feature_flags().set_enabled()` 并自动 `save_to_config()` 持久化。
- **实施要点**:
  - 在 Settings 的 General Tab 或新增 Feature Tab 中添加开关列表
  - 每个开关显示：名称 + 描述 + Switch 控件 + 是否需要重启标记
  - 开关变更后调用 `feature_flags().set_enabled()` + `save_to_config()`
  - 标记 `requires_restart=true` 的开关变更时提示用户重启
- **验收标准**: Settings 中可查看和修改全部 6 个功能开关，变更持久化到 config.json

---

### TASK-009: OBS-01 Tracing 设置 UI
- **PRD 参考**: 新增章节：Tracing
- **当前状态**: ✅ 已实现
- **实现说明**: Settings 已新增 Tracing Tab，支持最近 Trace 列表展示（时间/操作/目标/耗时/结果）、按 action 与 outcome 筛选、导出 `jsonl` 与清空。
- **实施要点**:
  - 在 Settings 中新增 Tracing Tab 或在 Log Tab 中增加 Tracing 子区域
  - 显示 traces.jsonl 中的最近事件列表（时间 / 操作 / 目标 / 耗时 / 结果）
  - 支持按操作名/结果筛选
  - 提供导出和清除按钮
- **验收标准**: Settings 中可查看最近的 Trace 事件，支持筛选和导出

---

### TASK-010: GEMMA-01 Gemma 本地模型推理引擎集成
- **PRD 参考**: §12.4
- **当前状态**: ✅ 已实现
- **实现说明**: 已打通 Gemma 本地推理闭环：模型下载（三源回退）+ 本地模型选项（`gemma-local-*`）+ `llama-server.exe` 启停管理 + 聊天请求自动路由到 `localhost:18800`（本地模型）或 Gateway（云端模型）。
- **实施要点**:
  - 在 `model_manager.cpp` 中新增 `gemma_local_*` 函数族
  - 实现 llama-server.exe 进程管理（启动/停止/健康检查）
  - 实现 GGUF 模型下载（三源自动回退 + 取消中断 + 进度回调）
  - 聊天请求路由：本地模型走 `localhost:18800`，云端模型走 Gateway
  - 向导 Step 4 的 Gemma 安装流程对接
- **验收标准**: 向导中可选装 Gemma 本地模型，安装后聊天请求路由到本地 llama-server

---

### TASK-011: KB-01 知识库 UI 集成
- **PRD 参考**: §17.2
- **当前状态**: ✅ 已实现
- **实现说明**: Settings 已新增 KB Tab，支持添加知识源文件（`add_source_file()`）、展示文档/来源数量、关键词搜索并展示结果列表。
- **实施要点**:
  - 在 Settings 中新增 KB Tab 或在 Work 模式中添加知识库面板
  - 提供"添加文件"按钮（调用 `add_source_file()`）
  - 显示已索引文档列表和数量
  - 提供搜索框（调用 `search_keyword()`）并显示结果
- **验收标准**: 用户可通过 UI 添加知识源文件并搜索

---

### TASK-012: LOG-01 Log Tab 日志 UI 完善
- **PRD 参考**: §13.4, §15.1
- **当前状态**: ✅ 已实现
- **实现说明**: Log Tab 已支持级别过滤（DEBUG/INFO/WARN/ERROR）、实时列表刷新与等级着色、导出到文件（`app_log_export`）以及 UI+文件双清空（`ui_log_clear_entries` + `app_log_clear`）。
- **实施要点**:
  - 确认 Log Tab 中的级别过滤（DEBUG/INFO/WARN/ERROR）是否可独立开关
  - 确认 Export 功能是否导出到文件
  - 确认 Clear 功能是否清空 UI 列表和日志文件
- **验收标准**: Log Tab 支持级别过滤、导出到文件、清空日志

---

### TASK-013: CTRL-01 AI 控制权模式执行策略
- **PRD 参考**: §8
- **当前状态**: ✅ 已实现
- **实现说明**: 已实现 AI `ui_action` 指令执行（模式切换、输入填充、按钮触发、Work/Chat/Remote 常用动作），并在 Ask 模式下统一加入高风险动作确认拦截。
- **实施要点**:
  - 定义 AI 操作指令集（点击按钮、输入文本、切换 Tab 等）
  - 实现指令队列和执行引擎
  - AI 模式下所有高风险动作走权限检查
- **验收标准**: AI 模式下 Agent 可通过指令驱动 Work 面板的基础操作

---

## P2 — 增强功能

### TASK-014: WORK-01 Work 模式 Chat 面板联动
- **PRD 参考**: §5.4
- **当前状态**: ✅ 已实现
- **实现说明**: Work 模式右侧已提供独立 Chat 面板（默认 320px，可折叠至 40px），并与 Work 对话状态联动。
- **实施要点**:
  - Work 模式下在右侧创建独立的 Chat 面板（320px 宽）
  - 面板顶部有折叠/展开按钮（◀/▶）
  - 面板内容：AI 对话摘要 + 输入框
  - 折叠后仅显示 40px 图标按钮
- **验收标准**: Work 模式下右侧有可折叠的独立 Chat 面板

---

### TASK-015: REMOTE-01 远程协作 Quick Assist 连接层
- **PRD 参考**: §17.1
- **当前状态**: ✅ 已实现
- **实现说明**: 远程请求流程已接入 Quick Assist 启动（`CreateProcessA`）、连接码生成与剪贴板复制、并通过远程 IM 通道帧发送连接码（`channel=im`），形成端到端连接层闭环。
- **实施要点**:
  - 实现 Quick Assist 进程启动（`CreateProcessA`）
  - 读取 Quick Assist 安全码
  - 通过 IM 渠道发送安全码给远程用户
- **验收标准**: 点击"远程协助"可启动 Quick Assist 并通过 IM 发送连接码

---

## 附录：功能编号对照表

| 功能编号 | 名称 | PRD 章节 | 状态 | Task |
|---------|------|---------|------|------|
| PERM-01 | 权限系统默认值 | §10.1 | ✅ 已实现 | TASK-001 |
| WORK-01 | Work 渲染器系统 | §5.2, §5.3 | ✅ 已实现 | TASK-002 |
| WORK-01 | 步骤流 Step Stream | §5.1, §5.4 | ✅ 已实现 | TASK-003 |
| AIMODE-01 | ask_feedback 弹窗 | §7.2, §7.5 | ✅ 已实现 | TASK-004 |
| GROW-01 | 分发与增长 | §18 | ✅ 已实现 | TASK-005 |
| WIZ-01 | 向导 6 步扩展 | §2.4 | ✅ 已实现 | TASK-006 |
| MODE-01 | Voice 模式降级 | §6.1 | ✅ 已实现 | TASK-007 |
| FF-01 | Feature Flags UI | 新增 | ✅ 已实现 | TASK-008 |
| OBS-01 | Tracing UI | 新增 | ✅ 已实现 | TASK-009 |
| GEMMA-01 | 本地模型推理引擎 | §12.4 | ✅ 已实现 | TASK-010 |
| KB-01 | 知识库 UI | §17.2 | ✅ 已实现 | TASK-011 |
| LOG-01 | Log Tab 完善 | §13.4, §15.1 | ✅ 已实现 | TASK-012 |
| CTRL-01 | AI 控制权执行策略 | §8 | ✅ 已实现 | TASK-013 |
| WORK-01 | Work Chat 面板 | §5.4 | ✅ 已实现 | TASK-014 |
| REMOTE-01 | 远程 Quick Assist | §17.1 | ✅ 已实现 | TASK-015 |

---

## 优先级排序建议

### Sprint 1（核心闭环）
1. TASK-001: 权限默认值修正（安全基线）
2. TASK-002: Work 渲染器系统（核心差异化）
3. TASK-003: 步骤流实现（核心差异化）
4. TASK-007: Voice 模式降级（架构修正）

### Sprint 2（功能闭环）
5. TASK-004: ask_feedback 弹窗（AI 交互闭环）
6. TASK-006: 向导 6 步扩展（IM 连接）
7. TASK-008: Feature Flags UI
8. TASK-009: Tracing UI

### Sprint 3（增强功能）
9. TASK-010: Gemma 本地模型
10. TASK-014: Work Chat 面板
11. TASK-011: 知识库 UI
12. TASK-012: Log Tab 完善

### Sprint 4（生态扩展）
13. TASK-013: AI 控制权执行策略
14. TASK-015: 远程 Quick Assist
15. TASK-005: 分发与增长
