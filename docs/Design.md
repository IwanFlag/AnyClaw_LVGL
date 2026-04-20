# AnyClaw LVGL — 设计规范 v2.2.2

> 版本：v2.2.11 | 目标用户：大学生 & 职场年轻人 | 更新时间：2026-04-21
> 本文档定义 AnyClaw "长什么样、怎么画、画在哪"。产品需求见 `PRD.md`。

---

## 编号索引（修改前必查）

> ⚠️ 引用锚点用功能编号（如 CI-01），不用章节号。修改任何功能时查此表找到所有关联节。

| PRD 编号 | PRD 章节 | 功能名 | Design UI 编号 | Design 章节 | UI 状态 |
|----------|---------|--------|---------------|------------|---------|
| INST-01 | §2.2 | 实例互斥 | UI-02 | §11 首次体验 | ✅ |
| SC-01 | §2.3 | 启动自检 | UI-01, UI-03 | §11 首次体验 | ✅ |
| WIZ-01 | §2.4 | 首次启动向导 | UI-04~10 | §11 首次体验 | ✅/🔧 |
| WIN-01 | §3.1 | 窗口与布局 | UI-11, UI-12, UI-55 | §12 窗口系统 | ✅/🔧 |
| TRAY-01 | §3.2 | 系统托盘 | UI-63 | §12 窗口系统 | ✅ |
| I18N-01 | §3.3 | 国际化 | — | §3 排版系统 | ✅ |
| SNAP-01 | §3.4 | 贴边大蒜头 | UI-13 | §12 窗口系统 | ✅ |
| CHAT-01 | §4.1 | 聊天架构 | — | — | ✅ |
| CI-01 | §4.2 | 输入与消息 | UI-17 | §13 Chat 界面 | ✅ |
| MB-01 | §4.2 | 消息气泡与流式 | UI-14, UI-15, UI-16 | §13 Chat 界面 | ✅ |
| MM-01 | §4.3 | 文件与多模态 | UI-18, UI-36 | §13 Chat 界面 | 🔧 |
| MO-01 | §4.4 | 消息操作 | UI-19, UI-20 | §13 Chat 界面 | ✅ |
| SM-01 | §4.5 | 会话管理 | — | §12 窗口系统 | ✅ |
| PROACTIVE-01 | §4.6 | Agent 主动行为 | — | — | ✅ |
| VOICE-01 | §4.7 | 语音输入 | UI-17 | §13 Chat 界面 | ⏸️ |
| WORK-01 | §5 | Work 模式 | UI-12, UI-23~30, UI-44 | §14 Work 界面 | 🔧 |
| MODE-01 | §6.1 | 界面模式切换 | UI-21 | §15 交互组件 | 🔧 |
| AIMODE-01 | §6.2 | AI 交互模式 | UI-22, UI-29, UI-32 | §15 交互组件 | ✅ |
| CTRL-01 | §6.3 | UI 控制权模式 | UI-22 | §15 交互组件 | 🔧 |
|| RT-01 | §12.1.5 | 运行时系统 | — | §12 窗口系统 | 🔧 |
|| RT-01 | §12.1.5 | 三 Agent 并行（Leader 模式） | — | §12 窗口系统 | 🔧 |
| MODEL-01 | §7.2 | 云端模型管理 | UI-35, UI-47 | §16 配置界面 | ✅ |
| LOCAL-01 | §7.3 | 本地模型管理 | UI-07, UI-42 | §16 配置界面 | 🔧 |
| FAILOVER-01 | §7.4 | Model Failover | UI-47 | §16 配置界面 | ✅ |
| SK-01 | §8.2 | Skill 管理 | UI-65 | §17 扩展界面 | 🔧 |
| PERM-01 | §9.1 | 权限系统 | UI-31, UI-46 | §18 权限与弹窗 | 🔧 |
| WS-01 | §10 | 工作区管理 | UI-39~41, UI-64 | §19 工作区界面 | 🔧 |
| LOG-01 | §13.1 | 日志 | UI-48 | §20 设置页 | 🔧 |
| FF-01 | §13.5 | Feature Flags | UI-50 | §20 系统界面 | 🔧 |
| OBS-01 | §13.6 | Tracing | UI-51 | §20 系统界面 | 🔧 |
| LIC-01 | §13.7 | 商业授权 | UI-33 | §21 系统界面 | ✅ |
| — | — | 自定义模型弹窗 | UI-35 | §16 配置界面 | ⏳ |
| — | — | 知识库面板 | UI-60 | §17 扩展界面 | ⏳ |
| — | — | 远程协助 | UI-59 | §17 扩展界面 | ⏳ |
| — | — | 崩溃日志 | UI-56 | §21 系统界面 | ⏳ |
| — | — | 更新检测 | UI-57 | §21 系统界面 | ⏳ |
| — | — | 托盘气泡通知 | UI-58 | §21 系统界面 | ⏳ |

---

## 目录

**第一层：基础规范**
- §1 设计原则
- §2 色彩系统
- §3 排版系统
- §4 间距与栅格
- §5 图标系统

**第二层：控件与状态**
- §6 控件库
- §7 状态系统
- §8 动效系统
- §9 音效系统
- §10 无障碍 · 性能 · 离线降级

**第三层：界面设计**
- §11 首次体验（UI-01~10）
- §12 窗口系统（UI-11~13, UI-52~56, UI-63）
- §13 Chat 界面（UI-14~22）
- §14 Work 界面（UI-23~30, UI-44）
- §15 交互组件（UI-21, UI-22）
- §16 配置界面（UI-07, UI-42, UI-47）
- §17 扩展界面（UI-08, UI-43, UI-60, UI-65）
- §18 权限与弹窗（UI-31, UI-32, UI-46）
- §19 工作区界面（UI-39~41, UI-64）
- §20 设置页（UI-45~51）
- §21 系统界面（UI-33, UI-34, UI-37, UI-38, UI-56~58）

**第四层：品牌**
- §22 品牌文案

---

# 第一层：基础规范

## §1 设计原则

> 需求 → PRD §1 产品概览

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

## §2 色彩系统

> 需求 → PRD §3 主界面

### 2.1 主题体系

五套主题，三套主打 + 两套备选，通过 Settings General Tab 切换，即时生效无需重启。

**主打主题（完整规格）：**

| 主题 ID | 名称 | 定位 | 默认 |
|---------|------|------|------|
| `matcha` | 抹茶 Matcha v1 | 深色系，薄荷绿清新风，长时间使用 | ✅ 默认 |
| `peachy` | 桃气 Peachy v2 | 暖色系，珊瑚橘苹果风，年轻人友好 | |
| `mochi` | 糯 Mochi v3 | 米白系，奶茶棕温柔风，质感文艺 | |

**备选主题（精简规格）：**

| 主题 ID | 名称 | 定位 |
|---------|------|------|
| `dark` | 经典暗色 | 低饱和度偏好，VS Code 风 |
| `light` | 亮色 | 日间/高亮环境 |

---

# 🍵 主题一：抹茶 Matcha v1（默认主打）

> 需求 → PRD §3 主界面

#### 2.2.1 基础色板

| 语义 Token | 色值 | 用途 |
|------------|------|------|
| `bg` | `#0F1117` | 窗口最底层背景 |
| `surface` | `#13151E` | 输入区、低层容器 |
| `panel` | `#1A1D27` | 面板、卡片、弹窗 |
| `raised` | `#1E2230` | 气泡、浮层、tooltip |
| `overlay` | `#252A38` | hover 态、选中态 |

#### 2.2.2 文字色

| Token | 色值 | 用途 |
|-------|------|------|
| `text_primary` | `#E8ECF4` | 主文字、标题 |
| `text_secondary` | `#8B92A8` | 次要文字、说明 |
| `text_tertiary` | `#6B7280` | 占位符、禁用态 |
| `text_inverse` | `#0F1117` | 深色背景上的白字 |

#### 2.2.3 强调色

| Token | 色值 | 用途 |
|-------|------|------|
| `accent` | `#3DD68C` | 主强调色、品牌色、成功 |
| `accent_hover` | `#2BB673` | 主按钮 hover |
| `accent_active` | `#1FA35E` | 主按钮 active |
| `accent_subtle` | `#3DD68C1A` | 背景着色（10% opacity） |
| `accent_secondary` | `#6C9FFF` | 辅助蓝、链接 |

#### 2.2.4 语义色

| Token | 色值 | 用途 |
|-------|------|------|
| `success` | `#3DD68C` | 成功、完成、Ready |
| `warning` | `#FFBE3D` | 警告、Busy |
| `danger` | `#FF6B6B` | 错误、删除、Error |
| `info` | `#6C9FFF` | 信息、Checking |

#### 2.2.5 功能色

| Token | 色值 | 用途 |
|-------|------|------|
| `border` | `#3DD68C0A` | 面板边框 |
| `border_strong` | `#3DD68C20` | 输入框焦点边框 |
| `divider` | `#3DD68C08` | 分隔线 |
| `focus_glow` | `#3DD68C30` | 焦点光晕 |
| `hover_overlay` | `#FFFFFF0A` | 通用 hover 叠加 |
| `active_overlay` | `#FFFFFF14` | 通用 active 叠加 |
| `disabled_bg` | `#1A1D2780` | 禁用态背景 |
| `disabled_text` | `#6B728080` | 禁用态文字 |

#### 2.2.6 气泡专用色

| Token | 色值 | 用途 |
|-------|------|------|
| `bubble_user_bg` | `#2B5C3E` | 用户气泡底色 |
| `bubble_user_bg_end` | `#1F4A30` | 用户气泡渐变终点 |
| `bubble_ai_bg` | `#1E2230` | AI 气泡底色 |
| `bubble_ai_accent_bar` | `#3DD68C` | AI 气泡左侧竖线 |

#### 2.2.7 阴影

| Token | 值 | 用途 |
|-------|-----|------|
| `shadow_sm` | `0 1px 3px rgba(0,0,0,0.2)` | 卡片轻微浮起 |
| `shadow_md` | `0 2px 8px rgba(0,0,0,0.25)` | 气泡、tooltip |
| `shadow_lg` | `0 8px 24px rgba(0,0,0,0.35)` | 弹窗、dropdown |
| `shadow_glow` | `0 0 12px rgba(61,214,140,0.15)` | 焦点/品牌发光 |

#### 2.2.8 Matcha 控件色映射

| 控件 | Default | Hover | Active | Disabled |
|------|---------|-------|--------|----------|
| 主按钮 | bg=accent, text=text_inverse | bg=accent_hover | bg=accent_active, scale(0.97) | bg=disabled_bg, text=disabled_text |
| 次要按钮 | border=accent, text=accent | bg=accent_subtle | — | border=disabled_text |
| 发送按钮 | bg=accent gradient, BTN_SEND 圆形 | bg=accent_hover, glow | bg=accent_active | — |
| 输入框 | bg=surface, border=border | border=border_strong | border=accent, glow | bg=disabled_bg |
| Switch 关闭 | bg=#373C55, thumb=#8B92A8 | — | — | — |
| Switch 开启 | bg=accent, thumb=white | — | — | — |
| 用户气泡 | gradient #2B5C3E→#1F4A30 | — | — | — |
| AI 气泡 | bg=raised, 左侧 accent 竖线 | — | — | — |

---

# 🍑 主题二：桃气 Peachy v2

> 需求 → PRD §3 主界面

**设计理念：** "简单但不简陋，活泼但不花哨，温暖但不刺眼"

- 🧡 **暖色系**：珊瑚橘主色 + 暖橙辅色 + 粉红点缀
- 🍎 **苹果风**：大圆角 20-24px + 柔和阴影 + 暖白背景
- 🐱 **萌系元素**：emoji 点缀 + 彩色脉冲动画 + 友好文案
- 💫 **年轻化**：渐变按钮 + 弹性胶囊 + 装饰圆点

### 2.3.1 基础色板

| 语义 Token | 色值 | 用途 |
|------------|------|------|
| `bg` | `#FFF8F3` | 暖白背景 |
| `surface` | `#FFF1E8` | 暖浅面，输入区 |
| `panel` | `#FFFFFF` | 卡片、面板、弹窗 |
| `raised` | `#FFF5EE` | 气泡、浮层、tooltip |
| `overlay` | `#FFE8D6` | hover 态、选中态 |

#### 2.3.2 文字色

| Token | 色值 | 用途 |
|-------|------|------|
| `text_primary` | `#2D1B14` | 暖黑主文字 |
| `text_secondary` | `#8B7355` | 暖灰次要文字 |
| `text_tertiary` | `#B8A089` | 占位符、禁用态 |
| `text_inverse` | `#FFFFFF` | 深色背景上的白字 |

#### 2.3.3 强调色

| Token | 色值 | 用途 |
|-------|------|------|
| `accent` | `#FF7F50` | 珊瑚橘主强调色 |
| `accent_hover` | `#E86D3C` | 主按钮 hover |
| `accent_active` | `#CC5A2A` | 主按钮 active |
| `accent_subtle` | `#FF7F501A` | 背景着色（10% opacity） |
| `accent_secondary` | `#FFB347` | 暖橙辅色 |
| `accent_tertiary` | `#FF6B8A` | 粉红点缀 |

#### 2.3.4 语义色

| Token | 色值 | 用途 |
|-------|------|------|
| `success` | `#7ECFB3` | 薄荷绿，成功、完成 |
| `warning` | `#FFB347` | 暖橙，警告、Busy |
| `danger` | `#FF5C5C` | 暖红，错误、删除 |
| `info` | `#B8A9E8` | 薰衣草紫，信息、Checking |

#### 2.3.5 功能色

| Token | 色值 | 用途 |
|-------|------|------|
| `border` | `#F0E0D0` | 暖边框 |
| `border_strong` | `#E0C8B0` | 输入框焦点边框 |
| `divider` | `#F5E6D8` | 分隔线 |
| `focus_glow` | `#FF7F5030` | 焦点光晕 |
| `hover_overlay` | `#FF7F500A` | 通用 hover 叠加 |
| `active_overlay` | `#FF7F5014` | 通用 active 叠加 |
| `disabled_bg` | `#FFF1E880` | 禁用态背景 |
| `disabled_text` | `#B8A08980` | 禁用态文字 |

#### 2.3.6 气泡专用色

| Token | 色值 | 用途 |
|-------|------|------|
| `bubble_user_bg` | `#FFE8D6` | 用户气泡底色（暖杏） |
| `bubble_user_bg_end` | `#FFD6BE` | 用户气泡渐变终点 |
| `bubble_ai_bg` | `#FFFFFF` | AI 气泡底色（纯白） |
| `bubble_ai_accent_bar` | `#FF7F50` | AI 气泡左侧竖线（珊瑚橘） |

#### 2.3.7 阴影

| Token | 值 | 用途 |
|-------|-----|------|
| `shadow_sm` | `0 1px 3px rgba(139,80,50,0.08)` | 卡片轻微浮起 |
| `shadow_md` | `0 2px 8px rgba(139,80,50,0.12)` | 气泡、tooltip |
| `shadow_lg` | `0 8px 24px rgba(139,80,50,0.18)` | 弹窗、dropdown |
| `shadow_glow` | `0 0 12px rgba(255,127,80,0.15)` | 焦点/品牌发光 |

#### 2.3.8 Peachy 控件色映射

| 控件 | Default | Hover | Active | Disabled |
|------|---------|-------|--------|----------|
| 主按钮 | bg=accent, text=text_inverse, radius=12 | bg=accent_hover | bg=accent_active, scale(0.97) | bg=disabled_bg, text=disabled_text |
| 次要按钮 | border=accent, text=accent | bg=accent_subtle | — | border=disabled_text |
| 发送按钮 | bg=accent gradient, BTN_SEND 圆形 | bg=accent_hover, glow | bg=accent_active | — |
| 输入框 | bg=panel, border=border, radius=12 | border=border_strong | border=accent, glow | bg=disabled_bg |
| Switch 关闭 | bg=#E0D0C0, thumb=#B8A089 | — | — | — |
| Switch 开启 | bg=accent, thumb=white | — | — | — |
| 用户气泡 | bg=暖杏 #FFE8D6 | — | — | — |
| AI 气泡 | bg=白色 #FFFFFF, 左侧 accent 竖线 | — | — | — |

#### 2.3.9 Peachy 特色元素

| 元素 | 说明 |
|------|------|
| 圆角放大 | 卡片/弹窗 radius=radius_2xl（Matcha 为 radius_lg） |
| 渐变按钮 | 主按钮使用 `linear-gradient(135deg, #FF7F50, #FFB347)` |
| 装饰圆点 | 空状态、向导背景添加半透明装饰圆点 |
| 暖色阴影 | 阴影色相偏暖（rgba 含暖棕分量） |
| emoji 风格 | 操作图标优先使用 emoji（🍑🧡💫） |

---

# 🍡 主题三：糯 Mochi v3

> 需求 → PRD §3 主界面

**设计理念：** "温润如玉，静谧如茶，精致如器"

- 🤎 **米白暖棕**：奶茶棕主色 + 香槟金辅色 + 豆沙粉点缀
- 🫖 **茶道美学**：克制圆角 SCALE(base) 随主题倍率 + 柔和阴影 + 米白留白
- 📖 **文艺质感**：细线图标 + 衬线点缀 + 安静的高级感
- 🎋 **侘寂风**：不张扬、不喧哗，越看越舒服

