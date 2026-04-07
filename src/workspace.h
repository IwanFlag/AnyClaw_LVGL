#ifndef WORKSPACE_H
#define WORKSPACE_H

#include <string>

/* Workspace health check result */
struct WorkspaceHealth {
    bool exists = false;          /* Directory exists */
    bool writable = false;        /* Has write permission */
    bool has_agents_md = false;   /* AGENTS.md present */
    bool has_soul_md = false;     /* SOUL.md present */
    bool has_memory_dir = false;  /* memory/ directory present */
    long long free_mb = 0;        /* Free disk space in MB */
    char error_msg[256] = {0};    /* Human-readable error */
};

/* Get workspace root path from config (default: %USERPROFILE%\.openclaw\workspace\) */
std::string workspace_get_root();

/* Set workspace root path (persisted to config.json) */
bool workspace_set_root(const char* path);

/* Run health check on workspace directory */
WorkspaceHealth workspace_check_health();

/* Initialize workspace: create directory + template files if missing */
bool workspace_init(const char* root_path = nullptr);

/* Generate WORKSPACE.md in workspace root */
bool workspace_generate_meta(const char* name = nullptr);

/* Get workspace name (from WORKSPACE.md or directory name) */
std::string workspace_get_name();

/* Workspace lock (.openclaw.lock) */
bool workspace_lock_acquire();
void workspace_lock_release();
bool workspace_lock_is_held();

/* Sync managed policy section to AGENTS.md / TOOLS.md */
bool workspace_sync_managed_sections();

/* Sync runtime workspace.json projection from permissions.json */
bool workspace_sync_runtime_config_from_permissions();

#endif /* WORKSPACE_H */
