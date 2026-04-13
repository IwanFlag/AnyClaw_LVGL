/* ═══════════════════════════════════════════════════════════════
 *  async_task.h — 异步任务队列（线程分离）
 *  将 HTTP/Exec 阻塞操作从 UI 线程卸载到 worker 线程
 * ═══════════════════════════════════════════════════════════════ */
#ifndef ANYCLAW_ASYNC_TASK_H
#define ANYCLAW_ASYNC_TASK_H

#include <functional>
#include <string>
#include <atomic>
#include <cstdint>

/* ═══ 任务状态 ═══ */
enum class AsyncTaskStatus : uint8_t {
    Pending,    /* 已入队 */
    Running,    /* 执行中 */
    Done,       /* 完成 */
    Failed,     /* 失败 */
    Cancelled   /* 已取消 */
};

/* ═══ 任务结果 ═══ */
struct AsyncTaskResult {
    AsyncTaskStatus status = AsyncTaskStatus::Pending;
    int http_status = -1;           /* HTTP 状态码（HTTP 任务用） */
    int exit_code = -1;             /* 进程退出码（Exec 任务用） */
    std::string body;              /* 响应体 / 输出内容 */
    std::string error;             /* 错误信息 */
    uint32_t elapsed_ms = 0;       /* 耗时 */

    bool ok() const { return status == AsyncTaskStatus::Done && http_status >= 200 && http_status < 400; }
};

/* ═══ 回调类型 ═══ */
using AsyncTaskDone = std::function<void(const AsyncTaskResult&)>;
using AsyncTaskStreamCB = std::function<void(const char* chunk)>;

/* ═══ 公共 API ═══ */

/* 初始化 worker 线程池（main() 中调用一次） */
void async_init(int thread_count = 2);

/* 关闭 worker 线程池（main() 退出时调用） */
void async_shutdown();

/* 投递异步 HTTP GET
 * url: 请求地址
 * timeout_sec: 超时秒数
 * done: 完成回调（在 LVGL timer 中安全调用） */
void async_http_get(const std::string& url, int timeout_sec, AsyncTaskDone done);

/* 投递异步 HTTP POST（JSON）
 * url: 请求地址
 * json_body: JSON 请求体
 * api_key: API Key（可选，为空不发 Authorization）
 * timeout_sec: 超时秒数
 * done: 完成回调 */
void async_http_post(const std::string& url, const std::string& json_body,
                     const std::string& api_key, int timeout_sec,
                     AsyncTaskDone done);

/* 投递异步 HTTP POST SSE 流式
 * url: 请求地址
 * json_body: JSON 请求体
 * api_key: API Key
 * on_chunk: 每个 SSE chunk 的回调（在 worker 线程中调用，注意线程安全）
 * on_done: 完成回调 */
void async_http_stream(const std::string& url, const std::string& json_body,
                       const std::string& api_key, int timeout_sec,
                       AsyncTaskStreamCB on_chunk, AsyncTaskDone on_done);

/* 取消所有正在执行的 HTTP 流 */
void async_cancel_streams();

/* 获取当前未完成任务数 */
int async_pending_count();

#endif /* ANYCLAW_ASYNC_TASK_H */
