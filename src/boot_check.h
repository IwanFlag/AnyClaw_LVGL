/* ═══════════════════════════════════════════════════════════════
 *  boot_check.h - Comprehensive environment health check + auto-repair
 *  AnyClaw LVGL v2.0 - BOOT-01
 * ═══════════════════════════════════════════════════════════════ */

#ifndef BOOT_CHECK_H
#define BOOT_CHECK_H

#include <string>
#include <vector>

/* ── Check status ── */
enum class BootCheckStatus {
    Ok,       /* Check passed */
    Warn,     /* Non-critical issue */
    Error     /* Critical issue — app may not work */
};

/* ── Single check result ── */
struct BootCheckResult {
    std::string check_name;         /* e.g. "Node.js", "Gateway Port" */
    BootCheckStatus status = BootCheckStatus::Ok;
    std::string message;            /* Human-readable description */
    bool auto_fix_available = false;/* Can auto_fix() handle this? */
    bool fix_applied = false;       /* Was fix attempted? Did it succeed? */
};

/* ── Manager: run checks and fix issues ── */
class BootCheckManager {
public:
    /* Run every check. Returns structured results. */
    std::vector<BootCheckResult> run_all_checks();

    /* Try to auto-fix a single result. Returns true if fixed. */
    bool auto_fix(BootCheckResult& result);

    /* Try to auto-fix all fixable issues in-place. Returns true if all fixed. */
    bool auto_fix_all(std::vector<BootCheckResult>& results);

    /* Convenience: run + fix all, return final results */
    std::vector<BootCheckResult> run_and_fix();

    /* Summary text (pass/fail count) */
    static std::string summarize(const std::vector<BootCheckResult>& results);
};

/* ── Individual check functions ── */
BootCheckResult check_nodejs_version();
BootCheckResult check_npm();
BootCheckResult check_openclaw();
BootCheckResult check_gateway_port();
BootCheckResult check_config_dir();
BootCheckResult check_workspace();
BootCheckResult check_disk_space();
BootCheckResult check_network();
BootCheckResult check_sdl2dll();

#endif /* BOOT_CHECK_H */
