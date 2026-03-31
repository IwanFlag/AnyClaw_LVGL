# AnyClaw LVGL Bug Tracker

## Bug 列表

| 编号 | 描述 | 发现时间 | 严重程度 | 修复状态 | 修复措施 | 修复时间 |
|------|------|----------|----------|----------|----------|----------|
| BUG-001 | 中文字符渲染为方框（□）—— Source Han Sans 字体缺少大量UI所需汉字（器、态、运、板、操、刷、径、标、新、端 等111个字符） | 2026-04-01 03:23 | P0 严重 | ✅ 已修复 | 重新生成 `lv_font_source_han_sans_sc_16_cjk.c`，使用 `npx lv_font_conv` 重新生成字体，`--symbols` 参数包含全部111个UI所需中文字符 | 2026-04-01 03:30 |
| BUG-002 | 右侧面板在窗口中显示不全 | 2026-04-01 03:23 | P0 严重 | ✅ 已修复 | 将窗口尺寸从 1000x650 扩大为 1200x800，重新布局左右面板（左520px + 右自适应650px） | 2026-04-01 03:35 |
| BUG-003 | 窗口尺寸不符合PRD要求（PRD第2.13节要求1200x800） | 2026-04-01 03:23 | P0 严重 | ✅ 已修复 | 修改 `main.cpp` 中 `lv_sdl_window_create(1200, 800)` 和 `ui_settings.cpp` 中 `WIN_W/WIN_H` 常量 | 2026-04-01 03:35 |
| BUG-004 | 语言默认仅中文，PRD要求中英双语 | 2026-04-01 03:23 | P0 严重 | ✅ 已修复 | 重写 `tr()` 函数，新增 `Lang::BOTH` 模式，所有 I18n 字符改为 `I18n` 结构体（en+cn），默认 `g_lang = Lang::BOTH` | 2026-04-01 03:40 |
| BUG-005 | 字体文件 include 路径错误——lv_font_source_han_sans_sc_16_cjk.c 使用 `#include "lvgl/lvgl.h"` 但编译路径不匹配 | 2026-04-01 03:30 | P0 严重 | ✅ 已修复 | 修改为 `#include "../../lvgl.h"` 以匹配字体文件在项目中的相对位置 | 2026-04-01 03:31 |
| BUG-006 | LVGL静态库未包含snapshot/canvas/lodepng——lv_conf.h 修改后未触发lvgl_static重编译 | 2026-04-01 03:45 | P1 中等 | ✅ 已修复 | 清空CMake缓存（删除 CMakeCache.txt、CMakeFiles、bin、lib 目录），强制完整重编译 | 2026-04-01 03:48 |
| BUG-007 | C++嵌套函数不合法——在 `app_ui_init()` 内定义 `screenshot_cb` 函数 | 2026-04-01 03:42 | P1 中等 | ✅ 已修复 | 将 `screenshot_cb` 移出为文件级 static 函数，后因截图功能无法验证而整体移除 | 2026-04-01 03:50 |
| BUG-008 | SDL2窗口使用Direct3D硬件渲染，无法通过GDI接口（PrintWindow/BitBlt）截取窗口截图 | 2026-04-01 03:45 | P2 低 | ⬜ 暂未修复 | 尝试方案：1) SDL_SetHint(SDL_HINT_RENDER_DRIVER,"software") 无效；2) PrintWindow+PW_RENDERFULLCONTENT 无效；3) Screen BitBlt 无效。需使用 Windows Graphics Capture API | - |
| BUG-009 | ui_settings.cpp 窗口尺寸硬编码为1000x650，与新窗口1200x800不匹配 | 2026-04-01 03:38 | P0 严重 | ✅ 已修复 | 重写 ui_settings.cpp，将所有布局常量改为1200x800，调整面板、标签页大小 | 2026-04-01 03:40 |

## 统计

- **总 Bug 数:** 9
- **已修复:** 8
- **未修复:** 1（截图验证功能，不影响核心功能）
- **P0 修复率:** 100%（6/6）
- **P1 修复率:** 100%（2/2）