**与现有主题的互补定位：**

| 维度 | Matcha v1 | Peachy v2 | Mochi v3 |
|------|-----------|-----------|----------|
| 色温 | 冷（暗底绿调） | 暖（亮底橘调） | 中性暖（米白棕调） |
| 情绪 | 清新、锐利、科技 | 活泼、可爱、社交 | 温柔、安静、文艺 |
| 目标 | 效率型用户 | 年轻社交用户 | 阅读/写作型用户 |
| 类比 | Linear / VS Code | Apple / Discord | Notion / Bear |

### 2.4.1 基础色板

| 语义 Token | 色值 | 用途 |
|------------|------|------|
| `bg` | `#FAF6F0` | 米白背景 |
| `surface` | `#F3EDE4` | 浅暖面，输入区 |
| `panel` | `#FFFDF9` | 卡片、面板（暖白） |
| `raised` | `#EDE7DC` | 气泡、浮层、tooltip |
| `overlay` | `#E4DDD0` | hover 态、选中态 |

#### 2.4.2 文字色

| Token | 色值 | 用途 |
|-------|------|------|
| `text_primary` | `#3D3226` | 深棕主文字 |
| `text_secondary` | `#8B7D6B` | 暖灰次要文字 |
| `text_tertiary` | `#B0A394` | 占位符、禁用态 |
| `text_inverse` | `#FFFDF9` | 深色背景上的白字 |

#### 2.4.3 强调色

| Token | 色值 | 用途 |
|-------|------|------|
| `accent` | `#A67B5B` | 奶茶棕主强调色 |
| `accent_hover` | `#8F6A4D` | 主按钮 hover |
| `accent_active` | `#7A5A40` | 主按钮 active |
| `accent_subtle` | `#A67B5B15` | 背景着色（8% opacity） |
| `accent_secondary` | `#C9A96E` | 香槟金辅色 |
| `accent_tertiary` | `#C4868C` | 豆沙粉点缀 |

#### 2.4.4 语义色

| Token | 色值 | 用途 |
|-------|------|------|
| `success` | `#7EA87A` | 苔藓绿，成功、完成 |
| `warning` | `#C9A96E` | 香槟金，警告、Busy |
| `danger` | `#C47070` | 砖红，错误、删除 |
| `info` | `#7B98A6` | 青瓷蓝，信息、Checking |

#### 2.4.5 功能色

| Token | 色值 | 用途 |
|-------|------|------|
| `border` | `#D6CFC4` | 暖边框 |
| `border_strong` | `#C4BBAD` | 输入框焦点边框 |
| `divider` | `#E8E2D8` | 分隔线 |
| `focus_glow` | `#A67B5B25` | 焦点光晕 |
| `hover_overlay` | `#A67B5B08` | 通用 hover 叠加 |
| `active_overlay` | `#A67B5B12` | 通用 active 叠加 |
| `disabled_bg` | `#F3EDE480` | 禁用态背景 |
| `disabled_text` | `#B0A39480` | 禁用态文字 |

#### 2.4.6 气泡专用色

| Token | 色值 | 用途 |
|-------|------|------|
| `bubble_user_bg` | `#E8DFD0` | 用户气泡底色（暖杏） |
| `bubble_user_bg_end` | `#DDD2C0` | 用户气泡渐变终点 |
| `bubble_ai_bg` | `#FFFDF9` | AI 气泡底色（暖白） |
| `bubble_ai_accent_bar` | `#A67B5B` | AI 气泡左侧竖线（奶茶棕） |

#### 2.4.7 阴影

| Token | 值 | 用途 |
|-------|-----|------|
| `shadow_sm` | `0 1px 3px rgba(120,100,80,0.06)` | 卡片轻微浮起 |
| `shadow_md` | `0 2px 8px rgba(120,100,80,0.10)` | 气泡、tooltip |
| `shadow_lg` | `0 8px 24px rgba(120,100,80,0.14)` | 弹窗、dropdown |
| `shadow_glow` | `0 0 12px rgba(166,123,91,0.12)` | 焦点/品牌发光 |

#### 2.4.8 Mochi 控件色映射

| 控件 | Default | Hover | Active | Disabled |
|------|---------|-------|--------|----------|
| 主按钮 | bg=accent, text=text_inverse, radius=8 | bg=accent_hover | bg=accent_active, scale(0.97) | bg=disabled_bg, text=disabled_text |
| 次要按钮 | border=accent, text=accent | bg=accent_subtle | — | border=disabled_text |
| 发送按钮 | bg=accent, BTN_SEND 圆形 | bg=accent_hover, glow | bg=accent_active | — |
| 输入框 | bg=panel, border=border, radius=8 | border=border_strong | border=accent, glow | bg=disabled_bg |
| Switch 关闭 | bg=#D6CFC4, thumb=#B0A394 | — | — | — |
| Switch 开启 | bg=accent, thumb=white | — | — | — |
| 用户气泡 | bg=暖杏 #E8DFD0 | — | — | — |
| AI 气泡 | bg=暖白 #FFFDF9, 左侧 accent 竖线 | — | — | — |

#### 2.4.9 Mochi 特色元素

| 元素 | 说明 |
|------|------|
| 圆角克制 | 卡片/弹窗 radius=radius_lg（介于 Matcha 和 Peachy 之间） |
| 纯色按钮 | 不用渐变，用 `bg=accent` 纯色，更安静 |
| 细线图标 | 图标描边 SCALE(1.5)（默认 SCALE(2)），更精致 |
| 暖色留白 | 大量 padding + bg 色差微妙（surface vs panel 差 5%） |
| 豆沙粉点缀 | 特殊操作（收藏、标记）使用 accent_tertiary #C4868C |

#### 2.4.10 Mochi 字体

| 类型 | 主选 | 备选 | 回退 |
|------|------|------|------|
| 中文 | **思源宋体 SC** | LXGW WenKai | 思源黑体 SC, serif |
| 英文 | **Lora** | Merriweather | Georgia, serif |
| 等宽 | JetBrains Mono | Fira Code | Consolas, monospace |

> Mochi 是三个主打主题中唯一使用衬线体的——思源宋体 + Lora 传递"安静阅读"的质感。标题用衬线，正文 UI 仍用 HarmonyOS Sans SC（可读性）。

**混合策略：**

| 场景 | 使用字体 |
|------|---------|
| 正文、消息、UI 控件 | HarmonyOS Sans SC / Plus Jakarta Sans（无衬线，保证可读性） |
| 标题、品牌文案、About 页 | 思源宋体 SC / Lora（衬线，传递质感） |
| 引用块、笔记 | 思源宋体 SC / Lora（衬线，阅读感） |
| 代码块 | JetBrains Mono（等宽） |

---

# 🌑 备选主题

### 2.5 经典暗色 (dark)

> VS Code 风，低饱和度偏好。

| Token | 色值 |
|-------|------|
| `bg` | `#1E1E1E` |
| `surface` | `#252526` |
| `panel` | `#2D2D30` |
| `raised` | `#383838` |
| `overlay` | `#404040` |
| `text_primary` | `#CCCCCC` |
| `text_secondary` | `#808080` |
| `accent` | `#007ACC` |
| `success` | `#4EC9B0` |
| `warning` | `#CE9178` |
| `danger` | `#F44336` |
| `info` | `#569CD6` |
| `bubble_user_bg` | `#264F78` |
| `bubble_ai_bg` | `#2D2D30` |
| `border` | `#3E3E42` |

### 2.6 亮色 (light)

> 日间/高亮环境。

| Token | 色值 |
|-------|------|
| `bg` | `#F5F7FA` |
| `surface` | `#EBEEF2` |
| `panel` | `#FFFFFF` |
| `raised` | `#FFFFFF` |
| `overlay` | `#E2E5EA` |
| `text_primary` | `#1A1D27` |
| `text_secondary` | `#6B7280` |
| `accent` | `#2BB673` |
| `success` | `#16A34A` |
| `warning` | `#D97706` |
| `danger` | `#DC2626` |
| `info` | `#3B82F6` |
| `bubble_user_bg` | `#DCFCE7` |
| `bubble_ai_bg` | `#F1F5F9` |
| `border` | `#E2E5EA` |

### 2.7 怀旧暗色 (classic)

> 极简深灰，保留兼容。

| Token | 色值 |
|-------|------|
| `bg` | `#2D2D2D` |
| `surface` | `#333333` |
| `panel` | `#3C3C3C` |
| `raised` | `#444444` |
| `overlay` | `#505050` |
| `text_primary` | `#FFFFFF` |
| `text_secondary` | `#B0B0B0` |
| `accent` | `#82AAF0` |
| `success` | `#4CAF50` |
| `warning` | `#FFC107` |
| `danger` | `#F44336` |
| `info` | `#2196F3` |
| `bubble_user_bg` | `#4070C0` |
| `bubble_ai_bg` | `#3C3C3C` |
| `border` | `#555555` |

---

### 2.7 色彩使用规则

| 规则 | 说明 |
|------|------|
| 同层元素不加边框 | `panel` 上的 `card` 不加 border |
| 相邻层 GAP_SMALL×0.25 分隔线 | `divider` 色，opacity 4-8% |
| 弹窗从 `panel` 层开始 | 遮罩 Matcha `rgba(0,0,0,0.6)` / Peachy `rgba(0,0,0,0.3)` / Mochi `rgba(0,0,0,0.35)` |
| 最多同时使用 3 种强调色 | accent + 1 语义色 + 1 功能色 |
| 红色只用于危险/错误 | 不用于装饰 |
| Peachy 圆角放大 | 卡片/弹窗 radius +SCALE(8) 对比 Matcha |
| Mochi 圆角微收敛 | 卡片/弹窗 radius -SCALE(2) 对比 Matcha |
| 主题切换 | 300ms 全局色值过渡，不硬切 |

### 2.8 三套主打主题完整对照表

| Token | 🍵 Matcha v1 | 🍑 Peachy v2 | 🍡 Mochi v3 |
|-------|-------------|-------------|------------|
| `bg` | `#0F1117` | `#FFF8F3` | `#FAF6F0` |
| `surface` | `#13151E` | `#FFF1E8` | `#F3EDE4` |
| `panel` | `#1A1D27` | `#FFFFFF` | `#FFFDF9` |
| `raised` | `#1E2230` | `#FFF5EE` | `#EDE7DC` |
| `overlay` | `#252A38` | `#FFE8D6` | `#E4DDD0` |
| `text_primary` | `#E8ECF4` | `#2D1B14` | `#3D3226` |
| `text_secondary` | `#8B92A8` | `#8B7355` | `#8B7D6B` |
| `text_tertiary` | `#6B7280` | `#B8A089` | `#B0A394` |
| `accent` | `#3DD68C` | `#FF7F50` | `#A67B5B` |
| `accent_secondary` | `#6C9FFF` | `#FFB347` | `#C9A96E` |
| `accent_tertiary` | — | `#FF6B8A` | `#C4868C` |
| `success` | `#3DD68C` | `#7ECFB3` | `#7EA87A` |
| `warning` | `#FFBE3D` | `#FFB347` | `#C9A96E` |
| `danger` | `#FF6B6B` | `#FF5C5C` | `#C47070` |
| `info` | `#6C9FFF` | `#B8A9E8` | `#7B98A6` |
| `bubble_user_bg` | `#2B5C3E` | `#FFE8D6` | `#E8DFD0` |
| `bubble_ai_bg` | `#1E2230` | `#FFFFFF` | `#FFFDF9` |
| `border` | `#3DD68C0A` | `#F0E0D0` | `#D6CFC4` |
| `focus_glow` | `#3DD68C30` | `#FF7F5030` | `#A67B5B25` |
| 英文字体 | Plus Jakarta Sans | Nunito | Plus Jakarta Sans(正文) / Lora(标题) |
| 中文字体 | HarmonyOS Sans SC | HarmonyOS Sans SC | HarmonyOS Sans SC(正文) / 思源宋体SC(标题) |
| 圆角风格 | 标准 SCALE(8/12/16) | 放大 SCALE(12/20/22) | 微收敛 SCALE(8/10/14) |
| 阴影色相 | 冷黑 | 暖棕 | 暖棕（更淡） |
| 按钮风格 | accent 渐变圆形 | 双色渐变圆形 | accent 纯色圆形 |

---

## §3 排版系统

> 需求 → PRD §3 主界面

### 3.1 字体栈（5 主题）

#### 🍵 Matcha v1 — 清新锐利

| 类型 | 主选 | 备选 | 回退 |
|------|------|------|------|
| 中文（UI/正文/标题通用） | **HarmonyOS Sans SC** | 思源黑体 SC | PingFang SC, 微软雅黑, sans-serif |
| 英文（UI/正文） | **Plus Jakarta Sans** | Inter | Segoe UI, sans-serif |
| 英文（标题/品牌） | **Plus Jakarta Sans Bold** | Inter Bold | Segoe UI Bold, sans-serif |
| 等宽 | **JetBrains Mono** | Fira Code | Consolas, monospace |

> Plus Jakarta Sans 比 Inter 更圆润有温度，比 Montserrat 更现代。HarmonyOS Sans SC 比微软雅黑清晰锐利，Variable Weight 支持好。全主题统一无衬线，干净利落。

#### 🍑 Peachy v2 — 温暖亲和

| 类型 | 主选 | 备选 | 回退 |
|------|------|------|------|
| 中文（UI/正文/标题通用） | **HarmonyOS Sans SC** | 思源黑体 SC | PingFang SC, 微软雅黑, sans-serif |
| 英文（UI/正文） | **Nunito** | Quicksand | Segoe UI, sans-serif |
| 英文（标题/品牌） | **Nunito Bold** | Quicksand Bold | Segoe UI Bold, sans-serif |
| 等宽 | **JetBrains Mono** | Fira Code | Consolas, monospace |

> Nunito 圆端设计天然萌系，和珊瑚橘暖色搭配完美。字重偏轻（Regular=400 感觉比其他字体细），标题用 Bold(700) 对比更鲜明。

#### 🍡 Mochi v3 — 文艺质感（唯一混用衬线体）

| 类型 | 用途 | 主选 | 备选 | 回退 |
|------|------|------|------|------|
| 中文 UI 控件 | 按钮/输入/菜单 | **HarmonyOS Sans SC** | 思源黑体 SC | PingFang SC, sans-serif |
| 中文 标题/引用 | 弹窗标题/引用块 | **思源宋体 SC** | LXGW WenKai | Noto Serif SC, serif |
| 英文 UI 控件 | 按钮/输入/标签 | **Plus Jakarta Sans** | Inter | Segoe UI, sans-serif |
| 英文 标题/引用 | 品牌/About/引用 | **Lora** | Merriweather | Georgia, serif |
| 等宽 | 代码/日志 | **JetBrains Mono** | Fira Code | Consolas, monospace |

> Mochi 混用衬线体——UI 控件用无衬线保证可读性，标题和引用用衬线传递「安静阅读」质感。思源宋体 SC + Lora 传递茶道美学。

#### 🌑 经典暗色 Dark — VS Code 风

| 类型 | 主选 | 备选 | 回退 |
|------|------|------|------|
| 中文 | **思源黑体 SC** | HarmonyOS Sans SC | 微软雅黑, sans-serif |
| 英文 | **Inter** | SF Pro Display | Segoe UI, sans-serif |
| 等宽 | **JetBrains Mono** | Fira Code | Consolas, monospace |

> 经典暗色偏好开发者风格，Inter + 思源黑体是经典组合。

#### ☀️ 亮色 Light — 日间高亮

| 类型 | 主选 | 备选 | 回退 |
|------|------|------|------|
| 中文 | **HarmonyOS Sans SC** | 思源黑体 SC | PingFang SC, 微软雅黑, sans-serif |
| 英文 | **Inter** | SF Pro Display | Segoe UI, sans-serif |
| 等宽 | **JetBrains Mono** | Fira Code | Consolas, monospace |

> 亮色主题追求简洁通用，Inter + HarmonyOS Sans SC 干净无干扰。

### 3.2 字号体系

> 所有字号 = 窗口高 × 比例 / 100。基准：窗口高 800px → body 13px。
> 实际像素 = `int(窗口高 × PCT / 100 + 0.5)`，不低于最小值。

| Token | PCT (/100) | 800px 基准 | 最小值 | 字重 | 用途 |
|-------|-----------|-----------|--------|------|------|
| `display` | 3.50% | 28px | 20px | 700 | 品牌标题 |
| `h1` | 2.75% | 22px | 16px | 700 | 弹窗标题 |
| `h2` | 2.25% | 18px | 14px | 600 | 区域标题 |
| `h3` | 1.88% | 15px | 12px | 600 | 卡片标题 |
| `body` | 1.63% | 13px | 11px | 400 | 正文、消息内容 |
| `body_strong` | 1.63% | 13px | 11px | 600 | 强调正文 |
| `small` | 1.38% | 11px | 10px | 400 | 次要信息、时间戳 |
| `caption` | 1.25% | 10px | 9px | 500 | 标签、胶囊状态 |
| `code` | 1.50% | 12px | 10px | 400 | 代码块内文字 |

> 示例：窗口高 1200px 时 body = 1200 × 1.63% ≈ 20px；窗口高 600px 时 body = 10px（触发最小值）。

