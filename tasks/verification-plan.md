# AnyClaw LVGL v2.2.1 — 无 Wine 验证计划

> 目标：在 Linux 服务器上，不依赖 Wine/图形环境，尽可能全面地验证 PRD + Design 的功能实现。
> 原则：能自动化的全部自动化，人工截图验证留给有 Wine 环境时补做。

---

## 验证矩阵

| 方法 | 覆盖范围 | 能替代 Wine 截图？ |
|------|---------|-------------------|
| ① 编译验证 | 语法、链接、依赖 | 部分 |
| ② 资源完整性 | 所有引用文件存在 | ✅ |
| ③ 代码-设计色值对照 | ThemeColors 全部 Token | ✅ |
| ④ 布局常量审查 | PCT 常量 vs Design §4 | ✅ |
| ⑤ PRD↔Design 编号一致性 | 功能编号映射 | ✅ |
| ⑥ 文档版本同步 | PRD/Design 版本号 | ✅ |
| ⑦ 代码逻辑走查 | 关键功能路径 | 部分 |
| ⑧ 配置/持久化 | config.json 字段 | ✅ |
| ⑨ 构建产物审查 | zip 内容完整性 | ✅ |

---

## 阶段 A：构建与产物（先跑通）

### A-1: 编译验证
```bash
bash build/linux/build.sh v2.2.1
```
- [ ] 编译零 error
- [ ] 警告数量可控（LV_SDL_USE_EGL 重定义除外）
- [ ] 产物 zip 存在且 > 40MB

### A-2: 构建产物内容审查
- [ ] zip 内包含 `AnyClaw_LVGL.exe` + `SDL2.dll`
- [ ] zip 内包含 `assets/tray/` 全部 5 主题子目录
- [ ] zip 内包含 `assets/fonts/` 全部字体文件
- [ ] zip 内包含 `assets/sounds/` 3 个主题子目录 + 24 个 wav
- [ ] zip 内包含 `assets/mascot/` 3 个主题子目录 + garlic png
- [ ] zip 内包含 `assets/icons/ai/` 31 个 png
- [ ] zip 内包含 `bundled/openclaw.tgz` + `bundled/workspace/`
- [ ] **Bug 排查**：garlic_sprout.png 和音效 wav 为什么丢失

---

## 阶段 B：资源完整性（自动化）

### B-1: 代码引用 vs 文件系统对照
编写脚本提取所有 `assets/` 路径引用，检查文件是否存在：
- `garlic_48.png`, `garlic_sprout.png` (root + 3 主题)
- `tray_*.png` (root + 5 主题 × 4 色 × 4 尺寸)
- `fonts/*.ttf/*.otf` (14 个字体文件)
- `sounds/{theme}/*.wav` (8 事件 × 3 主题)
- `icons/ai/*.png` (31 个)
- `icons/lucide/*.svg` (125 个)

### B-2: 音效文件完整性
```bash
# 每个主题目录应有 8 个 wav
for theme in matcha peachy mochi; do
  echo "=== $theme ==="
  ls assets/sounds/$theme/*.wav 2>/dev/null | wc -l
done
```
- [ ] 3 主题 × 8 wav = 24 文件

---

## 阶段 C：主题系统验证（代码静态分析）

### C-1: ThemeColors 字段完整性
- [ ] theme.h 中 ThemeColors 字段数量 ≈ 46 颜色 + 5 结构
- [ ] 每个 Theme enum 值都有对应的 THEME_XXX 定义
- [ ] 5 个主题全部 46 字段无 NULL/遗漏

### C-2: 色值对照 Design §2
对照 Design.md 逐 Token 校验：
- [ ] Matcha 色值 ↔ THEME_DARK 定义 (§2.2)
- [ ] Peachy 色值 ↔ THEME_PEACHY 定义 (§2.3)
- [ ] Mochi 色值 ↔ THEME_MOCHI 定义 (§2.4)
- [ ] Dark 色值 ↔ THEME_CLASSIC 定义 (§2.5)
- [ ] Light 色值 ↔ THEME_LIGHT 定义 (§2.6)

