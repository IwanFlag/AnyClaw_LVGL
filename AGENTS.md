# AGENTS.md — AnyClaw LVGL AI 开发规范

> 本文档定义 AI 助手在本仓库中的全部工作流。每次会话启动时必读。

---

## 项目概况

**AnyClaw LVGL** — AI 桌面管家客户端

| 项目 | 说明 |
|------|------|
| 语言 | C/C++17 |
| UI 框架 | LVGL 9.6 + SDL2 |
| 平台 | Windows 10/11 (x64)，Linux 交叉编译 |
| 架构 | AnyClaw (GUI) → OpenClaw Gateway (Agent) → LLM API |
| 仓库 | github.com/IwanFlag/AnyClaw_LVGL |

### 目录结构

```
AnyClaw_LVGL/
├── AGENTS.md                ← 本文件（AI 工作流规范）
├── docs/
│   ├── README.md            ← 文档索引（人类用）
│   ├── PRD.md               ← 产品需求（做什么、验收标准）
│   └── Design.md            ← 设计规范（长什么样、怎么画）
├── scripts/
│   └── verify-docs.sh       ← 文档一致性验证
├── tasks/                   ← 开发任务跟踪
├── src/                     ← 应用源码 (C/C++)
├── ui/                      ← UI 定义
├── assets/                  ← 运行时资源 (PNG 图标等)
├── bundled/                 ← 离线安装包 + workspace 模板
├── tools/                   ← 辅助脚本 (keygen 等)
├── thirdparty/              ← 第三方依赖（已提交到仓库）
│   ├── lvgl/                ←   LVGL 9.6 源码
│   ├── sdl2-mingw/          ←   SDL2 MinGW 交叉编译依赖
│   └── sdl2-windows/        ←   SDL2 MSVC 依赖
├── build/                   ← 构建脚本与产物
│   ├── linux/               ←   Linux 构建（主力）
│   ├── windows/             ←   Windows 原生构建
│   └── toolchain-mingw64.cmake
└── CMakeLists.txt           ← 构建配置
```

---

## 一、文档工作流

### 核心规则

- PRD 和 Design **版本号必须同步**
- 交叉引用锚点用**功能编号**（如 `CI-01`），不用章节号（如 `§4.2`）
- 改一份文档**必须查关联**（查 Design 开头的编号索引表）
- **PRD 是 Source of Truth**：功能增删改以 PRD 为准，Design 是下游

### 修改流程

当需要修改产品文档时，严格按以下 6 步执行：

```
1. 确定编号      — 找到要修改的功能编号（如 CI-01）
2. 查索引表      — 在 Design 开头的「编号索引表」中找到所有关联节
3. 改主文档      — 先改用户要求的那份
4. 同步关联      — 按每节末尾的 ⚠️ 提示，修改所有关联节
5. 更新状态      — 功能状态变更时，PRD 和 Design 的状态标记都要改
6. 验证          — 运行 bash scripts/verify-docs.sh
```

**禁止只改一份文档而不检查关联。**

### 编号索引速查

修改前先查 Design 开头的「编号索引表」，找到对应的 PRD↔Design 编号映射。

| PRD 编号 | 功能名 | Design UI 编号 |
|----------|--------|---------------|
| CI-01 | Chat 输入框 | UI-17 |
| MB-01 | 消息气泡与流式 | UI-14, UI-15, UI-16 |
| WORK-01 | Work 模式 | UI-12, UI-23~30 |
| PERM-01 | 权限系统 | UI-31, UI-46 |
| MODEL-01 | 云端模型管理 | UI-47 |
| LOCAL-01 | 本地模型管理 | UI-42 |
| VOICE-01 | 语音输入 | — |
| SKILL-01 | Skill 管理 | UI-65 |
| ... | 完整表见 Design.md 开头 | ... |

### 版本号规范

- 格式：`v{主版本}.{次版本}.{修订号}`
- 当前版本：`v2.2.1`
- 存储：PRD 标题行 + Design 标题行，两处必须一致

---

## 二、编译打包

### 编译前必读

- 构建详细文档：`build/README.md`
- **每次编译必须使用编译+打包脚本，不要执行单独编译**

### 系统依赖（apt）

