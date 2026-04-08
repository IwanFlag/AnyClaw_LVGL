#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
echo "[AnyClaw] Linux build environment check"
echo "Project: ${ROOT_DIR}"

need_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "[ERR] missing command: $1"
    return 1
  fi
  echo "[OK] $1"
}

need_cmd cmake
need_cmd make
need_cmd gcc
need_cmd g++
need_cmd zip

if command -v x86_64-w64-mingw32-gcc >/dev/null 2>&1; then
  echo "[OK] MinGW cross compiler found"
else
  echo "[WARN] MinGW cross compiler not found (Linux->Windows build unavailable)"
fi

ENV_FILE="${ROOT_DIR}/tools/linux/.env-path.sh"
cat > "${ENV_FILE}" <<EOF
export ANYCLAW_PROJECT_DIR="${ROOT_DIR}"
EOF
echo "[OK] Generated ${ENV_FILE}"
