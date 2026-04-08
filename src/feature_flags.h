#ifndef FEATURE_FLAGS_H
#define FEATURE_FLAGS_H

#include <string>
#include <vector>

/* ═══ Pre-defined feature flag names ═══ */
#define FLAG_SBX_SANDBOX_EXEC   "sbx_sandbox_exec"
#define FLAG_OBS_TRACING        "obs_tracing"
#define FLAG_BOOT_SELF_HEAL     "boot_self_heal"
#define FLAG_MEM_LONG_TERM      "mem_long_term"
#define FLAG_KB_KNOWLEDGE_BASE  "kb_knowledge_base"
#define FLAG_REMOTE_COLLAB      "remote_collab"

/* ═══ Feature flag entry ═══ */
struct FeatureFlag {
    std::string name;           /* Machine key, e.g. "sbx_sandbox_exec" */
    std::string description;    /* Human-readable label */
    bool enabled;               /* Current state */
    bool default_value;         /* Factory default */
    bool requires_restart;      /* Change needs app restart to take effect */
};

/* ═══ Feature flag manager (global singleton) ═══ */
class FeatureFlagManager {
public:
    /* Load flags from config.json "feature_flags" section.
     * Missing flags get their default values. */
    bool load_from_config();

    /* Save current flag state back to config.json */
    bool save_to_config() const;

    /* Check if a specific flag is enabled */
    bool is_enabled(const char* flag_name) const;

    /* Set a flag's enabled state (call save_to_config() to persist) */
    void set_enabled(const char* flag_name, bool enabled);

    /* Get all flags (for UI display) */
    const std::vector<FeatureFlag>& get_all_flags() const { return flags_; }

    /* Get a single flag by name (returns nullptr if not found) */
    const FeatureFlag* get_flag(const char* flag_name) const;

    /* Reset all flags to factory defaults */
    void reset_to_defaults();

private:
    std::vector<FeatureFlag> flags_;

    /* Initialize default flag set (called once on first use) */
    void init_defaults();

    int find_flag(const char* flag_name) const;
};

/* ═══ Global singleton ═══ */
FeatureFlagManager& feature_flags();

/* ═══ Convenience: init at startup ═══ */
void feature_flags_init();

#endif /* FEATURE_FLAGS_H */
