# AnyClaw LVGL — 设计规范 v2.2.11

> 版本：v2.2.11（整合修订） | 目标用户：大学生 & 职场年轻人 | 更新时间：2026-04-24
> 本文档定义 AnyClaw "长什么样、怎么画、画在哪"。产品需求见 `PRD.md`。

---

## 编号索引（修改前必查）

> ⚠️ 引用锚点用功能编号（如 CI-01），不用章节号。修改任何功能时查此表找到所有关联节。

| PRD 编号 | PRD 章节 | 功能名 | Design UI 编号 | Design 章节 | UI 状态 |
|----------|---------|--------|---------------|------------|---------|
| INST-01 | §2.2 | 实例互斥 | UI-02 | §11 首次体验 | ✅ |
| SC-01 | §2.3 | 启动自检 | UI-01, UI-03 | §11 首次体验 | ✅ |
| WIZ-01 | §2.4 | 首次启动向导 | UI-06~12 | §11 首次体验 | ✅/🔧 |
| WIN-01 | §3.1 | 窗口与布局 | UI-04, UI-05, UI-11, UI-12, UI-55 | §12 窗口系统 | ✅/🔧 |
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
| RT-01 | §12.1.5 | 运行时系统 / 三 Agent 并行（Leader 模式） | — | §12 窗口系统 | 🔧 |
| MODEL-01 | §7.2 | 云端模型管理 | UI-35, UI-47 | §16 配置界面 | ✅ |
| LOCAL-01 | §7.3 | 本地模型管理 | UI-07, UI-42 | §16 配置界面 | 🔧 |
| FAILOVER-01 | §7.4 | Model Failover | UI-47 | §16 配置界面 | ✅ |
| SK-01 | §8.2 | Skill 管理 | UI-65 | §17 扩展界面 | 🔧 |
| PERM-01 | §9.1 | 权限系统 | UI-31, UI-46 | §18 权限与弹窗 | 🔧 |
| WS-01 | §10 | 工作区管理 | UI-39~41, UI-64 | §19 工作区界面 | 🔧 |
| LOG-01 | §13.1 | 日志 | UI-48 | §20 设置页 | 🔧 |
| LOG-02 | §11.1.4 | C4 系统与诊断中心 | UI-48A | §20 设置页 | 🔧 |
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
- §20 设置页（UI-45~51, UI-48A）
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

三套主打主题，通过 Settings General Tab 切换，即时生效无需重启。

**主打主题（完整规格）：**

| 主题 ID | 名称 | 定位 | 默认 |
|---------|------|------|------|
| `matcha` | 抹茶 Matcha v1 | 深色系，薄荷绿清新风，长时间使用 | ✅ 默认 |
| `peachy` | 桃气 Peachy v2 | 暖色系，珊瑚橘苹果风，年轻人友好 | |
| `mochi` | 糯 Mochi v3 | 米白系，奶茶棕温柔风，质感文艺 | |

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

> Mochi 是三个主打主题中唯一使用衬线体的——思源宋体 + Lora 传递"安静阅读"的质感。标题用衬线，正文 UI 仍用 Noto Sans SC（可读性）。

**混合策略：**

| 场景 | 使用字体 |
|------|---------|
| 正文、消息、UI 控件 | Noto Sans SC / Plus Jakarta Sans（无衬线，保证可读性） |
| 标题、品牌文案、About 页 | 思源宋体 SC / Lora（衬线，传递质感） |
| 引用块、笔记 | 思源宋体 SC / Lora（衬线，阅读感） |
| 代码块 | JetBrains Mono（等宽） |

---

### 2.5 色彩使用规则

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

### 2.6 三套主打主题完整对照表

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
| 中文字体 | Noto Sans SC | Noto Sans SC | Noto Sans SC(正文) / 思源宋体SC(标题) |
| 圆角风格 | 标准 SCALE(8/12/16) | 放大 SCALE(12/20/22) | 微收敛 SCALE(8/10/14) |
| 阴影色相 | 冷黑 | 暖棕 | 暖棕（更淡） |
| 按钮风格 | accent 渐变圆形 | 双色渐变圆形 | accent 纯色圆形 |

---

## §3 排版系统

> 需求 → PRD §3 主界面

### 3.1 字体栈（三套主打主题）

#### 🍵 Matcha v1 — 清新锐利

| 类型 | 主选 | 备选 | 回退 |
|------|------|------|------|
| 中文（UI/正文/标题通用） | **Noto Sans SC** | 思源黑体 SC | PingFang SC, 微软雅黑, sans-serif |
| 英文（UI/正文） | **Plus Jakarta Sans** | Inter | Segoe UI, sans-serif |
| 英文（标题/品牌） | **Plus Jakarta Sans Bold** | Inter Bold | Segoe UI Bold, sans-serif |
| 等宽 | **JetBrains Mono** | Fira Code | Consolas, monospace |

> Plus Jakarta Sans 比 Inter 更圆润有温度，比 Montserrat 更现代。Noto Sans SC 比微软雅黑清晰锐利，Variable Weight 支持好。全主题统一无衬线，干净利落。

#### 🍑 Peachy v2 — 温暖亲和

| 类型 | 主选 | 备选 | 回退 |
|------|------|------|------|
| 中文（UI/正文/标题通用） | **Noto Sans SC** | 思源黑体 SC | PingFang SC, 微软雅黑, sans-serif |
| 英文（UI/正文） | **Nunito** | Quicksand | Segoe UI, sans-serif |
| 英文（标题/品牌） | **Nunito Bold** | Quicksand Bold | Segoe UI Bold, sans-serif |
| 等宽 | **JetBrains Mono** | Fira Code | Consolas, monospace |

> Nunito 圆端设计天然萌系，和珊瑚橘暖色搭配完美。字重偏轻（Regular=400 感觉比其他字体细），标题用 Bold(700) 对比更鲜明。

#### 🍡 Mochi v3 — 文艺质感（唯一混用衬线体）

| 类型 | 用途 | 主选 | 备选 | 回退 |
|------|------|------|------|------|
| 中文 UI 控件 | 按钮/输入/菜单 | **Noto Sans SC** | 思源黑体 SC | PingFang SC, sans-serif |
| 中文 标题/引用 | 弹窗标题/引用块 | **思源宋体 SC** | LXGW WenKai | Noto Serif SC, serif |
| 英文 UI 控件 | 按钮/输入/标签 | **Plus Jakarta Sans** | Inter | Segoe UI, sans-serif |
| 英文 标题/引用 | 品牌/About/引用 | **Lora** | Merriweather | Georgia, serif |
| 等宽 | 代码/日志 | **JetBrains Mono** | Fira Code | Consolas, monospace |

> Mochi 混用衬线体——UI 控件用无衬线保证可读性，标题和引用用衬线传递「安静阅读」质感。思源宋体 SC + Lora 传递茶道美学。

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

| UI 元素 | 字号 | 字重 | 🍵 Matcha | 🍑 Peachy | 🍡 Mochi |
|---------|------|------|-----------|-----------|----------|
| **品牌标题** (标题栏/About) | display | 700 | PJS Bold | Nunito Bold | Lora Bold |
| **弹窗标题** | h1 | 700 | PJS Bold | Nunito Bold | 思源宋体 SC Bold |
| **区域标题** (面板/Tab) | h2 | 600 | PJS SemiBold | Nunito SemiBold | 思源宋体 SC Bold |
| **卡片标题** (StepCard/Session) | h3 | 600 | PJS SemiBold | Nunito SemiBold | PJS SemiBold |
| **正文** (消息/内容) | body | 400 | PJS Regular | Nunito Regular | PJS Regular |
| **强调正文** | body | 600 | PJS SemiBold | Nunito SemiBold | PJS SemiBold |
| **Chat 消息** | body | 400 | PJS Regular | Nunito Regular | PJS Regular |
| **Chat 消息（Mochi引用块）** | body | 400 | — | — | Lora Regular |
| **按钮文字** | body | 600 | PJS SemiBold | Nunito SemiBold | PJS SemiBold |
| **输入框文字** | body | 400 | PJS Regular | Nunito Regular | PJS Regular |
| **输入框占位符** | body | 400 | PJS Regular | Nunito Regular | PJS Regular |
| **下拉选项** | body | 400 | PJS Regular | Nunito Regular | PJS Regular |
| **次要信息** (时间戳/说明) | small | 400 | PJS Regular | Nunito Regular | PJS Regular |
| **标签/胶囊** | caption | 500 | PJS Medium | Nunito Medium | PJS Medium |
| **代码块** | code | 400 | JetBrains Mono | JetBrains Mono | JetBrains Mono |
| **行内代码** | code | 400 | JetBrains Mono | JetBrains Mono | JetBrains Mono |
| **终端日志** | code | 400 | JetBrains Mono | JetBrains Mono | JetBrains Mono |
| **步骤指示器文字** | small | 400 | PJS Regular | Nunito Regular | PJS Regular |
| **向导步骤名** | caption | 500 | PJS Medium | Nunito Medium | PJS Medium |
| **设置行标签** | body | 400 | PJS Regular | Nunito Regular | PJS Regular |
| **设置行说明** | small | 400 | PJS Regular | Nunito Regular | PJS Regular |
| **Toast 通知** | body | 400 | PJS Regular | Nunito Regular | PJS Regular |
| **Tooltip** | small | 400 | PJS Regular | Nunito Regular | PJS Regular |
| **空状态文案** | body | 400 | PJS Regular | Nunito Regular | PJS Regular |
| **中文** (上述所有 CJK 场景) | 同上 | 同上 | Noto Sans SC | Noto Sans SC | 无衬线=Noto SC / 衬线=思源宋体 SC |

**缩写：** PJS = Plus Jakarta Sans, Noto SC = Noto Sans SC, 思源宋体 SC = Source Han Serif SC

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
| `NotoSansSC-Regular.otf` | ~2MB | Regular 400 |
| `NotoSansSC-Bold.otf` | ~2MB | Bold 700 |
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

| Font 指针 | 🍵 Matcha | 🍑 Peachy | 🍡 Mochi |
|-----------|-----------|-----------|----------|
| `display` | PJS Bold 28px | Nunito Bold 28px | Lora Bold 28px |
| `h1` | PJS Bold 22px | Nunito Bold 22px | 思源宋体 Bold 22px |
| `h2` | PJS SemiBold 18px | Nunito SemiBold 18px | 思源宋体 Bold 18px |
| `h3` | PJS SemiBold 15px | Nunito SemiBold 15px | PJS SemiBold 15px |
| `body` | PJS Regular 13px | Nunito Regular 13px | PJS Regular 13px |
| `body_strong` | PJS SemiBold 13px | Nunito SemiBold 13px | PJS SemiBold 13px |
| `small` | PJS Regular 11px | Nunito Regular 11px | PJS Regular 11px |
| `caption` | PJS Medium 10px | Nunito Medium 10px | PJS Medium 10px |
| `code` | JetBrains Mono 12px | JetBrains Mono 12px | JetBrains Mono 12px |
| `cjk_body` | Noto SC Reg | Noto SC Reg | Noto SC Reg |
| `cjk_title` | Noto SC Bold | Noto SC Bold | 思源宋体 SC Bold |

> Mochi 的 `cjk_title` 使用思源宋体 SC（衬线），与其他主题不同。
> 所有 CJK 字体通过 `lv_font_t->fallback` 链接到对应英文主字体，实现中英混排自动切换。

#### 回退链

