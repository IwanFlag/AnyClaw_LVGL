/*
 * ═══════════════════════════════════════════════════════════════
 *  AnyClaw LVGL — Migration Implementation
 *  Uses Win32 + shell commands for zip operations
 * ═══════════════════════════════════════════════════════════════
 */

#include "migration.h"
#include "app.h"
#include "app_log.h"
#include "paths.h"

#include <windows.h>
#include <shlobj.h>
#include <cstring>
#include <cstdio>
#include <string>
#include <fstream>
#include <filesystem>

/* Get user data directory */
static std::string mig_get_userdata_dir() {
    const char* appdata = std::getenv("APPDATA");
    if (appdata) return std::string(appdata) + "\\" + USERDATA_DIR_NAME;
    return ".";
}

/* Get OpenClaw home directory */
static std::string mig_get_openclaw_dir() {
    const char* up = std::getenv("USERPROFILE");
    if (up) return std::string(up) + "\\.openclaw";
    return "";
}

/* Generate manifest.json content */
static std::string mig_generate_manifest() {
    char hostname[256] = {0};
    DWORD len = sizeof(hostname);
    GetComputerNameA(hostname, &len);

    SYSTEMTIME st;
    GetLocalTime(&st);

    char buf[1024];
    snprintf(buf, sizeof(buf),
        "{\n"
        "  \"version\": \"2.0.1\",\n"
        "  \"type\": \"AnyClaw_Migration\",\n"
        "  \"hostname\": \"%s\",\n"
        "  \"export_time\": \"%04d-%02d-%02d %02d:%02d:%02d\",\n"
        "  \"format_version\": 1\n"
        "}\n",
        hostname,
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    return buf;
}

/* Count files in a directory tree */
static int count_files(const std::string& dir) {
    int count = 0;
    try {
        for (auto& p : std::filesystem::recursive_directory_iterator(dir)) {
            if (p.is_regular_file()) count++;
        }
    } catch (...) {}
    return count;
}

/* ═══ Export ═══ */

bool migration_export(const char* output_path, char* err_msg, int err_size) {
    if (!output_path || !output_path[0]) {
        if (err_msg) snprintf(err_msg, err_size, "No output path specified");
        return false;
    }

    std::string userdata = mig_get_userdata_dir();
    std::string openclaw = mig_get_openclaw_dir();

    /* Check what exists */
    bool has_userdata = std::filesystem::exists(userdata);
    bool has_openclaw = std::filesystem::exists(openclaw);

    if (!has_userdata && !has_openclaw) {
        if (err_msg) snprintf(err_msg, err_size, "No data to export");
        return false;
    }

    /* Create temp staging directory */
    std::string staging = userdata + "\\_migration_staging";
    std::filesystem::remove_all(staging);
    std::filesystem::create_directories(staging);

    /* Write manifest */
    {
        std::ofstream f(staging + "\\manifest.json");
        if (f.is_open()) {
            f << mig_generate_manifest();
            f.close();
        }
    }

    /* Copy AnyClaw config (excluding sensitive data) */
    if (has_userdata) {
        std::string dest = staging + "\\anyclaw";
        std::filesystem::create_directories(dest);

        /* Copy config.json but strip API keys */
        std::string config_src = userdata + "\\" + FILE_CONFIG;
        if (std::filesystem::exists(config_src)) {
            std::ifstream in(config_src);
            std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            in.close();

            /* Redact api_key field */
            size_t pos = content.find("\"api_key\"");
            if (pos != std::string::npos) {
                size_t val_start = content.find(':', pos);
                if (val_start != std::string::npos) {
                    val_start++;
                    while (val_start < content.size() && content[val_start] == ' ') val_start++;
                    if (val_start < content.size() && content[val_start] == '"') {
                        size_t val_end = content.find('"', val_start + 1);
                        if (val_end != std::string::npos) {
                            content.replace(val_start, val_end - val_start + 1, "\"\"");
                        }
                    }
                }
            }

            std::ofstream out(dest + "\\" + FILE_CONFIG);
            out << content;
            out.close();
        }

        /* Copy accepted.json */
        std::string acc_src = userdata + "\\" + FILE_ACCEPTED;
        if (std::filesystem::exists(acc_src)) {
            std::filesystem::copy_file(acc_src, dest + "\\" + FILE_ACCEPTED,
                std::filesystem::copy_options::overwrite_existing);
        }

        /* Copy logs directory */
        std::string logs_src = userdata + "\\" + DIR_LOGS;
        if (std::filesystem::exists(logs_src)) {
            std::filesystem::copy(logs_src, dest + "\\" + DIR_LOGS,
                std::filesystem::copy_options::recursive |
                std::filesystem::copy_options::overwrite_existing);
        }
    }

    /* Copy OpenClaw workspace (AGENTS.md, SOUL.md, MEMORY.md, etc.) */
    if (has_openclaw) {
        std::string ws_src = openclaw + "\\workspace";
        if (std::filesystem::exists(ws_src)) {
            std::string ws_dest = staging + "\\openclaw\\workspace";
            std::filesystem::copy(ws_src, ws_dest,
                std::filesystem::copy_options::recursive |
                std::filesystem::copy_options::overwrite_existing);
        }

        /* Copy openclaw.json (redact tokens) */
        std::string oc_config = openclaw + "\\openclaw.json";
        if (std::filesystem::exists(oc_config)) {
            std::ifstream in(oc_config);
            std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            in.close();

            /* Redact token field */
            size_t pos = content.find("\"token\"");
            if (pos != std::string::npos) {
                size_t val_start = content.find(':', pos);
                if (val_start != std::string::npos) {
                    val_start++;
                    while (val_start < content.size() && content[val_start] == ' ') val_start++;
                    if (val_start < content.size() && content[val_start] == '"') {
                        size_t val_end = content.find('"', val_start + 1);
                        if (val_end != std::string::npos) {
                            content.replace(val_start, val_end - val_start + 1, "\"__REDACTED__\"");
                        }
                    }
                }
            }
            /* Also redact apiKey fields */
            size_t search_pos = 0;
            while ((pos = content.find("\"apiKey\"", search_pos)) != std::string::npos) {
                size_t val_start = content.find(':', pos);
                if (val_start != std::string::npos) {
                    val_start++;
                    while (val_start < content.size() && content[val_start] == ' ') val_start++;
                    if (val_start < content.size() && content[val_start] == '"') {
                        size_t val_end = content.find('"', val_start + 1);
                        if (val_end != std::string::npos) {
                            content.replace(val_start, val_end - val_start + 1, "\"__REDACTED__\"");
                            search_pos = val_start + 14;
                        } else break;
                    } else break;
                } else break;
            }

            std::ofstream out(staging + "\\openclaw\\openclaw.json");
            out << content;
            out.close();
        }
    }

    /* Zip the staging directory using PowerShell */
    {
        char cmd[2048];
        snprintf(cmd, sizeof(cmd),
            "powershell -Command \"Compress-Archive -Path '%s\\*' -DestinationPath '%s' -Force\" 2>&1",
            staging.c_str(), output_path);

        FILE* pipe = _popen(cmd, "r");
        if (pipe) {
            char buf[256];
            while (fgets(buf, sizeof(buf), pipe)) {}
            _pclose(pipe);
        }
    }

    /* Verify zip was created */
    if (!std::filesystem::exists(output_path)) {
        if (err_msg) snprintf(err_msg, err_size, "Failed to create zip file");
        std::filesystem::remove_all(staging);
        return false;
    }

    /* Clean up staging */
    std::filesystem::remove_all(staging);

    int file_count = count_files(std::filesystem::path(output_path).parent_path().string());
    auto file_size = std::filesystem::file_size(output_path);

    LOG_I("Migration", "Exported to %s (%llu bytes)", output_path, (unsigned long long)file_size);
    return true;
}

/* ═══ Pre-flight Check ═══ */

bool migration_preflight_check(char* err_msg, int err_size) {
    /* Check Node.js */
    FILE* pipe = _popen("node --version 2>&1", "r");
    if (pipe) {
        char buf[64] = {0};
        fgets(buf, sizeof(buf), pipe);
        _pclose(pipe);
        if (buf[0] != 'v') {
            if (err_msg) snprintf(err_msg, err_size, "Node.js not found. Install from https://github.com/IwanFlag/AnyClaw_Tools/releases or https://nodejs.org/");
            return false;
        }
    }

    /* Check disk space (need >500MB) */
    ULARGE_INTEGER free_bytes;
    if (GetDiskFreeSpaceExA("C:\\", &free_bytes, nullptr, nullptr)) {
        if (free_bytes.QuadPart < 500ULL * 1024 * 1024) {
            if (err_msg) snprintf(err_msg, err_size, "Insufficient disk space (need >500MB)");
            return false;
        }
    }

    return true;
}

/* ═══ Import ═══ */

bool migration_import(const char* input_path, char* err_msg, int err_size) {
    if (!input_path || !input_path[0]) {
        if (err_msg) snprintf(err_msg, err_size, "No input file specified");
        return false;
    }

    if (!std::filesystem::exists(input_path)) {
        if (err_msg) snprintf(err_msg, err_size, "File not found: %s", input_path);
        return false;
    }

    /* Pre-flight check */
    if (!migration_preflight_check(err_msg, err_size)) {
        return false;
    }

    std::string userdata = mig_get_userdata_dir();
    std::string openclaw = mig_get_openclaw_dir();

    /* Create temp extraction directory */
    std::string extract_dir = userdata + "\\_migration_import";
    std::filesystem::remove_all(extract_dir);
    std::filesystem::create_directories(extract_dir);

    /* Extract zip using PowerShell */
    {
        char cmd[2048];
        snprintf(cmd, sizeof(cmd),
            "powershell -Command \"Expand-Archive -Path '%s' -DestinationPath '%s' -Force\" 2>&1",
            input_path, extract_dir.c_str());

        FILE* pipe = _popen(cmd, "r");
        if (pipe) {
            char buf[256];
            while (fgets(buf, sizeof(buf), pipe)) {}
            _pclose(pipe);
        }
    }

    /* Verify manifest */
    std::string manifest_path = extract_dir + "\\manifest.json";
    if (!std::filesystem::exists(manifest_path)) {
        if (err_msg) snprintf(err_msg, err_size, "Invalid migration package (no manifest.json)");
        std::filesystem::remove_all(extract_dir);
        return false;
    }

    /* Copy AnyClaw config */
    std::string anyclaw_src = extract_dir + "\\anyclaw";
    if (std::filesystem::exists(anyclaw_src)) {
        std::filesystem::create_directories(userdata);
        for (auto& entry : std::filesystem::directory_iterator(anyclaw_src)) {
            std::string dest = userdata + "\\" + entry.path().filename().string();
            if (entry.is_directory()) {
                std::filesystem::copy(entry.path(), dest,
                    std::filesystem::copy_options::recursive |
                    std::filesystem::copy_options::overwrite_existing);
            } else if (entry.is_regular_file()) {
                std::filesystem::copy_file(entry.path(), dest,
                    std::filesystem::copy_options::overwrite_existing);
            }
        }
    }

    /* Copy OpenClaw workspace */
    std::string oc_ws_src = extract_dir + "\\openclaw\\workspace";
    if (std::filesystem::exists(oc_ws_src)) {
        std::string oc_ws_dest = openclaw + "\\workspace";
        std::filesystem::create_directories(oc_ws_dest);
        for (auto& entry : std::filesystem::recursive_directory_iterator(oc_ws_src)) {
            if (entry.is_regular_file()) {
                std::string rel = std::filesystem::relative(entry.path(), oc_ws_src).string();
                std::string dest = oc_ws_dest + "\\" + rel;
                std::filesystem::create_directories(std::filesystem::path(dest).parent_path());
                std::filesystem::copy_file(entry.path(), dest,
                    std::filesystem::copy_options::overwrite_existing);
            }
        }
    }

    /* Clean up extraction directory */
    std::filesystem::remove_all(extract_dir);

    LOG_I("Migration", "Imported from %s", input_path);
    return true;
}
