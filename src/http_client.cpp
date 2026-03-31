#include "app.h"
#include <windows.h>
#include <winhttp.h>
#include <cstdio>
#include <cstring>
#include <vector>

/* Simple HTTP GET using WinHTTP */
int http_get(const char* url, char* response, int resp_size, int timeout_sec) {
    if (!url || !response || resp_size <= 0) return -1;
    response[0] = '\0';

    /* Convert URL to wide string */
    wchar_t wurl[4096];
    MultiByteToWideChar(CP_UTF8, 0, url, -1, wurl, 4096);

    /* Parse URL */
    URL_COMPONENTSW uc{};
    uc.dwStructSize = sizeof(uc);
    wchar_t host[256] = {};
    wchar_t wpath[2048] = {};
    uc.lpszHostName = host;
    uc.dwHostNameLength = 256;
    uc.lpszUrlPath = wpath;
    uc.dwUrlPathLength = 2048;

    if (!WinHttpCrackUrl(wurl, 0, 0, &uc)) return -1;

    HINTERNET hSession = WinHttpOpen(L"AnyClaw_LVGL/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return -1;

    WinHttpSetTimeouts(hSession, timeout_sec * 1000, timeout_sec * 1000,
                       timeout_sec * 1000, timeout_sec * 1000);

    HINTERNET hConnect = WinHttpConnect(hSession, host, uc.nPort, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return -1;
    }

    DWORD flags = (uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", wpath,
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return -1;
    }

    int status = -1;
    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                           WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
        if (WinHttpReceiveResponse(hRequest, nullptr)) {
            DWORD statusCode = 0;
            DWORD size = sizeof(statusCode);
            WinHttpQueryHeaders(hRequest,
                WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &size,
                WINHTTP_NO_HEADER_INDEX);
            status = static_cast<int>(statusCode);

            /* Read body */
            std::vector<char> body;
            DWORD avail = 0;
            while (WinHttpQueryDataAvailable(hRequest, &avail) && avail > 0) {
                std::vector<char> buf(avail + 1);
                DWORD read = 0;
                WinHttpReadData(hRequest, buf.data(), avail, &read);
                body.insert(body.end(), buf.begin(), buf.begin() + read);
            }
            body.push_back('\0');
            strncpy(response, body.data(), resp_size - 1);
            response[resp_size - 1] = '\0';
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return status;
}