```
英文渲染请求 → 主字体（如 PJS Regular）
  → 找不到字形 → fallback → CJK 字体（如 Noto SC Regular）
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
| `NAV_W` | 窗口宽 × 6% | 48px | 36px |
| `NAV_ICON_BTN` | 导航宽 × 70% | 28px | — |
| `NAV_QUICK_H` | 内容区高 × 25% | — | — |

#### 左面板（parent: 可用宽 = 窗口宽 − 导航宽）

| Token | 公式 | 1450px 基准 | 最小值 |
|-------|------|------------|--------|
| `LEFT_PANEL_W` | 可用宽 × 25% | 341px | 160px |
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

品牌角色统一使用 **PNG**，采用「body/sprout 分层」方案，便于在 LVGL 内做轻量动画：
- `body`：蒜瓣主体（左右轻摆、跳跃）
- `sprout`：叶茎层（独立弹跳、喷火粒子）
- 透明底统一使用 `COLORKEY = #FF00FF`（导出后替换为 alpha）

#### 角色设定与风格约束

| 项目 | 规范 |
|------|------|
| 风格关键词 | 日系 kawaii、圆润 Q 弹、年轻感、治愈感 |
| 头身比例 | 1:1（头体一体化大蒜） |
| 表情 | 微笑眯眼 + 两团腮红 |
| 线条 | 轮廓线粗细保持统一，避免锯齿感 |
| 动态基线 | idle 状态下 sprout 轻微呼吸，body 稳定 |

#### 大蒜三主题色板（精确色值）

| 部位 | 🍵 Matcha | 🍑 Peachy | 🍡 Mochi |
|------|-----------|-----------|----------|
| 蒜瓣主体 | `#FFFFFF → #E8F5E9` | `#FFFFFF → #FFF0E6` | `#FFFDF9 → #F5EDE4` |
| 蒜瓣轮廓 | `#B2DFDB` | `#FFD4B8` | `#D4C4B0` |
| 叶茎/茎 | `#3DD68C` | `#FF7F50` | `#A67B5B` |
| 叶茎高光 | `#6EE7B7` | `#FFB347` | `#C9A96E` |
| 腮红 | `#FFB3B3` (40%) | `#FF9F7A` (40%) | `#C4868C` (40%) |
| 眼睛 | `#2D3748` | `#5D3A1A` | `#4A3728` |
| 眼睛高光 | `#FFFFFF` | `#FFFFFF` | `#FFFFFF` |

#### 龙虾 AI 头像色板（全主题共用）

| 部位 | 色值 |
|------|------|
| 身体 | `#FF6B6B → #E84545` |
| 肚子 | `#FFE0E0` |
| 钳子 | `#FF4757` |
| 触须 | `#FF8787` |
| 腮红 | `#FFB3B3` (40%) |

#### 文件路径与命名

| 类型 | 路径规范 | 说明 |
|------|---------|------|
| 大蒜主体 | `assets/mascot/{theme}/garlic_48.png` | body 层 |
| 大蒜叶茎 | `assets/mascot/{theme}/garlic_sprout.png` | sprout 层 |
| AI 小头像 | `assets/icons/ai/garlic_24.png` | 通用小尺寸 |
| 龙虾头像 | `assets/icons/ai/lobster_24.png` / `lobster_32.png` / `lobster_48.png` | 三档展示 |

#### DPI 与尺寸策略

| 尺寸 | 目标 |
|------|------|
| 16px | 极简轮廓（保证可辨识） |
| 24px | 聊天气泡基础头像 |
| 32px | 面板与列表完整形态 |
| 48px | 标准品牌展示 |
| 96px | 插画级细节（欢迎页/About） |

### 5.4 托盘图标

大蒜 + LED 合体图标，按主题与运行状态组合：

| 状态 | Matcha LED | Peachy LED | Mochi LED |
|------|-----------|------------|-----------|
| 空闲 | `#E8ECF4` | `#8B7355` | `#B0A394` |
| 运行中 | `#3DD68C` | `#FF7F50` | `#A67B5B` |
| 异常 | `#FF6B6B` | `#FF5C5C` | `#C47070` |
| 检测中 | `#FFBE3D` | `#FFB347` | `#C9A96E` |

命名规范：`assets/tray/{theme}/tray_{state}_{size}.png`

示例：
- `assets/tray/matcha/tray_active_32.png`
- `assets/tray/peachy/tray_error_16.png`
- `assets/tray/mochi/tray_idle_48.png`

尺寸：16/20/32/48px（PNG，托盘 API 使用位图）。

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

### 5.6 资源生成工作流

#### 工具链

| 工具 | 用途 |
|------|------|
| `rsvg-convert` | SVG 转多尺寸 PNG |
| `pngquant` | 有损压缩，减小包体 |
| `optipng` | 无损优化，进一步压缩 |

#### 产出流程

```
设计稿 (SVG)
  → rsvg-convert 批量导出 16/20/24/32/48/96
  → pngquant 质量压缩
  → optipng 无损优化
  → 视觉验收 + 尺寸验收 + 色值验收
```

#### 质量标准

| 维度 | 标准 |
|------|------|
| 透明通道 | RGBA + alpha 正确，无黑边 |
| 体积阈值 | 24px < 1KB；48px < 5KB；96px < 15KB |
| 色值偏差 | 主题关键色偏差不超过 ±2 |
| 可辨识性 | 16px 下仍可区分大蒜/龙虾主体特征 |

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

### 6.3 按钮尺寸规范（统一标准）

#### 高度标准

| 按钮类型 | 高度公式 | 800px 基准 | 最小值 | 最大值 | 说明 |
|---------|---------|-----------|--------|--------|------|
| **文本按钮（Primary/Secondary/Ghost）** | 窗口高×6% | 48px | **40px** | 56px | 统一高度，互为同组时必须等高 |
| 发送按钮 BTN_SEND | 固定值 | — | 32px | — | 圆形特殊按钮，不参与高度统一 |
| 上传按钮 BTN_UPLOAD | 固定值 | — | 24px | — | 工具栏内嵌，不参与高度统一 |
| 右键菜单项 CTX_ITEM | 窗口高×8% | 64px | 28px | — | 下拉菜单项 |
| 窗口控制按钮 WC_BTN | 固定值 | — | 32px | — | 标题栏内嵌 |

> ⚠️ **高度统一原则**：同一按钮组内（并排放置的按钮），所有按钮必须**等高**。禁止出现一个 40px 另一个 48px 的参差布局。
> ⚠️ **对齐原则**：同一行的按钮，按高度值向下对齐（底部在同一条基线上）。

#### 文字截断规则

**核心原则：按钮宽度自适应文字，禁止固定宽度导致截断。**

```
规则优先级：
1. 文字先完整显示（LV_SIZE_CONTENT 宽度）
2. 宽度上限 PRIMARY_BTN_MAX_W = 200px（超过则截断）
3. 截断策略：简化文字内容，不使用省略号
```

| 场景 | 正确做法 | 错误做法 |
|------|---------|---------|
| 文字过长（>200px） | 简化文案，如"进入主界面（受限）" → "受限模式" | "进入主界..." 带省略号 |
| 按钮组等宽 | 文字最长的按钮决定宽度，其他按钮 padding 补齐 | 各按钮按文字长度自适应 |
| 文字与图标混排 | 图标 + 文字，文字在前 | 图标遮住文字 |

**Primary 按钮宽度示例（800px 窗口）：**

```
┌─────────────────┐  ┌─────────────────┐
│   受限模式       │  │   向导修复       │  ← 文字简化，等高48px
└─────────────────┘  └─────────────────┘
        ↓                      ↓
┌─────────────────┐  ┌─────────────────┐
│    继续         │  │    取消         │  ← 短文案，padding补齐
└─────────────────┘  └─────────────────┘
```

### 6.4 单行控件高度统一规范

单行控件（输入框、下拉框、列表项、滑块等）**共享同一高度标准**，确保同行排列时底部对齐。

#### 高度标准 LP_ROW_H

| 窗口高 | LP_ROW_H 公式 | 800px 基准值 | 最小值 | 最大值 |
|--------|--------------|-------------|--------|--------|
| 单行控件高度 | 窗口高 × 5.5% | **44px** | **36px** | **52px** |

```
垂直对齐原则：
┌─────────────────────────────────────────────┐
│  标签文字（caption，text_secondary）         │  ← 文字 baseline 对齐
│ ┌─────────────────────────────────────────┐ │
│ │ 单行输入框 / 下拉框 / 列表项             │ │  ← 底部在同一条基线上
│ └─────────────────────────────────────────┘ │
└─────────────────────────────────────────────┘
```

#### 下拉列表面板规范（Dropdown List）

下拉面板用于模型选择、设置项等场景，**必须保证文字不被截断**：

```
┌──────────────────────────────────────────────────┐
│ 下拉面板宽度 = 触发器宽度（LV_PCT(100) 跟随父容器）│
│ 面板最大高度 = 窗口高 × 28%（最多显示 5 项）      │
│ 最小高度 = LP_ROW_H × 2                          │
│ 超出 5 项：面板内部滚动（lv_dropdown 内置）       │
│ 列表项高度 = LP_ROW_H（与输入框等高）            │
│ 列表项 padding = 0 GAP×2                         │
│ 列表项文字：body，text_primary，左对齐            │
│ 选中项：accent 文字 + 左侧 check 图标            │
│ Hover：hover_overlay 背景                        │
└──────────────────────────────────────────────────┘

文字截断规则：
- 禁止文字被截断（下拉项文字完整显示）
- 面板宽度不足时：缩短触发器宽度，而不是截断文字
- 选项文字过长时：选项文字最多 24 字符，超出则简化
```

#### 滑块规范 (Slider)

```
高度: GAP_SMALL×1.5（约 6px）
轨道: surface, radius=轨道高/2
填充: accent gradient（左→右）
滑块: 直径=轨道高×3，bg=white，shadow=shadow_sm
拖动: 滑块 scale(1.2)，accent 光晕
值标签: 滑块上方 small + text_secondary（可选）
```

### 6.5 输入框

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

### 6.6 下拉框 (Dropdown)

```
  Default:   bg=surface, border=1px border, height=LP_ROW_H, radius=radius_md
  Open:      border=accent, chevron-down 旋转 180° (200ms)
  下拉面板:  bg=panel, shadow=shadow_lg, radius=radius_lg
  下拉项:    height=LP_ROW_H, hover=hover_overlay, 左侧图标 + 文字
  选中项:    accent 文字 + 左侧 check 图标
```

### 6.7 开关 (Switch/Toggle)

```
  尺寸: 宽=父容器宽×5%(min 36), 高=宽×56%, 圆角=高/2
  Matcha:
    Off:  bg=#373C55, thumb=#8B92A8
    On:   bg=accent(#3DD68C), thumb=white
  Peachy:
    Off:  bg=#E0D0C0, thumb=#B8A089
    On:   bg=accent(#FF7F50), thumb=white
  Mochi:
    Off:  bg=#D6CFC4, thumb=#B0A394
    On:   bg=accent(#A67B5B), thumb=white
  动画: 200ms ease-out 滑动 + thumb 弹性回弹
```

### 6.8 复选框 (Checkbox)

```
  尺寸: ICON_SMALL, radius=radius_sm
  Off:   bg=surface, border=1px border_strong
  On:    bg=accent, 内部 check 图标 (ICON_SMALL×60%, white)
  动画:  150ms scale(0.8→1.0) + check 画线动画
```

### 6.9 进度条

```
  背景: surface, 填充: accent gradient, height=GAP_SMALL×1.5, radius=GAP_SMALL×1.5/2
  动画: 填充段平滑过渡 300ms
  不确定: accent 色 shimmer, 1.5s 循环
  百分比: 右侧 small + text_secondary
```

### 6.10 加载指示器

