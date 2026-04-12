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
├── assets/                       # 运行时资源 (PNG 图标 + 字体文件等)
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
└── assets/                 # 图标 + 字体 + 音效资源
    ├── app_icon.png        # exe / 任务栏图标（16/32/48/64/256px）
    ├── app_icon.ico        # exe 图标（多尺寸 ICO）
    │
    ├── mascot/             # 品牌吉祥物（大蒜 + 龙虾）
    │   ├── matcha/         # Matcha 主题大蒜
    │   │   ├── garlic_body_64.png      # 蒜瓣主体
    │   │   ├── garlic_sprout_72.png    # 叶茎（带动画旋转锚点）
    │   │   ├── garlic_24.png           # 紧凑头像
    │   │   ├── garlic_32.png           # 标准头像
    │   │   ├── garlic_48.png           # 大头像
    │   │   └── garlic_96.png           # 品牌展示
    │   ├── peachy/         # Peachy 主题大蒜
    │   │   └── (同上结构)
    │   ├── mochi/          # Mochi 主题大蒜
    │   │   └── (同上结构)
    │   └── lobster/        # 龙虾 AI 头像（不随主题变）
    │       ├── lobster_24.png
    │       ├── lobster_32.png
    │       └── lobster_48.png
    │
    ├── tray/               # 托盘图标（PNG，按主题×状态）
    │   ├── matcha/
    │   │   ├── idle_16.png / idle_20.png / idle_32.png     # 白 #E8ECF4
    │   │   ├── active_16.png / active_20.png / active_32.png # 绿 #3DD68C
    │   │   ├── error_16.png / error_20.png / error_32.png   # 红 #FF6B6B
    │   │   └── busy_16.png / busy_20.png / busy_32.png      # 黄 #FFBE3D
    │   ├── peachy/         # 暖色系状态色
    │   │   └── (同上结构，idle=#8B7355, active=#FF7F50, error=#FF5C5C, busy=#FFB347)
    │   └── mochi/          # 棕色系状态色
    │       └── (同上结构，idle=#B0A394, active=#A67B5B, error=#C47070, busy=#C9A96E)
    │
    ├── icons/              # SVG 图标集
    │   └── lucide/         # Lucide Icons 子集（~120 个 SVG）
    │
    ├── fonts/              # 主题字体（.ttf/.otf）
    │   ├── PlusJakartaSans-Regular.ttf
    │   ├── PlusJakartaSans-SemiBold.ttf
    │   ├── PlusJakartaSans-Bold.ttf
    │   ├── Nunito-Regular.ttf
    │   ├── Nunito-Bold.ttf
    │   ├── Lora-Regular.ttf
    │   ├── Lora-Bold.ttf
    │   ├── HarmonyOS_Sans_SC_Regular.otf
    │   ├── HarmonyOS_Sans_SC_Bold.otf
    │   └── JetBrainsMono-Regular.ttf
    │
    └── sounds/             # 主题音效（.wav）
        ├── matcha/         # 清脆薄荷风
        │   ├── msg_incoming.wav
        │   ├── msg_sent.wav
        │   ├── success.wav
        │   ├── error.wav
        │   ├── permission_request.wav
        │   ├── wizard_complete.wav
        │   ├── click.wav
        │   └── switch.wav
        ├── peachy/         # 温暖桃气风
        │   └── (同上结构)
        └── mochi/          # 安静茶道风
            └── (同上结构)
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
