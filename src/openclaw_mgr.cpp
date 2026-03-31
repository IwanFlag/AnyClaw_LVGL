#include "app.h"
#include <windows.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include <shlobj.h>
#include <winhttp.h>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

/* ── Helpers ──────────────────────────────────────────────── */

static bool exec_cmd(const char* cmd, char* output, int out_size, DWORD timeout_ms = 5000) {
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE hRead = nullptr, hWrite = nullptr;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) return false;
    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.hStdOutput = hWrite;
    si.hStdError  = hWrite;
    si.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION pi{};

    std::string full = std::string("cmd /C \"") + cmd + "\"";
    std::vector<char> buf(full.begin(), full.end());
    buf.push_back('\0');

    if (!CreateProcessA(nullptr, buf.data(), nullptr, nullptr, TRUE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        CloseHandle(hRead);
        CloseHandle(hWrite);
        return false;
    }

    CloseHandle(hWrite);

    std::string result;
    char readBuf[4096];
    DWORD bytesRead = 0;
    while (ReadFile(hRead, readBuf, sizeof(readBuf), &bytesRead, nullptr) && bytesRead > 0) {
        result.append(readBuf, bytesRead);
    }

    WaitForSingleObject(pi.hProcess, timeout_ms);
    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hRead);

    /* Trim */
    while (!result.empty() && (result.back() == '\r' || result.back() == '\n' || result.back() == ' '))
        result.pop_back();

    strncpy(output, result.c_str(), out_size - 1);
    output[out_size - 1] = '\0';
    return exitCode == 0;
}

static bool is_process_running(const char* process_name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32 pe{};
    pe.dwSize = sizeof(pe);

    std::wstring wname(process_name, process_name + strlen(process_name));

    if (Process32First(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, wname.c_str()) == 0) {
                CloseHandle(snap);
                return true;
            }
        } while (Process32Next(snap, &pe));
    }
    CloseHandle(snap);
    return false;
}

static int extract_int(const std::string& content, const std::string& key) {
    auto pos = content.find("\"" + key + "\"");
    if (pos == std::string::npos) return -1;
    pos = content.find(':', pos);
    if (pos == std::string::npos) return -1;
    pos++;
    while (pos < content.size() && (content[pos] == ' ' || content[pos] == '\t')) pos++;
    int val = 0;
    while (pos < content.size() && content[pos] >= '0' && content[pos] <= '9') {
        val = val * 10 + (content[pos] - '0');
        pos++;
    }
    return val;
}

static void extract_string(const std::string& content, const std::string& key, char* out, int out_size) {
    auto pos = content.find("\"" + key + "\"");
    if (pos == std::string::npos) { out[0] = '\0'; return; }
    pos = content.find(':', pos);
    if (pos == std::string::npos) { out[0] = '\0'; return; }
    pos = content.find('"', pos + 1);
    if (pos == std::string::npos) { out[0] = '\0'; return; }
    auto end = content.find('"', pos + 1);
    if (end == std::string::npos) { out[0] = '\0'; return; }
    std::string val = content.substr(pos + 1, end - pos - 1);
    strncpy(out, val.c_str(), out_size - 1);
    out[out_size - 1] = '\0';
}

/* ── Detect ───────────────────────────────────────────────── */

OpenClawInfo app_detect_openclaw() {
    OpenClawInfo info;
    char output[1024] = {0};

    /* Strategy 1: PATH lookup */
    if (exec_cmd("where openclaw", output, sizeof(output)) && output[0]) {
        /* Take first line */
        char* nl = strchr(output, '\n');
        if (nl) *nl = '\0';
        while (strlen(output) > 0 && (output[strlen(output)-1] == '\r' || output[strlen(output)-1] == '\n'))
            output[strlen(output)-1] = '\0';

        if (fs::exists(output)) {
            strncpy(info.executable, output, sizeof(info.executable)-1);

            std::string dir = fs::path(output).parent_path().string();
            strncpy(info.install_dir, dir.c_str(), sizeof(info.install_dir)-1);

            std::string cfg = dir + "\\openclaw.json";
            strncpy(info.config_file, cfg.c_str(), sizeof(info.config_file)-1);

            /* Read config for port */
            std::ifstream f(cfg);
            if (f.is_open()) {
                std::string content((std::istreambuf_iterator<char>(f)),
                                     std::istreambuf_iterator<char>());
                int port = extract_int(content, "port");
                if (port > 0) info.gateway_port = port;
                extract_string(content, "version", info.version, sizeof(info.version));
            }

            /* Get version from CLI */
            if (!info.version[0]) {
                if (exec_cmd("openclaw --version", output, sizeof(output))) {
                    strncpy(info.version, output, sizeof(info.version)-1);
                }
            }
            return info;
        }
    }

    /* Strategy 2: npm global */
    const char* appdata = std::getenv("APPDATA");
    if (appdata) {
        std::string npm_path = std::string(appdata) + "\\npm\\node_modules\\openclaw\\openclaw.cmd";
        if (fs::exists(npm_path)) {
            strncpy(info.executable, npm_path.c_str(), sizeof(info.executable)-1);
            std::string dir = fs::path(npm_path).parent_path().string();
            strncpy(info.install_dir, dir.c_str(), sizeof(info.install_dir)-1);

            std::string cfg = dir + "\\openclaw.json";
            strncpy(info.config_file, cfg.c_str(), sizeof(info.config_file)-1);
            return info;
        }
    }

    return info;
}

/* ── Check Status ─────────────────────────────────────────── */

ClawStatus app_check_status(const OpenClawInfo& info) {
    bool running = is_process_running("node.exe");

    if (info.executable[0] == '\0' && !running)
        return ClawStatus::NotInstalled;

    char url[256];
    snprintf(url, sizeof(url), "http://127.0.0.1:%d/health", info.gateway_port);
    char response[4096] = {0};
    int status = http_get(url, response, sizeof(response), 3);

    if (running && status == 200) return ClawStatus::Running;
    if (running && status != 200) return ClawStatus::Error;
    if (!running && info.executable[0]) return ClawStatus::Detected;

    return ClawStatus::NotInstalled;
}

/* ── Start/Stop ───────────────────────────────────────────── */

bool app_start_gateway() {
    char output[512];
    return exec_cmd("openclaw gateway start", output, sizeof(output), 10000);
}

bool app_stop_gateway() {
    char output[512];
    return exec_cmd("openclaw gateway stop", output, sizeof(output), 10000);
}