```
  脉冲圆点: 3 个 GAP_SMALL 圆点, accent 色, 依次脉冲 1.2s 循环
  旋转圆环: ICON_TINY, 2px 描边, accent 色, 0.8s 线性循环
  骨架屏:   raised 色 shimmer, 1.5s 循环
```

### 6.11 卡片 (Card)

```
  Default:   bg=panel, radius=radius_lg, padding=GAP×2
  Hover:     bg=raised, shadow=shadow_sm
  Selected:  border-left=GAP_SMALL×0.5 accent
  Peachy:    radius=radius_2xl
```

### 6.12 列表项 (List Item)

```
  Height: LP_ROW_H (紧凑) / LP_ROW_H×1.375 (标准), padding: 0 GAP×2
  Hover:     bg=hover_overlay
  Selected:  bg=accent_subtle, 左侧 GAP_SMALL×0.5 accent 竖线
  Font 标题: body, 副标: small + text_secondary
  图标:     左侧 icon_small, 右侧 chevron-right (可选)
```

### 6.13 Tooltip

```
  圆角: radius_md, padding: GAP_SMALL GAP×2, bg=raised, shadow=shadow_md
  触发: hover 500ms 延迟
  消失: 移开即消失, 200ms fade-out
  最大宽度: 窗口宽×22%(任务详情) / 窗口宽×14%(通用)
  图标:     可选左侧小图标
```

### 6.14 上下文菜单

```
  bg=panel, radius=radius_lg, shadow=shadow_lg
  item height=LP_ROW_H, hover=hover_overlay
  每项: 左侧图标(ICON_TINY) + 文字 + 右侧快捷键
  危险项: text=danger + icon=danger
  分隔线: divider 色 1px
```

### 6.15 Tab 切换

```
  Height=LP_ROW_H×1.5
  Active:   text=accent, 底部 2px accent 下划线
  Inactive: text=text_secondary, hover=text_primary
  动画:     下划线 200ms 滑动
  图标:     可选左侧小图标
```

### 6.16 分隔条 (Splitter)

```
  宽: SPLITTER_W
  Default:   bg=surface
  Hover:     bg=accent, opacity=0.3, cursor=col-resize
  Dragging:  bg=accent, opacity=0.6
```

### 6.17 步骤指示器

```
  ●──●──○──○──○──○  Step 1/6
  已完成: ● accent ICON_SMALL×50% + check 图标(ICON_TINY×50%)
  当前:   ● accent ICON_SMALL×60% + glow + pulse
  未完成: ○ border_strong ICON_SMALL×50%
  连接线: 2px, 已完成段=accent, 未完成段=border
```

### 6.18 状态胶囊 (Status Badge)

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

> **⚠️ UI 章节写作规范（2026-04-25）：** 每个 UI 模块须按以下结构编写，三者缺一不可：
> 1. **布局框图**（ASCII 图）：展示 UI 元素的二维排布、尺寸、分层关系
> 2. **操作流程图**（ASCII 图）：展示用户操作路径和界面状态转换，不含函数名
> 3. **文字描述**（分层 prose）：对布局框图和操作流程做详细的分层说明，包括每层的功能、尺寸、间距、行为
> 每个模块须按此规范反复优化 **3 轮**（Round 1 → Round 2 → Round 3），方可视为完成。

---

### UI-01: 启动自检（Boot Check）

**窗口尺寸：** 与主界面一致（1280×800 logical @200% = 2560×1600 physical），非独占全屏。

**功能编号：** SC-01 | 优先级：P0

---

#### 9 项检查列表（BootCheck 模块）

|| # | 检查项 | 检测方式 | 通过条件 | 失败说明 |
|---|-----|--------|---------|---------|---------|
| 1 | Node.js | `node --version` | ≥ 22.14.0 | Node.js 版本过低或未安装 |
| 2 | npm | `npm --version` | 任意版本返回 | npm 不可用 |
| 3 | Network | HTTP GET google.com | 返回 200 | 网络不可达 |
| 4 | OpenClaw | `openclaw --version` | 任意版本返回 | OpenClaw 未安装 |
| 5 | Gateway | HTTP GET 127.0.0.1:18789 | 返回 200 | Gateway 未运行 |
| 6 | Hermes | Agent 健康检查 API | 返回 200 | Hermes Agent 不可用 |
| 7 | Claude Code CLI | `claude --version` | 任意版本返回 | Claude Code CLI 未安装 |
| 8 | Disk Space | `GetDiskFreeSpaceExW(C:\)` | 可用空间 > 1GB | 磁盘空间不足 |
| 9 | Port 18789 | `bind()` 测试 | 端口可用 | 端口 18789 被占用 |

---

**布局（logical px）：**

```
┌──────────────────────────────────────────────────────────────────┐
│  🧄 AnyClaw  │  AnyClaw Boot Check                    │  [—][✕] │  ← HEADER=30px (4%)
├──────────────────────────────────────────────────────────────────┤
│                                                              │
│                        检测项 9 项                              │
│                                                              │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  ● Node.js           v22.14.0           ● OK         │   │  ← LV_PCT(86), h=500px
│  │  ● npm              v10.8.0            ● OK         │   │  ← pad=24px, row_h=40, gap=4px
│  │  ● Network          openrouter.ai        ● OK         │   │  ← 列1=232px, 列2=268px, 列3=72px
│  │  ● OpenClaw         v2.1.0              ● OK         │   │
│  │  ● Gateway          127.0.0.1:18789    ● OK         │   │
│  │  ● Hermes           running              ● OK         │   │
│  │  ● Claude Code CLI  found                ● OK         │   │
│  │  ● Disk Space       85 GB free          ● OK         │   │
│  │  ● Port 18789       available            ● OK         │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                              │
│  ● = 状态徽标  OK(绿) / Err(红) / N/A(灰)                    │
│  ◐ = 检测中 spinner                                          │
│                                                              │
│            [ 向导 ]                  [ 下一步 ]               │  ← FOOTER=56px，始终显示
│                                                              │
└──────────────────────────────────────────────────────────────────┘
```

**操作流程图：**

```
[启动 AnyClaw]
       │
       ▼
┌─────────────────────────────┐
│   显示 Boot Check 页面       │
│   所有检测行初始状态 = ◐     │  ← spinner（检测中）
│   页面标题: AnyClaw Boot Check│
└─────────────────────────────┘
       │
       ▼  （9 项并行/串行检测）
┌─────────────────────────────┐
│   逐项完成 → 逐行刷新状态     │
│   可能的最终状态:             │
│   ◐ → ● OK (绿)              │  ← 该项检测通过
│   ◐ → ● Err (红)            │  ← 该项检测失败
│   ◐ → ● N/A (灰)            │  ← 该项无法检测
└─────────────────────────────┘
       │
       ├─── 存在任意 Err ───────────────────────────┐
       │                                            │
       ▼                                            ▼
┌─────────────────────┐              ┌─────────────────────┐
│  两个按钮均可用:     │              │  [向导]   [下一步]  │
│  [向导] = 次要样式  │              │   点击 → 进入向导   │
│  [下一步] = 强调样式│              └─────────────────────┘
└─────────────────────┘
       │
       ├─── 全 OK + wizard_completed=false ────────┐
       │                                            │
       ▼                                            ▼
┌─────────────────────┐              ┌─────────────────────┐
│  等待 2.5s          │              │  自动进入首次启动向导│
│  [下一步] 可点击     │              │  (mode = wizard)   │
│  点击 → 立即跳过等待 │              └─────────────────────┘
└─────────────────────┘
       │
       ├─── 全 OK + wizard_completed=true ─────────┐
       │                                            │
       ▼                                            ▼
┌─────────────────────┐              ┌─────────────────────┐
│  等待 2.5s          │              │  自动进入主界面      │
│  [下一步] 可点击     │              │  (mode = main)     │
│  点击 → 立即跳过等待 │              └─────────────────────┘
└─────────────────────┘
       │
       └─── 仅 N/A（无 Err）────────────────────────┐
                                                  ▼
                               ┌─────────────────────┐
                               │  直接进入主界面      │
                               │  标记为可复检项      │
                               │  (mode = main)      │
                               └─────────────────────┘
```

**布局规范（文字描述）：**

**整体分层结构（从上到下共 4 层）：**

1. **顶部栏（HEADER，30px，占窗口高 4%）：** 最上方一条细横条区域。左侧显示品牌图标 🧄 + 应用名称 "AnyClaw"，中间显示当前页面标题 "AnyClaw Boot Check"，右侧显示两个窗口控制按钮：最小化 [—] 和关闭 [✕]。顶部栏底部有一条分隔线（1px 宽）将标题栏与下方内容区分开。

2. **内容区（占顶部栏下方剩余全部高度，约 750px）：** 内容区内部没有任何背景色或边框，内部垂直居中放置检测列表 panel，panel 本身相对内容区水平居中（两边各 7% margin）。

3. **检测列表 panel（宽 LV_PCT(86)，高 500px）：** panel 是一个带背景色和圆角的矩形容器。宽度为窗口宽度的 86%，意味着 panel 左边框距左边缘 7% 窗口宽，右边框距右边缘 7% 窗口宽。panel 内部有 24px 的均匀内边距（上右下左四边相同）。panel 内部从上到下排列 9 行检测项，第 1 行距 panel 顶部 24px，最后一行距 panel 底部 24px。每行高度 40px，行与行之间有 4px 垂直间距（gap），因此 9 行总高为 9×40 + 8×4 = 392px，加上上下各 24px 内边距 = 440px，panel 总高 500px，内含 60px 额外垂直空间使内容不拥挤。

4. **Footer 按钮区（56px 高）：** 位于 panel 下方，按钮区内部水平排列两个按钮：左侧"向导"按钮靠左边缘，右侧"下一步"按钮靠右边缘，两按钮之间留白。按钮垂直居中于 Footer 区域内。

**三列布局（每行内部从左到右 3 列）：**

- **列1 - 检查项名称（固定 232px 宽）：** 位于每行最左侧，显示检测项的名称文字（如 "Node.js"、"npm"），左对齐。
- **列2 - 版本/说明（固定 268px 宽）：** 位于列1右侧，显示该项的详细状态或版本号（如 "v22.14.0"、"running"、"85 GB free"），左对齐。
- **列3 - 状态徽标（固定 72px 宽）：** 位于每行最右侧，右侧对齐。内部左侧有 20px 间距，徽标内容为彩色圆点 ● 配合状态文字（OK/Err/N/A）。

**状态徽标样式：**
- `● OK`：绿色实心圆 + 绿色文字，表示检测通过
- `● Err`：红色实心圆 + 红色文字，表示检测失败
- `● N/A`：灰色实心圆 + 灰色文字，表示无法检测（Unknown）
- `◐ spinner`：蓝灰色旋转图标，表示检测进行中

**Footer 按钮（两个，水平排列）：**
- 左侧"向导"按钮：secondary 按钮样式（次要外观），有 Fail 时文字高亮
- 右侧"下一步"按钮：accent 按钮样式（强调外观），有 Fail 时文字高亮
- 按钮高度 40px，宽度根据文字自适应，两按钮在 Footer 区域水平两端对齐分布

---

#### 按钮

| 按钮 | 文字 | 说明 |
|------|------|------|
| 向导（左侧） | 向导 | secondary 样式，Fail 时高亮 |
| 下一步（右侧） | 下一步 | accent 样式，Fail 时高亮 |

**显示条件：** 始终显示（全 Pass/有 Fail/仅 N/A 均显示）。全 Pass 时"下一步"可用（点击跳过 2.5s 等待）；有 Fail 时两个按钮均可用。

---

#### 分流规则

