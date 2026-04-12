#!/bin/bash
# ============================================================
# verify-docs.sh — PRD ↔ Design 文档一致性验证
# 用法: bash scripts/verify-docs.sh
# 检查项:
#   1. 版本号一致性
#   2. PRD 功能编号 ↔ Design 编号索引表完整性
#   3. 双向引用完整性
# ============================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
PRD="${ROOT_DIR}/docs/PRD.md"
DESIGN="${ROOT_DIR}/docs/Design.md"
ERRORS=0
WARNINGS=0

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m'

info()  { echo -e "  ${GREEN}✓${NC} $1"; }
warn()  { echo -e "  ${YELLOW}⚠${NC} $1"; WARNINGS=$((WARNINGS + 1)); }
error() { echo -e "  ${RED}✗${NC} $1"; ERRORS=$((ERRORS + 1)); }

echo "═══════════════════════════════════════════════════"
echo "  AnyClaw LVGL — 文档一致性验证"
echo "═══════════════════════════════════════════════════"

# Check files exist
if [ ! -f "${PRD}" ]; then
    error "PRD.md not found at ${PRD}"
    exit 1
fi
if [ ! -f "${DESIGN}" ]; then
    error "Design.md not found at ${DESIGN}"
    exit 1
fi

# ─── 1. Version consistency ───
echo ""
echo "[1/3] 版本号一致性检查"

PRD_VER=$(grep -oP '版本[：:]\s*v?([\d.]+)' "${PRD}" | head -1 | grep -oP '[\d.]+' | head -1)
DESIGN_VER=$(grep -oP '版本[：:]\s*v?([\d.]+)' "${DESIGN}" | head -1 | grep -oP '[\d.]+' | head -1)

if [ -z "${PRD_VER}" ]; then
    error "PRD: 未找到版本号"
elif [ -z "${DESIGN_VER}" ]; then
    error "Design: 未找到版本号"
elif [ "${PRD_VER}" != "${DESIGN_VER}" ]; then
    error "版本号不一致: PRD=v${PRD_VER}, Design=v${DESIGN_VER}"
else
    info "版本号一致: v${PRD_VER}"
fi

# ─── 2. Feature ID completeness ───
echo ""
echo "[2/3] 功能编号完整性检查"

# Extract feature IDs from PRD (pattern: XX-NNN like CI-01, WORK-01, PERM-01)
# Exclude UI-XX (those are UI numbers, checked separately in step 3)
PRD_IDS=$(grep -oP '\b[A-Z]{2,10}-\d{2}\b' "${PRD}" | grep -v '^UI-' | sort -u)

# Extract feature IDs from Design index table (first column of table rows)
# Exclude UI-XX and rows with "—" placeholder
DESIGN_INDEX_IDS=$(grep -oP '^\|?\s*([A-Z]{2,10}-\d{2})' "${DESIGN}" | grep -oP '[A-Z]{2,10}-\d{2}' | grep -v '^UI-' | sort -u)

# Find IDs in PRD but not in Design index
MISSING_IN_DESIGN=""
for id in ${PRD_IDS}; do
    if ! echo "${DESIGN_INDEX_IDS}" | grep -q "^${id}$"; then
        MISSING_IN_DESIGN="${MISSING_IN_DESIGN} ${id}"
    fi
done

# Find IDs in Design index but not in PRD
MISSING_IN_PRD=""
for id in ${DESIGN_INDEX_IDS}; do
    if ! echo "${PRD_IDS}" | grep -q "^${id}$"; then
        MISSING_IN_PRD="${MISSING_IN_PRD} ${id}"
    fi
done

if [ -n "${MISSING_IN_DESIGN}" ]; then
    warn "PRD 有但 Design 索引表没有的编号:${MISSING_IN_DESIGN}"
else
    info "PRD 编号全部在 Design 索引表中找到"
fi

if [ -n "${MISSING_IN_PRD}" ]; then
    warn "Design 索引表有但 PRD 没有的编号:${MISSING_IN_PRD}"
else
    info "Design 索引表编号全部在 PRD 中找到"
fi

# ─── 3. UI ID completeness ───
echo ""
echo "[3/3] UI 编号完整性检查"

# Extract UI IDs from Design (UI-NN pattern)
DESIGN_UI_IDS=$(grep -oP '\bUI-\d{2}\b' "${DESIGN}" | sort -u)

# Extract UI references from PRD
PRD_UI_REFS=$(grep -oP '\bUI-\d{2}\b' "${PRD}" | sort -u)

# Find UI IDs referenced in PRD but not defined in Design
MISSING_UI=""
for id in ${PRD_UI_REFS}; do
    if ! echo "${DESIGN_UI_IDS}" | grep -q "^${id}$"; then
        MISSING_UI="${MISSING_UI} ${id}"
    fi
done

if [ -n "${MISSING_UI}" ]; then
    warn "PRD 引用的 UI 编号在 Design 中未找到:${MISSING_UI}"
else
    info "PRD 引用的 UI 编号全部在 Design 中定义"
fi

# Count summary
PRD_ID_COUNT=$(echo "${PRD_IDS}" | wc -w)
DESIGN_INDEX_COUNT=$(echo "${DESIGN_INDEX_IDS}" | wc -w)
DESIGN_UI_COUNT=$(echo "${DESIGN_UI_IDS}" | wc -w)

echo ""
echo "  统计: PRD 功能编号 ${PRD_ID_COUNT} 个 | Design 索引表 ${DESIGN_INDEX_COUNT} 个 | Design UI 编号 ${DESIGN_UI_COUNT} 个"

# ─── Summary ───
echo ""
echo "═══════════════════════════════════════════════════"
if [ ${ERRORS} -gt 0 ]; then
    echo -e "  ${RED}验证失败: ${ERRORS} 个错误, ${WARNINGS} 个警告${NC}"
    exit 1
elif [ ${WARNINGS} -gt 0 ]; then
    echo -e "  ${YELLOW}验证通过（有警告）: ${WARNINGS} 个警告${NC}"
    exit 0
else
    echo -e "  ${GREEN}验证通过: 无错误无警告${NC}"
    exit 0
fi
