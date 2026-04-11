# AnyClaw LVGL — 构建指南

> **目标平台**：Windows 11 (x64)
> **最终产物**：`AnyClaw_LVGL.exe`（主程序）+ `SDL2.dll`（运行时）
> **设计理念**：clone 即 build，不需要额外下载工具或依赖。

---

## 目录结构

```
AnyClaw_LVGL/
├── build/                         # 构建脚本
│   ├── toolchain-mingw64.cmake    # MinGW 交叉编译工具链
│   ├── linux/                     # Linux 构建脚本
│   │   ├── build.sh               # 一键编译+打包（推荐）
│   │   ├── build-win.sh           # 交叉编译 Windows exe
│   │   ├── build-linux.sh         # Linux 原生编译
│   │   └── run-wine.sh            # Wine 测试运行
│   └── windows/                   # Windows 构建脚本
│       ├── build.bat              # 编译（支持 --quiet/--verbose/--help）
│       ├── build-package.bat      # 一键编译+打包
│       └── setup-env.bat          # 环境检查
├── thirdparty/
│   ├── lvgl/                      # LVGL 源码（共享）
│   ├── sdl2-mingw/                # SDL2 交叉编译产物（Linux → Windows）
│   │   ├── bin/SDL2.dll
│   │   ├── lib/libSDL2.dll.a
│   │   └── include/SDL2/
│   └── sdl2-windows/              # SDL2 预编译（Windows 原生）
│       ├── include/
│       └── lib/x64/
├── src/                           # 项目源码
├── assets/                        # 图标资源
├── bundled/                       # 打包模板文件
├── out/                           # 编译输出（gitignore）
└── CMakeLists.txt
```

所有第三方依赖（LVGL、SDL2）已包含在仓库中。**不需要联网下载任何东西。**

---

## 快速开始

### Linux 交叉编译（Linux → Windows）

**前置条件：** `mingw-w64`、`cmake`（`sudo apt install -y mingw-w64 cmake`）

```bash
git clone https://github.com/IwanFlag/AnyClaw_LVGL.git
cd AnyClaw_LVGL

# 一键编译 + 打包
bash build/linux/build.sh v2.0.9

# 输出: out/mingw/bin/AnyClaw_LVGL.exe + out/AnyClaw_LVGL_v2.0.9_*.zip
```

**分步执行：**

```bash
# 1. 编译
bash build/linux/build-win.sh v2.0.9

# 2. Wine 测试（需 wine + xvfb）
bash build/linux/run-wine.sh out/mingw 10
```

### Windows 原生编译

**前置条件：** Visual Studio 2022 或 MSYS2/MinGW-w64 + cmake

```cmd
git clone https://github.com/IwanFlag/AnyClaw_LVGL.git
cd AnyClaw_LVGL

:: 一键编译+打包（默认静默）
build\windows\build-package.bat

:: 或仅编译
build\windows\build.bat
```

输出: `out\windows\bin\Release\AnyClaw_LVGL.exe`

---

## CMake 配置

`CMakeLists.txt` 关键配置：

| 变量 | 默认值 | 说明 |
|------|--------|------|
| `SDL2_CROSS_PREFIX` | `thirdparty/sdl2-mingw` | MinGW 交叉编译 SDL2 路径 |
| `SDL2_BUNDLED_PREFIX` | `thirdparty/sdl2-windows` | Windows 原生 SDL2 路径 |

CMake 自动根据编译器选择 SDL2 来源：
- **MinGW**（交叉编译）→ `thirdparty/sdl2-mingw/`
- **MSVC**（Windows 原生）→ `thirdparty/sdl2-windows/`

### 链接库清单

```
# MinGW
mingw32 SDL2main SDL2
kernel32 user32 gdi32 winmm imm32 ole32 oleaut32 version uuid
advapi32 setupapi shell32 dinput8
dwmapi comctl32 dbghelp ws2_32 comdlg32
gdiplus winhttp wininet bcrypt
-static-libgcc -static-libstdc++

# MSVC
SDL2main SDL2
kernel32 user32 gdi32 winmm imm32 ole32 oleaut32 version uuid
advapi32 setupapi shell32 dinput8
dwmapi comctl32 dbghelp ws2_32 comdlg32
gdiplus winhttp wininet bcrypt
```

### Include 路径

