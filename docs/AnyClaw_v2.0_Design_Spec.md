# AnyClaw v2.0.9 — 设计规格文档

> 版本：v2.0.9 | 创建：2026-04-11
> 本文档为 v2.0 所有待实现功能的详细设计，覆盖需求细节、UI 框架、数据流和代码架构。

---

## 总览

| # | 功能编号 | 名称 | 状态 |
|---|---------|------|------|
| 1 | MM-01 | 多模态输入与文件上传 | 🔧 完善中 |
| 2 | GEMMA-01 | 本地模型（Gemma 4） | 🔧 完善中 |
| 3 | CTRL-01 | UI 控制权模式 | 🔧 完善中 |
| 4 | WS-01 | 工作区管理 | 🔧 完善中 |
| 5 | PERM-01 | 权限系统 | 🔧 完善中 |
| 6 | TM-01 | 会话与任务管理 | 🔧 完善中 |
| 7 | AIMODE-01 | AI 交互模式 | 🆕 设计中 |
| 8 | PROACTIVE-01 | Agent 主动行为 | 🆕 设计中 |
| 9 | WORK-01 | Work 模式 | 🆕 设计中 |
| 10 | LIC-01 | License 授权系统 | 🆕 设计中 |
| 11 | U-01 | 更新机制 | 🆕 设计中 |

---

## 1. MM-01 — 多模态输入与文件上传

### 1.1 需求细节

用户在聊天中发送文件/图片/目录给 AI Agent，Agent 能看到附件内容并据此回复。

**支持的附件类型：**

| 类型 | 格式 | AI 处理方式 |
|------|------|-----------|
| 图片 | PNG/JPG/GIF/BMP/WEBP | 视觉理解（通过 Omni 多模态模型） |
| 文本文件 | TXT/MD/JSON/YAML/XML/CSV | 直接读取内容拼入上下文 |
| 代码文件 | CPP/H/PY/JS/TS/HTML/CSS 等 | 直接读取内容拼入上下文 |
| 文档 | PDF/DOCX（如有解析能力） | 解析后拼入上下文 |
| 目录 | 任意目录 | 列出目录结构，用户可指定读取哪些文件 |

**附件大小限制：**
- 单文件 ≤ 10MB（防止大文件撑爆上下文窗口）
- 单次最多附 5 个文件
- 目录列出不限深度，但只显示文件名和大小，不自动读取内容

**UI 交互流程：**

```
用户点击 [📎] 按钮
  → 弹出菜单 [File] [Dir]
  → 用户选择 File → Win32 GetOpenFileNameA 对话框
  → 用户选择 Dir → Win32 SHBrowseForFolderA 对话框
  → 选中的文件/目录显示为输入区上方的附件卡片
  → 附件卡片：类型图标 + 文件名 + 大小 + [×] 删除按钮
  → 用户输入文字消息（可选），点击发送
  → 附件卡片 + 文字消息一起发送
```

### 1.2 UI 框架

**附件卡片组件（Attachment Card）：**

```
┌──────────────────────────────┐
│  📄 report.pdf    2.3MB  [×] │
└──────────────────────────────┘
```

- 位于输入框上方，水平排列
- 每个卡片可独立删除
- 图片附件显示缩略图（64×64）
- 发送后卡片变为聊天气泡中的附件预览

**聊天中的附件气泡：**

```
┌─────────────────────────────┐
│  📄 report.pdf · 2.3MB      │  ← 用户消息上方的附件展示
│  [点击查看]                  │
├─────────────────────────────┤
│  帮我总结这个报告的要点      │  ← 用户文字消息
└─────────────────────────────┘
```

- 附件在气泡顶部，文字在下方
- 点击附件可直接打开（文件用系统默认程序，目录打开资源管理器）

### 1.3 数据流

```
用户选择文件
  → AnyClaw 读取文件路径
  → 权限系统校验（§PERM-01）：路径在工作区内？外部文件需询问？
  → 通过校验后：
    → 图片：base64 编码 → OpenClaw Gateway /v1/chat/completions（多模态格式）
    → 文本/代码：读取文件内容 → 拼入 messages[].content
    → 目录：列出文件树结构 → 拼入 messages[].content
  → Gateway → Agent → LLM 处理
  → LLM 回复（流式）
  → AnyClaw 渲染回复
```

**Gateway API 调用格式（多模态消息）：**

```json
{
  "model": "openclaw:main",
  "messages": [
    {
      "role": "user",
      "content": [
        {
          "type": "text",
          "text": "帮我看看这张图"
        },
        {
          "type": "image_url",
          "image_url": {
            "url": "data:image/png;base64,iVBOR..."
          }
        }
      ]
    }
  ],
  "stream": true
}
```

### 1.4 代码设计

**新增文件：**
- `src/attachment.h` — 附件数据结构定义
- `src/attachment.cpp` — 文件读取、base64 编码、缩略图生成

**核心数据结构：**

```c
typedef enum {
    ATTACH_FILE,    // 普通文件
    ATTACH_IMAGE,   // 图片
    ATTACH_DIR,     // 目录
} AttachType;

typedef struct {
    AttachType type;
    char path[MAX_PATH];         // 完整路径
    char name[256];              // 文件名
    uint64_t size;               // 文件大小
    char *content;               // 文件内容（文本类）
    char *base64_data;           // base64 编码（图片类）
    char thumbnail_path[260];    // 缩略图路径（图片类）
    lv_obj_t *card_widget;      // UI 卡片控件
} Attachment;
```

**权限校验流程：**

```c
// attachment.cpp
bool attachment_check_permission(const char *path, PermissionMode mode) {
    // 1. 检查路径是否在工作区内
    if (is_inside_workspace(path)) {
        return (mode == PERMISSIVE); // 宽松模式直接允许，严格模式询问
    }
    // 2. 外部路径 → 弹窗询问
    return show_permission_dialog("读取外部文件", path);
}
```

---

## 2. GEMMA-01 — 本地模型（Gemma 4）

### 2.1 需求细节

用户在 AnyClaw 中下载、安装并运行 Gemma 4 本地模型，不依赖外部服务。

**支持模型：**

| 型号 | 文件大小（Q4） | 最低 RAM | 推荐 RAM |
|------|---------------|---------|---------|
| Gemma 4 E4B | ~2.5GB | 8GB | 8GB |
| Gemma 4 26B MoE | ~15GB | 16GB | 16GB |

**推理引擎：** llama.cpp `llama-server.exe`，后台隐藏进程，本地 HTTP 端口通信。

**安装流程：**

```
用户在 Settings → Model → 本地模型 开启安装
  → AnyClaw 检测 RAM 大小，自动推荐模型
  → 用户勾选要安装的模型（E4B / 26B MoE）
  → 下载 GGUF 模型文件（三源自动回退）
  → 下载完成 → 校验文件完整性
  → 检测/下载 llama-server.exe（如不存在）
  → 安装完成，状态显示 ✅
```

**推理流程：**

```
用户发送消息
  → AnyClaw 检查当前模型配置
  → 如果是本地模型：
    → 检查 llama-server.exe 是否在运行
    → 未运行 → 启动 llama-server.exe（CREATE_NO_WINDOW）
    → POST http://127.0.0.1:{port}/v1/chat/completions（OpenAI 兼容格式）
    → SSE 流式接收回复
  → 如果是云端模型：
    → POST http://127.0.0.1:18789/v1/chat/completions（Gateway）
```

**引擎管理：**

| 操作 | 方式 |
|------|------|
| 启动 | `CreateProcess` + `CREATE_NO_WINDOW`，指定端口和模型路径 |
| 健康检查 | HTTP GET `http://127.0.0.1:{port}/health` |
| 停止 | `TerminateProcess` 或等用户退出 AnyClaw |
| 端口 | 默认 18800（避免与 Gateway 18789 冲突），可配置 |

