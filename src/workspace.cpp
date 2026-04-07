#include "workspace.h"
#include "app.h"
#include "app_log.h"
#include "paths.h"
#include "permissions.h"
#include <windows.h>
#include <shlobj.h>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <sstream>
#include <ctime>
#include <vector>
#include <algorithm>
#include <cstdint>

/* ═══ Config helpers ═══ */

static std::string get_config_path() {
    const char* appdata = std::getenv("APPDATA");
    if (!appdata) return "";
    return std::string(appdata) + "\\AnyClaw_LVGL\\config.json";
}

static std::string get_default_workspace_root() {
    const char* userprofile = std::getenv("USERPROFILE");
    if (!userprofile) return "";
    return std::string(userprofile) + "\\.openclaw\\workspace";
}

static std::string get_lock_path() {
    std::string root = workspace_get_root();
    if (root.empty()) return "";
    return root + "\\.openclaw.lock";
}

static bool g_workspace_lock_held = false;

/* ═══ Workspace API ═══ */

std::string workspace_get_root() {
    /* Try reading from config.json */
    std::ifstream f(get_config_path());
    if (f.is_open()) {
        std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        f.close();

        char root[512] = {0};
        if (json_extract_string(content.c_str(), "workspace_root", root, sizeof(root))) {
            if (root[0]) return std::string(root);
        }
    }

    /* Fallback to default */
    return get_default_workspace_root();
}

