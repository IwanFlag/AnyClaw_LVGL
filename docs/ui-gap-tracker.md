# UI 对齐追踪文档

基于 PRD.md 与 Design.md，按界面逐项对齐并闭环：文档更新 -> 代码实现 -> 截图测试 -> 缺陷修复。

当前 UI-11 对齐优先级：`ui/matcha_v1/UI-11-chat-mode.svg` > Design.md > PRD.md。

## UI-01 启动自检（当前进行中）

### 需求基线（已确认）
- 启动进入 Boot Check 页面，页面尺寸与主界面一致（同窗口尺寸），不是全屏独占桌面。
- 检查项显示统一状态：OK / Err / N/A。
- 检查项包含：Node.js、npm、Network、OpenClaw、Agent（端口18789）、Hermes、Claude Code CLI。
- 全部 Pass：短暂停留后自动进入后续流程。
- 有 Fail：停留在页面，允许用户选择“进入主界面（受限）”或“进入向导修复”。
- 首次安装：Boot Check 后进入向导。
- 已安装：Boot Check 后进入主界面；有关键失败可改走向导。

### 文档对齐状态
- PRD：已更新（新增 UI-01 启动页面形态、分流规则、首次/已安装规则）。
- Design：已更新（UI-01 图示与交互规则改为同窗口尺寸 + Pass/Fail/Unknown）。

### 代码实现状态
- 状态：核心逻辑已修复，待最终视觉确认与截图留档。
- 已实现：
	- 启动页由右上角小卡片改为同窗口尺寸 Boot Check 页面。
	- 页面展示 7 项检查：Node.js、npm、Network、OpenClaw、Agent（端口18789）、Hermes、Claude Code CLI。
		- 状态语义统一为 OK / Err / N/A。
	- 全部通过时短暂停留后自动继续流程：首次安装进入向导，已安装进入主界面。
	- 存在 Fail 时停留页面，提供按钮：进入主界面（受限）/进入向导修复。
	- 原独立“自动弹向导”定时器已移除，分流统一由 Boot Check 页面处理。
	- 2026-04-25：Fail 场景已改为锁定停留，仅允许通过按钮离开，消除自动跳主界面问题。
	- 2026-04-25：布局已改为单一居中容器，日志验证 rows=(283,497)-(1635,750)，相对 1920 宽窗口保持水平居中。
	- 2026-04-25：状态文案统一为 OK/Err/N/A；Port 18789 与 Disk Space 判定已去除错误耦合。
	- 2026-04-25：底部分流按钮改为始终显示；全 OK 自动流转时序统一为 >=2.5s，仅 N/A（无Err）直接进入主界面。

### 验收清单
- [x] 启动显示 Boot Check 页面（同窗口尺寸）
- [x] 展示 7 项状态（OK/Err/N/A）
- [x] 全 Pass 自动跳转流程正确（代码逻辑）
- [x] 有 Fail 时两个分流按钮可用（代码逻辑）
- [x] 首次安装/已安装分流正确（代码逻辑）
- [x] 可视确认通过（用户已确认“现在能看到了”）
- [ ] 截图产物完成（本轮补充留档）
- [x] 测试通过并修复缺陷（Fail 自动跳转已修复，布局居中已通过日志验证）

### 本轮验证记录
- 日志验证：`apply_result fail=1 unknown=0 wizard_completed=1` 后输出 `stay locked on fail result page`，未再出现自动 `route -> main ui`。
- 布局验证：`BootCheckLayout` 输出 `overlay=(0,0)-(1919,1199)`、`rows=(283,497)-(1635,750)`，列表容器位于页面中轴。

## UI-02 已运行提示（实例互斥）

1. 互斥体命名已对齐：`AnyClaw_LVGL_SingleInstance_Mutex_v2.0`（ASCII + 版本号）。
2. 二次启动路径已对齐：按标题优先级尝试激活旧窗口（`AnyClaw LVGL v2.0 - Desktop Manager` → `AnyClaw LVGL` → `AnyClaw`），执行 `SW_RESTORE + SetForegroundWindow + FlashWindow`。
3. 找不到句柄时静默退出并记录 warning，不弹 MessageBox。
4. 回归验证：构建通过，重复启动后进程数保持 1。

## UI-03 启动错误阻断（SelfCheck）