### 2.2 UI 框架

**Settings → Model → 本地模型区域：**

```
┌─ 本地模型 ──────────────────────────────────┐
│                                              │
│  [开关] 启用本地推理                          │
│                                              │
│  ┌─ 可用模型 ───────────────────────────┐   │
│  │  ○ Gemma 4 E4B    2.5GB  推荐 ✅    │   │
│  │    适用于 8GB+ RAM                    │   │
│  │    状态: 已安装 ✅ / 未安装 [下载]    │   │
│  │                                       │   │
│  │  ○ Gemma 4 26B MoE  15GB             │   │
│  │    适用于 16GB+ RAM                   │   │
│  │    状态: 未安装 [下载]                │   │
│  └───────────────────────────────────────┘   │
│                                              │
│  推理引擎: llama-server.exe                  │
│  状态: 🟢 运行中 (端口 18800)               │
│  [停止引擎]                                  │
│                                              │
└──────────────────────────────────────────────┘
```

**下载进度：**

```
┌─ 下载 Gemma 4 E4B ──────────────────────────┐
│  ████████████████░░░░░░░░  68%              │
│  1.7 GB / 2.5 GB  ·  12.3 MB/s             │
│  预计剩余: 1 分钟                            │
│                                              │
│  [取消下载]                                  │
└──────────────────────────────────────────────┘
```

### 2.3 数据流

```
                    AnyClaw UI
                        │
            ┌───────────┴───────────┐
            │                       │
      云端模型路径             本地模型路径
            │                       │
  Gateway :18789           llama-server :18800
            │                       │
     OpenClaw Agent          llama.cpp 推理
            │                       │
      LLM API 云端             GGUF 模型文件
```

**模型切换逻辑：**

```c
// 在 chat_send_message() 中
void chat_route_request(const char *model_name) {
    if (is_local_model(model_name)) {
        // 本地模型 → 直连 llama-server
        ensure_llama_running();  // 确保引擎启动
        http_post_sse("http://127.0.0.1:18800/v1/chat/completions",
                       build_chat_payload(model_name));
    } else {
        // 云端模型 → 走 Gateway
        http_post_sse("http://127.0.0.1:18789/v1/chat/completions",
                       build_chat_payload("openclaw:main"));
    }
}
```

### 2.4 代码设计

**新增文件：**
- `src/llama_engine.h` / `.cpp` — llama-server 进程管理
- `src/model_local.h` / `.cpp` — 本地模型下载、安装、管理

**引擎管理核心：**

```c
typedef struct {
    PROCESS_INFORMATION proc_info;
    char model_path[MAX_PATH];    // GGUF 文件路径
    int port;                     // HTTP 端口，默认 18800
    bool running;
} LlamaEngine;

// 启动引擎
bool llama_engine_start(LlamaEngine *engine) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
        "llama-server.exe -m \"%s\" --port %d --host 127.0.0.1 -ngl 0",
        engine->model_path, engine->port);

    STARTUPINFOA si = { .cb = sizeof(si), .dwFlags = STARTF_USESHOWWINDOW,
                        .wShowWindow = SW_HIDE };
    CreateProcessA(NULL, cmd, NULL, NULL, FALSE,
                   CREATE_NO_WINDOW, NULL, NULL, &si, &engine->proc_info);
    engine->running = true;
    return true;
}

// 健康检查
bool llama_engine_healthy(LlamaEngine *engine) {
    // GET http://127.0.0.1:{port}/health
    return http_get_status(engine->port, "/health") == 200;
}

// 停止引擎
void llama_engine_stop(LlamaEngine *engine) {
    TerminateProcess(engine->proc_info.hProcess, 0);
    engine->running = false;
}
```

**下载管理（三源自动回退）：**

```c
// 模型下载源优先级
static const char *MIRROR_SOURCES[] = {
    "https://huggingface.co/google/gemma-4-E4B-GGUF/resolve/main/",
    "https://hf-mirror.com/google/gemma-4-E4B-GGUF/resolve/main/",
    "https://gitee.com/anyclaw/models/raw/main/gemma4/",
    NULL
};

// 下载线程
DWORD WINAPI model_download_thread(LPVOID param) {
    DownloadTask *task = (DownloadTask *)param;
    for (int i = 0; MIRROR_SOURCES[i]; i++) {
        if (try_download(MIRROR_SOURCES[i], task->filename, task->dest)) {
            if (verify_checksum(task->dest)) {
                task->status = DL_COMPLETE;
                return 0;
            }
        }
        // 当前源失败，尝试下一个
        task->status = DL_SWITCHING_MIRROR;
    }
    task->status = DL_FAILED;
    return 0;
}
```

---

## 3. CTRL-01 — UI 控制权模式（User / AI）

### 3.1 需求细节

AnyClaw UI 控制权决定**谁在操作 AnyClaw 的界面**。

| 模式 | 操作者 | 含义 |
|------|--------|------|
| **用户模式**（默认） | 人类用户 | 用户手动操作鼠标键盘，AI 仅建议 |
| **AI 模式** | Agent | Agent 模拟用户操作鼠标键盘来控制 AnyClaw UI |

**核心约束：** AI 拿到 UI 控制权后，必须模拟真实用户的鼠标键盘操作来操控 AnyClaw，没有捷径 API。AI 看到的界面和用户一样，操作方式也一样。

**模式切换：**
- 切换入口：Work 面板顶部显式切换按钮
- 默认：用户模式
- 切换即时生效
- 持久化到 config.json

**安全约束（AI 模式下）：**
- 所有高风险动作仍走权限系统
- 用户可随时一键回退到用户模式（全局热键或显眼按钮）
- AI 模式下界面有持续视觉提示（如边框高亮颜色变化）

### 3.2 UI 框架

**Work 面板顶部控制栏：**

```
┌─ Work ──────────────────────────────────────┐
│  [Chat] [Work]      ⚙️                       │
│                                              │
│  控制模式: [👤 用户] [🤖 AI]                 │
│  当前: 👤 用户模式                            │
│                                              │
│  ...Work 面板内容...                         │
└──────────────────────────────────────────────┘
```

**AI 模式下的视觉提示：**

```
┌─ Work ──── 🤖 AI 控制中 ─────────────────────┐  ← 顶栏变色
│  [Chat] [Work]      ⚙️     [👤 立即接管]      │  ← 一键回退按钮
│                                              │
│  ...AI 操作中...                             │
└──────────────────────────────────────────────┘
```

- 顶栏背景色切换为橙色/黄色系
- 标题栏显示 "🤖 AI 控制中"
- 显示 "👤 立即接管" 按钮，一键切回用户模式
- 托盘图标颜色变化（黄色 → AI 正在操作）

### 3.3 数据流

```
用户点击 [🤖 AI] 按钮
  → AnyClaw 设置 g_control_mode = CTRL_AI
  → 写入 config.json
  → 通知 Gateway（通过 systemEvent 或 Agent 感知）
  → Agent 获得 UI 操作权限

Agent 操作 UI:
  → Agent 发出操作意图（如"点击设置按钮"）
  → Gateway → AnyClaw SSE 通道
  → AnyClaw 接收操作指令
  → 权限系统校验（PERM-01）
  → 执行 Win32 操作（mouse_event / SendInput）
  → UI 状态变化反馈给 Agent

用户点击 [👤 立即接管]:
  → AnyClaw 设置 g_control_mode = CTRL_USER
  → 写入 config.json
  → 停止 Agent 的 UI 操作队列
  → 恢复用户控制
```

### 3.4 代码设计

**核心枚举和状态：**

