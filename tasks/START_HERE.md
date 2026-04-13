# 🤖 AI 接手指南 — AnyClaw LVGL

> 如果你是第一次打开这个仓库的 AI，按以下顺序读文件，然后开始干活。

## 第一步：读这 3 个文件（按顺序）

| 顺序 | 文件 | 用途 |
|------|------|------|
| ① | `tasks/MEMORY.md` | 长期记忆 — 项目现状、进度、阻塞项 |
| ② | `tasks/v2.2.1-dev.md` | 任务清单 — 找到第一个 ⏳ 状态的任务 |
| ③ | `AGENTS.md`（仓库根目录） | 开发规范 — 编译命令、提交格式、安全红线 |

## 第二步：开始干活

1. 在 `tasks/v2.2.1-dev.md` 中找到第一个 ⏳ 的任务
2. 按 AGENTS.md 的工作流执行：
   - 改代码 → 编译验证 → 改文档 → 验证一致性 → commit + push
3. 每完成一个任务，更新 `tasks/v2.2.1-dev.md` 状态 ⏳ → ✅
4. 每次会话结束，写一份会话记录到 `tasks/session-YYYY-MM-DD.md`

## 关键路径速查

```
AnyClaw_LVGL/
├── AGENTS.md              ← 开发规范（必读）
├── CMakeLists.txt         ← 构建配置
├── src/
│   ├── theme.h            ← ThemeColors 结构体 + Theme enum
│   ├── anim.h/anim.cpp    ← 动画工具模块
│   ├── sound.h/sound.cpp  ← 音效系统
│   ├── ui_main.cpp        ← 主 UI 代码 + 5 套主题定义
│   ├── ui_settings.cpp    ← 设置页
│   └── garlic_dock.cpp    ← 大蒜头贴图
├── tasks/
│   ├── START_HERE.md      ← 你在这里
│   ├── MEMORY.md          ← 长期记忆
│   ├── v2.2.1-dev.md      ← 任务清单（核心）
│   ├── v2.2.1-scan.md     ← Bug 扫描记录
│   └── session-*.md       ← 会话记录
├── docs/
│   ├── PRD.md             ← 产品需求
│   └── Design.md          ← 设计规范（色板/字体/控件/动效）
└── assets/                ← 运行时资源（主题子目录已创建）
```

## 编译命令

```bash
# Linux 交叉编译 + 打包（主力，需要 mingw-w64）
bash build/linux/build.sh v2.2.1

# 文档一致性检查
bash scripts/verify-docs.sh
```

## 提交格式

```
<类型>(<模块>): <简述，50字以内>

相关: <PRD功能编号或UI编号>
```

类型：feat / fix / docs / refactor / build / chore
