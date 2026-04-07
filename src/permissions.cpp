#include "permissions.h"
#include "app.h"
#include "app_log.h"
#include "workspace.h"
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <ctime>

/* ═══ Default values ═══ */

static PermValue default_value(PermKey key) {
    switch (key) {
        case PermKey::FS_READ_WORKSPACE:   return PermValue::Allow;
        case PermKey::FS_WRITE_WORKSPACE:  return PermValue::Allow;
        case PermKey::FS_READ_EXTERNAL:    return PermValue::Deny;
        case PermKey::FS_WRITE_EXTERNAL:   return PermValue::Deny;
        case PermKey::EXEC_SHELL:          return PermValue::Ask;
        case PermKey::EXEC_INSTALL:        return PermValue::Deny;
        case PermKey::EXEC_DELETE:         return PermValue::Ask;
        case PermKey::NET_OUTBOUND:        return PermValue::Allow;
        case PermKey::NET_INBOUND:         return PermValue::Deny;
        case PermKey::NET_INTRANET:        return PermValue::Deny;
        case PermKey::DEVICE_CAMERA:       return PermValue::Deny;
        case PermKey::DEVICE_MIC:          return PermValue::Deny;
        case PermKey::DEVICE_SCREEN:       return PermValue::Ask;
        case PermKey::DEVICE_USB_STORAGE:  return PermValue::Ask;
        case PermKey::DEVICE_REMOTE_NODE:  return PermValue::Ask;
        case PermKey::DEVICE_NEW_DEVICE:   return PermValue::Ask;
        case PermKey::CLIPBOARD_READ:      return PermValue::Allow;
        case PermKey::CLIPBOARD_WRITE:     return PermValue::Allow;
        case PermKey::SYSTEM_MODIFY:       return PermValue::Deny;
        default: return PermValue::Deny;
    }
}

static const char* perm_key_name(PermKey key) {
    switch (key) {
        case PermKey::FS_READ_WORKSPACE:   return "fs_read_workspace";
        case PermKey::FS_WRITE_WORKSPACE:  return "fs_write_workspace";
        case PermKey::FS_READ_EXTERNAL:    return "fs_read_external";
        case PermKey::FS_WRITE_EXTERNAL:   return "fs_write_external";
        case PermKey::EXEC_SHELL:          return "exec_shell";
        case PermKey::EXEC_INSTALL:        return "exec_install";
        case PermKey::EXEC_DELETE:         return "exec_delete";
        case PermKey::NET_OUTBOUND:        return "net_outbound";
        case PermKey::NET_INBOUND:         return "net_inbound";
        case PermKey::NET_INTRANET:        return "net_intranet";
        case PermKey::DEVICE_CAMERA:       return "device_camera";
        case PermKey::DEVICE_MIC:          return "device_mic";
        case PermKey::DEVICE_SCREEN:       return "device_screen";
        case PermKey::DEVICE_USB_STORAGE:  return "device_usb_storage";
        case PermKey::DEVICE_REMOTE_NODE:  return "device_remote_node";
        case PermKey::DEVICE_NEW_DEVICE:   return "device_new_device";
        case PermKey::CLIPBOARD_READ:      return "clipboard_read";
        case PermKey::CLIPBOARD_WRITE:     return "clipboard_write";
        case PermKey::SYSTEM_MODIFY:       return "system_modify";
        default: return "unknown";
    }
}

static const char* perm_value_str(PermValue v) {
    switch (v) {
        case PermValue::Allow:    return "allow";
        case PermValue::Deny:     return "deny";
        case PermValue::Ask:      return "ask";
        case PermValue::ReadOnly: return "read_only";
        default: return "deny";
    }
}

static PermValue parse_perm_value(const char* s) {
    if (!s) return PermValue::Deny;
    if (strcmp(s, "allow") == 0) return PermValue::Allow;
    if (strcmp(s, "deny") == 0) return PermValue::Deny;
    if (strcmp(s, "ask") == 0) return PermValue::Ask;
    if (strcmp(s, "read_only") == 0) return PermValue::ReadOnly;
    return PermValue::Deny;
}

