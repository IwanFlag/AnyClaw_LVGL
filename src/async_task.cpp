/* ═══════════════════════════════════════════════════════════════
 *  async_task.cpp — 异步任务队列实现
 *  使用 SDL 线程 + 消息队列将 HTTP/Exec 操作卸载到后台
 * ═══════════════════════════════════════════════════════════════ */
#include "async_task.h"
#include "app.h"
#include "app_log.h"

#include <windows.h>
#include <winhttp.h>
#include <SDL.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <cstring>
#include <ctime>

/* ═══════════════════════════════════════════════════════════════
 *  任务定义
 * ═══════════════════════════════════════════════════════════════ */
enum class TaskKind { HttpGet, HttpPost, HttpStream };

struct Task {
    TaskKind kind;
    std::string url;
    std::string body;
    std::string api_key;
    int timeout_sec = 30;
    AsyncTaskDone on_done;
    AsyncTaskStreamCB on_chunk;
    std::atomic<bool> cancelled{false};
    DWORD start_tick = 0;

    Task() = default;
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;
    Task(Task&&) = default;
    Task& operator=(Task&&) = default;
};

/* ═══════════════════════════════════════════════════════════════
 *  线程池 + 任务队列
 * ═══════════════════════════════════════════════════════════════ */
static std::mutex g_queue_mutex;
static std::condition_variable g_queue_cv;
static std::queue<std::unique_ptr<Task>> g_tasks;
static std::vector<SDL_Thread*> g_workers;
static std::atomic<bool> g_running{false};
static std::atomic<int> g_pending{0};
static std::atomic<bool> g_stream_cancel{false};

/* 完成回调队列（由 main 线程通过 LVGL timer 消费） */
static std::mutex g_done_mutex;
static std::vector<std::pair<AsyncTaskDone, AsyncTaskResult>> g_done_callbacks;

/* ═══════════════════════════════════════════════════════════════
 *  HTTP 请求核心（worker 线程中调用）
 * ═══════════════════════════════════════════════════════════════ */
