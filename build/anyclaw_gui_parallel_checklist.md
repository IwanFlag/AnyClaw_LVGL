# AnyClaw 模块并行对照清单（PRD × Design × GUI）

- 版本: v0.2
- 更新时间: 2026-04-22
- 维护策略: 只追加 Delta，不重写历史
- 基线文档:
  - PRD: [docs/PRD.md](../docs/PRD.md)
  - Design: [docs/Design.md](../docs/Design.md)

## 模块总览（当前）

| 模块 | 用户价值 | 复杂度 | 当前问题 | 优先级 | 状态 |
|---|---|---|---|---|---|
| 首次使用链路 | 高 | 高 | 决策密度过高 | P0 | 进行中 |
| 对话与任务链路 | 高 | 中 | 模式心智成本偏高 | P1 | 待开始 |
| Agent 中心 | 中高 | 中高 | 状态定义不统一 | P1 | 待开始 |
| 系统诊断中心 | 中 | 中 | 信息分散、入口过多 | P2 | 待开始 |

## 首次使用链路改造 v1（用户视角）

### 目标

- 3 分钟内完成“可用闭环”：可发送消息、可收到回复、可看到执行反馈。
- 技术概念后置：首次不强迫用户理解 Runtime/Leader/Hermes/Claude。
- 把“必须决策”降到最少：首次只做 2 个关键决策。

### 当前痛点（来自现有实现映射）

