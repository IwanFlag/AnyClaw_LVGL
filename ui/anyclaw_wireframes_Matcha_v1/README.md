# AnyClaw LVGL — 抹茶 Matcha v1 主题 Wireframe 集

> 🍵 **主打主题 · 默认** | 抹茶 Matcha (ClawMint) | 版本：v2.2.1 | 更新时间：2026-04-12
> 配合 `AnyClaw_LVGL_PRD_v2.2.1_Design_System.md` 使用
>
> 本主题为 AnyClaw 默认主打主题，完整规格见 Design.md §2.2。

---

## 设计关键词

**简单但不简陋，活泼但不花哨，专业但不冰冷**

## 色彩系统（Matcha Dark）

| 语义 Token | 色值 | 用途 |
|------------|------|------|
| `bg` | `#0F1117` | 窗口最底层背景 |
| `surface` | `#13151E` | 输入区、低层容器 |
| `panel` | `#1A1D27` | 面板、卡片、弹窗 |
| `raised` | `#1E2230` | 气泡、浮层、tooltip |
| `overlay` | `#252A38` | hover 态、选中态 |
| `text` | `#E8ECF4` | 主文字、标题 |
| `text2` | `#8B92A8` | 次要文字 |
| `text3` | `#6B7280` | 占位符、禁用态 |
| `accent` | `#3DD68C` | 主强调色（薄荷绿） |
| `danger` | `#FF6B6B` | 错误、删除 |
| `warning` | `#FFBE3D` | 警告、Busy |
| `info` | `#6C9FFF` | 信息、Checking |
| `bubble_user` | `#2B5C3E→#1F4A30` | 用户消息气泡 |
| `bubble_ai` | `#1E2230` | AI 消息气泡 |

## 字体

| 类型 | 主选 | 回退 |
|------|------|------|
| 中文 | HarmonyOS Sans SC | PingFang SC, 微软雅黑, sans-serif |
| 英文 | Plus Jakarta Sans | Inter, Segoe UI, sans-serif |
| 等宽 | JetBrains Mono | Consolas, monospace |

> 清新锐利风。Plus Jakarta Sans 比 Montserrat 更圆润现代。

## UI 场景清单（65 个）

### A. 启动流程（10 个）

| 编号 | 文件 | 说明 |
|------|------|------|
| UI-01 | `UI-01-selfcheck` | 启动自检 — 4项检查 LED |
| UI-02 | `UI-02-already-running` | 已运行提示（Win32 MessageBox） |
| UI-03 | `UI-03-startup-error` | 启动错误阻断弹窗 |
| UI-04 | `UI-04-wizard-lang` | 向导 Step 1 — 语言选择 |
| UI-05 | `UI-05-wizard-env` | 向导 Step 2 — 环境检测 |
| UI-06 | `UI-06-wizard-model` | 向导 Step 3 — 模型 & API Key |
| UI-07 | `UI-07-wizard-gemma` | 向导 Step 4 — 本地模型 |
| UI-08 | `UI-08-wizard-im` | 向导 Step 5 — 连接 IM |
| UI-09 | `UI-09-wizard-profile` | 向导 Step 6 — 个人信息 |
| UI-10 | `UI-10-workspace-template` | 工作区模板选择 |

### B. 主界面框架（3 个）

| 编号 | 文件 | 说明 |
|------|------|------|
| UI-11 | `UI-11-chat-mode` | 主界面 — Chat 模式 |
| UI-12 | `UI-12-work-mode` | 主界面 — Work 模式 |
| UI-13 | `UI-13-edge-garlic` | 贴边大蒜头 |

### C. Chat 组件（9 个）

| 编号 | 文件 | 说明 |
|------|------|------|
| UI-14 | `UI-14-user-bubble` | 用户消息气泡 |
| UI-15 | `UI-15-ai-bubble` | AI 消息气泡 |
| UI-16 | `UI-16-typing-indicator` | 流式打字指示器 |
| UI-17 | `UI-17-chat-input` | Chat 输入区 |
| UI-18 | `UI-18-file-attachment` | 文件附件卡片 |
| UI-19 | `UI-19-chat-empty` | Chat 空状态 |
| UI-20 | `UI-20-chat-search` | Chat 搜索 |
| UI-21 | `UI-21-mode-switcher` | 模式切换器 |
| UI-22 | `UI-22-ai-mode-selector` | AI 交互模式选择器 |

### D. Work 组件（8 个）

