#ifndef TRACING_H
#define TRACING_H

/*
 * ═══════════════════════════════════════════════════════════════
 *  AnyClaw LVGL — Observability Tracing (OBS-01)
 *  Session chain tracing, tool latency, error aggregation
 *  Ring buffer (1000 events) + JSON lines file output
 * ═══════════════════════════════════════════════════════════════
 */

#include <string>
#include <vector>
#include <windows.h>

/* ── Trace Event ────────────────────────────────────────────── */
struct TraceEvent {
    std::string ts;          /* ISO-8601 timestamp with ms */
    std::string trace_id;    /* Unique trace identifier */
    std::string action;      /* Operation name (e.g. "http_post_stream") */
    std::string target;      /* Target resource (e.g. URL, session key) */
    int latency_ms;          /* Duration in milliseconds */
    std::string outcome;     /* "ok", "fail", "timeout" */
};

/* ── Trace Manager (singleton) ──────────────────────────────── */
class TraceManager {
public:
    static const int RING_BUFFER_SIZE = 1000;

    static TraceManager& instance();

    /* Start a new trace — returns unique trace_id */
    std::string start_trace();

    /* Record a completed event */
    void record_event(const std::string& trace_id,
                      const std::string& action,
                      const std::string& target,
                      int latency_ms,
                      const std::string& outcome);

    /* Get most recent n events (newest first) */
    std::vector<TraceEvent> get_recent_events(int n) const;

    /* Flush all buffered events to traces.jsonl */
    void flush_to_file();

    /* Get the trace file path */
    std::string get_trace_path() const;

private:
    TraceManager();
    ~TraceManager();
    TraceManager(const TraceManager&) = delete;
    TraceManager& operator=(const TraceManager&) = delete;

    static std::string make_timestamp();
    static std::string generate_trace_id();

    TraceEvent ring_[RING_BUFFER_SIZE];
    int write_pos_;
    int count_;
    mutable CRITICAL_SECTION cs_;
    int trace_counter_;

    void ensure_trace_dir() const;
};

/* ── RAII Trace Span ────────────────────────────────────────── */
/*
 * Records a trace event when destroyed. Outcome defaults to "ok".
 *
 * Simple usage (auto-record, ok outcome):
 *   { TraceSpan span("http_post_stream", url); ... }
 *
 * With outcome control:
 *   TraceSpan span("perm_check", cmd);
 *   bool ok = perm_check_exec(...);
 *   if (!ok) span.set_fail();
 *   // records on scope exit
 */
class TraceSpan {
public:
    TraceSpan(const std::string& action, const std::string& target = "");
    ~TraceSpan();

    void set_fail()    { outcome_ = "fail"; }
    void set_timeout() { outcome_ = "timeout"; }
    void set_outcome(const std::string& o) { outcome_ = o; }

    const std::string& trace_id() const { return trace_id_; }

private:
    std::string trace_id_;
    std::string action_;
    std::string target_;
    std::string outcome_;
    DWORD tick_start_;
};

/* ── Convenience Macros ─────────────────────────────────────── */

/* TRACE(action, target) — creates a uniquely-named TraceSpan.
 * Use this when you don't need to call set_fail()/set_timeout().
 * For outcome control, declare TraceSpan explicitly. */
#define TRACE_CONCAT_(a, b) a##b
#define TRACE_CONCAT(a, b) TRACE_CONCAT_(a, b)
#define TRACE(action, target) \
    TraceSpan TRACE_CONCAT(_tr_, __LINE__)((action), (target))

/* TRACE_RECORD — one-shot recording without RAII (for manual timing).
 * Usage:
 *   DWORD t0 = GetTickCount();
 *   ... work ...
 *   TRACE_RECORD("http_post", url, GetTickCount() - t0, "ok");
 */
#define TRACE_RECORD(action, target, ms, outcome) \
    TraceManager::instance().record_event( \
        TraceManager::instance().start_trace(), (action), (target), (ms), (outcome))

#endif /* TRACING_H */
