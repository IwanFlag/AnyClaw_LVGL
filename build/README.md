# AnyClaw Build System

## 编译规则

**每次编译必须使用编译+打包脚本，不要执行单独编译。**

| 平台 | 命令 |
|------|------|
| Linux 交叉编译 → Windows | `bash build/linux/build.sh [version]` |
| Windows 原生 | `build\windows\build-package.bat` |

## 项目目录结构

```
AnyClaw_LVGL/
├── src/                          # 应用源码 (C/C++)
├── ui/                           # UI 定义
├── assets/                       # 运行时资源 (PNG 图标等)
├── bundled/                      # 离线安装包
├── tools/                        # 辅助脚本 (keygen 等)
├── thirdparty/                   # 第三方依赖（全部提交到仓库）
│   ├── lvgl/                     #   LVGL 源码
│   ├── sdl2-mingw/               #   SDL2 MinGW 交叉编译依赖
│   │   ├── include/SDL2/         #     头文件
│   │   ├── lib/                  #     静态库 (.a)
│   │   └── bin/                  #     运行时 DLL (SDL2.dll x64/x86)
│   └── sdl2-windows/             #   SDL2 MSVC 依赖
│       └── lib/x64/              #     .lib 文件
├── docs/                         # 产品文档 (PRD/Design Spec/Tasks)
├── build/                        # 构建脚本与产物
│   ├── linux/                    #   Linux 构建
│   │   ├── build.sh              #     编译+打包（主力脚本）
│   │   ├── build-win.sh          #     编译+打包（精简版）
│   │   ├── build-linux.sh        #     仅原生 Linux 编译
│   │   ├── run-wine.sh           #     Wine 测试运行
│   │   ├── out/                  #     中间产物（不提交）
│   │   └── artifacts/            #     打包 zip（提交）
│   ├── windows/                  #   Windows 构建
│   │   ├── build-package.bat     #     编译+打包
│   │   ├── build.bat             #     仅编译
│   │   ├── out/                  #     中间产物（不提交）
│   │   └── artifacts/            #     打包 zip（提交）
│   ├── toolchain-mingw64.cmake   #   MinGW 交叉编译工具链
│   ├── package_windows_full_runtime.sh
│   └── README.md                 #   本文件
├── CMakeLists.txt                # 构建配置
└── .gitignore
```

## 环境依赖

### 系统包（apt，不可提交到仓库）

```bash
apt-get install mingw-w64 libsdl2-dev zip cmake
```

### 仓库内依赖（已提交，无需下载）

| 依赖 | 位置 | 说明 |
|------|------|------|
| LVGL 9.6 | `thirdparty/lvgl/` | 源码 |
| SDL2 MinGW 头文件 | `thirdparty/sdl2-mingw/include/SDL2/` | 66 个头文件 |
| SDL2 MinGW 静态库 | `thirdparty/sdl2-mingw/lib/` | libSDL2.dll.a + libSDL2main.a |
| SDL2 Windows DLL | `thirdparty/sdl2-mingw/bin/` | SDL2.dll (x64) + SDL2-x86.dll |
| SDL2 MSVC 库 | `thirdparty/sdl2-windows/lib/` | .lib + .def |

## 产物

编译打包后产物在 `build/linux/artifacts/`：

```
AnyClaw_LVGL_v2.x.x_YYYYMMDD_HHMM.zip
├── AnyClaw_LVGL.exe        # 主程序
├── SDL2.dll                # 运行时依赖
└── assets/                 # 图标资源
    ├── app_icon.png
    ├── garlic_*.png
    ├── tray/*.png
    └── icons/ai/*.png
```

## SDL2 DLL 版本

| 文件 | 架构 | 大小 |
|------|------|------|
| `SDL2.dll` | win32-x64 | ~2.5MB |
| `SDL2-x86.dll` | win32-x86 | ~2.3MB |

> win32-arm64 无官方预编译版，需自行从源码编译。

## 重新生成 SDL2 MinGW 库

若 `thirdparty/sdl2-mingw/lib/` 中的库文件丢失：

```bash
cd AnyClaw_LVGL

# 1. 导入库（从 .def 生成）
x86_64-w64-mingw32-dlltool \
  -d thirdparty/sdl2-windows/lib/x64/SDL2.def \
  -l thirdparty/sdl2-mingw/lib/libSDL2.dll.a \
  -D SDL2.dll

# 2. WinMain 桥接 stub
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