/* ═══ Path helpers ═══ */

static std::string get_permissions_path() {
    const char* appdata = std::getenv("APPDATA");
    if (!appdata) return "";
    return std::string(appdata) + "\\AnyClaw_LVGL\\permissions.json";
}

static std::string get_audit_path() {
    const char* appdata = std::getenv("APPDATA");
    if (!appdata) return "";
    return std::string(appdata) + "\\AnyClaw_LVGL\\audit.log";
}

void perm_audit_log(const char* action, const char* target, const char* decision) {
    std::string path = get_audit_path();
    if (path.empty()) return;

    size_t last_sep = path.find_last_of("\\/");
    if (last_sep != std::string::npos) {
        std::string dir = path.substr(0, last_sep);
        if (!std::filesystem::exists(dir)) {
            try { std::filesystem::create_directories(dir); } catch (...) { return; }
        }
    }

    char ts[64] = {0};
    std::time_t now = std::time(nullptr);
    std::tm* lt = std::localtime(&now);
    if (lt) std::strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", lt);

    std::ofstream f(path, std::ios::app);
    if (!f.is_open()) return;
    f << "[" << ts << "]"
      << " action=" << (action ? action : "-")
      << " target=" << (target ? target : "-")
      << " decision=" << (decision ? decision : "-")
      << "\n";
}

/* ═══ Permissions singleton ═══ */

static Permissions g_perms;
Permissions& permissions() { return g_perms; }

bool perm_check_exec(PermKey key, const char* target) {
    PermValue v = permissions().get(key);
    if (v == PermValue::Allow) {
        perm_audit_log("exec_check", target, "allow");
        return true;
    }
    if (v == PermValue::Deny || v == PermValue::ReadOnly) {
        perm_audit_log("exec_check", target, "deny");
        return false;
    }

    const char* key_name = perm_key_name(key);
    char msg[1024] = {0};
    snprintf(
        msg, sizeof(msg),
        "Agent 请求执行命令：\n\n%s\n\n权限项: %s\n\n是: 仅本次允许\n取消: 永久允许\n否: 拒绝",
        target ? target : "(unknown)", key_name ? key_name : "exec_shell"
    );

    int r = MessageBoxA(
        nullptr,
        msg,
        "AnyClaw 权限确认",
        MB_ICONWARNING | MB_YESNOCANCEL | MB_TOPMOST | MB_SETFOREGROUND
    );

    if (r == IDYES) {
        perm_audit_log("exec_check", target, "allow_once");
        return true;
    }
    if (r == IDCANCEL) {
        permissions().set(key, PermValue::Allow);
        permissions().save();
        workspace_sync_managed_sections();
        perm_audit_log("exec_check", target, "allow_persist");
        return true;
    }

    perm_audit_log("exec_check", target, "deny_by_user");
    return false;
}

/* ═══ Permissions implementation ═══ */

bool Permissions::load() {
    /* Set defaults first */
    for (int i = 0; i < (int)PermKey::COUNT; i++) {
        values_[i] = default_value((PermKey)i);
    }

    std::string path = get_permissions_path();
    std::ifstream f(path);
    if (!f.is_open()) {
        LOG_I("PERM", "No permissions.json, using defaults");
        perm_audit_log("perm_load", "permissions.json", "defaults");
        return false;
    }

    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    f.close();

    /* Parse each permission key */
    for (int i = 0; i < (int)PermKey::COUNT; i++) {
        char val[32] = {0};
        if (json_extract_string(content.c_str(), perm_key_name((PermKey)i), val, sizeof(val))) {
            values_[i] = parse_perm_value(val);
        }
    }

    /* Parse workspace_root */
    char root[512] = {0};
    if (json_extract_string(content.c_str(), "workspace_root", root, sizeof(root))) {
        workspace_root_ = root;
    }

    LOG_I("PERM", "Loaded permissions from %s", path.c_str());
    perm_audit_log("perm_load", "permissions.json", "ok");
    return true;
}

