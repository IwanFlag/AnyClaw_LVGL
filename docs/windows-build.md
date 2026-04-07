# AnyClaw LVGL — Windows 构建文档

> 参考：`tools/linux/build.sh` 与 `docs/AnyClaw_LVGL_v2.0_build_guide.md`  
> 目标：在 Windows 本机完成编译、产物检查与打包。

---

## 1. 适用环境

- OS: Windows 10/11 x64
- CMake: 3.20+
- 编译器（二选一）：
  - Visual Studio 2022（推荐，脚本会优先使用）
  - MSYS2/MinGW-w64
- 项目目录：`AnyClaw_LVGL`

说明：

- `tools/windows/build.bat` 已支持自动探测 CMake（PATH 或 VS 内置 CMake）。
- 若检测到 `cl.exe`，脚本会自动用 `Visual Studio 17 2022` 生成器。
- 若未检测到 `cl.exe`，回退 `MinGW Makefiles`。

---

## 2. 一键编译（推荐）

在项目根目录执行：

```bat
tools\windows\build.bat
```

成功后（VS 生成器）默认产物路径：

- EXE: `build-windows\bin\Release\AnyClaw_LVGL.exe`
- DLL: `build-windows\bin\Release\SDL2.dll`（若存在）

建议在 **x64 Native Tools Command Prompt for VS 2022** 中执行，避免编译器环境变量缺失。

---

## 3. 手工编译（可选）

### 3.1 Visual Studio 2022

```bat
cmake -S . -B build-windows -G "Visual Studio 17 2022" -A x64
cmake --build build-windows --config Release -j 8
```

### 3.2 MinGW

```bat
cmake -S . -B build-windows -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build-windows -j 8
```

---

## 4. 打包发布（Windows）

建议包结构与 Linux 脚本一致：

```text
AnyClaw_LVGL_*.zip
├── AnyClaw_LVGL.exe
├── SDL2.dll                 (若构建环境提供)
├── assets/
└── bundled/
```

推荐 PowerShell 打包命令（在项目根目录执行）：

```powershell
$project = (Get-Location).Path
$bin = Join-Path $project "build-windows\bin\Release"
$ts = Get-Date -Format "yyyyMMdd_HHmm"
$zip = Join-Path $bin "AnyClaw_LVGL_v2.0.1_${ts}_windows.zip"
$pkg = Join-Path $bin "_pkg"

if (Test-Path $pkg) { Remove-Item $pkg -Recurse -Force }
New-Item -ItemType Directory -Path $pkg | Out-Null
New-Item -ItemType Directory -Path (Join-Path $pkg "assets\tray") -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $pkg "assets\icons\ai") -Force | Out-Null

Copy-Item (Join-Path $bin "AnyClaw_LVGL.exe") (Join-Path $pkg "AnyClaw_LVGL.exe") -Force
if (Test-Path (Join-Path $bin "SDL2.dll")) {
  Copy-Item (Join-Path $bin "SDL2.dll") (Join-Path $pkg "SDL2.dll") -Force
}

Copy-Item (Join-Path $project "assets\*.png") (Join-Path $pkg "assets") -ErrorAction SilentlyContinue
Copy-Item (Join-Path $project "assets\tray\*.png") (Join-Path $pkg "assets\tray") -ErrorAction SilentlyContinue
Copy-Item (Join-Path $project "assets\icons\ai\*.png") (Join-Path $pkg "assets\icons\ai") -ErrorAction SilentlyContinue
if (Test-Path (Join-Path $project "bundled")) {
  Copy-Item (Join-Path $project "bundled") (Join-Path $pkg "bundled") -Recurse -Force
}

if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path (Join-Path $pkg "*") -DestinationPath $zip -CompressionLevel Optimal
Write-Host "ZIP: $zip"
```

打包后快速校验：

```powershell
Get-Item .\build-windows\bin\Release\AnyClaw_LVGL*.zip | Select-Object FullName,Length,LastWriteTime
```

---

## 5. 常见问题

### 5.1 `cmake` 命令不存在

- 原因：未安装 CMake 或未加 PATH。
- 处理：安装 CMake，或安装 VS2022 并启用 CMake 工具。

### 5.2 `SDL2.dll` 缺失

- 现象：编译成功但运行时报缺少 `SDL2.dll`。
- 原因：本机仅有 SDL2 头文件/导入库，未带运行时 DLL。
- 处理：将 `SDL2.dll` 放到 `AnyClaw_LVGL.exe` 同目录再运行。

可直接下载官方运行时（x64）：

```powershell
$tmp = Join-Path $env:TEMP "sdl2_runtime_2303"
New-Item -ItemType Directory -Path $tmp -Force | Out-Null
Invoke-WebRequest "https://github.com/libsdl-org/SDL/releases/download/release-2.30.3/SDL2-2.30.3-win32-x64.zip" -OutFile (Join-Path $tmp "SDL2.zip")
Expand-Archive (Join-Path $tmp "SDL2.zip") -DestinationPath $tmp -Force
Copy-Item (Join-Path $tmp "SDL2.dll") ".\build-windows\bin\Release\SDL2.dll" -Force
```

### 5.3 MinGW 构建失败

- 原因：未安装 `mingw32-make` 或工具链不完整。
- 处理：优先改用 VS 生成器；若坚持 MinGW，请补齐 MSYS2 组件。

### 5.4 缺少 VC 运行库

- 现象：提示找不到 `MSVCP140.dll` 或 `VCRUNTIME140.dll`。
- 处理：安装 [Microsoft Visual C++ Redistributable 2015-2022 (x64)](https://aka.ms/vs/17/release/vc_redist.x64.exe)。

---

## 6. 运行检查（建议）

### 6.1 启动冒烟测试

```powershell
$exe = ".\build-windows\bin\Release\AnyClaw_LVGL.exe"
Start-Process -FilePath $exe -WorkingDirectory (Split-Path $exe)
```

### 6.2 依赖检查

```powershell
dumpbin /DEPENDENTS .\build-windows\bin\Release\AnyClaw_LVGL.exe
```

至少应看到：`SDL2.dll`、`KERNEL32.dll`、`USER32.dll` 等核心依赖。

---

## 7. 当前状态（本次验证）

- Windows 本机编译：已通过（Release）
- 已验证产物：
  - `build-windows\bin\Release\AnyClaw_LVGL.exe`
- 已完成打包（含 `assets/` 与 `bundled/`）：
  - `build-windows\bin\Release\AnyClaw_LVGL_v2.0.1_20260407_1742_windows.zip`