1. 启动链路已接入 SelfCheck（main 早期执行）：失败则设置阻断信息。
2. 阻断态已跳过 BootCheck 入口与后台启动 worker，避免与 UI-01 并行。
3. 阻断弹窗已对齐：标题“⚠ 检测到问题”、失败项列表容器、说明文字、唯一按钮“退出（120×40）”。
4. 按钮行为已对齐：点击“退出”直接 `PostQuitMessage(0)` 结束进程，无重试与跳过。

## UI-04 首次启动向导（整体框架）

1. 顶栏已对齐：右上改为 `[—] [✕]`；`[—]` 最小化窗口，`[✕]` 不再直接关向导。
2. 底栏已对齐：改为左 `[退出]`、右 `[上一步] [下一步]`，栏高 56px；step0 继续隐藏 `[上一步]`，并补了“已选择/请选择”状态提示。
3. 退出与文案语义已对齐：点 `[退出]` 或 `[✕]` 均弹确认「确定退出向导吗？应用将关闭」并退出；Step2 API Key 文案改为“可选”。

## UI-09 Step3 完成页（昵称/时区/Leader）

1. 结构已对齐：Step3 改为“完成标题 + 昵称 + 时区 + Leader 模式”，移除旧的摘要块展示。
2. 行为已对齐：Leader 改为单选语义（启用/跳过），跳过时自动回落单 Agent（OpenClaw）。
3. 完成语义已对齐：默认时区改为 `Asia/Shanghai`；最后按钮文案改为“开始使用 ->”；昵称为空时按 `anonymous` 写入 USER.md。

## UI-07 本地模型配置（Gemma）

1. 入口已接入：Step3 的“进阶能力”新增 `💾 本地模型`按钮，点击后进入 Gemma 配置子页。
2. 文案已对齐：子页标题更新为“本地模型配置”，补齐副标题“使用 Gemma 实现离线推理（可选）”。
3. 行为已对齐：新增“保存配置”按钮，保留“跳过此步骤”，两者在末步场景都会安全回到完成页。

## UI-08 IM 连接配置

1. 入口已接入：Step3 的“进阶能力”新增 `💬 IM 连接`按钮，点击后进入 IM 配置子页。
2. 子页已补齐：在原有 Telegram/Discord/WhatsApp 配置能力基础上，补充“保存配置 + 跳过此步骤”操作。
3. 返回已对齐：IM 子页在末步场景下保存/跳过均回到完成页，不触发越界下一步。

## UI-11 Bot · Chat 模式

1. 左侧已对齐：Bot 模式左侧改为“Agent 状态 -> 当前模型 -> Session/Cron 列表”的顺序（固定宽度），并接入 2s 自动刷新；无数据时显示空态占位。
2. 右侧骨架已对齐：Chat 区域保持“输出区 -> 控制行 -> 输入区”三层；移除输入区下方额外 footer，避免超出设计层级。
3. trace 占位已对齐：AI 思考/脚本 trace 面板改为“有内容才显示”，无内容不占据输出区高度。
4. 样式层已落地：左侧标题改为“Agent / Session / Cron”，并新增 Agent 状态与模型信息区；列表卡片按主题 token 统一文本、边框、圆角、行距与滚动行为。

## UI-12 Bot · Work 模式

1. 右侧运行态已收敛：Work 默认显示“输出区（Step Stream + Output）-> 输入区”主路径，运行所需控件保持可见。
2. 控制行已补齐：顶部控制行补入 `AI托管` 下拉与 `Voice` 按钮，并与 `Agent / AI行为 / Report` 共存；模式切换状态会同步到 Work 与控制行两个入口。
3. 输入区附件交互已对齐：`📎` 按钮改为弹层菜单，明确提供“选择文件 / 选择目录”两项，再走原生对话框与附件队列流程。
4. 扩展配置区默认隐藏：Trace/Profile/Remote/Cron/Gemma 等配置模块保留能力但默认折叠，避免干扰 Work 主流程的右侧布局。

## UI-11/交互稳定性修复（2026-04-26）