bool Permissions::save() const {
    std::string path = get_permissions_path();

    /* Ensure directory exists */
    size_t last_sep = path.find_last_of("\\/");
    if (last_sep != std::string::npos) {
        std::string dir = path.substr(0, last_sep);
        if (!std::filesystem::exists(dir)) {
            try { std::filesystem::create_directories(dir); } catch (...) {}
        }
    }

    std::ofstream f(path);
    if (!f.is_open()) {
        LOG_E("PERM", "Failed to write permissions.json");
        return false;
    }

    f << "{\n"
      << "  \"version\": 1,\n"
      << "  \"workspace_root\": \"" << workspace_root_ << "\",\n"
      << "  \"permissions\": {\n";

    for (int i = 0; i < (int)PermKey::COUNT; i++) {
        f << "    \"" << perm_key_name((PermKey)i) << "\": \"" << perm_value_str(values_[i]) << "\"";
        if (i < (int)PermKey::COUNT - 1) f << ",";
        f << "\n";
    }

    f << "  },\n"
      << "  \"limits\": {\n"
      << "    \"max_disk_mb\": " << limits_.max_disk_mb << ",\n"
      << "    \"cmd_timeout_sec\": " << limits_.cmd_timeout_sec << ",\n"
      << "    \"net_rpm\": " << limits_.net_rpm << ",\n"
      << "    \"max_subagents\": " << limits_.max_subagents << ",\n"
      << "    \"max_file_read_kb\": " << limits_.max_file_read_kb << ",\n"
      << "    \"max_remote_connections\": " << limits_.max_remote_connections << "\n"
      << "  }\n"
      << "}\n";

    f.close();
    LOG_I("PERM", "Saved permissions to %s", path.c_str());
    perm_audit_log("perm_save", "permissions.json", "ok");
    return true;
}

PermValue Permissions::get(PermKey key) const {
    int idx = (int)key;
    if (idx < 0 || idx >= (int)PermKey::COUNT) return PermValue::Deny;
    return values_[idx];
}

void Permissions::set(PermKey key, PermValue value) {
    int idx = (int)key;
    if (idx >= 0 && idx < (int)PermKey::COUNT) {
        values_[idx] = value;
        perm_audit_log("perm_set", perm_key_name(key), perm_value_str(value));
    }
}

bool Permissions::is_path_allowed(const char* path, bool write) const {
    if (!path) return false;
    std::string p(path);

    /* Normalize: lowercase drive letter, forward slashes */
    for (auto& c : p) if (c == '/') c = '\\';

    /* Check denied paths first */
    for (const auto& d : denied_paths_) {
        if (p.find(d) == 0) {
            perm_audit_log("path_check", p.c_str(), "deny:denied_path");
            return false;
        }
    }

    /* Check workspace root */
    if (!workspace_root_.empty() && p.find(workspace_root_) == 0) {
        bool ok = !write || get(PermKey::FS_WRITE_WORKSPACE) == PermValue::Allow;
        if (!ok) perm_audit_log("path_check", p.c_str(), "deny:workspace_write");
        return ok;
    }

    /* Check extra allowed paths */
    for (const auto& ea : extra_allowed_) {
        if (p.find(ea.path) == 0) {
            if (write && !ea.writable) {
                perm_audit_log("path_check", p.c_str(), "deny:read_only_extra_path");
                return false;
            }
            return true;
        }
    }

    /* Default: deny external access */
    perm_audit_log("path_check", p.c_str(), "deny:external_default");
    return false;
}

void Permissions::add_allowed_path(const char* path, bool write) {
    if (!path) return;
    extra_allowed_.push_back({path, write});
}

void Permissions::add_denied_path(const char* path) {
    if (!path) return;
    denied_paths_.push_back(path);
}
