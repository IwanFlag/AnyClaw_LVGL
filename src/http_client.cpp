#include "app.h"
#include "app_log.h"
#include "paths.h"
#include <windows.h>
#include <winhttp.h>
#include <cstdio>
#include <cstring>
#include <vector>

/* Global flag: set by UI watchdog to abort streaming (atomic) */
extern volatile LONG g_stream_done;

/* ── Internal: perform HTTP request and return status code ── */
static int http_request(const char* method, const char* url,
                        const char* body, int body_len,
                        const char* extra_headers,
                        char* response, int resp_size,
                        int timeout_sec,
                        void (*stream_cb)(const char* chunk, void* ctx),
                        void* stream_ctx) {
    if (!url) return -1;
    if (response) response[0] = '\0';

    LOG_D("HTTP", "%s %s", method, url);

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
        WINHTTP_ACCESS_TYPE_NO_PROXY,   /* bypass proxy for localhost */
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return -1;

    WinHttpSetTimeouts(hSession, timeout_sec * 1000, timeout_sec * 1000,
                       timeout_sec * 1000,
                       stream_cb ? 10000 : timeout_sec * 1000);  /* Shorter receive timeout for SSE */

    /* Enable decompression for all content types */
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

    /* Add headers */
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
        LOG_E("HTTP", "WinHttpSendRequest failed: err=%lu", GetLastError());
    } else if (!WinHttpReceiveResponse(hRequest, nullptr)) {
        LOG_E("HTTP", "WinHttpReceiveResponse failed: err=%lu", GetLastError());
    } else {
        DWORD statusCode = 0;
        DWORD size = sizeof(statusCode);
        WinHttpQueryHeaders(hRequest,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &size,
            WINHTTP_NO_HEADER_INDEX);
        status = static_cast<int>(statusCode);

        /* Log response headers for debugging */
        wchar_t resp_headers[4096] = {};
        DWORD hs = sizeof(resp_headers);
        if (WinHttpQueryHeaders(hRequest,
            WINHTTP_QUERY_RAW_HEADERS_CRLF,
            WINHTTP_HEADER_NAME_BY_INDEX, resp_headers, &hs,
            WINHTTP_NO_HEADER_INDEX)) {
            char resp_utf8[4096] = {};
            WideCharToMultiByte(CP_UTF8, 0, resp_headers, -1, resp_utf8, sizeof(resp_utf8), nullptr, nullptr);
            LOG_I("HTTP", "Response %d, headers: %.200s", status, resp_utf8);
        } else {
            LOG_I("HTTP", "Response %d (no headers)", status);
        }

        /* Read response — with SSE line buffering for stream_cb */
        std::vector<char> full_body;
        std::string line_buf;  /* accumulates partial SSE lines */
        DWORD avail = 0;
        DWORD total_bytes = 0;
        int chunk_count = 0;
        while (true) {
            /* FIX: Check abort flag — allows watchdog to kill stuck streams */
            if (g_stream_done) {
                LOG_W("HTTP", "Stream abort: g_stream_done set, exiting read loop");
                break;
            }

            if (!WinHttpQueryDataAvailable(hRequest, &avail) || avail == 0) break;
            std::vector<char> buf(avail + 1);
            DWORD read = 0;
            WinHttpReadData(hRequest, buf.data(), avail, &read);
            if (read == 0) break;
            buf[read] = '\0';
            total_bytes += read;
            chunk_count++;

            if (stream_cb) {
                /* Append chunk to line buffer, dispatch complete lines */
                line_buf.append(buf.data(), read);
                size_t pos;
                while ((pos = line_buf.find('\n')) != std::string::npos) {
                    std::string line = line_buf.substr(0, pos + 1);  /* include \n */
                    line_buf.erase(0, pos + 1);
                    if (!line.empty()) {
                        stream_cb(line.c_str(), stream_ctx);
                    }
                }
            }
            full_body.insert(full_body.end(), buf.begin(), buf.begin() + read);
        }
        /* Flush any remaining data in line buffer (last chunk without \n) */
        if (stream_cb && !line_buf.empty()) {
            stream_cb(line_buf.c_str(), stream_ctx);
        }

        LOG_D("HTTP", "Response body: %lu bytes in %d chunks", total_bytes, chunk_count);

        if (response && !full_body.empty()) {
            full_body.push_back('\0');
            strncpy(response, full_body.data(), resp_size - 1);
            response[resp_size - 1] = '\0';
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return status;
}

/* Simple HTTP GET using WinHTTP */
int http_get(const char* url, char* response, int resp_size, int timeout_sec) {
    return http_request("GET", url, nullptr, 0, nullptr, response, resp_size, timeout_sec, nullptr, nullptr);
}

/* HTTP POST with JSON body */
int http_post(const char* url, const char* json_body, const char* api_key,
              char* response, int resp_size, int timeout_sec) {
    char headers[1024];
    if (api_key && api_key[0]) {
        snprintf(headers, sizeof(headers),
                 "Content-Type: application/json\r\nAuthorization: Bearer %s", api_key);
    } else {
        snprintf(headers, sizeof(headers), "Content-Type: application/json");
    }
    int body_len = json_body ? (int)strlen(json_body) : 0;
    return http_request("POST", url, json_body, body_len, headers,
                        response, resp_size, timeout_sec, nullptr, nullptr);
}

/* HTTP POST with SSE streaming — calls cb for each chunk received */
int http_post_stream(const char* url, const char* json_body, const char* api_key,
                     void (*stream_cb)(const char* chunk, void* ctx), void* stream_ctx,
                     int timeout_sec) {
    char headers[1024];
    if (api_key && api_key[0]) {
        snprintf(headers, sizeof(headers),
                 "Content-Type: application/json\r\n"
                 "Accept: text/event-stream\r\n"
                 "Authorization: Bearer %s", api_key);
    } else {
        snprintf(headers, sizeof(headers),
                 "Content-Type: application/json\r\n"
                 "Accept: text/event-stream");
    }
    int body_len = json_body ? (int)strlen(json_body) : 0;
    DWORD tick_start = GetTickCount();
    LOG_I("HTTP", "SSE stream start: url=%.120s body_len=%d timeout=%ds", url, body_len, timeout_sec);
    int result = http_request("POST", url, json_body, body_len, headers,
                        nullptr, 0, timeout_sec, stream_cb, stream_ctx);
    DWORD elapsed = GetTickCount() - tick_start;
    LOG_I("HTTP", "SSE stream end: status=%d elapsed=%lums", result, elapsed);
    return result;
}
