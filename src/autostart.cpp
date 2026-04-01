#include "app.h"
#include <windows.h>
#include <cstdio>

#define REG_RUN_KEY L"Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define REG_VALUE   L"AnyClaw_LVGL"

static void get_exe_path(wchar_t* buf, int bufSize) {
    GetModuleFileNameW(nullptr, buf, bufSize);
}

bool is_auto_start() {
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_RUN_KEY, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return false;
    }

    wchar_t path[MAX_PATH] = {0};
    DWORD size = sizeof(path);
    DWORD type = 0;
    LONG result = RegQueryValueExW(hKey, REG_VALUE, nullptr, &type, (LPBYTE)path, &size);
    RegCloseKey(hKey);

    bool exists = (result == ERROR_SUCCESS && type == REG_SZ);
    return exists;
}

void set_auto_start(bool enable) {
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_RUN_KEY, 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS) {
        printf("[AUTOSTART] Failed to open registry key\n");
        return;
    }

    if (enable) {
        wchar_t exePath[MAX_PATH] = {0};
        get_exe_path(exePath, MAX_PATH);

        /* Wrap in quotes */
        wchar_t quoted[MAX_PATH + 4] = {0};
        swprintf_s(quoted, L"\"%s\"", exePath);

        RegSetValueExW(hKey, REG_VALUE, 0, REG_SZ,
                       (const BYTE*)quoted,
                       (DWORD)((wcslen(quoted) + 1) * sizeof(wchar_t)));
        printf("[AUTOSTART] Enabled\n");
    } else {
        RegDeleteValueW(hKey, REG_VALUE);
        printf("[AUTOSTART] Disabled\n");
    }

    RegCloseKey(hKey);
}
