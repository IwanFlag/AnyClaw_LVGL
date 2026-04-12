# AnyClaw Build System

本目录用于统一存放构建工具、构建脚本和构建产物。

> **⚠️ 编译规则：每次编译必须使用编译+打包脚本，不要执行单独编译的脚本。**
>
> | 平台 | 命令 |
> |------|------|
> | Windows 原生 | `build\windows\build-package.bat` |
> | Linux 交叉编译 | `bash build/linux/build.sh [version]` |

## 目录约定

- `build/linux/`：Linux 构建脚本与产物
- `build/linux/out/`：Linux 交叉编译中间产物（不提交）
- `build/linux/artifacts/`：Linux 交叉编译打包产物 zip（提交）
- `build/windows/`：Windows 一键构建与打包脚本
- `build/windows/out/`：Windows 编译中间产物（不提交）
- `build/windows/artifacts/`：Windows 打包产物 zip（提交）
- `build/windows/tools/`：Windows 构建运行时工具缓存（例如 SDL2 运行库）

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

## Linux 交叉编译（Linux → Windows x64）

### 前置依赖

```bash
# Ubuntu/Debian
sudo apt-get install mingw-w64 libsdl2-dev zip cmake

# SDL2 MinGW 库（thirdparty/sdl2-mingw/lib/）已随仓库提交：
# - libSDL2.dll.a  — 从 SDL2.def 生成的 MinGW 导入库
# - libSDL2main.a  — WinMain→main 桥接 stub
# 若需重新生成，见下方"重新生成 SDL2 MinGW 库"。
```

### 一键交叉编译 + 打包

```bash
cd /path/to/AnyClaw_LVGL
bash build/linux/build.sh [version]
# 示例: bash build/linux/build.sh v2.2.1
```

该命令会自动完成：

1. 清理旧构建产物
2. CMake 配置（MinGW 交叉编译）
3. 编译 `AnyClaw_LVGL.exe`
4. 复制 assets 到构建目录
5. 打包 zip 到 `out/mingw/bin/`

产物：`out/mingw/bin/AnyClaw_LVGL_*.zip`

> **注意：** 运行时还需要 `SDL2.dll`。从 [SDL2 Releases](https://github.com/libsdl-org/SDL/releases)
> 下载 `SDL2-*-win32-x64.zip`，将 `SDL2.dll` 放到 `thirdparty/sdl2-mingw/bin/`。
> 有此文件后编译会自动复制到输出目录。

### 仅交叉编译（不打包）

```bash
cd out/mingw
cmake ../.. -DCMAKE_TOOLCHAIN_FILE=../../build/toolchain-mingw64.cmake -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### 重新生成 SDL2 MinGW 库

若 `thirdparty/sdl2-mingw/lib/` 中的库文件丢失或需更新：

```bash
cd AnyClaw_LVGL

# 1. 从 sdl2-windows 的 .def 生成导入库
x86_64-w64-mingw32-dlltool \
  -d thirdparty/sdl2-windows/lib/x64/SDL2.def \
  -l thirdparty/sdl2-mingw/lib/libSDL2.dll.a \
  -D SDL2.dll

# 2. 生成 WinMain 桥接 stub
cat > /tmp/sdl2main_stub.c << 'CEOF'
#include <SDL.h>
#ifdef __MINGW32__
#include <windows.h>
int main(int argc, char *argv[]);
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow) {
    return SDL_main(__argc, __argv);
}
#endif
CEOF
x86_64-w64-mingw32-gcc -c /tmp/sdl2main_stub.c -o /tmp/sdl2main_stub.o \
  -Ithirdparty/sdl2-mingw/include -Ithirdparty/sdl2-mingw/include/SDL2 \
  -D_REENTRANT -Dmain=SDL_main
x86_64-w64-mingw32-ar rcs thirdparty/sdl2-mingw/lib/libSDL2main.a /tmp/sdl2main_stub.o
```