### 3.3 字体 × 场景矩阵

> 每个 UI 元素 = 字体族 + 字号 Token + 字重。按主题分列。
> "无衬线" = 该主题的 UI 正文字体；"衬线" = 该主题的标题/引用字体（仅 Mochi 有）。

| UI 元素 | 字号 | 字重 | 🍵 Matcha | 🍑 Peachy | 🍡 Mochi | 🌑 Dark | ☀️ Light |
|---------|------|------|-----------|-----------|----------|---------|----------|
| **品牌标题** (标题栏/About) | display | 700 | PJS Bold | Nunito Bold | Lora Bold | Inter Bold | Inter Bold |
| **弹窗标题** | h1 | 700 | PJS Bold | Nunito Bold | 思源宋体 SC Bold | Inter Bold | Inter Bold |
| **区域标题** (面板/Tab) | h2 | 600 | PJS SemiBold | Nunito SemiBold | 思源宋体 SC Bold | Inter SemiBold | Inter SemiBold |
| **卡片标题** (StepCard/Session) | h3 | 600 | PJS SemiBold | Nunito SemiBold | PJS SemiBold | Inter SemiBold | Inter SemiBold |
| **正文** (消息/内容) | body | 400 | PJS Regular | Nunito Regular | PJS Regular | Inter Regular | Inter Regular |
| **强调正文** | body | 600 | PJS SemiBold | Nunito SemiBold | PJS SemiBold | Inter SemiBold | Inter SemiBold |
| **Chat 消息** | body | 400 | PJS Regular | Nunito Regular | PJS Regular | Inter Regular | Inter Regular |
| **Chat 消息（Mochi引用块）** | body | 400 | — | — | Lora Regular | — | — |
| **按钮文字** | body | 600 | PJS SemiBold | Nunito SemiBold | PJS SemiBold | Inter SemiBold | Inter SemiBold |
| **输入框文字** | body | 400 | PJS Regular | Nunito Regular | PJS Regular | Inter Regular | Inter Regular |
| **输入框占位符** | body | 400 | PJS Regular | Nunito Regular | PJS Regular | Inter Regular | Inter Regular |
| **下拉选项** | body | 400 | PJS Regular | Nunito Regular | PJS Regular | Inter Regular | Inter Regular |
| **次要信息** (时间戳/说明) | small | 400 | PJS Regular | Nunito Regular | PJS Regular | Inter Regular | Inter Regular |
| **标签/胶囊** | caption | 500 | PJS Medium | Nunito Medium | PJS Medium | Inter Medium | Inter Medium |
| **代码块** | code | 400 | JetBrains Mono | JetBrains Mono | JetBrains Mono | JetBrains Mono | JetBrains Mono |
| **行内代码** | code | 400 | JetBrains Mono | JetBrains Mono | JetBrains Mono | JetBrains Mono | JetBrains Mono |
| **终端日志** | code | 400 | JetBrains Mono | JetBrains Mono | JetBrains Mono | JetBrains Mono | JetBrains Mono |
| **步骤指示器文字** | small | 400 | PJS Regular | Nunito Regular | PJS Regular | Inter Regular | Inter Regular |
| **向导步骤名** | caption | 500 | PJS Medium | Nunito Medium | PJS Medium | Inter Medium | Inter Medium |
| **设置行标签** | body | 400 | PJS Regular | Nunito Regular | PJS Regular | Inter Regular | Inter Regular |
| **设置行说明** | small | 400 | PJS Regular | Nunito Regular | PJS Regular | Inter Regular | Inter Regular |
| **Toast 通知** | body | 400 | PJS Regular | Nunito Regular | PJS Regular | Inter Regular | Inter Regular |
| **Tooltip** | small | 400 | PJS Regular | Nunito Regular | PJS Regular | Inter Regular | Inter Regular |
| **空状态文案** | body | 400 | PJS Regular | Nunito Regular | PJS Regular | Inter Regular | Inter Regular |
| **中文** (上述所有 CJK 场景) | 同上 | 同上 | HarmonyOS Sans SC | HarmonyOS Sans SC | 无衬线=HarmOS SC / 衬线=思源宋体 SC | 思源黑体 SC | HarmonyOS Sans SC |

**缩写：** PJS = Plus Jakarta Sans, HarmOS = HarmonyOS, 思源宋体 SC = Source Han Serif SC

**字重速查：**

| 字重值 | 名称 | 使用场景 |
|--------|------|---------|
| 400 | Regular | 正文、消息、输入、标签默认 |
| 500 | Medium | 胶囊状态、向导步骤名 |
| 600 | SemiBold | 强调正文、区域标题、按钮、卡片标题 |
| 700 | Bold | 品牌标题、弹窗标题 |

### 3.4 字体加载策略

#### 打包方式

| 文件 | 大小 | 包含变体 |
|------|------|---------|
| `PlusJakartaSans-Regular.ttf` | ~25KB | Regular 400 |
| `PlusJakartaSans-SemiBold.ttf` | ~25KB | SemiBold 600 |
| `PlusJakartaSans-Bold.ttf` | ~25KB | Bold 700 |
| `Nunito-Regular.ttf` | ~25KB | Regular 400 |
| `Nunito-Bold.ttf` | ~25KB | Bold 700 |
| `HarmonyOS_Sans_SC_Regular.otf` | ~2MB | Regular 400 |
| `HarmonyOS_Sans_SC_Bold.otf` | ~2MB | Bold 700 |
| `Lora-Regular.ttf` | ~150KB | Regular 400 |
| `Lora-Bold.ttf` | ~150KB | Bold 700 |
| `JetBrainsMono-Regular.ttf` | ~300KB | Regular 400 |
| **总计** | **~5MB** | |

> Nunito 不需要 SemiBold 变体（其 600 权重视觉上接近其他字体的 500，用 Regular + Bold 即可覆盖）。
> Mochi 的思源宋体 SC Bold 用于标题（对标其他主题的 SemiBold/Bold）。

#### 加载流程

```
启动 → 读取 config.json theme 字段
  → 根据主题 ID 选择字体族（见 3.1 字体栈表）
  → lv_tiny_ttf_create_file() 加载 .ttf/.otf，按 DPI 缩放
  → 创建 9 级字号对象（display/h1/h2/h3/body/small/caption/code + body_strong）
  → 注册为全局 g_theme_fonts 结构体
  → 主题切换时销毁旧字体 → 重新创建 → 触发 UI 刷新
```

#### 运行时字体结构体

```c
struct ThemeFonts {
    /* 英文/通用字体 */
    lv_font_t* display;       /* 品牌标题, 28px @800h, Bold */
    lv_font_t* h1;            /* 弹窗标题, 22px @800h, Bold */
    lv_font_t* h2;            /* 区域标题, 18px @800h, SemiBold */
    lv_font_t* h3;            /* 卡片标题, 15px @800h, SemiBold */
    lv_font_t* body;          /* 正文, 13px @800h, Regular */
    lv_font_t* body_strong;   /* 强调正文, 13px @800h, SemiBold */
    lv_font_t* small;         /* 次要信息, 11px @800h, Regular */
    lv_font_t* caption;       /* 标签胶囊, 10px @800h, Medium(500) */
    lv_font_t* code;          /* 代码, 12px @800h, Regular */

    /* 中文 CJK 字体（与英文同级，fallback 链接） */
    lv_font_t* cjk_body;      /* CJK 正文 */
    lv_font_t* cjk_title;     /* CJK 标题（仅 Mochi 用思源宋体） */
};
```

#### 各主题字体对应关系

| Font 指针 | 🍵 Matcha | 🍑 Peachy | 🍡 Mochi | 🌑 Dark | ☀️ Light |
|-----------|-----------|-----------|----------|---------|----------|
| `display` | PJS Bold 28px | Nunito Bold 28px | Lora Bold 28px | Inter Bold 28px | Inter Bold 28px |
| `h1` | PJS Bold 22px | Nunito Bold 22px | 思源宋体 Bold 22px | Inter Bold 22px | Inter Bold 22px |
| `h2` | PJS SemiBold 18px | Nunito SemiBold 18px | 思源宋体 Bold 18px | Inter SemiBold 18px | Inter SemiBold 18px |
| `h3` | PJS SemiBold 15px | Nunito SemiBold 15px | PJS SemiBold 15px | Inter SemiBold 15px | Inter SemiBold 15px |
| `body` | PJS Regular 13px | Nunito Regular 13px | PJS Regular 13px | Inter Regular 13px | Inter Regular 13px |
| `body_strong` | PJS SemiBold 13px | Nunito SemiBold 13px | PJS SemiBold 13px | Inter SemiBold 13px | Inter SemiBold 13px |
| `small` | PJS Regular 11px | Nunito Regular 11px | PJS Regular 11px | Inter Regular 11px | Inter Regular 11px |
| `caption` | PJS Medium 10px | Nunito Medium 10px | PJS Medium 10px | Inter Medium 10px | Inter Medium 10px |
| `code` | JetBrains Mono 12px | JetBrains Mono 12px | JetBrains Mono 12px | JetBrains Mono 12px | JetBrains Mono 12px |
| `cjk_body` | HarmOS SC Reg | HarmOS SC Reg | HarmOS SC Reg | 思源黑体 SC Reg | HarmOS SC Reg |
| `cjk_title` | HarmOS SC Bold | HarmOS SC Bold | 思源宋体 SC Bold | 思源黑体 SC Bold | HarmOS SC Bold |

> Mochi 的 `cjk_title` 使用思源宋体 SC（衬线），与其他主题不同。
> 所有 CJK 字体通过 `lv_font_t->fallback` 链接到对应英文主字体，实现中英混排自动切换。

#### 回退链

```
英文渲染请求 → 主字体（如 PJS Regular）
  → 找不到字形 → fallback → CJK 字体（如 HarmOS SC Regular）
    → 找不到字形 → fallback → 系统字体（msyh / segoeui）
      → 找不到字形 → LVGL 内置位图字体
```

#### 字号动态计算

```c
/* 所有字号通过此函数计算，不再硬编码 */
int font_size(int pct, int win_h, int min_px) {
    int sz = win_h * pct / 10000;  /* pct 如 FONT_BODY_PCT=163 → 1.63% */
    return (sz < min_px) ? min_px : sz;
}

/* 使用示例 */
int body_px = font_size(FONT_BODY_PCT, WIN_H, FONT_MIN_BODY);  /* 800h → 13px */
int h1_px   = font_size(FONT_H1_PCT,   WIN_H, FONT_MIN_H1);    /* 800h → 22px */
```

### 3.5 Markdown 渲染样式

| 元素 | 字体 | 样式 |
|------|------|------|
| `# H1` | h1 + cjk_title | 上边距 `GAP × 4` |
| `## H2` | h2 + cjk_title | 上边距 `GAP × 3` |
| **加粗** | body_strong | |
| `行内代码` | code | + `surface` 背景 + `radius_sm` 圆角 |
| ``` 代码块 ``` | code | + `surface` + `radius_md` 圆角 + padding `GAP × 3` |
| - 列表 | body | + 左缩进 `GAP × 4` |
| > 引用 | body (Mochi: cjk_title/衬线) | + `text_secondary` + 左侧 accent 竖线 (`GAP × 0.5` 宽) |
| [链接](url) | body | + `accent` + 下划线 |

### 3.6 文字排版规则

| 规则 | 值 |
|------|-----|
| 中文 CJK 逐字换行 | `LV_TEXT_FLAG_BREAK_ALL` |
| 消息气泡最大行宽 | 容器宽 × 70% |
| 段落间距 | `GAP × 2` |
| 文字截断 | 尾部 `...` 省略，最多 3 行 |
| 代码块背景 | `surface` 色, `radius_md` 圆角 |
| 选中文字高亮 | `accent` 色, opacity 30% |
| Mochi 引用块字体 | 切换为衬线体（Lora / 思源宋体 SC） |
| 字重切换动画 | 主题切换时字重不渐变，直接切换 |

---

## §4 间距与栅格

> 需求 → PRD §3 主界面
> 所有间距 = 窗口宽 × 比例，用 GAP 表示。SCALE() 仅用于 DPI 缩放视觉细节。

### 4.1 基础间距

> GAP = 窗口宽 × 1%，最小 4px。所有间距基于 GAP 的倍数或父容器百分比。

| Token | 公式 | 800px 基准 | 最小值 | 用途 |
|-------|------|-----------|--------|------|
| `GAP` | 窗口宽 × 1% | 8px | 4px | 基础间距单位 |
| `SAFE_PAD` | 窗口宽 × 1% | 8px | 4px | 通用内边距 |
| `GAP_SMALL` | GAP × 0.5 | 4px | 2px | 最小间距、图标与文字 |
| `GAP_MEDIUM` | GAP × 2 | 16px | 8px | 区域内元素间距 |
| `GAP_LARGE` | GAP × 3 | 24px | 12px | 区域间间距 |
| `GAP_XL` | GAP × 4 | 32px | 16px | 大区域分隔 |
| `CHAT_GAP` | 消息区宽 × 0.5% | 5px | 3px | 消息间距 |

### 4.2 圆角体系

> 圆角 = SCALE(base)，随 DPI 缩放。主题倍率影响实际值。

基础 Token：

| Token | base | 用途 |
|-------|------|------|
| `radius_sm` | 4 | 行内代码、小标签 |
| `radius_md` | 8 | 按钮、输入框 |
| `radius_lg` | 12 | 卡片、面板、弹窗 |
| `radius_xl` | 16 | 消息气泡 |
| `radius_2xl` | 20 | 模态框 |
| `radius_full` | 9999 | 胶囊标签、圆形头像 |

主题倍率（实际 = SCALE(base × 倍率)）：

| Token | Matcha v1 | Peachy v2 | Mochi v3 |
|-------|-----------|-----------|----------|
| `radius_sm` | 4 | 6 | 4 |
| `radius_md` | 8 | 12 | 8 |
| `radius_lg` | 12 | 20 | 10 |
| `radius_xl` | 16 | 22 | 14 |
| `radius_2xl` | 20 | 28 | 16 |
| `radius_full` | 9999 | 9999 | 9999 |

**风格总结：** Matcha 标准锐利 / Peachy 放大圆润 / Mochi 微收敛精致

### 4.3 容器尺寸常量

> 所有尺寸 = 父容器 × 百分比，不低于最小值。
> 容器层级：窗口 → 标题栏/内容区 → 左导航/左面板/右面板 → 消息区/输入区

#### 窗口（root）

| 常量 | 值 | 说明 |
|------|-----|------|
| `WIN_DEFAULT_W` | 1450 | 设计基准宽 |
| `WIN_DEFAULT_H` | 800 | 设计基准高 |
| `WIN_MIN_W` | 800 | 最小窗口宽 |
| `WIN_MIN_H` | 500 | 最小窗口高 |

#### 标题栏（parent: 窗口）

| Token | 公式 | 800px 基准 | 最小值 |
|-------|------|-----------|--------|
| `TITLE_H` | 窗口高 × 6% | 48px | 36px |
| `TITLE_ICON` | 标题栏高 × 80% | 38px | — |
| `TITLE_MODEL_W` | 窗口宽 × 10% | 145px | — |

#### 左导航栏（parent: 窗口）

| Token | 公式 | 800px 基准 | 最小值 |
|-------|------|-----------|--------|
| `NAV_W` | 窗口宽 × 4% | 32px | 40px |
| `NAV_ICON_BTN` | 导航宽 × 70% | 28px | — |
| `NAV_QUICK_H` | 内容区高 × 25% | — | — |

#### 左面板（parent: 可用宽 = 窗口宽 − 导航宽）

| Token | 公式 | 1450px 基准 | 最小值 |
|-------|------|------------|--------|
| `LEFT_PANEL_W` | 可用宽 × 25% | 353px | 160px |
| `LP_ROW_H` | 左面板高 × 8% | — | 24px |
| `TASK_ITEM_H` | 左面板高 × 8% | — | 24px |

#### 消息区（parent: 右面板）

| Token | 公式 | 800px 基准 | 最小值 |
|-------|------|-----------|--------|
| `CHAT_FEED_H` | 右面板高 × 85% | 612px | — |

#### 输入区（parent: 右面板）

| Token | 公式 | 800px 基准 | 最小值 |
|-------|------|-----------|--------|
| `CHAT_INPUT_H` | 输入区高 × 60% | — | 96px |
| `BTN_SEND` | 输入区高 × 45% | — | 32px |
| `BTN_UPLOAD` | 输入区高 × 30% | — | 24px |

#### 气泡（parent: 消息区）

| Token | 公式 | 说明 |
|-------|------|------|
| `CHAT_BUBBLE_MAX` | 消息区宽 × 70% | 最大宽度 |
| `CHAT_AVATAR` | 消息区宽 × 3%，clamp 20~32px | 头像尺寸 |

#### 加载遮罩（parent: 窗口）

| Token | 公式 | 1450×800 基准 | 最小值 |
|-------|------|-------------|--------|
| `LOADING_OVERLAY_W` | 窗口宽 × 32% | 464px | — |
| `LOADING_OVERLAY_H` | 窗口高 × 37% | 296px | — |
| `LOADING_ICON` | 遮罩高 × 21% | 62px | — |

#### 标题栏模式栏（parent: 窗口）