```c
typedef enum {
    CTRL_USER = 0,  // 用户控制
    CTRL_AI = 1,    // AI 控制
} ControlMode;

// 全局状态
static ControlMode g_control_mode = CTRL_USER;

// 模式切换
void set_control_mode(ControlMode mode) {
    g_control_mode = mode;
    // 更新 UI 视觉
    update_control_mode_ui(mode);
    // 持久化
    config_set_int("control_mode", (int)mode);
    // 日志
    LOG("[CTRL] Control mode changed to %s", mode == CTRL_AI ? "AI" : "USER");
}

// AI 操作接口（供 Agent 调用）
bool ai_perform_click(int x, int y) {
    if (g_control_mode != CTRL_AI) return false;
    // 权限校验
    if (!permission_check(PERM_UI_CLICK)) return false;
    // 执行
    SetCursorPos(x, y);
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
    return true;
}
```

---

## 4. WS-01 — 工作区管理

### 4.1 需求细节

工作区是 OpenClaw Agent 的活动边界。用户可创建和管理多个工作区。

**核心概念：**

| 概念 | 说明 |
|------|------|
| 工作区根目录 | Agent 文件操作的根路径 |
| 工作区配置 | AGENTS.md / SOUL.md / TOOLS.md / MEMORY.md 等 |
| 工作区模板 | 通用助手 / 开发者 / 写作者 / 数据分析 |
| 工作区锁 | `.openclaw.lock` 防止多实例访问同一工作区 |

**首次启动流程：**

```
检测无 config.json
  → 向导第 5 步：选择工作区
    → 默认路径：%USERPROFILE%\.openclaw\workspace\
    → 用户可修改路径
    → 选择模板（通用助手/开发者/写作者/数据分析）
    → 点击 "Get Started"
  → 创建目录结构
  → 生成 WORKSPACE.md / AGENTS.md / SOUL.md 等
  → 写入 config.json（workspace_path）
  → 启动完成
```

**日常使用：**

- Settings → General → 当前工作区路径显示 + "更改"按钮
- 支持创建新工作区 / 切换工作区
- 切换工作区 → 需要重启 Gateway（Agent 上下文变化）

**健康检查（每次启动）：**

| 检查项 | 处理 |
|--------|------|
| 目录是否存在 | 不存在 → 提示创建或更换 |
| 磁盘空间 > 100MB | 不足 → 警告 |
| 写入权限 | 无 → 降级只读 |
| lock 文件 | PID 存活 → 拒绝；PID 死亡 → 清理 |
| 目录结构完整性 | 缺失 → 引导创建 |

### 4.2 UI 框架

**Settings → General → 工作区区域：**

```
┌─ 工作区 ─────────────────────────────────────┐
│  当前工作区: 个人助手                          │
│  路径: D:\AI\personal\                       │
│                                              │
│  [更改路径]  [创建工作区]                     │
│                                              │
│  ── 工作区列表 ──                             │
│  🏠 个人助手    D:\AI\personal\      ← 当前  │
│  💻 开发项目    D:\Projects\                 │
│                                              │
└──────────────────────────────────────────────┘
```

**创建工作区对话框：**

```
┌─ 创建新工作区 ───────────────────────────────┐
│  名称: [________________]                    │
│  路径: [D:\AI\new_workspace\] [浏览]         │
│                                              │
│  模板:                                       │
│  ○ 通用助手  ○ 开发者  ○ 写作者  ○ 数据分析  │
│                                              │
│  [取消]  [创建]                              │
└──────────────────────────────────────────────┘
```

### 4.3 数据流

```
工作区切换流程:

用户在 Settings 选择新工作区
  → AnyClaw 更新 config.json workspace_path
  → 停止 Gateway（openclaw gateway stop）
  → 更新 Gateway 配置（工作区路径）
  → 启动 Gateway（openclaw gateway start）
  → Gateway 检测新工作区 → 加载新的 AGENTS.md/SOUL.md 等
  → AnyClaw 状态检测 → 确认 Gateway 已就绪
  → UI 更新显示新工作区信息
```

**Gateway 配置同步：**

```c
// workspace.cpp
void switch_workspace(const char *new_path) {
    // 1. 停止 Gateway
    gateway_stop();

    // 2. 更新 OpenClaw 配置
    // openclaw config set workspace.root "{new_path}"
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "openclaw config set workspace.root \"%s\"", new_path);
    run_command(cmd);

    // 3. 更新 AnyClaw config.json
    config_set_string("workspace_path", new_path);

    // 4. 启动 Gateway
    gateway_start();

    // 5. 等待健康检查通过
    wait_for_gateway_ready(10000); // 10s timeout
}
```

### 4.4 代码设计

**新增/修改文件：**
- `src/workspace.h` / `.cpp` — 工作区核心管理
- `src/ui_settings.cpp` — 工作区设置 UI

**核心数据结构：**

```c
typedef struct {
    char name[128];           // 工作区名称
    char path[MAX_PATH];      // 根目录路径
    char template_type[32];   // 模板类型
    time_t created_at;        // 创建时间
    bool healthy;             // 健康状态
} Workspace;

typedef struct {
    Workspace workspaces[16]; // 最多 16 个工作区
    int count;
    int current_index;        // 当前活跃工作区
} WorkspaceManager;
```

---

## 5. PERM-01 — 权限系统

### 5.1 需求细节

v2.0 两档制权限，零配置即可用。

| 模式 | 适用 | 核心规则 |
|------|------|---------|
| **宽松模式**（默认） | 普通用户/学生 | 工作区内自由操作，外部操作弹窗询问 |
| **严格模式** | 安全敏感用户 | 所有操作均弹窗确认 |

**权限矩阵：**

| 操作 | 宽松 | 严格 |
|------|------|------|
| 读工作区文件 | ✅ | ✅ |
| 写工作区文件 | ✅ | ⚠️ 询问 |
| 读外部文件 | ⚠️ 询问 | ⚠️ 询问 |
| 写外部文件 | ⚠️ 询问 | ❌ 禁止 |
| 执行 shell 命令 | ⚠️ 询问 | ⚠️ 询问 |
| 安装软件 | ⚠️ 询问 | ❌ 禁止 |
| 删除文件 | ⚠️ 询问 | ⚠️ 询问 |
| 出站网络 | ✅ | ⚠️ 询问 |
| 剪贴板 | ✅ | ⚠️ 询问 |
| 设备访问 | ⚠️ 询问 | ❌ 禁止 |

**工作区边界（始终生效）：**
1. 文件操作限定在工作区根目录内
2. 路径穿越（`../`）拒绝
3. 系统目录（`C:\Windows\`、`C:\Program Files\`）始终禁止写入

**弹窗确认机制：**
- LVGL 模态弹窗，显示操作类型、目标路径、当前模式
- 60 秒无响应自动拒绝
- 记录到审计日志

### 5.2 UI 框架

**Settings → General → 权限模式切换：**

```
┌─ 权限 ───────────────────────────────────────┐
│  当前模式: [宽松 ●  ○ 严格]                   │
│                                              │
│  宽松: 工作区内自由，外部需确认               │
│  严格: 所有操作需确认                         │
│                                              │
└──────────────────────────────────────────────┘
```

**权限弹窗（Agent 请求权限时）：**

```
┌─────────────────────────────────────┐
│  🔒 Agent 请求权限                    │
│                                      │
│  操作: 执行命令                       │
│  命令: pip install pandas            │
│  当前模式: 宽松                       │
│                                      │
│  ⏱️ 60秒内无操作将自动拒绝             │
│                                      │
│  [允许（仅本次）]  [拒绝]            │
└─────────────────────────────────────┘
```

### 5.3 数据流

```
Agent 执行操作（如 write_file）
  → AnyClaw 拦截
  → 读取 config.json permission_mode
  → 查询权限矩阵 → 该操作在当前模式下的权限
  → allow → 直接执行
  → deny → 拒绝，通知 Agent
  → ask → 弹窗等待用户响应
    → 用户点击允许 → 执行
    → 用户点击拒绝 → 拒绝
    → 60秒超时 → 自动拒绝
  → 写入审计日志