| 结果 | 行为 |
|------|------|
| 全 Pass | 停留 >= 2.5s 后自动进入（首次安装→向导；已安装→主界面）；点击"下一步"立即进入 |
| 有 Fail | 停留页面，两个按钮均可用 |
| 仅 N/A 无 Fail | 直接进入主界面，标记为可复检项 |

**路由时序：**

```
Boot Check 完成
  ├── 存在任意 Err
  │     └── 停留页面，显示[向导]+[下一步]
  ├── 全 OK + wizard_completed=false
  │     └── 等待 2.5s → 自动进入首次启动向导
  ├── 全 OK + wizard_completed=true
  │     └── 等待 2.5s → 自动进入主界面
  └── 仅 N/A（无 Err）+ 任意 wizard_completed
        └── 直接进入主界面，标记可复检项
```

---

#### 检测时序

1. 启动 → 显示"正在执行启动检测..."（所有行 spinner ◐）
2. 逐项完成 → 逐行刷新为 OK/Err/N/A
3. 全部完成 → 按分流规则处理

---

#### PRD/Design 偏差说明

| 偏差项 | 原 PRD/Design 定义 | 修订后定义 | 理由 |
|--------|------------------|----------|------|
| 状态文字 | Pass/Fail/Unknown | OK/Err/N/A | Badge 宽度 72px，原文 4-7 字会溢出 |
| Panel 高度 | 428px | 500px | 额外空间让内容更舒展 |
| 三列宽度 | 160/flex/64px | 232/268/72px | 版本信息需要更多空间 |
| Footer 高度 | 35px | 56px | 按钮最大 56px，35px 会截断 |
| Footer 按钮 | 仅 Fail 时显示 | 始终显示 | 全 Pass 有 2.5s 等待，始终显示允许跳过 |
| 按钮文字 | 进入主界面（受限）/ 进入向导修复 | 向导 / 下一步 | 简化文字避免截断 |

---

### UI-02: 已运行提示（实例互斥）

**功能编号：** INST-01 | 优先级：P0

**概述：** UI-02 本身不渲染任何界面元素——程序启动时检测到已有实例运行，直接激活旧实例窗口并静默退出。以下为完整的操作流程和文字描述。

---

**操作流程图：**

```
[用户双击 AnyClaw 图标]  ──→  新实例启动
                                    │
                                    ▼
                    ┌───────────────────────────────┐
                    │  检测到已有实例正在运行          │
                    │  （通过 Mutex 互斥体判断）       │
                    └───────────────────────────────┘
                                    │
                                    ▼
                    ┌───────────────────────────────┐
                    │  在任务栏找到已有实例窗口        │
                    │  并将其窗口状态恢复             │
                    │  （从最小化/隐藏 → 正常显示）    │
                    └───────────────────────────────┘
                                    │
                                    ▼
                    ┌───────────────────────────────┐
                    │  已有实例窗口获得焦点           │
                    │  任务栏图标闪烁（提示用户）     │
                    └───────────────────────────────┘
                                    │
                                    ▼
                    ┌───────────────────────────────┐
                    │  当前（新的）启动进程           │
                    │  静默退出（不弹任何对话框）      │
                    └───────────────────────────────┘
```

**与已有实例窗口的交互细节：**

```
用户看到：已有 AnyClaw 窗口出现在前台，任务栏图标闪烁
用户操作：无需任何操作，新实例已自动退出
用户感知：点击图标 → 已有窗口弹出 → 感觉"程序已运行"
```

**兜底行为：** 若极端情况下找不到已有实例的窗口句柄，程序静默退出（不弹 MessageBox），并在日志中记录警告。

---

**文字描述：**

**触发条件：** 启动 AnyClaw 时，系统 Mutex（命名：`AnyClaw_LVGL_SingleInstance_Mutex_v2.0`，仅 ASCII，版本号嵌入）已存在，说明有其他实例正在运行。

**第一步——窗口恢复：** 尝试在任务栏中找到已有实例的窗口（按优先级尝试三个窗口标题：`AnyClaw LVGL v2.0 - Desktop Manager` → `AnyClaw LVGL` → `AnyClaw`），找到后调用系统 API 将窗口从最小化或隐藏状态恢复为正常显示（SW_RESTORE）。

**第二步——窗口激活：** 调用系统 API 将已恢复的窗口提升到前台（SetForegroundWindow），并使任务栏图标闪烁（FlashWindow），吸引用户注意。

**第三步——静默退出：** 新的启动进程直接 return 0，进程终止，不弹出任何对话框，不显示任何 UI。

**Mutex 命名规范：** `AnyClaw_LVGL_SingleInstance_Mutex_v2.0`，仅 ASCII，包含版本号便于大版本升级后并行运行。

**与 PRD §2.2 的偏差：** PRD 原定"弹出原生 MessageBox 提示，退出程序"，实际为窗口激活 + 静默退出。理由：MessageBox 在后台弹出时用户体验类似 freeze，实际用户会以为程序卡死。

---

### UI-03: 启动错误阻断（SelfCheck 阻断弹窗）

**功能编号：** SC-01（P0）| 与 UI-01 关联：UI-01=BootCheck（非阻断），UI-03=SelfCheck 阻断

---

#### 触发条件

SelfCheck（快速启动检查，4项）在 BootCheck 之前运行。任意关键项失败即触发 UI-03 阻断弹窗，阻止启动。

| # | 检查项 | 检测方式 | 阻断条件 | 阻断说明 |
|---|-------|---------|---------|---------|
| 1 | Node.js | `node --version` | 命令失败或版本 < 22.14 | Node.js 是运行时基础，缺失则无法启动任何 Agent |
| 2 | npm | `npm --version` | 命令失败 | npm 缺失或损坏 |
| 3 | 网络连通性 | HTTP GET google.com | 超时/失败 | 有网时所有 Agent 依赖云端模型，离线包缺失则无法继续 |
| 4 | 配置目录 | `%USERPROFILE%\.openclaw` 写权限 | 目录不可写 | 配置无法持久化，程序无法正常运行 |

**阻断级别判断：**

| 场景 | 行为 |
|------|------|
| Node.js 缺失 + bundled/ 有离线包 | 自动静默安装便携版 → 继续 BootCheck |
| Node.js 缺失 + bundled/ 无离线包 | 弹出 UI-03 阻断弹窗 |
| npm 失败 | 弹出 UI-03 阻断弹窗 |
| 无网络 + bundled/ 有 openclaw.tgz | 自动离线安装 OpenClaw → 继续 |
| 无网络 + bundled/ 无离线包 | 弹出 UI-03 阻断弹窗 |
| 配置目录不可写 | 弹出 UI-03 阻断弹窗 |

---

#### 弹窗规格

**类型：** LVGL 模态弹窗（LV_MODAL），完全阻断启动流程。弹窗居中显示，覆盖整个窗口，用户无法点击弹窗以外的任何元素。

**弹窗框图：**

```
┌──────────────────────────────────────────────┐
│                                              │
│              ⚠ 检测到问题                    │  ← 标题行：图标+18px粗体
│                                              │
│  ┌────────────────────────────────────────┐  │
│  │                                        │  │  ← 失败项列表容器（带边框）
│  │  ● Node.js: 未检测到                  │  │    失败项：红色圆点+名称
│  │      原因: 缺少 node.exe              │  │    缩进灰色原因说明
│  │                                        │  │
│  │  ● 网络: 无法连接                       │  │
│  │      原因: 无可用网络                 │  │
│  │                                        │  │
│  └────────────────────────────────────────┘  │
│                                              │
│  请修复上述问题后重新启动应用。                │  ← 14px灰色说明文字
│                                              │
│           [ 退出 ]                           │  ← 居中按钮 120×40px
│                                              │
└──────────────────────────────────────────────┘
       ← 弹窗宽度480px，水平居中 →
```

**操作流程图：**

```
[程序启动]
       │
       ▼
SelfCheck 快速检测（4 项）
       │
       ├─── 全部通过 ────────────────────────────→ 继续进入 BootCheck（UI-01）
       │
       └─── 任意关键项失败
                 │
                 ▼
        ┌─────────────────────────────┐
        │   弹出 UI-03 阻断弹窗        │
        │   标题: ⚠ 检测到问题        │
        │   内容: 失败项列表 + 原因    │
        │   按钮: [ 退出 ]            │
        └─────────────────────────────┘
                 │
                 ▼
         用户点击 [ 退出 ]
                 │
                 ▼
         程序调用 exit(0)
         进程终止，窗口关闭

用户操作选项:
  - 只能点击 [退出]，无法点击弹窗外部
  - 无重试选项（环境问题需用户自行修复）
  - 无法跳过此界面
```

**布局结构（从上到下共 5 层）：**

1. **弹窗容器（宽 480px，内边距 32px）：** 弹窗固定宽度 480px，高度随内容自动撑开。容器内部四周有均匀的 32px 内边距。容器本身水平居中于父窗口。

2. **警告图标 + 标题行：** 顶部放置警告图标 ⚠，紧挨着标题文字 "检测到问题"（18px 粗体）。图标与标题文字在同一行，图标在左，文字在右。标题行与下方的失败项列表之间有 24px 垂直间距。

3. **失败项列表容器（带边框矩形）：** 一个带边框的内层区域，存放所有检测失败的条目。该容器有 1px 宽的边框线，容器内部有 16px 内边距。每个失败项的样式为：
   - 第一行：红色圆点图标 ● + 检测项名称（如 "Node.js"），文字 14px，黑色或深灰色
   - 第二行：缩进的灰色小字，书写失败原因（如 "未检测到"、"缺少 node.exe"），文字 14px，灰色
   - 每个失败项之间垂直间距 12px

4. **说明文字：** 位于失败项列表容器下方，内容为 "请修复上述问题后重新启动应用。"，文字 14px，灰色。与上方列表容器有 24px 垂直间距。

5. **按钮区：** 底部居中放置一个 "退出" 按钮，尺寸 120px 宽 × 40px 高，圆角样式。按钮与上方说明文字有 24px 垂直间距。按钮文字为 "退出"，居中显示。

**按钮行为：** 点击"退出"→ `exit(0)` 关闭程序。无其他选项（用户必须修复环境后才能启动）。

---

#### 与 UI-01 的关系

```
启动流程：

main()
  │
  ▼
SelfCheck（4项，快速）
  │
  ├── 全部通过 → 继续 BootCheck（UI-01）
  │
  └── 任意关键项失败 → 弹出 UI-03 阻断弹窗
                         └── 用户点"退出" → exit(0)
```

- **UI-01（BootCheck）：** 非阻断，显示 9 项环境检测结果，有 Fail 也可进入主界面（受限）或向导
- **UI-03（SelfCheck 阻断）：** 阻断启动，SelfCheck 失败时弹出，必须修复环境才能继续

---

#### PRD/Design 偏差说明

| 偏差项 | 原 Design 定义 | 修订后定义 |
|--------|--------------|----------|
| 内容单薄 | 仅一句话描述 | 展开完整规格：触发条件、布局、按钮行为、与 UI-01 关系 |


### UI-04: 首次启动向导 — 整体框架

**窗口尺寸：** 全窗口覆盖（尺寸与主界面一致，`WIN_W × WIN_H`，非独占全屏），以 LVGL 模态覆盖层形式展示。

**功能编号：** WIZ-01 | 优先级：P1

---

#### 布局框图