| Token | 公式 | 1450×800 基准 | 最小值 |
|-------|------|-------------|--------|
| `MODE_BAR_W` | 窗口宽 × 12% | 174px | 120px |
| `SIDE_BTN_W` | 窗口宽 × 5% | 72px | 48px |

#### 右键菜单项（parent: 窗口）

| Token | 公式 | 最小值 |
|-------|------|--------|
| `CTX_ITEM_H` | 窗口高 × 8% | 28px |

#### 弹窗（parent: 窗口）

| Token | 公式 | 1450×800 基准 | 最小值 |
|-------|------|-------------|--------|
| `DIALOG_W` | 窗口宽 × 40% | 580px | 300px |
| `DIALOG_H` | 窗口高 × 55% | 440px | 200px |
| `WIZARD_W` | 窗口宽 × 38% | 551px | 480px |
| `WIZARD_H` | 窗口高 × 60% | 480px | 400px |

#### Splitter

| Token | 公式 | 最小值 |
|-------|------|--------|
| `SPLITTER_W` | 窗口宽 × 0.5% | 3px |

#### 设置页

| Token | 公式 | 最小值 |
|-------|------|--------|
| `SETTINGS_ROW_H` | 设置面板高 × 4% | 28px |

---

## §5 图标系统

> 需求 → PRD §3 主界面

### 5.1 图标格式：SVG 优先

**统一使用 SVG**，不用 PNG 图标（品牌吉祥物除外）。

| 维度 | SVG | PNG |
|------|-----|-----|
| DPI 适配 | 天然矢量，任意缩放不失真 | 需要多套尺寸（1x/2x/3x） |
| 文件大小 | 简单图标 < 1KB | 16px 就要几百字节 |
| 主题换色 | 代码改 `fill` 即可 | 要重新切图 |
| 动画 | 支持 CSS/代码驱动描边动画 | 不支持 |
| 结论 | ✅ 所有 UI 图标 | 仅品牌吉祥物（大蒜/龙虾） |

**品牌吉祥物用 PNG：** 大蒜、龙虾等角色插画复杂度高，SVG 路径太多反而大，用 PNG（48/96/128px 三套）。

### 5.2 图标集

使用 **Lucide Icons**（开源 SVG 图标集，4000+ 图标，24px 标准网格）作为基础图标集，按需裁剪为项目自用子集（约 120 个）。

#### 通用 UI 图标

| 场景 | 图标名 | 用途 |
|------|--------|------|
| 设置 | `settings` | 标题栏、菜单 |
| 关闭 | `x` | 弹窗关闭、窗口控制 |
| 最小化 | `minus` | 窗口控制 |
| 最大化 | `square` | 窗口控制 |
| 搜索 | `search` | 搜索框、Chat 搜索 |
| 发送 | `send` | 发送按钮 |
| 停止 | `square` (fill) | 流式中断 |
| 复制 | `copy` | 消息操作 |
| 粘贴 | `clipboard` | 剪贴板操作 |
| 刷新 | `refresh-cw` | 重试、刷新 |
| 菜单 | `more-vertical` | 上下文菜单 |
| 返回 | `chevron-left` | 导航返回 |
| 展开 | `chevron-down` | 下拉箭头 |
| 折叠 | `chevron-up` | 下拉收起 |
| 添加 | `plus` | 新建 Session/Cron |
| 删除 | `trash-2` | 删除操作 |
| 编辑 | `pencil` | 编辑文件 |
| 下载 | `download` | 下载/导出 |
| 上传 | `upload` | 文件上传 |
| 链接 | `external-link` | 打开外部链接 |

#### 操作类型图标（StepCard 用）