```

**与 OpenClaw 的联动：**

```c
// 权限模式同步到 AGENTS.md MANAGED 区域
void sync_permission_to_agents_md(PermissionMode mode) {
    // 读取 AGENTS.md
    // 找到 <!-- ANYCLAW_MANAGED_START --> 和 <!-- ANYCLAW_MANAGED_END -->
    // 替换中间内容为:
    // ## 权限模式: {宽松|严格}
    // 写入前备份到 .backups/
}
```

### 5.4 代码设计

**核心结构：**

```c
typedef enum {
    PERM_PERMISSIVE = 0,
    PERM_STRICT = 1,
} PermissionMode;

typedef enum {
    PERM_ALLOW,   // 直接允许
    PERM_DENY,    // 直接拒绝
    PERM_ASK,     // 弹窗询问
} PermissionResult;

typedef struct {
    PermissionMode mode;
    char workspace_root[MAX_PATH];
} PermissionContext;

// 权限查询
PermissionResult permission_check(PermissionContext *ctx, 
                                   const char *action, 
                                   const char *target_path) {
    // 始终禁止的操作
    if (is_system_dir(target_path)) return PERM_DENY;
    if (has_path_traversal(target_path)) return PERM_DENY;

    // 工作区内操作
    if (is_inside_workspace(ctx->workspace_root, target_path)) {
        if (ctx->mode == PERM_PERMISSIVE) return PERM_ALLOW;
        return PERM_ASK; // 严格模式
    }

    // 外部操作
    if (ctx->mode == PERM_STRICT && is_write_action(action)) return PERM_DENY;
    return PERM_ASK;
}
```

---

## 6. TM-01 — 会话与任务管理

### 6.1 需求细节

AnyClaw GUI 中查看和管理 Gateway 的会话、子代理、定时任务。

**数据来源：** 所有数据通过 CLI 子进程调用获取（Gateway 不提供 REST API）。

| 数据 | 命令 | 耗时 |
|------|------|------|
| 健康状态 | `curl http://127.0.0.1:18789/health` | ~50ms |
| 会话列表 | `openclaw gateway call sessions.list --json` | ~1.5s |
| 定时任务 | `openclaw cron list --json` | ~1.5s |

**显示内容：**

| 区域 | 显示 |
|------|------|
| Gateway 服务 | 状态（Running/Error）+ 操作按钮 |
| Sessions | 渠道/模型/token 数/最后活跃时间 + Abort |
| Cron Jobs | 名称/调度表达式/状态 + Run/Enable/Delete |

**清理策略：**
- Session：ageMs < 30 分钟正常显示，> 30 分钟隐藏
- Sub-Agent：完成后保留 30 分钟
- Cron：手动删除

### 6.2 UI 框架

**Task List 面板（可从顶栏或 Settings 访问）：**

```
┌─ Task List (3) ─────────────────────────────┐
│                                              │
│  Gateway Service                 🟢 Running  │
│  [Stop] [Restart]                            │
│                                              │
│  ─── Sessions ───────────────────────        │
│  🟢 main · WebChat · Direct                  │
│     mimo-v2-pro · 137k tokens · 25s ago      │
│     [Abort]                                  │
│                                              │
│  🟡 subagent:abc123 · Cron                   │
│     mimo-v2-pro · 42k tokens · 2min ago      │
│     [Abort]                                  │
│                                              │
│  ─── Cron Jobs ──────────────────────        │
│  📋 每日提醒  0 9 * * *           ✅ Enabled │
│     [Run Now] [Disable] [Delete]             │
│                                              │
│  [+ Add Cron Job]                            │
│                                              │
└──────────────────────────────────────────────┘
```

### 6.3 数据流

```
Task List 刷新（每 30 秒自动）
  → 后台线程启动
  → 并行执行:
    → curl http://127.0.0.1:18789/health
    → openclaw gateway call sessions.list --json
    → openclaw cron list --json
  → 解析 JSON
  → 回调 UI 线程 → 更新 Task List 控件
  → 更新标题栏计数 badge (N)

用户操作（如 Abort）:
  → 点击 Abort 按钮
  → 按钮变为 Loading 状态
  → 后台执行: openclaw gateway call sessions.reset --json
  → 执行完成 → 刷新 Task List
```

### 6.4 代码设计

**核心数据结构：**

```c
typedef struct {
    char key[64];           // session key
    char channel[32];       // webchat / discord / telegram
    char agent[32];         // agent name
    char model[64];         // model name
    int tokens;             // token count
    int age_ms;             // last active (ms ago)
    bool active;            // is active session
} SessionInfo;

typedef struct {
    char id[64];            // cron job id
    char name[128];         // display name
    char schedule[64];      // cron expression
    bool enabled;
} CronJobInfo;

typedef struct {
    bool gateway_running;
    SessionInfo sessions[32];
    int session_count;
    CronJobInfo cron_jobs[32];
    int cron_count;
} TaskManagerState;
```

---

## 7. AIMODE-01 — AI 交互模式（自主 / Ask / Plan）

### 7.1 需求细节

AI 交互模式决定 **Agent 执行任务时的行为方式**，与 UI 控制权模式独立。

| 模式 | 含义 | 关键词 |
|------|------|--------|
| **自主模式** | AI 全自主判断与执行，不打断 | 自主 |
| **Ask 模式**（默认） | 自主执行，遇到决策点才暂停询问用户 | 临界询问 |
| **Plan 模式** | 先出完整方案与用户讨论，确认后自动执行 | 先谋后断 |

**模式行为详解：**

**自主模式：**
- AI 收到任务 → 自主拆解 → 逐步执行 → 完成后汇报
- 全程不向用户请求确认
- 需要完整的权限系统兜底（§PERM-01）

**Ask 模式（默认）：**
- AI 收到任务 → 自主执行
- 遇到决策节点 → 暂停 → 等待用户回答 → 继续
- 决策节点示例：
  - 有两个以上等价方案可选
  - 涉及删除/覆盖文件
  - 涉及对外发送消息
  - 涉及付费/订阅/注册

**Plan 模式：**
- 用户发布任务 → AI 先输出完整执行方案
- 方案包含：步骤列表、预期结果、风险评估
- 用户与 AI 讨论方案，可修改、调整
- 用户确认后 → AI 按方案自动执行
- 执行过程中遇到偏差 → 重新进入讨论

**模式切换：**
- Chat 面板和 Work 面板内都可快捷切换
- 切换即时生效，不需重启
- 持久化到 config.json
- 默认 Ask 模式

### 7.2 UI 框架

**Chat 面板模式切换（输入区上方）：**

```
┌─ Chat ──────────────────────────────────────┐
│                                              │
│  ...消息区域...                              │
│                                              │
│  ┌─ AI 模式 ──┐                              │
│  │ 🔄 Ask ●   │ ← 当前模式高亮               │
│  └────────────┘                              │
│  [自主] [Ask] [Plan]    [🎤] [📎] [发送]     │
│                                              │
└──────────────────────────────────────────────┘
```

- 三个模式按钮紧凑排列在输入区上方
- 当前模式高亮（绿色/蓝色背景）
- 点击即时切换，不弹确认

**Work 面板模式切换（控制栏区域）：**

```
┌─ Work ──────────────────────────────────────┐
│  [Chat] [Work]      ⚙️                       │
│                                              │
│  控制模式: [👤 用户] [🤖 AI]                 │
│  AI 模式:  [自主] [Ask ●] [Plan]             │
│                                              │
│  ...Work 内容...                             │
└──────────────────────────────────────────────┘
```

**Plan 模式下的方案展示（Chat 面板）：**

