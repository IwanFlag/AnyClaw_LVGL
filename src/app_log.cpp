/* ═══════════════════════════════════════════════════════════════
 *  app_log.cpp — 缓冲日志实现
 *  写入内存缓冲 + 后台刷盘线程，避免每次写都 fopen/fclose
 * ═══════════════════════════════════════════════════════════════ */
#include "app_log.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <cstdio>

/* ── 日志缓冲（线程安全） ── */
static std::mutex         g_log_mtx;
static std::condition_variable g_log_cv;
static std::vector<char>   g_log_buf;          /* 累积的日志行 */
static bool                 g_log_shutdown = false;
static const size_t         g_log_buf_flush_threshold = 8192; /* 8KB 触发刷盘 */

/* ── 刷盘线程 ── */
static void log_flush_thread() {
    std::unique_lock<std::mutex> lock(g_log_mtx);
    while (true) {
        /* 等待信号：500ms 周期 或 缓冲满 或 shutdown */
        g_log_cv.wait_for(lock, std::chrono::milliseconds(500), [&] {
            return g_log_shutdown || g_log_buf.size() >= g_log_buf_flush_threshold;
        });

        if (g_log_buf.empty() && !g_log_shutdown) continue;

        /* 取出当前缓冲内容 */
        std::vector<char> to_write = std::move(g_log_buf);
        g_log_buf.clear();
        lock.unlock();

        if (!to_write.empty()) {
            FILE* f = fopen(app_log_file().c_str(), "ab");
            if (f) {
                fwrite(to_write.data(), 1, to_write.size(), f);
                fclose(f);
            }
        }

        if (g_log_shutdown) break;
        lock.lock();
    }
}

/* ── 公开 API ── */
void app_log_init() {
    app_log_rotate();
    app_log_cleanup_old();

    /* 写启动分隔符（直接写，因为 init 只调一次） */
    std::string path = app_log_file();
    FILE* f = fopen(path.c_str(), "ab");
    if (f) {
        time_t now = time(NULL);
        struct tm* t = localtime(&now);
        fprintf(f, "\n═══════════════════════════════════════════════════════════\n");
        fprintf(f, "  AnyClaw LVGL — Session started %04d-%02d-%02d %02d:%02d:%02d\n",
                t->tm_year+1900, t->tm_mon+1, t->tm_mday,
                t->tm_hour, t->tm_min, t->tm_sec);
        fprintf(f, "═══════════════════════════════════════════════════════════\n");
        fclose(f);
    }

    /* 启动刷盘线程 */
    std::thread(log_flush_thread).detach();
}

void app_log_shutdown() {
    {
        std::lock_guard<std::mutex> lock(g_log_mtx);
        g_log_shutdown = true;
        g_log_cv.notify_all();
    }
}

/* ── 核心写函数（被 LOG_X 宏调用）── */
void app_log(AppLogLevel level, const char* module, const char* fmt, ...) {
#if !ANYCLAW_LOG_ENABLED
    (void)level; (void)module; (void)fmt;
    return;
#else
    if (!g_log_enabled || level < g_log_level) return;

    static const char* level_str[] = {"DEBUG", "INFO ", "WARN ", "ERROR"};

    /* 时间戳 */
#ifdef _WIN32
    SYSTEMTIME st;
    GetLocalTime(&st);
    int ms = st.wMilliseconds;
    char ts_buf[32];
    snprintf(ts_buf, sizeof(ts_buf), "%04d-%02d-%02d %02d:%02d:%02d.%03d",
             st.wYear, st.wMonth, st.wDay,
             st.wHour, st.wMinute, st.wSecond, ms);
#else
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm* t = localtime(&ts.tv_sec);
    int ms = (int)(ts.tv_nsec / 1000000);
    char ts_buf[32];
    snprintf(ts_buf, sizeof(ts_buf), "%04d-%02d-%02d %02d:%02d:%02d.%03d",
             t->tm_year+1900, t->tm_mon+1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec, ms);
#endif

    /* 格式化消息 */
    char msg_buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg_buf, sizeof(msg_buf), fmt, args);
    va_end(args);

    /* 拼装完整一行 */
    char line_buf[1024];
    int len = snprintf(line_buf, sizeof(line_buf),
                        "[%s] [%s] [%-5s] %s\n",
                        ts_buf, level_str[level], module, msg_buf);

    /* 写 stdout（同步，保持实时可见） */
    fprintf(stdout, "[%s] [%-5s] %s\n", level_str[level], module, msg_buf);

    /* 追加到缓冲（触发刷盘） */
    {
        std::lock_guard<std::mutex> lock(g_log_mtx);
        g_log_buf.insert(g_log_buf.end(), line_buf, line_buf + len);
        /* 缓冲满则立即唤醒刷盘线程 */
        if (g_log_buf.size() >= g_log_buf_flush_threshold) {
            g_log_cv.notify_all();
        }
    }
#endif
}
