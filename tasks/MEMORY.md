# MEMORY.md — 长期记忆

## 项目

### AnyClaw_LVGL
- 仓库: `git@github.com:IwanFlag/AnyClaw_LVGL.git`
- 语言: C/C++17, LVGL 9.6 + SDL2, Windows 目标平台
- AI 开发规范: `AnyClaw_LVGL/AGENTS.md`（每次会话必读）
- 任务追踪: `AnyClaw_LVGL/tasks/v2.2.1-dev.md`
- Git 配置: user.email=openclaw@anyclaw.dev, user.name=OpenClaw AI

### 当前进度（截至 2026-04-14 15:05）
- 阶段一~三: ✅ 全部完成
- 阶段四 4T-00~05: ✅ 5 套主题就绪（Matcha/Peachy/Classic/Mochi/Light）
- 4T-06: ~95% 完成，剩余 FA12 字体校验需 Wine
- 4T-07: 目录结构就位，资源文件 ⏸️ 需设计工具
- 4T-08: 5/6 完成
- 阶段五 5T-01: ✅ 全部完成
- 5T-02: 5T-02.1~02.3 ✅, 02.4 半完成（freetype_body/title 指针已加）, 02.5~07 ⏳
- 5T-03: 5T-03.1 ✅, 03.2~05 ⏳
- 5T-04: 5T-04.1~03 ✅, 04.4 ⏳
- 新增: widgets 目录 12 处硬编码颜色已 Token 化
- 新增: CMakeLists.txt FreeType 条件链接语法错误已修复
- 新增: CMakeLists.txt FreeType include 路径修复（freetype2 子目录）
- 新增: SSH key 已生成，GitHub SSH 已通，push 可用
- 新增: apt 源已切清华，mingw-w64/cmake/zip 已装
- 新增: FreeType 2.13.3 MinGW 交叉编译完成 → thirdparty/freetype-mingw/
- 新增: scripts/build-freetype-mingw.sh 自动化脚本
- 编译进展: FreeType 编译通过，garlic_dock.cpp 有语法错误待修

### 下一步
- 修复 garlic_dock.cpp 语法错误 → 编译验证
- 5T-02.5: init_theme_fonts() 接入 lv_freetype_font_create()
- 5T-02.6: FreeType fallback 逻辑
- 5T-03.2~04: GPU 渲染 CMake + 代码
- 5T-04.4: CI 推送验证
- 环境已解锁: mingw-w64 可用，可做编译验证
- 阻塞项: Wine 未装（截图验证）、字体文件（网络下载）

## 工作习惯
- 用户希望: 高效自主推进，不要反复确认
- 中文沟通
- 服务器网络: GitHub SSH 可用，apt 已切清华源
