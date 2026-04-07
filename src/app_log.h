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
 *  Windows: %APPDATA%\AnyClaw_LVGL\logs\app.log
 *  Linux:   ./logs/app.log
 *  轮转: 1MB阈值, 最多5个文件, 启动清理7天旧文件
 * ═══════════════════════════════════════════════════════════════ */

/* Log levels */
enum AppLogLevel { LOG_DEBUG = 0, LOG_INFO = 1, LOG_WARN = 2, LOG_ERROR = 3 };

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
    return app_log_dir() + "\\app.log";
#else
    return app_log_dir() + "/app.log";
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

/* Rotate log files: app.log.4 <- .3 <- .2 <- .1 <- app.log */
static inline void app_log_rotate() {
    std::string base = app_log_dir();
#ifdef _WIN32
    base += "\\app.log";
#else
    base += "/app.log";
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
    HANDLE hFind = FindFirstFileA((dir + "\\app.log.*").c_str(), &fd);
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
        if (name.find("app.log.") == 0) {
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

/* Initialize log system — call at app startup */
static inline void app_log_init() {
    std::string path = app_log_file();
    long sz = get_file_size(path);
    if (sz > 1024 * 1024) {
        app_log_rotate();
    }
    app_log_cleanup_old();

    if (!g_log_enabled) return;
    FILE* f = fopen(path.c_str(), "a");
    if (!f) return;
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    fprintf(f, "\n═══════════════════════════════════════════════════════════\n");
    fprintf(f, "  AnyClaw LVGL — Session started %04d-%02d-%02d %02d:%02d:%02d\n",
            t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
    fprintf(f, "═══════════════════════════════════════════════════════════\n");
    fclose(f);
}

/* Check if a log level should be written */
static inline bool log_level_is_enabled(AppLogLevel level) {
    return g_log_enabled && level >= g_log_level;
}

/* ═══════════════════════════════════════════════════════════════
 *  Core log function
 * ═══════════════════════════════════════════════════════════════ */
static inline void app_log(AppLogLevel level, const char* module, const char* fmt, ...) {
    if (!g_log_enabled || level < g_log_level) return;

    /* Check rotation on each write */
    static int write_count = 0;
    if (++write_count % 100 == 0) {
        if (get_file_size(app_log_file()) > 1024 * 1024) {
            app_log_rotate();
        }
    }

    static const char* level_str[] = {"DEBUG", "INFO ", "WARN ", "ERROR"};

    /* Get time with milliseconds */
#ifdef _WIN32
    SYSTEMTIME st;
    GetLocalTime(&st);
    int ms = st.wMilliseconds;
    int yr = st.wYear, mo = st.wMonth, dy = st.wDay;
    int hr = st.wHour, mi = st.wMinute, se = st.wSecond;
#else
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm* t = localtime(&ts.tv_sec);
    int ms = (int)(ts.tv_nsec / 1000000);
    int yr = t->tm_year+1900, mo = t->tm_mon+1, dy = t->tm_mday;
    int hr = t->tm_hour, mi = t->tm_min, se = t->tm_sec;
#endif

    /* Format message */
    char msg_buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg_buf, sizeof(msg_buf), fmt, args);
    va_end(args);

    /* Write to file */
    FILE* f = fopen(app_log_file().c_str(), "a");
    if (f) {
        fprintf(f, "[%04d-%02d-%02d %02d:%02d:%02d.%03d] [%s] [%-5s] %s\n",
                yr, mo, dy, hr, mi, se, ms, level_str[level], module, msg_buf);
        fclose(f);
    }

    /* Also write to stdout */
    fprintf(stdout, "[%s] [%-5s] %s\n", level_str[level], module, msg_buf);
}

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

/* Convenience macros */
#define LOG_D(module, fmt, ...) app_log(LOG_DEBUG, module, fmt, ##__VA_ARGS__)
#define LOG_I(module, fmt, ...) app_log(LOG_INFO,  module, fmt, ##__VA_ARGS__)
#define LOG_W(module, fmt, ...) app_log(LOG_WARN,  module, fmt, ##__VA_ARGS__)
#define LOG_E(module, fmt, ...) app_log(LOG_ERROR, module, fmt, ##__VA_ARGS__)

#endif /* APP_LOG_H */
