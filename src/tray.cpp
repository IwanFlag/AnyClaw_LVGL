#include "tray.h"
#include "app.h"
#include <shellapi.h>
#include <cstdio>
#include <cstring>
#include "SDL.h"
#include "SDL_syswm.h"

#define TRAY_ICON_UID   1
#define WINDOW_CLASS    L"AnyClaw_TrayWnd"

static NOTIFYICONDATAW g_nid = {};
static HWND g_hwnd = nullptr;
static HINSTANCE g_hInst = nullptr;
static bool g_shouldQuit = false;
static TrayState g_currentState = TrayState::Gray;
static HMENU g_hMenu = nullptr;
static bool g_autoStartChecked = false;

/* ── Icon generation ─────────────────────────────────────────────────── */
static HICON create_color_icon(COLORREF color, int size = 16) {
    int half = size * size / 2;
    /* 1bpp mask (all transparent) */
    BYTE* mask = new BYTE[size * size / 8];
    memset(mask, 0xFF, size * size / 8);  /* all 1 = transparent */

    /* 32bpp color bitmap */
    BYTE* colorBits = new BYTE[size * size * 4];
    memset(colorBits, 0, size * size * 4);

    /* Fill a circle shape */
    int cx = size / 2, cy = size / 2, r = size / 2 - 1;
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int dx = x - cx, dy = y - cy;
            if (dx * dx + dy * dy <= r * r) {
                int idx = (y * size + x) * 4;
                colorBits[idx + 0] = GetBValue(color);  /* B */
                colorBits[idx + 1] = GetGValue(color);  /* G */
                colorBits[idx + 2] = GetRValue(color);  /* R */
                colorBits[idx + 3] = 255;               /* A */
                /* Clear mask bit for visible pixel */
                mask[(y * size + x) / 8] &= ~(1 << ((y * size + x) % 8));
            }
        }
    }

    ICONINFO ii = {};
    ii.fIcon = TRUE;
    ii.xHotspot = 0;
    ii.yHotspot = 0;
    ii.hbmMask = CreateBitmap(size, size, 1, 1, mask);
    ii.hbmColor = CreateBitmap(size, size, 1, 32, colorBits);

    HICON hIcon = CreateIconIndirect(&ii);

    DeleteObject(ii.hbmMask);
    DeleteObject(ii.hbmColor);
    delete[] mask;
    delete[] colorBits;
    return hIcon;
}

/* ── Context menu ────────────────────────────────────────────────────── */
static HMENU create_tray_menu(TrayState state) {
    HMENU hMenu = CreatePopupMenu();

    /* Status display item (disabled) */
    const char* statusText = "● OpenClaw — 未知";
    switch (state) {
        case TrayState::Green:  statusText = "● OpenClaw — 运行中"; break;
        case TrayState::Yellow: statusText = "● OpenClaw — 检测中..."; break;
        case TrayState::Red:    statusText = "● OpenClaw — 异常"; break;
        case TrayState::Gray:   statusText = "● OpenClaw — 未配置"; break;
    }
    AppendMenuA(hMenu, MF_STRING | MF_GRAYED, IDM_TRAY_BASE, statusText);
    AppendMenuA(hMenu, MF_SEPARATOR, 0, nullptr);

    AppendMenuA(hMenu, MF_STRING, IDM_OPEN_SETTINGS, "打开设置 / Settings");
    AppendMenuA(hMenu, MF_STRING, IDM_RESTART_OC, "重启 OpenClaw");
    AppendMenuA(hMenu, MF_STRING, IDM_VIEW_LOGS, "查看日志 / Logs");
    AppendMenuA(hMenu, MF_SEPARATOR, 0, nullptr);

    /* Auto-start checkbox - read from registry */
    UINT flags = MF_STRING;
    if (g_autoStartChecked) flags |= MF_CHECKED;
    AppendMenuA(hMenu, flags, IDM_AUTO_START, "开机自启 / Auto Start");
    AppendMenuA(hMenu, MF_SEPARATOR, 0, nullptr);

    AppendMenuA(hMenu, MF_STRING, IDM_ABOUT, "关于 / About");
    AppendMenuA(hMenu, MF_STRING, IDM_EXIT, "退出 / Exit");

    return hMenu;
}

/* ── Window procedure ────────────────────────────────────────────────── */
static LRESULT CALLBACK tray_wndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_TRAYICON:
        switch (LOWORD(lParam)) {
        case WM_RBUTTONUP: {
            /* Update auto-start state from registry before showing menu */
            g_autoStartChecked = is_auto_start();

            POINT pt;
            GetCursorPos(&pt);
            if (g_hMenu) DestroyMenu(g_hMenu);
            g_hMenu = create_tray_menu(g_currentState);

            /* Required to make menu disappear on outside click */
            SetForegroundWindow(hwnd);
            TrackPopupMenu(g_hMenu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN,
                           pt.x, pt.y, 0, hwnd, nullptr);
            /* Per MSDN: post to handle menu cleanup */
            PostMessage(hwnd, WM_NULL, 0, 0);
            return 0;
        }
        case WM_LBUTTONDBLCLK:
            tray_show_window(true);
            return 0;
        }
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDM_OPEN_SETTINGS:
            ui_settings_open();
            tray_show_window(true);
            break;
        case IDM_RESTART_OC:
            app_stop_gateway();
            Sleep(1000);
            app_start_gateway();
            tray_balloon("OpenClaw", "正在重启 Gateway...", 3000);
            break;
        case IDM_VIEW_LOGS:
            tray_show_window(true);
            /* Focus on log tab if exists */
            break;
        case IDM_AUTO_START:
            g_autoStartChecked = !g_autoStartChecked;
            set_auto_start(g_autoStartChecked);
            break;
        case IDM_ABOUT:
            MessageBoxA(hwnd,
                "AnyClaw LVGL v2.0\n\n"
                "OpenClaw Desktop Manager\n"
                "基于 LVGL 9.x + SDL2\n\n"
                "Copyright © 2026",
                "关于 AnyClaw",
                MB_ICONINFORMATION | MB_OK);
            break;
        case IDM_EXIT: {
            /* P2-26: Check exit confirmation toggle */
            extern bool is_exit_confirmation_enabled();
            bool need_confirm = is_exit_confirmation_enabled();
            if (need_confirm) {
                int result = MessageBoxA(hwnd,
                    "确定要退出 AnyClaw 吗？\n\n退出后将同时停止 OpenClaw Gateway 服务。",
                    "退出确认",
                    MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
                if (result != IDYES) return 0;
            }
            g_shouldQuit = true;
            return 0;
        }
        }
        return 0;

    case WM_DESTROY:
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