```
┌──────────────────────────────────────────────────────────────────────────────┐
│  🧄 AnyClaw Setup                                          [—] [✕]          │  ← 顶部栏 56px
├──────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ● ─────── ● ─────── ● ─────── ○                                        │  ← 步骤条 ~60px
│  语言      环境     模型      完成                                            │
│                                                                              │
├──────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ┌──────────────────────────────────────────────────────────────────────┐  │
│  │                                                                     │  │
│  │                      当前步骤内容区                                   │  │  ← 内容区（弹性高度）
│  │                    （step 0~3 各自填充）                             │  │
│  │                                                                     │  │
│  └──────────────────────────────────────────────────────────────────────┘  │
│                                                                              │
├──────────────────────────────────────────────────────────────────────────────┤
│  [ 退出 ]                                              [ 上一步 ]  [ 下一步 ▶ ] │  ← 底部按钮栏 ~56px
└──────────────────────────────────────────────────────────────────────────────┘
```

#### 操作流程图

```
[BootCheck 全部 OK 后]
       │
       ▼
    显示向导模态覆盖层
       │
       ▼
    ┌─────────────────────────┐
    │   Step 0: 语言选择        │
    │   [中文]  [English]       │
    └───────────┬───────────────┘
                │
         用户选择语言
                │
                ▼
    ┌─────────────────────────┐
    │   Step 1: 环境就绪      │
    │   自动检测 + 一键修复    │
    │   [自动修复] [稍后手动]  │
    └───────────┬───────────────┘
                │
         环境检测完成 / 用户选择跳过
                │
                ▼
    ┌─────────────────────────┐
    │   Step 2: 模型 & API Key │
    │   模型下拉 + Key 输入     │
    │   [下一步] (Key 为空则禁用)│
    └───────────┬───────────────┘
                │
         API Key 验证通过
                │
                ▼
    ┌─────────────────────────┐
    │   Step 3: 完成页        │
    │   [进入主界面]           │
    │   + 阶段 B 推荐卡        │
    └───────────┬───────────────┘
                │
         用户点击 [进入主界面]
                │
                ▼
         向导关闭 → 进入主界面
         标记 wizard_completed=true
```

#### 文字描述（分层结构）

**整体分层（从上到下共 4 层）：**

1. **顶部栏（Header，56px）：** 最上方一条细横条区域，高度 56px，背景色为 panel 色。左侧显示品牌图标 🧄 + 标题 "AnyClaw Setup"，右侧显示窗口控制按钮：最小化 [—] 和关闭 [✕]。关闭按钮行为：询问用户是否确认退出向导（退出 → 整个应用退出）。

2. **步骤条（Step Bar，~60px）：** 位于顶部栏下方。步骤条内部水平排列 4 个步骤圆点（●/○），相邻圆点之间用连接线（───）相连。步骤条下方显示每个步骤的短名称（语言/环境/模型/完成）。当前步骤的圆点为实心 accent 色并带 glow pulse；已完成步骤为实心 accent 色加勾选图标；未到步骤为空心圆形 border 色。步骤条不响应用户点击（不可跳转，只能依次前进后退）。

3. **内容区（弹性高度）：** 步骤条下方、底部按钮栏上方之间的剩余空间。每个步骤（step 0~3）在同一块内容区中渲染，切换步骤时内容区整体替换（cross-fade 200ms）。内容区内部有 24px 的均匀内边距。

4. **底部按钮栏（56px）：** 位于最下方，高度 56px。左侧显示 [退出] 按钮（secondary 样式），右侧水平排列 [上一步] 和 [下一步 ▶] 两个按钮（前者 secondary，后者 accent/强调样式）。[上一步] 在 step 0 时隐藏（不可用）；[下一步] 在特定条件下禁用（如 step 2 的 API Key 未填写时）。

**按钮行为规则：**
- **[退出]**（左侧 secondary 按钮）：点击 → 弹出确认对话框「确定退出向导吗？应用将关闭」，确认 → `exit(0)`
- **[上一步]**：回到上一个步骤（step > 0 时可见，step 0 时隐藏）
- **[下一步]**：前进到下一个步骤（step < 3 且条件满足时可用；step 2 时 API Key 验证通过才可前进）
- **关闭按钮 [✕]**（顶部栏右侧）：点击 → 弹出确认对话框「确定退出向导吗？应用将关闭」，确认 → `exit(0)`（整个应用退出）
- **最小化按钮 [—]**（顶部栏右侧）：点击 → 将向导窗口最小化到任务栏

---

### UI-04-Step0: 语言选择

**所属向导步骤：** Phase A Step 0（共 4 步）

---

#### 布局框图

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                        请选择你的语言                                          │
│                      Please select your language                               │
│                                                                              │
│        ┌─────────────────────────┐    ┌─────────────────────────┐           │
│        │                         │    │                         │           │
│        │         中文            │    │        English          │           │
│        │                         │    │                         │           │
│        │      🇨🇳 简体中文        │    │      🇺🇸 English        │           │
│        │                         │    │                         │           │
│        └─────────────────────────┘    └─────────────────────────┘           │
│                                                                              │
│                        ○ 请选择一项以继续                                       │
│                                                                              │
└──────────────────────────────────────────────────────────────────────────────┘
```

#### 操作流程图

```
[进入 Step 0]
       │
       ▼
    显示两个语言选项按钮
       │
       ├─── 用户点击 [中文] ──────────────────────────┐
       │                                              │
       │   选中状态: 边框高亮 + 背景色变深             │
       │   右侧提示文字更新为"已选择: 中文"             │
       │   [下一步] 按钮变为可用状态                   │
       │                                              │
       │   用户点击 [下一步] ──→ 进入 Step 1          │
       │                                              │
       ├─── 用户点击 [English] ───────────────────────┐
       │                                              │
       │   选中状态: 边框高亮 + 背景色变深             │
       │   右侧提示文字更新为"已选择: English"         │
       │   [下一步] 按钮变为可用状态                  │
       │                                              │
       │   用户点击 [下一步] ──→ 进入 Step 1          │
       │                                              │
       └─── 用户未选择，直接点击 [下一步] ──────────────► [下一步] 保持禁用
```

#### 文字描述

**布局结构（内容区内部从上到下共 3 层）：**

1. **标题区：** 内容区顶部居中放置两行文字。上行为中文标题「请选择你的语言」（18px，粗体），下行为英文副标题「Please select your language」（14px，text_secondary 颜色）。标题区下方有 32px 垂直间距。

2. **选项按钮区：** 标题区下方水平排列两个大按钮（语言选项卡片），两个按钮之间有 24px 水平间距，两按钮在内容区内部水平居中。每个按钮尺寸约 200px 宽 × 140px 高，圆角样式（12px 圆角）。按钮内部垂直居中排列：上方为语言图标（🇨🇳 或 🇺🇸，48px），中间为语言名称（「中文」或「English」），下方为英文副标签（「简体中文」或「English」）。按钮有三种状态：默认（panel 背景色 + border 边框）、hover（背景色加深 + 轻微放大 scale(1.02)）、选中（accent 边框 + accent_subtle 背景色）。

3. **提示区：** 选项按钮区下方放置一行提示文字「○ 请选择一项以继续」，文字 14px，text_secondary 颜色。当用户选中某语言后，文字变为「✓ 已选择: 中文」或「✓ 已选择: English」，颜色变为 success 色，[下一步] 按钮同步变为可用状态。

---

### UI-04-Step1: OpenClaw 检测与安装

**所属向导步骤：** Phase A Step 1（共 4 步）

---

#### 布局框图

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                        🔧 OpenClaw 检测与安装                                │
│                   检测运行环境，一键安装 OpenClaw                              │
│                                                                              │
│  ┌─────────────────────────────┐    ┌────────────────────────────────────┐ │
│  │  检测结果                    │    │  📺 检测与安装日志                    │ │
│  │                             │    │                                    │ │
│  │  ● Node.js   v22.14.0  ✅ │    │  > 正在检测 Node.js...              │ │
│  │  ● npm        v10.8.0   ✅ │    │  > Node.js 已就绪 ✓                 │ │
│  │  ● 网络连通     OK       ✅ │    │  > 正在检测 npm...                  │ │
│  │  ● Gateway  18789端口  ✅ │    │  > npm 已就绪 ✓                     │ │
│  │  ● OpenClaw 未安装    ⚠️ │    │  > 正在检测 OpenClaw...             │ │
│  │                             │    │  > 未检测到 OpenClaw                │ │
│  │                             │    │  > 正在初始化 Gateway...            │ │
│  └─────────────────────────────┘    └────────────────────────────────────┘ │
│                                                                              │
│           [ 从网络下载安装（推荐）]                                             │
│           [ 使用本地离线包 ]                                                   │
│           [ 稍后手动处理 ]                                                    │
│                                                                              │
└──────────────────────────────────────────────────────────────────────────────┘
```

#### 操作流程图

```
[进入 Step 1]
       │
       ▼
    自动开始 5 项并行检测（Node.js / npm / 网络 / Gateway / OpenClaw）
       │
       ▼
    ┌─────────────────────────┐
    │  检测结果实时更新到左面板  │
    │  日志实时输出到右面板      │
    └───────────┬───────────────┘
                │
         所有项检测完成
                │
         ├─── OpenClaw 已安装 + Gateway 运行中 ✅ ────────────────────────┐
         │                                                               │
         │   [下一步] 自动变为可用状态                                      │
         │   用户点击 [下一步] ──→ 进入 Step 2                            │
         │                                                               │
         ├─── OpenClaw 未安装 ⚠️ ─────────────────────────────────────┐
         │                                                               │
         │   显示三种安装选项按钮                                          │
         │                                                               │
         │   ├─── 用户点击 [从网络下载安装（推荐）] ──→ 在线安装流程 ──┐  │
         │   │                                                        │  │
         │   │   安装中: 进度显示在右面板日志                           │  │
         │   │   安装成功 ──→ [下一步] 可用                           │  │
         │   │   安装失败 ──→ 显示错误 + [重试]                        │  │
         │   │                                                        │  │
         │   └─── 用户点击 [使用本地离线包] ──→ 离线安装流程 ──────┘  │
         │                                                               │
         │   └─── 用户点击 [稍后手动处理] ──→ [下一步] 可用 ─────────┘  │
         │                                                               │
         └─── 用户点击 [上一步] ──→ 返回 Step 0 ──────────────────────┘
```

#### 文字描述

**布局结构（内容区内部从上到下共 4 层）：**

1. **标题区：** 内容区顶部居中放置标题「🔧 OpenClaw 检测与安装」（18px 粗体）和副标题「检测运行环境，一键安装 OpenClaw」（14px，text_secondary 颜色）。标题区下方有 24px 垂直间距。

2. **双面板区：** 标题区下方左右并排两个矩形容器面板，中间有 16px 间距，两个面板总宽度占内容区宽度的 90%，在内容区内部水平居中。
   - **左面板（检测结果）：** 宽度约为双面板总宽的 40%，内部从上到下排列 5 个检测项状态行。每行包含：状态图标（✅/⏳/⚠️/❌）+ 检测项名称 + 版本号或状态文字。检测项包括 Node.js、npm、网络连通性、Gateway 端口、OpenClaw。检测项之间垂直间距 12px。
   - **右面板（检测与安装日志）：** 宽度约为双面板总宽的 58%，内部为一个终端风格日志展示区（monospace 字体，surface 背景色），实时显示检测和安装过程的命令行输出。日志自动滚动到底部，支持选中复制。

3. **安装选项按钮区（条件显示）：** 仅在 OpenClaw 未安装时出现。双面板区下方水平排列三个按钮：「从网络下载安装（推荐）」（accent 样式，主要按钮）、「使用本地离线包」（secondary 样式）、「稍后手动处理」（secondary 样式，文字较小）。三按钮在内容区内部水平居中，间距 16px。

4. **按钮区：** OpenClaw 已安装时显示底部按钮：「下一步」（accent 样式）；OpenClaw 未安装时安装选项按钮本身充当底部操作区。[上一步] 始终在底部左侧可见（secondary 样式）。

