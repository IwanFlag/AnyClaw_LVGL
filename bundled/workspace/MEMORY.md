# MEMORY.md — 长期记忆

## 🚀 如果你是新 AI，从这里开始

**你的任务：PRD 驱动验证 + 修复。** 代码开发已完成，现在需要逐章对照 PRD 和 Design 文档验证功能实现，实测并修 Bug。

**按以下顺序读文件：**

| 顺序 | 文件 | 用途 |
|------|------|------|
| ① | `AnyClaw_LVGL/bundled/workspace/AGENTS.md` | 开发规范（编译命令、Git 规范）— **必读** |
| ② | `memory/anyclaw/v2.2.1-verify.md` | PRD 验证清单 — 找到第一个 ⏳ 的章节 |
| ③ | `MEMORY.md`（本文件）往下读 | 项目现状、环境状态、工作习惯 |

**每章工作循环（严格遵守）：**
1. 在 `memory/anyclaw/v2.2.1-verify.md` 找第一个 ⏳ 的 PRD 章节
2. 读该章 PRD 验收标准 + Design 对应 UI 规范
3. 对照代码逐条检查实现
4. **必须实测**：编译 → Wine 启动 → 截图验证（Wine 没装就装，跑不起来查日志解决）
5. 发现 Bug → 修 → 编译 → 复测 → 确认修复
6. 更新 `memory/anyclaw/v2.2.1-verify.md` 状态 ⏳ → ✅
7. **commit + push**（每章至少一次提交）
8. 同步复制到 `bundled/workspace/`
9. 进入下一章

---

## 项目

### AnyClaw_LVGL
- 仓库位置: `AnyClaw_LVGL/`（workspace 内）
- 上游: `git@github.com:IwanFlag/AnyClaw_LVGL.git`
- 语言: C/C++17, LVGL 9.6 + SDL2, Windows 目标平台
- AI 开发规范: `AnyClaw_LVGL/bundled/workspace/AGENTS.md`（每次会话必读）
- 任务追踪: `memory/anyclaw/v2.2.1-dev.md`
- Bug 扫描: `memory/anyclaw/v2.2.1-scan.md`
- 验证计划: `memory/anyclaw/verification-plan.md`
- Git 配置: user.email=openclaw@anyclaw.dev, user.name=OpenClaw AI

### 当前进度（截至 2026-04-15 00:25）
- 阶段一~三: ✅ 全部完成
- 阶段四 4T-00~05: ✅ 5 套主题就绪（Matcha/Peachy/Classic/Mochi/Light）
- 4T-06: ~95% 完成，剩余 FA12 字体校验需 Wine
- 4T-07: 目录结构就位，资源文件 ⏸️ 需设计工具
- 4T-08: 5/6 完成，08.6 全量回归需 Wine
- 阶段五 5T-01: ✅ 全部完成
- 5T-02: ✅ 全部完成（02.1~06，含 FreeType fallback 链）
- 5T-03: ✅ 全部完成（03.1~05）
- 5T-04: ✅ 全部完成（CI 多次 build success）
- 新增: auto_restart_on_crash config 持久化（TODO 已修复）
- 新增: widgets 目录 12 处硬编码颜色已 Token 化
- 新增: CMakeLists.txt FreeType 条件链接语法错误 + include 路径修复
- 新增: FreeType 2.13.3 MinGW 交叉编译完成 → thirdparty/freetype-mingw/
- 新增: scripts/build-freetype-mingw.sh 自动化脚本

### 下一步（阻塞项）
- 安装 wine + xdotool + ImageMagick → Wine 截图验证 5 主题
- 4T-06.FA12 字体校验（需 Wine 截图）
- 4T-08.6 全量回归（需 Wine 截图）

### 环境状态
- ✅ MinGW 交叉编译通过，产出 AnyClaw_LVGL.exe (13MB)
- ✅ Wine 启动成功
- ✅ apt 清华源 (mirrors.tuna.tsinghua.edu.cn)
- ✅ GitHub SSH 可用，push 正常
- ✅ mingw gcc13 posix 离线包 (68MB) 已安装
- ✅ 代码质量: 0 硬编码颜色、0 TODO、verify-docs.sh 通过

## 工作习惯
- 用户希望: 高效自主推进，不要反复确认
- 中文沟通
- 服务器网络: GitHub SSH 可用，apt 已切清华源
