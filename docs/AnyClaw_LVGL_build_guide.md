# AnyClaw LVGL v2.0 — 编译、部署与维护指南

> **目标平台**：Windows 11 (x64)
> **版本格式**：`v2.x.x`（主版本.次版本.修订号）
> **最终产物**：`AnyClaw_LVGL.exe`（主程序）+ `SDL2.dll`（运行时）

---

## 任务执行协议

| 原则 | 说明 |
|------|------|
| **并行调度** | 每个任务分配子代理并行执行；调度前分析依赖，无依赖的并行，有依赖的串行；最高优先级优先启动 |
| **主动监控** | 主任务主动轮询子代理状态，不等汇报 |
| **实时汇报** | 阶段性结果立即通知，不等全部完成 |
| **异常干预** | 子代理卡住/超时/报错 → 主任务立即介入（重试、换方案、手动接管） |

---

## 一、快速开始

所有构建工具在 `tools/` 目录，按操作系统分类。**新环境部署直接使用脚本。**

### Linux 交叉编译

```bash
# 1. 安装构建环境（MinGW、Wine、Xvfb）— 需 sudo
sudo bash tools/linux/setup-build-env.sh

# 2. 交叉编译 SDL2 for Windows（一次性）
bash tools/linux/setup-sdl2-mingw.sh

# 3. 一键编译
bash tools/linux/build.sh

# 4. Wine 测试
bash tools/linux/run-wine.sh
```

### Windows 原生编译（推荐）

```cmd
:: 默认静默模式（推荐）
tools\windows\build-package.bat

:: 详细日志模式
tools\windows\build-package.bat --verbose

:: 查看参数说明
tools\windows\build-package.bat --help
```

说明：
- `build-package.bat` 会自动尝试加载 VS Build Tools 开发环境（优先 `C:\VSBuildTools\Common7\Tools\VsDevCmd.bat`）。
- `build.bat` / `build-package.bat` 均支持 `--help`。
- `build.bat` 默认也是静默模式，可用 `--verbose` 切换全量输出。

### tools/ 目录

```
tools/
├── linux/
│   ├── setup-build-env.sh      ← 一键安装 MinGW、Wine、Xvfb
│   ├── setup-sdl2-mingw.sh     ← 交叉编译 SDL2
│   ├── toolchain-mingw64.cmake ← CMake 工具链文件
│   ├── build.sh                ← 一键编译
│   └── run-wine.sh             ← Wine 测试
└── windows/
    ├── setup-env.bat           ← 环境检查
    ├── build.bat               ← Windows 原生编译（支持 --quiet/--verbose/--help）
    ├── fetch-runtime-deps.bat  ← 拉取 SDL2 运行时依赖
    └── build-package.bat       ← 一键编译+打包（默认静默，支持 --verbose/--help）
```

---

## 二、编译环境搭建

### 2.0 CMakeLists.txt

项目根目录 `CMakeLists.txt` 是编译的核心配置文件。仓库中可能不包含此文件（已在 `.gitignore` 或手动排除），新环境需确认存在。

**关键配置项：**

| 配置 | 说明 |
|------|------|
| `SDL2_CROSS_PREFIX` | 交叉编译 SDL2 路径，默认 `/opt/sdl2-mingw` |
| `LVGL_SOURCES` | `thirdparty/lvgl/src/*.c`（GLOB_RECURSE），排除自带字体文件 |
| `APP_SOURCES` | `src/*.cpp` + `src/*.c`（字体文件） |
| `APP_RESOURCES` | `src/AnyClaw_LVGL.rc`（图标嵌入） |
| 链接顺序 | `mingw32 → SDL2main → SDL2 → Windows 系统库` |

**完整链接库清单：**

```
mingw32 SDL2main SDL2
kernel32 user32 gdi32 winmm imm32 ole32 oleaut32 version uuid
advapi32 setupapi shell32 dinput8
dwmapi comctl32 dbghelp ws2_32 comdlg32
gdiplus winhttp
-static-libgcc -static-libstdc++
```

**Include 路径：**

```
src/                              ← 项目头文件 + lv_conf.h
thirdparty/lvgl/                  ← lvgl.h
thirdparty/lvgl/src/              ← drivers/sdl/*.h, widgets/**/*.h
${SDL2_CROSS_PREFIX}/include/SDL2 ← SDL.h
```

**注意事项：**
- LVGL 的 `file(GLOB_RECURSE)` 会扫描所有 `.c` 文件，项目自带的 `lv_font_mshy_16.c` 和 `lv_font_simhei_16.c` 需排除，否则多重定义
- `WIN32` 标志使程序为 GUI 子系统（不弹 CMD 窗口），需要 `mingw32` 库提供正确的 CRT 入口
- `mingw32` 必须在 `SDL2main` 之前链接，否则 `WinMain` 无法解析

### 2.1 安装 MinGW-w64

```bash
sudo apt update
sudo apt install -y mingw-w64 cmake pkg-config git
```

> **⚠️ 阿里云镜像源不可达**：阿里云 ECS 上 `mirrors.cloud.aliyuncs.com` 可能超时。
> 如果 `apt update` 报错连接超时，切换到清华源：
> ```bash
> cat > /etc/apt/sources.list << 'EOF'
> deb https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ noble main restricted universe multiverse
> deb https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ noble-updates main restricted universe multiverse
> deb https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ noble-backports main restricted universe multiverse
> deb https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ noble-security main restricted universe multiverse
> EOF
> apt-get update
> ```

验证：

```bash
x86_64-w64-mingw32-gcc --version | head -1
x86_64-w64-mingw32-g++ --version | head -1
cmake --version | head -1
```

### 2.2 交叉编译 SDL2

> **⚠️ git clone 网络问题**：GitHub 克隆大仓库可能报 `RPC failed; curl 56 Recv failure: Connection reset by peer`。
> 解决：增大 HTTP 缓冲区：
> ```bash
> git config --global http.postBuffer 524288000
> ```

```bash
git clone --depth 1 https://github.com/libsdl-org/SDL.git -b release-2.30.3 /tmp/SDL2-src
mkdir /tmp/SDL2-build && cd /tmp/SDL2-build
cmake /tmp/SDL2-src \
  -DCMAKE_SYSTEM_NAME=Windows \
  -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
  -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
  -DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres \
  -DCMAKE_INSTALL_PREFIX=/opt/sdl2-mingw \
  -DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TEST=OFF
make -j$(nproc) && make install
```

### 2.3 项目依赖

| 依赖 | 路径 | 说明 |
|------|------|------|
| SDL2 (MinGW x64) | 交叉编译安装 | `libSDL2.dll.a`、`SDL2.dll` |
| LVGL 源码 | `thirdparty/lvgl/` | 源码编译 |
| LVGL Win32 FS | `lv_conf.h` | `LV_USE_FS_WIN32 1`，drive letter `A:`，用于加载 PNG 图标 |
| Assets | `assets/` | 图标资源（编译后需复制到 build 目录） |

---

## 三、编译与打包

### 3.1 编译主程序

```bash
cd AnyClaw_LVGL
mkdir -p build-mingw && cd build-mingw
cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=../tools/linux/toolchain-mingw64.cmake \
  -DSDL2_CROSS_PREFIX=/opt/sdl2-mingw \
  -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

产物：`build-mingw/bin/`
- `AnyClaw_LVGL.exe` — 主程序
- `SDL2.dll` — SDL2 运行时

验证：

```bash
file bin/AnyClaw_LVGL.exe
# PE32+ executable (GUI) x86-64, for MS Windows

x86_64-w64-mingw32-objdump -p bin/AnyClaw_LVGL.exe | grep "DLL Name:" | sort -u
# 应无 libgcc/libstdc++ 依赖（静态链接生效）
```

### 3.2 图标资源

应用图标（大蒜角色）通过 `.rc` 文件嵌入 exe：

```
# src/AnyClaw_LVGL.rc
IDI_ICON1 ICON "../assets/garlic_icon.ico"
```

ICO 包含 32x32 + 16x16 双尺寸，由大蒜角色原图生成：

```bash
convert assets/garlic_icon.png -resize 32x32 -define icon:auto-resize=32,16 assets/garlic_icon.ico
```

LVGL 标题栏图标使用 `assets/garlic_32.png`（32x32 大蒜角色，无 LED）。

图标加载逻辑（tray.cpp）：
1. `LoadImageW(MAKEINTRESOURCEW(1))` — 嵌入资源
2. `CreateIconFromResourceEx` — RT_GROUP_ICON 回退
3. 文件 fallback：exe 同目录 → `../assets/` → `%APPDATA%/AnyClaw/`

### 3.3 打包发布

#### 版本号格式

```
v{主版本}.{次版本}.{修订号}
```

示例：`v2.0.1`、`v2.1.0`、`v2.0.1`

- **主版本**：大版本升级（v1→v2 框架迁移）
- **次版本**：功能新增或 PRD 状态变更
- **修订号**：Bug 修复、小改动

#### 命名格式

```
AnyClaw_LVGL_{版本号}_{时间戳}.zip
```

示例：`AnyClaw_LVGL_v2.0.1_20260404_2009.zip`

#### 打包命令

```bash
cd build-mingw/bin
VERSION="v2.0.1"
TIMESTAMP=$(date '+%Y%m%d_%H%M')
ZIP_NAME="AnyClaw_LVGL_${VERSION}_${TIMESTAMP}.zip"