1. Settings 打开稳定性已加固：`ui_settings_open()` 增加 LVGL 对象有效性校验；检测到悬空 `settings_panel` 时自动重建，避免点击 Settings 后闪退。
2. Settings 关闭路径已补齐清理：关闭时会清理确认弹层 overlay，并在对象失效场景重置 `settings_panel/settings_tabs/settings_visible` 状态，避免遮罩残留导致界面点击失效。
3. 控制行点击无响应已修复：`ctrl_bar` 在创建、模式切换、窗口重排后都会显式 `move_foreground`，保证下拉与按钮位于可交互层级。
4. 聊天 HTTP 200 误报已修复：SSE 回调新增 plain JSON 回退解析（兼容非 `data:` 流），并将“200 但无可解析内容”错误文案单独区分，不再误导为通用 HTTP 失败。

## UI-11/UI-13 深化对齐（2026-04-26 第二轮）

1. Settings 打开链路二次加固：新增 refresh token 代际校验，关闭时会失效旧异步回调；当初始化重入时改为短延迟重试，避免“点击无反应/偶发卡死”。
2. Chat 气泡结构重排：用户与 AI 气泡统一为“头像同排 + 气泡内时间戳 + 文本主体”，并保持 70% 最大宽度与自动换行，贴近 UI-14/UI-15 规范。
3. 控制行交互层强化：ctrl_bar 增加 floating + clickable 属性，并在布局阶段持续前置，降低被其他控件遮挡导致的点击失效风险。
4. Voice 与拟人态联动：左侧 AI 伙伴卡新增 AI 头像动态区（ring + 状态文本），根据 Listening/Speaking/Thinking/Error 自动切换节奏与视觉状态。
5. HTTP 200 兼容解析增强：非 SSE 回退路径新增 OpenAI choices.content 解析与 error.message 优先解析，减少“200 但无文本”误判。
6. UI-13 贴边大蒜头扩展：补充 idle 呼吸、随机特效（喷火/跳跃/摇头/闪光/冒泡/打盹）与恢复时 shrink+fade+位移动画，贴边阈值同步放宽以提升触发稳定性。

## UI-11 运行态闭环修复（2026-04-26 第三轮）

1. Chat 网关自愈已接入：发送前增加 Gateway 健康预检，端口不可达时自动尝试 `openclaw gateway start` 并等待健康恢复后重试。
2. Chat 连接失败提示已细化：`status <= 0` 时改为明确提示 `127.0.0.1:port` 连接失败，避免误导成笼统 HTTP 错误。
3. Chat 失败二次恢复已接入：首次流式请求连接失败后会触发一次自动恢复并重试，减少“首次必报错”体验。
4. Settings 干扰降载已接入：Bot 侧栏会话刷新在 Settings 打开时降频到 12s，Health Error 状态降频到 8s，减少后台高频请求对交互的影响。
5. Session 网络故障降级已优化：`/api/sessions` 在网关不可达（`code <= 0`）时跳过 CLI fallback，避免每轮触发子进程超时造成卡顿叠加。

## UI-11 SVG 权威对齐（2026-04-26 第四轮）

1. 已按 SVG 重命名主标题体系：Bot Chat/Work 与模块标题统一为 `AnyClaw · xxx`，避免旧版 `Desktop Manager / Bot-Chat` 文案偏差。
2. 左 NAV 改为图标优先：Bot/File/KB/Skill 使用图标态，底部 Chat/Work 与 Settings 也改为图标按钮，选中态改为 SVG 同类高亮风格。
3. 左侧 Bot 卡片结构更新：新增“任务列表(count)”头部行，第二卡语义从“定时任务”改为“所有任务”，并统一空态文案“暂无任务”。
4. Bot 左栏布局重排：AI 伙伴头像区改为“左头像 + 右名称/说明 + 底部 Agent/Model”的布局，不再使用旧居中结构。
5. 控制行尺寸与语义收敛：四个芯片（Agent / AI行为 / AI托管 / Voice）宽度统一，`AI托管` 改为更明确的托管语义选项。
6. 聊天链路稳定性增强：
	- 修复 `health.cpp` 中 `%s` 误传 `std::string` 的未定义行为（乱码/潜在卡死根因之一）。
	- `chat_api_thread` 增加“模型未配置”前置阻断（Model=N/A 时直接提示设置，不再盲发请求）。
	- SSE/非SSE 解析增强：支持 `data:`（无空格）和 `content/text/message/output_text` 多形态提取。
	- 对“HTTP 200 但无内容”新增 payload sample 日志，便于后续日志闭环定位。

## UI-11 会话链路降噪（2026-04-26 第五轮）

