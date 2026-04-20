#ifndef RUNTIME_MGR_H
#define RUNTIME_MGR_H

/*
 * runtime_mgr.h — Hermes Agent & Claude CLI 检测 / 安装 / 健康检查
 * 模式与 openclaw_mgr.cpp 完全对齐：auto / network / local 三模式
 */

/* ═══════════════════════════════════════════════════════════════════
 * Hermes Agent
 * ═══════════════════════════════════════════════════════════════════ */

/* 检测 Hermes 是否已安装（hermes.exe 存在 + --version 能运行） */
bool app_detect_hermes(char* version_out, int ver_size);

/* Hermes 健康检查：HTTP GET :18790/health */
bool app_check_hermes_health(void);

/* Hermes 安装（auto=先网络降级本地 / network=强制网络 / local=强制本地） */
bool app_install_hermes(const char* mode /* "auto" | "network" | "local" */,
                        void (*progress_cb)(const char* step, int pct, void* ctx),
                        void* ctx);

/* Hermes 完整初始化（启动 gateway + 生成配置） */
bool app_init_hermes(void);

/* ═══════════════════════════════════════════════════════════════════
 * Claude CLI
 * ═══════════════════════════════════════════════════════════════════ */

/* 检测 Claude CLI 是否已安装 */
bool app_detect_claude_cli(char* version_out, int ver_size);

/* Claude CLI 健康检查：进程是否存活 */
bool app_check_claude_cli_health(void);

/* Claude CLI 快速检测：仅检查 claude.exe 文件是否存在（主线程安全，无子进程） */
bool app_check_claude_cli_exists(void);

/* Claude CLI 安装（auto=先网络降级本地 / network=强制网络 / local=强制本地） */
bool app_install_claude_cli(const char* mode /* "auto" | "network" | "local" */,
                            void (*progress_cb)(const char* step, int pct, void* ctx),
                            void* ctx);

/* ═══════════════════════════════════════════════════════════════════
 * 全量环境检测结果（扩展 EnvCheckResult）
 * 由 openclaw_mgr.cpp 的 app_check_environment() 填充
 * ═══════════════════════════════════════════════════════════════════ */
struct HermesCheckResult {
    bool installed;          /* hermes.exe 存在 */
    bool healthy;           /* gateway :18790 HTTP 200 */
    char version[64];       /* --version 输出 */
};

struct ClaudeCheckResult {
    bool installed;         /* claude.exe 存在 */
    bool healthy;          /* 进程在运行 */
    char version[64];       /* --version 输出 */
};

/* 获取 Hermes/Claude 检测结果（由 app_check_environment 填充） */
const HermesCheckResult* app_get_hermes_result(void);
const ClaudeCheckResult* app_get_claude_result(void);

#endif /* RUNTIME_MGR_H */
