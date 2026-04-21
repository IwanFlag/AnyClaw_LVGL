# AnyClaw 会话总计划追踪（v2.2.3）

> 来源：本次会话（PRD + Design 梳理、与代码一致性校对、后续开发计划）
> 更新时间：2026-04-22

---

## 0. 目标范围

- [x] G0-1 梳理并统一 PRD / Design 的产品与设计描述
- [x] G0-2 合并 4T-07 关键设计规范到 Design
- [ ] G0-3 按文档补齐代码实现（主题、向导、配置、设置页）
- [ ] G0-4 修复高优先级缺陷（字体乱码、卡死、GUI 异常）

---

## 1. 文档阶段（已完成）

### 1.1 PRD.md

- [x] P1-1 Step 2 增加 Agent 安装卡片交互细节（OpenClaw/Hermes/Claude）
- [x] P1-2 Step 6 改为可跳过（Skip -> 单 Agent）
- [x] P1-3 左导航宽度统一为 窗口宽 x 6%，最小 36px
- [x] P1-4 左侧面板统一为 可用宽 x 25%，最小 160px
- [x] P1-5 Settings 增加 Agent Tab 描述

### 1.2 Design.md

- [x] D1-1 统一为三套主打主题（Matcha/Peachy/Mochi）
- [x] D1-2 修复章节编号与残留冲突（含旧主题残留）
- [x] D1-3 字体规范统一到 Noto Sans SC
- [x] D1-4 NAV_W/LEFT_PANEL_W 数值对齐代码（6%、341px）
- [x] D1-5 UI-45 主题按钮更新为 Matcha/Peachy/Mochi
- [x] D1-6 UI-52~54 更新为三套主打主题
- [x] D1-7 重写 5.3 吉祥物规范（含三主题精确色板）
- [x] D1-8 完善 5.4 托盘图标（含 Mochi + 命名规范）
- [x] D1-9 新增 5.6 资源生成工作流（rsvg-convert/pngquant/optipng）
- [x] D1-10 修复 3.3 表格列错误（Mochi 引用行）

---

## 2. 代码补齐阶段（进行中）

### 2.1 主题系统

- [x] C2-1 `src/theme.h` 主题枚举收敛为 Matcha/Peachy/Mochi
- [x] C2-2 全量更新主题分支与映射（移除 Dark/Light/Classic）
- [x] C2-3 验证配置 theme 字段与 UI 显示一致

### 2.2 向导系统（7 步）

- [x] C2-4 `src/ui_main.cpp` `WIZARD_STEPS: 6 -> 7`
- [x] C2-5 补齐 Step short names / titles 的第 6 步（Leader）
- [x] C2-6 Step 2 增加 Hermes/Claude 安装卡片 UI
- [x] C2-7 新增 Step 6 Leader 模式确认 + Skip 降级逻辑
- [x] C2-8 向导状态写入 config 字段（agent_mode/leader_mode 等）

### 2.3 配置与设置页

- [x] C2-9 `src/app_config.h` 增加 agent_mode/leader_mode/hermes_enabled/claude_code_path/active_runtime
- [x] C2-10 `src/ui_settings.cpp` 增加 Agent 管理 Tab
- [x] C2-11 General Tab 主题按钮与文档一致（三主题）

---

## 3. 缺陷修复阶段（待执行）

### 3.1 高优先级

- [ ] B3-1 字体乱码：检查 CJK fallback 链与字体加载覆盖
- [ ] B3-2 App 卡死：排查主线程阻塞（openclaw_mgr / async_task）

### 3.2 中优先级

- [ ] B3-3 GUI 异常：布局、hover、流式渲染稳定性修复

---

## 4. 验证与验收（待执行）

- [ ] V4-1 文档一致性复核（PRD <-> Design <-> 代码）
- [ ] V4-2 编译验证（MSVC 构建通过）
- [ ] V4-3 运行验证（向导7步、主题切换、设置页Tab）
- [ ] V4-4 回归验证（中文显示、托盘状态、会话流程）

---

## 5. 当前结论

- [x] 文档阶段已完成，可作为后续开发基线
- [ ] 代码阶段未开始（按本清单从 2.1 顺序推进）
