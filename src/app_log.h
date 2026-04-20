#ifndef APP_LOG_H
#define APP_LOG_H

#include <cstdio>
#include <ctime>
#include <cstdarg>
#include <cstring>
#include <cstdlib>
#include <string>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#else
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#endif

/* ═══════════════════════════════════════════════════════════════
 *  日志系统 — 跨平台日志模块
 *  Windows: %APPDATA%\AnyClaw_LVGL\logs\anyclaw_app.log
 *  Linux:   ./logs/anyclaw_app.log
 *  轮转: 1MB阈值, 最多5个文件, 启动清理7天旧文件
 * ═══════════════════════════════════════════════════════════════ */

/* Log levels */
enum AppLogLevel { LOG_DEBUG = 0, LOG_INFO = 1, LOG_WARN = 2, LOG_ERROR = 3 };

/* Compile-time logging switches
 * ANYCLAW_LOG_ENABLED: 1 enable / 0 disable all logs
 * ANYCLAW_LOG_COMPILE_LEVEL: compile-in minimum level (default: LOG_DEBUG = highest verbosity)
 */
#ifndef ANYCLAW_LOG_ENABLED
#define ANYCLAW_LOG_ENABLED 1
#endif

#ifndef ANYCLAW_LOG_COMPILE_LEVEL
#define ANYCLAW_LOG_COMPILE_LEVEL LOG_DEBUG
#endif

/* Runtime config — set from config.json */
extern bool g_log_enabled;
extern int  g_log_level;

/* ═══════════════════════════════════════════════════════════════
 *  Cross-platform file helpers
 * ═══════════════════════════════════════════════════════════════ */

static inline void plat_create_dir(const std::string& path) {
#ifdef _WIN32
    CreateDirectoryA(path.c_str(), NULL);
#else
    mkdir(path.c_str(), 0755);
#endif
}

static inline void plat_delete_file(const std::string& path) {
#ifdef _WIN32
    DeleteFileA(path.c_str());
#else
    remove(path.c_str());
#endif
}

static inline bool plat_move_file(const std::string& from, const std::string& to) {
#ifdef _WIN32
    return MoveFileExA(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING) != 0;
#else
    return rename(from.c_str(), to.c_str()) == 0;
#endif
}

/* ═══════════════════════════════════════════════════════════════
 *  Log directory / file paths
 * ═══════════════════════════════════════════════════════════════ */

static inline std::string app_log_dir() {
    static std::string dir;
    if (!dir.empty()) return dir;
#ifdef _WIN32
    char appdata[MAX_PATH];
    if (SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appdata) == S_OK) {
        dir = std::string(appdata) + "\\AnyClaw_LVGL\\logs";
        plat_create_dir(std::string(appdata) + "\\AnyClaw_LVGL");
        plat_create_dir(dir);
    } else
#endif
    {
        dir = "logs";
        plat_create_dir(dir);
    }
    return dir;
}

/* Log file path — cross-platform separator */
static inline std::string app_log_file() {
#ifdef _WIN32
    return app_log_dir() + "\\anyclaw_app.log";
#else
    return app_log_dir() + "/anyclaw_app.log";
#endif
}

/* Get file size */
static inline long get_file_size(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fclose(f);
    return sz;
}

/* Rotate log files: anyclaw_app.log.4 <- .3 <- .2 <- .1 <- anyclaw_app.log */
static inline void app_log_rotate() {
    std::string base = app_log_dir();
#ifdef _WIN32
    base += "\\anyclaw_app.log";
#else
    base += "/anyclaw_app.log";
#endif
    /* Delete oldest */
    plat_delete_file(base + ".4");
    /* Shift */
    for (int i = 3; i >= 1; i--) {
        std::string from = base + "." + std::to_string(i);
        std::string to   = base + "." + std::to_string(i + 1);
        plat_move_file(from, to);
    }
    plat_move_file(base, base + ".1");
}

