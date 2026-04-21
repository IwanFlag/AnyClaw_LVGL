# AnyClaw LVGL v2.2.2 实现追踪

> 基于 PRD v2.2.2 + Design.md v2.2.2，Auto 模式

---

## 文档更新

| 位置 | 改动 | 状态 |
|------|------|------|
| Design.md UI-11/12 ASCII 图 | 移除分隔条，控制行改为 [Agent▼][Report][AI行为▼] | ✅ 已完成 |
| Design.md §15 控制行 | 同上 | ✅ 已完成 |
| Design.md §15 UI-22 | 更新 Agent/Report/AI行为 | ✅ 已完成 |
| Design.md 向导 6→7 步 | Step2=Agent, Step6=Leader, Step7=个人信息 | ✅ 已完成 |
| Design.md §12 窗口系统 | 重读完整确认无报错 | ✅ 已完成 |

---

## 代码改动

### C1. 控制栏重组 — [Agent▼][Report][AI行为▼]

| 子任务 | 状态 |
|--------|------|
| 顺序改为 Agent▼ → Report → AI行为▼ | ✅ 已完成 |
| Agent 下拉选项改为 OpenClaw/Hermes/Claude | ✅ 已完成 |
| 删除 AI托管 下拉 (ctrl_dd_ai_host) | ✅ 已完成 |
| 删除 Text\|Voice 按钮 (ctrl_btn_text_voice) | ✅ 已完成 |
| 添加 Report 按钮 | ✅ 已完成 |
| Agent 下拉添加回调（模型联动） | ✅ 已完成(stub) |

**位置：** `src/ui_main.cpp:11896-11964`
**依赖：** C5

---

### C2. 左 NAV 底部 Chat/Work 切换按钮

| 子任务 | 状态 |
|--------|------|
| nav_session 改为全局变量 + 点击回调 | ✅ 已完成 |
| 初始图标：Chat=⇢ / Work=◂ | ✅ 已完成 |
| apply_mode_switch_visuals() 更新图标 | ✅ 已完成 |
| 颜色：accent（当前模式）/ dim（另一个） | ✅ 已完成 |

**位置：** `src/ui_main.cpp:11694-11711` + `apply_mode_switch_visuals()`

---

### C3. Work 模式 "Open Last Boot Report" → "Last Report"

| 子任务 | 状态 |
|--------|------|
| 行 12044 文字修改 | ✅ 已完成 |

---

### C2. 左 NAV 底部 Chat/Work 切换按钮

| 子任务 | 状态 |
|--------|------|
| 查找左 NAV 代码位置 | 🔍 待完成 |
| 单按钮图标替换会话图标 | 🔍 待完成 |
| hover tooltip 显示模式 | 🔍 待完成 |
| 切换逻辑 < 150ms | 🔍 待完成 |

**PRD：** 左 NAV 底部，单按钮，替换会话图标位置

---

### C3. 移除分隔条

| 子任务 | 状态 |
|--------|------|
| 查找分隔线代码 | 🔍 待完成 |

**PRD：** 左 NAV 与内容区之间无分隔条

---

### C4. 左侧面板自动刷新（2s）

| 子任务 | 状态 |
|--------|------|
| 确认 Session/Cron 列表是否已有 2s 刷新 | 🔍 待完成 |

**PRD：** Chat 模式左侧面板 2s 间隔自动刷新

---

### C5. Agent 下拉模型联动

| 子任务 | 状态 |
|--------|------|
| 定义 AgentType 枚举 | 🔍 待完成 |
| 每个 Agent 对应模型配置 | 🔍 待完成 |
| 切换 Agent 时联动切换模型 | 🔍 待完成 |

**依赖：** C1

---

### C6. Runtime Agent 安装向导 (Step 2)

| 子任务 | 状态 |
|--------|------|
| 查找向导代码位置 | 🔍 待完成 |
| Step 2 改为 Agent 安装界面 | 🔍 待完成 |

**PRD：** OpenClaw 必须 / Hermes 可选 / Claude 可选

---

### C7. Leader 模式确认 (Step 6)

| 子任务 | 状态 |
|--------|------|
| 新增 Step 6 向导页面 | 🔍 待完成 |

**PRD：** 默认开启，Skip 切换单 Agent 模式

---

### C8. Boot Check 自动运行

| 子任务 | 状态 |
|--------|------|
| 确认启动时自动跑 | 🔍 待完成 |

**PRD：** 从控制行移除，启动自动跑

---

## 编译验证

| 步骤 | 命令 | 状态 |
|------|------|------|
| 增量编译 | `pwsh -File build.ps1` | 🔍 待测试 |
| 完整重建 | 删除 build 目录后编译 | 🔍 待测试 |
| exe 启动截图验证 | — | 🔍 待测试 |
| Git push | — | 🔍 待完成 |

---

## 进度总览

```
C1 控制栏   ████████████ 100%  ✅ (origin/main 981f4bf)
C2 左NAV    ████████████ 100%  ✅ (origin/main 981f4bf)
C3 分隔条   ████████████ 100%  ✅ (origin/main 981f4bf, 像素验证通过)
C4-C8      ⏭️  v2.3 规划，跳过
编译打包    ████████████ 100%  ✅ (build PASSED, exe 运行正常)
版本号     ████████████ 100%  ✅ v2.2.1 → v2.2.11 (ui_main.cpp:8602)
模型搜索    ████████████ 100%  ✅ build_model_tab 加搜索过滤输入框 (ui_settings.cpp)
```

---

## 日志

```
[3d78fa5] feat: control bar reorder [Agent▼][Report][AI行为▼], nav bottom toggle, Last Report
[da8ca7d] docs: update Design.md v2.2.2
```