```bash
apt-get install mingw-w64 libsdl2-dev zip cmake
```

### 仓库内依赖（已提交，无需下载）

| 依赖 | 位置 |
|------|------|
| LVGL 9.6 | `thirdparty/lvgl/` |
| SDL2 MinGW | `thirdparty/sdl2-mingw/` (头文件 + 静态库 + DLL) |
| SDL2 MSVC | `thirdparty/sdl2-windows/` |

### 命令参考

#### `bash build/linux/build.sh [version]`

**用途：** Linux 交叉编译 + 打包 + Wine 截图（完整流程，主力脚本）
**参数：** `version` — 版本号，默认 `v2.0.0`
**前置：** `mingw-w64`, `cmake`, `zip`, `xvfb`（截图用）, `wine`（截图用）
**流程：** 清理 → CMake 配置 → make -j → 复制 assets → 代码签名（可选）→ zip 打包 → Wine 截图
**输出：** `build/linux/artifacts/AnyClaw_LVGL_v{version}_{YYYYMMDD_HHMM}.zip`
**产物内容：** `AnyClaw_LVGL.exe` + `SDL2.dll` + `assets/` + `bundled/`
**退出码：** 0=成功, 1=编译/打包失败

#### `bash build/linux/build-win.sh [version]`

**用途：** 精简版交叉编译 + 打包（无截图）
**参数：** `version` — 版本号，默认 `v2.0.0`
**前置：** `mingw-w64`, `cmake`, `zip`
**输出：** `build/linux/artifacts/AnyClaw_LVGL_v{version}_{timestamp}.zip`

#### `bash build/linux/build-linux.sh`

**用途：** 仅原生 Linux 编译（测试用，不打包）
**前置：** `cmake`, `libsdl2-dev`
**输出：** `build/linux/out/bin/AnyClaw_LVGL`

#### `bash build/linux/run-wine.sh [build_dir] [timeout_seconds]`

**用途：** Wine 测试运行（headless + Xvfb）
**参数：** `build_dir` — 构建目录，默认 `out/mingw`；`timeout_seconds` — 超时，默认 10
**前置：** `wine`, `xvfb`
**输出：** 终端日志 + `/tmp/anyclaw-wine-test-*.log`

#### `build\windows\build-package.bat [--quiet|--verbose]`

**用途：** Windows 原生编译 + 打包（自动加载 VS 环境）
**平台：** Windows
**前置：** VS2022/VS2026 或 MinGW, cmake
**输出：** `build/windows/artifacts/`
**生成器降级链：** VS2026 → VS2022 → NMake → MinGW

#### `build\windows\build.bat [--quiet|--verbose]`

**用途：** Windows 仅编译
**平台：** Windows
**输出：** `build/windows/out/bin/`

---

## 三、Git 工作流

### 提交前检查清单

- [ ] 编译通过：`bash build/linux/build.sh v2.2.1` 无报错
- [ ] 文档验证：`bash scripts/verify-docs.sh` 无报错
- [ ] 产物正常：zip 包含 `AnyClaw_LVGL.exe` + `SDL2.dll` + `assets/`

### 提交信息格式

```
<类型>(<模块>): <简述，50字以内>

<详细说明（可选，72字换行）>

相关: <PRD功能编号或UI编号>
```

**类型：**

| 类型 | 用途 |
|------|------|
| `feat` | 新功能 |
| `fix` | Bug 修复 |
| `docs` | 文档变更 |
| `refactor` | 重构（不改变外部行为） |
| `build` | 构建脚本变更 |
| `chore` | 杂项（配置、依赖等） |

**示例：**

```
fix(chat): 修复 KB 上下文覆盖附件信息的 bug

submit_prompt_to_chat() 中 KB 分支直接覆盖了已拼接附件信息的 payload，
改为追加而非覆盖。

Bug-001 | 相关: CI-01, MM-01
```

```
docs: 重构 PRD 为 13 章结构，移除 UI 内容

- 合并 §6/§7/§8 为模式体系
- 新增 Agent 能力、语音输入、本地模型管理章节
- UI 内容全部移入 Design.md

相关: 全部功能编号
```

### 分支策略