```
src/                                 # 项目头文件 + lv_conf.h
thirdparty/lvgl/                     # lvgl.h
thirdparty/lvgl/src/                 # drivers/sdl/*.h, widgets/**/*.h
thirdparty/sdl2-mingw/include/SDL2/  # SDL.h (交叉编译)
thirdparty/sdl2-windows/include/     # SDL.h (Windows 原生)
```

---

## 环境搭建

### Linux

```bash
# MinGW-w64 + cmake
sudo apt install -y mingw-w64 cmake pkg-config git

# Wine + Xvfb（可选，用于本地测试）
sudo apt install -y wine64 xvfb

# 验证
x86_64-w64-mingw32-gcc --version
cmake --version
```

> **阿里云镜像源不可达**时，切换清华源：
> ```bash
> cat > /etc/apt/sources.list << 'EOF'
> deb https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ noble main restricted universe multiverse
> deb https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ noble-updates main restricted universe multiverse
> deb https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ noble-security main restricted universe multiverse
> EOF
> apt-get update
> ```

### Windows

- 安装 [Visual Studio 2022](https://visualstudio.microsoft.com/)（勾选 C++ 桌面开发 + CMake）
- 或安装 [MSYS2](https://www.msys2.org/) + MinGW-w64
- cmake 需在 PATH 中，或使用 VS 自带 cmake

```cmd
build\windows\setup-env.bat    :: 检查环境
build\windows\build.bat        :: 编译
```

---

## 图标资源

应用图标（大蒜角色）通过 `.rc` 文件嵌入 exe：

```
src/AnyClaw_LVGL.rc → assets/garlic_icon.ico
```

ICO 生成（如需重新生成）：
```bash
convert assets/garlic_icon.png -resize 32x32 -define icon:auto-resize=32,16 assets/garlic_icon.ico
```

---

## 常见问题

### Q: 编译报 `SDL2 headers not found`

确保 `thirdparty/sdl2-mingw/include/SDL2/SDL.h` 或 `thirdparty/sdl2-windows/include/SDL.h` 存在。如果不存在，重新 clone 仓库。

### Q: 运行时找不到 `SDL2.dll`

`SDL2.dll` 会自动复制到 exe 同目录。如果缺失，检查编译日志中是否有 `Copying SDL2.dll` 步骤。

### Q: git clone 报 TLS 错误

```bash
git clone --depth 1 https://<TOKEN>@github.com/IwanFlag/AnyClaw_LVGL.git
```

### Q: Wine 测试黑屏

确保 Xvfb 在运行：`Xvfb :99 -screen 0 1920x1080x24 &`

---

## Git 备忘

### Ubuntu 上 git push/pull 卡住或报 TLS 错误

**现象：** `git clone`、`git push`、`git pull` 报 `GnuTLS recv error (-110): The TLS connection was non-properly terminated`，或长时间卡住无响应。

**原因：** Ubuntu 24.04 默认的 git 编译时链接的是 `libcurl-gnutls.so.4`（GnuTLS 版），与 GitHub 的 TLS 握手有已知兼容问题。HTTPS 小请求（API 调用）可能正常，但大数据传输（clone/push）容易中断。

**解决方案：改用 SSH 协议。**

```bash
# 1. 生成 SSH key（如果没有）
ssh-keygen -t ed25519 -C "your_email@example.com" -f ~/.ssh/id_ed25519 -N ""

# 2. 添加到 GitHub
cat ~/.ssh/id_ed25519.pub
# 复制输出 → GitHub → Settings → SSH and GPG keys → New SSH key → 粘贴

# 3. 验证
ssh -T git@github.com

# 4. 切换远程地址
git remote set-url origin git@github.com:IwanFlag/AnyClaw_LVGL.git
```

> **不要尝试替换 libcurl**——Ubuntu 的 `libcurl.so.4` 和 `libcurl-gnutls.so.4` 都同时依赖 OpenSSL 和 GnuTLS，替换无效。

### 其他 workaround（临时方案）

如果暂时无法用 SSH，可用以下方法：

```bash
# shallow clone（避开了部分 GnuTLS 问题）
git clone --depth 1 https://<TOKEN>@github.com/IwanFlag/AnyClaw_LVGL.git

# 增大 HTTP 缓冲区
git config --global http.postBuffer 524288000
```
