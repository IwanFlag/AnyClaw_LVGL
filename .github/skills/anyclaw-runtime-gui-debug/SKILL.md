---
name: anyclaw-runtime-gui-debug
description: "AnyClaw 端到端调试与修复工作流。Use when: AnyClaw 卡死、聊天不回复、OpenClaw/Hermes/Claude 运行时切换异常、GUI 交互失效、需要自动化点击与快捷键、需要全屏截图分析、需要压测与回归验证、需要外部工具安装并联调。"
---

# AnyClaw Runtime + GUI Debug Skill

## 目标

用可复现、可回归、可审计的方式完成以下闭环：

1. 自主定位与修复代码问题（不只分析，直接落地修复）。
2. 覆盖 OpenClaw / Hermes / Claude 三运行时聊天链路。
3. 覆盖 GUI 操作链路：点击、快捷键、菜单、弹层、切换。
4. 覆盖证据采集：日志、全屏截图、自动化操作记录、构建结果。
5. 覆盖压力与回归：短压测 + 稳定性验证。

## 适用场景关键词

- AnyClaw 卡死
- Settings 卡死
- 聊天无回复
- 第二条消息超时
- Runtime 切换无效
- OpenClaw/Hermes/Claude 调不通
- GUI 按钮无响应
- 需要截图分析
- 需要 computer-use 操作 GUI
- 需要快捷键自动化

## 工具清单

1. 构建与诊断
- CMake Tools 构建
- 问题面板诊断

2. GUI 自动化
- mano-cua: C:/Users/thundersoft/mano-cua/mano-cua.exe
- 规则: 先确保 AnyClaw 在前台，再执行自动化

3. 全屏截图
- Screenshot.exe: C:/Users/thundersoft/.openclaw/skills/screenshot/recompiled/Screenshot.exe
- 推荐命令:
  - C:/Users/thundersoft/.openclaw/skills/screenshot/recompiled/Screenshot.exe "D:/Screenshots"

4. 日志
- App 日志: %APPDATA%/AnyClaw_LVGL/logs/anyclaw_app.log
- 关键关键词: WATCHDOG, stalled, SETTINGS, Chat, Runtime, timeout, error

5. 外部工具安装（按需）
- 仅安装必要工具，不修改已有环境变量
- 如必须变更环境变量，先征求用户确认

## 标准执行流程（必须按序）

### Phase 1: 复现与证据固定

1. 启动应用并记录进程状态（Responding、CPU、启动时间）。
2. 复现问题场景（例如进入 Settings、切 Runtime、发送消息）。
3. 抓取日志窗口（最近 100-300 行）。
4. 执行全屏截图，记录操作前后界面差异。
5. 若是 GUI 问题，执行一次 mano-cua 自动化重放，保留命令与结果。

输出物：
- 复现步骤
- 关键日志
- 截图路径
- 自动化命令记录

### Phase 2: 定位与最小修复

1. 从日志和代码定位根因（避免拍脑袋修复）。
2. 优先做最小可验证修改：
- 不改无关逻辑
- 不大规模重构
- 先修主故障链路
3. 在关键路径补充低成本诊断日志（可后续降级）。

输出物：
- 根因说明
- 修改文件清单
- 为什么这样修

### Phase 3: 编译与功能回归

1. 构建工程并处理编译/链接错误。
2. 重新运行应用，验证主问题是否消失。
3. 至少验证一次完整用户路径：
- 打开主界面
- 打开 Settings
- 返回聊天
- 发送消息并收到回复

输出物：
- 构建成功证据
- 回归通过证据

### Phase 4: 三运行时联调（OpenClaw/Hermes/Claude）

1. 逐个切换 Runtime：OpenClaw -> Hermes -> Claude。
2. 每个 Runtime 做最小聊天用例：
- 发送短提示词
- 期待可解析输出
3. 若某 Runtime 未安装或配置缺失：
- 明确告警
- 给出自动降级策略（例如回退 OpenClaw）
- 不允许导致 UI 卡死
4. 验证切换后确实生效（日志中应可见 active runtime / route model 变化）。

输出物：
- 三运行时通过/失败矩阵
- 每项失败的原因与修复状态

### Phase 5: 压测与稳定性验证

1. 快速压测（建议 2-5 分钟）：
- 连续发送多轮消息
- 中间切换 Runtime
- 插入 GUI 操作（切 tab、开关设置页）
2. 观察：
- 主进程是否持续 Responding
- 是否出现 WATCHDOG stalled
- 是否出现持续超时
3. 压测后执行一次全屏截图与日志收尾。

输出物：
- 压测结论（通过/失败）
- 失败时给出下一轮修复入口

## GUI 操作规范

1. 自动化前把 AnyClaw 切到前台。
2. 优先使用语义指令（按钮文本、区域名称），减少硬编码坐标。
3. 坐标操作必须注明是物理像素坐标。
4. 支持快捷键路径验证，例如：
- Ctrl+, 打开设置
- Enter 触发发送
- Esc 关闭弹层

## 质量门禁（通过前必须满足）

1. 无新增编译错误。
2. 主流程无卡死（含 Settings 与聊天发送）。
3. 至少一个 Runtime 能稳定回复；目标是三运行时都能回复。
4. 有完整证据链：日志 + 截图 + 操作步骤。
5. 修复说明必须可复现、可回归。

## 常见故障与处理

1. 链接失败 LNK1168
- 先结束运行中的 AnyClaw_LVGL 进程，再重建。

2. GUI 自动化点错窗口
- 先前台激活 AnyClaw，再执行 mano-cua。

3. 聊天 200 但无文本
- 检查 SSE 与 plain JSON 双路径解析。
- 检查 runtime/model/provider/apiKey 绑定关系。

4. Runtime 切换后像没生效
- 检查 active runtime 持久化。
- 检查发送时 route model 与 provider 配置是否同步到网关。

## 交付格式

每次执行结束至少输出：

1. 本轮修改文件列表。
2. 构建结果。
3. OpenClaw/Hermes/Claude 三运行时状态。
4. GUI 验证结果（含截图路径）。
5. 剩余风险与下一步建议。