```
┌─────────────────────────────────────┐
│  🤖 执行方案                         │
│                                      │
│  步骤 1: 读取 config.json           │
│  步骤 2: 解析模型配置               │
│  步骤 3: 同步到 Gateway             │
│  步骤 4: 重启 Gateway               │
│                                      │
│  预计耗时: ~10秒                     │
│  风险: 重启期间短暂断开连接          │
│                                      │
│  [确认执行]  [修改方案]  [取消]      │
└─────────────────────────────────────┘
```

### 7.3 数据流

```
用户发送消息
  → AnyClaw 读取当前 AI 模式 (g_ai_mode)
  → 构建消息体，附加模式标识
  → POST Gateway /v1/chat/completions
  → Gateway → Agent

Agent 行为取决于模式：

自主模式:
  Agent 收到任务 → 自主拆解执行 → 流式输出结果 → 完成

Ask 模式:
  Agent 收到任务 → 执行到决策点
    → 输出决策请求（特殊消息格式标记为"询问"）
    → AnyClaw 渲染为决策气泡
    → 用户输入回答
    → AnyClaw 发送回答 → Agent 继续执行

Plan 模式:
  Agent 收到任务 → 生成执行方案
    → 输出方案（特殊消息格式标记为"方案"）
    → AnyClaw 渲染为方案卡片（含确认/修改/取消按钮）
    → 用户点击确认
    → AnyClaw 发送"确认执行" → Agent 按方案执行
```

**Ask 模式的实现——通过 system prompt 驱动：**

```json
{
  "model": "openclaw:main",
  "messages": [
    {
      "role": "system",
      "content": "当前 AI 交互模式: Ask\n规则: 执行任务时，遇到需要决策的节点（选择方案、确认风险操作、对外发送消息），必须暂停并以 [DECISION] 标记向用户提问。不要自主决定。"
    },
    {
      "role": "user",
      "content": "帮我清理工作区的临时文件"
    }
  ]
}
```

Agent 回复示例（Ask 模式）：
```
我找到了以下临时文件：
- temp/cache_001.tmp (2.3MB)
- temp/cache_002.tmp (1.1MB)
- .backups/old_backup.zip (15MB)

[DECISION] 是否删除以上 3 个文件？预计释放 18.4MB 空间。
```

AnyClaw 解析 `[DECISION]` 标记 → 渲染为决策气泡 → 等待用户输入 → 追加到对话 → 继续流式输出。

### 7.4 代码设计

**核心枚举：**

```c
typedef enum {
    AI_MODE_AUTONOMOUS = 0,  // 自主模式
    AI_MODE_ASK = 1,         // Ask 模式（默认）
    AI_MODE_PLAN = 2,        // Plan 模式
} AIMode;

static AIMode g_ai_mode = AI_MODE_ASK; // 默认 Ask
```

**模式切换处理：**

```c
void set_ai_mode(AIMode mode) {
    g_ai_mode = mode;
    // 更新 UI 高亮
    update_ai_mode_buttons(mode);
    // 持久化
    config_set_int("ai_mode", (int)mode);
    LOG("[AIMODE] AI mode changed to %s",
        mode == AI_MODE_AUTONOMOUS ? "Autonomous" :
        mode == AI_MODE_ASK ? "Ask" : "Plan");
}
```

**消息构建时注入模式提示：**

```c
// chat.cpp
const char* get_mode_system_prompt(AIMode mode) {
    switch (mode) {
        case AI_MODE_AUTONOMOUS:
            return "当前 AI 交互模式: 自主模式。全自主判断与执行，不向用户请求确认。";
        case AI_MODE_ASK:
            return "当前 AI 交互模式: Ask 模式。自主执行任务，但遇到需要决策的节点"
                   "（选择方案、确认风险操作、对外发送消息），必须暂停并以"
                   " [DECISION] 标记向用户提问。不要自主决定。";
        case AI_MODE_PLAN:
            return "当前 AI 交互模式: Plan 模式。收到任务后，先输出完整执行方案"
                   "（步骤列表、预期结果、风险评估），以 [PLAN] 标记包裹。"
                   "等待用户确认 [PLAN_CONFIRM] 后再执行。";
    }
    return "";
}
```

**决策气泡渲染（Ask 模式）：**

```c
// 渲染 AI 回复时检测特殊标记
void render_ai_response(const char *text) {
    // 检测 [DECISION] 标记
    const char *decision = strstr(text, "[DECISION]");
    if (decision) {
        // 提取决策问题
        char question[512];
        extract_after_marker(text, "[DECISION]", question, sizeof(question));
        // 渲染为决策气泡（带输入框和确认按钮）
        render_decision_bubble(question);
        // 等待用户输入
        return;
    }
    // 检测 [PLAN] 标记
    const char *plan = strstr(text, "[PLAN]");
    if (plan) {
        render_plan_card(text);
        return;
    }
    // 普通消息 → 标准 Markdown 渲染
    render_markdown_to_label(text);
}
```

---

## 8. PROACTIVE-01 — Agent 主动行为

### 8.1 需求细节

AI 管家不应该"你问它才答"，应该主动帮用户做事。

**基本原则：读和整理自己做，写和发送必须问。**

**v2.0 内置预设（4 个）：**