/* ── Public API ──────────────────────────────────────────────────────── */
bool tray_init(HINSTANCE hInstance) {
    g_hInst = hInstance;

    /* Register hidden window class */
    WNDCLASSW wc = {};
    wc.lpfnWndProc = tray_wndproc;
    wc.hInstance = hInstance;
    wc.lpszClassName = WINDOW_CLASS;
    if (!RegisterClassW(&wc)) {
        printf("[TRAY] Failed to register window class\n");
        return false;
    }

    /* Create hidden message window */
    g_hwnd = CreateWindowExW(0, WINDOW_CLASS, L"AnyClaw_Tray",
                             0, 0, 0, 0, 0,
                             HWND_MESSAGE, nullptr, hInstance, nullptr);
    if (!g_hwnd) {
        printf("[TRAY] Failed to create message window\n");
        return false;
    }

    /* Add tray icon */
    memset(&g_nid, 0, sizeof(g_nid));
    g_nid.cbSize = sizeof(NOTIFYICONDATAW);
    g_nid.hWnd = g_hwnd;
    g_nid.uID = TRAY_ICON_UID;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = create_color_icon(RGB(180, 180, 180));  /* Gray */
    wcscpy_s(g_nid.szTip, L"AnyClaw LVGL");

    if (!Shell_NotifyIconW(NIM_ADD, &g_nid)) {
        printf("[TRAY] Failed to add tray icon\n");
        return false;
    }

    printf("[TRAY] Tray icon initialized\n");
    return true;
}

void tray_cleanup() {
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
    if (g_nid.hIcon) DestroyIcon(g_nid.hIcon);
    if (g_hMenu) DestroyMenu(g_hMenu);
    if (g_hwnd) DestroyWindow(g_hwnd);
    UnregisterClassW(WINDOW_CLASS, g_hInst);
    printf("[TRAY] Tray cleaned up\n");
}

void tray_set_state(TrayState state) {
    if (state == g_currentState) return;
    g_currentState = state;

    COLORREF color;
    const wchar_t* tip;
    switch (state) {
        case TrayState::Green:  color = RGB(0, 200, 0);   tip = L"AnyClaw — 运行中"; break;
        case TrayState::Yellow: color = RGB(255, 200, 0);  tip = L"AnyClaw — 检测中"; break;
        case TrayState::Red:    color = RGB(220, 40, 40);  tip = L"AnyClaw — 异常"; break;
        case TrayState::Gray:   color = RGB(180, 180, 180); tip = L"AnyClaw — 未配置"; break;
    }

    HICON hNewIcon = create_color_icon(color);
    if (g_nid.hIcon) DestroyIcon(g_nid.hIcon);
    g_nid.hIcon = hNewIcon;
    wcscpy_s(g_nid.szTip, tip);
    g_nid.uFlags = NIF_ICON | NIF_TIP;
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}

void tray_balloon(const char* title, const char* message, int timeout_ms) {
    g_nid.uFlags = NIF_INFO;
    g_nid.dwInfoFlags = NIIF_INFO;
    g_nid.uTimeout = timeout_ms;

    /* Convert to wide */
    wchar_t wtitle[256] = {0}, wmsg[512] = {0};
    MultiByteToWideChar(CP_UTF8, 0, title, -1, wtitle, 256);
    MultiByteToWideChar(CP_UTF8, 0, message, -1, wmsg, 512);
    wcscpy_s(g_nid.szInfoTitle, wtitle);
    wcscpy_s(g_nid.szInfo, wmsg);

    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
    printf("[TRAY] Balloon: %s - %s\n", title, message);
}

void tray_show_window(bool show) {
    /* We need to find the SDL window and show/hide it */
    extern SDL_Window* app_get_window();
    SDL_Window* sdlWin = app_get_window();
    if (!sdlWin) return;

    HWND hwnd = (HWND)SDL_GetWindowData(sdlWin, "HWND");
    if (!hwnd) {
        /* Try SDL_SysWMinfo approach */
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        if (SDL_GetWindowWMInfo(sdlWin, &wmInfo)) {
            hwnd = wmInfo.info.win.window;
        }
    }

    if (hwnd) {
        if (show) {
            ShowWindow(hwnd, SW_SHOW);
            SetForegroundWindow(hwnd);
            SDL_RaiseWindow(sdlWin);
        } else {
            ShowWindow(hwnd, SW_HIDE);
        }
    }
}

void tray_process_messages() {
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

bool tray_should_quit() {
    return g_shouldQuit;
}

HWND tray_get_hwnd() {
    return g_hwnd;
}
