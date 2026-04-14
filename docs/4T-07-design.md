# 4T-07 资源文件设计文档

> 创建时间：2026-04-14 21:29
> 状态：设计阶段 → 待确认后执行

---

## 一、总览

4T-07 包含 5 大资源类目，按优先级排序：

| # | 类目 | 文件数 | 方法 |
|---|------|--------|------|
| A | 大蒜吉祥物 | 18 PNG + 6 body/sprout | SVG → rsvg-convert → PNG |
| B | 龙虾 AI 头像 | 3 PNG | SVG → rsvg-convert → PNG |
| C | 托盘图标 | 80 PNG (5 主题 × 4 状态 × 4 尺寸) | SVG → rsvg-convert → PNG |
| I | Lucide 图标子集 | ~120 SVG | 下载 Lucide npm 包提取 |
| FB | 字体文件 | 5 族 ~10 个 TTF/OTF | Google Fonts + 开源字体下载 |

音效 WAV（4T-07.S1~S4）暂不处理，需音频工具。

---

## 二、设计工具链

### 核心工具

| 工具 | 用途 | 状态 |
|------|------|------|
| **rsvg-convert** (librsvg2-bin) | SVG → PNG 渲染 | ✅ 已安装 |
| **pngquant** | PNG 有损压缩（减小 60-80%） | 需安装 |
| **optipng** | PNG 无损优化 | 需安装 |
| **jq** | JSON 处理（Lucide metadata） | 需安装 |
| **curl/wget** | 下载字体 + Lucide 包 | ✅ 可用 |

### 工作流

```
设计 SVG (手写代码，精度控制)
  → rsvg-convert --width=N --height=N → PNG (渲染)
  → pngquant --quality=85-95 --speed=1 → 压缩
  → optipng -o2 → 优化
  → 验证尺寸/透明度/文件大小
```

### DPI 策略

- 所有 SVG 基于 **96×96 视口**设计（96 DPI 基准）
- 渲染时按目标尺寸缩放，rsvg-convert 自动抗锯齿
- 16px 版本：极简轮廓 + 纯色填充，无渐变/阴影
- 24px 版本：基础轮廓 + 简单高光
- 32px 版本：完整轮廓 + 渐变 + 阴影
- 48px 版本：完整细节 + 高光 + 纹理暗示
- 96px 版本：完整插画品质

---

## 三、大蒜吉祥物设计 (4T-07.A1~A3 + body/sprout)

### 角色设定

- **风格**：日系 kawaii（卡哇伊），圆润 Q 弹
- **比例**：头身比 1:1（大头小身体）
- **表情**：微笑眯眼，两团腮红
- **身体**：椭圆形蒜瓣，底部微尖

### Matcha 薄荷绿

| 部位 | 色值 | 说明 |
|------|------|------|
| 蒜瓣主体 | `#FFFFFF` → `#E8F5E9` 渐变 | 白到薄荷绿底部渐变 |
| 蒜瓣轮廓 | `#B2DFDB` | 淡薄荷描边 |
| 叶茎/茎 | `#3DD68C` | 品牌绿 |
| 叶茎高光 | `#6EE7B7` | 浅绿高光 |
| 腮红 | `#FFB3B3` opacity 40% | 半透明粉 |
| 眼睛 | `#2D3748` | 深色圆眼 |
| 眼睛高光 | `#FFFFFF` | 白色反光点 |

### Peachy 暖杏

| 部位 | 色值 | 说明 |
|------|------|------|
| 蒜瓣主体 | `#FFFFFF` → `#FFF0E6` 渐变 | 白到暖杏渐变 |
| 蒜瓣轮廓 | `#FFD4B8` | 暖杏描边 |
| 叶茎/茎 | `#FF7F50` | 珊瑚橘 |
| 叶茎高光 | `#FFB347` | 暖橙高光 |
| 腮红 | `#FF9F7A` opacity 40% | 橘粉腮红 |
| 眼睛 | `#5D3A1A` | 暖棕圆眼 |
| 眼睛高光 | `#FFFFFF` | |

### Mochi 奶茶棕

| 部位 | 色值 | 说明 |
|------|------|------|
| 蒜瓣主体 | `#FFFDF9` → `#F5EDE4` 渐变 | 米白到奶茶渐变 |
| 蒜瓣轮廓 | `#D4C4B0` | 奶茶描边 |
| 叶茎/茎 | `#A67B5B` | 奶茶棕 |
| 叶茎高光 | `#C9A96E` | 焦糖高光 |
| 腮红 | `#C4868C` opacity 40% | 豆沙粉腮红 |
| 眼睛 | `#4A3728` | 深棕圆眼 |
| 眼睛高光 | `#FFFFFF` | |

