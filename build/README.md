# AnyClaw LVGL — 构建系统文档

> 本文档描述 AnyClaw LVGL 的构建方法、依赖关系和常见问题处理。

---

## 1. 快速开始

### 1.1 推荐方式：CMake + MSBuild

```powershell
cd D:\workspace\AnyClaw\AnyClaw_LVGL_Code\AnyClaw_LVGL

# 首次配置（仅需要运行一次，或清除缓存后）
cmake -B build -G "Visual Studio 18 2026" -A x64

# 编译
cmake --build build --config Release --target AnyClaw_LVGL

# 输出路径
# build\bin\Release\AnyClaw_LVGL.exe
```

### 1.2 方式二：直接 MSBuild 解决方案

Visual Studio 或命令行的 `.slnx` 解决方案文件：

```powershell
msbuild build\AnyClaw_LVGL.slnx /p:Configuration=Release /p:Platform=x64
```

### 1.3 方式三：旧脚本（NMake，已不推荐）

```powershell
powershell -File build.ps1
```

> `build.ps1` 使用 NMake Makefiles 方式，流程较慢，仅作备用。

---

## 2. 前提条件

| 依赖 | 版本 | 说明 |
|------|------|------|
| Visual Studio Build Tools | **2026** (18.x, MSVC 19.51) | 需包含 MSVC v143+ |
| CMake | ≥ 3.20 | 生成器需选择 VS 2026 |
| Windows SDK | 10.0.26100+ | 由 vcvarsall.bat 自动选择 |
| SDL2 (bundled) | — | 已包含在 `thirdparty/sdl2-windows/` |
| FreeType (MSVC) | — | 已包含在 `thirdparty/freetype-msvc/` |

### 环境变量设置

构建需要先初始化 VS 环境，使用 `vcvarsall.bat`：

```powershell
& "C:\VSBuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
```

`build.ps1` 脚本中的固定路径：

```
vcvarsall.bat:     C:\VSBuildTools\VC\Auxiliary\Build\vcvarsall.bat
cmake.exe:         C:\Program Files\CMake\bin\cmake.exe
```

> **注意**：系统安装的是 **Visual Studio 2026**（toolset 18.x），不是 VS 2022。
> 生成器名称为 `Visual Studio 18 2026`。

---

## 3. 构建流程详解

### 3.1 CMake 配置

```powershell
cmake -B build -G "Visual Studio 18 2026" -A x64
```

CMake 会自动检测：

- **SDL2**：优先从 `thirdparty/sdl2-windows/` 读取（MSVC），回退到 `thirdparty/sdl2-mingw/`（MinGW）
- **FreeType**：优先从 `thirdparty/freetype-msvc/` 读取（MSVC），回退到 `thirdparty/freetype-mingw/`（MinGW）
- **LVGL**：从 `thirdparty/lvgl/src/` GLOB 递归收集所有 `.c` 文件，自动排除 NuttX 驱动和字体符号冲突

CMake 输出 `build/AnyClaw_LVGL.slnx`（轻量级解决方案）和 `.vcxproj` 文件。

### 3.2 编译

```powershell
# 编译 Release 版本
cmake --build build --config Release --target AnyClaw_LVGL

# 编译 Debug 版本
cmake --build build --config Debug --target AnyClaw_LVGL
```

MSBuild 并行编译（约 4 分钟首次构建，增量编译约 30 秒）。

### 3.3 输出产物

```
build/bin/Release/AnyClaw_LVGL.exe  ← 主程序
build/bin/SDL2.dll                  ← 后构建自动复制
build/bin/assets/fonts/             ← CJK + UI 字体（后构建自动复制）
```

---

## 4. 目录结构

```
AnyClaw_LVGL/
├── build/                          # CMake 输出目录（勿手动修改）
│   ├── bin/
│   │   ├── Release/AnyClaw_LVGL.exe  # 编译产物
│   │   ├── SDL2.dll
│   │   └── assets/fonts/           # 自动复制
│   ├── AnyClaw_LVGL.slnx           # VS 解决方案
│   ├── AnyClaw_LVGL.vcxproj
│   └── CMakeCache.txt
│
├── src/                            # 源代码
│   ├── main.cpp                    # 入口 + 启动线程
│   ├── ui_main.cpp                 # 主 UI（~13000 行）
│   ├── ui_settings.cpp            # 设置页
│   ├── lv_font_mshy_16.c          # 内嵌雅黑 16px 位图字体
│   └── ...
│
├── thirdparty/
│   ├── lvgl/                       # LVGL 源码库
│   ├── sdl2-windows/               # SDL2 (MSVC 版本)
│   ├── sdl2-mingw/                 # SDL2 (MinGW 交叉编译版本)
│   ├── freetype-msvc/              # FreeType (MSVC 编译)
│   └── freetype-mingw/             # FreeType (MinGW 交叉编译)
│
├── assets/fonts/                    # 主题字体包（OTF/TTF）
│   ├── Inter-Regular.ttf
│   ├── SourceHanSansSC-Regular.otf
│   └── ...
│
├── CMakeLists.txt                  # 主构建配置
├── build.ps1                       # NMake 构建脚本（备用）
└── build/                          # 构建产物目录
```