**检测项状态语义：**
- `✅ 就绪`：绿色 check-circle + 版本号 + 「已就绪」
- `⏳ 检测中`：蓝色 spinner + 「检测中...」
- `⚠️ 需处理`：黄色 alert-circle + 「需处理」（如 OpenClaw 未安装）
- `❌ 失败可重试`：红色 x-circle + 错误摘要 + 「可重试」

**注意：** 本步骤与 PRD §2.4 中描述的"环境就绪"不完全对应——PRD 的环境就绪检测（Node/npm/OPENCLAW）在本步骤中与 OpenClaw 检测合并展示，而非独立步骤。

---

### UI-06-Step2: 模型与 API Key

**所属向导步骤：** Phase A Step 2（共 4 步）

---

#### 布局框图

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                        🤖 选择你的 AI 模型                                    │
│                      连接你的 API Key 即可开始聊天                              │
│                                                                              │
│  ┌──────────────────────────────────────────────────────────────────────┐  │
│  │  模型列表                                                             │  │
│  │                                                                       │  │
│  │   ┌────────────────────────────────────────────────────────────┐   │  │
│  │   │  ● MiniMax Text   MiniMax   v2.8   ★ 4.8                  │   │  │
│  │   └────────────────────────────────────────────────────────────┘   │  │
│  │   ┌────────────────────────────────────────────────────────────┐   │  │
│  │   │    Claude 4 Sonnet   Anthropic  v4.0   ★ 4.9               │   │  │
│  │   └────────────────────────────────────────────────────────────┘   │  │
│  │   ┌────────────────────────────────────────────────────────────┐   │  │
│  │   │    GPT-4o            OpenAI    v4.0   ★ 4.7               │   │  │
│  │   └────────────────────────────────────────────────────────────┘   │  │
│  │                                                                       │  │
│  │  API Key（可选）                                                     │  │
│  │   ┌────────────────────────────────────────────────────────────┐   │  │
│  │   │  sk-xxx...xxxx                │   │  │                   │   │  │
│  │   └────────────────────────────────────────────────────────────┘   │  │
│  │   ○ Key 将安全存储，仅本地使用                                         │  │
│  │                                                                       │  │
│  └──────────────────────────────────────────────────────────────────────┘  │
│                                                                              │
└──────────────────────────────────────────────────────────────────────────────┘
```

#### 操作流程图

```
[进入 Step 2]
       │
       ▼
    加载模型列表（默认选中第一项）
       │
       ▼
    ┌──────────────────────────────────────┐
    │  用户在模型列表中点击选中一个模型         │
    │  用户可在 API Key 输入框中填写 Key（可选）│
    └──────────────────────────────────────┘
                │
         用户点击 [下一步 >]
                │
                ▼
         保存选中的模型（写入配置）
         （API Key 在 Step 3 填写，不在本步骤验证）
                │
                ▼
         进入 Step 3（昵称 / 时区 / Leader 模式）
```

#### 文字描述

**布局结构（内容区内部从上到下共 4 层）：**

1. **标题区：** 内容区顶部居中放置两行文字。上行为主标题「🤖 选择你的 AI 模型」（18px 粗体），下行为副标题「连接你的 API Key 即可开始聊天」（14px，text_secondary 颜色）。标题区下方有 24px 垂直间距。

2. **模型列表区：** 标题区下方为一个垂直排列的模型选项卡片列表，每个卡片高度约 56px，卡片之间间距 12px。卡片内部包含：左侧圆形 Radio 选择圈（○ 未选中 / ● 选中）、模型名称、Provider 名称、版本号、星级评分（★）。默认第一项（推荐模型）处于选中状态（● + accent 背景色高亮）。用户点击任意卡片可切换选中项。

3. **API Key 输入区（可选）：** 模型列表区下方，有 24px 垂直间距。显示标签文字「API Key（可选）」（14px 粗体），下方为文本输入框（密码样式，字符显示为 ●），输入框高度 40px。输入框下方有一行小字提示「○ Key 将安全存储，仅本地使用」（14px，text_secondary 颜色）。本步骤不验证 Key，Key 可在 Step 3 完成后或主界面设置中填写。

4. **按钮区：** 底部按钮栏包含 [上一步]（secondary，左侧）和 [下一步 >]（accent，右侧）。[下一步 >] 始终可用（API Key 非必填，本步骤不验证 Key）。

---

### UI-09-Step3: 完成页（含昵称、时区、Leader 模式）

**所属向导步骤：** Phase A Step 3（共 4 步，向导最后一步）

---

#### 布局框图

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                        🎉 恭喜！设置完成                                      │
│                   最后一步：确认配置并进入 AnyClaw                             │
│                                                                              │
│  ┌──────────────────────────────────────────────────────────────────────┐  │
│  │  昵称                                                               │  │
│  │  ┌──────────────────────────────────────────────────────────────┐   │  │
│  │  │  你的昵称...                                                   │   │  │
│  │  └──────────────────────────────────────────────────────────────┘   │  │
│  │                                                                       │  │
│  │  时区                                                               │  │
│  │  ┌──────────────────────────────────────────────────────────────┐   │  │
│  │  │  Asia/Shanghai                                             ▼  │   │  │
│  │  └──────────────────────────────────────────────────────────────┘   │  │
│  │                                                                       │  │
│  │  Leader 模式                                                         │  │
│  │  ┌──────────────────────────────────────────────────────────────┐   │  │
│  │  │  ● 启用 Leader 模式（多 Agent 并行，推荐）                    │   │  │
│  │  │  ○ 跳过（单 Agent，仅 OpenClaw）                            │   │  │
│  │  └──────────────────────────────────────────────────────────────┘   │  │
│  │  Skip 将降级为单 Agent 模式（仅 OpenClaw）。                      │  │
│  │                                                                       │  │
│  │  进阶能力（非阻断，可后续在设置中配置）                              │  │
│  │  💾 本地模型   💬 IM 连接                                           │  │
│  └──────────────────────────────────────────────────────────────────────┘  │
│                                                                              │
│                        ╔═══════════════════════╗                             │
│                        ║    [ 开始使用 → ]     ║                             │
│                        ╚═══════════════════════╝                             │
│                                                                              │
└──────────────────────────────────────────────────────────────────────────────┘
```

#### 操作流程图

```
[进入 Step 3]
       │
       ▼
    加载配置摘要（语言、模型、API Key 等）
       │
       ▼
    ┌─────────────────────────┐
    │  昵称输入（可选，跳过显示 anonymous）                          │
    │  时区下拉（默认 Asia/Shanghai）                              │
    │  Leader 模式选择                                                │
    │  底部显示 [开始使用 →] 主按钮                                  │
    └───────────┬───────────────┘
                │
         用户点击 [开始使用 →]
                │
                ▼
    保存配置（语言 + 昵称 + 时区 + Leader 模式 + wizard_completed=true）
                │
                ▼
         关闭向导模态覆盖层
                │
                ▼
         进入主界面（Bot · Chat 模式）
```

#### 文字描述

**布局结构（内容区内部从上到下共 4 层）：**

1. **标题区：** 内容区顶部居中放置标题「🎉 恭喜！设置完成」（18px 粗体）和副标题「最后一步：确认配置并进入 AnyClaw」（14px，text_secondary 颜色）。标题区下方有 24px 垂直间距。

2. **配置表单区（可滚动）：** 标题区下方为一个垂直排列的表单区域，从上到下包含：
   - **昵称输入：** 标签文字「昵称」（14px），下方为文本输入框（占整行宽，高度 40px），placeholder 为「你的昵称...」，支持中英文输入，跳过则显示为 anonymous。
   - **时区下拉：** 标签文字「时区」（14px），下方为一个下拉选择框（占整行宽，高度 40px），默认值为「Asia/Shanghai」，展开显示时区列表供用户选择。
   - **Leader 模式单选：** 标签文字「Leader 模式」（14px），下方为一个单选选项列表，两个选项垂直排列：选项1「● 启用 Leader 模式（多 Agent 并行，推荐）」（默认选中，●），选项2「○ 跳过（单 Agent，仅 OpenClaw）」。选项下方有一行小字提示「Skip 将降级为单 Agent 模式（仅 OpenClaw）。」
   - **进阶能力提示行：** 表单区底部显示一行小字「进阶能力（非阻断，可后续在设置中配置）」以及两个图标文字「💾 本地模型   💬 IM 连接」，提示用户这些能力可后续在设置中配置。

3. **主按钮区：** 配置表单区下方放置一个大型居中按钮「开始使用 →」，按钮宽度 240px，高度 48px，accent 背景色，白色文字，按钮内文字水平垂直均居中。此按钮始终可用（昵称/时区均可跳过）。

4. **阶段 B 推荐卡片区（条件显示）：** 主按钮区下方显示一行小字提示「💾 本地模型   💬 IM 连接」，表示这些进阶能力可点击——但本步骤的主流程是点击「开始使用」直接进入主界面。

**完成后行为：** 用户点击「开始使用」→ 保存语言设置 + 昵称（跳过则 anonymous）+ 时区 + Leader 模式选择 → 标记 `wizard_completed=true`（写入配置文件）→ 销毁向导模态覆盖层 → 显示主界面（Bot · Chat 模式）。

---

### UI-07: 本地模型（阶段 B 进阶引导）

**优先级：** P2 | 状态：规划中

---

#### 布局框图

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                        💾 本地模型配置                                         │
│                     使用 Gemma 实现离线推理                                     │
│                                                                              │
│  ┌──────────────────────────────────────────────────────────────────────┐  │
│  │                                                                       │  │
│  │   [ ● 启用本地模型    ○ 禁用（使用云端） ]                              │  │
│  │                                                                       │  │
│  │   ─────────────────────────────────────────────────────────────     │  │
│  │                                                                       │  │
│  │   模型选择                                                             │  │
│  │   ┌────────────────────────────────────────────────────────────┐    │  │
│  │   │  Gemma 3 4B IT      Quantized Q4   ~2.1GB    [下载]         │    │  │
│  │   └────────────────────────────────────────────────────────────┘    │  │
│  │   ┌────────────────────────────────────────────────────────────┐    │  │
│  │   │  Gemma 3 2B IT      Quantized Q4   ~1.2GB    [下载]         │    │  │
│  │   └────────────────────────────────────────────────────────────┘    │  │
│  │                                                                       │  │
│  │   下载目录:  D:\Models\gemma-3-4b-it-q4                            │  │
│  │                                                                       │  │
│  └──────────────────────────────────────────────────────────────────────┘  │
│                                                                              │
│           [ 保存配置 ]                      [ 跳过此步骤 ]                    │
│                                                                              │
└──────────────────────────────────────────────────────────────────────────────┘
```

#### 操作流程图

```
[用户从完成页点击「本地模型」卡片 或 从设置页进入]
       │
       ▼
    显示本地模型配置页
       │
       ├─── 用户点击 [启用本地模型] ──────────────────────────────────┐
       │                                                              │
       │   启用 Switch 打开                                          │
       │   模型选择列表变为可交互状态                                  │
       │                                                              │
       │   ├─── 用户选择模型 + [下载] ──→ 开始下载 ──→ 进度显示 ──┐  │
       │   │                                                        │  │
       │   │   下载完成 → [保存配置] 可用                           │  │
       │   │                                                        │  │
       │   └─── 用户点击 [保存配置] ──→ 保存 + 返回 ─────────────┘  │
       │                                                              │
       ├─── 用户点击 [禁用（使用云端）] ──────────────────────────┐  │
       │                                                              │
       │   启用 Switch 关闭                                         │
       │   模型列表禁用                                              │
       │   用户点击 [跳过此步骤] ──→ 保存禁用状态 + 返回            │
       │                                                              │
       └─── 用户点击 [跳过此步骤] ──→ 直接返回完成页 ───────────────┘