### 输出规格

**代码实际路径（garlic_dock.cpp）：**
- body: `assets/mascot/{theme}/garlic_48.png` → fallback `assets/garlic_48.png`
- sprout: `assets/mascot/{theme}/garlic_sprout.png` → fallback `assets/garlic_sprout.png`
- {theme} = matcha/peachy/mochi

**代码实际路径（ui_main.cpp 等）：**
- `assets/icons/ai/garlic_24.png` — 聊天气泡内联
- `assets/icons/ai/lobster_24/32/48.png` — AI 头像各尺寸

| 文件 | 路径 | 尺寸 | 用途 | 细节级别 |
|------|------|------|------|----------|
| `garlic_24_{theme}.png` | `assets/icons/ai/` | 24×24 | 聊天气泡内联 | 轮廓 + 色块 + 眼睛 |
| `garlic_32_{theme}.png` | `assets/icons/ai/` | 32×32 | 列表/面板 | + 渐变 + 腮红 |
| `garlic_48_{theme}.png` | `assets/mascot/{theme}/` | 48×48 | 桌面宠物 body | + 高光 + 叶茎细节 |
| `garlic_96_{theme}.png` | `assets/icons/ai/` | 96×96 | 品牌展示 | 完整插画 + 投影 |
| `garlic_sprout.png` | `assets/mascot/{theme}/` | 512×512 | 桌面宠物叶茎（源尺寸，运行时缩放到 72px） | 叶茎独立层 |
| `lobster_01_24.png` | `assets/icons/ai/` | 24×24 | 聊天气泡头像 | 简化轮廓 |
| `lobster_01_32.png` | `assets/icons/ai/` | 32×32 | 左侧面板 | + 渐变 |
| `lobster_01_48.png` | `assets/icons/ai/` | 48×48 | 关于页 | 完整细节 |

每主题大蒜 5 文件 × 3 主题 = 15 + 龙虾 3 = **18 个吉祥物 PNG**。
同时替换 `assets/garlic_32.png`、`assets/garlic_48.png`、`assets/garlic_sprout.png` 为 Matcha 版本作为通用 fallback。

### body/sprout 分层说明

