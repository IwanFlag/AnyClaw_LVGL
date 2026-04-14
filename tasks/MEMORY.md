# MEMORY.md — 长期记忆

## 项目

### AnyClaw_LVGL
- 仓库: `git@github.com:IwanFlag/AnyClaw_LVGL.git`
- 语言: C/C++17, LVGL 9.6 + SDL2, Windows 目标平台
- AI 开发规范: `AnyClaw_LVGL/AGENTS.md`（每次会话必读）
- 任务追踪: `AnyClaw_LVGL/tasks/v2.2.1-dev.md`
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
- 新增: 阿里云 apt 镜像不可达，已切清华源（mirrors.tuna.tsinghua.edu.cn）
- 新增: mingw-w64 已安装（GCC 13-win32），交叉编译环境解锁
- 新增: MinGW 交叉编译成功，产出 AnyClaw_LVGL.exe (13MB)
- 新增: zip 已安装，打包流程完整

### 下一步
- 查看 CI 结果（5T-04.4 确认通过后标记 ✅）
- 安装 wine + xdotool + ImageMagick → Wine 截图验证 5 主题
- 4T-06.FA12 字体校验（需 Wine 截图）
- 4T-08.6 全量回归（需 Wine 截图）
- 代码已全部干净：0 硬编码、0 TODO、文档验证通过、交叉编译通过

## 工作习惯
- 用户希望: 高效自主推进，不要反复确认
- 中文沟通
- 服务器网络: GitHub SSH 可用，apt 已切清华源