/* Clean old rotated logs (>7 days) */
static inline void app_log_cleanup_old() {
    std::string dir = app_log_dir();
    time_t now = time(NULL);
#ifdef _WIN32
    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA((dir + "\\anyclaw_app.log.*").c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;
    FILETIME ft_now;
    GetSystemTimeAsFileTime(&ft_now);
    ULARGE_INTEGER nowUL, fileUL;
    nowUL.LowPart = ft_now.dwLowDateTime;
    nowUL.HighPart = ft_now.dwHighDateTime;
    do {
        fileUL.LowPart = fd.ftLastWriteTime.dwLowDateTime;
        fileUL.HighPart = fd.ftLastWriteTime.dwHighDateTime;
        ULONGLONG diff = nowUL.QuadPart - fileUL.QuadPart;
        if (diff > 6048000000000ULL) { /* 7 days in 100ns units */
            DeleteFileA((dir + "\\" + fd.cFileName).c_str());
        }
    } while (FindNextFileA(hFind, &fd));
    FindClose(hFind);
#else
    DIR* d = opendir(dir.c_str());
    if (!d) return;
    struct dirent* ent;
    while ((ent = readdir(d)) != NULL) {
        std::string name = ent->d_name;
        if (name.find("anyclaw_app.log.") == 0) {
            std::string full = dir + "/" + name;
            struct stat st;
            if (stat(full.c_str(), &st) == 0) {
                double age = difftime(now, st.st_mtime);
                if (age > 7 * 86400) { /* 7 days */
                    remove(full.c_str());
                }
            }
        }
    }
    closedir(d);
#endif
}

/* Initialize log system — call at app startup（实现在 app_log.cpp） */
extern void app_log_init();

/* Check if a log level should be written */
static inline bool log_level_is_enabled(AppLogLevel level) {
    return g_log_enabled && level >= g_log_level;
}

/* ═══════════════════════════════════════════════════════════════
 *  Core log function（实现在 app_log.cpp）
 *  写缓冲队列 + 后台刷盘线程，主线程零阻塞
 * ═══════════════════════════════════════════════════════════════ */
extern void app_log(AppLogLevel level, const char* module, const char* fmt, ...);

/* 初始化日志系统（启动刷盘线程，在 main() 最早调用） */
extern void app_log_init();

/* 关闭日志系统（退出前调用，强制刷最后一批） */
extern void app_log_shutdown();

/* ═══════════════════════════════════════════════════════════════
 *  Log export — copy current log file to a target path
 *  Returns: number of bytes written, or -1 on error
 * ═══════════════════════════════════════════════════════════════ */
static inline long app_log_export(const std::string& dest_path) {
    std::string src = app_log_file();
    FILE* fsrc = fopen(src.c_str(), "rb");
    if (!fsrc) return -1;
    FILE* fdst = fopen(dest_path.c_str(), "wb");
    if (!fdst) { fclose(fsrc); return -1; }

    char buf[4096];
    long total = 0;
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fsrc)) > 0) {
        fwrite(buf, 1, n, fdst);
        total += (long)n;
    }
    fclose(fsrc);
    fclose(fdst);
    return total;
}

/* ═══════════════════════════════════════════════════════════════
 *  Log clear — truncate the current log file
 * ═══════════════════════════════════════════════════════════════ */
static inline void app_log_clear() {
    std::string path = app_log_file();
    FILE* f = fopen(path.c_str(), "w");
    if (f) {
        time_t now = time(NULL);
        struct tm* t = localtime(&now);
        fprintf(f, "═══════════════════════════════════════════════════════════\n");
        fprintf(f, "  AnyClaw LVGL — Log cleared %04d-%02d-%02d %02d:%02d:%02d\n",
                t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
        fprintf(f, "═══════════════════════════════════════════════════════════\n");
        fclose(f);
    }
}

/* ═══════════════════════════════════════════════════════════════
 *  Get log level display name (for UI)
 * ═══════════════════════════════════════════════════════════════ */
static inline const char* app_log_level_name(AppLogLevel level) {
    static const char* names[] = {"DEBUG", "INFO", "WARN", "ERROR"};
    if (level >= 0 && level <= 3) return names[level];
    return "???";
}

/* Convenience macros (compile-time filtered) */
#if ANYCLAW_LOG_ENABLED && (ANYCLAW_LOG_COMPILE_LEVEL <= LOG_DEBUG)
#define LOG_D(module, fmt, ...) app_log(LOG_DEBUG, module, fmt, ##__VA_ARGS__)
#else
#define LOG_D(module, fmt, ...) ((void)0)
#endif

#if ANYCLAW_LOG_ENABLED && (ANYCLAW_LOG_COMPILE_LEVEL <= LOG_INFO)
#define LOG_I(module, fmt, ...) app_log(LOG_INFO, module, fmt, ##__VA_ARGS__)
#else
#define LOG_I(module, fmt, ...) ((void)0)
#endif

#if ANYCLAW_LOG_ENABLED && (ANYCLAW_LOG_COMPILE_LEVEL <= LOG_WARN)
#define LOG_W(module, fmt, ...) app_log(LOG_WARN, module, fmt, ##__VA_ARGS__)
#else
#define LOG_W(module, fmt, ...) ((void)0)
#endif

#if ANYCLAW_LOG_ENABLED && (ANYCLAW_LOG_COMPILE_LEVEL <= LOG_ERROR)
#define LOG_E(module, fmt, ...) app_log(LOG_ERROR, module, fmt, ##__VA_ARGS__)
#else
#define LOG_E(module, fmt, ...) ((void)0)
#endif

#endif /* APP_LOG_H */