```

#### 文字描述

**布局结构（内容区内部从上到下共 4 层）：**

1. **标题区：** 内容区顶部居中放置标题「💾 本地模型配置」（18px 粗体）和副标题「使用 Gemma 实现离线推理」（14px，text_secondary 颜色）。标题区下方有 24px 垂直间距。

2. **启用开关区：** 一个横向排列的 Switch 组件 + 标签文字。Switch 左侧为「启用本地模型」（accent 色文字），右侧为「禁用（使用云端）」。Switch 尺寸 48×24px，启用时滑块在右侧（accent 背景），禁用时滑块在左侧（border 色背景）。Switch 下方有一条水平分隔线（1px，border 颜色）。

3. **模型列表区（条件显示）：** 仅在启用本地模型时显示。一个垂直排列的模型卡片列表，每张卡片高度约 56px，包含：模型名称、量化方式（Q4）、预估大小（~2.1GB）、下载进度条（下载中时显示）、[下载] 按钮（未下载时）/ 状态文字（已下载时）。卡片之间间距 12px。

4. **按钮区：** 底部放置两个按钮：「保存配置」（accent 样式，启用本地模型后可用）和「跳过此步骤」（secondary 样式，始终可用）。两按钮左右两端对齐分布，间距 16px。

---

### UI-08: 连接 IM（阶段 B 进阶引导）

**优先级：** P2 | 状态：规划中

---

#### 布局框图

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                        💬 连接即时通讯                                         │
│                    让手机成为 AnyClaw 的远程控制器                               │
│                                                                              │
│  ┌──────────────────────────────────────────────────────────────────────┐  │
│  │                                                                       │  │
│  │  ┌─────────────────────────────────────────────────────────────────┐ │  │
│  │  │  🟦 飞书 (Feishu)              未连接              [配置]         │ │  │
│  │  └─────────────────────────────────────────────────────────────────┘ │  │
│  │  ┌─────────────────────────────────────────────────────────────────┐ │  │
│  │  │  🌀 Telegram               未连接              [配置]            │ │  │
│  │  └─────────────────────────────────────────────────────────────────┘ │  │
│  │  ┌─────────────────────────────────────────────────────────────────┐ │  │
│  │  │  🟣 Discord                未连接              [配置]            │ │  │
│  │  └─────────────────────────────────────────────────────────────────┘ │  │
│  │                                                                       │  │
│  └──────────────────────────────────────────────────────────────────────┘  │
│                                                                              │
│           [ 保存配置 ]                      [ 跳过此步骤 ]                    │
│                                                                              │
└──────────────────────────────────────────────────────────────────────────────┘
```

#### 操作流程图

```
[用户从完成页点击「IM 连接」卡片 或 从设置页进入]
       │
       ▼
    显示 IM 连接配置页（3 个渠道卡片列表）
       │
       ├─── 用户点击 [配置]（飞书）───────────────────────────────────┐
       │                                                              │
       │   展开飞书配置面板                                            │
       │   显示 AppID + App Secret 输入框                              │
       │   [连接] / [测试] 按钮                                       │
       │                                                              │
       │   ├─── 连接成功 ──→ 飞书卡片更新为「已连接」(绿色)           │
       │   │                                                        │
       │   └─── 连接失败 ──→ 显示错误原因 + [重试]                   │
       │                                                              │
       ├─── 用户点击 [配置]（Telegram）─────────────────────────────┐
       │   展开 Telegram 配置面板                                     │
       │   显示 Bot Token 输入框                                     │
       │   同上连接成功/失败流程                                      │
       │                                                              │
       ├─── 用户点击 [配置]（Discord）──────────────────────────────┐
       │   展开 Discord 配置面板                                      │
       │   显示 Bot Token 输入框                                      │
       │   同上连接成功/失败流程                                      │
       │                                                              │
       └─── 用户点击 [跳过此步骤] ──→ 保存已配置项 + 返回完成页 ───────┘
```

#### 文字描述

**布局结构（内容区内部从上到下共 3 层）：**

1. **标题区：** 内容区顶部居中放置标题「💬 连接即时通讯」（18px 粗体）和副标题「让手机成为 AnyClaw 的远程控制器」（14px，text_secondary 颜色）。标题区下方有 24px 垂直间距。

2. **渠道卡片列表区：** 标题区下方垂直排列 3 张渠道卡片（飞书/Telegram/Discord），卡片之间间距 12px。每张卡片高度约 56px，内部包含：渠道图标（🟦/🌀/🟣）+ 渠道名称 + 连接状态文字（未连接/连接中/已连接）+ [配置] 按钮。已连接时状态文字变为绿色「已连接 ✓」，[配置] 按钮变为「修改」。卡片有边框，连接成功时边框变为 success 色。

3. **按钮区：** 底部放置两个按钮：「保存配置」（accent 样式，至少有一个渠道连接成功时可用）和「跳过此步骤」（secondary 样式，始终可用）。两按钮左右分布，间距 16px。

**连接状态语义：**
- 未连接：灰色文字 + [配置] 按钮
- 连接中：蓝色 spinner + 「连接中...」
- 已连接：绿色文字 + 「已连接 ✓」+ [修改] 按钮
- 连接失败：红色文字 + 错误原因 + [重试] 按钮

---

### UI-10: 工作区模板选择

**优先级：** P1 | 状态：规划中

---

#### 布局框图

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                        📁 选择您的工作区                                        │
│                    为不同任务类型预设最佳 AI 行为模板                             │
│                                                                              │
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐          │
│  │                  │  │                  │  │                  │          │
│  │   🤖 通用助手     │  │   💻 开发者      │  │   ✍️ 写作者      │          │
│  │                  │  │                  │  │                  │          │
│  │   日常问答、       │  │   代码生成、     │  │   文章撰写、     │          │
│  │   信息查询、       │  │   代码审查、     │  │   文案创作、     │          │
│  │   创意建议        │  │   技术研究       │  │   翻译润色       │          │
│  │                  │  │                  │  │                  │          │
│  │     ○            │  │     ○            │  │     ○            │          │
│  └──────────────────┘  └──────────────────┘  └──────────────────┘          │
│  ┌──────────────────┐                                                       │
│  │                  │                                                       │
│  │   📊 数据分析     │                                                       │
│  │                  │                                                       │
│  │   数据处理、       │                                                       │
│  │   可视化、        │                                                       │
│  │   报告生成        │                                                       │
│  │                  │                                                       │
│  │     ○            │                                                       │
│  └──────────────────┘                                                       │
│                                                                              │
│  工作区路径:  D:\MyWorkSpace\AnyClaw                         [浏览]         │
│                                                                              │
│                        [ 创建工作区 ]                                          │
│                                                                              │
└──────────────────────────────────────────────────────────────────────────────┘
```

#### 操作流程图

```
[用户从主界面或设置页进入工作区选择页]
       │
       ▼
    显示 4 个工作区模板卡片（初始均未选中）
       │
       ├─── 用户点击某个模板卡片 ──────────────────────────────────────┐
       │                                                              │
       │   该卡片变为选中状态（● + accent 边框高亮）                    │
       │   其他卡片取消选中状态（○）                                    │
       │   用户可随时点击其他卡片切换选择                               │
       │                                                              │
       │   用户在路径输入框中填写/浏览选择目录                          │
       │                                                              │
       │   用户点击 [创建工作区] ──→ 创建目录 + 初始化配置              │
       │                                                              │
       ├─── 用户直接点击 [创建工作区]（未选择模板）──────────────────┐
       │   [创建工作区] 按钮保持禁用（必须有模板和路径才能创建）       │
       │                                                              │
       └─── 用户关闭页面 ──→ 不创建工作区，返回主界面 ──────────────────┘
