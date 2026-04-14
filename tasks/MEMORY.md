# MEMORY.md — 长期记忆

## 项目

### AnyClaw_LVGL
- 仓库: `git@github.com:IwanFlag/AnyClaw_LVGL.git`
- 语言: C/C++17, LVGL 9.6 + SDL2, Windows 目标平台
- AI 开发规范: `AnyClaw_LVGL/AGENTS.md`（每次会话必读）
- 任务追踪: `AnyClaw_LVGL/tasks/v2.2.1-dev.md`
- Git 配置: user.email=openclaw@anyclaw.dev, user.name=OpenClaw AI

### 当前进度（截至 2026-04-14 17:45）
- 阶段一~三: ✅ 全部完成
- 阶段四 4T-00~05: ✅ 5 套主题就绪（Matcha/Peachy/Classic/Mochi/Light）
- 4T-06: ~95% 完成，剩余 FA12 字体校验需 Wine
- 4T-07: 目录结构就位，资源文件 ⏸️ 需设计工具
- 4T-08: 5/6 完成，08.6 全量回归需 Wine
- 阶段五 5T-01: ✅ 全部完成
- 5T-02: ✅ 全部完成（02.1~06，含 FreeType fallback 链）
- 5T-03: ✅ 全部完成（03.1~05）
- 5T-04: 04.1~03 ✅, 04.4 🔧 CI 已触发待确认结果
- 新增: auto_restart_on_crash config 持久化（TODO 已修复）
- 新增: widgets 目录 12 处硬编码颜色已 Token 化
- 新增: CMakeLists.txt FreeType 条件链接语法错误已修复
- 新增: CMakeLists.txt FreeType include 路径修复（freetype2 子目录）
- 新增: SSH key 已生成，GitHub SSH 已通，push 可用
- 新增: FreeType 2.13.3 MinGW 交叉编译完成 → thirdparty/freetype-mingw/
- 新增: scripts/build-freetype-mingw.sh 自动化脚本
- 代码质量: 0 硬编码颜色（除 helper）、0 TODO
- verify-docs.sh: 通过

### 下一步
- 查看 CI 结果（5T-04.4 确认通过后标记 ✅）
- 环境已解锁后：apt install mingw-w64 wine → 编译验证 + Wine 截图
- 新增: Nunito/Inter/JetBrainsMono 字体已下载（fontsource woff2→TTF 转换），FA5/FA7/FA8 阻塞解除
- 阻塞项: apt 镜像不可达（当前服务器阿里云源 timeout）、Wine 未装（截图验证）
- 代码已全部干净：0 硬编码、0 TODO、文档验证通过

## 工作习惯
- 用户希望: 高效自主推进，不要反复确认
- 中文沟通
- 服务器网络: GitHub SSH 可用，apt 已切清华源
