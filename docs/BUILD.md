# AnyClaw LVGL 编译与打包

## 一键编译打包（推荐）

在项目根目录执行：

```bat
build\build_oneclick.bat
```

脚本会自动完成：

1. 调用 build.bat 完成编译
2. 组装便携运行目录
3. 生成 zip 包

输出位置：

- 可执行文件：build/bin/AnyClaw_LVGL.exe
- 打包目录：build/package/AnyClaw_LVGL/
- 打包 zip：build/package/AnyClaw_LVGL_portable.zip

## 仅编译（不打包）

```bat
build.bat
```

## 运行应用

```bat
build\bin\AnyClaw_LVGL.exe
```

## 常见问题

### 1) 编译失败，提示工具链不可用

- 确认已安装 VS Build Tools 与 CMake。
- build.bat 内部会调用：
  - C:\VSBuildTools\VC\Auxiliary\Build\vcvarsall.bat
  - cmake -S .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release
  - ninja

### 2) 运行文件被占用

先结束旧进程再编译：

```powershell
Stop-Process -Name AnyClaw_LVGL -Force -ErrorAction SilentlyContinue
```

### 3) 想做干净重建

```powershell
Remove-Item .\build -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path .\build | Out-Null
build\build_oneclick.bat
```