```

#### 文字描述

**布局结构（内容区内部从上到下共 4 层）：**

1. **标题区：** 内容区顶部居中放置标题「📁 选择您的工作区」（18px 粗体）和副标题「为不同任务类型预设最佳 AI 行为模板」（14px，text_secondary 颜色）。标题区下方有 24px 垂直间距。

2. **模板卡片网格：** 标题区下方为一个 2×2 网格布局的模板卡片区（前排 3 张，后排 1 张居左）。每张卡片尺寸约 220px 宽 × 160px 高，圆角 12px，panel 背景色 + border 边框。卡片内部：顶部大图标（🤖/💻/✍️/📊，48px），中间为模板名称（16px 粗体），下方为 3 行简短描述文字（12px，text_secondary），底部右侧为选择指示圈（○ 未选中 / ● 选中）。选中卡片的边框变为 accent 色，背景变为 accent_subtle 色。

3. **路径输入区：** 模板卡片区下方，有 24px 垂直间距。左侧为一个文本输入框（只读，显示选定的工作区路径），右侧为 [浏览] 按钮（secondary 样式，点击弹出目录选择对话框）。

4. **按钮区：** 底部居中放置一个「创建工作区」按钮（accent 样式，宽 200px，高 44px）。该按钮在以下条件同时满足时可用：已选择 1 个模板 + 路径输入框非空。

**模板类型：**
- 🤖 通用助手：日常问答、信息查询、创意建议
- 💻 开发者：代码生成、代码审查、技术研究
- ✍️ 写作者：文章撰写、文案创作、翻译润色
- 📊 数据分析：数据处理、可视化、报告生成

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

||| Icon | 模块 | 含义 |
|------|------|------|
| 🦞 | Bot | AI 对话入口（Chat + Work） |
| ⚡ | Task | 任务调度中心（活跃/定时/历史） |
| 📁 | File | 文件 + 知识库 + Skills |

**底部（快捷入口）：**

||| Icon | 功能 | 说明 |
|------|------|------|
| [↔] | Chat/Work | 单按钮图标切换模式，位于底部 |
| ⚙ | Settings | 点击覆盖内容区显示设置 |

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

||| 左导航 | 标题栏显示 | 额外控件 |
|--------|-----------|---------|
| 🦞 Bot | `Bot · Chat` / `Bot · Work` | — |
| ⚡ Task | `Task` | [+] 新建任务 |
| 📁 File | `File` | — |
| ⚙ Settings | `Settings` | — |

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

### UI-04: Task 模块

Task 模块激活时，左侧导航高亮 Task，左侧面板（LEFT_PANEL_W）显示 Task 内容，右侧面板隐藏。

**布局：**
```
┌────┬─────────────────────────────────────────────────────────────────┐
│    │ 🧄 Task                                               [✕]      │
├────┼─────────────────────────────────────────────────────────────────┤
│ ⚡ │                                                                 │
│    │  ┌─ Task ───────────────────────────────────────────────┐        │
│    │  │  ● 进行中        curl -s https://api.example.com     │        │
│    │  │  ✓ 已完成        echo "done"                        │        │
│    │  │  ○ 队列中        sleep 5 && echo "waited"           │        │
│    │  └──────────────────────────────────────────────────────┘        │
│    │                                                                 │
│    │  状态：队列已刷新                          [新建任务]           │
└────┴─────────────────────────────────────────────────────────────────┘
```

**约束：**
- Task 内容容器（module_placeholder）父容器为 `left_panel`，宽度 ≤ `LEFT_PANEL_W`
- `left_panel` 宽度 = `LEFT_PANEL_W`，高度 = `PANEL_H`
- 内容区（右侧）完全隐藏，不占用渲染资源
- module_placeholder 在 Bot/Task/File 之间切换时，只切换内容，不改变窗口结构

**Task 内容结构：**

| 元素 | 说明 |
|------|------|
| module_tasks_panel | 任务列表容器，背景 surface 色，1px border |
| module_tasks_view | textarea，任务队列文本（进行中/已完成/队列中） |
| module_tasks_state | 状态标签，caption 字体，显示同步状态 |

### UI-05: File 模块

File 模块激活时，左侧导航高亮 File，左侧面板显示 File 内容，右侧面板隐藏。

**布局：**
```
┌────┬─────────────────────────────────────────────────────────────────┐
│    │ 🧄 File                                               [✕]      │
├────┼─────────────────────────────────────────────────────────────────┤
│ 📁 │                                                                 │
│    │  ┌─ File ───────────────────────────────────────────────┐        │
│    │  │  📁 D:/workspace/AnyClaw/...                        │        │
│    │  │  📄 ui_main.cpp                    2026-04-24        │        │
│    │  │  📄 PRD.md                        2026-04-24        │        │
│    │  └──────────────────────────────────────────────────────┘        │
│    │                                                                 │
│    │  状态：文件索引已更新                        [刷新] [上传]        │
└────┴─────────────────────────────────────────────────────────────────┘
```

**约束：**
- File 内容容器（module_files_panel）父容器为 `left_panel`，宽度 ≤ `LEFT_PANEL_W`
- `left_panel` 宽度 = `LEFT_PANEL_W`，高度 = `PANEL_H`
- 内容区（右侧）完全隐藏

**File 内容结构：**

| 元素 | 说明 |
|------|------|
| module_files_panel | 文件列表容器，背景 surface 色，1px border |
| module_files_view | textarea，文件路径 + 修改时间 |
| module_files_state | 状态标签，caption 字体，显示索引状态 |

**Bot / Task / File 布局对比：**

| 模块 | 左侧面板（LEFT_PANEL_W） | 右侧面板 |
|------|-----------|---------|
| Bot | Session/Cron 列表 或 StepCard | 控制行 + 输出区 + 输入区 |
| Task | Task 内容（module_placeholder 内部） | **隐藏** |
| File | File 内容（module_placeholder 内部） | **隐藏** |

> ⚠️ Task 和 File 的 module_placeholder 父容器必须为 `left_panel`，不是根容器（scr/pr）。否则内容会溢出左侧面板范围，覆盖整个主界面。

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

### UI-52 ~ UI-54: 三套主打主题

**主打主题：**

| 主题 | bg | panel | text | accent | 风格 |
|------|----|-------|------|--------|------|
| 🍵 Matcha v1 (默认) | #0F1117 | #1A1D27 | #E8ECF4 | #3DD68C | 薄荷绿深色 |
| 🍑 Peachy v2 | #FFF8F3 | #FFFFFF | #2D1B14 | #FF7F50 | 珊瑚橘暖色 |
| 🍡 Mochi v3 | #FAF6F0 | #FFFDF9 | #3D3226 | #A67B5B | 奶茶棕米白 |

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

### UI-14A: C2 统一输入与双结果视图

> 目标：用户不需要先理解 Chat/Work 差异，先输入即可得到正确反馈。

**布局规则：**
1. 输入区固定单入口（底部固定）。
2. 输出区按请求类型切换：
- Q&A：消息流视图。
- Task：消息摘要 + 执行轨迹双区视图。

**判定与反馈：**

| 请求类型 | 判定后 UI | 必显元素 |
|---------|-----------|----------|
| Q&A | 单列消息流 | 用户气泡 + AI 气泡 |
| Task | 双区视图 | 对话摘要、StepCard 轨迹、结果块 |

**交互约束：**
1. 请求提交后 200ms 内必须出现首个可见反馈（输入锁定或占位回复）。
2. Task 轨迹区在有步骤事件前显示骨架态，避免空白。
3. 显示层切换（Chat/Work）仅改变呈现，不中断任务执行。

**错误与降级：**
1. 类型判定失败时降级为 Q&A 视图，并提示“已按对话处理”。
2. 执行轨迹流中断时，保留已完成步骤并提供“继续执行/重试”。

**页面级验收（UI-14A）：**
1. 同一输入入口可覆盖问答与任务两类请求。
2. Task 请求全程可见进度，不出现无反馈时段。
3. Chat/Work 切换后同一请求上下文保持一致。

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
  > ⚠️ Voice 模式已在 v2.2.11 移除（Text|Voice 按钮已删除），语音功能作为独立模块待后续实现
```

### UI-KS: 键盘快捷键

> 需求 → PRD §6.4 键盘快捷键

全局快捷键，任意界面均有效（文本编辑中性质冲突的除外）。

#### 导航快捷键

| 快捷键 | 操作 | 说明 |
|--------|------|------|
| `Ctrl+1` | 切换到 Bot 模块 | 左导航选中 Bot |
| `Ctrl+2` | 切换到 Tasks 模块 | 左导航选中 Tasks |
| `Ctrl+3` | 切换到 Files 模块 | 左导航选中 Files |
| `Ctrl+Tab` | 循环切换导航模块 | Bot→Tasks→Files→Bot |
| `Ctrl+T` | 切换 Chat/Work 模式 | 仅 Bot 模块有效 |

#### 界面快捷键

| 快捷键 | 操作 | 说明 |
|--------|------|------|
| `Ctrl+,` | 打开设置 | 等同点击 Settings 按钮 |
| `Ctrl+Shift+T` | 循环切换主题 | Matcha→Peachy→Mochi→Matcha |
| `Ctrl+N` | 新建对话 | 清空输入框并聚焦（仅 Bot·Chat 模式） |

#### 窗口快捷键

| 快捷键 | 操作 | 说明 |
|--------|------|------|
| `Ctrl+M` | 最小化窗口 | 同标题栏最小化按钮 |
| `F11` | 最大化/还原 | 切换最大化状态 |
| `Ctrl+W` | 关闭到托盘 | 同标题栏关闭按钮（不退出进程） |

#### 文本编辑快捷键（已有）

| 快捷键 | 操作 |
|--------|------|
| `Ctrl+C` | 复制选中文本 |
| `Ctrl+V` | 粘贴文本 |
| `Ctrl+X` | 剪切选中文本 |
| `Ctrl+A` | 全选文本 |
| `Ctrl+Enter` | 发送消息 |

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
│  Theme             [■ Matcha] [□ Peachy] [□ Mochi]       │
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

### UI-45A: C1 开始使用中心（General + Model 合并视图）

> 目标：新用户在一个面板完成“可用闭环”基础配置，不在首层暴露进阶决策。

**信息架构（由上到下）：**
1. 环境就绪卡片
2. 模型与 API 卡片
3. 基础偏好卡片（语言/主题）
4. 主行动区（保存并测试）

**字段规范（首层必显）：**

| 分组 | 字段 | 组件 | 必填 | 默认值 | 校验 |
|------|------|------|------|--------|------|
| 环境就绪 | OpenClaw 状态 | 只读状态行 | 是 | 自动检测 | `就绪/修复中/需处理/失败可重试` |
| 环境就绪 | 一键修复 | 主按钮 | 否 | 可点击 | 修复中禁用 |
| 模型/API | 推荐模型 | Dropdown | 是 | `openrouter/auto` | 不能为空 |
| 模型/API | API Key | Password Textarea | 是 | 空 | 非空 + 前缀校验 |
| 基础偏好 | 语言 | Dropdown | 是 | 系统语言 | `CN/EN` |
| 基础偏好 | 主题 | 3 枚快捷按钮 | 是 | `Matcha` | `Matcha/Peachy/Mochi` |

**按钮规范：**

| 按钮 | 位置 | 样式 | 触发结果 |
|------|------|------|----------|
| 自动修复 | 环境卡片右侧 | 主按钮 | 触发依赖修复流程并刷新状态 |
| 保存并测试 | 面板底部主按钮 | 主按钮 | 保存配置并触发一次健康检查 |
| 稍后处理 | 面板底部次按钮 | 次按钮 | 仅关闭面板，不覆盖已保存值 |
| 重试 | 错误摘要旁 | 文本按钮 | 重试最近失败动作 |

**状态与反馈：**

| 场景 | UI 状态 | 文案模板 |
|------|---------|----------|
| 依赖检查通过 | Success | `环境已就绪` |
| 正在修复 | Loading | `正在修复依赖...` |
| API 校验失败 | Error | `API Key 无效：{原因}` |
| 保存成功 | Toast Success | `设置已保存，连接测试通过` |
| 保存失败 | Inline Error + Toast | `保存失败：{原因}` |

**禁用条件（必须执行）：**
1. API Key 为空时，`保存并测试` 禁用。
2. 依赖修复进行中时，`自动修复` 与 `保存并测试` 同时禁用。
3. 模型列表为空时，模型下拉不可交互并显示 `无可用模型`。

**错误摘要区域（统一样式）：**
- 位置：对应卡片底部
- 结构：错误标题 + 单行摘要 + `重试`
- 颜色：`danger` 文本 + `surface` 背景

**页面级验收（UI-45A）：**
1. 用户在 C1 内可完成环境修复、模型选择、API 配置并通过测试。
2. 任一失败场景均可见错误摘要和可用重试入口。
3. 禁用逻辑在交互上可感知，不允许“点了无反应”。
4. 首层不出现 Runtime/Leader 强制决策。

### UI-46A: C3 Agent 中心（Runtime + Leader + 权限治理）

> 目标：把运行时切换、权限策略、可选 Agent 安装统一到一个可控中心。

**信息架构（由上到下）：**
1. 运行时选择与状态区
2. Leader 与权限策略区
3. 可选 Agent 安装区
4. 保存/回滚行动区

**运行时状态语义（统一四态）：**

| 状态 | 颜色 | 文案 |
|------|------|------|
| Healthy | success | `已健康` |
| Installed-Unhealthy | warning | `已安装，待启动/待修复` |
| Missing | danger | `未安装` |
| Conflict | danger | `配置冲突` |

**副作用提示规范：**
- 切换运行时后在行动区显示提示：`仅新会话生效，当前会话保持不变`。
- 保存成功后提示：`已切换到 {runtime}，建议新开会话验证`。

**回滚规范：**
1. 当运行时发生已提交变更后显示 `回滚到上一个运行时`。
2. 回滚后立即恢复 Dropdown 选择与配置写入。
3. 回滚成功给出成功 toast，失败给出错误摘要与重试。

**禁用条件：**
1. 状态为 Conflict 时，保存按钮禁用。
2. 安装任务进行中时，保存与二次安装按钮禁用。
3. 运行时状态未知且重检未完成时，保存按钮置为次要态。

**页面级验收（UI-46A）：**
1. 用户可感知运行时状态与副作用，不会“无提示切换”。
2. 发生错误配置时可阻断保存并引导修复。
3. 回滚路径完整可用。

### UI-48A: C4 系统与诊断中心（Log + Feature + Tracing + About）

> 目标：把工程诊断能力集中收敛，默认低曝光，按需进入。

**分区布局：**
1. 日志区（查询、过滤、导出、清空）
2. 开关区（Feature Flags）
3. 追踪区（Tracing 开关与过滤）
4. 系统信息区（版本、迁移、导入导出）

**交互规则：**
1. 日志区支持一键过滤 `ERROR/WARN/INFO/DEBUG`。
2. Feature/Tracing 的实验项必须带 `实验` 标识。
3. 高影响动作（清空日志、迁移导入）必须二次确认。

**页面级验收（UI-48A）：**
1. 工程用户可在单中心完成排障闭环。
2. 普通用户不进入 C4 不影响核心功能使用。
3. 所有高影响操作都有明确确认与结果反馈。

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
|           AnyClaw LVGL                                   │
|       Garlic Lobster - Your AI Assistant                 │
|              Version: 2.2.11                             │
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
  │  当前: v2.2.11 → 新版本: v2.3.0         │
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
