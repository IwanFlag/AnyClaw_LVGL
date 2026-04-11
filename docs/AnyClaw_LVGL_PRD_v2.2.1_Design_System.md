# AnyClaw LVGL PRD v2.2.1 — UI 设计系统规范（Design System）

> 版本：v2.2.1 | 目标用户：大学生 & 职场年轻人 | 更新时间：2026-04-12
> 配合 `AnyClaw_LVGL_PRD_v2.2.1_UI_layouts.md` 使用 — Layout 文档定义"页面长什么样"，本文档定义"每个零件怎么画"。

---

## 目录

1. [设计原则](#1-设计原则)
2. [色彩系统](#2-色彩系统)
3. [字体排版系统](#3-字体排版系统)
4. [间距与栅格系统](#4-间距与栅格系统)
5. [控件库](#5-控件库)
6. [窗口系统](#6-窗口系统)
7. [动效系统](#7-动效系统)
8. [音效系统](#8-音效系统)
9. [图标系统](#9-图标系统)
10. [DPI 适配](#10-dpi-适配)
11. [国际化样式](#11-国际化样式)

---

## 1. 设计原则

### 1.1 目标用户画像

| 维度 | 大学生 | 职场年轻人 |
|------|--------|-----------|
| 年龄 | 18-24 | 23-30 |
| 设备习惯 | 手机优先, 熟悉 Material/iOS | 桌面为主, 用过 VS Code/Notion |
| 审美偏好 | 鲜活、个性、社交感 | 简洁、专业、高效 |
| 容忍度 | 低 — 加载慢就走 | 中 — 但讨厌卡顿 |
| 核心诉求 | 免费 + 好看 + 能用 | 效率 + 稳定 + 专业 |

### 1.2 设计关键词

**简单但不简陋，活泼但不花哨，专业但不冰冷**

- **层次分明** — 5 层深度系统，信息有主次
- **即时反馈** — 每个操作 200ms 内给出视觉/听觉响应
- **个性表达** — 大蒜龙虾品牌角色贯穿，有温度
- **年轻人友好** — 圆角大、色彩明快、动画流畅

### 1.3 参考设计语言

| 参考 | 借鉴什么 |
|------|---------|
| Linear | 极简深色 + 精致微交互 |
| Raycast | 快速搜索 + 命令面板 |
| Discord | 聊天气泡 + 社区感 |
| Notion | 卡片化 + 信息层级 |
| macOS | 圆角统一 + 动画曲线 |

---

## 2. 色彩系统

### 2.1 主题切换

PRD 定义三套主题，通过 General Tab 切换，即时生效无需重启。

| 主题 ID | 名称 | 定位 | 默认 |
|---------|------|------|------|
| `dark` | 暗色 (薄荷暗夜) | 主力主题，长时间使用 | ✅ 默认 |
| `light` | 亮色 | 日间/高亮环境 | |
| `classic` | 经典暗色 | 怀旧/低饱和度偏好 | |

### 2.2 暗色主题 — 薄荷暗夜 (Mint Dark)

> 面向年轻人的推荐方案，薄荷绿 accent + 深色层次

#### 2.2.1 基础色板

| 语义 Token | 色值 | RGB | 用途 |
|------------|------|-----|------|
| `bg` | `#0F1117` | 15,17,23 | 窗口最底层背景 |
| `surface` | `#13151E` | 19,21,30 | 输入区、低层容器 |
| `panel` | `#1A1D27` | 26,29,39 | 面板、卡片、弹窗 |
| `raised` | `#1E2230` | 30,34,48 | 气泡、浮层、tooltip |
| `overlay` | `#252A38` | 37,42,56 | hover 态、选中态 |

#### 2.2.2 文字色

| Token | 色值 | 用途 |
|-------|------|------|
| `text_primary` | `#E8ECF4` | 主文字、标题 |
| `text_secondary` | `#8B92A8` | 次要文字、说明 |
| `text_tertiary` | `#6B7280` | 占位符、禁用态 |
| `text_inverse` | `#0F1117` | 深色背景上的白字（按钮内） |

#### 2.2.3 强调色 (Accent)

| Token | 色值 | 用途 |
|-------|------|------|
| `accent` | `#3DD68C` | 主强调色、品牌色、成功 |
| `accent_hover` | `#2BB673` | 主按钮 hover 态 |
| `accent_active` | `#1FA35E` | 主按钮 active/pressed |
| `accent_subtle` | `#3DD68C1A` | 背景着色（10% opacity） |
| `accent_secondary` | `#6C9FFF` | 辅助蓝、链接、次要高亮 |

#### 2.2.4 语义色

| Token | 色值 | 用途 |
|-------|------|------|
| `success` | `#3DD68C` | 成功、完成、Ready 状态 |
| `warning` | `#FFBE3D` | 警告、Busy 状态、磁盘不足 |
| `danger` | `#FF6B6B` | 错误、删除、Error 状态、关闭按钮 |
| `info` | `#6C9FFF` | 信息提示、Checking 状态 |

#### 2.2.5 功能色

| Token | 色值 | 用途 |
|-------|------|------|
| `border` | `#3DD68C0A` | 面板边框（极淡 accent） |
| `border_strong` | `#3DD68C20` | 输入框焦点边框 |
| `divider` | `#3DD68C08` | 分隔线 |
| `focus_glow` | `#3DD68C30` | 焦点光晕 |
| `hover_overlay` | `#FFFFFF0A` | 通用 hover 叠加层 |
| `active_overlay` | `#FFFFFF14` | 通用 active 叠加层 |
| `disabled_bg` | `#1A1D2780` | 禁用态背景（50% opacity） |
| `disabled_text` | `#6B728080` | 禁用态文字 |

#### 2.2.6 阴影

| Token | 值 | 用途 |
|-------|-----|------|
| `shadow_sm` | `0 1px 3px rgba(0,0,0,0.2)` | 卡片轻微浮起 |
| `shadow_md` | `0 2px 8px rgba(0,0,0,0.25)` | 气泡、tooltip |
| `shadow_lg` | `0 8px 24px rgba(0,0,0,0.35)` | 弹窗、dropdown |
| `shadow_glow` | `0 0 12px rgba(61,214,140,0.15)` | 焦点/品牌发光 |

#### 2.2.7 用户气泡专用色

| Token | 色值 | 用途 |
|-------|------|------|
| `bubble_user_bg` | `#2B5C3E` | 用户消息气泡底色 |
| `bubble_user_bg_end` | `#1F4A30` | 用户消息气泡渐变终点 |
| `bubble_user_text` | `#E8ECF4` | 用户消息文字 |
| `bubble_user_time` | `#B8D8C8` | 用户消息时间戳 |
| `bubble_ai_bg` | `#1E2230` | AI 消息气泡底色 |
| `bubble_ai_accent_bar` | `#3DD68C` | AI 气泡左侧竖线 |
| `bubble_ai_text` | `#E8ECF4` | AI 消息文字 |
| `bubble_ai_time` | `#6B7280` | AI 消息时间戳 |

### 2.3 亮色主题

| 语义 Token | 色值 | 说明 |
|------------|------|------|
| `bg` | `#F5F7FA` | |
| `surface` | `#EBEEF2` | |
| `panel` | `#FFFFFF` | |
| `raised` | `#FFFFFF` | |
| `overlay` | `#E2E5EA` | |
| `text_primary` | `#1A1D27` | |
| `text_secondary` | `#6B7280` | |
| `accent` | `#2BB673` | 深一点确保对比度 |
| `danger` | `#DC2626` | |
| `border` | `#E2E5EA` | |

> 亮色主题完整色板约 40 个 Token，此处仅列核心。切换时 300ms 全局色值过渡。

### 2.4 经典暗色主题

| 语义 Token | 色值 | 说明 |
|------------|------|------|
| `bg` | `#2D2D2D` | |
| `panel` | `#3C3C3C` | |
| `text_primary` | `#FFFFFF` | |
| `accent` | `#82AAF0` | 保持原蓝色方案 |

### 2.5 色彩使用规则

| 规则 | 说明 |
|------|------|
| 同层元素不加边框 | `panel` 上的 `card` 不加 border |
| 相邻层 1px 分隔线 | `divider` 色，opacity 4-8% |
| 弹窗从 `panel` 层开始 | 遮罩 `rgba(0,0,0,0.6)` |
| 最多同时使用 3 种强调色 | accent + 1 语义色 + 1 功能色 |
| 红色只用于危险/错误 | 不用于装饰 |

---

## 3. 字体排版系统

### 3.1 字体栈

| 类型 | 主选 | 备选 | 回退 |
|------|------|------|------|
| 中文 | HarmonyOS Sans SC | 思源黑体 (Source Han Sans) | 微软雅黑, sans-serif |
| 英文 | Inter | SF Pro Display | Segoe UI, sans-serif |
| 等宽 | JetBrains Mono | Fira Code | Consolas, monospace |
| 图标 | LVGL Symbols | — | — |

> PRD 当前实现使用 Montserrat + 微软雅黑。建议 v2.3 升级到 Inter + HarmonyOS Sans（更现代，免费商用）。

### 3.2 字号体系

| Token | 字号 | 行高 | 字重 | 用途 |
|-------|------|------|------|------|
| `display` | 28px | 36px | 700 (Bold) | 品牌标题、About 页大标题 |
| `h1` | 22px | 30px | 700 | 弹窗标题、设置页标题 |
| `h2` | 18px | 26px | 600 (SemiBold) | 区域标题 (SESSIONS, CRON TASKS) |
| `h3` | 15px | 22px | 600 | 卡片标题、Step 标题 |
| `body` | 13px | 20px | 400 (Regular) | 正文、消息内容 |
| `body_strong` | 13px | 20px | 600 | 强调正文 |
| `small` | 11px | 16px | 400 | 次要信息、时间戳 |
| `caption` | 10px | 14px | 500 (Medium) | 标签、胶囊状态、注释 |
| `code` | 12px | 18px | 400 | 代码块内文字 |

### 3.3 文字颜色使用

| 场景 | Token | 说明 |
|------|-------|------|
| 标题 | `text_primary` | |
| 正文 | `text_primary` | |
| 说明文字 | `text_secondary` | |
| 占位符 | `text_tertiary` | 输入框 placeholder |
| 时间戳 | `text_tertiary` | 消息时间 |
| 禁用文字 | `disabled_text` | |
| 按钮内文字 | `text_inverse` 或 `text_primary` | 取决于按钮背景 |
| 链接 | `accent` | 可点击的文本 |
| 错误文字 | `danger` | 表单验证错误 |
| 代码注释 | `text_tertiary` | |
| 代码关键字 | `accent` | |
| 代码字符串 | `warning` | |

### 3.4 文字排版规则

| 规则 | 值 |
|------|-----|
| 中文 CJK 逐字换行 | `LV_TEXT_FLAG_BREAK_ALL` |
| 最大行宽 | 70% 容器（消息气泡） |
| 段落间距 | 8px |
| 列表项间距 | 4px |
| 文字截断 | 尾部 `...` 省略 |
| 多行截断 | 最多 3 行，尾部 `...` |
| 代码块背景 | `surface` 色, 圆角 8px, padding 12px |
| 代码块语言标签 | 右上角 `caption` 字号, `text_tertiary` |
| 选中文字高亮 | `accent` 色, opacity 30% |

### 3.5 Markdown 渲染样式

| 元素 | 样式 |
|------|------|
| `# H1` | `h1` + `text_primary` + 上边距 16px |
| `## H2` | `h2` + `text_primary` + 上边距 12px |
| `### H3` | `h3` + `text_primary` + 上边距 8px |
| **加粗** | `body_strong` |
| *斜体* | `body` + 倾斜（LVGL 有限支持） |
| `行内代码` | `code` + `surface` 背景 + 圆角 4px + padding 2px 4px |
| ``` 代码块 ``` | `code` + `surface` 背景 + 圆角 8px + padding 12px |
| - 列表 | `body` + 左缩进 16px + 项目符号 |
| 1. 有序列表 | `body` + 左缩进 16px + 数字 |
| > 引用 | `body` + `text_secondary` + 左侧 3px accent 竖线 |
| [链接](url) | `accent` + 下划线 |
| --- 分隔线 | 1px `divider` + 上下 12px |

---

## 4. 间距与栅格系统

### 4.1 基础间距单位

基础单位: **4px**

| Token | 值 | 用途 |
|-------|-----|------|
| `space_1` | 4px | 最小间距、图标与文字 |
| `space_2` | 8px | 内边距、相关元素间距 |
| `space_3` | 12px | 区域内元素间距 |
| `space_4` | 16px | 卡片内边距、段落间距 |
| `space_5` | 20px | 弹窗内边距 |
| `space_6` | 24px | 区域间间距 |
| `space_8` | 32px | 大区域分隔 |

### 4.2 圆角体系

| Token | 值 | 用途 |
|-------|-----|------|
| `radius_sm` | 4px | 行内代码、小标签 |
| `radius_md` | 8px | 按钮、输入框、小卡片 |
| `radius_lg` | 12px | 卡片、面板、下拉框 |
| `radius_xl` | 16px | 消息气泡、大卡片 |
| `radius_2xl` | 20px | 弹窗、模态框 |
| `radius_full` | 9999px | 胶囊标签、圆形头像 |

### 4.3 PRD 定义的尺寸常量

> 以下来自 `app_config.h`，所有值为基础像素，运行时乘以 `dpi_scale`

| 类别 | 常量 | 值 | 说明 |
|------|------|-----|------|
| **窗口** | `WIN_DEFAULT_PCT` | 75% | 默认窗口占屏幕比例 |
| | `WIN_MIN_W/H` | 800/500 | 窗口最小尺寸 |
| | `TITLE_BAR_H_BASE` | 48px | 标题栏高度 |
| **面板** | `LEFT_PANEL_DEFAULT_W` | 240px | 左面板默认宽度 |
| | `PANEL_MIN_W` | 200px | 面板最小宽度 |
| | `SPLITTER_W_BASE` | 6px | 分隔条宽度 |
| **气泡** | `CHAT_BUBBLE_MAX_PCT` | 70% | 气泡最大宽度 |
| | `CHAT_BUBBLE_PAD_H/V` | 8/6px | 气泡内边距 |
| | `CHAT_BUBBLE_RADIUS` | 10 | 气泡圆角 |
| | `CHAT_GAP` | 6px | 消息间距 |
| | `CHAT_MSG_MARGIN` | 8px | 消息边距 |
| **输入** | `CHAT_INPUT_DEFAULT_H` | 96px | 输入区默认高度 |
| **按钮** | `BTN_SEND_SIZE_BASE` | 24px | 发送按钮基础尺寸 |
| **弹窗** | `DIALOG_PAD_ALL` | 16px | 弹窗内边距 |
| | `DIALOG_RADIUS` | 12 | 弹窗圆角 |
| **设置** | `SETTINGS_ROW_H` | 32px | 设置行高度 |
| **下拉** | `DROPDOWN_RADIUS` | 6 | 下拉框圆角 |
| **图标** | ICON_TINY~XLARGE | 16/20/24/32/48px | 图标层级 |
| **Session** | `MAX_TASK_WIDGETS` | 12 | 左面板最大任务数 |
| **Task 行** | 高度/间距 | 32px / 12px | 左面板任务行 |
| **Tooltip** | 宽度 | 320px | 任务详情浮层 |

---

## 5. 控件库

### 5.1 按钮 (Button)

#### 5.1.1 主按钮 (Primary / btn_action)

```
  ┌──────────────────────────┐
  │       Get Started        │  ← 高度: 36px, 圆角: 8px
  └──────────────────────────┘  ← 内边距: 12px 24px

  Default:  bg=accent (#3DD68C), text=text_inverse (#0F1117), font=body_strong
  Hover:    bg=accent_hover (#2BB673), cursor=hand
  Active:   bg=accent_active (#1FA35E), transform=scale(0.97)
  Disabled: bg=disabled_bg, text=disabled_text, cursor=default
  Focus:    box-shadow=0 0 0 2px focus_glow, outline=none
```

#### 5.1.2 次要按钮 (Secondary / btn_secondary)

```
  ┌──────────────────────────┐
  │        < Back            │  ← 高度: 36px, 圆角: 8px
  └──────────────────────────┘

  Default:  bg=transparent, border=1px accent, text=accent, font=body_strong
  Hover:    bg=accent_subtle
  Active:   bg=accent_subtle + active_overlay
  Disabled: border=disabled_text, text=disabled_text
```

#### 5.1.3 幽灵按钮 (Ghost)

```
  ┌──────────────────────────┐
  │       退出向导            │  ← 高度: 36px, 圆角: 8px
  └──────────────────────────┘

  Default:  bg=transparent, text=text_secondary
  Hover:    bg=hover_overlay, text=text_primary
  Active:   bg=active_overlay
```

#### 5.1.4 危险按钮 (Danger / btn_close)

```
  ┌──────────────────────────┐
  │         拒绝              │  ← 高度: 36px, 圆角: 8px
  └──────────────────────────┘

  Default:  bg=danger (#FF6B6B), text=white, font=body_strong
  Hover:    bg=#E55A5A
  Active:   bg=#CC4D4D
```

#### 5.1.5 图标按钮 (Icon Button)

```
  ┌────┐
  │ ⚙️ │  ← 28×28px, 圆角: 6px
  └────┘

  Default:  bg=transparent, icon=text_secondary (14px)
  Hover:    bg=hover_overlay, icon=text_primary
  Active:   bg=active_overlay
```

#### 5.1.6 发送按钮 (Send Button)

```
      ┌──┐
      │▶ │  ← 40×40px, 圆形
      └──┘

  Default:  bg=accent gradient, icon=text_inverse (16px), shadow=shadow_md
  Hover:    bg=accent_hover, shadow=shadow_glow
  Active:   bg=accent_active, transform=scale(0.95)
  Sending:  bg=accent, icon=⏹ (停止), 可点击中断流式
```

#### 5.1.7 Abort 按钮

```
  ┌────────┐
  │  [✕]   │  ← 28×22px, 圆角: 4px
  └────────┘

  Default:  bg=#B43232 (PRD 原值), text=white, font=caption
  Hover:    bg=#CC3B3B
```

#### 5.1.8 窗口控制按钮组

```
  [⚙] [—] [□] [✕]  ← 每个 46×48px (占满标题栏高度)

  Default:  bg=transparent, icon=text_secondary
  Hover ⚙:  bg=hover_overlay, icon=text_primary
  Hover —:  bg=hover_overlay, icon=success
  Hover □:  bg=hover_overlay, icon=warning
  Hover ✕:  bg=danger, icon=white
  Active:   bg=active_overlay
```

### 5.2 输入框 (Input)

#### 5.2.1 单行输入框

```
  ┌──────────────────────────────────────┐
  │  输入昵称...                         │  ← 高度: 36px, 圆角: 8px
  └──────────────────────────────────────┘  ← 内边距: 8px 12px

  Default:   bg=surface, border=1px border, text=text_primary, placeholder=text_tertiary
  Hover:     border=border_strong
  Focus:     border=accent (2px), box-shadow=0 0 0 3px focus_glow
  Error:     border=danger, 底部显示 error 文字 (caption, danger)
  Disabled:  bg=disabled_bg, text=disabled_text, border=border
  Font:      body (13px), placeholder=body + text_tertiary
```

#### 5.2.2 密码输入框 (API Key)

```
  ┌──────────────────────────────────────┐
  │  ••••••••••••••••••••               │  ← 同单行输入框
  └──────────────────────────────────────┘

  额外: 右侧 [👁] 切换明文/密码显示按钮
```

#### 5.2.3 多行输入框 (Chat/Work 输入区)

```
  ┌──────────────────────────────────────────────┐
  │  输入消息... (Shift + Enter 换行)            │  ← 最小高度: 96px
  │                                              │     圆角: 12px
  │                                              │     自动增长: 按行数
  └──────────────────────────────────────────────┘

  Default:   bg=panel, border=1px border
  Focus:     border=accent (1.5px), box-shadow=0 0 0 2px focus_glow
  自动增长:  监听行数变化, 调整高度, 保存/恢复 scroll_y
  Font:      body (13px)
  内边距:    12px
```

### 5.3 下拉框 (Dropdown)

```
  ┌──────────────────────────────────────┐
  │  gemma-3-4b-it:free             ▾   │  ← 高度: 36px, 圆角: 8px
  └──────────────────────────────────────┘

  Default:   bg=surface, border=1px border, text=text_primary
  Hover:     border=border_strong
  Focus/Open: border=accent, ▾ 旋转 180° (200ms)
  下拉面板:  bg=panel, border=1px border, shadow=shadow_lg, 圆角=12px
  下拉项:    高度 32px, padding 8px 12px, hover=hover_overlay
  选中项:    accent 文字 + 左侧 accent 圆点 (6px)
  Font:      body (13px)
```

### 5.4 开关 (Switch / Toggle)

```
        关: ○──────    开: ──────●
  ┌────────────────┐  ┌────────────────┐
  │    ○──────     │  │     ──────●    │  ← 高度: 20px, 宽度: 36px
  └────────────────┘  └────────────────┘     圆角: 10px

  Off:     bg=#373C55, thumb=#8B92A8, 圆形 16px
  On:      bg=accent, thumb=white, 圆形 16px
  动画:    200ms ease-out 滑动
  Hover:   阴影加深
  Disabled: bg=disabled_bg, thumb=disabled_text
```

### 5.5 复选框 (Checkbox)

```
  ☐ 未选中    ☑ 已选中    ☒ 禁用

  尺寸: 18×18px, 圆角: 4px
  Off:     bg=surface, border=1px border_strong, 内容空
  On:      bg=accent, 内部白色 ✓ (12px)
  动画:    150ms scale(0.8→1.0) + fade-in
  Disabled: bg=disabled_bg, border=disabled_text
```

### 5.6 步骤指示器 (Step Indicator)

```
  ●──●──○──○──○──○  Step 1/6

  已完成: ● accent 色圆点 (10px)
  当前:   ● accent 色圆点 (12px) + glow
  未完成: ○ border_strong 色圆点 (10px)
  连接线: 2px, 已完成段=accent, 未完成段=border
  文字:   "Step N/6", caption + text_secondary
  动画:   步骤切换时 300ms 连接线渐变延伸
```

### 5.7 状态胶囊 (Status Badge)

```
  ┌──────────┐
  │ RUNNING  │  ← 高度: 18px, 圆角: 9px, padding: 0 8px
  └──────────┘     font: caption (10px) + bold

  Ready/Running: bg=accent_subtle, text=accent
  Busy:          bg=#FFBE3D1A, text=warning
  Error:         bg=#FF6B6B1A, text=danger
  Checking:      bg=#6C9FFF1A, text=info + pulse 动画
```

### 5.8 进度条 (Progress Bar)

```
  ┌────────────────────────────────────────────┐
  │████████████████████░░░░░░░░  62%           │  ← 高度: 6px, 圆角: 3px
  └────────────────────────────────────────────┘

  背景:     surface
  填充:     accent gradient (左→右)
  圆角:     3px
  动画:     填充段平滑过渡 300ms
  带文字:   右侧 body + text_secondary 显示百分比
  不确定:   accent 色从左到右滑动 (shimmer, 1.5s 循环)
```

### 5.9 加载指示器 (Spinner)

```
  脉冲圆点 (聊天用):
  ●  ●  ●   ← 3 个 6px 圆点, accent 色, 依次脉冲 1.2s 循环

  旋转圆环 (通用):
  ◜    ← 16px, 2px 描边, accent 色, 旋转 0.8s 线性循环
```

### 5.10 卡片 (Card)

```
  ┌──────────────────────────────────┐
  │  内容区域                        │  ← 圆角: 12px
  │                                  │     padding: 12px
  │                                  │     bg: panel
  └──────────────────────────────────┘

  Default:   bg=panel, border=1px border (可选)
  Hover:     bg=raised, shadow=shadow_sm
  Active:    bg=overlay
  Selected:  border=accent (1px), bg=panel
  可选属性:  header, footer, cover_image
```

### 5.11 列表项 (List Item)

```
  ┌──────────────────────────────────────────────┐
  │  [icon]  标题文字                  [action]  │  ← 高度: 32px (紧凑) / 44px (标准)
  │           副标题文字                         │     padding: 0 12px
  └──────────────────────────────────────────────┘

  Default:   bg=transparent, text=text_primary
  Hover:     bg=hover_overlay
  Active:    bg=active_overlay
  Selected:  bg=accent_subtle, 左侧 2px accent 竖线
  Font 标题: body (13px)
  Font 副标: small (11px) + text_secondary
```

### 5.12 Tooltip

```
  ┌────────────────────────────────────┐
  │  提示文字内容                      │  ← 圆角: 8px, padding: 6px 10px
  └────────────────────────────────────┘  bg: raised, shadow: shadow_md
                                          font: small (11px), text: text_primary
  触发: hover 500ms 延迟
  位置: 目标元素上方/下方, 居中对齐
  消失: 移开即消失, 200ms fade-out
  最大宽度: 320px (任务详情) / 200px (通用)
```

### 5.13 上下文菜单 (Context Menu)

```
  ┌────────────────────────────┐
  │  📋 复制                   │  ← 圆角: 10px
  │  ✂️ 剪切                   │     item 高度: 32px
  │  📋 粘贴                   │     padding: 4px 0
  │  ──────────────────────── │
  │  🗑️ 删除                   │  ← 分隔线: 1px divider
  └────────────────────────────┘     bg: panel, shadow: shadow_lg

  默认:     bg=panel
  Hover 项: bg=hover_overlay
  快捷键:   右侧对齐, font=caption + text_tertiary
  危险项:   text=danger
```

### 5.14 Tab 切换

```
  ┌──────────────────────────────────────────────┐
  │  General  │ Permissions │ Model │  Log       │  ← 高度: 36px
  │  ──────── │                                 │     underline: 2px accent
  └──────────────────────────────────────────────┘

  Active:   text=accent, 底部 2px accent 下划线
  Inactive: text=text_secondary
  Hover:    text=text_primary
  动画:     下划线 200ms 滑动到新位置
```

### 5.15 分隔条 (Splitter)

```
  │  ← 6px 宽

  Default:   bg=surface
  Hover:     bg=accent, opacity=0.3, cursor=col-resize
  Dragging:  bg=accent, opacity=0.6
  拖拽时:    面板半透明 85% opacity, 释放时回弹
```

---

## 6. 窗口系统

### 6.1 窗口框架

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│ 标题栏 (Title Bar)                         48px                                 │
├─────────────────────────────────────────────────────────────────────────────────┤ 1px divider
│                                                                                 │
│ 左侧面板 (240px)   │ 分隔条(6px) │              右侧内容区                      │
│                     │             │                                               │
│                     │             │                                               │
│                     │             │                                               │
│                     │             │                                               │
└─────────────────────────────────────────────────────────────────────────────────┘
```

**窗口属性**:
- 设计尺寸: 1450×800 (逻辑像素)
- 实际尺寸: 屏幕 75%×75%, 上限=屏幕-边距(宽-60, 高-80)
- 最小尺寸: 800×500
- 关闭行为: 最小化到托盘 (非退出)
- 位置记忆: config.json 保存 window_x/window_y, 重启恢复

### 6.2 标题栏 (Title Bar)

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│ 🧄 AnyClaw       [ Chat  Work ]              [⚙] [—] [□] [✕]                │
│ ←12px→←品牌→←16px→←模式切换→←flex→                    ←窗口按钮 184px→        │
└─────────────────────────────────────────────────────────────────────────────────┘

  总高度: 48px
  背景: panel (#1A1D27) 渐变 (顶到底 1°)
  底部边框: 1px divider
```

**子元素详细规格**:

| 元素 | 尺寸 | 位置 | 说明 |
|------|------|------|------|
| 品牌图标 🧄 | 22px | x=12, 垂直居中 | |
| 品牌名 AnyClaw | font=h3, text=text_primary | x=40, 垂直居中 | 半粗体, 不显示 "LVGL" |
| 模式切换器 | 150×28px | 品牌右侧 16px | 胶囊式, bg=surface |
| 弹性空间 | flex | 中间 | 拖拽区域 (WM_NCHITTEST→HTCAPTION) |
| 设置按钮 ⚙ | 46×48px | 右侧起始 | icon=14px |
| 最小化 — | 46×48px | 设置右侧 | |
| 最大化 □ | 46×48px | 最小化右侧 | |
| 关闭 ✕ | 46×48px | 最右侧 | hover bg=danger |

**拖拽排除区**: 窗口按钮区域 (约 184px) 从拖拽区域排除, 由 `app_update_btn_exclude()` 动态更新。

### 6.3 模式切换器详细

```
  ┌──────────────────────────────┐
  │ ┌────────┐ ┌───────────────┐ │
  │ │  Chat  │ │     Work      │ │  ← 胶囊背景: surface
  │ └────────┘ └───────────────┘ │     选中: bg=accent (80% opacity)
  └──────────────────────────────┘     未选: bg=transparent

  总尺寸: 150×28px, 圆角: 14px
  单项: 72×24px, 圆角: 12px
  Active: bg=accent + text=text_inverse, font=caption + bold
  Inactive: text=text_secondary, font=caption + medium
  动画: 200ms 滑动指示器
  切换延迟: < 150ms
```

### 6.4 左侧面板 (Left Panel)

```
┌──────────────────────────┐
│ ● Gateway  RUNNING       │ ← 状态卡片: bg=raised, 圆角 10px, padding 8px 12px
│   Port 18789 · 1 session │   font=caption + text_secondary
│ 📁 个人助手              │ ← 工作区行: bg=surface, 圆角 8px, padding 8px 12px
├──────────────────────────┤ ← divider: 1px
│ SESSIONS            [+]  │ ← 区域标题: font=caption + text_tertiary + letter-spacing 1
│ ┌──────────────────────┐ │   [+]: icon button 26×18px
│ │🦞 Chat  🟢 ACTIVE    │ │ ← Session 卡片: bg=raised, 圆角 10px
│ │ 2m ago · ~1.2k tkns  │ │   高度 52px, padding 8px 12px
│ └──────────────────────┘ │   Active: border 1px accent (30%)
│ ┌──────────────────────┐ │
│ │⚡ Work   🟡 BUSY      │ │
│ └──────────────────────┘ │
├──────────────────────────┤
│ CRON TASKS          [+]  │
│ 📭 暂无定时任务          │ ← 空状态: font=body + text_tertiary, 居中
└──────────────────────────┘

  宽度: 240px (默认), 可拖拽 200px~400px
  背景: panel (#1A1D27)
  右边框: 1px border
  内边距: 8px 12px
  gap: 8px
```

### 6.5 状态栏指示

> PRD 定义三级统一规范: LED / 托盘图标 / Task List 圆点 三者一致

| 状态 | LED 颜色 | 托盘图标 | Task 圆点 | 胶囊文字 |
|------|---------|---------|----------|---------|
| Ready | 🟢 `#3DD68C` | 白→绿 | 绿● | RUNNING |
| Busy | 🟡 `#FFBE3D` | 白→黄 | 黄● | BUSY |
| Checking | 🔵 `#6C9FFF` | 白→黄(闪) | 蓝● | CHECKING |
| Error | 🔴 `#FF6B6B` | 白→红 | 红● | ERROR |

### 6.6 托盘菜单

```
  ┌─────────────────────────────┐
  │  🟢 Gateway: Ready          │  ← 状态行, font=small, LED 嵌入文字
  ├─────────────────────────────┤
  │  Start Gateway              │  ← item 高度 28px, font=body
  │  Stop Gateway               │     bg=panel (owner-drawn 深色主题)
  │  Restart OpenClaw           │
  │  Refresh Status             │
  ├─────────────────────────────┤
  │  ☑ Boot Start               │  ← 开关项
  ├─────────────────────────────┤
  │  Open Settings              │
  │  About                      │
  │  Exit                       │  ← hover=danger 文字色
  └─────────────────────────────┘

  实现: Win32 NOTIFYICONDATAW, owner-drawn, 支持深色主题
  双击: 显示主窗口
  气泡: tray_balloon(title, message, timeout_ms)
```

### 6.7 贴边大蒜头

```
     屏幕边缘
        │
  ┌─────┤
  │ 🧄  │  ← 80×110px, WS_EX_LAYERED 透明背景
  │  🌱 │     身体 64px, 嫩芽 72px
  └─────┤
        │

  贴边检测: EDGE_SNAP_PX=20px
  悬停动画: 葱头 ±12° 正弦波摇摆, 1.5Hz, PlgBlt 仿射变换
  点击: 恢复主窗口到原始位置
  WM_ACTIVATE: Alt+Tab/任务栏点击自动恢复
  实现: 纯 Win32, stb_image 加载 PNG
```

---

## 7. 动效系统

### 7.1 动画原则

| 原则 | 说明 |
|------|------|
| 有意义 | 每个动画传达状态变化, 不做无意义装饰 |
| 快速 | 大部分 150-300ms, 不让用户等 |
| 自然 | 使用 ease-out (进入) / ease-in (退出) / spring (弹性) |
| 一致 | 同类交互用同一时长和曲线 |
| 可跳过 | 尊重系统 `prefers-reduced-motion` 设置 |

### 7.2 持续时间 Token

| Token | 值 | 用途 |
|-------|-----|------|
| `dur_instant` | 100ms | 微交互 (hover 高亮) |
| `dur_fast` | 150ms | 按钮反馈、checkbox 切换 |
| `dur_normal` | 200ms | 大部分过渡 (模式切换、气泡出现) |
| `dur_slow` | 300ms | 弹窗、面板展开、主题切换 |
| `dur_very_slow` | 500ms | 向导步骤切换、页面级过渡 |

### 7.3 缓动曲线 Token

| Token | 值 | 用途 |
|-------|-----|------|
| `ease_out` | `cubic-bezier(0.0, 0.0, 0.2, 1)` | 元素进入 (从无到有) |
| `ease_in` | `cubic-bezier(0.4, 0.0, 1.0, 1)` | 元素退出 (从有到无) |
| `ease_in_out` | `cubic-bezier(0.4, 0.0, 0.2, 1)` | 位置/大小变化 |
| `spring` | `cubic-bezier(0.34, 1.56, 0.64, 1)` | 弹性回弹 (开关、按钮) |
| `linear` | 线性 | 旋转、进度条 |

### 7.4 动画清单

#### 7.4.1 消息相关

| 动画 | 触发 | 效果 | 时长 | 曲线 |
|------|------|------|------|------|
| 消息出现 | 新消息到达 | 4px 上移 + fade-in | `dur_normal` (200ms) | `ease_out` |
| 消息飞出 | 用户发送 | 输入区文字 fade-out → 气泡从底部"飞入" | `dur_normal` | `ease_out` |
| 流式打字 | AI 回复中 | 光标 `\|` 脉冲闪烁 | 500ms 循环 | `linear` |
| 流式追加 | stream_timer_cb | 每 50ms 追加 3 字符 | 50ms | `linear` |
| 打字指示器 | AI 思考中 | 3 个圆点依次脉冲 | 1.2s 循环 | `linear` |
| 复制反馈 | Ctrl+C | 气泡右下角 "✅ 已复制" fade-in → 1.5s 后 fade-out | 200ms in / 200ms out | `ease_out` / `ease_in` |

#### 7.4.2 按钮 & 控件

| 动画 | 触发 | 效果 | 时长 | 曲线 |
|------|------|------|------|------|
| 按钮 hover | 鼠标进入 | 背景色加深 10% | `dur_instant` | `ease_out` |
| 按钮 active | 鼠标按下 | scale(0.97) | `dur_fast` | `ease_in` |
| 按钮 release | 鼠标释放 | scale(1.0) 回弹 | `dur_fast` | `spring` |
| Switch 切换 | 点击 | 滑块 16px 滑动 + 颜色渐变 | `dur_normal` | `spring` |
| Checkbox 勾选 | 点击 | scale(0.8→1.0) + ✓ fade-in | `dur_fast` | `spring` |
| 发送按钮 hover | 鼠标进入 | scale(1.05) + glow 阴影 | `dur_instant` | `ease_out` |

#### 7.4.3 面板 & 布局

| 动画 | 触发 | 效果 | 时长 | 曲线 |
|------|------|------|------|------|
| 模式切换 | Chat↔Work | 内容 cross-fade + 指示器滑动 | `dur_normal` | `ease_in_out` |
| 面板折叠 | Chat 面板 ▶ | 宽度 320→40px 压缩 | `dur_slow` | `ease_in_out` |
| 面板展开 | Chat 面板 ◀ | 宽度 40→320px 展开 | `dur_slow` | `ease_in_out` |
| 分隔条拖拽 | 拖拽中 | 面板半透明 85%, 实时跟随 | 即时 | — |
| 分隔条释放 | 释放 | 面板 opacity 回到 100% | `dur_fast` | `ease_out` |
| 最大化/还原 | 双击标题栏 | 窗口尺寸过渡 | `dur_slow` | `ease_in_out` |

#### 7.4.4 弹窗 & 浮层

| 动画 | 触发 | 效果 | 时长 | 曲线 |
|------|------|------|------|------|
| 弹窗出现 | 打开弹窗 | 从 95% 缩放 + fade-in | `dur_slow` | `ease_out` |
| 弹窗关闭 | 关闭弹窗 | 到 95% 缩放 + fade-out | `dur_normal` | `ease_in` |
| 遮罩出现 | 弹窗打开 | opacity 0→60% | `dur_slow` | `ease_out` |
| 遮罩关闭 | 弹窗关闭 | opacity 60%→0 | `dur_normal` | `ease_in` |
| Dropdown 展开 | 点击 | 从顶部 fade-in + 4px 下移 | `dur_normal` | `ease_out` |
| Dropdown 收起 | 点击外部 | fade-out + 4px 上移 | `dur_fast` | `ease_in` |
| Tooltip 出现 | hover 500ms | fade-in + 2px 上移 | `dur_fast` | `ease_out` |
| 右键菜单 | 右键点击 | fade-in + 从点击点 scale(0.95→1.0) | `dur_fast` | `ease_out` |

#### 7.4.5 Toast & 通知

| 动画 | 触发 | 效果 | 时长 | 曲线 |
|------|------|------|------|------|
| Toast 进入 | 触发通知 | 从右下 slide-in + fade-in | `dur_normal` | `ease_out` |
| Toast 消失 | 3s 超时 | fade-out | `dur_normal` | `ease_in` |
| Toast 堆叠 | 新通知到达 | 已有 Toast 向上推 44px | `dur_normal` | `ease_in_out` |

#### 7.4.6 向导 & 步骤

| 动画 | 触发 | 效果 | 时长 | 曲线 |
|------|------|------|------|------|
| 向导步骤切换 | Next/Back | 内容左/右滑动 + 连接线渐变延伸 | `dur_very_slow` | `ease_in_out` |
| 向导完成 | Get Started | 角色插图撒花 🎉 → 缩小到主界面 | `dur_very_slow` | `ease_in_out` |
| StepCard 进入 | 新步骤 | 从左侧 slide-in | `dur_normal` | `ease_out` |
| 状态 LED 脉冲 | Checking 状态 | opacity 40%↔100% 循环 | 1.5s | `linear` |

#### 7.4.7 窗口 & 系统

| 动画 | 触发 | 效果 | 时长 | 曲线 |
|------|------|------|------|------|
| 窗口最小化到托盘 | 点击 ✕ | 缩小 + 透明度 → 飞向托盘位置 | `dur_slow` | `ease_in` |
| 窗口从托盘恢复 | 双击托盘 | 从托盘位置放大 + 透明度恢复 | `dur_slow` | `ease_out` |
| 主题切换 | 切换主题 | 全局色值 300ms 过渡 (不硬切) | `dur_slow` | `ease_in_out` |
| 大蒜头摇摆 | hover | 正弦波 ±12°, 1.5Hz | 连续 | `linear` |

### 7.5 Reduced Motion

当系统开启 `prefers-reduced-motion` 时:
- 所有动画时长降为 0ms (即时切换)
- 流式打字改为一次性显示
- 保留功能性动画 (进度条、加载指示器)

---

## 8. 音效系统

### 8.1 设计原则

| 原则 | 说明 |
|------|------|
| 克制 | 只在关键交互点使用, 不做连续 BGM |
| 可关闭 | 设置中提供全局音效开关 |
| 系统集成 | 优先使用系统通知音, 自定义音效为增强 |
| 低侵入 | 音量不超过系统音量 50% |

### 8.2 音效清单

| 音效 ID | 触发场景 | 音效类型 | 时长 | 音量 | 说明 |
|---------|---------|---------|------|------|------|
| `msg_incoming` | AI 回复完成 | 短促清脆叮咚 | ~200ms | 40% | 类似 iMessage 提示音 |
| `msg_sent` | 用户消息发送 | 轻微 "嗖" | ~100ms | 20% | 可选, 极轻 |
| `alert_critical` | 警报弹窗 (磁盘不足等) | 系统警告音 | ~500ms | 60% | 使用 `PlaySound(MB_ICONWARNING)` |
| `alert_error` | 操作失败 | 系统错误音 | ~300ms | 50% | 使用 `PlaySound(MB_ICONERROR)` |
| `success` | Gateway 启动/任务完成 | 短促上扬 | ~200ms | 30% | 类似 macOS 完成音 |
| `permission_request` | 权限确认弹窗 | 短促双音 | ~300ms | 40% | 引起注意但不惊吓 |
| `wizard_complete` | 向导完成 (Get Started) | 欢快短旋律 | ~600ms | 50% | 品牌感, 有记忆点 |
| `copy` | 复制到剪贴板 | 轻微点击 | ~50ms | 15% | 极轻, 可选 |
| `error_toast` | Toast 错误通知 | 系统错误音 | ~300ms | 40% | |
| `boot_check_done` | Boot Check 完成 | 短促确认 | ~200ms | 30% | |

### 8.3 音效实现

```
技术方案:
  - Windows: PlaySound() API + WAV 资源内嵌
  - 系统音效优先: MB_ICONWARNING, MB_ICONERROR, MB_OK
  - 自定义音效: WAV 格式, 内嵌 EXE 资源段
  - 音量控制: 全局开关 + 单独音量滑块 (0~100%)
  - 存储: config.json → sound_enabled: true/false
```

### 8.4 设置中的音效控制

```
  ┌──────────────────────────────────────────┐
  │  Sound Effects                           │
  │  [====] 启用音效                         │
  │  音量  ████████░░  80%                   │  ← 可选: 音量滑块
  │  ────────────────────────────────────── │
  │  [🔊 测试音效]                           │  ← 点击播放示例
  └──────────────────────────────────────────┘
```

---

## 9. 图标系统

### 9.1 图标层级

| 层级 | 尺寸 | 用途 |
|------|------|------|
| ICON_TINY | 16px | 托盘最小、内联小图标 |
| ICON_SMALL | 20px | 托盘默认、菜单项 |
| ICON_MEDIUM | 24px | 聊天头像(AI)、按钮内图标 |
| ICON_LARGE | 32px | 聊天头像(用户)、标题栏 |
| ICON_XLARGE | 48px | 任务栏图标、关于页 |

### 9.2 图标来源

| 来源 | 用途 |
|------|------|
| LVGL Symbols | LV_SYMBOL_PLAY/STOP/REFRESH/SETTINGS/FILE/HOME/EDIT/CLOSE |
| 品牌图标 | 大蒜角色 (garlic_48.png 系列) |
| AI 头像 | lobster 系列 + ai 系列 (24/32/48 三尺寸) |
| 托盘图标 | 大蒜+LED 合体 PNG, 灰/绿/黄/红 × 16/20/32/48/96px |
| Emoji | 操作类型 (📄✏️⚡🔍)、品牌 (🧄🦞) |

### 9.3 操作类型图标映射

| 操作 | 图标 | 颜色 |
|------|------|------|
| 读文件 | 📄 | text_secondary |
| 写文件 | ✏️ | accent |
| 执行命令 | ⚡ | warning |
| 搜索 | 🔍 | info |
| 成功 | ✓ | accent |
| 失败 | ✗ | danger |
| 进行中 | ● | accent (pulse) |
| 等待确认 | ? | info |

---

## 10. DPI 适配

### 10.1 机制

```
  启动时:
  1. SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)
  2. 检测主显示器 DPI → 计算 scale 因子
  3. dpi_scale 保存到 config.json
  4. 所有尺寸常量 × SCALE() 宏

  SCALE(base) = (int)(base * dpi_scale + 0.5f)
  dpi_scale = GetDpiForWindow(hwnd) / 96.0f
```

### 10.2 缩放示例

| 原始值 | 100% (96 DPI) | 125% (120 DPI) | 150% (144 DPI) |
|--------|---------------|----------------|----------------|
| 标题栏 48px | 48 | 60 | 72 |
| 左面板 240px | 240 | 300 | 360 |
| 气泡圆角 10 | 10 | 12 | 15 |
| 字号 13px | 13 | 16 | 19 |

---

## 11. 国际化样式

### 11.1 语言支持

| 语言 | 代码 | 字体回退 | 默认 |
|------|------|---------|------|
| 中文 | CN | HarmonyOS Sans SC → 微软雅黑 | ✅ (系统为中文时) |
| 英文 | EN | Inter → Segoe UI | ✅ (其他系统语言) |
| 韩文 | KR | HarmonyOS Sans → 맑은 고딕 | |
| 日文 | JP | HarmonyOS Sans → Yu Gothic | |

### 11.2 文字长度适配

| 语言 | 相对英文长度 | 布局影响 |
|------|-------------|---------|
| EN | 1.0x (基准) | — |
| CN | 0.6x | 按钮可缩短, 节省空间 |
| KR | 0.7x | 同 CN |
| JP | 0.65x | 同 CN |

> 按钮宽度: 对 CJK 语言可使用更紧凑的内边距 (12px 20px vs 12px 24px)。

### 11.3 切换行为

- 切换后立即生效, 无需重启
- 所有 UI 文本通过 `I18n` 结构体定义, `tr()` 函数返回当前语言
- 配置持久化到 config.json (`language` 字段)

---

## 附录: 与 PRD 原始规格对照

| PRD 原始值 | 本文档建议值 | 变更原因 |
|-----------|------------|---------|
| bg `#1A1E2E` | bg `#0F1117` | 更深, 增加层次空间感 |
| panel `#232838` | panel `#1A1D27` | 微暖灰, 更干净 |
| accent `#82AAF0` | accent `#3DD68C` | 薄荷绿, 更年轻有活力 |
| btn_action `#4070C0` | accent `#3DD68C` | 统一品牌色 |
| 气泡圆角 10px | 气泡圆角 16px | 更圆润, 年轻感 |
| 字体 Montserrat | 字体 Inter | 更现代, 可读性更好 |
| 字体 微软雅黑 | 字体 HarmonyOS Sans SC | 免费商用, 更现代 |

> **向后兼容**: PRD 原始色值保留在 `theme.h` 中作为 `classic` 主题。本文档建议值作为新 `dark` 主题的默认值。切换通过 `config.json` 的 `theme` 字段控制。

## 12. 控件状态总表

> 所有控件在 6 种交互状态下的视觉定义。

### 12.1 状态定义

| 状态 | 触发条件 | 含义 |
|------|---------|------|
| `default` | 初始状态 | 控件静止 |
| `hover` | 鼠标悬停 | 可交互暗示 |
| `focus` | Tab 键/点击聚焦 | 键盘可达, 准备接收输入 |
| `active/pressed` | 鼠标按下 / 键盘回车 | 正在交互 |
| `disabled` | 条件不满足 | 不可交互 |
| `loading` | 异步操作中 | 操作进行中, 暂不可用 |

### 12.2 全局状态样式 Token

| Token | 值 | 适用状态 |
|-------|-----|---------|
| `state_hover_bg` | `#FFFFFF0A` (8% white) | hover |
| `state_active_bg` | `#FFFFFF14` (12% white) | active |
| `state_focus_ring` | `0 0 0 2px accent 30%` | focus |
| `state_disabled_opacity` | `0.4` | disabled |
| `state_loading_overlay` | `#0F111780` (50% bg) | loading |

### 12.3 控件 × 状态矩阵

#### 按钮

| 状态 | 背景 | 文字 | 边框 | 光标 | 其他 |
|------|------|------|------|------|------|
| default | accent | text_inverse | none | pointer | shadow_sm |
| hover | accent_hover | text_inverse | none | pointer | shadow_md |
| focus | accent | text_inverse | 2px focus_ring | pointer | — |
| active | accent_active | text_inverse | none | pointer | scale(0.97) |
| disabled | disabled_bg | disabled_text | none | not-allowed | opacity 0.4 |
| loading | accent | text_inverse | none | wait | spinner 叠加 |

#### 输入框

| 状态 | 背景 | 文字 | 边框 | 光标 | 其他 |
|------|------|------|------|------|------|
| default | surface | text_primary | 1px border | text | placeholder=text_tertiary |
| hover | surface | text_primary | 1px border_strong | text | — |
| focus | surface | text_primary | 2px accent | text | glow shadow |
| active | surface | text_primary | 2px accent | text | 光标闪烁 |
| disabled | disabled_bg | disabled_text | 1px border | not-allowed | opacity 0.4 |
| loading | surface | text_primary | 1px border | wait | — |

#### Switch

| 状态 | 背景 | 滑块 | 光标 |
|------|------|------|------|
| default (off) | #373C55 | #8B92A8 | pointer |
| hover (off) | #404560 | #A0A6BC | pointer |
| default (on) | accent | white | pointer |
| hover (on) | accent_hover | white | pointer |
| active | 缩放 0.95 | — | pointer |
| disabled | disabled_bg | disabled_text | not-allowed |

#### Dropdown

| 状态 | 背景 | 文字 | 边框 | 箭头 |
|------|------|------|------|------|
| default | surface | text_primary | 1px border | text_secondary, 0° |
| hover | surface | text_primary | 1px border_strong | text_primary |
| open/focus | surface | text_primary | 2px accent | accent, 180° |
| disabled | disabled_bg | disabled_text | 1px border | disabled_text |

#### Session 卡片

| 状态 | 背景 | 边框 | 光标 |
|------|------|------|------|
| default | raised | none | pointer |
| hover | overlay | none | pointer |
| selected | raised | 1px accent 30% | pointer |
| active (当前) | raised | 1px accent | pointer |

---

## 13. 加载态与骨架屏

### 13.1 加载状态分类

| 类型 | 时长预期 | UI 表现 | 例子 |
|------|---------|---------|------|
| 即时 | < 200ms | 无加载态, 直接显示 | 本地配置读取 |
| 快速 | 200ms~2s | spinner + 禁用按钮 | Gateway 健康检查 |
| 中等 | 2s~10s | 进度条 + 状态文字 | 模型切换 |
| 慢速 | > 10s | 进度条 + 百分比 + 取消按钮 | Gemma 下载 |

### 13.2 骨架屏 (Skeleton)

```
  消息气泡骨架 (AI 回复等待中):
  ┌──────────────────────────────────────────┐
  │  ┌──┐                                    │
  │  │  │  ████████████████                  │  ← bg=raised, pulse 1.5s
  │  └──┘  ████████████████████████████      │     圆角同气泡
  │        ██████████████                    │
  └──────────────────────────────────────────┘

  Session 卡片骨架:
  ┌────────────────────────────────────────┐
  │  ┌──┐  ████████████                    │
  │  │  │  ██████████████                  │
  └────────────────────────────────────────┘
```

**骨架动画**: 
- 背景渐变从左到右滑动 (shimmer), 1.5s 循环
- 颜色: `raised` → `overlay` → `raised`
- 骨架消失: 200ms cross-fade 到真实内容

### 13.3 全屏加载

```
  ┌────────────────────────────────────────────┐
  │                                            │
  │              ◜                             │  ← 品牌色 spinner (24px)
  │                                            │
  │        正在连接 Gateway...                  │  ← body + text_secondary
  │                                            │
  └────────────────────────────────────────────┘

  场景: 启动自检、Gateway 重启
  遮罩: 半透明 bg 色
  spinner: 24px, 2px 描边, accent 色, 0.8s 旋转
```

### 13.4 按钮内加载

```
  ┌──────────────────────────┐
  │  ◜  Saving...            │  ← spinner 12px + 文字
  └──────────────────────────┘

  触发: 点击 Save 后, 异步操作中
  按钮: disabled 态 + spinner 替代 icon
  恢复: 操作完成 → 显示 ✓ 1.5s → 恢复原始文字
```

---

## 14. 错误态与异常处理

### 14.1 错误层级

| 层级 | 影响范围 | UI 表现 | 例子 |
|------|---------|---------|------|
| 页内错误 | 单个输入/操作 | 输入框红边框 + 底部错误文字 | API Key 格式错误 |
| 区域错误 | 面板/Tab 内 | 区域顶部错误条 | 模型列表加载失败 |
| 全局错误 | 整个应用 | 弹窗/阻断 | Gateway 连接失败 |
| 崩溃 | 进程级别 | 崩溃日志 + 重启提示 | 未处理异常 |

### 14.2 输入框错误

```
  ┌──────────────────────────────────────┐
  │  invalid-api-key-xxx                │  ← border: 2px danger
  └──────────────────────────────────────┘
  ⚠ API Key 格式不正确                    ← caption + danger, 4px 上边距

  规则:
  - 红色边框 + 底部错误文字
  - 错误文字: caption (10px) + danger, 4px 上边距
  - 输入框获得焦点时错误不消失
  - 修正后: 200ms border 渐变回 accent
```

### 14.3 区域错误条

```
  ┌──────────────────────────────────────────────┐
  │ ⚠ 模型列表加载失败    [ 重试 ]              │  ← bg: #FF6B6B10, border-left: 3px danger
  └──────────────────────────────────────────────┘     高度: 36px, padding: 8px 12px

  位置: 出错区域顶部
  内容: 图标 + 错误描述 + 操作按钮
  消失: 点击重试成功后 200ms slide-out
```

### 14.4 全局错误弹窗

```
  ┌────────────────────────────────────────────────┐
  │   ┌────────────────────────────────────────┐   │
  │   │  ❌ 连接失败                            │   │
  │   │                                        │   │
  │   │  无法连接到 Gateway (localhost:18789)   │   │
  │   │                                        │   │
  │   │  可能原因:                              │   │
  │   │  · Gateway 未启动                       │   │
  │   │  · 端口被占用                           │   │
  │   │  · 防火墙阻止                           │   │
  │   │                                        │   │
  │   │  ┌────────────┐  ┌──────────────┐      │   │
  │   │  │  重试连接  │  │  查看日志    │      │   │
  │   │  └────────────┘  └──────────────┘      │   │
  │   │  ┌────────────┐                        │   │
  │   │  │  忽略      │                        │   │
  │   │  └────────────┘                        │   │
  │   └────────────────────────────────────────┘   │
  └────────────────────────────────────────────────┘
```

### 14.5 空状态 (各处汇总)

| 位置 | 图标 | 文案 | CTA |
|------|------|------|-----|
| Chat 空状态 | 🧄🦞 | "龙虾要吃蒜蓉的 — 有什么我能帮你？" | 快捷建议气泡 |
| Work 空状态 | ⚡ | "AI 工作台已就绪 — 在 Chat 中发送任务" | 查看示例 |
| Session 空 | 📭 | "暂无活跃会话 — 发送消息自动创建" | — |
| Cron 空 | ⏰ | "暂无定时任务 — 点击 + 创建" | [+按钮] |
| 知识库空 | 📚 | "知识库为空 — 添加文件开始索引" | 添加按钮 |
| Skill 空 | 🧩 | "暂无 Skill — 从 ClawHub 浏览安装" | 浏览按钮 |
| 搜索无结果 | 🔍 | "未找到匹配结果" | 修改搜索词 |
| Log 空 | 📋 | "暂无日志记录" | — |

**空状态通用样式**:
- 图标: 32px + text_tertiary, 居中
- 文案: body + text_tertiary, 居中, 最大宽度 280px
- CTA: accent 色链接 或 次要按钮
- 整体垂直居中于容器

---

## 15. 表单验证

### 15.1 验证时机

| 时机 | 说明 | 适用场景 |
|------|------|---------|
| `onBlur` | 失去焦点时验证 | API Key、昵称 |
| `onChange` | 每次输入实时验证 | 搜索框 |
| `onSubmit` | 提交时验证 | 向导完成、设置保存 |
| `debounced` | 停止输入 500ms 后验证 | 模型名称 |

### 15.2 验证状态视觉

```
  验证中:  输入框右侧 spinner (12px)
  通过:    输入框右侧 ✓ accent (16px), fade-in
  失败:    输入框红色边框 + 底部错误文字
  警告:    输入框黄色边框 + 底部警告文字 (可提交)
```

### 15.3 特定字段验证规则

| 字段 | 规则 | 错误文案 |
|------|------|---------|
| API Key | 非空 + `sk-` 开头或自定义格式 | "API Key 格式不正确" |
| 昵称 | 1-20 字符 | "昵称长度需在 1-20 字符之间" |
| 模型名称 | 非空 + 含 `/` | "请输入完整的模型名称 (如 provider/model)" |
| 端口号 | 1-65535 | "端口号需在 1-65535 之间" |
| 工作区路径 | 目录存在 + 可写 | "目录不存在或无写入权限" |

---

## 16. 无障碍设计 (Accessibility)

### 16.1 对比度要求 (WCAG 2.1 AA)

| 组合 | 前景 | 背景 | 对比度 | 达标? |
|------|------|------|--------|------|
| 正文 | text_primary #E8ECF4 | bg #0F1117 | 12.8:1 | ✅ AAA |
| 次要文字 | text_secondary #8B92A8 | bg #0F1117 | 4.9:1 | ✅ AA |
| 占位符 | text_tertiary #6B7280 | surface #13151E | 3.2:1 | ⚠️ AA-large |
| 按钮文字 | text_inverse #0F1117 | accent #3DD68C | 8.1:1 | ✅ AAA |
| 链接 | accent #3DD68C | bg #0F1117 | 7.2:1 | ✅ AAA |
| 危险文字 | danger #FF6B6B | bg #0F1117 | 5.6:1 | ✅ AA |
| 警告文字 | warning #FFBE3D | bg #0F1117 | 9.1:1 | ✅ AAA |

### 16.2 焦点管理

| 规则 | 说明 |
|------|------|
| 焦点可见 | 所有可交互元素 focus 态有 2px focus_ring |
| 焦点顺序 | 从左到右, 从上到下, 符合视觉流 |
| 焦点陷阱 | 弹窗打开时焦点锁定在弹窗内 |
| 焦点恢复 | 弹窗关闭后焦点回到触发元素 |
| Tab 导航 | 所有按钮/输入/下拉/开关支持 Tab 聚焦 |
| Esc 关闭 | 所有弹窗/Esc 关闭 |

### 16.3 键盘导航

| 键 | 行为 |
|----|------|
| Tab | 下一个可聚焦元素 |
| Shift+Tab | 上一个可聚焦元素 |
| Enter/Space | 激活按钮/开关/复选框 |
| Esc | 关闭弹窗/下拉/菜单 |
| ↑↓ | Dropdown 选项导航 |
| ←→ | 模式切换器导航 |

### 16.4 屏幕阅读器

| 元素 | ARIA 角色 | Label |
|------|----------|-------|
| 模式切换器 | `tablist` > `tab` | "Chat mode" / "Work mode" |
| 开关 | `switch` | 功能名称 + "on/off" |
| 按钮 | `button` | 按钮文字 |
| 弹窗 | `dialog` + `aria-modal=true` | 弹窗标题 |
| 进度条 | `progressbar` + `aria-valuenow` | "Downloading: 62%" |
| 状态 LED | `status` | "Gateway status: Running" |

---

## 17. 滚动条样式

### 17.1 自定义滚动条

```
  轨道:  4px 宽, bg=transparent
  滑块:  4px 宽, bg=#FFFFFF15 (15% white), 圆角 2px
  hover: bg=#FFFFFF30 (30% white)
  active: bg=accent (拖拽中)
```

### 17.2 滚动行为

| 场景 | 行为 | 代码 |
|------|------|------|
| 新消息 | 强制滚到底 | `chat_force_scroll_bottom()` |
| 流式输出中 | 软滚 (40px 容差) | `SCROLL_BOTTOM_TOLERANCE=40px` |
| 用户手动回看 | 不强制拉回 | 检测 scroll_y ≠ bottom |
| StepCard 新增 | 滚到最新可见 | 自动滚动 |
| 列表加载更多 | 保持当前位置 | 滚动偏移补偿 |

### 17.3 滚动边界

```
  顶部弹性: 无 (不允许过度下拉)
  底部弹性: 无 (不允许过度上拉)
  惯性滚动: 有, 减速曲线 ease_out
  鼠标滚轮: 每格 60px
```

---

## 18. 光标类型

| 场景 | 光标 | 说明 |
|------|------|------|
| 默认 | `default` (箭头) | 非交互区域 |
| 可点击 | `pointer` (手型) | 按钮、链接、可点击元素 |
| 输入 | `text` (I-beam) | 输入框、可选中文字 |
| 拖拽 | `move` (四向箭头) | 标题栏拖拽区域 |
| 调整大小 | `col-resize` | 分隔条拖拽 |
| 禁用 | `not-allowed` (🚫) | disabled 态控件 |
| 等待 | `wait` (spinner) | loading 态 |
| 抓取 | `grab` → `grabbing` | 可拖拽元素 |

---

## 19. 键盘快捷键指示

### 19.1 快捷键列表 (PRD §16.1)

| 快捷键 | 功能 | UI 指示位置 |
|--------|------|------------|
| Enter | 发送消息 | 发送按钮 tooltip |
| Shift+Enter | 换行 | 输入框 placeholder |
| Ctrl+V | 粘贴 | 右键菜单 |
| Ctrl+C | 复制 | 右键菜单 |
| Ctrl+X | 剪切 | 右键菜单 |
| Ctrl+A | 全选 | — |
| Esc | 关闭弹窗 | 弹窗右上角 × |

### 19.2 快捷键 UI 样式

```
  右键菜单中的快捷键:
  ┌──────────────────────────────┐
  │  📋 复制              Ctrl+C │  ← 右对齐, font=caption + text_tertiary
  │  ✂️ 剪切              Ctrl+X │
  └──────────────────────────────┘

  Tooltip 中的快捷键:
  ┌──────────────────────────────┐
  │  发送消息     Enter          │
  └──────────────────────────────┘
```

---

## 20. 拖拽反馈

### 20.1 分隔条拖拽

```
  开始拖拽:
  1. 高亮分隔条 bg=accent, opacity=0.6
  2. 光标变为 col-resize
  3. 面板半透明 85% opacity

  拖拽中:
  1. 实时更新面板宽度
  2. relayout_panels() 刷新子控件

  结束拖拽:
  1. 分隔条颜色恢复
  2. 光标恢复 default
  3. 面板 opacity 回到 100% (200ms ease_out)
```

### 20.2 弹窗拖拽

```
  实现: dialog_drag_cb
  1. 记录鼠标按下点偏移
  2. 跟随鼠标移动弹窗位置
  3. 限制: 不可拖出屏幕边界
  4. 阴影: 拖拽时 shadow_lg 加深
```

### 20.3 文件拖放 (未来)

```
  拖入聊天区:
  1. 输入区边框高亮 accent (2px dashed)
  2. 输入区背景变为 accent_subtle
  3. 显示 "释放以发送文件" 文案
  4. 释放: 创建附件卡片
  5. 取消: 恢复默认样式
```

---

## 21. 品牌文案风格

### 21.1 品牌关键词

**龙虾要吃蒜蓉的 🧄🦞**

### 21.2 文案调性

| 维度 | 要 | 不要 |
|------|-----|------|
| 人称 | "我" / "你" | "系统" / "用户" |
| 语气 | 友好、轻松、有个性 | 冷冰冰、机械、官腔 |
| 长度 | 简洁, 一句说完 | 冗长、多段 |
| 标点 | 适度使用 emoji | 满屏感叹号 |

### 21.3 常用文案模板

| 场景 | 文案模板 |
|------|---------|
| 欢迎 | "龙虾要吃蒜蓉的 🧄🦞 — 你的 AI 助手已就位！" |
| 空状态引导 | "有什么我能帮你？" |
| 操作成功 | "搞定！✓" |
| 操作失败 | "出了点问题 — {原因}" |
| 加载中 | "正在{动词}..." |
| 确认退出 | "Gateway 将继续在后台运行。" |
| 权限请求 | "Agent 想{动作}，你批准吗？" |
| 超时 | "⚠️ AI 回复超时，请重试。" |
| 磁盘不足 | "🚨 空间快满了，清理一下？" |
| 更新可用 | "📦 有新版本啦！" |

### 21.4 按钮文案

| 类型 | 推荐 | 避免 |
|------|------|------|
| 主操作 | "Get Started" / "保存" / "确认" | "提交" / "执行" |
| 次要 | "取消" / "跳过" / "稍后" | "关闭" / "返回" |
| 危险 | "删除" / "拒绝" / "断开" | "确认删除" (用描述代替) |
| 链接 | "了解更多" / "查看详细" | "点击这里" |

---

## 22. 主题 Token 完整对照表

### 22.1 暗色 vs 亮色 vs 经典 对照

| Token | 暗色 (Mint Dark) | 亮色 | 经典暗色 |
|-------|-----------------|------|---------|
| `bg` | `#0F1117` | `#F5F7FA` | `#2D2D2D` |
| `surface` | `#13151E` | `#EBEEF2` | `#333333` |
| `panel` | `#1A1D27` | `#FFFFFF` | `#3C3C3C` |
| `raised` | `#1E2230` | `#FFFFFF` | `#444444` |
| `overlay` | `#252A38` | `#E2E5EA` | `#505050` |
| `text_primary` | `#E8ECF4` | `#1A1D27` | `#FFFFFF` |
| `text_secondary` | `#8B92A8` | `#6B7280` | `#B0B0B0` |
| `text_tertiary` | `#6B7280` | `#9CA3AF` | `#808080` |
| `text_inverse` | `#0F1117` | `#FFFFFF` | `#000000` |
| `accent` | `#3DD68C` | `#2BB673` | `#82AAF0` |
| `accent_hover` | `#2BB673` | `#249A62` | `#6B9BE0` |
| `accent_active` | `#1FA35E` | `#1D8452` | `#5588D0` |
| `accent_subtle` | `#3DD68C1A` | `#2BB67315` | `#82AAF01A` |
| `accent_secondary` | `#6C9FFF` | `#4F8BE6` | `#A0C4FF` |
| `success` | `#3DD68C` | `#16A34A` | `#4CAF50` |
| `warning` | `#FFBE3D` | `#D97706` | `#FFC107` |
| `danger` | `#FF6B6B` | `#DC2626` | `#F44336` |
| `info` | `#6C9FFF` | `#2563EB` | `#2196F3` |
| `border` | `#3DD68C0A` | `#E2E5EA` | `#555555` |
| `border_strong` | `#3DD68C20` | `#CBD5E1` | `#777777` |
| `divider` | `#3DD68C08` | `#E2E5EA80` | `#55555580` |
| `focus_glow` | `#3DD68C30` | `#2BB67320` | `#82AAF030` |
| `hover_overlay` | `#FFFFFF0A` | `#00000008` | `#FFFFFF0A` |
| `active_overlay` | `#FFFFFF14` | `#00000012` | `#FFFFFF14` |
| `disabled_bg` | `#1A1D2780` | `#EBEEF280` | `#3C3C3C80` |
| `disabled_text` | `#6B728080` | `#9CA3AF80` | `#80808080` |
| `shadow_sm` | `0 1px 3px #0003` | `0 1px 3px #0001` | `0 1px 3px #0003` |
| `shadow_md` | `0 2px 8px #0004` | `0 2px 8px #00015` | `0 2px 8px #0004` |
| `shadow_lg` | `0 8px 24px #0005` | `0 8px 24px #0002` | `0 8px 24px #0005` |
| `bubble_user_bg` | `#2B5C3E` | `#DCFCE7` | `#4070C0` |
| `bubble_ai_bg` | `#1E2230` | `#F1F5F9` | `#3C3C3C` |

### 22.2 切换动画

```
  主题切换时:
  所有 Token 值在 300ms 内 ease_in_out 过渡
  不硬切, 不闪烁
  实现: 全局 style prop 绑定, 切换时批量 update
```

---

## 23. 窗口响应式

### 23.1 断点定义

| 断点 | 宽度范围 | 典型场景 |
|------|---------|---------|
| `compact` | 800~1000px | 小屏笔记本、半屏贴靠 |
| `default` | 1001~1450px | 主流笔记本/显示器 |
| `wide` | 1450px+ | 外接显示器、全屏 |

> PRD 设计尺寸 1450×800，最小 800×500。所有尺寸常量按 `dpi_scale` 缩放。

### 23.2 Compact 模式 (800~1000px)

```
  ┌──────────────────────────────────────────────────────────┐
  │ 🧄 AnyClaw   [Chat|Work]               [⚙][—][□][✕]   │ 48px
  ├──────────────────────────────────────────────────────────┤
  │ [≡ Sessions]  │                                          │  ← 左面板折叠为
  │               │          右侧内容区                       │     侧边抽屉
  │               │                                          │
  │               │                                          │
  │               │                                          │
  └───────────────┴──────────────────────────────────────────┘
```

**折叠规则**:

| 宽度 | 左面板 | 分隔条 | 右侧内容 | Chat 面板 (Work) |
|------|--------|--------|---------|-----------------|
| > 1000px | 240px 固定 | 6px 可拖拽 | 剩余宽度 | 320px |
| 800~1000px | **折叠为抽屉** (36px 按钮) | 隐藏 | 全宽 | **折叠为 40px** |
| 拖拽到边缘 | 弹出覆盖层 (overlay) | — | 全宽 | — |

**抽屉模式细节**:
- 左侧仅显示 36px 宽的图标列: [≡] 展开按钮 + LED 状态点
- 点击 [≡] → 左面板以 overlay 方式从左侧滑入 (240px 宽, 80% 不透明)
- 点击遮罩/按 Esc → 关闭抽屉
- Session 列表在抽屉内正常显示

**Work 模式 Compact**:
- Chat 面板默认折叠到 40px (仅显示 ▶ 按钮)
- 步骤流和输出面板上下分栏保持 (55% / 45%)

### 23.3 窗口高度响应

| 高度 | 调整 |
|------|------|
| > 700px | 正常布局 |
| 500~700px | 输入区从 96px 压缩到 64px (最小 2 行) |
| 500px | 步骤流与输出面板从 55/45 改为 50/50 |

### 23.4 面板宽度限制

| 面板 | 最小 | 默认 | 最大 |
|------|------|------|------|
| 左面板 | 200px | 240px | 400px |
| Chat 面板 (Work) | 40px (折叠) | 320px | 480px |
| 分隔条 | 6px | 6px | 6px |

---

## 24. 性能预算

### 24.1 渲染性能

| 指标 | 目标 | 测量方式 |
|------|------|---------|
| 帧率 | ≥ 30fps 稳定, 60fps 空闲 | SDL 帧计数 |
| 帧时间 | < 33ms (30fps) / < 16ms (60fps) | 每帧 `TraceSpan` |
| 流式渲染刷新 | 50ms 间隔, 3 字符/次 | `CHAT_STREAM_INTERVAL_MS` |
| 窗口启动到可交互 | < 2s | 启动 `TraceSpan` |
| 模式切换延迟 | < 150ms | 切换 `TraceSpan` |
| 弹窗打开 | < 100ms | 弹窗 `TraceSpan` |

### 24.2 网络性能

| 指标 | 目标 | 超时 |
|------|------|------|
| 健康检查 | < 50ms | 3s |
| 聊天首 token | < 5s | 30s idle |
| 流式空闲超时 | 30s 无数据 → 终止 | — |
| 流式总超时 | 45s → 终止 | — |
| CLI 子进程 (session list) | < 2s | 12s |
| Failover 探针 | < 5s | 12s |
| 更新检查 | < 5s | 10s |

### 24.3 内存预算

| 状态 | 目标上限 | 说明 |
|------|---------|------|
| 空闲 (Idle) | < 200 MB | PRD 明确要求 |
| Chat 活跃 | < 300 MB | 含流式 buffer + 历史 |
| Work 活跃 | < 350 MB | 含步骤流 + 输出渲染 |
| Gemma 加载 | +2~8 GB | 模型权重, 独立进程 |

### 24.4 存储预算

| 文件 | 大小限制 | 轮转策略 |
|------|---------|---------|
| `app.log` | 1MB / 文件, 最多 5 个 | 超过自动轮转, 7 天清理 |
| `traces.jsonl` | 无硬限, 追加写入 | Ring Buffer 1000 事件, 50 事件 flush |
| `config.json` | < 100KB | — |
| `chat_history` | 50 条持久化 | 内存 buffer 4096 条 |
| `crash_logs/` | 最近 10 个 | — |
| `.backups/` | 最近 10 份 | AGENTS.md / TOOLS.md 备份 |
| 崩溃日志 | 最近 10 个 | 自动清理 |

### 24.5 启动性能预算

| 阶段 | 目标 | 说明 |
|------|------|------|
| SDL + LVGL 初始化 | < 500ms | 窗口创建 |
| DPI 检测 + 布局 | < 50ms | 启动时一次 |
| Mutex 检查 | < 10ms | 实例互斥 |
| 日志初始化 | < 20ms | — |
| SelfCheck (4 项) | < 2s | Node/npm/网络/配置目录 |
| Feature Flags 加载 | < 10ms | JSON 读取 |
| Tray 创建 | < 50ms | Win32 NOTIFYICONDATA |
| 首次健康检查 | < 3s | Gateway HTTP GET |
| **总计: 启动到可交互** | **< 3s** | SelfCheck 为最大瓶颈 |

### 24.6 Trace 集成点

> 所有性能关键路径已接入 `TraceSpan`

| Trace action | 说明 |
|-------------|------|
| `app_startup` | 全启动流程 |
| `selfcheck_run` | 自检执行 |
| `health_check_cycle` | 每轮健康检查 |
| `http_post_stream` | 聊天 SSE 流式请求 |
| `session_refresh` / `session_abort` | 会话管理 |
| `boot_check_run_and_fix` | Boot Check |
| `perm_check` | 权限检查 |
| `config_set_gateway` | Gateway 配置同步 |

---

## 25. 离线降级

### 25.1 故障场景分级

| 场景 | 检测方式 | 严重级别 |
|------|---------|---------|
| Gateway 未启动 | HTTP 无响应 + 进程不存在 | 🔴 Critical |
| Gateway 运行但 HTTP 不通 | HTTP 超时 + 进程存在 | 🟠 Error |
| LLM API 超时 | SSE idle > 30s | 🟡 Warning |
| LLM API 429 限流 | HTTP 429 | 🟡 Warning |
| 网络断开 | 健康检查连续失败 ≥ 3 | 🔴 Critical |
| 模型不可用 | Failover 所有模型都 fail | 🟠 Error |

### 25.2 Gateway 断开 — UI 降级

```
  ┌─────────────────────────────────────────────────────────────────────────────────┐
  │ 🧄 AnyClaw   [Chat|Work]                     [⚙][—][□][✕]                     │
  ├─────────────────────────────────────────────────────────────────────────────────┤
  │                                                                                 │
  │  ┌─────────────────────┐   ╎   ┌─────────────────────────────────────────────┐  │
  │  │ 🔴 Gateway: ERROR   │   ╎   │                                             │  │
  │  │   未连接             │   ╎   │  ┌───────────────────────────────────────┐  │  │
  │  │   [ 启动 Gateway ]   │   ╎   │  │ ⚠️ Gateway 未运行                    │  │  │
  │  ├─────────────────────┤   ╎   │  │                                       │  │  │
  │  │ SESSIONS            │   ╎   │  │ AI 助手暂时不可用。                   │  │  │
  │  │ (无连接)             │   ╎   │  │ 请先启动 Gateway。                    │  │  │
  │  └─────────────────────┘   ╎   │  │                                       │  │  │
  │                             ╎   │  │ [ 启动 Gateway ]  [ 查看帮助 ]        │  │  │
  │                             ╎   │  └───────────────────────────────────────┘  │  │
  │                             ╎   │                                             │  │
  │                             ╎   │  ┌───────────────────────────────────────┐  │  │
  │                             ╎   │  │ 输入消息... (Gateway 未连接)         │  │  │  ← placeholder 变灰
  │                             ╎   │  └───────────────────────────────────────┘  │  │  ← 发送按钮 disabled
  │                             ╎   └─────────────────────────────────────────────┘  │
  └─────────────────────────────────────────────────────────────────────────────────┘
```

**禁用项**:
- 发送按钮: disabled + tooltip "请先启动 Gateway"
- 输入框: 可输入但不可发送, placeholder 变为 "Gateway 未连接"
- Session 列表: 灰显, Abort 按钮隐藏
- Work 模式任务输入: disabled
- Boot Check 按钮: 仍然可用 (本地检测不依赖 Gateway)

**保持可用**:
- 设置页: 全部可操作
- 主题/语言切换: 正常
- License 查看: 正常
- Log Tab: 正常
- About Tab: 正常
- Boot Check: 正常 (本地检测)
- Reconfigure Wizard: 正常

### 25.3 LLM 超时 — 流式降级

```
  正常:  流式打字中 ●●● → 追加文字 → 完成
  超时:  流式打字中 ●●● → "⚠️ AI 回复超时，请重试。"
  部分:  流式打字中 → 已接收部分文字 → 超时 → 保留已接收内容 + 超时提示
```

**处理逻辑**:
- `idle_ms > 30000 || total_ms > 45000` → 终止 SSE
- buffer 为空: 仅显示超时提示
- buffer 不为空: 保留已接收内容 + 追加超时提示
- 发送按钮恢复可点击状态
- 音效: `alert_error` (可选)

### 25.4 LLM 429 限流

```
  ┌──────────────────────────────────────────────┐
  │  ┌────────────────────────────────────────┐  │
  │  │ ⚠️ 请求过于频繁                       │  │  ← 区域错误条
  │  │ 模型 API 限流中, 请稍后重试。          │  │     bg: #FFBE3D10
  │  │ 倒计时: 45s                           │  │     border-left: 3px warning
  │  │ [ 重试 ]  [ 切换模型 ]                 │  │
  │  └────────────────────────────────────────┘  │
  └────────────────────────────────────────────────┘
```

**处理逻辑**:
- 解析 HTTP 429 响应头 `Retry-After`
- 显示倒计时, 倒计时结束自动重试或显示重试按钮
- Failover 自动切换到备用模型 (如果启用)
- 音效: `warning` (可选)

### 25.5 Failover 切换通知

```
  ┌──────────────────────────────────────────────┐
  │ 🔄 已自动切换模型                            │  ← Toast 通知
  │ gemma-3-4b-it:free → gemini-2.5-flash       │     accent 色左边框
  │ 原因: 原模型响应超时                         │
  └──────────────────────────────────────────────┘
```

**触发条件**: 当前模型连续失败 ≥ 3 → Failover 选择评分最高的健康模型
**显示位置**: Toast 通知, 5 秒后自动消失
**可操作**: 点击 Toast → 跳转到 Model Tab

### 25.6 网络完全断开

```
  状态: Gateway 进程在 + HTTP 连续失败 ≥ 3 → Error

  左面板:
  │ 🔴 Gateway: ERROR          │
  │   HTTP 连接失败             │
  │   [ 重试连接 ]              │

  聊天区:
  │ ⚠️ 网络连接异常             │
  │ 请检查网络后重试。          │
  │ [ 重试连接 ]  [ 查看帮助 ]  │

  自动恢复: 后台健康检查每 30s 重试, 连接恢复后自动切换到 Ready
```

### 25.7 降级状态总览

| 功能 | Gateway 断开 | LLM 超时 | LLM 429 | 网络断开 |
|------|-------------|----------|---------|---------|
| 发送消息 | ❌ 禁用 | ⏸️ 当前阻塞 | ⏸️ 当前阻塞 | ❌ 禁用 |
| 输入框 | ⚠️ 可输不可发 | ✅ 正常 | ✅ 正常 | ⚠️ 可输不可发 |
| 历史消息查看 | ✅ 正常 | ✅ 正常 | ✅ 正常 | ✅ 正常 |
| 设置 | ✅ 正常 | ✅ 正常 | ✅ 正常 | ✅ 正常 |
| Boot Check | ✅ 正常 | ✅ 正常 | ✅ 正常 | ✅ 正常 |
| 切换模型 | ✅ 正常 | ✅ 正常 | ✅ 正常 | ⚠️ 仅本地 |
| Failover | — | ✅ 自动切换 | ✅ 自动切换 | — |
| 音效 | `alert_error` | `alert_error` | `warning` | `alert_error` |
| 自动恢复 | 手动/健康检查 | 下次发送 | 倒计时/重试 | 健康检查 |

---

> **文档完成（终极版）。** 22 + 3 = 25 个章节。
>
> | 新增 | 内容 |
> |------|------|
> | §23 窗口响应式 | 3 级断点 + Compact 折叠规则 + 高度响应 + 面板限制 |
> | §24 性能预算 | 渲染/网络/内存/存储/启动 5 维性能目标 + Trace 集成 |
> | §25 离线降级 | 6 种故障场景 + UI 禁用/可用矩阵 + Failover 通知 + 自动恢复 |
>
> 配合使用:
> - `AnyClaw_LVGL_PRD_v2.2.1.md` — 产品需求 (做什么)
> - `AnyClaw_LVGL_PRD_v2.2.1_UI_layouts.md` — 页面布局 (65 个 UI 场景)
> - `AnyClaw_LVGL_PRD_v2.2.1_Design_System.md` — **本文档** (25 章完整规范)
