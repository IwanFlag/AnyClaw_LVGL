#include "workspace.h"
#include "app.h"
#include "app_log.h"
#include "paths.h"
#include <windows.h>
#include <shlobj.h>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <filesystem>

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