bool workspace_set_root(const char* path) {
    if (!path || !path[0]) return false;

    std::string config_path = get_config_path();
    std::string content;

    /* Read existing config */
    {
        std::ifstream f(config_path);
        if (f.is_open()) {
            content.assign((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
            f.close();
        }
    }

    /* Simple: add/replace workspace_root field */
    /* If config is empty, create minimal */
    if (content.empty() || content.find('{') == std::string::npos) {
        content = "{\n  \"workspace_root\": \"" + std::string(path) + "\"\n}";
    } else {
        /* Check if workspace_root already exists */
        size_t pos = content.find("\"workspace_root\"");
        if (pos != std::string::npos) {
            /* Replace existing value */
            size_t val_start = content.find('"', pos + 16);
            if (val_start != std::string::npos) {
                val_start++;
                size_t val_end = content.find('"', val_start);
                if (val_end != std::string::npos) {
                    content.replace(val_start, val_end - val_start, path);
                }
            }
        } else {
            /* Insert before closing brace */
            size_t last_brace = content.rfind('}');
            if (last_brace != std::string::npos) {
                std::string insertion = "  \"workspace_root\": \"";
                insertion += path;
                insertion += "\",\n";
                content.insert(last_brace, insertion);
            }
        }
    }

    /* Write back */
    std::ofstream f(config_path);
    if (!f.is_open()) return false;
    f << content;
    f.close();

    LOG_I("WORKSPACE", "Set workspace root: %s", path);
    return true;
}

WorkspaceHealth workspace_check_health() {
    WorkspaceHealth h;
    std::string root = workspace_get_root();

    if (root.empty()) {
        snprintf(h.error_msg, sizeof(h.error_msg), "No workspace path configured");
        return h;
    }

    /* Check existence */
    h.exists = std::filesystem::exists(root);
    if (!h.exists) {
        snprintf(h.error_msg, sizeof(h.error_msg), "Directory not found: %s", root.c_str());
        return h;
    }

    /* Check writable: try creating a temp file */
    std::string test_file = root + "\\.anyclaw_test";
    {
        std::ofstream tf(test_file);
        h.writable = tf.is_open();
        if (h.writable) {
            tf.close();
            std::remove(test_file.c_str());
        }
    }

    /* Check for key files */
    h.has_agents_md = std::filesystem::exists(root + "\\AGENTS.md");
    h.has_soul_md = std::filesystem::exists(root + "\\SOUL.md");
    h.has_memory_dir = std::filesystem::exists(root + "\\memory");

    /* Check free disk space */
    try {
        auto space = std::filesystem::space(root);
        h.free_mb = (long long)(space.available / (1024 * 1024));
    } catch (...) {
        h.free_mb = -1;
    }

    if (!h.writable) {
        snprintf(h.error_msg, sizeof(h.error_msg), "No write permission: %s", root.c_str());
    } else if (h.free_mb >= 0 && h.free_mb < 100) {
        snprintf(h.error_msg, sizeof(h.error_msg), "Low disk space: %lld MB free", h.free_mb);
    }

    return h;
}

bool workspace_init(const char* root_path) {
    std::string root = root_path ? root_path : workspace_get_root();
    if (root.empty()) return false;

    /* Create directory if missing */
    if (!std::filesystem::exists(root)) {
        try {
            std::filesystem::create_directories(root);
            LOG_I("WORKSPACE", "Created workspace directory: %s", root.c_str());
        } catch (const std::exception& e) {
            LOG_E("WORKSPACE", "Failed to create directory: %s (%s)", root.c_str(), e.what());
            return false;
        }
    }

    /* Create memory/ subdirectory */
    std::string mem_dir = root + "\\memory";
    if (!std::filesystem::exists(mem_dir)) {
        try {
            std::filesystem::create_directories(mem_dir);
        } catch (...) {}
    }

    /* Create .backups/ subdirectory */
    std::string bak_dir = root + "\\.backups";
    if (!std::filesystem::exists(bak_dir)) {
        try {
            std::filesystem::create_directories(bak_dir);
        } catch (...) {}
    }

    /* Generate template files if missing */
    struct TemplateFile {
        const char* name;
        const char* content;
    };

    TemplateFile templates[] = {
        {"AGENTS.md",
            "# AGENTS.md - Your Workspace\n\n"
            "This folder is home. Treat it that way.\n\n"
            "## Session Startup\n\n"
            "Before doing anything else:\n\n"
            "1. Read `SOUL.md` — this is who you are\n"
            "2. Read `USER.md` — this is who you're helping\n"
            "3. Read `memory/YYYY-MM-DD.md` (today + yesterday) for recent context\n\n"
            "## Memory\n\n"
            "You wake up fresh each session. Files are your continuity.\n"},
        {"SOUL.md",
            "# SOUL.md - Who You Are\n\n"
            "You are an AI assistant.\n\n"
            "## Core Principles\n\n"
            "- Be genuinely helpful\n"
            "- Have opinions and personality\n"
            "- Figure things out yourself before asking\n"
            "- Earn trust through capability\n"},
        {"USER.md",
            "# USER.md - About Your Human\n\n"
            "- **Name:**\n"
            "- **What to call them:**\n"
            "- **Timezone:**\n"
            "- **Notes:**\n"},
        {"TOOLS.md",
            "# TOOLS.md - Local Notes\n\n"
            "Skills define _how_ tools work. This file is for _your_ specifics.\n"},
        {"HEARTBEAT.md",
            "# HEARTBEAT.md\n\n"
            "# Keep this file empty (or with only comments) to skip heartbeat API calls.\n"
            "# Add tasks below when you want the agent to check something periodically.\n"},
        {"IDENTITY.md",
            "# IDENTITY.md - Who Am I?\n\n"
            "- **Name:**\n"
            "- **Creature:** AI Assistant\n"
            "- **Vibe:** Helpful, direct\n"
            "- **Emoji:** 🤖\n"},
        {"MEMORY.md",
            "# MEMORY.md - Long-Term Memory\n\n"
            "This file contains curated memories and important context.\n"
            "Updated periodically from daily memory files.\n"},
        {"BOOTSTRAP.md",
            "# BOOTSTRAP.md - First Run\n\n"
            "Follow this file to set up your identity, then delete it.\n"},
    };

    for (const auto& t : templates) {
        std::string path = root + "\\" + t.name;
        if (!std::filesystem::exists(path)) {
            std::ofstream f(path);
            if (f.is_open()) {
                f << t.content;
                f.close();
                LOG_I("WORKSPACE", "Created template: %s", t.name);
            }
        }
    }

    /* Generate WORKSPACE.md */
    workspace_generate_meta();

    /* Generate workspace.json (runtime permissions config) */
    std::string ws_json_path = root + "\\workspace.json";
    if (!std::filesystem::exists(ws_json_path)) {
        std::ofstream f(ws_json_path);
        if (f.is_open()) {
            f << "{\n"
              << "  \"version\": 1,\n"
              << "  \"workspace_root\": \"" << root << "\",\n"
              << "  \"denied_paths\": [\"C:\\\\Windows\\\\\", \"C:\\\\Program Files\\\\\"],\n"
              << "  \"permissions\": {\n"
              << "    \"fs_read\": \"allow\",\n"
              << "    \"fs_write\": \"allow\",\n"
              << "    \"exec\": \"ask\",\n"
              << "    \"net_outbound\": \"allow\",\n"
              << "    \"device\": \"deny\"\n"
              << "  },\n"
              << "  \"limits\": {\n"
              << "    \"cmd_timeout_sec\": 30,\n"
              << "    \"max_subagents\": 5,\n"
              << "    \"net_rpm\": 60\n"
              << "  }\n"
              << "}\n";
            f.close();
        }
    }

    return true;
}

bool workspace_generate_meta(const char* name) {
    std::string root = workspace_get_root();
    if (root.empty()) return false;

    std::string path = root + "\\WORKSPACE.md";
    std::string ws_name = name ? name : "Default";

    /* If no name provided, try to get from directory name */
    if (!name) {
        size_t last_sep = root.find_last_of("\\/");
        if (last_sep != std::string::npos) {
            ws_name = root.substr(last_sep + 1);
        }
    }

    /* Get current date */
    char date_str[32] = {0};
    {
        time_t now = time(nullptr);
        struct tm* t = localtime(&now);
        if (t) strftime(date_str, sizeof(date_str), "%Y-%m-%d", t);
    }

    std::ofstream f(path);
    if (!f.is_open()) return false;

    f << "# WORKSPACE.md\n\n"
      << "- Workspace Name: " << ws_name << "\n"
      << "- Workspace Root: " << root << "\n"
      << "- Created: " << date_str << "\n"
      << "- AnyClaw Version: 2.0.1\n"
      << "- Workspace Version: 1\n";

    f.close();
    LOG_I("WORKSPACE", "Generated WORKSPACE.md: %s", path.c_str());
    return true;
}

std::string workspace_get_name() {
    std::string root = workspace_get_root();

    /* Try reading WORKSPACE.md for name */
    std::string ws_md = root + "\\WORKSPACE.md";
    std::ifstream f(ws_md);
    if (f.is_open()) {
        std::string line;
        while (std::getline(f, line)) {
            size_t pos = line.find("Workspace Name:");
            if (pos != std::string::npos) {
                std::string name = line.substr(pos + 15);
                /* Trim whitespace */
                while (!name.empty() && (name[0] == ' ' || name[0] == '\t')) name.erase(0, 1);
                while (!name.empty() && (name.back() == ' ' || name.back() == '\t' || name.back() == '\r' || name.back() == '\n'))
                    name.pop_back();
                if (!name.empty()) return name;
            }
        }
        f.close();
    }

    /* Fallback: directory name */
    size_t last_sep = root.find_last_of("\\/");
    if (last_sep != std::string::npos) {
        return root.substr(last_sep + 1);
    }
    return "Default";
}

bool workspace_lock_acquire() {
    std::string lock_path = get_lock_path();
    if (lock_path.empty()) return false;

    DWORD cur_pid = GetCurrentProcessId();

    /* If lock exists, check whether holder process is still alive */
    if (std::filesystem::exists(lock_path)) {
        std::ifstream in(lock_path);
        unsigned long old_pid = 0;
        if (in.is_open()) {
            in >> old_pid;
            in.close();
        }

        if (old_pid > 0 && old_pid != (unsigned long)cur_pid) {
            HANDLE h = OpenProcess(SYNCHRONIZE, FALSE, (DWORD)old_pid);
            if (h) {
                DWORD w = WaitForSingleObject(h, 0);
                CloseHandle(h);
                if (w == WAIT_TIMEOUT) {
                    LOG_W("WORKSPACE", "Workspace lock held by pid=%lu", old_pid);
                    return false;
                }
            }
            /* stale lock file: process no longer exists */
        }
    }

    std::ofstream out(lock_path, std::ios::trunc);
    if (!out.is_open()) {
        LOG_E("WORKSPACE", "Failed to create lock file: %s", lock_path.c_str());
        return false;
    }
    out << cur_pid << "\n";
    out.close();

    g_workspace_lock_held = true;
    LOG_I("WORKSPACE", "Lock acquired: %s (pid=%lu)", lock_path.c_str(), (unsigned long)cur_pid);
    return true;
}

void workspace_lock_release() {
    if (!g_workspace_lock_held) return;

    std::string lock_path = get_lock_path();
    if (lock_path.empty()) return;

    try {
        if (std::filesystem::exists(lock_path)) {
            std::filesystem::remove(lock_path);
        }
    } catch (...) {
        /* best effort */
    }
    g_workspace_lock_held = false;
    LOG_I("WORKSPACE", "Lock released: %s", lock_path.c_str());
}

bool workspace_lock_is_held() {
    return g_workspace_lock_held;
}

static const char* perm_value_name(PermValue v) {
    switch (v) {
        case PermValue::Allow: return "allow";
        case PermValue::Deny: return "deny";
        case PermValue::Ask: return "ask";
        case PermValue::ReadOnly: return "read_only";
        default: return "deny";
    }
}

static bool replace_or_append_managed(const std::string& path, const std::string& managed_body) {
    const std::string start_marker = "<!-- ANYCLAW_MANAGED_START -->";
    const std::string end_marker = "<!-- ANYCLAW_MANAGED_END -->";
    const std::string managed_block =
        start_marker + "\n" + managed_body + "\n" + end_marker + "\n";

    std::string content;
    {
        std::ifstream in(path, std::ios::binary);
        if (in.is_open()) {
            content.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            in.close();
        }
    }

    if (content.empty()) {
        content = "# Managed by AnyClaw\n\n";
    }

    size_t s = content.find(start_marker);
    size_t e = content.find(end_marker);
    if (s != std::string::npos && e != std::string::npos && e > s) {
        e += end_marker.size();
        content.replace(s, e - s, managed_block);
    } else {
        if (!content.empty() && content.back() != '\n') content.push_back('\n');
        content += "\n" + managed_block;
    }

    /* Backup before overwrite: keep latest 10 backups */
    if (std::filesystem::exists(path)) {
        std::string root = workspace_get_root();
        std::string bak_dir = root + "\\.backups";
        try { std::filesystem::create_directories(bak_dir); } catch (...) {}

        char ts[32] = {0};
        std::time_t now = std::time(nullptr);
        std::tm* lt = std::localtime(&now);
        if (lt) std::strftime(ts, sizeof(ts), "%Y%m%d_%H%M%S", lt);

        std::string base = std::filesystem::path(path).filename().string();
        std::string bak_path = bak_dir + "\\" + base + "." + ts + ".bak";
        try {
            std::filesystem::copy_file(path, bak_path, std::filesystem::copy_options::overwrite_existing);
        } catch (...) {
            /* best effort */
        }

        std::vector<std::filesystem::path> backups;
        std::string prefix = base + ".";
        for (const auto& entry : std::filesystem::directory_iterator(bak_dir)) {
            if (!entry.is_regular_file()) continue;
            std::string fn = entry.path().filename().string();
            if (fn.find(prefix) == 0 && fn.rfind(".bak") == fn.size() - 4) {
                backups.push_back(entry.path());
            }
        }
        std::sort(backups.begin(), backups.end());
        while (backups.size() > 10) {
            try { std::filesystem::remove(backups.front()); } catch (...) {}
            backups.erase(backups.begin());
        }
    }

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) return false;
    out << content;
    return out.good();
}

bool workspace_sync_managed_sections() {
    std::string root = workspace_get_root();
    if (root.empty()) return false;

    char ts[64] = {0};
    std::time_t now = std::time(nullptr);
    std::tm* lt = std::localtime(&now);
    if (lt) std::strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", lt);

    std::ostringstream oss;
    oss << "## AnyClaw Managed Policy\n"
        << "- generated_at: " << ts << "\n"
        << "- workspace_root: " << root << "\n"
        << "- denied_paths: C:\\\\Windows\\\\, C:\\\\Program Files\\\\\n\n"
        << "## Permission Snapshot\n"
        << "- exec_shell: " << perm_value_name(permissions().get(PermKey::EXEC_SHELL)) << "\n"
        << "- exec_install: " << perm_value_name(permissions().get(PermKey::EXEC_INSTALL)) << "\n"
        << "- exec_delete: " << perm_value_name(permissions().get(PermKey::EXEC_DELETE)) << "\n"
        << "- fs_read_workspace: " << perm_value_name(permissions().get(PermKey::FS_READ_WORKSPACE)) << "\n"
        << "- fs_write_workspace: " << perm_value_name(permissions().get(PermKey::FS_WRITE_WORKSPACE)) << "\n"
        << "- net_outbound: " << perm_value_name(permissions().get(PermKey::NET_OUTBOUND)) << "\n";

    bool ok_agents = replace_or_append_managed(root + "\\AGENTS.md", oss.str());
    bool ok_tools = replace_or_append_managed(root + "\\TOOLS.md", oss.str());

    if (ok_agents && ok_tools) {
        LOG_I("WORKSPACE", "Synced managed sections to AGENTS.md and TOOLS.md");
        return true;
    }
    LOG_W("WORKSPACE", "Managed section sync partial/fail: agents=%d tools=%d", ok_agents ? 1 : 0, ok_tools ? 1 : 0);
    return false;
}

static const char* runtime_perm_value(PermValue v) {
    switch (v) {
        case PermValue::Allow: return "allow";
        case PermValue::Ask: return "ask";
        default: return "deny";
    }
}

static const char* runtime_device_value() {
    PermValue vals[] = {
        permissions().get(PermKey::DEVICE_CAMERA),
        permissions().get(PermKey::DEVICE_MIC),
        permissions().get(PermKey::DEVICE_SCREEN),
        permissions().get(PermKey::DEVICE_USB_STORAGE),
        permissions().get(PermKey::DEVICE_REMOTE_NODE),
        permissions().get(PermKey::DEVICE_NEW_DEVICE),
    };
    bool has_allow = false;
    bool has_ask = false;
    for (PermValue v : vals) {
        if (v == PermValue::Allow) has_allow = true;
        if (v == PermValue::Ask) has_ask = true;
    }
    if (has_allow) return "allow";
    if (has_ask) return "ask";
    return "deny";
}

static std::string build_runtime_workspace_json(const std::string& root) {
    std::ostringstream oss;
    oss << "{\n"
        << "  \"version\": 2,\n"
        << "  \"source\": \"permissions.json\",\n"
        << "  \"workspace_root\": \"" << root << "\",\n"
        << "  \"denied_paths\": [\"C:\\\\Windows\\\\\", \"C:\\\\Program Files\\\\\"],\n"
        << "  \"permissions\": {\n"
        << "    \"fs_read\": \"" << runtime_perm_value(permissions().get(PermKey::FS_READ_WORKSPACE)) << "\",\n"
        << "    \"fs_write\": \"" << runtime_perm_value(permissions().get(PermKey::FS_WRITE_WORKSPACE)) << "\",\n"
        << "    \"exec\": \"" << runtime_perm_value(permissions().get(PermKey::EXEC_SHELL)) << "\",\n"
        << "    \"net_outbound\": \"" << runtime_perm_value(permissions().get(PermKey::NET_OUTBOUND)) << "\",\n"
        << "    \"device\": \"" << runtime_device_value() << "\"\n"
        << "  },\n"
        << "  \"limits\": {\n"
        << "    \"cmd_timeout_sec\": " << permissions().limits().cmd_timeout_sec << ",\n"
        << "    \"max_subagents\": " << permissions().limits().max_subagents << ",\n"
        << "    \"net_rpm\": " << permissions().limits().net_rpm << "\n"
        << "  }\n"
        << "}\n";
    return oss.str();
}

bool workspace_sync_runtime_config_from_permissions() {
    std::string root = workspace_get_root();
    if (root.empty()) return false;

    std::string path = root + "\\workspace.json";
    std::string next = build_runtime_workspace_json(root);

    std::string current;
    {
        std::ifstream in(path, std::ios::binary);
        if (in.is_open()) {
            current.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            in.close();
        }
    }

    if (current == next) {
        perm_audit_log("config_sync", "workspace.json", "up_to_date");
        return true;
    }

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        perm_audit_log("config_sync", "workspace.json", "write_failed");
        return false;
    }
    out << next;
    out.close();

    perm_audit_log("config_sync", "workspace.json", current.empty() ? "created" : "rebuilt");
    LOG_I("WORKSPACE", "Synced runtime config from permissions: %s", path.c_str());
    return true;
}