1. Session HTTP 端点改为自动探测：优先尝试 `/api/sessions`，并回退探测 `/api/session/list`、`/api/v1/sessions`、`/api/sessions/list`；仅当返回 2xx 且为 JSON 时才作为有效端点。
2. 当未发现有效 HTTP 端点时，新增 60s 探测退避：不会每 2s 重复请求同一路径，显著降低 `/api/sessions` 404 日志噪声。
3. Gateway 不可达（`code <= 0`）时维持快失败策略：标记 `Gateway unreachable` 并短退避（8s），避免持续触发外部命令阻塞。
4. CLI fallback 增加失败重试冷却（30s）：`openclaw gateway call sessions.list --json` 超时或失败后，不再每轮触发子进程；冷却期内刷新直接复用当前状态。
5. Health 侧观测已改善：冷却期 `session_refresh` 返回 `ok`（空会话视为可接受状态），不再持续产出 `Session refresh failed: Invalid sessions JSON` 告警刷屏。

## UI-11 SVG 像素细节收尾（2026-04-26 第六轮）

1. 主骨架坐标对齐：布局计算改为贴合 `UI-11-chat-mode.svg` 基准，导航栏最小宽度提升到 82（DPI 缩放后等比），左面板与导航栏取消横向额外偏移，左右主面板间距统一为 7px（缩放后等比）。
2. 左面板纵向对齐：左面板增加 7px 顶部内缩（与 SVG 的 y=48 对齐），并同步修正侧栏子容器（Bot Sidebar、Files 模块容器）的高度计算，避免 resize 后底部溢出。
3. 标题栏样式对齐：标题栏底色切换为 `#0A0C10`，窗口控制区改为 `30x17` 的横向三段布局（最小化透明态、最大化深色胶囊、关闭按钮浅绿色描边态），并在 `ui_relayout_all()` 中保持同样重排规则。
4. 右侧 Chat 布局收敛：Trace/消息区卡片统一 8px 左右边距；消息区高度计算去掉旧 `CHAT_GAP` 额外扣减，使 chat_feed 与 ctrl_bar 贴合，不再出现中间冗余空隙；输入框动态伸缩时同样采用新公式。
5. 控制条细节修正：`ctrl_bar` 宽度改为与右面板同宽，四个胶囊控件采用固定优先宽度并在窄窗口下回退，避免出现首项严重截断；右侧 `0/2000` 计数保持固定对齐。
6. 主题刷新一致性：`apply_theme_to_all()` 增加标题栏/左导航/右面板样式兜底，避免主题切换或重排后被旧配色回写导致偏离 SVG。

## UI-01 Network 语义修复（2026-04-26 第七轮）

1. BootCheck `Network` 判定逻辑已改为“先判定通网，再附带 API 可达性”：
	- 无法联网：`Error`（Network unreachable）
	- 可联网但 `openrouter.ai` 不可达：`Warn`
	- 可联网且 `openrouter.ai` 可达：`Ok`
2. 启动页 BootCheck 的 `Network` 行细节文本已由 `openrouter.ai` 调整为 `internet`，避免将通网性误读为单一 API 可达性。
3. 向导 Step1 与启动异步检测中的 `net_ok` 判定已统一为通网检测（`https://www.google.com`），与 `Network` 项语义一致。

## UI-11 / C3 运行时配置整合（2026-04-26 第八轮）

1. 新增三运行时配置档（OpenClaw/Hermes/Claude）：
	- 配置项：`model + api_key`，独立存储到 `APPDATA\\AnyClaw_LVGL\\runtime_profiles.json`。
	- 启动时按本机现有 OpenClaw 配置自动预填，作为默认模板。
2. Settings -> Agent 页面新增统一配置卡片：
	- 每个运行时卡片包含：模型输入、API Key 输入、状态提示、`保存` 与 `应用` 按钮。
	- 卡片头部增加运行时缩写徽章（OC/HM/CL）并使用差异化背景色。
3. 主界面控制条 Agent 下拉增强：
	- 下拉项改为 `OC/HM/CL` 前缀，切换时显示差异化芯片背景（OpenClaw/Hermes/Claude）。
	- 切换前增加运行时配置完整性校验，避免切换到未配置状态。
4. Chat 发送链路接入运行时配置：
	- 当激活运行时为 Hermes/Claude 时，发送前自动应用对应 `model/api_key` 到网关配置。
	- 配置缺失或应用失败时，前置拦截并给出明确提示，避免发送后空响应。
