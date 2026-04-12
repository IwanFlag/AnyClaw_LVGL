#!/usr/bin/env bash
# ============================================================
# AnyClaw × Hermes Agent 一键安装脚本 (WSL)
# 用法: bash install-hermes.sh
# ============================================================
set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

info()  { echo -e "${CYAN}[INFO]${NC} $*"; }
ok()    { echo -e "${GREEN}[✓]${NC} $*"; }
warn()  { echo -e "${YELLOW}[!]${NC} $*"; }
fail()  { echo -e "${RED}[✗]${NC} $*"; exit 1; }

echo ""
echo -e "${CYAN}========================================${NC}"
echo -e "${CYAN}  🧄🦞 AnyClaw × Hermes Agent 安装器${NC}"
echo -e "${CYAN}========================================${NC}"
echo ""

# ---- 1. 检查系统依赖 ----
info "检查系统依赖..."

check_cmd() {
    if command -v "$1" &>/dev/null; then
        ok "$1 已安装: $(command -v "$1")"
        return 0
    else
        warn "$1 未安装，将自动安装"
        return 1
    fi
}

MISSING_DEPS=()
check_cmd curl   || MISSING_DEPS+=(curl)
check_cmd git    || MISSING_DEPS+=(git)
check_cmd node   || MISSING_DEPS+=(node)
check_cmd npm    || MISSING_DEPS+=(npm)

# 检查 Node.js 版本
if command -v node &>/dev/null; then
    NODE_VER=$(node --version | sed 's/v//')
    NODE_MAJOR=$(echo "$NODE_VER" | cut -d. -f1)
    if [ "$NODE_MAJOR" -lt 22 ]; then
        warn "Node.js 版本 $NODE_VER 过低，需要 >= 22"
        MISSING_DEPS+=(node-upgrade)
    else
        ok "Node.js 版本 $NODE_VER ✓"
    fi
fi

# 安装缺失依赖
if [ ${#MISSING_DEPS[@]} -gt 0 ]; then
    info "安装缺失依赖: ${MISSING_DEPS[*]}"
    sudo apt-get update -qq
    for dep in "${MISSING_DEPS[@]}"; do
        case "$dep" in
            node-upgrade)
                info "升级 Node.js..."
                curl -fsSL https://deb.nodesource.com/setup_22.x | sudo -E bash -
                sudo apt-get install -y nodejs
                ;;
            *)
                sudo apt-get install -y "$dep"
                ;;
        esac
    done
    ok "依赖安装完成"
fi

# ---- 2. 安装 Hermes Agent ----
info "安装 Hermes Agent..."

if command -v hermes &>/dev/null; then
    warn "Hermes Agent 已安装，检查更新..."
    npm update -g hermes-agent 2>/dev/null || true
    ok "Hermes Agent 已是最新"
else
    info "正在安装 hermes-agent..."
    npm install -g hermes-agent
    ok "Hermes Agent 安装完成"
fi

# 验证安装
if ! command -v hermes &>/dev/null; then
    # 尝试直接安装方式
    info "尝试官方安装脚本..."
    curl -fsSL https://raw.githubusercontent.com/NousResearch/hermes-agent/main/scripts/install.sh | bash
fi

hermes --version 2>/dev/null && ok "Hermes 版本确认" || warn "版本检测失败，可能需要手动验证"

# ---- 3. 配置 Nous Portal + MiMo-V2 ----
info "配置 Nous Portal + Xiaomi MiMo-V2..."

HERMES_CONFIG_DIR="$HOME/.hermes"
mkdir -p "$HERMES_CONFIG_DIR"

# 引导用户获取 API Key
echo ""
echo -e "  ${YELLOW}📌 获取 Nous Portal API Key:${NC}"
echo -e "  1. 访问 ${CYAN}https://portal.nousresearch.com${NC}"
echo -e "  2. 注册/登录账号"
echo -e "  3. 进入 API Keys 页面，创建新 Key"
echo -e "  4. 复制 Key 粘贴到下方"
echo ""
echo -e "  ${GREEN}MiMo-V2 Pro/Omni/Flash 限免两周（4月10日起）${NC}"
echo ""

read -rp "请输入 Nous Portal API Key（留空跳过，稍后 hermes setup 配置）: " NOUS_API_KEY

# 读取用户时区
SYSTEM_TZ=$(timedatectl show -p Timezone --value 2>/dev/null || echo "Asia/Shanghai")
echo ""
read -rp "时区 [默认: $SYSTEM_TZ]: " USER_TZ
USER_TZ="${USER_TZ:-$SYSTEM_TZ}"

# 检查是否已有配置
if [ -f "$HERMES_CONFIG_DIR/config.yaml" ] || [ -f "$HERMES_CONFIG_DIR/config.json" ]; then
    warn "检测到已有配置文件，跳过自动配置"
    warn "如需重新配置，请运行: hermes setup"
else
    info "生成默认配置..."

    cat > "$HERMES_CONFIG_DIR/config.yaml" << EOF
# Hermes Agent 配置 — AnyClaw 一键安装生成
# 文档: https://hermes-agent.nousresearch.com

provider:
  name: nous-portal
  # Nous Portal 提供免费 MiMo-V2 调用（限免期内）
  # 如需 API Key，访问 https://portal.nousresearch.com 获取
  api_key: "${NOUS_API_KEY}"
  base_url: "https://api.nousresearch.com/v1"

