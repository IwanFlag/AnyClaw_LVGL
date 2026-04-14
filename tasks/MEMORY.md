# MEMORY.md — 长期记忆

## 项目

### AnyClaw_LVGL
- 仓库: `git@github.com:IwanFlag/AnyClaw_LVGL.git`
- 语言: C/C++17, LVGL 9.6 + SDL2, Windows 目标平台
- AI 开发规范: `AnyClaw_LVGL/AGENTS.md`（每次会话必读）
- 任务追踪: `AnyClaw_LVGL/tasks/v2.2.1-dev.md`
- Git 配置: user.email=openclaw@anyclaw.dev, user.name=OpenClaw AI

### 当前进度（截至 2026-04-14 14:36）
- 阶段一~三: ✅ 全部完成
- 阶段四 4T-00~05: ✅ 5 套主题就绪（Matcha/Peachy/Classic/Mochi/Light）
- 4T-06: ~95% 完成，剩余 FA12 字体校验需 Wine
- 4T-07: 目录结构就位，资源文件 ⏸️ 需设计工具
- 4T-08: 5/6 完成
- 阶段五 5T-01: ✅ 全部完成
- 5T-02: 5T-02.1~02.2 ✅, 5T-02.4 半完成（freetype_body/title 指针已加）, 02.5~07 ⏳
- 5T-03: 5T-03.1 ✅, 03.2~05 ⏳
- 5T-04: 5T-04.1~03 ✅, 04.4 ⏳
- 新增: widgets 目录 12 处硬编码颜色已 Token 化
- 新增: CMakeLists.txt FreeType 条件链接语法错误已修复
- 新增: SSH key 已生成，push 已通

### 下一步
- 从 `tasks/v2.2.1-dev.md` 找到下一个 ⏳ 继续
- 阶段四（主题系统）: 代码任务全部 ✅，剩 Wine 截图验证 + ⏸️ 资源文件
- 阶段五（工程改进）: 5T-01 全部 ✅, 5T-02/03/04 部分完成，从 5T-02.3 开始继续
- 阻塞项: 字体文件(.ttf/.otf)下载、LVGL gradient API、MinGW 编译验证
- 环境限制: 服务器无 mingw-w64 / Wine / apt 源不可用 / 无 GitHub 凭证→无法 push

## 工作习惯
- 用户希望: 高效自主推进，不要反复确认
- 中文沟通
- 服务器网络受限（GitHub HTTPS 不通，SSH 可用；外部下载慢）