| 编号 | 文件 | 说明 |
|------|------|------|
| UI-23 | `UI-23-stepcard-read` | StepCard — 读操作 |
| UI-24 | `UI-24-stepcard-write` | StepCard — 写操作 |
| UI-25 | `UI-25-stepcard-exec` | StepCard — 执行命令 |
| UI-26 | `UI-26-output-panel` | 输出面板（终端/文档/表格） |
| UI-27 | `UI-27-work-empty` | Work 空状态 |
| UI-28 | `UI-28-diff-view` | Diff 可视化 |
| UI-29 | `UI-29-plan-mode` | Plan 模式方案展示 |
| UI-30 | `UI-30-work-input` | Work 任务输入框 |

### E. 弹窗 & 浮层（14 个）

| 编号 | 文件 | 说明 |
|------|------|------|
| UI-31 | `UI-31-permission` | 权限确认弹窗 |
| UI-32 | `UI-32-ask-feedback` | Ask 模式反馈弹窗 |
| UI-33 | `UI-33-license` | License 激活弹窗 |
| UI-34 | `UI-34-exit-confirm` | 退出确认弹窗 |
| UI-35 | `UI-35-custom-model` | 自定义模型添加弹窗 |
| UI-36 | `UI-36-file-upload` | 文件上传菜单 |
| UI-37 | `UI-37-toast` | Toast 通知 |
| UI-38 | `UI-38-alert` | 警报弹窗 |
| UI-39 | `UI-39-workspace-lock` | 工作区锁冲突 |
| UI-40 | `UI-40-workspace-missing` | 工作区目录丢失 |
| UI-41 | `UI-41-workspace-detail` | 工作区详情弹窗 |
| UI-42 | `UI-42-gemma-download` | Gemma 下载进度 |
| UI-43 | `UI-43-app-auth` | 应用授权界面 |
| UI-44 | `UI-44-boot-check` | Boot Check 进度条 |

### F. 设置页（7 个 Tab）

| 编号 | 文件 | 说明 |
|------|------|------|
| UI-45 | `UI-45-general-tab` | General Tab |
| UI-46 | `UI-46-permissions-tab` | Permissions Tab |
| UI-47 | `UI-47-model-tab` | Model Tab |
| UI-48 | `UI-48-log-tab` | Log Tab |
| UI-49 | `UI-49-about-tab` | About Tab |
| UI-50 | `UI-50-feature-flags` | Feature Flags Tab |
| UI-51 | `UI-51-tracing-tab` | Tracing Tab |

### G. 外观系统（4 个）

| 编号 | 文件 | 说明 |
|------|------|------|
| UI-52 | `UI-52-dark-theme` | 暗色主题（Matcha Dark） |
| UI-53 | `UI-53-light-theme` | 亮色主题 |
| UI-54 | `UI-54-classic-theme` | 经典暗色主题 |
| UI-55 | `UI-55-widget-system` | 控件体系合集 |

### H. 系统管理（3 个）

| 编号 | 文件 | 说明 |
|------|------|------|
| UI-56 | `UI-56-crash-log` | 崩溃日志查看 |
| UI-57 | `UI-57-update-check` | 更新检测 & 下载 |
| UI-58 | `UI-58-tray-bubble` | 托盘气泡通知 |

### I. 远程协作 & 高级功能（2 个）

| 编号 | 文件 | 说明 |
|------|------|------|
| UI-59 | `UI-59-remote-assist` | 远程协助对话框 |
| UI-60 | `UI-60-knowledge-base` | 知识库面板 |

### J. 其他（3 个）

| 编号 | 文件 | 说明 |
|------|------|------|
| UI-61 | `UI-61-reserved` | 预留 |
| UI-62 | `UI-62-reserved` | 预留 |
| UI-63 | `UI-63-tray-menu` | 系统托盘菜单 |
| UI-64 | `UI-64-workspace-switcher` | 工作区切换器 |
| UI-65 | `UI-65-skill-manager` | Skill 管理面板 |

---

## 文件格式

- `.svg` — 矢量源文件，可用浏览器或 Inkscape 打开编辑
- `.png` — 位图预览（1450×800 或自适应尺寸）

## 主题对照

| 主题 | 命名 | 主色 | 定位 |
|------|------|------|------|
| Matcha Dark | 抹茶 | `#3DD68C` 薄荷绿 | 主力主题，年轻活力 |
| Peachy | 桃气 | 待补充 | 暖色系备选 |
| Classic | 经典 | `#82AAF0` 蓝色 | 怀旧/低饱和度 |
