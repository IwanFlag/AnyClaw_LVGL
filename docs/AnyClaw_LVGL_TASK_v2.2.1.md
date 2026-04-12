# AnyClaw LVGL v2.2.1 — PRD 对齐扫描任务清单

> 扫描基准：PRD v2.2.1 vs `src/` 代码库 | 扫描时间：2026-04-12（持续中）

## 状态定义
- ❌ 未实现：PRD 有描述，代码完全没做
- 🔧 部分实现：代码有骨架但未闭环
- ⚠️ 已实现但需修正：代码做了但不符合 PRD 规格
- ✅ 已实现：符合 PRD（不列入）
- 🐛 Bug 已修：发现并已修复
- 🐛 Bug 待修：发现待修复

---

## 扫描进度总览

| 章节 | 名称 | 状态 | 发现 |
|------|------|------|------|
| §1 | 产品概述 | ⏭️ 跳过（非功能模块） | — |
| §2 | 安装与首次启动 | ✅ 扫描完成 | 无 bug |
| §3 | 主界面 | ✅ 扫描完成 | 无 bug |
| §4 | Chat 模式 | ✅ 扫描完成 | 1 bug 已修 |
| §5 | Work 模式 | ✅ 扫描完成 | 1 bug 已修 |
| §6 | 模式切换与 Agent 行为 | ✅ 扫描完成 | 无 bug |
| §7 | AI 交互模式 | ✅ 扫描完成 | 无 bug |
| §8 | UI 控制权模式 | ✅ 扫描完成 | 无 bug |
| §9 | 工作区管理 | ✅ 扫描完成 | 无 bug（模板不一致为设计如此） |
| §10 | 权限系统 | ✅ 扫描完成 | 无 bug（决策映射经 trace 确认正确） |
| §11 | OpenClaw 集成 | ✅ 扫描完成 | 无 bug（自动重启逻辑符合 PRD） |
| §12 | 模型与 API 配置 | ✅ 扫描完成 | 1 bug 已修（failover 速度公式） |
| §13 | 设置 | ✅ 扫描完成 | 无 bug（19 项权限完整显示） |
| §14 | 外观与主题 | ✅ 扫描完成 | 无 bug（三套主题完整） |
| §15 | 系统管理 | ✅ 扫描完成 | 1 bug 已修（license 试用期逻辑） |
| §16 | 技术规格 | ✅ 扫描完成 | 无 bug（DPI/IME/日志轮转正常） |
| §17 | 远程协作与高级功能 | ✅ 扫描完成 | 无 bug（状态机/token旋转/KB评分正确） |
| §18 | 分发与增长 | ✅ 扫描完成 | 无 bug（license 激活流程正确） |

**下次从 §8 开始继续扫描。**

---

## 已修复 Bug（本轮）

### Bug-001: §4 KB 上下文覆盖附件信息 🐛→✅
- **文件**: `src/ui_main.cpp` → `submit_prompt_to_chat()`
- **现象**: 用户同时附加文件且 KB 有匹配上下文时，KB 分支 `payload = kb_ctx + "\nUser request:\n" + text` 直接覆盖了已附加附件信息的 payload
- **修复**: 改为 `payload = kb_ctx + "\nUser request:\n" + payload`，保留附件信息
- **提交**: `1d2b8c8`

### Bug-002: §5 Work 垂直分隔条拖拽松手不重置颜色 🐛→✅
- **文件**: `src/ui_main.cpp` → `work_vertical_splitter_cb()`
- **现象**: 拖拽步骤流/输出面板之间的垂直分隔条时，松手后分隔条保持高亮蓝色不恢复
- **修复**: 添加 `LV_EVENT_RELEASED` 回调重置颜色 + `LV_EVENT_PRESSED` 回调设置高亮色 + 注册 RELEASED 事件
- **提交**: `1d2b8c8`

### Bug-003: §12 Failover 速度公式分母错误 🐛→✅
- **文件**: `src/model_manager.cpp` → `failover_calc_score()`
- **现象**: PRD 定义 `speed = 1.0 - (latency / 15000) clamp 0~1`，代码用了 30000，导致速度维度区分度减半
- **修复**: 两处 30000 → 15000（计算 + debug log）
- **提交**: `9a0dbde`

### Bug-004: §15 License 试用期逻辑反转 🐛→✅
- **文件**: `src/license.cpp` → `license_is_valid()` + `license_remaining_seconds()` + `license_get_remaining_str()`
- **现象**: PRD 规定 `expiry <= 0` 为试用期（始终有效），但 `license_is_valid()` 返回 false；`license_remaining_seconds()` 返回 0
- **修复**:
  - `license_is_valid()`: expiry<=0 时 return true
  - `license_remaining_seconds()`: expiry<=0 时 return 9999999
  - `license_get_remaining_str()`: secs>=9999999 时显示 "Trial"
- **提交**: `1e0061e`

---

## 上一轮 Bug（已修，提交 `022be5a`）

| # | Bug | 文件 | 状态 |
|---|-----|------|------|
| 2 | strncpy 未保证 null 终止 | garlic_dock.cpp | ✅ |
| 3 | sizeof 跨编译单元 | main.cpp/ui_main.cpp/app.h | ✅ |
| 4 | static char answer 竞态 | ui_main.cpp | ✅ |
| 5 | g_stream_done 直读竞态 | http_client.cpp | ✅ |
| 6 | b32table 初始化竞态 | license.cpp | ✅ |
| 8 | config.json 重复读 4 次 | ui_main.cpp | ✅ |
| 11 | json_find_key token 边界 | json_util.cpp | ✅ |

---

## 待扫描项（下次继续）

**全量扫描 §2~§18 已完成。** 下一步方向：

1. **深度复查** — 对 🔧 部分实现的模块（Work 模式 StepCard/UI-23~26、IM 连接 UI-08 等）做代码级 bug 检查
2. **UI 线框对齐** — 对照 `UI_layouts.md` 的 65 个场景，逐个检查 UI 代码布局常量/颜色是否匹配 Design System
3. **安全审计** — 检查 `is_path_allowed` 边界穿越检测、API Key 内存擦除、沙箱执行隔离
4. **性能验证** — 对照 §24 性能预算（启动<3s、内存<200MB、流式50ms/3字符等）
5. **新增模块扫描** — 若后续 PRD 新增章节，从本清单末尾继续

**扫描线记录**：`2026-04-12 17:31` | 全量首次通过 | 共修 4 bug | 提交 9a0dbde + 1e0061e