PKG_DIR="_pkg"
mkdir -p "${PKG_DIR}/assets/tray" "${PKG_DIR}/assets/icons/ai"
cp AnyClaw_LVGL.exe SDL2.dll "${PKG_DIR}/"
cp ../../assets/app_icon.png ../../assets/garlic_icon.png ../../assets/garlic_32.png ../../assets/oc_icon_24.png "${PKG_DIR}/assets/"
cp ../../assets/tray/*.png "${PKG_DIR}/assets/tray/"
cp ../../assets/icons/ai/*.png "${PKG_DIR}/assets/icons/ai/"
cd "${PKG_DIR}" && zip -r "../${ZIP_NAME}" . && cd .. && rm -rf "${PKG_DIR}"
```

#### 包内容

```
AnyClaw_LVGL/
├── AnyClaw_LVGL.exe          主程序（静态链接 MinGW 运行时） ~7.4 MB
├── SDL2.dll                   SDL2 运行时库 ~3.1 MB
└── assets/
    ├── app_icon.png           应用图标（512x512）
    ├── garlic_icon.png        大蒜角色图标（用户头像）
    ├── garlic_32.png          大蒜角色图标 32x32（标题栏用，无 LED）
    ├── oc_icon_24.png         OpenClaw 图标（备用）
    ├── icons/
    │   └── ai/
    │       ├── ai_01_24.png   AI 头像 24x24
    │       ├── ai_01_32.png   AI 头像 32x32
    │       ├── ai_01_48.png   AI 头像 48x48
    │       ├── ai_02_*.png    AI 头像方案2
    │       └── ai_03_*.png    AI 头像方案3
    └── tray/
        ├── tray_gray_*.png    托盘图标（灰色 LED）
        ├── tray_green_*.png   托盘图标（绿色 LED）
        ├── tray_yellow_*.png  托盘图标（黄色 LED）
        └── tray_red_*.png     托盘图标（红色 LED）
```

**合计 zip：** ~4.7 MB

### 3.4 代码签名

证书文件保存在 `build-mingw/` 目录下：

| 文件 | 说明 |
|------|------|
| `sign_key.pem` | 私钥 |
| `sign_cert.pem` | 自签名证书 |
| `sign.pfx` | PKCS12 打包（密码 `anyclaw`） |

**首次生成证书（仅需一次，后续复用）：**

```bash
cd build-mingw
openssl req -x509 -newkey rsa:2048 -keyout sign_key.pem \
  -out sign_cert.pem -days 1095 -nodes -subj "/CN=AnyClaw/O=AnyClaw/C=CN"
openssl pkcs12 -export -out sign.pfx -inkey sign_key.pem -in sign_cert.pem -password pass:anyclaw
```

**每次编译后签名：**

```bash
sudo apt install -y osslsigncode

cd build-mingw/bin
osslsigncode sign -pkcs12 ../sign.pfx -pass anyclaw \
  -n "AnyClaw LVGL" -i "https://github.com/IwanFlag/AnyClaw_LVGL" \
  -t http://timestamp.digicert.com \
  -in AnyClaw_LVGL.exe -out AnyClaw_LVGL_signed.exe
mv AnyClaw_LVGL_signed.exe AnyClaw_LVGL.exe
```

> **注意**：自签名证书在 Windows 上会显示"无法验证发布者"，适用于开发测试。正式发布需购买受信任的代码签名证书。
> `build-mingw/` 证书文件已加入 `.gitignore`，不会上传到仓库。

### 3.5 一键全流程

```bash
#!/bin/bash
cd AnyClaw_LVGL
mkdir -p build-mingw && cd build-mingw
cmake .. -DCMAKE_TOOLCHAIN_FILE=../tools/linux/toolchain-mingw64.cmake \
         -DSDL2_CROSS_PREFIX=/opt/sdl2-mingw -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)

# 代码签名
if [ -f sign.pfx ]; then
  cd bin
  osslsigncode sign -pkcs12 ../sign.pfx -pass anyclaw \
    -n "AnyClaw LVGL" -i "https://github.com/IwanFlag/AnyClaw_LVGL" \
    -t http://timestamp.digicert.com \
    -in AnyClaw_LVGL.exe -out AnyClaw_LVGL_signed.exe
  mv AnyClaw_LVGL_signed.exe AnyClaw_LVGL.exe
  echo "🔐 Signed OK"
else
  cd bin
  echo "⚠️ sign.pfx not found, skipping signing"
fi

# 打包
VERSION="v2.0.1"
TS=$(date '+%Y%m%d_%H%M')
ZIP_NAME="AnyClaw_LVGL_${VERSION}_${TS}.zip"
PKG_DIR="_pkg"
mkdir -p "${PKG_DIR}/assets/tray" "${PKG_DIR}/assets/icons/ai"
cp AnyClaw_LVGL.exe SDL2.dll "${PKG_DIR}/"
cp ../../assets/app_icon.png ../../assets/garlic_icon.png ../../assets/garlic_32.png ../../assets/oc_icon_24.png "${PKG_DIR}/assets/"
cp ../../assets/tray/*.png "${PKG_DIR}/assets/tray/"
cp ../../assets/icons/ai/*.png "${PKG_DIR}/assets/icons/ai/"
cd "${PKG_DIR}" && zip -r "../${ZIP_NAME}" . && cd .. && rm -rf "${PKG_DIR}"
echo "✅ 发布包: build-mingw/bin/${ZIP_NAME}"
```

---

## 四、部署与运行

### 4.1 部署步骤

1. 将 zip 解压到任意目录
2. 确保 `AnyClaw_LVGL.exe` 和 `SDL2.dll` 在同一目录
3. 双击 `AnyClaw_LVGL.exe` 运行

无需安装，无外部依赖。

### 4.2 首次运行行为

1. 创建系统托盘图标（紫色图标 + 灰色状态 LED）
2. 显示法律声明弹窗（需勾选同意）
3. 自动检测 OpenClaw Gateway 状态
4. 窗口关闭最小化到托盘（非退出）

### 4.3 运行依赖

| 依赖 | 位置 | 说明 |
|------|------|------|
| `SDL2.dll` | 同目录 | SDL2 运行时 |
| OpenClaw | 系统 PATH 或默认安装 | 可选，未安装时显示"未找到" |
| Windows 系统 DLL | 系统自带 | KERNEL32、USER32、GDI32 等 |

### 4.4 配置文件

| 文件 | 路径 | 说明 |
|------|------|------|
| `config.json` | `%APPDATA%\AnyClaw_LVGL\` | 主配置（语言、主题、窗口位置等） |
| `accepted.json` | `%APPDATA%\AnyClaw_LVGL\` | 法律声明接受标记 |
| `app.log` | `%APPDATA%\AnyClaw_LVGL\logs\` | 应用日志 |
| `crash_*.log` | `%APPDATA%\AnyClaw_LVGL\crash_logs\` | 崩溃日志 |

### 4.5 卸载

删除以下内容：
1. 程序目录（exe + dll）
2. `%APPDATA%\AnyClaw_LVGL\` 目录
3. 开机自启动注册表项（如有）：`HKCU\Software\Microsoft\Windows\CurrentVersion\Run\AnyClaw_LVGL`

---

## 五、测试验证

### 5.0 截图规范

**每次修改代码后必须截图验证，截图保存到项目根目录。**

- 命名格式：`AAA_{描述}_{YYYYMMDD_HHMM}.png`
- 示例：`AAA_chat_layout_20260405_1415.png`
- 测试前先删除旧的 `AAA_*.png` 文件
- 截图内容应包含完整的聊天区域，验证消息布局、文字显示、换行效果

```bash
# 标准截图流程
cd AnyClaw_LVGL
rm -f AAA_*.png
cd build-mingw/bin
# 启动应用
DISPLAY=:99 WINEDEBUG=-all wine AnyClaw_LVGL.exe &>/dev/null &
sleep 7
# 截图
TS=$(date '+%Y%m%d_%H%M')
xwd -root -display :99 -out /tmp/anyclaw.xwd
convert /tmp/anyclaw.xwd ../../AAA_chat_${TS}.png
```

### 5.1 Wine 测试（Linux 本地）

```bash
sudo apt install -y wine64 xvfb
cd build-mingw/bin
xvfb-run -a wine AnyClaw_LVGL.exe
```

### 5.2 UI 截图

```bash
xvfb-run -a --server-args="-screen 0 1280x720x24" bash -c '
  WINEDEBUG=warn,err wine AnyClaw_LVGL.exe 2>/tmp/wine_err.log &
  sleep 5
  xwd -root -out /tmp/anyclaw.xwd
  convert /tmp/anyclaw.xwd -resize 800x /tmp/anyclaw.png
  pkill -f AnyClaw_LVGL
'
```

### 5.3 实测记录

| 日期 | 版本 | Wine | 状态 |
|------|------|------|------|
| 2026-04-04 | v2.0.1-dev | 9.0 | ✅ 启动正常，UI 完整渲染 |
| 2026-04-04 | v2.0.1-dev (rebuild) | 9.0 | ✅ 窗口创建、SDL2 渲染、OpenClaw 检测正常 |

> **⚠️ run-wine.sh 误报**：`run-wine.sh` 通过 grep `CRASH` 判断崩溃，但日志中的 `"[CRASH] Crash handler installed"` 是正常启动信息，会被误判。
> 实际判断标准：看是否有 `[INFO ] [MAIN ] === AnyClaw LVGL` 输出、窗口句柄是否创建。
> Wine 环境下 `fixme:` 消息属于正常（Wine API 不完整实现），非错误。

---

## 六、CJK 字体生成

当前字体 `src/lv_font_mshy_16.c` 覆盖：ASCII + CJK 标点 + 汉字 12000+ 字 + 全角字符。

如需修改字符范围：

```bash
npx lv_font_conv \
  --font msyh_0.ttf \
  --format lvgl --size 16 --bpp 4 \
  --no-compress --no-prefilter --force-fast-kern-format \
  -r 0x20-0x7E -r 0x3000-0x303F -r 0xFF00-0xFFEF -r 0x4E00-0x7FFF \
  --lv-include lvgl/lvgl.h \
  --lv-font-name lv_font_mshy_16 \
  -o src/lv_font_mshy_16.c
```

> CJK 范围超过 0x5FFF 需在 `lv_conf.h` 启用 `LV_FONT_FMT_TXT_LARGE 1`。

---

## 七、已知问题与修复

### 线程安全（2026-04-04）

`tray_set_state()` 被后台线程调用时直接执行 GDI 操作，与主线程 LVGL 渲染并发冲突 → 空闲后点击崩溃。

修复：后台线程只写原子标志（`InterlockedExchange`），主线程在 `tray_process_messages()` 中应用。

### 聊天输入框崩溃（2026-04-04）

`chat_cont` 的 `SCROLL_ON_FOCUS` 与自定义拖拽拦截冲突 → 快速点击崩溃。

修复：清除 `LV_OBJ_FLAG_SCROLL_ON_FOCUS` + re-entrancy guard。

### 崩溃日志（2026-04-04，E-01-2）

`SetUnhandledExceptionFilter` 捕获异常 → 自动生成崩溃日志到 `%APPDATA%\AnyClaw_LVGL\crash_logs\`。

### CJK 换行（2026-04-04）

LVGL 默认按词换行，CJK 无空格导致不换行。修复：`LV_TEXT_FLAG_BREAK_ALL` 逐字换行。

### 图标加载（2026-04-04）

`LoadImageW` 不支持 PNG 压缩 ICO（错误码 1813）。修复：生成 BMP-based ICO + 4 级 fallback。

### CMakeLists.txt 缺失（2026-04-06）

仓库中未包含 `CMakeLists.txt`，新环境克隆后无法编译。需在项目根目录创建，包含 LVGL 源码编译、SDL2 交叉编译链接、Windows 系统库依赖、资源文件嵌入等完整配置。详见 §2.0。

### LVGL include 路径缺失（2026-04-06）

源码中 `#include "drivers/sdl/lv_sdl_window.h"` 等路径相对于 `thirdparty/lvgl/src/`，CMake 仅添加 `thirdparty/lvgl/` 到 include 路径会导致找不到头文件。

修复：`target_include_directories` 同时添加 `thirdparty/lvgl` 和 `thirdparty/lvgl/src`。

### 字体文件多重定义（2026-04-06）

`file(GLOB_RECURSE LVGL_SOURCES)` 扫描 LVGL 全部 `.c` 文件，但项目 `src/` 下的 `lv_font_mshy_16.c` 和 `lv_font_simhei_16.c` 也在 `APP_SOURCES` 中单独列出，导致链接时多重定义。

修复：GLOB 后遍历结果，排除与 APP_SOURCES 重名的字体文件。

### WinMain 未找到（2026-04-06）

`WIN32` 标志使 MinGW 链接器使用 `crtexewin.c` 入口点（期望 `WinMain`），但 `libSDL2main.a` 中的 `WinMain` 未被正确解析。

原因：MinGW 的 `libmingw32.a` 必须在 `SDL2main` 之前链接，提供 CRT 初始化；否则 `WinMain` 符号无法从 `SDL2main.a` 中提取。

修复：链接顺序 `mingw32 → SDL2main → SDL2 → ...`。

### 缺少 gdiplus / winhttp / comdlg32 库（2026-04-06）

`tray.cpp` 使用 GDI+（托盘图标绘制）、`http_client.cpp` 使用 WinHTTP、`ui_main.cpp` 使用 `GetOpenFileNameA`（文件选择对话框），编译时未链接对应库导致大量 `undefined reference`。

修复：CMakeLists.txt 添加 `gdiplus winhttp comdlg32`。

---

## 八、常见问题

| 问题 | 原因 | 解决方案 |
|------|------|---------|
| `x86_64-w64-mingw32-gcc: not found` | mingw-w64 未安装 | `sudo apt install -y mingw-w64` |
| `Unable to locate package` | apt 镜像源不可达 | 切换清华源（见 2.1 节） |
| `RPC failed; curl 56` | git clone 网络不稳定 | `git config --global http.postBuffer 524288000` |
| SDL2 链接失败 | 交叉编译库缺失 | 重新执行 `setup-sdl2-mingw.sh` |
| `Too large font or glyphs` | CJK 字体超 4096 字形 | `lv_conf.h` 启用 `LV_FONT_FMT_TXT_LARGE` |
| Wine 运行崩溃 | SDL2 D3D 后端不兼容 | 已知限制，需真实 Windows 环境 |
| 任务栏/托盘无图标 | ICO 格式不兼容 | 用 ImageMagick 生成 BMP-based ICO |
| `Already Running` 弹窗 | Wine 旧进程未退出 | `pkill -f AnyClaw_LVGL` |
| run-wine.sh 误报 CRASH | `"[CRASH] Crash handler installed"` 被 grep 匹配 | 正常行为，看 `[INFO ] [MAIN ]` 确认实际启动 |
| Windows 弹出 CMD 窗口 | AllocConsole() 已移除 (2026-04-05) | 已修复，debug 输出走 app.log |

---

## 九、Git 操作规范

### 9.1 仓库信息

| 项目 | 值 |
|------|-----|
| 仓库 | `https://github.com/IwanFlag/AnyClaw_LVGL.git` |
| 分支 | `main` |
| 认证 | HTTPS + Token |

### 9.2 常用命令

```bash
git clone https://{token}@github.com/IwanFlag/AnyClaw_LVGL.git
git pull origin main
git add -A
git commit -m '[AnyClaw][LVGL] 模块&模块'
git push origin main
```

### 9.3 提交格式

```
[AnyClaw][LVGL] 功能模块&功能模块

修改内容：
1. 功能点1：具体描述
2. 修改文件：file1.cpp、file2.h

修改人：{hostname}
上传时间：{yyyy-MM-dd HH:mm}
```

### 9.4 规则

- 交叉编译验证通过后再提交
- 每次代码修改检查 `docs/` 是否同步更新
- `build-mingw/` 不提交（.gitignore 已配置）
- 文档与代码一并提交

### 9.5 文档目录

| 文件 | 内容 |
|------|------|
| `docs/AnyClaw_LVGL_PRD.md` | 产品需求文档 |
| `docs/AnyClaw_LVGL_build_guide.md` | 本文档 |
| `tools/` | 构建脚本 |

---


---

## 九附、Bundled 包 & 安装流程

### 安装链路

AnyClaw 负责帮用户安装和配置 OpenClaw，用户无需手动操作。

```
Step 1: 环境检测
  ├── Node.js    → node --version → >= v22.14
  ├── npm        → npm --version
  ├── Network    → HTTP openrouter.ai
  └── OpenClaw   → where openclaw

Step 2: 安装（如需要）
  ┌─────────────────────────┐  ┌─────────────────────────┐
  │  🟢 从网络下载（推荐）    │  │  🔵 使用本地包（离线）    │
  └─────────────────────────┘  └─────────────────────────┘
  网络失败 → 自动回退到本地

Step 3: 初始化 → openclaw gateway start → stop（生成默认配置）

Step 4: 配置向导 → 模型 + API Key → 写入 openclaw.json

Step 5: 验证 → openclaw --version + curl /health
```

### 配置路径

| 内容 | 路径 |
|------|------|
| OpenClaw 配置 | `%USERPROFILE%\.openclaw\openclaw.json` |
| OpenClaw 工作区 | `%USERPROFILE%\.openclaw\workspace\` |
| AnyClaw 配置 | `%APPDATA%\AnyClaw_LVGL\config.json` |

### Bundled 包

AnyClaw 安装包自带 OpenClaw npm 包，用于离线安装。

```bash
# 生成 bundled 包（在有 openclaw 的机器上运行）
bash tools/linux/prepare-bundled.sh

# 打包时自动包含 bundled/
bash tools/linux/build.sh
```

产物结构：
```
AnyClaw_LVGL_v2.x.x.zip
├── AnyClaw_LVGL.exe
├── SDL2.dll
├── assets/
└── bundled/
    ├── openclaw.tgz           ← 离线安装包
    └── workspace/             ← OpenClaw 工作区模板
        ├── AGENTS.md
        ├── SOUL.md
        ├── BOOTSTRAP.md
        ├── HEARTBEAT.md
        ├── IDENTITY.md
        ├── USER.md
        └── TOOLS.md
```

### 工作区模板

`bundled/workspace/` 包含 OpenClaw Agent 的基础配置文件。AnyClaw 安装 OpenClaw 后，自动将模板文件复制到 `%USERPROFILE%\.openclaw\workspace\`（仅复制不存在的文件，不覆盖用户已有配置）。

模板内容：
- **AGENTS.md** — Agent 行为协议、安全规则、群聊规范
- **SOUL.md** — Agent 人格与安全思维链
- **BOOTSTRAP.md** — 首次运行引导（完成后自动删除）
- **HEARTBEAT.md** — 心跳检查配置
- **IDENTITY.md** — Agent 身份信息（名称/头像/性格）
- **USER.md** — 用户信息记录
- **TOOLS.md** — 本地工具配置笔记

### 卸载

- **保留配置卸载**：`npm uninstall -g openclaw`
- **完全卸载**：npm uninstall + 删除 `%USERPROFILE%\.openclaw\` + `%APPDATA%\AnyClaw_LVGL\`
- **向导中断**：安装了 OpenClaw 但没完成向导 → 自动触发完全卸载

### Node.js 要求

OpenClaw 官方要求：Node 24（推荐）或 Node 22.14+（兼容）

下载源：
- GitHub (AnyClaw): https://github.com/IwanFlag/AnyClaw_Tools/releases
- 主站：https://nodejs.org/
- 国内镜像：https://registry.npmmirror.com/-/binary/node/

## 十、构建记录

| 时间 | 版本 | 产物 | 大小 | 环境 |
|------|------|------|------|------|
| 2026-04-04 20:09 | v2.0.1-dev | AnyClaw_LVGL.exe + SDL2.dll | 7.4M + 4.1M | MinGW 13 / Ubuntu 24.04 |
| 2026-04-04 20:33 | v2.0.1-dev (rebuild) | AnyClaw_LVGL.exe + SDL2.dll | 7.4M + 3.1M | MinGW 13 / Ubuntu 24.04 / 清华源 |
| 2026-04-04 22:08 | v2.0.1-dev | AnyClaw_LVGL.exe + SDL2.dll | 7.4M + 4.1M | MinGW 13 / Ubuntu 24.04 |
| 2026-04-04 22:22 | v2.0.1-dev | AnyClaw_LVGL.exe + SDL2.dll | 7.3M + 3.1M | MinGW 13 / Ubuntu 24.04 | 初次构建（新环境） |
| 2026-04-04 22:58 | v2.0.1-dev | AnyClaw_LVGL.exe + SDL2.dll | 7.4M + 3.1M | MinGW 13 / Ubuntu 24.04 | TASK-006/007 全部修复 |
| 2026-04-04 23:11 | v2.0.1-dev | AnyClaw_LVGL.exe + SDL2.dll | 7.4M + 3.1M | MinGW 13 / Ubuntu 24.04 | 气泡修复 + 弹窗延迟修复 |
| 2026-04-05 00:47 | v2.0.1-dev | exe + dll + assets/ (13 files) | 7.4M + 3.1M + 28K | MinGW 13 / Ubuntu 24.04 | 托盘图标重绘 + 头像 + UI 优化 |
| 2026-04-05 05:27 | v2.0.1-dev | AnyClaw_LVGL_v2.0.1_20260405_0527.zip | 4.8M | MinGW 13 / Ubuntu 24.04 | 首次环境搭建测试 |
| 2026-04-05 05:55 | v2.0.1-dev | AnyClaw_LVGL_v2.0.1_20260405_0555.zip | 4.8M | MinGW 13 / Ubuntu 24.04 | UI 修复批次：任务列表间距/按钮/图标/主题下拉框 |
| 2026-04-05 06:04 | v2.0.1-dev | AnyClaw_LVGL_v2.0.1_20260405_0604.zip | 4.8M | MinGW 13 / Ubuntu 24.04 | 同上，含 task_test 文档更新 |
| 2026-04-05 07:10 | v2.0.1-dev | AnyClaw_LVGL_v2.0.1_20260405_0710.zip | 4.8M | MinGW 13 / Ubuntu 24.04 | TASK-013: UI修复批次2+设置页重构+图标重绘+AllocConsole移除 |
| 2026-04-05 11:04 | v2.0.1-dev | AnyClaw_LVGL_v2.0.1_20260405_1104.zip | 5.2M | MinGW 13 / Ubuntu 24.04 | TASK-014: 聊天UI优化+编译环境搭建 |
| 2026-04-05 12:31 | v2.0.1-dev | AnyClaw_LVGL_v2.0.1_20260405_1231.zip | 5.1M | MinGW 13 / Ubuntu 24.04 | 新环境搭建：MinGW+SDL2交叉编译+全量编译+打包 |
| 2026-04-05 12:42 | v2.0.1-dev | AnyClaw_LVGL_v2.0.1_20260405_1242.zip | 5.1M | MinGW 13 / Ubuntu 24.04 | 消息边距从百分比改固定50px |
| 2026-04-05 12:43 | v2.0.1-dev | AnyClaw_LVGL_v2.0.1_20260405_1243.zip | 5.1M | MinGW 13 / Ubuntu 24.04 | 边距50px→6px |
| 2026-04-05 12:46 | v2.0.1-dev | AnyClaw_LVGL_v2.0.1_20260405_1246.zip | 5.1M | MinGW 13 / Ubuntu 24.04 | 用户消息右对齐(style_align) |
| 2026-04-05 12:48 | v2.0.1-dev | AnyClaw_LVGL_v2.0.1_20260405_1248.zip | 5.1M | MinGW 13 / Ubuntu 24.04 | 用户气泡文字改左对齐 |
| 2026-04-05 12:50 | v2.0.1-dev | AnyClaw_LVGL_v2.0.1_20260405_1250.zip | 5.1M | MinGW 13 / Ubuntu 24.04 | 统一AI/用户容器结构(wrap→meta+label) |
| 2026-04-05 13:14 | v2.0.1-dev | AnyClaw_LVGL_v2.0.1_20260405_1314.zip | 5.1M | MinGW 13 / Ubuntu 24.04 | 用户消息右对齐最终方案(IGNORE_LAYOUT+手动定位) |
| 2026-04-05 14:04 | v2.0.1-dev | AnyClaw_LVGL_v2.0.1_20260405_1404.zip | 5.1M | MinGW 13 / Ubuntu 24.04 | TASK-015: QQ风格聊天布局重构+流式时间戳修复+max_width 70% |
| 2026-04-05 14:51 | v2.0.1-dev | AnyClaw_LVGL_v2.0.1_20260405_1451.zip | 5.2M | MinGW 13 / Ubuntu 24.04 | TASK-016: 气泡自适应宽度
| 2026-04-05 15:28 | v2.0.0 | AnyClaw_LVGL_v2.0.0_20260405_1528.zip | 5.2M | MinGW 13 / Ubuntu 24.04 | 新环境首次编译
| 2026-04-05 15:38 | v2.0.1 | AnyClaw_LVGL_v2.0.1_20260405_1538.zip | 5.2M | MinGW 13 / Ubuntu 24.04 | 剪贴板通用化+设置页textarea
| 2026-04-05 15:43 | v2.0.1 | AnyClaw_LVGL_v2.0.1_20260405_1543.zip | 5.2M | MinGW 13 / Ubuntu 24.04 | textarea光标+按钮27px
| 2026-04-05 15:46 | v2.0.1 | AnyClaw_LVGL_v2.0.1_20260405_1546.zip | 5.2M | MinGW 13 / Ubuntu 24.04 | 上传按钮全局化+chat_cont透明
| 2026-04-05 15:48 | v2.0.1 | AnyClaw_LVGL_v2.0.1_20260405_1548.zip | 5.2M | MinGW 13 / Ubuntu 24.04 | 双击标题栏PRESSED
| 2026-04-05 16:10 | v2.0.1 | AnyClaw_LVGL_v2.0.1_20260405_1610.zip | 5.2M | MinGW 13 / Ubuntu 24.04 | TASK-017: SDL Event Watch重构 |
| 2026-04-05 17:18 | v2.0.1 | AnyClaw_LVGL_v2.0.1_20260405_1718.zip | 5.2M | MinGW 13 / Ubuntu 24.04 | TASK-018: 选中淡蓝色+双击最大化+分隔条+上传菜单 |
| 2026-04-05 22:06 | v2.0.0 | AnyClaw_LVGL_v2.0.0_20260405_2206.zip | 4.8M | MinGW 13 / Ubuntu 24.04 | 新环境首次编译+签名（自签名证书） |
| 2026-04-06 14:29 | v2.0.0 | AnyClaw_LVGL_v2.0.0_20260406_1429.zip | 6.5M | MinGW 13 / Ubuntu 24.04 | 新环境搭建：MinGW+SDL2交叉编译+全量编译 |
| 2026-04-06 14:44 | v2.0.0 | AnyClaw_LVGL_v2.0.0_20260406_1444.zip | 6.5M | MinGW 13 / Ubuntu 24.04 | 模型列表清理（移除付费）+ 自定义模型功能 |
| 2026-04-06 14:47 | v2.0.0 | AnyClaw_LVGL_v2.0.0_20260406_1447.zip | 6.5M | MinGW 13 / Ubuntu 24.04 | 自定义模型：重复检测+已有模型提示+API Key更新 |
| 2026-04-06 14:49 | v2.0.0 | AnyClaw_LVGL_v2.0.0_20260406_1449.zip | 6.5M | MinGW 13 / Ubuntu 24.04 | custom_models→custom_add_models重命名 |
| 2026-04-06 14:51 | v2.0.0 | AnyClaw_LVGL_v2.0.0_20260406_1451.zip | 6.5M | MinGW 13 / Ubuntu 24.04 | OpenClaw配置自动同步：新增模型自动保存+UI刷新 |
| 2026-04-07 04:06 | v2.0.1 | AnyClaw_LVGL_v2.0.1_20260407_0406.zip | 7.1M | MinGW 13 + FreeType 2.13.3 / Ubuntu 24.04 | 弹性通道+任务hover+FreeType+工作区模板+详细日志+Backspace修复 |
| 2026-04-07 11:30 | v2.0.1 | AnyClaw_LVGL.exe + SDL2.dll | 12M + 3.1M | MinGW 13 / Ubuntu 24.04 | 新环境首次编译+SDL2交叉编译+弹性通道评分算法完善+LVGL源码排除优化 |

---

## 十一、聊天消息对称布局调试记录（2026-04-05）

### 11.1 需求

聊天区域消息布局要求：
- **AI 消息**：整体靠左，左边距 6px
- **用户消息**：整体靠右，右边距 6px
- 两者 meta row 镜像对称（外侧头像，内侧名字+时间戳）
- 结构统一：`wrap → [meta, label]`

```
┌─ chat_cont ──────────────────────────────────────┐
│  [🦞] [AI] [时间戳]                    ← AI 靠左  │
│  消息内容...                                       │
│                                                   │
│                    [时间戳] [User] [🧄]  ← 用户靠右│
│                       ┌──────────┐                │
│                       │ 消息内容  │                │
│                       └──────────┘                │
└───────────────────────────────────────────────────┘
  ↑6px                                      6px↑
```

### 11.2 问题根因

`chat_cont` 是 LVGL flex COLUMN 容器，`cross_align = FLEX_ALIGN_START`（所有子元素左对齐）。

**尝试过的失败方案：**

| 方案 | 结果 | 原因 |
|------|------|------|
| `lv_obj_set_style_align(wrap, LV_ALIGN_RIGHT_MID, 0)` | ❌ 无效 | flex 容器覆盖子元素的 style_align |
| `wrap` 用 ROW flex + `FLEX_ALIGN_END` + spacer 弹性填充 | ❌ 无效 | spacer 的 flex_grow 在 COLUMN 父容器内不生效 |
| `wrap` 设 `width=LV_PCT(100)` + 内部 `FLEX_ALIGN_END` | ❌ 部分生效 | width=100% 时 cross_place=END 的偏移量为 0 |
| `lv_obj_set_x(wrap, ...)` 手动设坐标 | ❌ 无效 | flex 布局刷新时覆盖手动坐标 |
| `lv_obj_set_style_translate_x(wrap, offset, 0)` | ❌ 越界 | translate 把 100% 宽的元素推到容器外面 |

### 11.3 最终方案

使用 `LV_OBJ_FLAG_IGNORE_LAYOUT` 让用户消息 wrap 跳出 flex 布局，手动计算 `x, y` 坐标：

```c
// 1. 创建 wrap，设 IGNORE_LAYOUT 跳出 flex
lv_obj_t* wrap = lv_obj_create(chat_cont);
lv_obj_set_width(wrap, LV_SIZE_CONTENT);
lv_obj_add_flag(wrap, LV_OBJ_FLAG_IGNORE_LAYOUT);

// 2. 内部正常用 COLUMN flex 排列 meta + label
lv_obj_set_flex_flow(wrap, LV_FLEX_FLOW_COLUMN);
lv_obj_set_flex_align(wrap, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);

// 3. 填充内容后，手动计算右对齐位置
lv_obj_update_layout(chat_cont);
int32_t cont_w = lv_obj_get_content_width(chat_cont);
lv_obj_update_layout(wrap);
int32_t wrap_w = lv_obj_get_width(wrap);
int32_t x_pos = cont_w - wrap_w - CHAT_MSG_MARGIN;  // 右边距 6px

// 4. 计算 y 位置：找到最后一个非 IGNORE_LAYOUT 的子元素底部
int32_t y_pos = 0;
for (uint32_t i = 0; i < lv_obj_get_child_count(chat_cont); i++) {
    lv_obj_t* child = lv_obj_get_child(chat_cont, i);
    if (child == wrap) continue;
    if (lv_obj_has_flag(child, LV_OBJ_FLAG_IGNORE_LAYOUT)) continue;
    int32_t bottom = lv_obj_get_y2(child);
    if (bottom > y_pos) y_pos = bottom;
}
y_pos += 4; // gap

lv_obj_set_pos(wrap, x_pos, y_pos);
```

### 11.4 Wine 截图调试方法

在无 GUI 的 Linux 服务器上用 Xvfb + Wine 截图验证 UI：

```bash
# 1. 启动虚拟显示器
Xvfb :97 -screen 0 1400x900x24 &

# 2. 复制 assets 到 bin 目录
cd build-mingw/bin
mkdir -p assets/tray assets/icons/ai
cp ../../assets/*.png assets/
cp ../../assets/tray/*.png assets/tray/
cp ../../assets/icons/ai/*.png assets/icons/ai/

# 3. 运行应用
DISPLAY=:97 WINEPREFIX=/root/.wine WINEDEBUG=-all wine AnyClaw_LVGL.exe &

# 4. 等待渲染完成后截图
sleep 8
xwd -root -display :97 -out /tmp/anyclaw.xwd
convert /tmp/anyclaw.xwd screenshot.png
```

**测试模式**（自动注入消息）：设置环境变量 `ANYCLAW_TEST_MODE=1` 或在 exe 目录创建 `test_mode.txt`，应用会在启动 500ms 后自动添加测试消息。

也可在代码中加临时 timer 注入测试消息：
```c
lv_timer_create([](lv_timer_t* t) {
    chat_add_user_bubble("测试消息");
    chat_add_ai_bubble("AI 回复");
    lv_timer_del(t);
}, 300, nullptr);
```

### 11.5 经验教训

1. **LVGL flex 不支持 per-child cross alignment**：所有子元素共享同一个 `cross_place`，无法让部分子元素左对齐、部分右对齐
2. **`LV_OBJ_FLAG_IGNORE_LAYOUT` 是唯一可靠方案**：需要右对齐的 flex 子元素必须跳出布局，手动定位
3. **手动定位必须在内容填充后调用 `lv_obj_update_layout()`**：否则 `lv_obj_get_width()` 返回 0
4. **`style_align` 在 flex 容器内无效**：flex 算法完全控制子元素位置
5. **Wine 截图调试**：`xwd` + `convert` 替代不可用的截图工具，配合 Xvfb 在 headless 环境下验证 UI

---

## 十二、新环境编译问题记录（2026-04-06）

### 12.0 编译环境

| 项目 | 值 |
|------|-----|
| 服务器 | 阿里云 ECS (Linux 6.8.0-100-generic x86_64) |
| MinGW | GCC 13-win32 |
| CMake | 3.28.3 |
| SDL2 | 2.30.3 (交叉编译到 /opt/sdl2-mingw) |
| 编译产物 | AnyClaw_LVGL.exe (12MB) + SDL2.dll (4.1MB) → zip 6.6MB |

### 12.1 GitHub Clone 网络问题

**现象**：`git clone https://github.com/IwanFlag/AnyClaw_LVGL.git` 反复失败
- 错误1：`GnuTLS recv error (-110): The TLS connection was non-properly terminated`
- 错误2：`RPC failed; curl 56 Recv failure: Connection timed out`
- 错误3：`could not lock config file ... No such file or directory`（目录冲突）

**原因**：阿里云服务器到 GitHub HTTPS 不稳定，大仓库 clone 容易中断

**解决方案**：改用 GitHub API 下载 zip 包（稳定可靠）
```bash
curl -sL -H "Authorization: token {TOKEN}" \
  "https://api.github.com/repos/IwanFlag/AnyClaw_LVGL/zipball/main" \
  -o anyclaw.zip --connect-timeout 30 --max-time 300
unzip -q anyclaw.zip
mv IwanFlag-AnyClaw_LVGL-* AnyClaw_LVGL
```

**对比**：
| 方式 | 成功率 | 耗时 |
|------|--------|------|
| `git clone --depth 1` | ❌ 反复失败 (GnuTLS/curl 56) | >5min 超时 |
| `git clone` + `http.postBuffer=524288000` | ❌ 仍然失败 | >3min 超时 |
| `git clone` + `http.sslVerify=false` | ❌ 仍然失败 | >3min 超时 |
| API zipball 下载 | ✅ 每次成功 | ~3s (60MB) |

**注意**：zip 包不含 `.git` 目录，无法执行 git 操作。如需提交代码，clone 成功后才能用。

### 12.2 CMake configure 路径问题

**现象**：`cmake .. -DCMAKE_TOOLCHAIN_FILE=../tools/...` 报错 `source directory does not appear to contain CMakeLists.txt`

**原因**：cmake 对相对路径 `..` 的解析在某些工作目录下异常（可能因 build 目录嵌套或 cwd 不一致）

**解决方案**：使用绝对路径
```bash
cmake /root/.openclaw/workspace/AnyClaw_LVGL \
  -DCMAKE_TOOLCHAIN_FILE=/root/.openclaw/workspace/AnyClaw_LVGL/tools/linux/toolchain-mingw64.cmake \
  -DSDL2_CROSS_PREFIX=/opt/sdl2-mingw \
  -DCMAKE_BUILD_TYPE=Release
```

### 12.3 子代理超时问题

**现象**：多次使用子代理搭建编译环境，均超时退出（5分钟限制）

**原因**：
1. `apt install` 等系统操作耗时长
2. SDL2 编译（`make -j$(nproc)`）耗时 >2 分钟
3. 主项目编译耗时 >1 分钟
4. 子代理的 token 消耗和通信开销增加了延迟

**建议**：编译环境搭建适合直接在主会话执行，不适合用子代理。子代理更适合代码修改等轻量任务。

### 12.4 仓库目录反复消失

**现象**：子代理执行过程中，`~/.openclaw/workspace/AnyClaw_LVGL/` 目录内容消失（仅剩 `.git`）

**原因**：子代理在执行 `git clone` 或 `rm -rf` 操作时，与其他子代理的文件操作冲突

**解决方案**：避免多个子代理操作同一目录。如果需要并行任务，使用独立的工作目录。

### 12.5 SDL2.dll 未自动复制

**现象**：编译成功后 `build-mingw/bin/` 中只有 `AnyClaw_LVGL.exe`，没有 `SDL2.dll`

**原因**：CMake 配置了链接 SDL2 的 import library (`libSDL2.dll.a`)，但不会自动复制 DLL 到输出目录

**解决方案**：手动复制
```bash
cp /opt/sdl2-mingw/bin/SDL2.dll build-mingw/bin/
```

**建议**：在 `CMakeLists.txt` 中添加 post-build 命令自动复制：
```cmake
add_custom_command(TARGET AnyClaw_LVGL POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${SDL2_CROSS_PREFIX}/bin/SDL2.dll"
    $<TARGET_FILE_DIR:AnyClaw_LVGL>)
```

### 12.6 编译产物大小变化

| 日期 | 版本 | exe | SDL2.dll | zip |
|------|------|-----|---------|-----|
| 2026-04-06 | v2.0.1 | 12MB | 4.1MB | 6.6MB |
| 2026-04-05 (历史) | v2.0.1 | 7.4MB | 3.1MB | 5.2MB |

exe 从 7.4MB 增长到 12MB，可能原因：
- LVGL 版本更新，新增功能（gltf、thorvg、nema_gfx 等大量第三方驱动被编译进去）
- CMakeLists.txt 使用 `GLOB_RECURSE` 编译了 LVGL 全部源码（包括不需要的平台驱动）

**优化建议**：显式列出需要的 LVGL 源文件，排除 NXP/Renesas/NemaGFX/UEFI/QNX/Wayland 等不相关驱动，可显著减小体积。

### 12.7 构建脚本（一键编译）

记录实际可用的一键编译流程，供后续使用：

```bash
#!/bin/bash
set -e
PROJECT=/root/.openclaw/workspace/AnyClaw_LVGL

# 1. 确保 SDL2 已安装
if [ ! -f /opt/sdl2-mingw/lib/libSDL2.dll.a ]; then
    echo "SDL2 not found, building..."
    cd /tmp && rm -rf SDL2-src SDL2-build
    curl -sL https://github.com/libsdl-org/SDL/releases/download/release-2.30.3/SDL2-2.30.3.tar.gz | tar xz
    mv SDL2-2.30.3 SDL2-src
    mkdir SDL2-build && cd SDL2-build
    cmake ../SDL2-src \
      -DCMAKE_SYSTEM_NAME=Windows \
      -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
      -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
      -DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres \
      -DCMAKE_INSTALL_PREFIX=/opt/sdl2-mingw \
      -DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TEST=OFF
    make -j$(nproc) && sudo make install
fi

# 2. 编译项目
cd $PROJECT && rm -rf build-mingw && mkdir build-mingw && cd build-mingw
cmake $PROJECT \
  -DCMAKE_TOOLCHAIN_FILE=$PROJECT/tools/linux/toolchain-mingw64.cmake \
  -DSDL2_CROSS_PREFIX=/opt/sdl2-mingw \
  -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)

# 3. 复制 SDL2.dll
cp /opt/sdl2-mingw/bin/SDL2.dll bin/

# 4. 打包
cd bin
VERSION="v2.0.1"
TS=$(date '+%Y%m%d_%H%M')
ZIP="AnyClaw_LVGL_${VERSION}_${TS}.zip"
PKG="_pkg"
mkdir -p "$PKG/assets/tray" "$PKG/assets/icons/ai"
cp AnyClaw_LVGL.exe SDL2.dll "$PKG/"
cp $PROJECT/assets/app_icon.png $PROJECT/assets/garlic_icon.png $PROJECT/assets/garlic_32.png "$PKG/assets/"
cp $PROJECT/assets/tray/*.png "$PKG/assets/tray/"
cp $PROJECT/assets/icons/ai/*.png "$PKG/assets/icons/ai/"
cd "$PKG" && zip -r "../$ZIP" . && cd .. && rm -rf "$PKG"
echo "✅ $ZIP ($(du -h "$ZIP" | cut -f1))"
```

### 12.8 DPI 窗口超出屏幕

**现象**：窗口高度超出真实屏幕（日志显示 `System DPI scale: 200%`，screen 2560x1600，window 2480x1440）

**根因**：`max_h = screen_h - SCALE(100)` 中 SCALE(100) 在 200% DPI 时变成 200。但 `GetSystemMetrics` 在 DPI-aware 模式下已返回物理像素，SCALE() 导致 margin 计算偏大，实际窗口尺寸反而超限。

**修复**：`max_w`/`max_h` 去掉 SCALE()，直接用固定物理像素值
```cpp
int max_w = screen_w - 60;   /* 固定 margin，不用 SCALE */
int max_h = screen_h - 80;
```

**提交**: `7fc9fb4`

### 12.9 SCALE() 双重放大导致面板截断

**现象**：200% DPI 下左面板 240→480px，聊天区域被挤压，右侧内容截断

**根因**：`SCALE(px) = px * dpi_scale / 100`。app 已声明 DPI-aware，display 工作在物理像素，所有布局已经是物理像素尺寸。SCALE() 再乘一遍 DPI 系数 = 双重放大。

**修复**：SCALE() 改为直接返回 px
```cpp
inline int SCALE(int px) { return px; }  /* DPI-aware: no double-scaling */
```

**原因分析**：SCALE() 是在非 DPI-aware 模式下设计的（Windows 自动缩放 + 手动补偿）。改为 DPI-aware 后，display 已经是物理像素，SCALE() 应该移除。

**提交**: `6b5618c`

### 12.10 主题切换不完整

**现象**：下拉框切换主题后，首页部分字体颜色未变，设置页控件背景保持原色

**根因**：
- `ui_settings_apply_theme()` 只更新面板背景和 tab 按钮，不递归更新内部控件
- `ui_settings.cpp` 有 77 处硬编码颜色 `lv_color_make(r,g,b)`，不跟随 `g_colors`

**修复**：
1. 新增 `apply_theme_recursive()` 递归遍历子控件，按类型（label/textarea/dropdown/switch/divider）应用 `g_colors` 对应色
2. `ui_settings_apply_theme()` 调用递归函数更新全部 tab 内容
3. `apply_theme_to_all()` 增加左侧面板 label 颜色更新

**待优化**：77 处硬编码颜色应全部替换为 `g_colors->*` 引用，目前递归函数只能处理部分场景

**提交**: `48f2298`

### 12.11 HTTP 客户端 SSE 流式问题（AI 不回复）

**现象**：POST `/v1/chat/completions` 后无响应，日志无 HTTP 状态码

**根因**：
1. 缺少 `Accept: text/event-stream` 请求头，Gateway 可能不启用流式响应
2. `WINHTTP_ACCESS_TYPE_DEFAULT_PROXY` 对 localhost 走代理检测，延迟或失败
3. 无响应头日志，出问题无法排查

**修复**（`http_client.cpp`）：
1. SSE 请求增加 `Accept: text/event-stream` 头
2. 改用 `WINHTTP_ACCESS_TYPE_NO_PROXY` 跳过代理
3. 增加 `WinHttpSendRequest` / `WinHttpReceiveResponse` 错误日志
4. 增加响应头日志输出

**提交**: `92fd609`

### 12.12 Gateway 404 自动修复 + 备份回滚

**现象**：`/v1/chat/completions` 返回 404，Gateway HTTP API 未开启

**修复**（`openclaw_mgr.cpp` → `app_enable_chat_endpoint()`）：
1. 备份 `openclaw.json` → `.bak`
2. `openclaw config set gateway.http.endpoints.chatCompletions.enabled true`
3. 停止 Gateway → **轮询端口直到释放**（避免双进程）
4. 启动 Gateway → **轮询 health 直到就绪**
5. POST `/v1/chat/completions` 测试
   - ✅ 通过 → 删备份
   - ❌ 失败 → 恢复 .bak，重启 Gateway

**提交**: `339dfd4`, `581e016`

### 12.13 Backspace 双重删除

**现象**：按一次 Backspace 删除两个字符

**根因**：`SDL_AddEventWatch` 的返回值被 SDL 忽略（watch 是观察者，不能拦截事件）。显式删除一次 + LVGL 默认处理一次 = 双删除。

**修复**：移除显式 Backspace 处理，让 LVGL 原生处理。Delete 键保留显式处理（LVGL 不处理 SDL Delete）。

**提交**: `24c945c`

### 12.14 AI 回复中文乱码

**现象**：AI 回复中的中文显示为乱码

**根因**：
1. SSE `\uXXXX` 解码只处理 BMP（3 字节 UTF-8），不支持代理对（4 字节 UTF-8）
2. `g_stream_buffer` 8KB 不够长回复
3. buffer 满时截断可能切在多字节字符中间

**修复**（`ui_main.cpp`）：
1. `\uXXXX` 解码增加代理对检测（`\uD8xx\uDCxx` → 4 字节 UTF-8）
2. `g_stream_buffer` 8KB → 16KB
3. buffer 满时回退到 UTF-8 字符边界再截断

**提交**: `92fd609`

### 12.15 窗口放大 + 聊天字体缩小

**修改**：
- 窗口默认大小 60% → 75% 屏幕（`main.cpp`）
- 新增 `CJK_FONT_CHAT`（13px = 16px 的 80%），用于聊天气泡和输入框
- 主界面按钮/标题等保持 `CJK_FONT`（16px）

**提交**: `92fd609`

### 12.16 About 页经典黑主题棕色字体

**现象**：经典黑主题下 "Version v2.0.1" 显示为棕色/金色

**根因**：硬编码 `lv_color_make(255, 200, 100)` 金色

**修复**：改为 `c->text`（跟随主题色）

**提交**: `92fd609`

### 12.17 Authorization Header 双重 Bearer 前缀（401 Unauthorized）

**现象**：AnyClaw 发送聊天请求，Gateway 返回 401 Unauthorized。curl 用相同 token + model 直连 Gateway 正常。

**根因**：`Bearer` 前缀被拼接了两次：
- `ui_main.cpp` 中 `snprintf(auth_header, "Bearer %s", gw_token)` → 传给 `http_post_stream`
- `http_client.cpp` 中 `snprintf(headers, "Authorization: Bearer %s", api_key)` → 最终 header 变为 `Authorization: Bearer Bearer <token>`

Gateway 解析到 `Bearer Bearer <token>` 无法识别，返回 401。

**排查过程**：
1. curl 直连 Gateway `/health` → 200（Gateway 正常）
2. curl 直连 `/v1/chat/completions` + 同 token → SSE 正常（token 有效）
3. 通过 debug 日志打印 `raw_headers` 确认：`Authorization: Bearer Bearer 6cfc8fa7...`

**修复**：
- `ui_main.cpp`：`auth_header` 改为只传原始 token（去掉 `"Bearer "` 前缀），由 `http_client.cpp` 统一添加

**相关文件**：`src/ui_main.cpp`、`src/http_client.cpp`

**提交**: `6c90ecf`

### 12.18 模型列表不显示 openclaw.json 已配置模型

**现象**：用户在 OpenClaw 中配置了多个 provider models（如 xiaomi/mimo-v2-pro），但 AnyClaw 下拉列表只有硬编码的 21 个默认模型。

**根因**：`model_manager_init()` 只加载硬编码默认列表 + `custom_add_models.json`，不读取 openclaw.json 的 `models.providers.*.models` 配置。

**修复**：
- `openclaw_mgr.cpp`：新增 `app_get_all_models()` — 调用 `openclaw config get models` 解析全部 provider models
- `model_manager.cpp`：新增 `model_load_from_openclaw_config()` — 在 `model_manager_init()` 中调用，自动合并到模型列表

**提交**: `91ef187`

### 12.19 app_update_model_config 无验证无备份

**现象**：`app_update_model_config()` 直接调用 `openclaw config set` 修改 openclaw.json，无格式验证、无备份、无回滚。如果文件损坏或命令执行失败，配置可能丢失。

**修复**：
1. 验证：执行前检查 openclaw.json 是否可读、是否包含 `gateway`/`models` 关键字
2. 备份：复制为 `openclaw.json.bak`
3. 回滚：如果任何 `config set` 命令失败，恢复 .bak 并重启 Gateway

**提交**: `91ef187`

### 12.20 API Key 显示为 __OPENCLAW_REDACTED__

**现象**：`openclaw config get models.providers.xxx.apiKey` 返回 `__OPENCLAW_REDACTED__`，AnyClaw 无法显示真实 API Key，用户无法确认 key 是否正确配置。

**根因**：OpenClaw CLI 对敏感字段自动脱敏。

**修复**：`app_get_provider_api_key()` 改为直接读取 openclaw.json 文件（同 `read_gateway_token()` 方式），绕过 CLI 脱敏。仅在文件读取失败时回退到 CLI。

**提交**: `3e48c12`

### 12.21 API Key 验证按钮

**新增功能**：设置页 Model Tab 新增 "Verify" 按钮，后台线程 curl 测试 API Key 有效性：
- HTTP 200 → ✅ key valid
- HTTP 401/403 → ❌ key invalid
- 其他 → ⚠️ network issue

**提交**: `3e48c12`

### 12.22 退出时关闭 OpenClaw 开关

**新增功能**：设置 → 通用页新增 "退出时关闭 OpenClaw" 开关，默认开启。
- 开启：AnyClaw 退出时自动调用 `app_stop_gateway()`
- 关闭：AnyClaw 退出不影响 OpenClaw 运行

配置持久化到 `config.json` 的 `close_gateway_on_exit` 字段。

**提交**: `74ad980`

### 12.23 修复 Backspace 无法删除（返回值问题）

**现象**：输入框按 Backspace 无法删除字符（Wine 环境下）。

**根因**：commit `24c945c` 移除了 backspace 显式处理，依赖 LVGL 原生。但 Wine 下 SDL 事件传递不可靠，backspace 事件到不了 LVGL。同时事件 watch 的 `return 0` 导致 LVGL 也处理一次（双重删除）。

**修复**：
1. 恢复 backspace 显式处理（`lv_textarea_delete_char`）
2. 所有 key 事件处理改为 `return 1`（消费事件，不让 LVGL 二次处理）
3. 去掉错误注释 "SDL_AddEventWatch 返回值被忽略"

**提交**: `75a1f9b`

### 12.24 聊天区文字可选中复制 & 图标放大 2x

**新增功能**：
- 用户气泡、AI 气泡、流式输出 label 均加 `make_label_selectable()`，支持鼠标拖拽选中 + Ctrl+C
- 托盘图标 48px → 96px，窗口标题栏图标 48/32/16 → 96/64/32

**提交**: `7ad3515`

### 12.25 剪贴板中文乱码

**现象**：选中聊天区文字 Ctrl+C 复制后，粘贴出来是乱码（中文）。

**根因**：`clipboard_copy_to_win()` 使用 `CF_TEXT`（ANSI 编码），不支持 UTF-8 中文。

**修复**：改用 `CF_UNICODETEXT`：
- 复制：UTF-8 → UTF-16（`MultiByteToWideChar`）
- 粘贴：UTF-16 → UTF-8（`WideCharToMultiByte`）

**提交**: `b1c7698`

### 12.26 贴边大蒜头动画（初版）

**新增功能**：拖拽窗口到屏幕边缘松开 → 隐藏主窗口 → 浮动大蒜头 → 悬停葱头摇摆 → 点击恢复。

初版使用 SDL 子线程实现，存在线程安全问题。

**提交**: `b1c7698`

### 12.27 光标跳转 bug

**现象**：输入框光标会突然跳到第二个字符末尾，方向键有时无法控制光标。

**根因**：`lv_textarea_get_text_selection()` 返回的是"是否启用选中功能"（`text_sel_en`），不是"是否有实际选区"。代码误判为有选区并执行删除/替换操作。

无实际选区时 `sel_end = 0xFFFF`（`LV_LABEL_TEXT_SELECTION_OFF`），代码判断 `0 < 65535` 为有选区 → `lv_textarea_set_cursor_pos(0)` + 删除文字。

**修复**：selection replacement 块增加实际选区校验：
- `sel_start` 和 `sel_end` 必须在文本长度范围内
- 两者都不能是 `LV_LABEL_TEXT_SELECTION_OFF`

**提交**: `3d203b0`

### 12.28 大蒜 dock 重写为纯 Win32

**现象**：大蒜头悬停动画后几秒卡死。

**根因**：初版使用 SDL 子线程创建窗口+渲染，和主线程 SDL 事件循环冲突（SDL 非线程安全），悬停时触发竞态条件。

**修复**：完全重写为纯 Win32 实现：
- `CreateWindowExA` + `WS_EX_LAYERED` 创建透明浮动窗口
- WIC 加载 PNG 为 `HBITMAP`（不依赖 SDL_image）
- GDI `StretchBlt` / `PlgBlt` 渲染（葱头摇摆用 `PlgBlt` 仿射变换）
- Win32 Timer 驱动 30fps 动画 + 100ms 悬停检测
- 无多线程，所有操作在主线程消息循环中

**提交**: `fbc982f`

### 12.29 窗口图标加载失败

**现象**：`app_icon.png` 512x512 无法从资源加载（err=1813），标题栏无图标。

**状态**：未修复。资源嵌入方式待调整，文件 fallback 路径的 GDI+ `Bitmap::FromFile` 返回 status=0 但日志报失败（疑似误报）。兜底通过 `tray_set_window_icon()` 从文件加载。

**相关提交**: `7ad3515`（图标尺寸放大）

### 12.30 事件watch默认return值错误（双重删除）

**现象**：Backspace/Delete按一次删除两个字符。

**根因**：`SDL_AddEventWatch`回调底部默认`return 1`（从队列移除事件），但注释写的是"保留事件"。所有未被if分支匹配的事件都被错误消费，LVGL仍然通过`SDL_PollEvent`处理了一次→双重效果。

**修复**：`return 1` → `return 0`（保留事件在队列中供LVGL处理）。

**提交**: `760f955`

### 12.31 剪贴板复制使用ANSI编码

**现象**：textarea Ctrl+C复制中文后粘贴为乱码。

**根因**：`SDL_SetClipboardText()`在Windows上使用`CF_TEXT`（ANSI编码），不支持UTF-8中文。

**修复**：textarea Ctrl+C/X改用`clipboard_copy_to_win()`（`CF_UNICODETEXT` UTF-16）。

**提交**: `760f955`

### 12.32 聊天气泡选中颜色不统一

**现象**：聊天输入框选中文字为淡蓝色，聊天气泡label选中无高亮色。

**修复**：`make_label_selectable()`添加`LV_PART_SELECTED`样式：背景色(180,215,255)，文字色(30,30,40)。

**提交**: `760f955`

### 12.33 SSE流式UI卡死

**现象**：发送消息后UI卡住，新消息无法发送。Gateway SSE连接阻塞在`WinHttpReadData`。

**根因**：`g_streaming=true`永不重置，后续消息被`if (g_streaming) return;`拦截。

**修复**：stream_timer_cb添加超时看门狗：30s无新数据或总耗时>45s，强制`InterlockedExchange(g_stream_done, 1)`。

**提交**: `73117ad`

### 12.34 大蒜dock全面修复

**问题序列与修复**：

| 问题 | 修复 | 提交 |
|------|------|------|
| SDL子线程创建窗口卡死 | 重写为纯Win32（CreateWindowExA + WIC） | `fbc982f` |
| 二次拖拽到边缘不触发 | WM_EXITSIZEMOVE已dock时改为restore | `b273c61` |
| 大蒜窗口丢失无法恢复 | WM_ACTIVATE自动restore | `b273c61` |
| 大蒜跑到屏幕外 | position_dock增加边界clamp | `b273c61` |
| PNG加载失败(WIC) | 改用stb_image（LVGL自带） | `3be4575` |
| DPI坐标不一致 | SDL坐标→Win32 GetWindowRect | `73117ad` |

**最终实现**：
- 纯Win32，无SDL子线程
- stb_image加载PNG（Wine兼容）
- 四边吸附 + 悬停葱头摇摆 + 点击恢复 + WM_ACTIVATE安全恢复

### 12.35 Emoji渲染限制

**现象**：AI回复中的飞书表情（小黄脸emoji）显示为乱码。

**根因**：LVGL的tiny_ttf底层使用stb_truetype，不支持COLR/CPAL彩色emoji字体表。即使加载`seguiemj.ttf`也无法渲染。

**当前方案**：SSE解码时过滤emoji代码点(U+1F300-U+1F9FF)，不再显示乱码。

**未来方案**：启用`LV_USE_FREETYPE` + 交叉编译FreeType for MinGW。

**提交**: `7d76a0a`, `d2ebb87`

### 12.36 任务列表Session计数

**新增功能**：
- "Task List"标题旁显示`(N)`计数badge
- Gateway通过`sessions.list`获取活跃会话数
- Busy状态列出每个session的渠道来源

**提交**: `852d717`