- `main` — 稳定版本（当前单人开发，直接在 main 提交）
- 大功能开 `feature/xxx` 分支

### 提交内容

| 提交 | 不提交（.gitignore） |
|------|---------------------|
| `src/` 源码变更 | `build/*/out/` 编译中间产物 |
| `docs/` 文档变更 | `build/windows/.env-path.bat`（个人路径） |
| `build/` 构建脚本 | `*.o`, `*.obj` 编译对象文件 |
| `assets/` 资源变更 | `*.log`, `*.bak`, `*.tmp` |
| `scripts/` 工具脚本 | |
| `tasks/` 任务跟踪 | |

### 典型工作流

```
改代码 → 编译验证 → 改文档（PRD + Design 同步）→ 验证文档一致性 → 提交 → 推送
```

具体步骤：
1. 修改 `src/` 代码
2. `bash build/linux/build.sh v2.2.1` — 编译通过
3. 修改 `docs/PRD.md` 对应功能状态
4. 修改 `docs/Design.md` 对应 UI 状态
5. `bash scripts/verify-docs.sh` — 一致性检查
6. `git add -A && git commit -m "<格式>"` — 提交
7. `git push` — 推送

---

## 四、命令速查

### 编译打包

| 命令 | 用途 | 平台 | 产物位置 |
|------|------|------|---------|
| `bash build/linux/build.sh [version]` | 交叉编译+打包+截图 | Linux→Win | `build/linux/artifacts/` |
| `bash build/linux/build-win.sh [version]` | 交叉编译+打包 | Linux→Win | `build/linux/artifacts/` |
| `bash build/linux/build-linux.sh` | 原生编译 | Linux | `build/linux/out/` |
| `bash build/linux/run-wine.sh [dir] [timeout]` | Wine 测试运行 | Linux | — |
| `build\windows\build-package.bat [--quiet]` | 原生编译+打包 | Windows | `build/windows/artifacts/` |
| `build\windows\build.bat [--quiet]` | 仅编译 | Windows | `build/windows/out/` |

### 文档验证

| 命令 | 用途 |
|------|------|
| `bash scripts/verify-docs.sh` | PRD↔Design 编号+版本一致性检查 |

### Git

| 命令 | 用途 |
|------|------|
| `git add -A && git commit -m "<格式>"` | 提交变更（编译通过+文档验证通过后） |
| `git push` | 推送到远程 |

### OpenClaw CLI（AnyClaw 代码中调用）

| 命令 | 用途 | PRD |
|------|------|-----|
| `openclaw gateway start` | 启动 Gateway | §12.2 |
| `openclaw gateway stop` | 停止 Gateway | §12.2 |
| `openclaw gateway call sessions.list --json` | 查询会话列表 | §4.5 |
| `openclaw gateway call sessions.reset --json -d '{"sessionKey":"..."}'` | 终止单个会话 | §4.5 |
| `openclaw gateway call sessions.reset --json` | 重置所有会话 | §4.5 |
| `openclaw config set <key> <value>` | 修改配置 | §7.2 |
| `openclaw config get <key>` | 读取配置 | §7.2 |
| `openclaw cron list --json` | 查询定时任务 | §4.5 |
| `openclaw update` | 更新 OpenClaw | §7.5 |

### 系统检测（AnyClaw 启动自检调用）

| 命令 | 检测项 | PRD |
|------|--------|-----|
| `node --version` | Node.js ≥ 22.14 | §2.2 |
| `npm --version` | npm 可用性 | §2.2 |
| `npm cache clean --force` | npm 缓存修复 | §2.2 |
| `where openclaw` | OpenClaw PATH 查找 | §2.4 |
| `npm root -g` | npm 全局目录查找 | §2.4 |

---

## 五、安全红线（不可违反）

- **不提交** API Key、Token、密码、私钥到仓库
- **不提交** `openclaw.json` 配置内容（含认证信息）
- **不提交** `~/.openclaw/identity/` 或 `~/.openclaw/credentials/` 下的任何文件
- **不在提交信息中**包含密钥或敏感数据
- 构建产物中的迁移导出需脱敏（`api_key` → `""`, `token` → `"__REDACTED__"`）
- 对外部动作（发邮件、发推、公开发布）要谨慎，先问再做