---

## 5. 第三方依赖说明

### 5.1 SDL2

SDL2 提供 Windows 窗口和输入抽象。bundled 版本在 `thirdparty/sdl2-windows/`：

- 头文件：`include/SDL.h`
- 库文件：`lib/x64/SDL2.lib`
- DLL：`lib/x64/SDL2.dll`

运行时 `SDL2.dll` 必须与 `AnyClaw_LVGL.exe` 同目录或在 PATH 中。

### 5.2 FreeType

FreeType 提供矢量字体渲染（通过 LVGL 的 `LV_USE_FREETYPE`）。MSVC 版本在 `thirdparty/freetype-msvc/`：

- 头文件：`include/freetype2/`
- 库文件：`lib/freetype.lib`
- DLL：`lib/freetype.dll`

> **注意**：MinGW 交叉编译的 `libfreetype.a` 与 MSVC 不兼容，必须使用 `thirdparty/freetype-msvc/` 版本。

### 5.3 LVGL

LVGL 源码在 `thirdparty/lvgl/src/`。CMake GLOB 收集所有 `.c` 文件，自动排除：
- `lv_font_mshy_16.c`（与项目内自定义版本冲突）
- NuttX 平台驱动（Windows 不需要）

### 5.4 构建 FreeType MSVC 版本

如果 `thirdparty/freetype-msvc/lib/freetype.lib` 不存在，需要手动编译：

```powershell
# 方式 A：vcpkg（推荐）
vcpkg install freetype:x64-windows

# 方式 B：CMake + MSVC
cd thirdparty/freetype
cmake -B build-msvc -G "Visual Studio 18 2026" -A x64
cmake --build build-msvc --config Release
# 产物复制到 thirdparty/freetype-msvc/
```

---

## 6. 编译定义

| 宏 | 说明 |
|-----|------|
| `LV_CONF_INCLUDE_SIMPLE` | LVGL 简版配置包含 |
| `LV_LVGL_H_INCLUDE_SIMPLE` | LVGL 简版头文件 |
| `_WIN32_WINNT=0x0A00` | Windows 10+ API |
| `LV_USE_FREETYPE=1` | 仅当 FreeType 可用时定义 |
| `NOMINMAX` | 禁用 Windows min/max 宏污染 |
| `/utf-8` | MSVC 源码 UTF-8 编码 |

---

## 7. 常见错误与处理

### 7.1 LNK1104: exe 被占用

```
LINK : fatal error LNK1104: cannot open file 'build\bin\Release\AnyClaw_LVGL.exe'
```

**原因**：程序正在运行。  
**解决**：
```powershell
taskkill /F /IM AnyClaw_LVGL.exe
```

### 7.2 LNK1104: SDL2.dll / freetype.dll 找不到

**原因**：DLL 未复制到 bin 目录，或不在 PATH 中。  
**解决**：
```powershell
# 手动复制
copy thirdparty\sdl2-windows\lib\x64\SDL2.dll build\bin\Release\
copy thirdparty\freetype-msvc\lib\freetype.dll build\bin\Release\
```

### 7.3 LNK2001 / LNK2019: 未解析的外部符号

**原因**：某个 `.cpp` 中调用了未定义的函数或变量。  
**排查**：
1. 查找符号定义位置：`grep -r "symbol_name" src/`
2. 检查是否有对应的 `extern` 声明
3. 检查 CMakeLists.txt 第 174-208 行确认所有源文件已包含

### 7.4 CMake 配置失败：SDL2 not found

**原因**：SDL2 路径不在预期位置。  
**解决**：确认 `thirdparty/sdl2-windows/include/SDL.h` 存在。

### 7.5 FreeType 不可用时的降级行为

FreeType 不可用时，程序回退到：
1. TinyTTF 内嵌数据（CJK 字体字节）
2. LVGL 内置位图字体 `lv_font_mshy_16`（16px 固定大小）

程序仍可运行，但矢量字体渲染质量下降。

---

## 8. 调试构建

```powershell
cmake -B build -G "Visual Studio 18 2026" -A x64 -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug --target AnyClaw_LVGL
```

VS 中打开 `build/AnyClaw_LVGL.slnx` 可直接 F5 调试。

---

## 9. 打包发布

`build.ps1` 包含打包步骤（ZIP）：

```powershell
powershell -File build.ps1
# 输出：build/windows/artifacts/AnyClaw_LVGL_windows_full-runtime_<timestamp>.zip
```

---

## 10. 相关文档

| 文档 | 说明 |
|------|------|
| `../docs/README.md` | 项目文档索引（PRD + Design） |
| `../AGENTS.md` | AI 开发工作流规范 |
| `thirdparty/lvgl/README.md` | LVGL 官方文档 |