static int http_request_worker(const char* method, const char* url,
                               const char* body, int body_len,
                               const char* extra_headers,
                               std::string& response, int timeout_sec,
                               AsyncTaskStreamCB stream_cb,
                               const std::atomic<bool>* cancel_flag) {
    if (!url) return -1;

    wchar_t wurl[4096];
    MultiByteToWideChar(CP_UTF8, 0, url, -1, wurl, 4096);

    URL_COMPONENTSW uc{};
    uc.dwStructSize = sizeof(uc);
    wchar_t host[256] = {};
    wchar_t wpath[2048] = {};
    uc.lpszHostName = host;
    uc.dwHostNameLength = 256;
    uc.lpszUrlPath = wpath;
    uc.dwUrlPathLength = 2048;

    if (!WinHttpCrackUrl(wurl, 0, 0, &uc)) return -1;

    wchar_t wmethod[16];
    MultiByteToWideChar(CP_UTF8, 0, method, -1, wmethod, 16);

    HINTERNET hSession = WinHttpOpen(L"AnyClaw_LVGL/1.0",
        WINHTTP_ACCESS_TYPE_NO_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return -1;

    WinHttpSetTimeouts(hSession, timeout_sec * 1000, timeout_sec * 1000,
                       timeout_sec * 1000,
                       stream_cb ? 10000 : timeout_sec * 1000);

    DWORD decompress_flag = WINHTTP_DECOMPRESSION_FLAG_ALL;
    WinHttpSetOption(hSession, WINHTTP_OPTION_DECOMPRESSION, &decompress_flag, sizeof(decompress_flag));

    HINTERNET hConnect = WinHttpConnect(hSession, host, uc.nPort, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return -1; }

    DWORD flags = (uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, wmethod, wpath,
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return -1;
    }

    wchar_t wheaders[2048] = {};
    if (extra_headers) {
        MultiByteToWideChar(CP_UTF8, 0, extra_headers, -1, wheaders, 2048);
    }

    int status = -1;
    LPCWSTR hdrs = wheaders[0] ? wheaders : WINHTTP_NO_ADDITIONAL_HEADERS;
    DWORD hdrs_len = wheaders[0] ? (DWORD)-1 : 0;

    if (!WinHttpSendRequest(hRequest, hdrs, hdrs_len,
                           (LPVOID)body, body ? body_len : 0,
                           body ? body_len : 0, 0)) {
        LOG_E("ASYNC", "WinHttpSendRequest failed: %lu", GetLastError());
    } else if (!WinHttpReceiveResponse(hRequest, nullptr)) {
        LOG_E("ASYNC", "WinHttpReceiveResponse failed: %lu", GetLastError());
    } else {
        DWORD statusCode = 0;
        DWORD size = sizeof(statusCode);
        WinHttpQueryHeaders(hRequest,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &size,
            WINHTTP_NO_HEADER_INDEX);
        status = static_cast<int>(statusCode);

        /* Read response */
        std::string line_buf;
        DWORD avail = 0;
        while (true) {
            if (cancel_flag && cancel_flag->load()) {
                LOG_W("ASYNC", "Stream cancelled by flag");
                break;
            }
            if (!WinHttpQueryDataAvailable(hRequest, &avail) || avail == 0) break;
            std::vector<char> buf(avail + 1);
            DWORD read = 0;
            WinHttpReadData(hRequest, buf.data(), avail, &read);
            if (read == 0) break;
            buf[read] = '\0';

            if (stream_cb) {
                line_buf.append(buf.data(), read);
                size_t pos;
                while ((pos = line_buf.find('\n')) != std::string::npos) {
                    std::string line = line_buf.substr(0, pos + 1);
                    line_buf.erase(0, pos + 1);
                    if (!line.empty()) stream_cb(line.c_str());
                }
            }
            response.append(buf.data(), read);
        }
        if (stream_cb && !line_buf.empty()) {
            stream_cb(line_buf.c_str());
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return status;
}

/* ═══════════════════════════════════════════════════════════════
 *  Worker 线程主循环
 * ═══════════════════════════════════════════════════════════════ */
static int worker_thread(void* data) {
    (void)data;
    LOG_I("ASYNC", "Worker thread started");

    while (g_running.load()) {
        std::unique_ptr<Task> task;
        {
            std::unique_lock<std::mutex> lock(g_queue_mutex);
            g_queue_cv.wait(lock, [] {
                return !g_tasks.empty() || !g_running.load();
            });
            if (!g_running.load() && g_tasks.empty()) break;
            if (g_tasks.empty()) continue;
            task = std::move(g_tasks.front());
            g_tasks.pop();
        }

        if (task->cancelled.load()) {
            g_pending--;
            if (task->on_done) {
                AsyncTaskResult r;
                r.status = AsyncTaskStatus::Cancelled;
                std::lock_guard<std::mutex> lock(g_done_mutex);
                g_done_callbacks.push_back({std::move(task->on_done), r});
            }
            continue;
        }

        /* Execute */
        AsyncTaskResult result;
        result.status = AsyncTaskStatus::Running;
        task->start_tick = GetTickCount();

        std::string response;
        std::string headers;

        /* Build headers */
        if (!task->api_key.empty()) {
            if (task->kind == TaskKind::HttpGet) {
                headers = "Content-Type: application/json";
            } else {
                headers = "Content-Type: application/json\r\n";
                if (task->kind == TaskKind::HttpStream) {
                    headers += "Accept: text/event-stream\r\n";
                }
                headers += "Authorization: Bearer " + task->api_key;
            }
        } else {
            headers = "Content-Type: application/json";
            if (task->kind == TaskKind::HttpStream) {
                headers += "\r\nAccept: text/event-stream";
            }
        }

        const char* method = (task->kind == TaskKind::HttpGet) ? "GET" : "POST";
        int body_len = (int)task->body.size();

        result.http_status = http_request_worker(
            method, task->url.c_str(),
            body_len > 0 ? task->body.c_str() : nullptr, body_len,
            headers.c_str(), response, task->timeout_sec,
            task->kind == TaskKind::HttpStream ? task->on_chunk : nullptr,
            task->kind == TaskKind::HttpStream ? &g_stream_cancel : nullptr
        );

        result.elapsed_ms = GetTickCount() - task->start_tick;
        result.body = std::move(response);

        if (result.http_status < 0) {
            result.status = AsyncTaskStatus::Failed;
            result.error = "HTTP request failed";
        } else {
            result.status = AsyncTaskStatus::Done;
        }

        g_pending--;

        /* Enqueue done callback */
        if (task->on_done) {
            std::lock_guard<std::mutex> lock(g_done_mutex);
            g_done_callbacks.push_back({std::move(task->on_done), result});
        }

        LOG_D("ASYNC", "Task done: %s status=%d elapsed=%ums",
              task->url.c_str(), result.http_status, result.elapsed_ms);
    }

    LOG_I("ASYNC", "Worker thread exiting");
    return 0;
}

/* ═══════════════════════════════════════════════════════════════
 *  LVGL Timer — 在主线程中消费完成回调
 * ═══════════════════════════════════════════════════════════════ */
static void async_timer_cb(lv_timer_t* timer) {
    (void)timer;
    std::vector<std::pair<AsyncTaskDone, AsyncTaskResult>> batch;
    {
        std::lock_guard<std::mutex> lock(g_done_mutex);
        batch.swap(g_done_callbacks);
    }
    for (auto& [cb, result] : batch) {
        cb(result);
    }
}

/* ═══════════════════════════════════════════════════════════════
 *  公共 API
 * ═══════════════════════════════════════════════════════════════ */
void async_init(int thread_count) {
    if (g_running.load()) return;
    g_running = true;
    g_stream_cancel = false;

    for (int i = 0; i < thread_count; i++) {
        char name[32];
        snprintf(name, sizeof(name), "async_w%d", i);
        SDL_Thread* t = SDL_CreateThread(worker_thread, name, nullptr);
        if (t) g_workers.push_back(t);
        else LOG_E("ASYNC", "Failed to create worker thread %d", i);
    }

    /* LVGL timer to poll done callbacks every 16ms (~60fps) */
    lv_timer_create(async_timer_cb, 16, nullptr);

    LOG_I("ASYNC", "Initialized %zu worker threads", g_workers.size());
}

void async_shutdown() {
    if (!g_running.load()) return;
    g_running = false;
    g_queue_cv.notify_all();

    for (auto* t : g_workers) {
        SDL_WaitThread(t, nullptr);
    }
    g_workers.clear();

    /* Flush remaining callbacks */
    {
        std::lock_guard<std::mutex> lock(g_done_mutex);
        for (auto& [cb, r] : g_done_callbacks) {
            if (cb) { r.status = AsyncTaskStatus::Cancelled; cb(r); }
        }
        g_done_callbacks.clear();
    }

    LOG_I("ASYNC", "Shutdown complete");
}

void async_http_get(const std::string& url, int timeout_sec, AsyncTaskDone done) {
    auto task = std::make_unique<Task>();
    task->kind = TaskKind::HttpGet;
    task->url = url;
    task->timeout_sec = timeout_sec;
    task->on_done = std::move(done);
    g_pending++;

    {
        std::lock_guard<std::mutex> lock(g_queue_mutex);
        g_tasks.push(std::move(task));
    }
    g_queue_cv.notify_one();
}

void async_http_post(const std::string& url, const std::string& json_body,
                     const std::string& api_key, int timeout_sec,
                     AsyncTaskDone done) {
    auto task = std::make_unique<Task>();
    task->kind = TaskKind::HttpPost;
    task->url = url;
    task->body = json_body;
    task->api_key = api_key;
    task->timeout_sec = timeout_sec;
    task->on_done = std::move(done);
    g_pending++;

    {
        std::lock_guard<std::mutex> lock(g_queue_mutex);
        g_tasks.push(std::move(task));
    }
    g_queue_cv.notify_one();
}

void async_http_stream(const std::string& url, const std::string& json_body,
                       const std::string& api_key, int timeout_sec,
                       AsyncTaskStreamCB on_chunk, AsyncTaskDone on_done) {
    g_stream_cancel = false; /* Reset cancel for new stream */

    auto task = std::make_unique<Task>();
    task->kind = TaskKind::HttpStream;
    task->url = url;
    task->body = json_body;
    task->api_key = api_key;
    task->timeout_sec = timeout_sec;
    task->on_chunk = std::move(on_chunk);
    task->on_done = std::move(on_done);
    g_pending++;

    {
        std::lock_guard<std::mutex> lock(g_queue_mutex);
        g_tasks.push(std::move(task));
    }
    g_queue_cv.notify_one();
}

void async_cancel_streams() {
    g_stream_cancel = true;
    LOG_I("ASYNC", "All streams cancelled");
}

int async_pending_count() {
    return g_pending.load();
}
