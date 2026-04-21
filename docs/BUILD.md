# AnyClaw LVGL Build Guide

## 快速开始

```powershell
cd D:\workspace\AnyClaw\AnyClaw_LVGL_Code\AnyClaw_LVGL
pwsh -File build.ps1
```

输出：
- exe：`build\bin\AnyClaw_LVGL.exe`
- zip：`build\windows\artifacts\AnyClaw_LVGL_windows_full-runtime_YYYYMMDD_HHMMSS.zip`

---

## 构建脚本体系

### Windows 脚本（根目录）

| 脚本 | 用途 | 备注 |
|------|------|------|
| `build.ps1` | **主要构建脚本**（5步：clean → cmake → nmake → 找exe → 打包zip） | 推荐使用 |
| `do_build.ps1` | cmake --build 方式 | 检测 "error C\|fatal error" |
| `do_build2.ps1` | cmake --build 简化版 | vcvarsall 手动注入 |
| `build_msvc.ps1` | nmake /F Makefile install | 最简路径 |
| `quick_compile2.ps1` | 仅编译 ui_main.cpp | 快速验证单文件改动 |
| `rebuild.ps1` | 清理 build 目录后重新构建 | 等同 build.ps1  clean |
| `build_watch.ps1` | watch 模式增量编译 | 未测试 |
| `check_depth.ps1` | 检查递归链接 | 工具脚本 |

---

## 工具链

| 组件 | 路径 |
|------|------|
| MSVC cl.exe | `C:\VSBuildTools\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64\cl.exe` |
| CMake | `C:\Program Files\CMake\bin\cmake.exe` |
| Windows SDK | `C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\ucrt\` |
| vcvarsall.bat | `C:\VSBuildTools\VC\Auxiliary\Build\vcvarsall.bat` |

### CMake 配置（build\CMakeCache.txt）

```
CMAKE_GENERATOR:INTERNAL=NMake Makefiles
CMAKE_BUILD_TYPE:STRING=Release
```

**生成器不可更改**，必须与 CMakeCache.txt 保持一致。

---

## 编译参数

- C++ 标准：`/std:c++17`
- 字符集：`/utf-8`（解决 MSVC 中文注释问题）
- min/max 宏：`/DNOMINMAX`（解决 `<windows.h>` 冲突）
- SDL2：bundled 模式，`thirdparty/sdl2-windows`

---

## 目录结构

```
AnyClaw_LVGL/
├── build/                    # CMake 构建目录
│   ├── bin/                 # exe 输出目录
│   │   └── AnyClaw_LVGL.exe
│   ├── CMakeCache.txt       # CMake 配置缓存
│   ├── nmake_build.log      # NMake 输出日志
│   └── windows/
│       ├── artifacts/       # zip 包输出
│       └── out/             # 打包临时目录
├── src/
│   ├── ui_main.cpp          # 主 UI（~12800行）
│   └── ...                  # 其他源文件
├── docs/
│   ├── BUILD.md             # 本文档
│   ├── PRD.md               # 产品需求
│   └── Design.md            # 设计文档
└── scripts/                 # 辅助脚本（跨平台）
```

---

## 增量编译 vs 完整重建

### 增量编译（不改 CMake 配置）

```powershell
pwsh -File build.ps1
```
cmake 自动检测哪些 .cpp/.h 变了，只重编译改过的文件。

### 完整重建（清理缓存）

```powershell
Remove-Item build\CMakeCache.txt -Force
pwsh -File build.ps1
```
或者手动删除 build 目录：
```powershell
Remove-Item build -Recurse -Force -ErrorAction SilentlyContinue
pwsh -File build.ps1
```

---

## 运行产物

```powershell
# 先停止旧进程（否则 exe 被锁）
Stop-Process -Name AnyClaw_LVGL -Force -ErrorAction SilentlyContinue

# 直接运行
& build\bin\AnyClaw_LVGL.exe
```

---

## 常见问题

### 编译失败：文件被占用
```
Stop-Process -Name AnyClaw_LVGL -Force -ErrorAction SilentlyContinue
pwsh -File build.ps1
```

### 编译失败：U1077 cmake 错误
通常是 CMake 缓存损坏，删除缓存重试：
```
Remove-Item build\CMakeCache.txt -Force
pwsh -File build.ps1
```

### nmake warning 被误判为 error
MSVC 输出中文"注意: 包含文件"含"错"字，`build.ps1` 只检测 "CMake Error"（英文）和 "fatal error"，不会误判。

### 判断 build 是否成功
1. `Test-Path build\bin\AnyClaw_LVGL.exe` 为 True
2. `build.ps1` 输出无 "CMake Error"
3. `nmake_build.log` 无 "fatal error C"

---

## 历史修复记录

### 链接错误
- `ftp_transfer_file` LNK2005：`ftp_client.cpp` 和 `stub_ftp.cpp` 同时编译
  - 修复：`#ifdef ENABLE_FTP` 保护，`CMakeLists.txt` 只在 `if(NOT ENABLE_FTP)` 下编译 stub
- `RemoteProtocolManager` LNK2019：`stub_remote.cpp` 缺实现
  - 修复：`stub_remote.cpp` 加所有方法的空实现，`CMakeLists.txt` 只在 `if(NOT ENABLE_REMOTE)` 下编译 stub

### 编译错误（ui_main.cpp）
- `led_sz` undeclared — 在 "Model name (short)" 块之前加 `int led_sz = mode_w;`
- `btn_work_chat_send` 拼写 — 改为 `mode_btn_work_chat_send`
- `mode_ta_lan_users` 等 undeclared — 添加 LAN chat / cron 相关静态变量声明

### MSVC 中文注释 bug
MSVC `/utf-8` 开关对 include 头文件不生效，中文字符组合触发 C2062。
- 修复：替换所有中文注释为英文

### json_util.h 默认参数冲突
declaration 中不能有默认参数，与 .cpp 定义冲突导致 C4005 redefinition。
- 修复：删除 declaration 中的默认值，只在 .cpp 定义处保留

---

## 高 DPI 缩放

`SCALE()` 宏在 200% DPI 时将布局尺寸翻倍，导致 LVGL 软件渲染器处理 4× 像素区域时主线程阻塞。

缓解：SCALE 宏上限 100%，布局保持原生尺寸，`g_dpi_scale=200` 仍传给 LVGL 内部处理字体缩放。

如仍卡死，修改 `app.h`：`return px` 完全禁用布局缩放。

---

## 设计决策记录

| 日期 | 决策 | 原因 |
|------|------|------|
| 2026-04-19 | 分隔条删除隐藏，保留全局变量 | 避免 relayout_panels() 大面积改动 |
| 2026-04-19 | 使用 NMake Makefiles generator | 与 CMakeCache.txt 一致，不可改 |
| 2026-04-19 | build.ps1 exit code 判定：检查 exe 存在 | nmake 返回 1 有 warning，不等于失败 |