### C-3: 硬编码颜色扫描
```bash
grep -rn "lv_color_make" src/ --include="*.cpp" | grep -v "helper\|fallback\|static const"
```
- [ ] 结果 = 0（除 helper 函数允许的 1 处）

### C-4: 圆角系统
- [ ] ThemeColors 包含 radius_sm/md/lg/xl/2xl
- [ ] 5 个主题的 radius 值已填写
- [ ] 代码中无硬编码 `lv_obj_set_style_radius(..., 8, ...)` 类写法

### C-5: 字体系统
- [ ] ThemeFonts 结构体存在
- [ ] FONT_PATHS[] 数组 5 行 × 6 列
- [ ] 每行的 .ttf/.otf 文件均存在（B-1 交叉验证）
- [ ] init_theme_fonts() switch 覆盖全部 5 个 Theme enum

---

## 阶段 D：功能逻辑走查（阅读代码）

### D-1: 主界面框架 (PRD §3)
- [ ] 左导航 + 标题栏 + 内容区布局代码存在
- [ ] 百分比布局：PCT 常量在 app_config.h 中，cpp 中无硬编码像素
- [ ] 窗口 resize 响应逻辑存在

### D-2: Chat 模式 (PRD §4)
- [ ] 多行输入框实现
- [ ] 消息气泡（用户/AI）渲染
- [ ] 流式渲染逻辑（g_stream_done + 回调）
- [ ] 文件/多模态上传
- [ ] 会话管理（列表/终止/重置）

### D-3: Work 模式 (PRD §5)
- [ ] StepCard 组件
- [ ] 输出渲染器（Terminal/Preview/Diff/Table）
- [ ] Chat/Work 模式切换

### D-4: 模型管理 (PRD §7)
- [ ] 云端模型选择
- [ ] API Key 管理
- [ ] Model Failover 逻辑

### D-5: 设置 (PRD §11)
- [ ] 主题选择器（5 选项）
- [ ] 主题切换 → config.json 持久化
- [ ] 音效开关/音量控制

### D-6: Agent 能力 (PRD §8)
- [ ] 权限系统（19 项枚举）
- [ ] 弹窗确认逻辑
- [ ] Skill 管理
- [ ] 工作区管理

---

## 阶段 E：文档一致性

### E-1: verify-docs.sh
```bash
bash scripts/verify-docs.sh
```
- [ ] 无报错

### E-2: PRD ↔ Design 编号映射
- [ ] PRD 每个功能编号在 Design 中有对应 UI 编号
- [ ] Design 每个 UI 编号在 PRD 中有对应功能
- [ ] 状态标记一致（✅/🔧/⏳）

### E-3: 版本号同步
- [ ] PRD 标题 = Design 标题 = v2.2.1

---

## 阶段 F：配置与持久化

### F-1: config.json 字段验证
代码中读取/写入的 config 字段应全部有默认值：
- [ ] theme（0-4）
- [ ] model
- [ ] log_enabled / log_level
- [ ] sound_enabled / sound_volume
- [ ] auto_restart_on_crash
- [ ] feature flags

### F-2: 迁移兼容性
- [ ] 旧 config.json (theme=Light) 能正确映射到新 enum

---

## 执行顺序

```
A-1 编译 → A-2 产物审查 → B-1 资源完整性 → B-2 音效
→ C-1~C-5 主题系统 → D-1~D-6 功能走查
→ E-1~E-3 文档一致性 → F-1~F-2 配置验证
```

预估时间：3-5 小时（自动化脚本 + 人工走查）

---

## 产出物

1. `tasks/verification-report.md` — 每项检查结果（✅/❌/⚠️）
2. Bug 清单 — 发现的问题记录到 v2.2.1-scan.md
3. 修复 — 代码问题即时修复 + commit + push