- 向导虽为 7 步，但 Step 2 承载过多决策（模型 + API + 可选运行时安装），用户易卡顿。
  - 代码位点: [src/ui_main.cpp](../src/ui_main.cpp#L10659)
- Leader 属于高级策略，但在首次流程中认知成本偏高。
  - 代码位点: [src/ui_main.cpp](../src/ui_main.cpp#L11091)
- 设置页入口完整但对新用户信息密度偏高。
  - 代码位点: [src/ui_settings.cpp](../src/ui_settings.cpp#L3470)

### 改造原则

1. 首次流程只保留“能用”相关项，其他一律延后到“完成后推荐”。
2. 每步只允许一个主任务，避免并行决策。
3. 每一步必须给出可见的通过条件和失败恢复。

### 新流程（首次 4 步）

1. 语言与欢迎
- 内容: 语言选择 + 一句话承诺（3 分钟可用）
- 通过条件: 选择语言即可 Next

2. 环境与默认能力就绪
- 内容: 自动检测 + 一键修复（用户只看“就绪/修复中/失败”）
- 通过条件: 关键依赖可用
- 失败处理: 给出一个主按钮“自动修复”，二级按钮“稍后手动处理”

3. 模型与密钥（最小闭环）
- 内容: 只显示一个推荐模型 + API Key 输入
- 通过条件: API Key 非空且格式基本可用
- 失败处理: 明确错误摘要 + 重试

4. 完成页（高级能力后置）
- 内容: 进入主界面主按钮 + 高级能力推荐卡（Leader、Hermes、Claude、IM）
- 通过条件: 点击“进入 AnyClaw”

### 延后流程（从首次剥离）

- Local 模型安装
- IM 连接
- Leader 模式决策
- 可选运行时安装

这些项改为“进入主界面后的推荐任务清单”，避免首次阻塞。

### PRD 需要改动点

1. 把“首次向导 7 步”改为“双阶段”：
- 阶段 A: 首次可用闭环（4 步）
- 阶段 B: 进阶能力引导（任务清单，不阻断）

2. 增加 Step 3 验收条款：
- API 校验失败必须有具体错误原因。
- Next 按钮禁用条件要明确写入 PRD。

3. 增加“跳过策略统一说明”：
- 跳过进阶能力不影响基础聊天与任务执行。

### Design 需要改动点

1. 向导信息密度规范
- 每步只允许一个一级标题 + 一个主行动按钮。
- 同屏控制在一个决策点。

2. 状态语义规范
- 统一状态色和文案: `就绪` / `修复中` / `需处理` / `失败可重试`。

3. 完成页组件规范
- 主按钮: 进入 AnyClaw
- 次级卡片: 进阶能力（可关闭）

### 可验收标准（可转测试）

1. 首次用户在 3 分钟内完成向导并可发起一次对话。
2. 首次流程中不出现 Runtime/Leader 的强制决策弹窗。
3. 任一步失败时均可重试，不会卡死在不可前进状态。
4. 完成后推荐清单可在设置页复访，不影响主流程。

## Delta

### Delta 2026-04-22 #2

- 新建 build 跟踪文档: [build/anyclaw_gui_parallel_checklist.md](anyclaw_gui_parallel_checklist.md)
- 写入“首次使用链路改造 v1”完整草案。
- 下一轮计划: 输出 Chat/Work 链路改造 v1，并补“可合并的 4 大中心”具体 IA 图。

## Chat/Work 链路改造 v1（用户视角）

### 问题总结

- 用户先要理解 Chat 与 Work 的差别，再决定入口，学习成本高。
- 任务输入与执行反馈在认知上割裂，用户会怀疑“我到底在聊天还是在执行任务”。

### 改造目标

- 一个输入入口，两种结果形态（对话答复 / 任务执行）自动分流。
- 用户不必先学模式，先完成任务再理解模式。

### 交互方案

1. 统一输入框
- 默认文案: “问我问题，或交给我做一件事”
- 用户提交后，系统自动判断是 `Q&A` 还是 `Task`。

2. 结果双区同屏
- 左侧: 对话结果摘要
- 右侧: 执行轨迹（仅任务时出现）

3. 一键切换可见层
- 保留 Chat/Work 切换，但降级为“显示层切换”，不是“入口切换”。

### 验收标准

1. 新用户无需切换模式即可完成一次任务请求。
2. 执行型请求必须出现可见进度与结果摘要。
3. 普通问答请求不出现冗余执行面板。

## 合并为 4 大中心（IA v1）

### C1 开始使用中心

- 目标: 快速可用
- 含: 向导、环境修复、基础模型/API

### C2 对话与任务中心

- 目标: 完成日常使用
- 含: 输入区、结果区、执行轨迹、会话管理

### C3 Agent 中心

- 目标: 管理能力上限
- 含: Runtime、Leader、可选 Agent、权限策略

### C4 系统与诊断中心

- 目标: 保障稳定性
- 含: 日志、Feature、Tracing、版本与更新

### 合并落地原则

1. 对用户高频路径只暴露 C1/C2。
2. C3/C4 作为进阶入口，默认低曝光。
3. 相同配置只保留一个主入口，其他地方仅做快捷跳转。

## Delta

### Delta 2026-04-22 #3

- 追加 Chat/Work 链路改造 v1。
- 追加“合并为 4 大中心”IA v1。
- 下一轮计划: 把 C1~C4 映射到现有设置页 Tabs，给出“保留/合并/下沉”清单。

## C1~C4 与现有 Tabs 映射（执行清单）

参考当前代码: [src/ui_settings.cpp](../src/ui_settings.cpp#L3473)

| 现有Tab | 归属中心 | 建议动作 | 说明 |
|---|---|---|---|
| General | C1 开始使用中心 | 保留 | 承担高频基础配置 |
| Model | C1 开始使用中心 | 合并到 C1 | 与首次可用闭环强相关 |
| Agent | C3 Agent 中心 | 保留 | 运行时与Leader治理核心 |
| Permissions | C3 Agent 中心 | 合并到 C3 | 与Agent执行策略强相关 |
| KB | C3 Agent 中心 | 下沉为二级页 | 进阶能力，不应首层高曝光 |
| Log | C4 系统与诊断中心 | 合并到 C4 | 诊断类 |
| Feature | C4 系统与诊断中心 | 合并到 C4 | 诊断/实验类 |
| Tracing | C4 系统与诊断中心 | 合并到 C4 | 诊断类 |
| About | C4 系统与诊断中心 | 合并到 C4 | 系统信息类 |

## 执行优先级（按用户影响）

1. P0: C1 合并（General + Model）
2. P1: C3 合并（Agent + Permissions，KB 下沉）
3. P2: C4 合并（Log + Feature + Tracing + About）

## Delta

### Delta 2026-04-22 #4

- 追加 C1~C4 与现有 Tabs 的“保留/合并/下沉”映射。
- 明确执行优先级为 P0/P1/P2 三段。

### Delta 2026-04-22 #5

- PRD 已落地双阶段向导模型（阶段 A: 4 步可用闭环，阶段 B: 进阶能力推荐）。
- PRD 已增加 WIZ-01 可验收标准（3分钟可用、非阻断、可重试、可复访）。
- 影响文档: [docs/PRD.md](../docs/PRD.md)

### Delta 2026-04-22 #6

- Design 已同步首次体验为双阶段向导（阶段 A 4 步 + 阶段 B 非阻断）。
- Design 已把 UI-05 从“安装导向”收敛为“环境就绪导向”，统一状态语义与主按钮策略。
- 影响文档: [docs/Design.md](../docs/Design.md)

### Delta 2026-04-22 #7

- PRD 设置章节已补齐实际 Tabs（含 KB/Feature/Tracing）。
- PRD 已新增“用户视角 IA（4 大中心）”与曝光策略。
- 影响文档: [docs/PRD.md](../docs/PRD.md)

### Delta 2026-04-22 #8

- Design 已新增 UI-45A（C1 开始使用中心）页面级规格：字段、按钮、状态、错误摘要、禁用条件。
- PRD 已新增 11.1.2 C1 可执行验收条款，打通实现与测试口径。
- 影响文档: [docs/Design.md](../docs/Design.md), [docs/PRD.md](../docs/PRD.md)

### Delta 2026-04-22 #9

- PRD 已新增 C2 对话与任务中心统一入口规范（3.1.1）。
- Design 已新增 UI-14A（统一输入与双结果视图）交互与降级规则。
- 影响文档: [docs/PRD.md](../docs/PRD.md), [docs/Design.md](../docs/Design.md)

### Delta 2026-04-22 #10

- PRD 已补齐 C3/C4 可执行验收（11.1.3 / 11.1.4）。
- Design 已补齐 UI-46A（C3 Agent 中心）与 UI-48A（C4 诊断中心）页面级规格。
- 影响文档: [docs/PRD.md](../docs/PRD.md), [docs/Design.md](../docs/Design.md)

### Delta 2026-04-22 #11

- 已开始代码实现 C3：Agent Tab 增加运行时状态机（Healthy/Installed-Unhealthy/Missing/Conflict）。
- 已实现运行时副作用提示（仅新会话生效）、冲突阻断保存、重检状态按钮。
- 已实现运行时回滚按钮（回到上一个已提交运行时）。
- 影响代码: [src/ui_settings.cpp](../src/ui_settings.cpp)

### Delta 2026-04-22 #12

- 已实现 C4 首批代码：Feature/Tracing 页新增“实验/调试风险”标识文案。
- 目标：降低高影响开关的误操作预期偏差。
- 影响代码: [src/ui_settings.cpp](../src/ui_settings.cpp)

### Delta 2026-04-22 #13

- 已实现高影响操作确认弹窗：Log 清空、Tracing 清空均需二次确认。
- 已优化 C3 刷新策略：输入变化走轻量刷新，手动“重检状态”走深度检测。
- 影响代码: [src/ui_settings.cpp](../src/ui_settings.cpp)

### Delta 2026-04-22 #14

- 已实现 Agent 安装流程并发保护：安装中禁用保存与安装按钮，防止重复点击。
- 安装状态通过轮询自动恢复按钮可用性。
- 影响代码: [src/ui_settings.cpp](../src/ui_settings.cpp)

### Delta 2026-04-22 #15

- 已实现 C1 模型/API 闭环入口：新增“保存并测试（Save & Test）”按钮。
- 已实现 API Key 必填禁用逻辑：无 key 时禁用 Verify/Save/Save & Test。
- 影响代码: [src/ui_settings.cpp](../src/ui_settings.cpp)

### Delta 2026-04-22 #16

- 已实现 C3 异步重检：点击“重检状态”走后台线程，完成后异步回填 UI。
- 已实现 About 高影响操作确认：清除聊天、导入迁移需二次确认。
- 已细化 Save & Test 失败分类：鉴权失败/限流/服务不可用/网络异常。
- 影响代码: [src/ui_settings.cpp](../src/ui_settings.cpp)

### Delta 2026-04-22 #17

- 已实现 C2 统一入口增强：Chat 发送可镜像到 Work 结果视图（Dual 开关）。
- 已新增 Chat 侧任务快捷入口：`W` 按钮可直接按 Work 任务路径发起执行并切到 Work 页。
- 已完成 C2 双结果视图首版：统一输入一次，Chat/Work 两侧均可看到任务上下文与后续结果。
- 已完成向导阶段化表达：Step 标题与步骤条改为 `Phase A / Phase B` 心智模型。
- 影响代码: [src/ui_main.cpp](../src/ui_main.cpp)

### Delta 2026-04-22 #18

- 已实现 C2 稳定双结果布局：Dual 开启时 Work Chat 面板固定展开并放大宽度，避免折叠导致结果丢焦。
- 已实现统一入口快捷分发：Chat 输入支持 `Ctrl+Enter` 直达 Work 任务路径。
- 已实现双向上下文增强：Chat/Work 发送均追加到 Work Chat Feed，便于同屏追踪执行脉络。
- 已更新输入提示文案：明确 Shift+Enter 换行、Ctrl+Enter 任务。
- 影响代码: [src/ui_main.cpp](../src/ui_main.cpp)

### Delta 2026-04-22 #19

- 已实现 Work 双栏避让布局：Dual 开启时主内容区自动预留右栏空间，减少遮挡与覆盖。
- 已实现 Work 输入快捷发送：`Enter` 直接发送任务，`Shift+Enter` 保留换行。
- 已实现 Work 面板宽度重算策略：窗口缩放时右栏与主内容同步重排。
- 影响代码: [src/ui_main.cpp](../src/ui_main.cpp)

### Delta 2026-04-22 #20

- 已将首次向导主流程收敛为阶段 A 四步闭环（语言 -> 环境就绪 -> 模型/API -> 完成），去除阶段 B 在主流程中的阻断路径。
- 已在完成页补充“阶段 B 非阻断推荐”说明（本地模型/IM/Leader/Runtime），与 PRD/Design 的双阶段模型对齐。
- 已修复主题 CJK 字体映射：改为有效的 `NotoSansSC` 字体文件，避免占位字体导致中文乱码。
- 影响代码: [src/ui_main.cpp](../src/ui_main.cpp)

### Delta 2026-04-22 #21

- 已将设置面板改为首次打开时懒初始化，移除启动阶段预构建，避免在 LVGL 定时器早期执行大块 UI 构建导致主线程长阻塞。
- 已调整托盘消息泵为“预算内泵全线程消息”，防止仅泵托盘窗口导致主窗口消息饥饿。
- 挂起复测结果：标准 6 秒窗口内 `Responding=True` 且 `IsHungAppWindow=False`，主循环心跳连续。
- 影响代码: [src/ui_main.cpp](../src/ui_main.cpp), [src/ui_settings.cpp](../src/ui_settings.cpp), [src/tray.cpp](../src/tray.cpp)

### Delta 2026-04-22 #22

- 已完成标题栏入口收敛：移除标题栏中的设置入口与模式条重复创建逻辑，避免与左导航/主工作区职责冲突。
- 目标：减少同一功能多入口造成的心智分裂，为后续 IA 收口打基础。
- 影响代码: [src/ui_main.cpp](../src/ui_main.cpp)
- 关联提交: `b7eeb6d`

### Delta 2026-04-22 #23

- 已完成左侧中部导航真实切页：Bot / Tasks / Resources 按钮不再只做高亮，已接入实际页面切换行为。
- 已实现主标题动态联动：标题随模块与 Chat/Work 显示层变化更新。
- 已新增非 Bot 模块占位页与布局策略：进入 Tasks/Resources 时右侧全宽展示占位内容，Bot 区域正常隐藏/恢复。
- 影响代码: [src/ui_main.cpp](../src/ui_main.cpp)
- 关联提交: `7c96d92`

### Delta 2026-04-22 #24

- 已完成 C2 单入口收敛：Work Chat 次输入与发送按钮统一隐藏，保留双结果展示能力。
- 已将控制动作 `send_work_message` 统一路由到主任务输入框，避免旁路输入导致行为不一致。
- 影响代码: [src/ui_main.cpp](../src/ui_main.cpp)
- 关联提交: `f822da6`

### Delta 2026-04-22 #25

- 已完成设置页 IA 文案对齐：C2/C3/C4 页签与页内标题说明统一到“中心”语义。
- 已补充 C4 页首说明，明确该中心承载日志、特性、Tracing、关于信息。
- 影响代码: [src/ui_settings.cpp](../src/ui_settings.cpp)
- 关联提交: `6790ebd`

### Delta 2026-04-22 #26

- 已为 C2 设置引导页增加“返回主界面”主按钮，减少用户在设置内迷航。
- 行为：点击后直接关闭设置面板，回到主工作区继续 C2 统一输入流程。
- 影响代码: [src/ui_settings.cpp](../src/ui_settings.cpp)
- 关联提交: `2dbacc9`

### Delta 2026-04-22 #27

- 已将左导航中部非 Bot 区从纯占位升级为“可交互页面骨架”：Tasks/Resources 各自独立面板、状态标签与快捷按钮。
- Tasks 提供队列刷新/执行入口；Resources 提供工作区扫描与资源目录打开入口。
- 切换逻辑已更新：根据当前模块显示对应面板并同步标题说明。
- 影响代码: [src/ui_main.cpp](../src/ui_main.cpp)

### Delta 2026-04-22 #28

- 已补齐 Tasks/Resources 骨架在窗口缩放时的自适应宽度重排。
- 目标：避免非 Bot 页面在小窗口挤压或大窗口过窄，保持卡片可读性。
- 影响代码: [src/ui_main.cpp](../src/ui_main.cpp)

### Delta 2026-04-22 #29

- 已将 Tasks/Resources 快捷按钮接入真实数据读取：
  - Tasks：读取当前任务控件数、活动会话数、Cron 会话数，并输出任务快照。
  - Resources：读取工作区根目录并统计文件/目录数量，同时统计 assets 文件数。
- 已新增对应只读快照视图，支持手动刷新与缩放自适应。
- 影响代码: [src/ui_main.cpp](../src/ui_main.cpp)

### Delta 2026-04-22 #30

- 已将 Tasks/Resources 从“统计查看”推进到“可执行操作”链路：
  - Tasks：新增会话同步（Sync Sessions）与全部会话终止（Abort All）动作，并联动状态刷新。
  - Resources：新增托管段落同步（Sync Managed）与打开工作区目录（Open Workspace）动作。
- Resources 快照已补充 workspace 健康信息（exists/writable），用于快速诊断权限与目录可用性。
- 影响代码: [src/ui_main.cpp](../src/ui_main.cpp)

### Delta 2026-04-23 #31

- 已完成 Tasks 一键任务模板执行：新增 Health Check / Workspace Sync / Resource Audit 三个模板按钮。
- 行为：点击后自动生成任务提示并立即触发 Work 执行，同时自动切换到 Work 视图查看执行过程。
- 影响代码: [src/ui_main.cpp](../src/ui_main.cpp)

### Delta 2026-04-23 #32

- 已完成 Resources 分组筛选能力：新增 All / Fonts / Icons / Sounds 四个筛选按钮。
- 快照视图已扩展为“分组统计 + 按筛选列出条目”，支持快速定位资源目录内容。
- 影响代码: [src/ui_main.cpp](../src/ui_main.cpp)