model:
  # Xiaomi MiMo-V2 系列（限免）
  primary: "xiaomi/mimo-v2-pro"
  fallback: "xiaomi/mimo-v2-flash"
  # 可用模型:
  #   xiaomi/mimo-v2-pro   — 推理最强，1M 长上下文，复杂任务首选
  #   xiaomi/mimo-v2-omni  — 全模态理解（图像/视频/音频+文本）
  #   xiaomi/mimo-v2-flash — 速度最快，日常对话/轻量任务
  available:
    - id: "xiaomi/mimo-v2-pro"
      name: "MiMo-V2 Pro"
      description: "推理旗舰，1M 长上下文，深度 Agent 优化"
      context_window: 1048576
      capabilities: [chat, tools, reasoning]
    - id: "xiaomi/mimo-v2-omni"
      name: "MiMo-V2 Omni"
      description: "全模态理解，图像/视频/音频+文本"
      context_window: 1048576
      capabilities: [chat, tools, reasoning, vision, audio, video]
    - id: "xiaomi/mimo-v2-flash"
      name: "MiMo-V2 Flash"
      description: "极速响应，轻量任务首选"
      context_window: 262144
      capabilities: [chat, tools]

agent:
  name: "AnyClaw-Hermes"
  timezone: "$USER_TZ"

gateway:
  port: 18790
  # 消息平台（按需启用）
  # telegram:
  #   enabled: true
  #   bot_token: "YOUR_BOT_TOKEN"
  # discord:
  #   enabled: true
  #   bot_token: "YOUR_BOT_TOKEN"

memory:
  enabled: true
  max_sessions: 100

skills:
  auto_generate: true
  directory: "$HOME/.hermes/skills"
EOF

    ok "配置文件已生成: $HERMES_CONFIG_DIR/config.yaml"
fi

# ---- 4. 创建工作区 ----
info "创建工作区..."

HERMES_WORKSPACE="$HOME/.hermes/workspace"
mkdir -p "$HERMES_WORKSPACE/memory"
mkdir -p "$HERMES_WORKSPACE/skills"
mkdir -p "$HERMES_WORKSPACE/projects"

# 生成 MEMORY.md
if [ ! -f "$HERMES_WORKSPACE/MEMORY.md" ]; then
    cat > "$HERMES_WORKSPACE/MEMORY.md" << 'EOF'
# MEMORY.md — Hermes Agent 长期记忆

## 用户偏好
（待学习）

## 项目记录
（待记录）

## 经验教训
（待积累）
EOF
    ok "MEMORY.md 已创建"
fi

# 生成 USER.md
if [ ! -f "$HERMES_WORKSPACE/USER.md" ]; then
    cat > "$HERMES_WORKSPACE/USER.md" << 'EOF'
# USER.md — 用户档案

- **时区:** Asia/Shanghai
- **语言:** 中文
- **备注:**
EOF
    ok "USER.md 已创建"
fi

# ---- 5. 注册系统服务（开机自启） ----
info "配置开机自启..."

SYSTEMD_SERVICE="/etc/systemd/system/hermes-agent.service"
if [ ! -f "$SYSTEMD_SERVICE" ]; then
    HERMES_PATH=$(command -v hermes || echo "/usr/local/bin/hermes")
    sudo tee "$SYSTEMD_SERVICE" > /dev/null << EOF
[Unit]
Description=Hermes Agent
After=network.target

[Service]
Type=simple
User=$USER
ExecStart=$HERMES_PATH gateway start --foreground
Restart=on-failure
RestartSec=5
Environment=HOME=$HOME

[Install]
WantedBy=multi-user.target
EOF

    sudo systemctl daemon-reload
    sudo systemctl enable hermes-agent
    ok "系统服务已注册"
else
    ok "系统服务已存在"
fi

# ---- 6. 启动 Hermes Gateway ----
info "启动 Hermes Gateway..."

if hermes gateway start 2>/dev/null; then
    ok "Hermes Gateway 已启动 (端口 18790)"
else
    warn "Gateway 启动失败，请手动运行: hermes gateway start"
fi

# ---- 7. 完成 ----
echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  ✅ Hermes Agent 安装完成！${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo -e "  运行时端口:   ${CYAN}:18790${NC}"
echo -e "  默认模型:     ${CYAN}xiaomi/mimo-v2-pro${NC}"
echo -e "  可用模型:     ${CYAN}Pro / Omni / Flash${NC}"
echo -e "  API Key:      ${CYAN}$( [ -n "$NOUS_API_KEY" ] && echo '已配置 ✓' || echo '未配置，运行 hermes setup' )${NC}"
echo -e "  配置文件:     ${CYAN}$HERMES_CONFIG_DIR/config.yaml${NC}"
echo -e "  工作区:       ${CYAN}$HERMES_WORKSPACE${NC}"
echo ""
echo -e "  ${YELLOW}下一步:${NC}"
echo -e "  1. 打开 AnyClaw → Settings → Runtime → 选择 Hermes Agent"
echo -e "  2. （可选）运行 ${CYAN}hermes setup${NC} 配置 API Key"
echo -e "  3. （可选）运行 ${CYAN}hermes gateway setup${NC} 接入 Telegram/Discord"
echo ""
echo -e "  ${YELLOW}常用命令:${NC}"
echo -e "  hermes gateway start     # 启动 Gateway"
echo -e "  hermes gateway stop      # 停止 Gateway"
echo -e "  hermes gateway status    # 查看状态"
echo -e "  hermes setup             # 重新配置"
echo ""
