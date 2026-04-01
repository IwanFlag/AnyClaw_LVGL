# AnyClaw LVGL — P0 功能实现报告

## 实现日期
2026-04-01

## 已完成的功能模块

### 1. 系统托盘 (`src/tray.h` + `src/tray.cpp`)
**状态：✅ 已完成，编译通过，运行测试通过**

实现内容：
- `tray_init(HINSTANCE)` — 注册隐藏窗口类 `AnyClaw_TrayWnd`，创建消息窗口（`HWND_MESSAGE`），添加 Shell_NotifyIcon 托盘图标
- `tray_cleanup()` — 删除托盘图标，释放资源
- `tray_set_state(TrayState)` — 更新图标颜色（Gray/Green/Yellow/Red），通过 `CreateIconIndirect` 程序化生成圆形图标
- `tray_balloon(title, message)` — 弹出气泡通知
- `tray_process_messages()` — 非阻塞处理 Windows 消息（`PeekMessage`）
- `tray_should_quit()` — 退出菜单项触发退出标志
- `tray_show_window()` — 通过 `SDL_GetWindowWMInfo` 获取 HWND 显示/隐藏窗口

右键菜单包含：
- 状态显示（灰色禁用）
- 打开设置、重启 OpenClaw、查看日志
- 开机自启复选框（读取注册表实时状态）
- 关于、退出

双击托盘图标恢复窗口。关闭按钮最小化到托盘。

### 2. 健康监控 (`src/health.h` + `src/health.cpp`)
**状态：✅ 已完成，编译通过**

实现内容：
- `health_start()` — 启动后台监控线程（`_beginthreadex`），每 10 秒巡检
- `health_stop()` — 停止监控线程，等待退出
- `health_set_callback()` — 状态变化回调

状态判定逻辑：
- `is_node_running()` — `CreateToolhelp32Snapshot` 枚举进程查找 `node.exe`
- `check_http_health()` — HTTP GET `http://127.0.0.1:18789/health`（3秒超时）
- 进程不存在 + HTTP 无响应 → `NotRunning` → 自动重启一次（调用 `app_start_gateway()`）
- 进程存在 + HTTP 连续失败 3 次 → `Error` → 图标变红
- 进程存在 + HTTP 正常 → `Running` → 图标变绿

状态变化自动更新托盘图标颜色。

### 3. 开机自启 (`src/autostart.cpp`)
**状态：✅ 已完成，编译通过**

实现内容：
- `is_auto_start()` — 读取注册表 `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` 下的 `AnyClaw_LVGL` 值
- `set_auto_start(bool)` — 注册/删除开机自启项，路径为当前 exe 路径（带引号）

### 4. 主程序集成 (`src/main.cpp`)
**状态：✅ 已修改**

修改内容：
- 启动时调用 `tray_init()`
- 主循环改为 `while (!tray_should_quit())`，内含 `tray_process_messages()`
- 添加 `SDL_PollEvent` 拦截 `SDL_QUIT`，实现关闭按钮最小化到托盘
- 启动健康监控 `health_start()`，初始状态设为 Yellow
- 退出时调用 `health_stop()` + `tray_cleanup()`

### 5. 构建配置 (`CMakeLists.txt`)
**状态：✅ 已更新**

新增源文件：
- `src/tray.cpp`
- `src/health.cpp`
- `src/autostart.cpp`

### 6. 接口声明 (`src/app.h`)
**状态：✅ 已更新**

新增接口：
- `bool is_auto_start()`
- `void set_auto_start(bool enable)`

## 编译结果
- **编译：✅ 通过**（使用 VS Build Tools 14.50 + CMake + NMake）
- **运行测试：✅ 通过**（进程启动正常，PID 41152，内存约 114MB）

## 文件清单

| 文件 | 操作 | 说明 |
|------|------|------|
| `src/tray.h` | 新建 | 系统托盘头文件 |
| `src/tray.cpp` | 新建 | 系统托盘实现 |
| `src/health.h` | 新建 | 健康监控头文件 |
| `src/health.cpp` | 新建 | 健康监控实现 |
| `src/autostart.cpp` | 新建 | 开机自启实现 |
| `src/main.cpp` | 修改 | 集成托盘 + 健康监控 |
| `src/app.h` | 修改 | 添加 auto-start 接口 |
| `CMakeLists.txt` | 修改 | 添加新源文件 |

## 注意事项
- 不测试停止 Gateway 功能（按约束要求）
- GUI 效果（托盘图标、右键菜单、气泡通知）需要目视确认
- `tray_show_window()` 使用 `SDL_GetWindowWMInfo` 获取 HWND，兼容 SDL2 的窗口管理
- 健康监控线程在 `app_start_gateway()` 失败后会记录 `g_autoRestarted = true`，避免重复重启