| 预设 | 触发条件 | 做什么 | 前置条件 |
|------|---------|--------|---------|
| 启动体检 | AnyClaw 启动时 | 检查系统状态、配置一致性、Gateway 健康 | 本地检测 |
| 空闲整理 | 用户 30 分钟无操作 + AI 控制模式 | 整理 MEMORY.md、memory 日志、自省缺陷、技能梳理 | AI 控制模式 |
| 记忆维护 | 每天 18:00 | 读取近几天 memory/*.md → 更新 MEMORY.md | MEMORY.md 存在 |
| 日终总结 | 每天 23:00 | 总结当天活动、记录到 memory 日志 | 无 |

**待激活预设（应用授权后）：**

| 预设 | 触发条件 | 前置条件 |
|------|---------|---------|
| 日程提醒 | 每天 09:00 | 日历应用已授权 |
| 邮件摘要 | 每天 12:00 | 邮件应用已授权 |

**用户配置：**
- Settings 中预设列表：显示名称、触发条件、开关
- 支持添加自定义预设
- 支持编辑触发条件（时间/事件）
- 支持删除预设

**通知分级：**

| 级别 | 方式 | 示例 |
|------|------|------|
| 静默 | 不通知，写日志 | 整理了 MEMORY.md |
| 摘要 | 固定时间汇总 | "今天帮你做了 3 件事" |
| 提醒 | 托盘气泡 | "日历提醒：20 分钟后有会议" |

### 8.2 UI 框架

**Settings → Agent 主动行为：**

```
┌─ Agent 主动行为 ─────────────────────────────┐
│                                              │
│  ┌─ 预设列表 ────────────────────────────┐   │
│  │                                        │   │
│  │  ✅ 启动体检    启动时触发      [编辑] │   │
│  │  ✅ 空闲整理    30min无操作     [编辑] │   │
│  │  ✅ 记忆维护    每天 18:00     [编辑]  │   │
│  │  ✅ 日终总结    每天 23:00     [编辑]  │   │
│  │  ⏳ 日程提醒    每天 09:00     需授权  │   │
│  │  ⏳ 邮件摘要    每天 12:00     需授权  │   │
│  │                                        │   │
│  └────────────────────────────────────────┘   │
│                                              │
│  [+ 添加预设]                                 │
│                                              │
└──────────────────────────────────────────────┘
```

**添加/编辑预设对话框：**

```
┌─ 编辑预设 ──────────────────────────────────┐
│  名称: [每日检查磁盘空间]                    │
│  触发类型: [定时 ● 下拉]                     │
│  触发时间: [每天] [10:00]                    │
│  任务描述:                                   │
│  ┌──────────────────────────────────────┐   │
│  │ 检查工作区磁盘空间，不足100MB时通知    │   │
│  └──────────────────────────────────────┘   │
│  通知级别: [摘要 ●  ○ 提醒 ○ 静默]          │
│                                              │
│  [取消]  [保存]  [删除]                      │
└──────────────────────────────────────────────┘
```

### 8.3 数据流

```
触发流程（以"记忆维护"为例）:

定时器（Cron）触发
  → AnyClaw 创建隔离 session
  → 发送 systemEvent: "执行预设任务: 记忆维护。读取近几天的 memory/*.md 文件，"
    "更新 MEMORY.md 中的长期记忆。完成后汇报摘要。"
  → Gateway → Agent 独立执行
  → Agent 读取 memory/*.md → 分析 → 更新 MEMORY.md
  → 完成 → 输出摘要
  → AnyClaw 接收摘要 → 根据通知级别决定是否通知用户

空闲整理触发:

用户 30 分钟无操作（鼠标键盘无输入）
  → AnyClaw 检测 g_control_mode == CTRL_AI
  → 是 → 触发空闲整理
  → 创建隔离 session
  → 发送 systemEvent: "执行预设任务: 空闲整理。整理 MEMORY.md、"
    "memory 日志、自省缺陷、技能梳理。不要动用户工作区文件。"
  → Agent 执行 → 完成后静默记录
  → 否 → 不触发
```

**预设存储（config.json）：**

```json
{
  "proactive_presets": [
    {
      "id": "startup_check",
      "name": "启动体检",
      "enabled": true,
      "trigger": "startup",
      "task": "检查系统状态、配置一致性、Gateway 健康",
      "notify": "summary"
    },
    {
      "id": "idle_cleanup",
      "name": "空闲整理",
      "enabled": true,
      "trigger": "idle_30min",
      "condition": "ai_control_mode",
      "task": "整理 MEMORY.md、memory 日志、自省缺陷、技能梳理",
      "notify": "silent"
    },
    {
      "id": "memory_maintain",
      "name": "记忆维护",
      "enabled": true,
      "trigger": "cron",
      "cron_expr": "0 18 * * *",
      "task": "读取近几天 memory/*.md，更新 MEMORY.md",
      "notify": "silent"
    },
    {
      "id": "daily_summary",
      "name": "日终总结",
      "enabled": true,
      "trigger": "cron",
      "cron_expr": "0 23 * * *",
      "task": "总结当天活动，记录到 memory 日志",
      "notify": "summary"
    }
  ]
}
```

### 8.4 代码设计

**预设管理核心：**

```c
typedef enum {
    TRIGGER_STARTUP,      // 启动时
    TRIGGER_IDLE,         // 空闲（需要额外条件）
    TRIGGER_CRON,         // 定时
    TRIGGER_EVENT,        // 事件
} TriggerType;

typedef enum {
    NOTIFY_SILENT,    // 静默
    NOTIFY_SUMMARY,   // 摘要
    NOTIFY_REMINDER,  // 提醒（托盘气泡）
} NotifyLevel;

typedef struct {
    char id[64];
    char name[128];
    bool enabled;
    TriggerType trigger;
    char cron_expr[64];       // TRIGGER_CRON 时使用
    char condition[64];       // 额外条件（如 "ai_control_mode"）
    char task[512];           // 任务描述（发给 Agent 的 prompt）
    NotifyLevel notify;
} ProactivePreset;

// 预设触发执行
void execute_preset(ProactivePreset *preset) {
    // 1. 检查前置条件
    if (strcmp(preset->condition, "ai_control_mode") == 0) {
        if (g_control_mode != CTRL_AI) return; // 不满足条件，跳过
    }

    // 2. 创建隔离 session
    // 3. 发送 systemEvent 包含 preset->task
    // 4. 等待 Agent 完成
    // 5. 根据 notify 级别决定通知方式

    char event_text[1024];
    snprintf(event_text, sizeof(event_text),
        "执行预设任务: %s。%s", preset->name, preset->task);

    // 通过 Gateway cron 或 sessions_spawn 执行
    gateway_send_system_event(event_text);
}
```

**空闲检测：**

```c
// 全局鼠标键盘 Hook
static DWORD g_last_input_tick = 0;

LRESULT CALLBACK input_hook_proc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        g_last_input_tick = GetTickCount();
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

// 空闲检测线程
DWORD WINAPI idle_monitor_thread(LPVOID param) {
    while (g_running) {
        Sleep(60000); // 每分钟检查一次
        DWORD idle_ms = GetTickCount() - g_last_input_tick;
        if (idle_ms >= 30 * 60 * 1000) { // 30 分钟
            // 触发空闲整理
            ProactivePreset *p = find_preset("idle_cleanup");
            if (p && p->enabled) {
                execute_preset(p);
            }
        }
    }
    return 0;
}
```

---

## 9. WORK-01 — Work 模式（AI 操作监控面板）

### 9.1 需求细节

Work 模式不是传统代码编辑器，而是 **AI 操作监控面板**——用户注意力放在"AI 做了什么、对不对"上。

**核心组件：**

| 组件 | 功能 |
|------|------|
| 步骤流 | 实时展示 Agent 每步操作（读文件/写文件/执行命令） |
| Diff 可视化 | 修改前后对比（绿增红删） |
| 终端输出 | Agent 执行命令的实时输出 |
| Chat 面板 | 侧边对话 + 工具输出 |

**与 Chat 模式的关系：**
- Chat 和 Work 共享同一个 Agent 的记忆、会话、工具调用
- 切换不丢上下文
- 顶栏按钮一键切换

### 9.2 UI 框架

**Work 模式完整布局：**

```
┌──────────────────────────────┬──────┐
│  [Chat] [Work]      ⚙️       │      │  ← 顶栏
├──────────────────────────────┤ Chat │
│                              │ 面板  │
│  ┌─ 步骤流 ───────────────┐ │      │
│  │                         │ │      │
│  │  ▶ 步骤1: 读取文件      │ │      │
│  │    src/config.json      │ │      │
│  │    ✅ 完成 (12ms)       │ │      │
│  │                         │ │      │
│  │  ▶ 步骤2: 修改配置      │ │      │
│  │    src/config.json      │ │      │
│  │    ┌─ Diff ──────────┐  │ │      │
│  │    │ - "model": "a"  │  │ │      │
│  │    │ + "model": "b"  │  │ │      │
│  │    └─────────────────┘  │ │      │
│  │    ✅ 完成              │ │      │
│  │                         │ │      │
│  │  ▶ 步骤3: 重启服务      │ │      │
│  │    ⏳ 执行中...         │ │      │
│  │                         │ │      │
│  └─────────────────────────┘ │      │
│                              │      │
├──────────────────────────────┤      │
│  ┌─ 终端 ──────────────────┐ │      │
│  │ $ openclaw gateway stop │ │      │
│  │ Gateway stopped.        │ │      │
│  │ $ openclaw gateway start│ │      │
│  │ Gateway started. ✅     │ │      │
│  └─────────────────────────┘ │      │
└──────────────────────────────┴──────┘
```

**步骤流中的每个步骤卡片：**

```
┌─────────────────────────────────┐
│  ▶ 步骤 N: {操作类型}           │  ← 点击展开/折叠
│    {目标文件/命令}              │
│    ⏳ 执行中... / ✅ 完成 / ❌ 失败 │
│                                 │
│  （展开时）                      │
│  ┌─ Diff ────────────────────┐  │
│  │  - old_line               │  │
│  │  + new_line               │  │
│  └───────────────────────────┘  │
│  [接受] [拒绝] [回退]          │  ← 仅写操作有
└─────────────────────────────────┘
```

### 9.3 数据流

```
Agent 执行编程任务
  → Agent 通过 Gateway 工具调用执行操作
  → 每个操作步骤通过 SSE 流式推送到 AnyClaw
  → AnyClaw 解析操作类型:
    → read_file → 步骤流中显示"读取文件 {path}"
    → write_file → 步骤流中显示"修改文件 {path}" + Diff 可视化
    → exec → 步骤流中显示"执行命令 {cmd}" + 终端输出
  → 写操作暂停等待用户 [接受]/[拒绝]
  → 用户接受 → 继续下一步
  → 用户拒绝 → Agent 回退，调整方案

数据格式（SSE 通道）:

event: step
data: {"type": "read", "path": "src/config.json", "status": "executing"}

event: step_result
data: {"type": "read", "path": "src/config.json", "content": "...", "status": "done"}

event: step
data: {"type": "write", "path": "src/config.json", "diff": {"old": "...", "new": "..."}}

event: step_pause
data: {"type": "write", "path": "src/config.json", "reason": "等待用户确认"}

event: step
data: {"type": "exec", "cmd": "openclaw gateway restart", "status": "executing"}

event: step_output
data: {"type": "exec", "cmd": "openclaw gateway restart", "output": "Gateway started."}
```

### 9.4 代码设计

**步骤数据结构：**

```c
typedef enum {
    STEP_READ,      // 读取文件
    STEP_WRITE,     // 写入文件
    STEP_EXEC,      // 执行命令
    STEP_SEARCH,    // 搜索
    STEP_OTHER,     // 其他
} StepType;

typedef enum {
    STEP_RUNNING,
    STEP_DONE,
    STEP_FAILED,
    STEP_WAITING_USER,  // 等待用户确认（写操作）
} StepStatus;

typedef struct {
    int step_number;
    StepType type;
    StepStatus status;
    char target[512];         // 文件路径或命令
    char *output;             // 输出内容
    char *diff_old;           // 修改前内容
    char *diff_new;           // 修改后内容
    lv_obj_t *card_widget;   // UI 卡片
} StepCard;
```

**Diff 可视化渲染：**

```c
void render_diff_to_container(lv_obj_t *container, 
                               const char *old_text, 
                               const char *new_text) {
    // 简单逐行对比
    char **old_lines = split_lines(old_text);
    char **new_lines = split_lines(new_text);

    for (int i = 0; i < max(old_count, new_count); i++) {
        if (line_changed(old_lines[i], new_lines[i])) {
            // 删除行：红色背景
            lv_obj_t *del = lv_label_create(container);
            lv_label_set_text_fmt(del, "- %s", old_lines[i]);
            // 设置红色背景样式

            // 新增行：绿色背景
            lv_obj_t *add = lv_label_create(container);
            lv_label_set_text_fmt(add, "+ %s", new_lines[i]);
            // 设置绿色背景样式
        } else {
            // 未变行：普通样式
            lv_obj_t *same = lv_label_create(container);
            lv_label_set_text(same, old_lines[i]);
        }
    }
}
```

---

## 10. LIC-01 — License 授权系统

### 10.1 需求细节

**商业模式：** 纯付费买断，无免费版。授权即全功能。

**两种密钥类型：**

| 类型 | 前缀 | 生成方式 | 有效期 |
|------|------|---------|--------|
| HW 密钥 | `HW-` | `keygen.py hw <MAC>` | 永久（绑定 MAC 地址） |
| TM 密钥 | `TM-` | `keygen.py tm <hours>` | 指定小时数 |

**密钥格式：** `HW-XXXXX-XXXXX-XXXXX` / `TM-XXXXX-XXXXX-XXXXX`

**验证流程：**
1. 读取密钥前缀 → 判断类型
2. HW：解码 → 采集当前 MAC → HMAC 比对 → 通过则永久解锁
3. TM：解码 → 提取过期时间 → 检查是否过期 → 未过期则临时解锁
4. 两种密钥独立存储，任意一种有效即可使用

**过期行为：**
- 过期弹窗：全屏遮罩 + 大蒜图标 + "试用期已结束" + 密钥输入框 + 激活按钮
- 底部状态栏显示：剩余 XXh（低时间时变红）

### 10.2 UI 框架

**过期/未授权弹窗（全屏遮罩）：**

```
┌─────────────────────────────────────────────┐
│                                             │
│              🧄 AnyClaw                     │
│                                             │
│         授权密钥验证                          │
│                                             │
│     密钥: [________________________]        │
│           格式: HW-XXXXX-XXXXX-XXXXX       │
│                 或 TM-XXXXX-XXXXX-XXXXX     │
│                                             │
│              [激活]                          │
│                                             │
│     获取密钥请联系开发者                      │
│                                             │
└──────────────────────────────────────────────┘
```

**已授权状态（Settings → About）：**

```
┌─ License ───────────────────────────────────┐
│  状态: ✅ 已激活                             │
│  类型: HW 硬件密钥（永久）                   │
│  机器码: AA:BB:CC:DD:EE:FF                  │
│                                              │
│  [重新输入密钥]                              │
└──────────────────────────────────────────────┘
```

**TM 密钥低时间警告（底部状态栏）：**

```
AnyClaw v2.0.9 | LVGL 9.6 + SDL2 | ⏰ 剩余 23h
```

### 10.3 数据流

```
启动流程:
  → AnyClaw 启动
  → 读取 config.json 中的 hw_key / tm_key
  → 无任何有效密钥 → 显示授权弹窗（阻断启动）
  → 有密钥 → 验证:
    → HW: 采集 MAC → HMAC(hw_key, MAC) 比对
    → TM: 解码 tm_key → 提取过期时间 → 与当前时间比对
  → 验证通过 → 正常启动
  → 验证失败 / 已过期 → 显示授权弹窗

用户输入新密钥:
  → 输入框输入密钥
  → 点击 [激活]
  → 解析前缀 → 验证
  → 通过 → 写入 config.json → 关闭弹窗 → 启动
  → 失败 → 显示错误提示

密钥生成（开发者侧，独立工具）:
  → python3 keygen.py hw AA:BB:CC:DD:EE:FF
  → 输出: HW-XXXXX-XXXXX-XXXXX
  → python3 keygen.py tm 720
  → 输出: TM-XXXXX-XXXXX-XXXXX（720小时有效）
```

### 10.4 代码设计

**新增文件：**
- `src/license.h` / `.cpp` — 密钥验证逻辑

**密钥验证核心：**

```c
typedef enum {
    LIC_NONE = 0,      // 未授权
    LIC_HW = 1,        // 硬件永久授权
    LIC_TM = 2,        // 时间授权
} LicenseType;

typedef struct {
    LicenseType type;
    bool valid;
    time_t tm_expiry;  // TM 密钥过期时间（0 = 永久）
    char hw_mac[32];   // 绑定的 MAC 地址
} LicenseState;

// 获取 MAC 地址
bool get_first_mac(char *mac_buf, int buf_size) {
    // 使用 GetAdaptersAddresses 获取第一个非回环网卡
    // 格式: "AA:BB:CC:DD:EE:FF"
}

// HW 密钥验证
bool verify_hw_key(const char *key, const char *mac) {
    // 1. 检查前缀 "HW-"
    // 2. 解码密钥 → 得到 expected_hmac
    // 3. 计算 HMAC(mac, salt) → actual_hmac
    // 4. 比对 expected_hmac == actual_hmac
}

// TM 密钥验证
bool verify_tm_key(const char *key, time_t *expiry_out) {
    // 1. 检查前缀 "TM-"
    // 2. 解码密钥 → 得到 encoded_hours + expiry_timestamp
    // 3. 验证 expiry_timestamp > now
    // 4. 输出过期时间
}

// 综合验证
LicenseState check_license(const char *hw_key, const char *tm_key) {
    LicenseState state = {0};
    char mac[32];

    // 优先检查 HW 密钥
    if (hw_key && hw_key[0]) {
        get_first_mac(mac, sizeof(mac));
        if (verify_hw_key(hw_key, mac)) {
            state.type = LIC_HW;
            state.valid = true;
            return state;
        }
    }

    // 再检查 TM 密钥
    if (tm_key && tm_key[0]) {
        if (verify_tm_key(tm_key, &state.tm_expiry)) {
            state.type = LIC_TM;
            state.valid = true;
            return state;
        }
    }

    state.valid = false;
    return state;
}
```

**keygen.py（独立工具，不随发布包）：**

```python
import hmac, hashlib, sys, time, base64

SALT = "AnyClaw_v2.0"

def gen_hw_key(mac: str) -> str:
    """生成硬件绑定密钥"""
    sig = hmac.new(SALT.encode(), mac.encode(), hashlib.sha256).hexdigest()[:20]
    # sig 分为 3 段: XXXXX-XXXXX-XXXXX
    chunks = [sig[i:i+5] for i in range(0, 15, 5)]
    return f"HW-{chunks[0]}-{chunks[1]}-{chunks[2]}"

def gen_tm_key(hours: int) -> str:
    """生成时间密钥"""
    expiry = int(time.time()) + hours * 3600
    payload = f"{hours}:{expiry}"
    sig = hmac.new(SALT.encode(), payload.encode(), hashlib.sha256).hexdigest()[:20]
    # 编码 + 签名
    chunks = [sig[i:i+5] for i in range(0, 15, 5)]
    return f"TM-{chunks[0]}-{chunks[1]}-{chunks[2]}"

if __name__ == "__main__":
    if sys.argv[1] == "hw":
        print(gen_hw_key(sys.argv[2]))
    elif sys.argv[1] == "tm":
        print(gen_tm_key(int(sys.argv[2])))
```

---

## 11. U-01 — 更新机制

### 11.1 需求细节

**AnyClaw 更新：** GitHub Releases 自动检测 + 一键下载覆盖。

**OpenClaw 更新：** AnyClaw GUI 内一键触发 `openclaw update`。

**更新检查流程：**

```
AnyClaw 启动时
  → GET https://api.github.com/repos/IwanFlag/AnyClaw_LVGL/releases/latest
  → 解析 JSON → 获取 tag_name（版本号）
  → 与当前版本 ANYCLAW_VERSION 比较
  → 有新版本 → 弹窗提示
  → 用户确认 → 下载 exe → 临时目录
  → 下载完成 → 验证文件完整性
  → 覆盖当前 exe → 重启 AnyClaw

手动检查:
  → Settings → About → [检查更新] 按钮
  → 同上流程
```

### 12.2 UI 框架

**更新发现弹窗：**

```
┌─ 发现新版本 ────────────────────────────────┐
│                                              │
│  当前版本: v2.0.9                            │
│  最新版本: v2.1.0                            │
│                                              │
│  更新内容:                                   │
│  • 新增 Work 模式 Diff 可视化                │
│  • 修复聊天滚动问题                          │
│  • 性能优化                                  │
│                                              │
│  [稍后提醒]  [立即更新]                      │
│                                              │
└──────────────────────────────────────────────┘
```

**下载进度：**

```
┌─ 正在更新 ──────────────────────────────────┐
│                                              │
│  ████████████████░░░░░░░░  72%              │
│  14.4 MB / 20.0 MB  ·  3.2 MB/s            │
│                                              │
│  更新完成后将自动重启                         │
│  [取消]                                      │
│                                              │
└──────────────────────────────────────────────┘
```

### 11.3 数据流

```
启动检查:
  → CreateThread(update_check_thread)
  → http_get("https://api.github.com/repos/IwanFlag/AnyClaw_LVGL/releases/latest")
  → 解析 JSON:
    {
      "tag_name": "v2.1.0",
      "body": "更新内容...",
      "assets": [
        { "name": "AnyClaw_LVGL.exe", "browser_download_url": "..." }
      ]
    }
  → 比较版本 semver_compare("2.0.9", "2.1.0") < 0
  → 通知 UI 线程 → 显示更新弹窗

下载:
  → 用户点击 [立即更新]
  → 下载到 %TEMP%\AnyClaw_update.exe
  → 验证文件大小（与 GitHub API 返回的 size 比对）
  → 复制 exe → 覆盖当前 exe
  → ShellExecute 重启自身
```

### 11.4 代码设计

```c
typedef struct {
    char version[32];       // 最新版本号
    char changelog[4096];   // 更新内容
    char download_url[512]; // 下载地址
    uint64_t file_size;     // 文件大小
    bool update_available;
} UpdateInfo;

// 检查更新（后台线程）
DWORD WINAPI update_check_thread(LPVOID param) {
    UpdateInfo *info = (UpdateInfo *)param;

    char response[8192];
    http_get("https://api.github.com/repos/IwanFlag/AnyClaw_LVGL/releases/latest",
             response, sizeof(response));

    // 解析 JSON
    parse_release_json(response, info);

    // 版本比较
    if (version_compare(ANYCLAW_VERSION, info->version) < 0) {
        info->update_available = true;
        // 通知 UI
        PostMessage(g_main_hwnd, WM_UPDATE_AVAILABLE, 0, 0);
    }

    return 0;
}

// 下载更新
DWORD WINAPI update_download_thread(LPVOID param) {
    UpdateInfo *info = (UpdateInfo *)param;

    char temp_path[MAX_PATH];
    GetTempPathA(MAX_PATH, temp_path);
    strcat(temp_path, "AnyClaw_update.exe");

    // 下载
    http_download(info->download_url, temp_path, download_progress_cb);

    // 覆盖
    char current_exe[MAX_PATH];
    GetModuleFileNameA(NULL, current_exe, MAX_PATH);

    // 需要先退出再覆盖，用批处理辅助
    char bat_cmd[1024];
    snprintf(bat_cmd, sizeof(bat_cmd),
        "timeout /t 2 && copy /y \"%s\" \"%s\" && start \"\" \"%s\"",
        temp_path, current_exe, current_exe);
    // 执行批处理后退出
    system(bat_cmd);
    exit(0);

    return 0;
}
```

---

## 附录：AnyClaw ↔ OpenClaw 通信架构总结

```
┌─────────────────────────────────────────────────────────┐
│                     AnyClaw (GUI)                        │
│                                                          │
│  Chat UI ──→ [消息构建] ──→ HTTP POST                    │
│                              │                           │
│  Work UI ──→ [步骤解析] ──→ │ SSE 流式接收               │
│                              │                           │
│  Settings ─→ [配置同步] ──→ │ CLI 子进程调用             │
│                              │                           │
│  License ──→ [本地验证]     │ (不走 Gateway)             │
│                              │                           │
│  本地模型 ─→ [直连推理]     │ (不走 Gateway)             │
└──────────────┬──────────────┼────────────────────────────┘
               │              │
       ┌───────┴──────┐      │
       │              │      │
       ▼              ▼      ▼
  Gateway :18789   llama-server :18800
       │              │
       ▼              ▼
  OpenClaw Agent   GGUF 模型本地推理
       │
       ▼
  LLM API (云端)
```

**通信方式一览：**

| 场景 | AnyClaw → | 协议 | 数据格式 |
|------|-----------|------|---------|
| 聊天（云端） | Gateway :18789 | HTTP POST + SSE | OpenAI chat completions |
| 聊天（本地） | llama-server :18800 | HTTP POST + SSE | OpenAI chat completions |
| 健康检查 | Gateway :18789 | HTTP GET | /health JSON |
| 配置读取 | openclaw CLI | 子进程 | JSON stdout |
| 配置写入 | openclaw config set | 子进程 | 命令行参数 |
| 会话管理 | gateway call sessions | 子进程 | JSON stdout |
| 定时任务 | openclaw cron | 子进程 | JSON stdout |
| License | 本地 | 内存 | 无网络 |
| 更新检查 | GitHub API | HTTP GET | JSON |
| 模型下载 | HuggingFace 镜像 | HTTP 下载 | 文件流 |