| 操作 | 图标名 | Matcha 颜色 | Peachy 颜色 |
|------|--------|------------|------------|
| 读文件 | `file-text` | text_secondary | text_secondary |
| 写文件 | `file-edit` | accent (#3DD68C) | accent (#FF7F50) |
| 执行命令 | `terminal` | warning (#FFBE3D) | accent_secondary (#FFB347) |
| 搜索 | `search` | info (#6C9FFF) | accent_tertiary (#B8A9E8) |
| 成功 | `check-circle` | accent (#3DD68C) | success (#7ECFB3) |
| 失败 | `x-circle` | danger (#FF6B6B) | danger (#FF5C5C) |
| 进行中 | `loader-2` (spin) | accent (#3DD68C) | accent (#FF7F50) |
| 等待确认 | `help-circle` | info (#6C9FFF) | accent_tertiary (#B8A9E8) |
| 警告 | `alert-triangle` | warning (#FFBE3D) | accent_secondary (#FFB347) |
| 信息 | `info` | info (#6C9FFF) | accent_tertiary (#B8A9E8) |

#### 导航/区域图标

| 区域 | 图标名 | 用途 |
|------|--------|------|
| Chat | `message-circle` | 模式切换 |
| Work | `briefcase` | 模式切换 |
| Session | `layers` | 左侧面板 |
| Cron | `clock` | 左侧面板 |
| Agent | `radio` / `wifi` | Agent 状态 |
| 模型 | `cpu` | 模型配置 |
| 权限 | `shield` | 权限设置 |
| 工作区 | `folder` | 工作区管理 |
| Skill | `puzzle` | Skill 管理 |
| 日志 | `file-text` | Log Tab |
| 关于 | `info` | About Tab |
| 附件 | `paperclip` | Chat 输入区 |
| 语音 | `mic` | Chat 输入区 |

### 5.3 品牌吉祥物（大蒜 + 龙虾）

品牌角色用 **PNG**，按主题换色：

#### Matcha v1 大蒜

| 尺寸 | 文件 | 色调 |
|------|------|------|
| 24px | `garlic_24_matcha.png` | 蒜瓣薄荷绿渐变，叶茎 `#3DD68C` |
| 32px | `garlic_32_matcha.png` | 同上 |
| 48px | `garlic_48_matcha.png` | 同上 + 高光 |
| 96px | `garlic_96_matcha.png` | 品牌用，完整细节 |

#### Peachy v2 大蒜

| 尺寸 | 文件 | 色调 |
|------|------|------|
| 24px | `garlic_24_peachy.png` | 蒜瓣暖杏渐变，叶茎 `#FF7F50` |
| 32px | `garlic_32_peachy.png` | 同上 |
| 48px | `garlic_48_peachy.png` | 同上 + 暖色高光 |
| 96px | `garlic_96_peachy.png` | 品牌用，完整细节 |

#### Mochi v3 大蒜

| 尺寸 | 文件 | 色调 |
|------|------|------|
| 24px | `garlic_24_mochi.png` | 蒜瓣奶茶棕渐变，叶茎 `#A67B5B` |
| 32px | `garlic_32_mochi.png` | 同上 |
| 48px | `garlic_48_mochi.png` | 同上 + 暖色高光 |
| 96px | `garlic_96_mochi.png` | 品牌用，完整细节 |

#### 龙虾 AI 头像

| 尺寸 | 文件 | 说明 |
|------|------|------|
| 24px | `lobster_24.png` | 聊天气泡内头像 |
| 32px | `lobster_32.png` | 左侧面板 |
| 48px | `lobster_48.png` | 关于页 |

> 龙虾头像不按主题换色，保持品牌一致性。

### 5.4 托盘图标

大蒜+LED 合体，按主题 + 状态组合：

| 状态 | Matcha LED | Peachy LED |
|------|-----------|------------|
| 空闲 | 白 `#E8ECF4` | 暖灰 `#8B7355` |
| 运行中 | 薄荷绿 `#3DD68C` | 珊瑚橘 `#FF7F50` |
| 异常 | 红 `#FF6B6B` | 暖红 `#FF5C5C` |
| 检测中 | 黄 `#FFBE3D` | 暖橙 `#FFB347` |

尺寸：16/20/32/48/96px（PNG，因托盘 API 需要位图）。

### 5.5 图标层级

> 图标尺寸 = 窗口高 × 比例 / 100。实际像素 = SCALE(int(窗口高 × PCT / 100 + 0.5))。
> 基准：窗口高 800px。

| Token | PCT (/100) | 800px 基准 | 最小值 | 用途 |
|-------|-----------|-----------|--------|------|
| `ICON_TINY` | 2.0% | 16px | 12px | 托盘最小、内联小图标 |
| `ICON_SMALL` | 2.5% | 20px | 14px | 菜单项、列表图标 |
| `ICON_MEDIUM` | 3.0% | 24px | 16px | 按钮内图标、AI 头像 |
| `ICON_LARGE` | 4.0% | 32px | 20px | 用户头像、标题栏、空状态 |
| `ICON_XLARGE` | 6.0% | 48px | 28px | 任务栏、关于页 |
| `ICON_HUGE` | 12.0% | 96px | 48px | 品牌展示 |

> 示例：窗口高 1200px 时 ICON_MEDIUM = 1200 × 3.0% = 36px。

---

# 第二层：控件与状态

## §6 控件库

> 需求 → PRD §3 主界面 / §11 设置
> 控件尺寸 = 父容器 × 百分比，圆角/内边距 = SCALE(base)。SCALE() 仅用于 DPI 缩放视觉细节。

### 6.1 按钮层次体系

按钮分 5 层，从强到弱：**Primary → Secondary → Ghost → Danger → Tiny**。每层清晰传达操作重要性。

#### 主按钮 (Primary / btn_action) — 最高权重

```
  ┌──────────────────────────────┐
  │  [icon]  Get Started         │  ← height=父容器高×10%(min 36), radius=radius_md
  └──────────────────────────────┘                          padding=GAP×2 GAP×3

  Default:  bg=accent, text=text_inverse, font=body_strong
  Hover:    bg=accent_hover, shadow=shadow_md
  Active:   bg=accent_active, scale(0.97)
  Disabled: bg=disabled_bg, text=disabled_text
  Focus:    box-shadow=focus_glow
  图标:     左侧 GAP 间距，icon_small
```

**Matcha:** bg=#3DD68C 圆角 radius_md
**Peachy:** bg=linear-gradient(135deg, #FF7F50, #FFB347) 圆角 radius_lg，hover 放大 shadow_glow

#### 次要按钮 (Secondary) — 中权重

```
  Default:  bg=transparent, border=1px accent, text=accent
  Hover:    bg=accent_subtle
  Active:   bg=accent_subtle, border=accent_hover
  Disabled: border=disabled_text, text=disabled_text
  图标:     左侧 GAP_SMALL×1.5 间距
```

#### 幽灵按钮 (Ghost) — 低权重

```
  Default:  bg=transparent, text=text_secondary, padding=GAP_SMALL GAP×2
  Hover:    bg=hover_overlay, text=text_primary
  Active:   bg=active_overlay
  图标:     左侧 GAP_SMALL 间距，图标颜色跟随文字
```

#### 危险按钮 (Danger) — 破坏操作专用

```
  Default:  bg=danger, text=white, radius=radius_md
  Hover:    bg=danger_hover（加深 10%）
  Active:   scale(0.97)
  图标:     左侧 GAP_SMALL×1.5 间距
```

**Matcha:** danger=#FF6B6B
**Peachy:** danger=#FF5C5C

#### 图标按钮 (Icon Button) — 紧凑操作

```
  ┌────┐
  │ 🔍 │  ← ICON_LARGE (默认) / ICON_MEDIUM (紧凑) / ICON_XLARGE (大)
  └────┘

  Default:  bg=transparent, icon=text_secondary
  Hover:    bg=hover_overlay, icon=text_primary
  Active:   bg=active_overlay
  圆形:     radius_full
  Tooltip:  hover 500ms 显示操作说明
```

#### 发送按钮 — 特殊主按钮

```
  Default:  BTN_SEND 尺寸, 圆形, bg=accent gradient, icon=send (text_inverse)
  Hover:    bg=accent_hover, shadow=shadow_glow
  Active:   bg=accent_active, scale(0.95)
  Sending:  icon=square(停止), 可点击中断流式
  Disabled: bg=disabled_bg, icon=disabled_text
```

**Peachy 特化:** bg=linear-gradient(135deg, #FF7F50, #FF6B8A)，hover 时 glow 色改为 `#FF7F5040`

#### 窗口控制按钮

```
  [settings] [minus] [square] [x]  ← 每个 WC_BTN 尺寸
  Hover x: bg=danger, icon=white
  其他 Hover: bg=hover_overlay
```

### 6.2 按钮组 (Button Group)

多个按钮并排时的排列规则：

```
  ┌────────────┐ ┌────────────┐ ┌────────────┐
  │  取消      │ │  保存      │ │  删除      │
  │  Ghost     │ │  Primary   │ │  Danger    │
  └────────────┘ └────────────┘ └────────────┘
     ← 最弱          ← 最强         ← 破坏
```

| 排列规则 | 说明 |
|---------|------|
| 右对齐 | 操作按钮组右对齐 |
| 间距 | 按钮间 GAP |
| 顺序 | 从弱到强排列（左 Ghost → 右 Primary） |
| 危险按钮 | 始终在最右，与 Primary 间距 GAP×2 |
| 最多 3 个并排 | 超过 3 个改用下拉菜单 |

### 6.3 输入框

#### 单行输入框

```
  Default:   bg=surface, border=1px border, height=LP_ROW_H, radius=radius_md
  Hover:     border=border_strong
  Focus:     border=accent (2px), box-shadow=focus_glow
  Error:     border=danger, 底部显示 error 文字（caption, danger）
  图标前缀:  左侧图标 + GAP padding-left
```

#### 密码输入框（API Key）

同单行输入框 + 右侧 `eye` / `eye-off` 图标切换明文/密码。

#### 多行输入框（Chat/Work）

```
  Default:   bg=panel, border=1px border, min-height=CHAT_INPUT_MIN_H, radius=radius_lg
  Focus:     border=accent (1.5px), box-shadow=focus_glow
  自动增长:  监听行数变化, 调整高度
  工具栏:    左侧 paperclip + mic 图标按钮, 右侧字数 + 发送按钮
```

### 6.4 下拉框 (Dropdown)

```
  Default:   bg=surface, border=1px border, height=LP_ROW_H, radius=radius_md
  Open:      border=accent, chevron-down 旋转 180° (200ms)
  下拉面板:  bg=panel, shadow=shadow_lg, radius=radius_lg
  下拉项:    height=LP_ROW_H, hover=hover_overlay, 左侧图标 + 文字
  选中项:    accent 文字 + 左侧 check 图标
```

### 6.5 开关 (Switch/Toggle)

```
  尺寸: 宽=父容器宽×5%(min 36), 高=宽×56%, 圆角=高/2
  Matcha:
    Off:  bg=#373C55, thumb=#8B92A8
    On:   bg=accent(#3DD68C), thumb=white
  Peachy:
    Off:  bg=#E0D0C0, thumb=#B8A089
    On:   bg=accent(#FF7F50), thumb=white
  动画: 200ms ease-out 滑动 + thumb 弹性回弹
```

### 6.6 复选框 (Checkbox)

```
  尺寸: ICON_SMALL, radius=radius_sm
  Off:   bg=surface, border=1px border_strong
  On:    bg=accent, 内部 check 图标 (ICON_SMALL×60%, white)
  动画:  150ms scale(0.8→1.0) + check 画线动画
```

### 6.7 进度条

```
  背景: surface, 填充: accent gradient, height=GAP_SMALL×1.5, radius=GAP_SMALL×1.5/2
  动画: 填充段平滑过渡 300ms
  不确定: accent 色 shimmer, 1.5s 循环
  百分比: 右侧 small + text_secondary
```

### 6.8 加载指示器

```
  脉冲圆点: 3 个 GAP_SMALL 圆点, accent 色, 依次脉冲 1.2s 循环
  旋转圆环: ICON_TINY, 2px 描边, accent 色, 0.8s 线性循环
  骨架屏:   raised 色 shimmer, 1.5s 循环
```

### 6.9 卡片 (Card)

```
  Default:   bg=panel, radius=radius_lg, padding=GAP×2
  Hover:     bg=raised, shadow=shadow_sm
  Selected:  border-left=GAP_SMALL×0.5 accent
  Peachy:    radius=radius_2xl
```

### 6.10 列表项 (List Item)

```
  Height: LP_ROW_H (紧凑) / LP_ROW_H×1.375 (标准), padding: 0 GAP×2
  Hover:     bg=hover_overlay
  Selected:  bg=accent_subtle, 左侧 GAP_SMALL×0.5 accent 竖线
  Font 标题: body, 副标: small + text_secondary
  图标:     左侧 icon_small, 右侧 chevron-right (可选)
```

### 6.11 Tooltip

```
  圆角: radius_md, padding: GAP_SMALL GAP×2, bg=raised, shadow=shadow_md
  触发: hover 500ms 延迟
  消失: 移开即消失, 200ms fade-out
  最大宽度: 窗口宽×22%(任务详情) / 窗口宽×14%(通用)
  图标:     可选左侧小图标
```

### 6.12 上下文菜单

```
  bg=panel, radius=radius_lg, shadow=shadow_lg
  item height=LP_ROW_H, hover=hover_overlay
  每项: 左侧图标(ICON_TINY) + 文字 + 右侧快捷键
  危险项: text=danger + icon=danger
  分隔线: divider 色 1px
```

### 6.13 Tab 切换

```
  Height=LP_ROW_H×1.5
  Active:   text=accent, 底部 2px accent 下划线
  Inactive: text=text_secondary, hover=text_primary
  动画:     下划线 200ms 滑动
  图标:     可选左侧小图标
```

### 6.14 分隔条 (Splitter)

```
  宽: SPLITTER_W
  Default:   bg=surface
  Hover:     bg=accent, opacity=0.3, cursor=col-resize
  Dragging:  bg=accent, opacity=0.6
```

### 6.15 步骤指示器

```
  ●──●──○──○──○──○  Step 1/6
  已完成: ● accent ICON_SMALL×50% + check 图标(ICON_TINY×50%)
  当前:   ● accent ICON_SMALL×60% + glow + pulse
  未完成: ○ border_strong ICON_SMALL×50%
  连接线: 2px, 已完成段=accent, 未完成段=border
```

### 6.16 状态胶囊 (Status Badge)

```
  height=LP_ROW_H×75%, radius=height/2, padding: 0 GAP, font=caption + bold
  图标(ICON_TINY×60%) + 文字
  Ready:    icon=check-circle, bg=accent_subtle, text=accent
  Busy:     icon=loader-2(spin), bg=#FFBE3D1A, text=warning
  Error:    icon=x-circle, bg=#FF6B6B1A, text=danger
  Checking: icon=refresh-cw(spin), bg=#6C9FFF1A, text=info
```

---

## §7 状态系统

> 需求 → PRD §12 技术架构

### 7.1 控件状态定义

| 状态 | 触发 | 含义 |
|------|------|------|
| `default` | 初始 | 控件静止 |
| `hover` | 鼠标悬停 | 可交互暗示 |
| `focus` | Tab/点击聚焦 | 键盘可达 |
| `active` | 鼠标按下 | 正在交互 |
| `disabled` | 条件不满足 | 不可交互 |
| `loading` | 异步操作中 | 暂不可用 |

### 7.2 全局状态样式 Token

| Token | 值 | 状态 |
|-------|-----|------|
| `state_hover_bg` | `#FFFFFF0A` | hover |
| `state_active_bg` | `#FFFFFF14` | active |
| `state_focus_ring` | `0 0 0 2px accent 30%` | focus |
| `state_disabled_opacity` | `0.4` | disabled |

### 7.3 控件 × 状态矩阵

#### 按钮

| 状态 | 背景 | 文字 | 边框 | 其他 |
|------|------|------|------|------|
| default | accent | text_inverse | none | shadow_sm |
| hover | accent_hover | text_inverse | none | shadow_md |
| focus | accent | text_inverse | 2px focus_ring | — |
| active | accent_active | text_inverse | none | scale(0.97) |
| disabled | disabled_bg | disabled_text | none | opacity 0.4 |
| loading | accent | text_inverse | none | spinner 叠加 |

#### 输入框

| 状态 | 背景 | 文字 | 边框 |
|------|------|------|------|
| default | surface | text_primary | 1px border |
| hover | surface | text_primary | 1px border_strong |
| focus | surface | text_primary | 2px accent + glow |
| disabled | disabled_bg | disabled_text | 1px border |

### 7.4 加载态与骨架屏

#### 消息气泡骨架

```
  ┌──────────────────────────────────────────┐
  │  ┌──┐  ████████████████                  │  ← bg=raised, shimmer 1.5s
  │  │  │  ████████████████████████████      │
  │  └──┘  ██████████████                    │
  └──────────────────────────────────────────┘
```

#### 全屏加载

```
  ◜  ICON_MEDIUM spinner, accent 色, 0.8s 旋转
  "正在连接 Agent..."
  遮罩: 半透明 bg 色
```

#### 按钮内加载

```
  ◜ Saving...  ← spinner ICON_SMALL + 文字
  完成 → 显示 ✓ 1.5s → 恢复原始文字
```

### 7.5 错误态

#### 输入框错误

```
  border: 2px danger
  ⚠ 错误文案（caption + danger, GAP_SMALL 上边距）
  修正后: 200ms border 渐变回 accent
```

#### 区域错误条

```
  bg: #FF6B6B10, border-left: GAP_SMALL×0.75 danger, height=LP_ROW_H
  内容: 图标 + 错误描述 + 操作按钮
```

#### 全局错误弹窗

```
  ❌ 连接失败
  无法连接到 Agent (localhost:18789)
  可能原因: ... 
  [ 重试连接 ] [ 查看日志 ] [ 忽略 ]
```

### 7.6 空状态

| 位置 | 图标 (SVG) | 文案 | CTA |
|------|-----------|------|-----|
| Chat 空 | `garlic_48` + `lobster_32` 并排 | "有什么我能帮你？" | 快捷建议气泡 |
| Work 空 | `briefcase` (ICON_LARGE, accent) | "AI 工作台已就绪 — 在 Chat 中发送任务" | 查看示例 |
| Session 空 | `layers` (ICON_LARGE, text_tertiary) | "暂无活跃会话" | — |
| Cron 空 | `clock` (ICON_LARGE, text_tertiary) | "暂无定时任务 — 点击 + 创建" | [+按钮] |
| Skill 空 | `puzzle` (ICON_LARGE, text_tertiary) | "暂无 Skill — 从 ClawHub 浏览安装" | 浏览按钮 |
| 搜索无结果 | `search` (ICON_LARGE, text_tertiary) | "未找到匹配结果" | 修改搜索词 |
| Log 空 | `file-text` (ICON_LARGE, text_tertiary) | "暂无日志记录" | — |

**通用样式：** SVG 图标 ICON_LARGE + text_tertiary, 居中; 文案 body + text_tertiary, 居中; 最大宽度 窗口宽×19%。

**Peachy 特化：** 空状态背景添加半透明装饰圆点浮动动画（见 §8.5）。

---

## §8 动效系统

> 需求 → PRD §3 主界面

### 8.1 动画原则

| 原则 | 说明 |
|------|------|
| 有意义 | 每个动画传达状态变化 |
| 快速 | 大部分 150-300ms |
| 自然 | ease-out(进入) / ease-in(退出) / spring(弹性) |
| 一致 | 同类交互用同一时长和曲线 |
| 可跳过 | 尊重 `prefers-reduced-motion` |

### 8.2 持续时间 Token

| Token | 值 | 用途 |
|-------|-----|------|
| `dur_instant` | 100ms | 微交互（hover 高亮） |
| `dur_fast` | 150ms | 按钮反馈、checkbox 切换 |
| `dur_normal` | 200ms | 大部分过渡 |
| `dur_slow` | 300ms | 弹窗、面板展开、主题切换 |
| `dur_very_slow` | 500ms | 向导步骤切换 |

### 8.3 缓动曲线 Token

| Token | 值 | 用途 |
|-------|-----|------|
| `ease_out` | `cubic-bezier(0.0, 0.0, 0.2, 1)` | 元素进入 |
| `ease_in` | `cubic-bezier(0.4, 0.0, 1.0, 1)` | 元素退出 |
| `ease_in_out` | `cubic-bezier(0.4, 0.0, 0.2, 1)` | 位置/大小变化 |
| `spring` | `cubic-bezier(0.34, 1.56, 0.64, 1)` | 弹性回弹 |
| `bounce` | `cubic-bezier(0.34, 1.8, 0.64, 1)` | Peachy 专用弹性 |

### 8.4 动画清单

#### 消息相关

| 动画 | 效果 | 时长 | 曲线 |
|------|------|------|------|
| 消息出现 | GAP_SMALL 上移 + fade-in | 200ms | ease_out |
| 流式打字 | 光标 `\|` 脉冲 | 500ms 循环 | linear |
| 流式追加 | 每 50ms 追加 3 字符 | 50ms | linear |
| 打字指示器 | 3 圆点依次脉冲 | 1.2s 循环 | linear |
| 复制反馈 | check 图标 scale(0→1) → 1.5s fade-out | 200ms | spring |

#### 按钮 & 控件

| 动画 | 效果 | 时长 | 曲线 |
|------|------|------|------|
| 按钮 hover | bg 渐变 + shadow 增强 | 150ms | ease_out |
| 按钮 active | scale(0.97) | 150ms | ease_in |
| 按钮 release | scale(1.0) 回弹 | 150ms | spring |
| Switch 切换 | 滑块滑动 + 颜色渐变 | 200ms | spring |
| Checkbox 勾选 | scale(0.8→1.0) + check 画线 | 150ms | spring |
| Dropdown 打开 | chevron 旋转 180° | 200ms | ease_out |
| 进度条填充 | 宽度平滑过渡 | 300ms | ease_in_out |
| 图标切换 | cross-fade（如发送→停止） | 150ms | ease_in_out |

#### 面板 & 布局

| 动画 | 效果 | 时长 | 曲线 |
|------|------|------|------|
| 模式切换 | cross-fade + 指示器滑动 | 200ms | ease_in_out |
| 面板折叠/展开 | 宽度 LEFT_PANEL↔NAV_W | 300ms | ease_in_out |
| 最大化/还原 | 窗口尺寸过渡 | 300ms | ease_in_out |
| Session 列表增减 | GAP×3 上移/下移 + fade | 200ms | ease_out |

#### 弹窗 & 浮层

| 动画 | 效果 | 时长 | 曲线 |
|------|------|------|------|
| 弹窗出现 | 95% 缩放 + fade-in | 300ms | ease_out |
| 弹窗关闭 | 95% 缩放 + fade-out | 200ms | ease_in |
| 遮罩出现 | opacity 0→60%（Matcha）/ 0→30%（Peachy） | 300ms | ease_out |
| Dropdown 展开 | fade-in + GAP_SMALL 下移 | 200ms | ease_out |
| 权限弹窗出现 | scale(0.9→1.0) + fade-in | 300ms | spring |

#### Toast & 通知

| 动画 | 效果 | 时长 | 曲线 |
|------|------|------|------|
| Toast 进入 | 右下 slide-in + fade-in | 200ms | ease_out |
| Toast 消失 | fade-out | 200ms | ease_in |
| Toast 堆叠 | 已有 Toast 向上推 44px | 200ms | ease_in_out |

#### 向导 & 系统

| 动画 | 效果 | 时长 | 曲线 |
|------|------|------|------|
| 向导步骤切换 | 内容左/右滑动 + 连接线延伸 | 500ms | ease_in_out |
| 窗口最小化到托盘 | 缩小 + 透明度 → 飞向托盘 | 300ms | ease_in |
| 主题切换 | 全局色值过渡 | 300ms | ease_in_out |
| 大蒜头摇摆 | 正弦波 ±12°, 1.5Hz | 连续 | linear |

### 8.5 主题特化动画

#### Matcha v1 特效

| 特效 | 描述 | 场景 |
|------|------|------|
| 薄荷呼吸光 | accent 色 box-shadow 脉冲，3s 循环 | Agent Ready 状态卡片 |
| 绿色粒子 | 6px 圆点从按钮散开 300ms 消失 | 主按钮成功点击反馈 |
| 代码高亮 | 等宽字淡入 + 背景色渐变 | 代码块首次渲染 |

#### Peachy v2 特效

| 特效 | 描述 | 场景 |
|------|------|------|
| 暖色呼吸光 | accent 色 box-shadow 脉冲，2.5s 循环 | Agent Ready 状态卡片 |
| 桃心弹跳 | heart 图标 scale(0→1.2→1) 弹性 | 操作成功反馈 |
| 背景装饰圆点 | 半透明圆点缓慢浮动，8s 循环 | 向导背景、空状态 |
| 按钮光泽 | hover 时 45° 光带扫过按钮表面 | 主按钮 hover |
| 弹性回弹 | 弹窗、下拉出现时放大到 102% 再回弹 | Peachy 场景下的所有弹窗 |

### 8.6 Reduced Motion

系统开启 `prefers-reduced-motion` 时：所有动画时长降为 0ms（即时切换），保留功能性动画（进度条、加载指示器）。

---

## §9 音效系统

> 需求 → PRD §3 主界面

### 9.1 音效原则

| 原则 | 说明 |
|------|------|
| 不打扰 | 默认音量低（20-40%），不刺耳 |
| 有反馈 | 每个关键操作都有对应的声学反馈 |
| 跟主题 | Matcha 清脆，Peachy 温暖 |
| 可关闭 | 全局开关 + 音量滑块（0~100%） |

### 9.2 音效清单（通用）

| 音效 ID | 触发 | 时长 | 默认音量 |
|---------|------|------|---------|
| `msg_incoming` | AI 回复完成 | ~200ms | 40% |
| `msg_sent` | 用户消息发送 | ~100ms | 20% |
| `alert_critical` | 警报弹窗 | ~500ms | 60% |
| `alert_error` | 操作失败 | ~300ms | 50% |
| `success` | Agent 启动/任务完成 | ~200ms | 30% |
| `permission_request` | 权限确认弹窗 | ~300ms | 40% |
| `wizard_complete` | 向导完成 | ~600ms | 50% |
| `copy` | 复制到剪贴板 | ~50ms | 15% |
| `click` | 按钮点击 | ~30ms | 10% |
| `switch` | 开关切换 | ~80ms | 15% |
| `dropdown_open` | 下拉展开 | ~100ms | 15% |
| `toast` | Toast 通知出现 | ~150ms | 25% |

### 9.3 Matcha v1 音效风格

**清脆、通透、薄荷感。** 高频为主，带轻微混响。

| 音效 ID | 音色描述 |
|---------|---------|
| `msg_incoming` | 水滴落玉盘，C5→E5 上行 |
| `msg_sent` | 轻微气泡声，短促 |
| `success` | 清脆上扬双音，C5→G5 |
| `error` | 低沉警示，A4 短促下降 |
| `permission_request` | 双音敲击，C5-C5 |
| `wizard_complete` | 三音上行旋律，C5-E5-G5 |
| `click` | 微型水滴声 |
| `switch` | 轻快拨动声 |

### 9.4 Peachy v2 音效风格

**温暖、柔和、圆润感。** 中频为主，带轻微颤音。

| 音效 ID | 音色描述 |
|---------|---------|
| `msg_incoming` | 木琴轻敲，E4→G4 上行 |
| `msg_sent` | 轻柔气声，带暖色 |
| `success` | 温暖上扬双音，E4→B4 |
| `error` | 柔和低音警示，D4 短促下降 |
| `permission_request` | 双音木琴，E4-E4 |
| `wizard_complete` | 三音上行旋律，E4-G4-B4 + 颤音 |
| `click` | 软木碰撞声 |
| `switch` | 圆润拨动声 |

### 9.5 实现方案

| 方案 | 说明 |
|------|------|
| 音频格式 | WAV（内嵌资源），Ogg Vorbis（外部文件备选） |
| 播放 API | Windows: `PlaySound()` / SDL2 Audio |
| 资源位置 | `assets/sounds/matcha/` 和 `assets/sounds/peachy/` |
| 音效文件 | 按主题分目录，文件名统一（如 `msg_incoming.wav`） |
| 主题切换 | 指向不同目录，即时生效 |
| 全局开关 | config.json `sound_enabled: true` |
| 音量控制 | config.json `sound_volume: 40`（0~100） |
| 备选 | 系统音效降级：`MB_ICONWARNING`, `MB_ICONERROR`, `MB_OK` |

---

## §10 无障碍 · 性能 · 离线降级

> 需求 → PRD §12 技术架构

### 10.1 无障碍

#### 对比度（WCAG 2.1 AA）

| 组合 | 前景 | 背景 | 对比度 | 达标? |
|------|------|------|--------|------|
| 正文 | `#E8ECF4` | `#0F1117` | 12.8:1 | ✅ AAA |
| 次要文字 | `#8B92A8` | `#0F1117` | 4.9:1 | ✅ AA |
| 按钮文字 | `#0F1117` | `#3DD68C` | 8.1:1 | ✅ AAA |
| 链接 | `#3DD68C` | `#0F1117` | 7.2:1 | ✅ AAA |

#### 焦点管理

- 所有可交互元素 focus 态有 2px focus_ring
- 弹窗打开时焦点锁定在弹窗内
- 弹窗关闭后焦点回到触发元素

#### 键盘导航

| 键 | 行为 |
|----|------|
| Tab / Shift+Tab | 下一个/上一个可聚焦元素 |
| Enter / Space | 激活按钮/开关/复选框 |
| Esc | 关闭弹窗/下拉/菜单 |
| ↑↓ | Dropdown 选项导航 |

### 10.2 性能预算

| 指标 | 目标 |
|------|------|
| 帧率 | ≥ 30fps 稳定, 60fps 空闲 |
| 窗口启动到可交互 | < 2s |
| 模式切换 | < 150ms |
| 弹窗打开 | < 100ms |
| 空闲内存 | < 200 MB |
| Chat 活跃 | < 300 MB |
| Work 活跃 | < 350 MB |

### 10.3 DPI 适配

```
启动时: SetProcessDpiAwarenessContext(PER_MONITOR_AWARE_V2)
dpi_scale = GetDpiForWindow(hwnd) / 96.0f
SCALE(base) = (int)(base * dpi_scale + 0.5f)
```

> PCT 值计算出的像素再乘以 dpi_scale = 最终渲染尺寸。

| PCT 计算值 | 100% (96 DPI) | 125% (120 DPI) | 150% (144 DPI) |
|-----------|---------------|----------------|----------------|
| 标题栏 (6%) | 48 | 60 | 72 |
| 左面板 (可用×25%) | 240 | 300 | 360 |
| body 字号 (1.63%) | 13 | 16 | 19 |

### 10.4 滚动条样式

```
轨道: GAP_SMALL, bg=transparent
滑块: GAP_SMALL, bg=#FFFFFF15, radius=GAP_SMALL/2
hover: bg=#FFFFFF30
active: bg=accent
```

### 10.5 光标类型

| 场景 | 光标 |
|------|------|
| 默认 | `default`（箭头） |
| 可点击 | `pointer`（手型） |
| 输入 | `text`（I-beam） |
| 拖拽 | `move`（四向箭头） |
| 调整大小 | `col-resize` |
| 禁用 | `not-allowed` |
| 等待 | `wait` |

### 10.6 离线降级

> 需求 → PRD §12.6 错误恢复

| 故障 | UI 表现 |
|------|---------|
| Agent 断开 | 发送按钮禁用 + 提示"Agent 未连接" + [启动 Agent]按钮 |
| LLM 超时 | "⚠️ AI 回复超时，请重试。" |
| LLM 429 | 区域错误条 + Retry-After 倒计时 + [重试] / [切换模型] |
| 网络断开 | 左面板变红 + 聊天区提示 + [重试连接] |

**保持可用：** 设置页、主题/语言切换、License 查看、Log、About、Boot Check

---

# 第三层：界面设计

## §11 首次体验

> 需求 → PRD §2 安装与首次启动

### UI-01: 启动自检

```
┌────────────────────────────────────────┐
│           🧄 AnyClaw LVGL              │
│  ┌──────────────────────────────────┐  │
│  │  ● Node.js  v22.14.0      ✓     │  │
│  │  ● npm      v10.8.0       ✓     │  │
│  │  ● Network  openrouter.ai  ✓     │  │
│  │  ● OpenClaw v2.1.0        ✓     │  │
│  └──────────────────────────────────┘  │
│           [ 启动中... ]                │
└────────────────────────────────────────┘
```

每项检测中 spinner → 绿色 ✓ / 红色 ✗。全部通过 → 自动进入主界面。

### UI-02: 已运行提示

Win32 原生 MessageBox："已有一个实例正在运行。请勿重复启动。"

### UI-03: 启动错误阻断

LVGL 模态弹窗，阻断启动。如"未检测到 Node.js，请安装后重新启动。"

### UI-04 ~ UI-09: 首次启动向导（7步）

**布局：** 全窗口（与主窗口同尺寸 1450×800），非模态弹窗。
**结构：** 标题栏(TITLE_H) + 横向步骤条 + 内容区(剩余高度) + 底部按钮栏

```
┌──────────────────────────────────────────────────────────────────────────────┐
│ 🧄 AnyClaw Setup                                            [—] [✕]          │
├──────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ● ─────── ● ─────── ● ─────── ○ ─────── ○ ─────── ○ ─────── ○            │
│  语言      Agent    模型      本地模型  IM       Leader    个人信息           │
│                                                                              │
├──────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ┌────────────────────────────────────────────────────────────────────────┐  │
│  │                      当前步骤内容区 (全宽 × 剩余高度)                     │  │
│  └────────────────────────────────────────────────────────────────────────┘  │
│                                                                              │
├──────────────────────────────────────────────────────────────────────────────┤
│  [ 退出 ]                                            [ Next Step > ▶ ]        │
└──────────────────────────────────────────────────────────────────────────────┘
```

**步骤条状态：**

| 状态 | 圆点 | 连接线 | 可点击 |
|------|------|--------|--------|
| ✅ 已完成 | ● accent 实心圆 | accent 色 | ✅ 可回看修改 |
| ▶ 当前步 | ● accent 实心圆 + glow pulse | — | ✅ |
| ○ 未到 | ○ border_strong 空心圆 | border 弱色 | ❌ 不可交互 |

**步骤指示器控件（§6.15）：**
```
  ●──●──○──○──○──○──○  Step 1/7
  已完成: ● accent ICON_SMALL×50% + check 图标(ICON_TINY×50%)
  当前:   ● accent ICON_SMALL×60% + glow + pulse
  未完成: ○ border_strong ICON_SMALL×50%
  连接线: 1px, 已完成段=accent, 未完成段=border
```

**步骤切换：** 内容区 cross-fade 200ms，步骤条圆点 transition 200ms。

| Step | 内容 |
|------|------|
| UI-04 | 语言选择 — 中文/English 两个按钮 窗口宽×10% × LP_ROW_H×1.5 |
| UI-05 | Agent 安装 — OpenClaw 必须 / Hermes 可选 / Claude Code 可选 + 安装按钮组 |
| UI-06 | 模型 & API Key — 模型 Dropdown + Provider + API Key 密码输入 + "+" 添加自定义模型 |
| UI-07 | 本地模型 — Enable Switch + E4B/26B Checkbox + Skip |
| UI-08 | 连接 IM — Telegram/Discord/WhatsApp 渠道卡片 + 配置/Skip |
| UI-09 | Leader 模式确认 — 默认开启并行三 Agent / 切换为单 Agent 模式 |
| UI-10 | 个人信息 & 确认 — 昵称/时区 + 配置摘要 + "Get Started" 主按钮 |

### UI-05 详细: 环境检测 & 安装（嵌入终端）

```
  ┌──────────────────────────────────────────────────────────────────┐  │
  │                                                                  │  │
  │              🔧  环境检测 & 安装                                 │  │
  │                                                                  │  │
  │  ┌─────────────────────────────┐  ┌─────────────────────────┐   │  │
  │  │  检测结果 (左半)             │  │  📺 安装日志 (右半)      │   │  │
  │  │                             │  │                         │   │  │
  │  │  ● Node.js v22.14.0  ✅    │  │  > 检测 Node.js... ✓    │   │  │
  │  │  ● npm v10.8.0       ✅    │  │  > 检测 npm... ✓        │   │  │
  │  │  ● OpenClaw          ✅    │  │  > 检测网络... ✓        │   │  │
  │  │  ● 网络连通          ✅    │  │  > 全部就绪             │   │  │
  │  │                             │  │  _                      │   │  │
  │  └─────────────────────────────┘  └─────────────────────────┘   │  │
  │                                                                  │  │
  │         [ 在线安装（推荐） ]  [ bundled/ 离线安装 ]                    │  │
  │         扫描结果自动刷新 · 找不到？手动指定路径 >                      │  │
  │                                                                  │  │
  └──────────────────────────────────────────────────────────────────┘  │
```

**终端组件：** monospace 字体(JetBrains Mono) + surface 色背景 + 自动滚动到底部 + 可选中复制。
**CLI 日志重定向：** 所有 CLI 子进程 stdout/stderr 同时写入终端组件 + `logs/app.log`。
**安装按钮：**
- 在线安装（推荐）: 在线下载 Node.js + npm install -g openclaw
- bundled/ 离线安装: 解压 node-win-x64.zip + npm install bundled/openclaw.tgz
- 手动指定路径: 文字链，兜底入口，弹出文件选择器指定 node.exe 和 openclaw 路径

**自动扫描逻辑（检测结果自动刷新）：**
1. PATH 中查找 `node` 和 `openclaw` 命令
2. npm 全局目录查找
3. 常见安装位置扫描（Program Files、AppData 等）
4. bundled/ 目录检测
5. 扫描完成 → 更新 LED 状态，找到就标 ✅

**检测结果样式：**

| 状态 | 图标 | 文案 |
|------|------|------|
| ✅ 已安装 | 绿色 check-circle | 版本号 + "已安装" |
| ❌ 未安装 | 红色 x-circle | "未安装" |
| ⚠️ 待检测 | 黄色 alert-circle | "待检测" / "需要安装" |

### UI-10: 工作区模板选择

全屏弹窗，4 个模板卡片（通用助手/开发者/写作者/数据分析），选中后高亮 + 路径输入 + 确认创建。

---

## §12 窗口系统

> 需求 → PRD §3 主界面

### 主界面 v2 布局总览

三区域结构：左导航栏(NAV_W) + 标题栏(TITLE_H) + 内容区(剩余空间)

**布局比例基准：**
- 主窗体基准 = 屏幕分辨率（物理像素）
- 所有子控件宽高以父控件比例为基准描述
- 父控件尺寸变化时，子控件按比例响应

**内边距规范（必须遵守）：**
- 控件与控件之间：GAP_MEDIUM
- 控件与父容器边缘：PAD_NORMAL
- 文字与控件边缘：TEXT_PAD（文字左/右至少 TEXT_PAD，上/下至少文本行高之一半）
- 气泡与容器边缘：PAD_NORMAL
- 代码块与容器边缘：PAD_NORMAL
- 所有容器内部必须有 padding，禁止控件贴边

```
┌────┬──────────────────────────────────────────────────────────────────┐
│ 🧄 │ 🧄 Bot · Chat                                         [✕]       │ 标题栏 TITLE_H
├────┼──────────────────────────────────────────────────────────────────┤
│    │                                                                  │
│ 🦞 │  ← Bot                                                          │
│ ⚡ │  ← 任务    内容区（跟随左导航切换模块）                            │
│ 📁 │  ← 资源                                                          │
│    │                                                                  │
│ [↔]│  ← Chat/Work 切换（左 NAV 底部，上一行）                         │
│ ⚙ │  ← 设置（左 NAV 底部，下一行，上下垂直排列）                       │
└────┴──────────────────────────────────────────────────────────────────┘
←56→
```

### 左导航栏 (56px)

icon-only，hover 300ms 显示 tooltip。

**顶部：** 品牌 Logo（ICON_LARGE，居中）
**中部（一级导航）：**

|| Icon | 模块 | 含义 |
|------|------|------|
| 🦞 | Bot | AI 对话入口（Chat + Work） |
| ⚡ | 任务 | 任务调度中心（活跃/定时/历史） |
| 📁 | 资源 | 文件 + 知识库 + Skills |

**底部（快捷入口）：**

|| Icon | 功能 | 说明 |
|------|------|------|
| [↔] | Chat/Work | 单按钮图标切换模式，位于底部 |
| ⚙ | 设置 | 点击覆盖内容区显示设置 |

**状态样式：**

|| 状态 | 样式 |
|------|------|
| 激活 | icon=accent 色 + 左侧 GAP_SMALL×0.75 accent 竖线 + accent_subtle 背景 |
| hover | bg=hover_overlay + tooltip |
| 默认 | icon=text_secondary |

### 标题栏 (48px)

```
│ 🧄 Bot · Chat                                         [✕] │
  ↑品牌 ↑模块名                                              ↑操作
```

||| 元素 | 说明 |
|------|------|
| 🧄 | 品牌 Logo，固定 |
| Bot · Chat | 当前模块名 + 子状态（随左导航切换） |
| ✕ | 关闭到托盘 |

**Chat/Work 切换已移至左导航栏底部，不再位于标题栏。**

**标题栏随模块变化：**

|| 左导航 | 标题栏显示 | 额外控件 |
|--------|-----------|---------|
| 🦞 Bot | `Bot · Chat` / `Bot · Work` | — |
| ⚡ 任务 | `任务` | [+] 新建任务 |
| 📁 资源 | `资源` | — |

### UI-11: Bot · Chat 模式

```
┌────┬─────────────────────────────────────────────────────────────────┐
│ 🧄 │ 🧄 Bot · Chat                                        [✕]      │
├────┼─────────────────────────────────────────────────────────────────┤
│    │                                                                 │
│ 🦞*│                                                                 │
│    │   🔵  帮我查一下今天的天气                         12:28       │ ← 对话
│    │                                                                 │
│    │   🦞  今天天气晴，26°C。待办 3 项未完成。          12:28       │ ← 对话
│    │                                                                 │
│    │   ┌─ AI 思考 ───────────────────────────────────────┐          │ ← 思考（折叠）
│    │   │ 用户查询天气，需要调用搜索工具获取数据...         │          │
│    │   └──────────────────────────────────────────────────┘          │
│    │   ┌─ terminal ────────────────────────────────────────┐        │ ← 文件输出
│    │   │ $ curl weather-api...                            │        │
│    │   │ {"temp":26,"condition":"sunny"}                   │        │
│    │   └───────────────────────────────────────────────────┘          │
│    ├─────────────────────────────────────────────────────────────────┤
│    │  [Agent ▼]  [Report] [AI行为 ▼]                         │ ← 控制行
│    ├─────────────────────────────────────────────────────────────────┤
│    │  [📎] │ 输入消息...                                 [▶]       │ ← 输入区
│    │                                                                 │
│    │  ← 左侧: Session/Cron 列表（固定宽度）                       │
└────┴─────────────────────────────────────────────────────────────────┘
```

**右侧区域结构：** 输出区 → 控制行 → 输入区
**输出区内容：** 对话气泡 / AI思考（折叠） / 文件输出（代码/终端）
**左侧面板：** Session 列表 + Cron 列表，自动刷新（2s 间隔），固定宽度

#### 输出区三种内容视觉规范

**1. 对话气泡（AI/用户）**
- AI 消息靠左，用户消息靠右，像 QQ 那样左右分明
- 气泡背景：AI 侧 `surface` 色，用户侧 `accent_subtle` 色
- 气泡内文字：`body` 字体
- 气泡与容器边缘：`PAD_NORMAL`
- 气泡之间间距：`GAP_MEDIUM`

**2. AI 思考块（折叠）**
- 位于对话气泡下方，紧凑排列
- 折叠时显示标题 `[AI 思考]` + 展开箭头
- 展开时内容缩进 + 背景 `surface` 色
- 标题字体：`caption`，颜色 `text_secondary`
- 折叠块与上下内容间距：`GAP_SMALL`

**3. 文件输出块（终端/代码）**
- 位于 AI 思考块下方，或独立区域
- 顶部标签栏：显示语言类型（`terminal` / `python` / `javascript` 等）
- 标签字体：`caption`，背景 `surface_subtle`
- 代码/终端背景：`surface` 色，`radius_md` 圆角
- 内容字体：`code` 字体（JetBrains Mono 等宽）
- 代码块与容器边缘：`PAD_NORMAL`
- 与上方内容间距：`GAP_SMALL`

### UI-12: Bot · Work 模式

```
┌────┬─────────────────────────────────────────────────────────────────┐
│ 🧄 │ 🧄 Bot · Work                                       ●  [✕]     │
├────┼─────────────────────────────────────────────────────────────────┤
│    │                                                                 │
│ 🦞*│                                                                 │
│    │   ┌─ 步骤流 ─────────────────────────────────────────┐           │
│    │   │ 📄 读取 config.json              ✓               │           │
│    │   │ ✏️ 写入 main.cpp              ✓               │           │
│    │   │ ⚡ 执行 npm install              ● (进行中)      │           │
│    │   └─────────────────────────────────────────────────┘           │
│    │   🔵 帮我完成这个任务                             12:28         │ ← 对话
│    │                                                                 │
│    │   ┌─ terminal ─────────────────────────────────────────┐        │ ← 终端输出
│    │   │ $ npm install                                      │        │
│    │   │ added 234 packages in 12s                          │        │
│    │   └────────────────────────────────────────────────────┘        │
│    ├─────────────────────────────────────────────────────────────────┤
│    │  [Agent ▼]  [Report] [AI行为 ▼]                         │ ← 控制行
│    ├─────────────────────────────────────────────────────────────────┤
│    │  [📎] │ 输入消息...                                 [▶]       │ ← 输入区
│    │                                                                 │
│    │  ← 左侧: StepCard 步骤流（固定宽度）                        │
└────┴─────────────────────────────────────────────────────────────────┘
```

**右侧区域结构：** 输出区 → 控制行 → 输入区（与 Chat 完全相同）
**输出区内容：** 步骤流 / 对话 / AI思考 / 文件输出（代码/终端）

**与 Chat 的区别：** 左侧面板内容不同（Chat=Session/Cron，Work=StepCard）

**StepCard 状态：** ● 进行中(绿 spinner) / ✓ 完成(绿) / ✗ 失败(红) / ? 等待确认(蓝)
**写操作按钮：** [接受 ✓] [拒绝 ✗] [回退 ↩]

### UI-13: 贴边大蒜头（桌面宠物）

窗口拖到屏幕边缘松手 → 隐藏主窗口 → 屏幕边缘显示浮动大蒜头。纯 Win32 实现。

#### 基础参数

| 参数 | 值 |
|------|-----|
| 尺寸 | ICON_HUGE×83% × ICON_HUGE（body ICON_HUGE×67% + sprout ICON_HUGE×75%） |
| 窗口属性 | WS_EX_LAYERED + WS_EX_TOPMOST + WS_EX_TOOLWINDOW |
| 透明方式 | COLORKEY（品红 #FF00FF 为透明色） |
| 帧率 | 30fps（33ms timer） |

#### 主题变体

| 主题 | body 贴图 | sprout 贴图 | 整体色调 |
|------|----------|------------|---------|
| Matcha | `garlic_body_matcha.png` | `garlic_sprout_matcha.png` | 薄荷绿叶茎 + 白蒜瓣 |
| Peachy | `garlic_body_peachy.png` | `garlic_sprout_peachy.png` | 橘粉叶茎 + 暖杏蒜瓣 |
| Mochi | `garlic_body_mochi.png` | `garlic_sprout_mochi.png` | 奶茶棕叶茎 + 米白蒜瓣 |

#### 动画状态机

```
[空闲待机] ──鼠标进入──→ [悬停摇摆] ──鼠标离开──→ [空闲待机]
     │                        │
     │────定时触发────→ [随机动作] ──完成──→ [空闲待机]
     │                        │
     └────点击────────→ [恢复窗口]
```

| 状态 | 动画 | 触发 | 时长 |
|------|------|------|------|
| 空闲待机 | sprout 微呼吸（±2° 缓慢摆动，0.5Hz） | 默认 | 持续 |
| 悬停摇摆 | sprout ±12° 正弦波，1.5Hz | 鼠标进入 | 持续到离开 |
| 随机动作 | 见下方特效表 | 空闲 30-120s 随机 | 1-3s |
| 恢复窗口 | 缩小 + fade-out → 飞向主窗口位置 | 点击 | 300ms |

#### 随机特效（空闲时触发）

| 特效 | 描述 | Matcha | Peachy | Mochi | 概率 |
|------|------|--------|--------|-------|------|
| 喷火 | 口部喷出 3-5 个粒子向上飘散消失 | 绿色粒子 | 橙色粒子 | 茶色粒子 | 20% |
| 跳跃 | body 整体上跳 GAP×2.5 + 弹性落地 | 同 | 同 | 同 | 20% |
| 摇头 | body 左右摇头 ±8° | 同 | 同 | 同 | 15% |
| 闪光 | body 短暂高亮（opacity 脉冲） | 绿色光晕 | 橘色光晕 | 棕色光晕 | 15% |
| 冒泡 | 头顶冒出 2-3 个透明圆泡上升消失 | 同 | 同 | 同 | 15% |
| 打盹 | sprout 缓慢下垂 → 回弹 | 同 | 同 | 同 | 15% |

#### 粒子参数（喷火特效）

```
粒子数量: 3-5 个
粒子大小: GAP_SMALL~GAP×2 圆形
起始位置: sprout 顶部
方向: 向上 + 随机偏移 ±15°
速度: 窗口宽×3%~6%/s
生命周期: 500-1000ms
淡出: alpha 1→0
颜色:
  Matcha: #3DD68C → #2BB673 (渐变)
  Peachy: #FF7F50 → #FFB347 (渐变)
  Mochi:  #A67B5B → #C9A96E (渐变)
```

### UI-52 ~ UI-56: 五套主题

**主打主题：**

| 主题 | bg | panel | text | accent | 风格 |
|------|----|-------|------|--------|------|
| 🍵 Matcha v1 (默认) | #0F1117 | #1A1D27 | #E8ECF4 | #3DD68C | 薄荷绿深色 |
| 🍑 Peachy v2 | #FFF8F3 | #FFFFFF | #2D1B14 | #FF7F50 | 珊瑚橘暖色 |

**备选主题：**

| 主题 | bg | panel | text | accent | 风格 |
|------|----|-------|------|--------|------|
| 经典暗色 | #1E1E1E | #2D2D30 | #CCCCCC | #007ACC | VS Code 风 |
| 亮色 | #F5F7FA | #FFFFFF | #1A1D27 | #2BB673 | 日间高亮 |
| 怀旧暗色 | #2D2D2D | #3C3C3C | #FFFFFF | #82AAF0 | 极简深灰 |

主题切换：300ms 全局色值过渡，不硬切。Peachy 圆角放大（radius +radius_lg）。

### UI-55: 通用控件体系

控件尺寸常量集中在 `app_config.h`，所有值乘以 `dpi_scale`。弹窗系统：`create_dialog()` 创建居中弹窗，支持拖拽移动。

### UI-63: 系统托盘菜单

```
┌─────────────────────────────┐
│  🟢 Agent: Ready          │
├─────────────────────────────┤
│  Start Agent              │
│  Stop Agent               │
│  Restart OpenClaw           │
│  Refresh Status             │
├─────────────────────────────┤
│  ☑ Boot Start               │
├─────────────────────────────┤
│  Open Settings              │
│  About                      │
│  Exit                       │
└─────────────────────────────┘
```

> 托盘菜单采用中性风格，不跟随主题（Matcha/Peachy/Mochi）变化。

四色托盘图标：白=空闲 / 绿=运行中 / 红=异常 / 黄=检测中。

---

## §13 Chat 界面

> 需求 → PRD §4 对话 Chat

### UI-14: 用户消息气泡

```
                              ┌─────────────────────────────┐
                              │ 帮我查一下今天的天气和待办  │
                              │                       12:28 │
                              └─────────────────────────────┘
                                        ↘
                                     🔵 ICON_LARGE
```

- 靠右，最大宽度 70%
- 背景：渐变 `#2B5C3E → #1F4A30`
- 文字：白色，圆角 radius_xl
- 阴影：shadow_md

### UI-15: AI 消息气泡

```
  🦞 ICON_MEDIUM
   ↘
  ┌──────────────────────────────────────────┐ ← 左侧 GAP_SMALL×0.5 accent 竖线
  │ 系统状态良好：                            │
  │ · Agent 运行中                         │
  │ · 内存使用 45%                           │
  │                                   12:28  │
  └──────────────────────────────────────────┘
```

- 靠左，最大宽度 70%
- 背景：raised `#1E2230`，圆角 radius_xl
- 左侧 accent 竖线标记
- 支持 Markdown 渲染

### UI-16: 流式打字指示器

```
  🦞
   ↘
  ┌──────────────────┐
  │ ●  ●  ●          │  ← 3 个 GAP_SMALL×1.5 圆点, accent 色, 1.2s 依次脉冲
  └──────────────────┘
```

### UI-17: Chat 输入区

```
  ┌──────────────────────────────────────────────────┐
  │ ┌──────────────────────────────────────────────┐ │
  │ │ 输入消息... (Shift + Enter 换行)             │ │  ← CHAT_INPUT_MIN_H, 圆角 radius_lg
  │ └──────────────────────────────────────────────┘ │
  │ [📎 附件]                                字数 [▶] │  ← 圆形 BTN_SEND 发送按钮
  └──────────────────────────────────────────────────┘
```

- 默认高度 CHAT_INPUT_MIN_H，最小 CHAT_INPUT_MIN_H，自动增长
- 焦点态：box-shadow `0 0 0 2px focus_glow`
- 发送按钮：accent 渐变圆形，hover glow

### UI-18: 文件附件卡片

```
  ┌──────────────────────────────────┐
  │  📎 report.pdf · 2.4 MB          │  ← 嵌入消息气泡
  │  📁 project-folder/ · 12 个文件  │
  │  🖼️ screenshot.png · 1.2 MB      │
  │     ┌────────────────────────┐   │
  │     │     (缩略图预览)        │   │
  │     └────────────────────────┘   │
  └──────────────────────────────────┘
```

点击可直接打开（目录→资源管理器，文件→系统默认程序）。路径需通过权限系统校验。

### UI-19: Chat 空状态

```
               🧄🦞
        "龙虾要吃蒜蓉的"
         有什么我能帮你？

  ┌─────────────────────────────┐
  │ 💡 试试: 帮我整理一下文件   │  ← 快捷建议气泡
  └─────────────────────────────┘
```

### UI-20: Chat 搜索

输入框 + 搜索结果列表（时间/角色/内容），点击跳转到对应消息。

### UI-36: 文件上传菜单

弹出菜单：📄 选择文件 / 📁 选择目录。调用 Win32 原生对话框。

---

## §14 Work 界面

> 需求 → PRD §5 工作台 Work

### UI-23: StepCard — 读操作

```
  ┌─────────────────────────────────────────────────┐
  │  📄  读取 config.json              ● 进行中     │
  │       ┌─────────────────────────────────────┐   │
  │       │ { "model": "gemma-3-4b-it:free" }  │   │  ← 输出摘要（可折叠）
  │       └─────────────────────────────────────┘   │
  └─────────────────────────────────────────────────┘
```

### UI-24: StepCard — 写操作

```
  ┌─────────────────────────────────────────────────┐
  │  ✏️  写入 settings.json          ✓ 已完成       │
  │       修改内容: 3 处变更                         │
  │       ┌────────┐ ┌────────┐ ┌────────┐         │
  │       │ 接受 ✓ │ │ 拒绝 ✗ │ │ 回退 ↩ │         │
  │       └────────┘ └────────┘ └────────┘         │
  └─────────────────────────────────────────────────┘
```

按钮颜色：接受(#3DD68C) / 拒绝(#FF6B6B) / 回退(#6C9FFF)

### UI-25: StepCard — 执行命令

```
  ┌─────────────────────────────────────────────────┐
  │  ⚡  执行 npm install              ✗ 失败       │
  │       error: ENOENT package.json               │
  │       ┌────────┐                               │
  │       │ 重试 ↻ │                               │
  │       └────────┘                               │
  └─────────────────────────────────────────────────┘
```

状态：● 进行中(绿 spinner) / ✓ 完成(绿) / ✗ 失败(红) / ? 等待确认(蓝)

### UI-26: 输出面板

```
  终端渲染:                          文档预览:
  ┌────────────────────────┐        ┌────────────────────────┐
  │ $ npm install          │        │ ## 配置说明             │
  │ added 234 packages     │        │ 模型: gemma-3-4b-it    │
  └────────────────────────┘        └────────────────────────┘
```

渲染器自动切换：根据 SSE 事件类型匹配 TerminalRenderer / PreviewRenderer / DiffRenderer / TableRenderer

### UI-27: Work 空状态

```
              ⚡
       AI 工作台已就绪
    在 Chat 中发送任务
  执行过程将在这里实时展示

    [ 查看示例任务 ]
```

### UI-28: Diff 可视化视图

```
  ┌──────────────────────────────────────────────────┐
  │  ✏️ 修改 main.cpp                  ✓ 已完成      │
  │  ┌────────────────────────────────────────────┐  │
  │  │  1  #include <stdio.h>                     │  │
  │  │  2  -int old_value = 10;                   │  │  ← 红色背景
  │  │  3  +int new_value = 20;                   │  │  ← 绿色背景
  │  │  4  void main() {                          │  │
  │  └────────────────────────────────────────────┘  │
  │  [ 接受 ✓ ] [ 拒绝 ✗ ] [ 回退 ↩ ]               │
  └──────────────────────────────────────────────────┘
```

### UI-29: Plan 模式方案展示

```
  ┌──────────────────────────────────────────────────┐
  │  📋 执行方案                                      │
  │  步骤: 1. 读取 config  2. 修改配置  3. 重启      │
  │  风险: 低 · 预计耗时: ~30s                       │
  │  ┌──────────────┐  ┌──────────────┐              │
  │  │  确认执行 ✓  │  │  修改方案 ✏  │              │
  │  └──────────────┘  └──────────────┘              │
  │  ┌──────────────┐                                │
  │  │  取消 ✗      │                                │
  │  └──────────────┘                                │
  └──────────────────────────────────────────────────┘
```

### UI-30: Work 模式任务输入 + Boot Check

```
  ┌──────────────────────────────────────────┐
  │  ┌────────────────────────────────────┐  │
  │  │ 输入任务描述...                    │  │
  │  └────────────────────────────────────┘  │
  │  [🔧 Boot Check]                  [ ▶ ] │
  └──────────────────────────────────────────┘
```

### UI-44: Boot Check 进度条

```
  ┌──────────────────────────────────────────────┐
  │  Boot Check 环境体检                          │
  │  ┌────────────────────────────────────────┐  │
  │  │ ████████████████░░░░░░  6/9           │  │
  │  └────────────────────────────────────────┘  │
  │  ● Node.js v22.14.0               ✓        │
  │  ● npm v10.8.0                    ✓        │
  │  ○ 磁盘空间 85GB                  检测中    │
  │  ○ 网络连通                       等待      │
  └──────────────────────────────────────────────┘
```

---

## §15 交互组件

> 需求 → PRD §6 模式体系

### UI-21: Chat/Work 模式切换器

位置：左导航栏底部，单按钮图标 `[↔]`。

```
  ┌────┐
  │[↔]│  ← 左 NAV 底部，icon-only
  └────┘

  hover: tooltip 显示当前模式（Chat / Work）
  点击: 切换模式，< 150ms 延迟
  图标: 根据当前模式显示不同 icon
```

### UI-22: AI行为选择器

控制行四控件之一，位于右侧面板顶部控制行。

```
  ┌──────────────────────────────────────────────────────────┐
  │  [Agent ▼]  [Report]  [AI行为 ▼]                  │
  └──────────────────────────────────────────────────────────┘

  位置: 右侧面板顶部控制行
  Agent  选项: OpenClaw(默认) / Hermes / Claude Code
  AI行为 选项: 自主(默认) / Ask / Plan
  Report  按钮: 打开上次 BootCheck 诊断报告
  即时生效, 持久化到 config.json
```

### 控制行（Chat/Work 共用）

Chat 和 Work 模式的输入区完全相同，区别仅在于输出区。控制行位于右侧面板顶部：

```
  ┌──────────────────────────────────────────────────────────┐
 │  [Agent ▼]  [Report]  [AI行为 ▼]                       │ ← 控制行
  └──────────────────────────────────────────────────────────┘
  ┌──────────────────────────────────────────────────────────┐
  │  [📎] │ 输入消息...                                 [▶] │ ← 输入行
  └──────────────────────────────────────────────────────────┘

  控制行: Agent下拉 / Report / AI行为下拉
  输入行: 📎附件 + 多行输入 + 字数 + 发送按钮(BTN_SEND圆形)
  (Voice 模式: 输入行隐藏，通过 [Text|Voice] 切换)
```

---

## §16 配置界面

> 需求 → PRD §7 模型管理

**通用设置（General）Tab：** Report 按钮（查看诊断报告）及其他通用选项。

### UI-47: Model Tab

```
┌──────────────────────────────────────────────────────────┐
│ General  │ Permissions │ Model  │  Log  │  About         │
├──────────────────────────────────────────────────────────┤
│  Current Model    ● openrouter/google/gemma-3-4b-it:free │
│  ─────────────────────────────────────────────────────── │
│  Select Model                                            │
│  ┌──────────────────────────────────────────┐  ┌──────┐  │
│  │ gemma-3-4b-it:free                 ▾     │  │  +   │  │
│  └──────────────────────────────────────────┘  └──────┘  │
│  Provider: OpenRouter                                    │
│  ─────────────────────────────────────────────────────── │
│  API Key                                                 │
│  ┌──────────────────────────────────────────────┐        │
│  │ ••••••••••••••••••••                   [Save] │        │
│  └──────────────────────────────────────────────┘        │
│  💡 Free model available                                 │
│  [ 使用免费模型 ]                                        │
│  ─────────────────────────────────────────────────────── │
│  Failover                                                │
│  [====] 启用自动切换                                      │
│  备用模型: [gemini-2.5-flash ▾]                          │
└──────────────────────────────────────────────────────────┘
```

### UI-07: 本地模型安装（向导 Step 4）

```
  启用 Gemma 本地推理     [====] 开/关

  ┌────────────────────────────────────────┐
  │  ☑ Gemma 3 E4B (入门, 8GB RAM)        │
  │  ☐ Gemma 3 26B MoE (中档, 16GB RAM)   │
  └────────────────────────────────────────┘

  下载进度（安装时显示）:
  ┌────────────────────────────────────────┐
  │ ████████████░░░░░░░░  62%             │
  │ 1.2GB / 2.0GB · Mirror 1 (HuggingFace)│
  └────────────────────────────────────────┘

                        [ Skip ⏭ ]
```

### UI-42: Gemma 下载进度（独立弹窗）

```
  ┌────────────────────────────────────────┐
  │  📦 下载 Gemma 3 E4B                   │
  │  ┌──────────────────────────────────┐  │
  │  │████████████████░░░░░░░░  62%     │  │
  │  └──────────────────────────────────┘  │
  │  1.2 GB / 2.0 GB                       │
  │  速度: 12.5 MB/s · 预计剩余: 1 分钟    │
  │  来源: Mirror 1 (HuggingFace)          │
  │  ┌──────────────┐                      │
  │  │   取消下载   │                      │
  │  └──────────────┘                      │
  └────────────────────────────────────────┘
```

三源自动回退：HuggingFace → ModelScope → 镜像站

---

## §17 扩展界面

> 需求 → PRD §8 Agent 能力

### UI-08: 向导 Step 5 — 连接 IM

```
  在手机上随时指挥 AnyClaw 📱

  ┌────────────────────────────────────────┐
  │  ✈️ Telegram           🟢 已连接       │
  │  ┌────────────────────────────────────┐│
  │  │ 🎮 Discord           ○ 未配置     ││
  │  │    [ 配置 ]                        ││
  │  └────────────────────────────────────┘│
  │  ┌────────────────────────────────────┐│
  │  │ 💬 WhatsApp          ○ 未配置     ││
  │  │    [ 配置 ]  扫码绑定              ││
  │  └────────────────────────────────────┘│
                        [ Skip ⏭ ]
```

### UI-43: 应用授权界面

```
  ┌──────────────────────────────────────────────────────┐
  │  📱 应用授权                                          │
  │  ┌────────────────────────────────────────────────┐  │
  │  │ 📧 Outlook           推荐: CLI  [授权] [拒绝] │  │
  │  └────────────────────────────────────────────────┘  │
  │  ┌────────────────────────────────────────────────┐  │
  │  │ 📅 Google Calendar   推荐: MCP  [授权] [拒绝] │  │
  │  └────────────────────────────────────────────────┘  │
  │  交互方式优先级: CLI > MCP > COM > Skill             │
  └──────────────────────────────────────────────────────┘
```

### UI-60: 知识库面板

```
  📚 知识库
  已索引: 3 个文档
  ┌────────────────────────────────────────┐
  │ 📄 notes.md          12KB  ✓           │
  │ 📄 api-docs.md       45KB  ✓           │
  └────────────────────────────────────────┘
  [ + 添加知识源文件 ]
  🔍 搜索: [________________]
  ┌────────────────────────────────────────┐
  │ 结果: "模型配置" 在 notes.md 中找到     │
  └────────────────────────────────────────┘
```

### UI-65: Skill 管理面板

```
  🧩 Skill 管理
  ────────────────────────────────────────
  内置 (Built-in)
  ┌────────────────────────────────────┐
  │ ✅ healthcheck   v1.0  安全检查    │
  │ ✅ summarize     v1.2  内容摘要    │
  │ ✅ weather       v1.0  天气查询    │
  └────────────────────────────────────┘
  第三方 (ClawHub)
  ┌────────────────────────────────────┐
  │ ✅ github        v2.1  GitHub集成  │
  │ ⬜ excel-xlsx    v1.0  Excel操作  [安装]
  └────────────────────────────────────┘
  [ 检查版本更新 ]  [ 从 ClawHub 浏览更多 ]
```

---

## §18 权限与弹窗

> 需求 → PRD §9 权限系统

### UI-31: 权限确认弹窗

```
  ┌────────────────────────────────────────────────┐
  │  全屏遮罩 (50% opacity)                         │
  │  ┌──────────────────────────────────────────┐  │
  │  │  🔐 AnyClaw 权限确认                     │  │
  │  │  Agent 请求执行命令：                     │  │
  │  │  ┌────────────────────────────────────┐  │  │
  │  │  │ pip install pandas                 │  │  │
  │  │  └────────────────────────────────────┘  │  │
  │  │  权限项: EXEC_SHELL                     │  │
  │  │                                          │  │
  │  │  ┌────────┐ ┌──────────┐ ┌──────────┐   │  │
  │  │  │ 拒绝   │ │ 此次授权  │ │ 永久授权  │   │  │
  │  │  │#FF6B6B │ │ #6C9FFF  │ │ #3DD68C  │   │  │
  │  │  └────────┘ └──────────┘ └──────────┘   │  │
  │  │  ⏱ 60秒超时自动拒绝                      │  │
  │  └──────────────────────────────────────────┘  │
  └────────────────────────────────────────────────┘
```

弹窗：DIALOG_W 宽，bg=panel，圆角 radius_lg，可拖拽。Fallback：UI 不可用时降级 Win32 MessageBox。

### UI-32: Ask 模式 ask_feedback 弹窗

```
  ┌────────────────────────────────────────────────┐
  │  🤔 AI 需要你的决定                             │
  │  建议: 将模型切换到 gemini-2.5-flash            │
  │  原因: 当前模型响应超时                         │
  │  ┌──────────────────────────────────────────┐  │
  │  │ ✅ 同意切换                              │  │  ← 预设选项
  │  └──────────────────────────────────────────┘  │
  │  ┌──────────────────────────────────────────┐  │
  │  │ ❌ 保持当前模型                           │  │
  │  └──────────────────────────────────────────┘  │
  │  自定义回复:                                   │  │
  │  ┌──────────────────────────────────────────┐  │
  │  │                                          │  │
  │  └──────────────────────────────────────────┘  │
  │                    [ 发送 ]                    │
  └────────────────────────────────────────────────┘
```

### UI-46: Permissions Tab

```
┌──────────────────────────────────────────────────────────┐
│ General  │ Permissions │ Model  │  Log  │  About         │
├──────────────────────────────────────────────────────────┤
│  权限策略 (19 项)                                         │
│  ── 文件系统 ──────────────────────────────────────────── │
│  FS Read Workspace       [allow ▾]                       │
│  FS Write Workspace      [allow ▾]                       │
│  FS Read External        [ask ▾]                         │
│  FS Write External       [deny ▾]                        │
│  ── 命令执行 ──────────────────────────────────────────── │
│  Exec Shell              [ask ▾]                         │
│  Exec Install            [deny ▾]                        │
│  Exec Delete             [deny ▾]                        │
│  ── 网络 ──────────────────────────────────────────────── │
│  Net Outbound            [allow ▾]                       │
│  Net Inbound             [deny ▾]                        │
│  Net Intranet            [ask ▾]                         │
│  ── 设备 ──────────────────────────────────────────────── │
│  Device Camera           [deny ▾]                        │
│  Device Mic              [deny ▾]                        │
│  Device Screen           [ask ▾]                         │
│  Device USB Storage      [deny ▾]                        │
│  Device Remote Node      [ask ▾]                         │
│  Device New Device       [deny ▾]                        │
│  ── 系统 ──────────────────────────────────────────────── │
│  Clipboard Read          [allow ▾]                       │
│  Clipboard Write         [allow ▾]                       │
│  System Modify           [deny ▾]                        │
│  [ 保存权限配置 ]    ● 未保存 / ✓ Saved                   │
└──────────────────────────────────────────────────────────┘
```

---

## §19 工作区界面

> 需求 → PRD §10 工作区管理

### UI-39: 工作区锁冲突弹窗

```
  ┌────────────────────────────────────────┐
  │  ⚠️ 工作区已被占用                     │
  │  D:\Projects\AnyClaw · PID: 12345     │
  │  ┌──────────────┐  ┌──────────────┐   │
  │  │  选择其他目录 │  │   退出程序   │   │
  │  └──────────────┘  └──────────────┘   │
  └────────────────────────────────────────┘
```

### UI-40: 工作区目录丢失恢复弹窗

```
  ┌────────────────────────────────────────┐
  │  ❌ 工作区目录不可达                   │
  │  D:\Projects\AnyClaw 已不存在          │
  │  ┌──────────────────┐                  │
  │  │  重新创建目录     │                  │
  │  └──────────────────┘                  │
  │  ┌──────────────────┐                  │
  │  │  选择其他目录     │                  │
  │  └──────────────────┘                  │
  │  ┌──────────────────┐                  │
  │  │  创建新工作区     │                  │
  │  └──────────────────┘                  │
  └────────────────────────────────────────┘
```

### UI-41: 工作区详情弹窗

点击左面板工作区名称触发。显示：名称、路径、磁盘可用空间、健康状态、核心文件检查结果。

### UI-64: 工作区切换器/管理器

```
  ┌────────────────────────────────────────┐
  │  📂 工作区管理                          │
  │  ┌──────────────────────────────────┐  │
  │  │ ■ 个人助手    D:\Projects\...    │  │  ← 当前选中
  │  │   通用助手 · 2026-03-15 创建     │  │
  │  └──────────────────────────────────┘  │
  │  ┌──────────────────────────────────┐  │
  │  │ □ 开发项目    D:\Dev\...         │  │
  │  └──────────────────────────────────┘  │
  │  ┌──────────────┐  ┌──────────────┐   │
  │  │  + 新建工作区 │  │   关闭       │   │
  │  └──────────────┘  └──────────────┘   │
  └────────────────────────────────────────┘
```

---

## §20 设置页

> 需求 → PRD §11 设置

### UI-45: General Tab

```
┌──────────────────────────────────────────────────────────┐
│ General  │ Permissions │ Model  │  Log  │  About         │
├──────────────────────────────────────────────────────────┤
│  Agent Status    ● Ready                               │
│  Install Path      C:\Users\xxx\AppData\Roaming\...      │
│  Workspace         D:\Projects\AnyClaw                   │
│  ─────────────────────────────────────────────────────── │
│  Boot Start        [====]  Start on boot                 │
│  Auto Start        [    ]  Restart on crash              │
│  ─────────────────────────────────────────────────────── │
│  Theme             [■ Dark] [□ Light] [□ Classic]        │
│  Language          [CN / EN ▾]                            │
│  Exit Confirm      [====]  弹出退出确认                  │
│  Close on Exit     [====]  Close OpenClaw on exit        │
│  ─────────────────────────────────────────────────────── │
│  Security Status                                           │
│    API Key: ● 已配置                                      │
│    Agent Port: 18789                                    │
│    Config Dir: 可写 ✓                                     │
│  ─────────────────────────────────────────────────────── │
│  ┌────────────────────────────────────────────────────┐  │
│  │           [ 🔧 Reconfigure Wizard ]                │  │
│  └────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────┘
```

### UI-48: Log Tab

```
┌──────────────────────────────────────────────────────────┐
│ General  │ Permissions │ Model  │  Log  │  About         │
├──────────────────────────────────────────────────────────┤
│  Log System   [====]  Write to logs\app.log              │
│  Log Level    [Debug ▾]                                  │
│  ─────────────────────────────────────────────────────── │
│  Filter       [All ▾]  [🔄 Refresh]                      │
│  ┌────────────────────────────────────────────────────┐  │
│  │ [12:30:01 INFO] Agent health check OK            │  │
│  │ [12:30:02 WARN] Session timeout approaching        │  │
│  │ [12:30:03 ERROR] Failed to connect to model        │  │
│  └────────────────────────────────────────────────────┘  │
│  ● DEBUG  ● INFO  ● WARN  ● ERROR  [Export] [Clear]      │
└──────────────────────────────────────────────────────────┘
```

级别颜色：DEBUG `#808080` / INFO `#4090E0` / WARN `#E0C020` / ERROR `#E03030`

### UI-49: About Tab

```
┌──────────────────────────────────────────────────────────┐
│ General  │ Permissions │ Model  │  Log  │  About         │
├──────────────────────────────────────────────────────────┤
│              ┌──────────────┐                            │
│              │  🧄🦞        │ 120×120                    │
│              └──────────────┘                            │
│           AnyClaw LVGL                                   │
│       Garlic Lobster - Your AI Assistant                 │
│              Version: 2.2.1                              │
│  Tech Stack: LVGL 9.6 + SDL2 / C++17 / Win32 API        │
│  ─────────────────────────────────────────────────────── │
│  ┌────────────────┐  ┌────────────────┐                  │
│  │  Export Config  │  │   Clear Chat   │                  │
│  └────────────────┘  └────────────────┘                  │
│  ┌────────────────┐  ┌────────────────┐                  │
│  │ Export Migration│  │Import Migration│                  │
│  └────────────────┘  └────────────────┘                  │
│        Copyright 2026 AnyClaw Project                    │
│  ┌────────────────────────────────────────────────────┐  │
│  │         Check Skill Versions                       │  │
│  └────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────┘
```

---

## §21 系统界面

> 需求 → PRD §13 系统管理

### UI-33: License 激活弹窗

```
  ┌────────────────────────────────────────────────┐
  │  License Activation                             │
  │  ⚠ Trial Period Ended                           │
  │  Enter a license key to continue                │
  │  Current sequence: 3 (next: seq 4, 240h)       │
  │  ┌──────────────────────────────────────────┐   │
  │  │ XXXXX-XXXXX-XXXXX                        │   │
  │  └──────────────────────────────────────────┘   │
  │  ┌──────────────────┐ ┌─────────────────┐       │
  │  │    Activate      │  │     Close      │       │
  │  │   绿色 #3DD68C   │  │ btn_secondary  │       │
  │  └──────────────────┘ └─────────────────┘       │
  └────────────────────────────────────────────────┘
```

### UI-34: 退出确认弹窗

```
  ┌────────────────────────────────────────┐
  │  确认退出 AnyClaw？                     │
  │  Agent 将继续在后台运行。             │
  │  ┌────────────┐  ┌─────────────────┐   │
  │  │ 最小化到    │  │    退出程序     │   │
  │  │  托盘 📌    │  │    ⚠️          │   │
  │  └────────────┘  └─────────────────┘   │
  │  ┌────────────┐                        │
  │  │   取消     │                        │
  │  └────────────┘                        │
  └────────────────────────────────────────┘
```

### UI-37: Toast 通知

```
                        ┌─────────────────────────────┐
                        │ ✅ Agent 已启动            │  ← 绿色左边框
                        └─────────────────────────────┘
                  ┌─────────────────────────────┐
                  │ ⚠️ 磁盘空间不足              │  ← 黄色左边框
                  └─────────────────────────────┘
```

右下角，向上堆叠，200ms slide-in，3 秒自动消失，最多同时 3 条。

### UI-38: 警报弹窗（带声音）

```
  ┌────────────────────────────────────────┐
  │  🚨 警报                                │
  │  磁盘空间不足 100MB                     │
  │  ┌────────────┐  ┌──────────────┐      │
  │  │  立即处理  │  │   稍后提醒    │      │
  │  └────────────┘  └──────────────┘      │
  └────────────────────────────────────────┘
```

模态弹窗 + 系统警报音。与 Toast 不同：需要用户操作。

### UI-56: 崩溃日志查看

启动时检测 `last_crash.txt` → 弹出"💥 AnyClaw 异常退出"→ [查看日志] / [继续启动]

### UI-57: 更新检测 & 下载

```
  ┌────────────────────────────────────────┐
  │  📦 发现新版本 v2.3.0                   │
  │  当前: v2.2.1 → 新版本: v2.3.0         │
  │  ┌──────────────────────────────────┐  │
  │  │████████████████░░░░  下载中 78%  │  │
  │  └──────────────────────────────────┘  │
  │  ┌────────────┐  ┌──────────────┐      │
  │  │  立即更新  │  │   稍后提醒   │      │
  │  └────────────┘  └──────────────┘      │
  └────────────────────────────────────────┘
```

### UI-58: 托盘气泡通知

Windows 系统托盘区域弹出气泡：`tray_balloon(title, message, timeout_ms)`

### UI-50: Feature Flags Tab

```
┌──────────────────────────────────────────────────────────┐
│  功能开关 / Feature Flags                                │
│  ─────────────────────────────────────────────────────── │
│  沙箱执行           [    ]  sbx_sandbox_exec             │
│  可观测性追踪       [    ]  obs_tracing    ⚠ 需重启      │
│  启动自修复         [    ]  boot_self_heal               │
│  长期记忆           [    ]  mem_long_term                │
│  知识库             [    ]  kb_knowledge_base            │
│  远程协作           [    ]  remote_collab   ⚠ 需重启    │
│  ─────────────────────────────────────────────────────── │
│  [ 重置全部默认值 ]                                       │
└──────────────────────────────────────────────────────────┘
```

### UI-51: Tracing Tab

```
┌──────────────────────────────────────────────────────────┐
│  Tracing 可观测性                                         │
│  ─────────────────────────────────────────────────────── │
│  Filter  [All ▾]  [🔄 Refresh]                           │
│  ┌────────────────────────────────────────────────────┐  │
│  │ 12:30:01  health_check_cycle    52ms   ok         │  │
│  │ 12:30:02  http_post_stream     1200ms  ok         │  │
│  │ 12:30:10  http_post_stream    30000ms  timeout    │  │
│  └────────────────────────────────────────────────────┘  │
│  事件总数: 847   Ring Buffer: 1000/1000                   │
│  [Export traces.jsonl]  [Clear]                           │
└──────────────────────────────────────────────────────────┘
```

---

# 第四层：品牌

## §22 品牌文案

> 需求 → PRD §1 产品概览

### 22.1 品牌关键词

**龙虾要吃蒜蓉的 🧄🦞**

### 22.2 文案调性

| 维度 | 要 | 不要 |
|------|-----|------|
| 人称 | "我" / "你" | "系统" / "用户" |
| 语气 | 友好、轻松、有个性 | 冷冰冰、机械、官腔 |
| 长度 | 简洁，一句说完 | 冗长、多段 |
| 标点 | 适度使用 emoji | 满屏感叹号 |

### 22.3 常用文案模板

| 场景 | 文案 |
|------|------|
| 欢迎 | "龙虾要吃蒜蓉的 🧄🦞 — 你的 AI 助手已就位！" |
| 空状态 | "有什么我能帮你？" |
| 操作成功 | "搞定！✓" |
| 操作失败 | "出了点问题 — {原因}" |
| 加载中 | "正在{动词}..." |
| 确认退出 | "Agent 将继续在后台运行。" |
| 权限请求 | "Agent 想{动作}，你批准吗？" |
| 超时 | "⚠️ AI 回复超时，请重试。" |
| 磁盘不足 | "🚨 空间快满了，清理一下？" |
| 更新可用 | "📦 有新版本啦！" |

### 22.4 按钮文案

| 类型 | 推荐 | 避免 |
|------|------|------|
| 主操作 | "Get Started" / "保存" / "确认" | "提交" / "执行" |
| 次要 | "取消" / "跳过" / "稍后" | "关闭" / "返回" |
| 危险 | "删除" / "拒绝" / "断开" | "确认删除" |
| 链接 | "了解更多" / "查看详细" | "点击这里" |

---

> **文档完成。** 22 章，4 层结构（基础规范 / 控件与状态 / 界面设计 / 品牌）。
>
> | 产品需求 | `docs/PRD.md` |
> |----------|--------------|
> | 文档索引 | `docs/README.md` |
> | 编号索引 | 见本文档开头「编号索引」 |
