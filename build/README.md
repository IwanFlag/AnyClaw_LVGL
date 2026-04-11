# AnyClaw Build System

本目录用于统一存放构建工具、构建脚本和构建产物。

## 目录约定

- `build/windows/`：Windows 一键构建与打包脚本
- `build/windows/tools/`：Windows 构建运行时工具缓存（例如 SDL2 运行库）
- `build/windows/out/`：Windows 编译中间产物与可执行文件输出
- `build/windows/artifacts/`：Windows 打包产物（zip）
- `build/linux/`：Linux 构建脚本

## Windows 一键构建打包

在仓库根目录执行：

```bat
build\windows\build-package.bat
```

可选参数：

```bat
build\windows\build-package.bat --quiet
build\windows\build-package.bat --verbose
```

该命令会自动完成：

1. 检查并初始化构建环境
2. 编译 Windows Release 版本
3. 自动准备运行时依赖（SDL2.dll）
4. 打包 zip 到 `build/windows/artifacts/`

## 仅编译（不打包）

```bat
build\windows\build.bat
```
