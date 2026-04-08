#include "tracing.h"
#include "app_log.h"
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <shlobj.h>

/* ═══════════════════════════════════════════════════════════════
 *  Trace File Path
 * ═══════════════════════════════════════════════════════════════ */

std::string TraceManager::get_trace_path() const {
    char appdata[MAX_PATH] = {0};
    if (SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appdata) == S_OK) {
        return std::string(appdata) + "\\AnyClaw_LVGL\\traces.jsonl";
    }
    return "traces.jsonl";
}

void TraceManager::ensure_trace_dir() const {
    char appdata[MAX_PATH] = {0};
    if (SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appdata) == S_OK) {
        std::string dir = std::string(appdata) + "\\AnyClaw_LVGL";
        try { std::filesystem::create_directories(dir); } catch (...) {}
    }
}

/* ═══════════════════════════════════════════════════════════════
 *  Timestamp Generation (Windows FILETIME → ISO-8601)
 * ═══════════════════════════════════════════════════════════════ */

std::string TraceManager::make_timestamp() {
    SYSTEMTIME st;
    GetLocalTime(&st);

    char buf[32];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d.%03d",
             st.wYear, st.wMonth, st.wDay,
             st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    return std::string(buf);
}

/* ═══════════════════════════════════════════════════════════════
 *  Trace ID Generation
 * ═══════════════════════════════════════════════════════════════ */

std::string TraceManager::generate_trace_id() {
    /* Format: tr_<tick32>_<counter> */
    char buf[48];
    static int seq = 0;
    snprintf(buf, sizeof(buf), "tr_%08lx_%04x",
             (unsigned long)GetTickCount(), seq++ & 0xFFFF);
    return std::string(buf);
}

/* ═══════════════════════════════════════════════════════════════
 *  Singleton
 * ═══════════════════════════════════════════════════════════════ */

TraceManager& TraceManager::instance() {
    static TraceManager tm;
    return tm;
}

TraceManager::TraceManager()
    : write_pos_(0)
    , count_(0)
    , trace_counter_(0)
{
    InitializeCriticalSection(&cs_);
    LOG_D("TRACE", "TraceManager initialized (ring buffer: %d events)", RING_BUFFER_SIZE);
}

TraceManager::~TraceManager() {
    flush_to_file();
    DeleteCriticalSection(&cs_);
}

/* ═══════════════════════════════════════════════════════════════
 *  Core API
 * ═══════════════════════════════════════════════════════════════ */

std::string TraceManager::start_trace() {
    EnterCriticalSection(&cs_);
    std::string id = generate_trace_id();
    LeaveCriticalSection(&cs_);
    return id;
}

void TraceManager::record_event(const std::string& trace_id,
                                 const std::string& action,
                                 const std::string& target,
                                 int latency_ms,
                                 const std::string& outcome) {
    EnterCriticalSection(&cs_);

    TraceEvent& ev = ring_[write_pos_];
    ev.ts = make_timestamp();
    ev.trace_id = trace_id;
    ev.action = action;
    ev.target = target;
    ev.latency_ms = latency_ms;
    ev.outcome = outcome;

    write_pos_ = (write_pos_ + 1) % RING_BUFFER_SIZE;
    if (count_ < RING_BUFFER_SIZE) count_++;
    trace_counter_++;

    LeaveCriticalSection(&cs_);

    LOG_D("TRACE", "%s %s %s %dms %s",
          trace_id.c_str(), action.c_str(), target.c_str(), latency_ms, outcome.c_str());

    /* Auto-flush every 50 events to avoid data loss on crash */
    if (trace_counter_ % 50 == 0) {
        flush_to_file();
    }
}

std::vector<TraceEvent> TraceManager::get_recent_events(int n) const {
    std::vector<TraceEvent> result;
    if (n <= 0) return result;

    EnterCriticalSection(&cs_);

    if (n > count_) n = count_;
    result.reserve(n);

    /* Read backwards from write_pos_ (newest first) */
    for (int i = 0; i < n; i++) {
        int idx = (write_pos_ - 1 - i + RING_BUFFER_SIZE) % RING_BUFFER_SIZE;
        result.push_back(ring_[idx]);
    }

    LeaveCriticalSection(&cs_);
    return result;
}

/* ═══════════════════════════════════════════════════════════════
 *  File Output (JSON Lines)
 * ═══════════════════════════════════════════════════════════════ */

/* Escape a string for JSON (minimal: just backslash and double-quote) */
static std::string json_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        if (c == '\\') out += "\\\\";
        else if (c == '"') out += "\\\"";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else out += c;
    }
    return out;
}

void TraceManager::flush_to_file() {
    ensure_trace_dir();
    std::string path = get_trace_path();

    /* Snapshot events under lock, write after releasing */
    std::vector<TraceEvent> snapshot;

    EnterCriticalSection(&cs_);
    int total = count_;
    int start_idx;
    if (total >= RING_BUFFER_SIZE) {
        start_idx = write_pos_;
    } else {
        start_idx = 0;
    }
    snapshot.reserve(total);
    for (int i = 0; i < total; i++) {
        int idx = (start_idx + i) % RING_BUFFER_SIZE;
        snapshot.push_back(ring_[idx]);
    }
    LeaveCriticalSection(&cs_);

    /* Write snapshot to file (no lock held) */
    std::ofstream f(path, std::ios::app);
    if (!f.is_open()) {
        LOG_E("TRACE", "Failed to open trace file: %s", path.c_str());
        return;
    }

    for (const auto& ev : snapshot) {
        f << "{\"ts\":\"" << json_escape(ev.ts)
          << "\",\"trace_id\":\"" << json_escape(ev.trace_id)
          << "\",\"action\":\"" << json_escape(ev.action)
          << "\",\"target\":\"" << json_escape(ev.target)
          << "\",\"latency_ms\":" << ev.latency_ms
          << ",\"outcome\":\"" << json_escape(ev.outcome)
          << "\"}\n";
    }

    f.close();
    LOG_D("TRACE", "Flushed %d events to %s", total, path.c_str());
}

/* ═══════════════════════════════════════════════════════════════
 *  RAII TraceSpan
 * ═══════════════════════════════════════════════════════════════ */

TraceSpan::TraceSpan(const std::string& action, const std::string& target)
    : action_(action)
    , target_(target)
    , outcome_("ok")
    , tick_start_(GetTickCount())
{
    trace_id_ = TraceManager::instance().start_trace();
}

TraceSpan::~TraceSpan() {
    DWORD elapsed = GetTickCount() - tick_start_;
    TraceManager::instance().record_event(
        trace_id_, action_, target_, (int)elapsed, outcome_);
}