桌面宠物需要两层贴图独立动画：
- **body**：蒜瓣身体，摇摆动画（±12°正弦）
- **sprout**：叶茎，独立弹跳
- body 用 COLORKEY (#FF00FF) 透明，sprout 用 alpha 透明

---

## 四、龙虾 AI 头像设计 (4T-07.A4)

### 角色设定

- **风格**：与大蒜同系列 kawaii
- **色调**：珊瑚红（不随主题变）
- **特征**：两只大钳子当手，弯弯的身体，两根触须

### 色板

| 部位 | 色值 |
|------|------|
| 身体 | `#FF6B6B` → `#E84545` 渐变 |
| 肚子 | `#FFE0E0` |
| 钳子 | `#FF4757` |
| 触须 | `#FF8787` |
| 眼睛 | `#2D3748` + `#FFFFFF` 高光 |
| 腮红 | `#FFB3B3` opacity 40% |

### 输出

| 文件 | 尺寸 | 用途 |
|------|------|------|
| `lobster_24.png` | 24×24 | 聊天气泡内头像 |
| `lobster_32.png` | 32×32 | 左侧面板 |
| `lobster_48.png` | 48×48 | 关于页 |

共 **3 个龙虾 PNG**。

---

## 五、托盘图标设计 (4T-07.T2~T4)

### 设计思路

托盘图标 = **简化大蒜轮廓** + **LED 状态圆点**

大蒜轮廓：极简半个蒜瓣剪影（在 16px 时几乎是个圆点 + 小尖尖）
LED 圆点：右下角，按状态变色

### 5 主题 × 4 状态色值

| 主题 | 空闲 (idle) | 运行中 (active) | 异常 (error) | 检测中 (busy) |
|------|------------|----------------|-------------|--------------|
| Matcha | `#E8ECF4` | `#3DD68C` | `#FF6B6B` | `#FFBE3D` |
| Peachy | `#8B7355` | `#FF7F50` | `#FF5C5C` | `#FFB347` |
| Mochi | `#B0A394` | `#A67B5B` | `#C47070` | `#C9A96E` |
| Dark | `#808080` | `#4EC9B0` | `#F44747` | `#FFD700` |
| Light | `#9CA3AF` | `#10B981` | `#EF4444` | `#F59E0B` |

### 尺寸

16/20/32/48px（96px 用 garlic_96 替代，托盘不需要）

### 文件命名

代码查找模式：`tray_{white|green|red|yellow}_{16|20|32|48}.png`

```
assets/tray/{theme}/tray_{state}_{size}.png
```

例如：`assets/tray/matcha/tray_green_32.png`

**注意**：现有 fallback 文件名 `tray_gray_*` 与代码查找的 `tray_white_*` 不匹配，新生成使用 `tray_white_*`，旧 `tray_gray_*` 保留作历史。

5 主题 × 4 状态 × 4 尺寸 = **80 个托盘 PNG**。

### 兼容性

代码 fallback 链：`tray/{theme}/` → `tray/`（通用）
通用 `tray/` 目录保留现有 gray/green/red/yellow 文件作为最终 fallback。

---

## 六、Lucide 图标子集 (4T-07.I1~I2)

### 方案

1. 下载 Lucide 0.460+ npm 包（或 GitHub release tar.gz）
2. 提取 ~120 个 SVG 到 `assets/icons/lucide/`
3. 按 Design.md §5 的分类目录组织

### 需要的图标分类

从代码中 grep 所有图标引用，加上 Design.md 定义的图标清单：
- 导航类：home, settings, message-circle, terminal, etc.
- 操作类：send, copy, trash, edit, search, etc.
- 状态类：check, x, alert-circle, info, etc.
- 文件类：file, file-text, folder, download, upload, etc.

### 下载完成

125 个 SVG 图标已提取到 `assets/icons/lucide/`（Lucide 0.460.0）。
覆盖 Design.md 定义的全部图标类别 + 常用 UI 图标。

---

## 七、字体文件 (4T-07.FB1~FB5)

### 字体清单

| # | 字体族 | 文件 | 来源 | License | 状态 |
|---|--------|------|------|---------|------|
| FB1 | Plus Jakarta Sans | Regular/Medium(as SemiBold)/Bold .ttf | GitHub | OFL | ✅ |
| FB2 | Nunito | Regular/Bold .ttf | GitHub (googlefonts) | OFL | ✅ |
| FB3 | HarmonyOS Sans SC | — | 华为源不可达 | — | ⏸️ 用 Noto Sans SC 替代 |
| FB3b | Noto Sans SC | Regular/Bold .otf | GitHub (googlefonts) | OFL | ✅ CJK 替代 |
| FB4 | Lora | Regular/Bold .ttf | GitHub (googlefonts) | OFL | ✅ |
| FB5 | JetBrains Mono | Regular .ttf | GitHub (JetBrains) | OFL | ✅ |

### 下载源

- Google Fonts：`https://fonts.google.com/download?family={name}`
- 或 GitHub release 直链

---

## 八、执行顺序

```
1. 安装工具 (pngquant, optipng, jq)
2. 编写 SVG 源文件 → assets/svg/{mascot,tray}/
3. 生成脚本 scripts/generate-assets.sh
4. 逐批渲染 PNG：
   a. 大蒜吉祥物 3 主题 × 4 尺寸 + body/sprout
   b. 龙虾头像 3 尺寸
   c. 托盘图标 5 主题 × 4 状态 × 4 尺寸
5. 压缩优化全部 PNG
6. 下载 Lucide 图标子集
7. 下载字体文件
8. 验证：文件存在性 + 尺寸 + 透明度
9. 更新 tasks/v2.2.1-dev.md 状态
10. git add + commit + push
```

---

## 九、质量标准

| 检查项 | 标准 |
|--------|------|
| 透明度 | 所有 PNG 带 alpha 通道（RGBA） |
| 文件大小 | 24px < 1KB, 48px < 5KB, 96px < 15KB |
| 无锯齿 | rsvg-convert 默认抗锯齿 + pngquant 后无明显色带 |
| 色彩准确 | 对照 Design.md 色值表，误差 < 2 |
| 一致性 | 3 主题大蒜角色视觉风格统一，仅色调不同 |
| 可识别性 | 16px 时仍能辨认是大蒜/龙虾 |
