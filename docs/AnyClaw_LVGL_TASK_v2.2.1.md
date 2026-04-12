# AnyClaw LVGL v2.2.1 — PRD 对齐扫描任务清单

> 扫描基准：PRD v2.2.1 vs `src/` 代码库 | 扫描时间：2026-04-12

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
| §8 | UI 控制权模式 | ⏳ 待扫描 | — |
| §9 | 工作区管理 | ✅ 扫描完成 | 无 bug（模板不一致为设计如此） |
| §10 | 权限系统 | ✅ 扫描完成 | 无 bug（决策映射经 trace 确认正确） |
| §11 | OpenClaw 集成 | ⏳ 待扫描 | — |
| §12 | 模型与 API 配置 | ⏳ 待扫描 | — |
| §13 | 设置 | ⏳ 待扫描 | — |
| §14 | 外观与主题 | ⏳ 待扫描 | — |
| §15 | 系统管理 | ⏳ 待扫描 | — |
| §16 | 技术规格 | ⏳ 待扫描 | — |
| §17 | 远程协作与高级功能 | ⏳ 待扫描 | — |
| §18 | 分发与增长 | ⏳ 待扫描 | — |

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

1. **§8** UI 控制权模式 — 检查 AI 模拟输入/点击的安全逻辑
2. **§11** OpenClaw 集成 — 检查 Gateway 健康检测、自动重启
3. **§12** 模型与 API 配置 — 检查 model dropdown、failover 逻辑
4. **§13** 设置 — 检查各 tab 功能完整性
5. **§14** 外观与主题 — 检查主题切换完整性
6. **§15** 系统管理 — 检查日志轮转、迁移
7. **§16** 技术规格 — 检查窗口/DPI/IME 实现
8. **§17** 远程协作 — 检查远程会话状态机
9. **§18** 分发与增长 — 检查 license/growth 逻辑
