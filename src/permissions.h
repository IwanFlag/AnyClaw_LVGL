#ifndef PERMISSIONS_H
#define PERMISSIONS_H

#include <string>
#include <vector>

/* Permission values */
enum class PermValue {
    Allow,      /* Always allowed */
    Deny,       /* Always denied */
    Ask,        /* Ask user each time */
    ReadOnly,   /* Read only (file operations) */
};

/* Permission keys */
enum class PermKey {
    /* File system */
    FS_READ_WORKSPACE,
    FS_WRITE_WORKSPACE,
    FS_READ_EXTERNAL,
    FS_WRITE_EXTERNAL,

    /* Command execution */
    EXEC_SHELL,
    EXEC_INSTALL,
    EXEC_DELETE,

    /* Network */
    NET_OUTBOUND,
    NET_INBOUND,
    NET_INTRANET,

    /* Device */
    DEVICE_CAMERA,
    DEVICE_MIC,
    DEVICE_SCREEN,
    DEVICE_USB_STORAGE,
    DEVICE_REMOTE_NODE,
    DEVICE_NEW_DEVICE,

    /* System */
    CLIPBOARD_READ,
    CLIPBOARD_WRITE,
    SYSTEM_MODIFY,

    COUNT  /* Total count */
};

/* Resource limits */
struct ResourceLimits {
    int max_disk_mb = 2048;
    int cmd_timeout_sec = 30;
    int net_rpm = 60;
    int max_subagents = 5;
    int max_file_read_kb = 500;
    int max_remote_connections = 3;
};

/* Permission configuration */
class Permissions {
public:
    /* Load from permissions.json (or use defaults) */
    bool load();

    /* Save to permissions.json */
    bool save() const;

    /* Get permission value */
    PermValue get(PermKey key) const;

    /* Set permission value */
    void set(PermKey key, PermValue value);

    /* Get resource limits */
    const ResourceLimits& limits() const { return limits_; }

    /* Set resource limits */
    void set_limits(const ResourceLimits& limits) { limits_ = limits; }

    /* Check if a path is allowed (workspace boundary) */
    bool is_path_allowed(const char* path, bool write = false) const;

    /* Add extra allowed path */
    void add_allowed_path(const char* path, bool write = false);

    /* Add denied path */
    void add_denied_path(const char* path);

    /* Get workspace root */
    const std::string& workspace_root() const { return workspace_root_; }

    /* Set workspace root */
    void set_workspace_root(const char* root) { workspace_root_ = root ? root : ""; }

private:
    PermValue values_[(int)PermKey::COUNT];
    ResourceLimits limits_;
    std::string workspace_root_;

    struct ExtraPath {
        std::string path;
        bool writable;
    };
    std::vector<ExtraPath> extra_allowed_;
    std::vector<std::string> denied_paths_;
};

/* Global singleton */
Permissions& permissions();

/* Runtime exec permission check with interactive ask flow */
bool perm_check_exec(PermKey key, const char* target);

/* Audit log (APPDATA\\AnyClaw_LVGL\\audit.log) */
void perm_audit_log(const char* action, const char* target, const char* decision);

/* Verify audit log chain integrity. Returns true if unbroken. */
bool perm_audit_verify_chain(std::string* error_out = nullptr);

#endif /* PERMISSIONS_H */
