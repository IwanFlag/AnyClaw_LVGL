# AnyClaw v2.2.1 分发与增长执行包

## 渠道执行清单

| 渠道 | 状态 | 交付物 | 备注 |
|---|---|---|---|
| GitHub Release | ready | 发布说明模板、更新要点、截图清单 | 可直接粘贴发布 |
| ai-bot.cn | ready | 产品简介、亮点、安装步骤、截图要求 | 可直接提交 |
| Product Hunt | ready | 英文简介、tagline、feature list | 作为备选渠道 |

## GitHub Release 模板

### 标题
`AnyClaw LVGL v2.2.1`

### 更新要点
- 完成 Ask 模式确认弹窗与风险动作拦截
- Work 模式支持多渲染器与步骤流
- 新增 Tracing / Feature Flags / KB 设置页
- 完成 Quick Assist 远程连接层闭环

### 验证信息
- 构建命令：`cmake --build "build-windows" --config Release`
- 产物路径：`build-windows/bin/Release/AnyClaw_LVGL.exe`

## ai-bot.cn 提交材料

- 产品名：AnyClaw LVGL
- 一句话描述：本地优先、可控权限的 AI 工作台
- 核心亮点：
  - Ask/Plan/Autonomous 三种 AI 交互模式
  - Work 模式步骤流 + 输出渲染
  - 本地 Gemma 模型与 Gateway 双路由
  - 细粒度权限与审计日志
- 安装说明：下载发布包后运行 `AnyClaw_LVGL.exe`

## 截图与演示素材清单

- `assets/screenshots/chat_mode.png`
- `assets/screenshots/work_mode.png`
- `assets/screenshots/settings_feature_flags.png`
- `assets/screenshots/settings_tracing.png`
- `assets/screenshots/ask_feedback_popup.png`

> 注：以上截图路径为统一约定，可由测试/产品同学补图。
