#include "tray.h"
#include "app.h"
#include "theme.h"
#include "lang.h"
#include "SDL.h"
#include "SDL_syswm.h"
#include <shellapi.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <uxtheme.h>

#define TRAY_ICON_UID   1
#define WINDOW_CLASS    L"AnyClaw_TrayWnd"
#define ABOUT_DIALOG_CLASS L"AnyClaw_AboutDlg"
#define WM_SHOW_TRAY_MENU  (WM_USER + 100)   /* Custom: show tray menu at cursor pos */

static NOTIFYICONDATAW g_nid = {};
static HWND g_hwnd = nullptr;
static HINSTANCE g_hInst = nullptr;
static bool g_shouldQuit = false;
static TrayState g_currentState = TrayState::Gray;
static HMENU g_hMenu = nullptr;
static bool g_autoStartChecked = false;

/* ── Owner-drawn menu item data ──────────────────────────────────── */
struct MenuItemData {
    std::wstring text;   /* Wide string for proper CJK rendering */
    bool isSeparator;
    bool isDisabled;
    bool isChecked;
    UINT id;
};

static std::vector<MenuItemData> g_menuItems;

/* ── Theme colors as GDI COLORREF ────────────────────────────────── */
static COLORREF theme_bg()       { return RGB(0x1A, 0x1E, 0x2E); }
static COLORREF theme_panel()    { return RGB(0x23, 0x28, 0x38); }
static COLORREF theme_text_gdi() { return RGB(0xE0, 0xE0, 0xE0); }
static COLORREF theme_text_dim() { return RGB(0x90, 0x95, 0xB0); }
static COLORREF theme_hover()    { return RGB(0x2A, 0x30, 0x4A); }
static COLORREF theme_sep()      { return RGB(0x37, 0x3C, 0x55); }

/* ── Static brushes ──────────────────────────────────────────────── */
static HBRUSH s_bgBrush = nullptr;
static HBRUSH s_panelBrush = nullptr;

static void ensure_brushes() {
    if (!s_bgBrush) s_bgBrush = CreateSolidBrush(theme_bg());
    if (!s_panelBrush) s_panelBrush = CreateSolidBrush(theme_panel());
}

/* ── Icon generation ─────────────────────────────────────────────── */
static HICON create_color_icon(COLORREF color, int size = 16) {
    BYTE* mask = new BYTE[size * size / 8];
    memset(mask, 0xFF, size * size / 8);
    BYTE* colorBits = new BYTE[size * size * 4];
    memset(colorBits, 0, size * size * 4);

    int cx = size / 2, cy = size / 2, r = size / 2 - 1;
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int dx = x - cx, dy = y - cy;
            if (dx * dx + dy * dy <= r * r) {
                int idx = (y * size + x) * 4;
                colorBits[idx + 0] = GetBValue(color);
                colorBits[idx + 1] = GetGValue(color);
                colorBits[idx + 2] = GetRValue(color);
                colorBits[idx + 3] = 255;
                mask[(y * size + x) / 8] &= ~(1 << ((y * size + x) % 8));
            }
        }
    }

    ICONINFO ii = {};
    ii.fIcon = TRUE;
    ii.hbmMask = CreateBitmap(size, size, 1, 1, mask);
    ii.hbmColor = CreateBitmap(size, size, 1, 32, colorBits);
    HICON hIcon = CreateIconIndirect(&ii);
    DeleteObject(ii.hbmMask);
    DeleteObject(ii.hbmColor);
    delete[] mask;
    delete[] colorBits;
    return hIcon;
}

/* ── CJK font for GDI ───────────────────────────────────────────── */
static HFONT get_cjk_font(int height = -16) {
    /* Use height as key to cache multiple sizes */
    static HFONT hFont16 = nullptr;
    static HFONT hFont20 = nullptr;
    HFONT* cached = (height <= -20) ? &hFont20 : &hFont16;
    if (*cached) return *cached;

    /* List of fonts to try, ordered by preference */
    const wchar_t* names[] = {
        L"Microsoft YaHei",
        L"Microsoft YaHei UI",
        L"SimHei",
        L"WenQuanYi Micro Hei",
        L"Segoe UI",
        L"Arial",
        L"Tahoma",
        nullptr
    };

    for (int i = 0; names[i]; i++) {
        HFONT hf = CreateFontW(height, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS,
            names[i]);
        if (hf) {
            /* Verify the font can actually render CJK by testing it */
            HDC hdc = GetDC(nullptr);
            HFONT old = (HFONT)SelectObject(hdc, hf);
            TEXTMETRICW tm = {};
            GetTextMetricsW(hdc, &tm);
            SelectObject(hdc, old);
            ReleaseDC(nullptr, hdc);

            /* If the font has reasonable char width, it's likely working */
            if (tm.tmAveCharWidth > 0) {
                *cached = hf;
                return hf;
            }
            DeleteObject(hf);
        }
    }

    /* Last resort: system default */
    *cached = CreateFontW(height, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, nullptr);
    return *cached;
}

/* ── Owner-drawn menu helpers ────────────────────────────────────── */
static void append_owner_item(HMENU hMenu, UINT id, const wchar_t* text,
                               bool disabled = false, bool checked = false) {
    MenuItemData data;
    data.text = text ? text : L"";
    data.isSeparator = false;
    data.isDisabled = disabled;
    data.isChecked = checked;
    data.id = id;
    g_menuItems.push_back(data);

    MENUITEMINFOW mii = {};
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_FTYPE | MIIM_ID | MIIM_DATA | MIIM_STATE;
    mii.fType = MFT_OWNERDRAW;
    mii.wID = id;
    mii.dwItemData = (ULONG_PTR)&g_menuItems.back();
    if (disabled) mii.fState = MFS_DISABLED;
    if (checked) mii.fState |= MFS_CHECKED;
    InsertMenuItemW(hMenu, GetMenuItemCount(hMenu), TRUE, &mii);
}

static void append_owner_sep(HMENU hMenu) {
    MenuItemData data;
    data.text = L"";
    data.isSeparator = true;
    data.isDisabled = false;
    data.isChecked = false;
    data.id = 0;
    g_menuItems.push_back(data);

    MENUITEMINFOW mii = {};
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_FTYPE | MIIM_DATA;
    mii.fType = MFT_OWNERDRAW | MFT_SEPARATOR;
    mii.dwItemData = (ULONG_PTR)&g_menuItems.back();
    InsertMenuItemW(hMenu, GetMenuItemCount(hMenu), TRUE, &mii);
}

/* ── WM_MEASUREITEM ──────────────────────────────────────────────── */
static void on_measure_item(LPMEASUREITEMSTRUCT mis) {
    MenuItemData* data = (MenuItemData*)mis->itemData;
    if (!data) return;

    if (data->isSeparator) {
        mis->itemHeight = 6;
        mis->itemWidth = 80;
        return;
    }

    HDC hdc = GetDC(nullptr);
    HFONT oldFont = (HFONT)SelectObject(hdc, get_cjk_font(-12));
    SIZE sz = {};
    int len = (int)data->text.size();
    GetTextExtentPoint32W(hdc, data->text.c_str(), len, &sz);
    SelectObject(hdc, oldFont);
    ReleaseDC(nullptr, hdc);

    mis->itemWidth = (UINT)(sz.cx + 24);
    mis->itemHeight = 24;
}

/* ── WM_DRAWITEM ─────────────────────────────────────────────────── */
static void on_draw_item(LPDRAWITEMSTRUCT dis) {
    MenuItemData* data = (MenuItemData*)dis->itemData;
    if (!data) return;

    HDC hdc = dis->hDC;
    RECT rc = dis->rcItem;

    if (data->isSeparator) {
        HPEN hPen = CreatePen(PS_SOLID, 1, theme_sep());
        HPEN oldPen = (HPEN)SelectObject(hdc, hPen);
        MoveToEx(hdc, rc.left + 8, rc.top + 3, nullptr);
        LineTo(hdc, rc.right - 8, rc.top + 3);
        SelectObject(hdc, oldPen);
        DeleteObject(hPen);
        return;
    }

    bool hovered = (dis->itemState & ODS_SELECTED) != 0;
    HBRUSH hBg = CreateSolidBrush(hovered ? theme_hover() : theme_bg());
    FillRect(hdc, &rc, hBg);
    DeleteObject(hBg);

    SetTextColor(hdc, data->isDisabled ? theme_text_dim() : theme_text_gdi());
    SetBkMode(hdc, TRANSPARENT);

    if (data->isChecked) {
        RECT checkRc = rc;
        checkRc.right = checkRc.left + 18;
        DrawTextW(hdc, L"\x2713", 1, &checkRc,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    HFONT oldFont = (HFONT)SelectObject(hdc, get_cjk_font(-12));
    RECT textRc = rc;
    textRc.left += 20;
    textRc.right -= 6;
    int tlen = (int)data->text.size();
    DrawTextW(hdc, data->text.c_str(), tlen, &textRc,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    SelectObject(hdc, oldFont);
}

/* ── About Dialog WNDPROC ────────────────────────────────────────── */
static LRESULT CALLBACK about_wndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        int id = GetDlgCtrlID((HWND)lParam);
        SetBkMode(hdc, TRANSPARENT);
        switch (id) {
            case 1001: SetTextColor(hdc, RGB(100, 160, 255)); break;
            case 1002: SetTextColor(hdc, RGB(255, 200, 100)); break;
            case 1005:
            case 1006: SetTextColor(hdc, theme_text_dim()); break;
            default:   SetTextColor(hdc, theme_text_gdi()); break;
        }
        ensure_brushes();
        return (LRESULT)s_bgBrush;
    }
    case WM_CTLCOLORBTN: {
        HDC hdc = (HDC)wParam;
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, theme_text_gdi());
        ensure_brushes();
        return (LRESULT)s_panelBrush;
    }
    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hwnd, &rc);
        ensure_brushes();
        FillRect(hdc, &rc, s_bgBrush);
        return 1;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) { DestroyWindow(hwnd); return 0; }
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static void show_about_dialog(HWND parent) {
    ensure_brushes();

    int W = 420, H = 360;
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);

    static bool regDone = false;
    if (!regDone) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = about_wndproc;
        wc.hInstance = g_hInst;
        wc.lpszClassName = ABOUT_DIALOG_CLASS;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        RegisterClassW(&wc);
        regDone = true;
    }

    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        ABOUT_DIALOG_CLASS,
        g_lang == Lang::CN ? L"关于 AnyClaw" : L"About AnyClaw",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        (sw - W) / 2, (sh - H) / 2, W, H,
        parent, nullptr, g_hInst, nullptr);
    if (!hDlg) return;

    HFONT hf = get_cjk_font();
    HFONT hfBig = get_cjk_font(-20);

    /* Title */
    HWND h = CreateWindowW(L"STATIC", L"AnyClaw LVGL",
        WS_CHILD | WS_VISIBLE | SS_CENTER, 0, 25, W, 36, hDlg, (HMENU)1001, g_hInst, nullptr);
    SendMessage(h, WM_SETFONT, (WPARAM)hfBig, TRUE);

    /* Brand */
    const wchar_t* brand = (g_lang == Lang::CN) ? L"龙虾要吃蒜蓉的" : L"Garlic Lobster - Your AI Assistant";
    h = CreateWindowW(L"STATIC", brand, WS_CHILD | WS_VISIBLE | SS_CENTER,
        0, 65, W, 24, hDlg, (HMENU)1002, g_hInst, nullptr);
    SendMessage(h, WM_SETFONT, (WPARAM)hf, TRUE);

    /* Version */
    h = CreateWindowW(L"STATIC", L"Version 2.0.0", WS_CHILD | WS_VISIBLE | SS_CENTER,
        0, 95, W, 20, hDlg, (HMENU)1003, g_hInst, nullptr);
    SendMessage(h, WM_SETFONT, (WPARAM)hf, TRUE);

    /* Separator - use a simple STATIC with no special style, painted via WM_CTLCOLORSTATIC */
    HWND hSep = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
        40, 122, W - 80, 2, hDlg, (HMENU)1004, g_hInst, nullptr);

    /* Copyright */
    const wchar_t* copy = (g_lang == Lang::CN)
        ? L"Copyright \x00A9 2026 \x76AE\x5361\x4E14\x4E0E\x6770\x5C3C\x9F9F"
        : L"Copyright \x00A9 2026 Pikachu & Squirtle";
    h = CreateWindowW(L"STATIC", copy, WS_CHILD | WS_VISIBLE | SS_CENTER,
        0, 135, W, 22, hDlg, (HMENU)1005, g_hInst, nullptr);
    SendMessage(h, WM_SETFONT, (WPARAM)hf, TRUE);

    /* Email */
    h = CreateWindowW(L"STATIC", L"2443150429@qq.com", WS_CHILD | WS_VISIBLE | SS_CENTER,
        0, 162, W, 20, hDlg, (HMENU)1006, g_hInst, nullptr);
    SendMessage(h, WM_SETFONT, (WPARAM)hf, TRUE);

    /* Tech */
    h = CreateWindowW(L"STATIC", L"LVGL 9.6 + SDL2 | C++17 / Win32 API",
        WS_CHILD | WS_VISIBLE | SS_CENTER, 0, 192, W, 20, hDlg, (HMENU)1007, g_hInst, nullptr);
    SendMessage(h, WM_SETFONT, (WPARAM)hf, TRUE);

    /* OK button */
    h = CreateWindowW(L"BUTTON",
        (g_lang == Lang::CN) ? L"确定" : L"OK",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        (W - 90) / 2, H - 80, 90, 34, hDlg, (HMENU)IDOK, g_hInst, nullptr);
    SendMessage(h, WM_SETFONT, (WPARAM)hf, TRUE);
    SetWindowTheme(h, L"", L"");

    ShowWindow(hDlg, SW_SHOW);
    SetForegroundWindow(hDlg);
    EnableWindow(parent, FALSE);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_QUIT) break;
        if (!IsDialogMessage(hDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (!IsWindow(hDlg)) break;
    }

    EnableWindow(parent, TRUE);
    SetForegroundWindow(parent);
}

/* ── Exit Confirmation Dialog ────────────────────────────────────── */
/* Store result in a global so wndproc can set it */
static INT_PTR g_exitResult = IDNO;

static LRESULT CALLBACK exit_wndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, theme_text_gdi());
        ensure_brushes();
        return (LRESULT)s_bgBrush;
    }
    case WM_CTLCOLORBTN: {
        HDC hdc = (HDC)wParam;
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, theme_text_gdi());
        ensure_brushes();
        return (LRESULT)s_panelBrush;
    }
    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hwnd, &rc);
        ensure_brushes();
        FillRect(hdc, &rc, s_bgBrush);
        return 1;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDYES:
            g_exitResult = IDYES;
            PostQuitMessage(0);
            return 0;
        case IDNO:
            g_exitResult = IDNO;
            PostQuitMessage(0);
            return 0;
        }
        break;
    case WM_CLOSE:
        g_exitResult = IDNO;
        PostQuitMessage(0);
        return 0;
    case WM_DESTROY:
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static bool show_exit_dialog(HWND parent) {
    ensure_brushes();
    g_exitResult = IDNO;

    int W = 380, H = 200;
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);

    static bool regDone = false;
    if (!regDone) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = exit_wndproc;
        wc.hInstance = g_hInst;
        wc.lpszClassName = L"AnyClaw_ExitDlg";
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        RegisterClassW(&wc);
        regDone = true;
    }

    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        L"AnyClaw_ExitDlg",
        g_lang == Lang::CN ? L"退出确认" : L"Exit Confirmation",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        (sw - W) / 2, (sh - H) / 2, W, H,
        parent, nullptr, g_hInst, nullptr);
    if (!hDlg) return false;

    HFONT hf = get_cjk_font();

    /* Message */
    const wchar_t* msgText = (g_lang == Lang::CN)
        ? L"确定要退出 AnyClaw 吗？\n\n退出后将同时停止 OpenClaw Gateway 服务。"
        : L"Are you sure you want to exit AnyClaw?\n\nExiting will also stop the OpenClaw Gateway service.";
    HWND h = CreateWindowW(L"STATIC", msgText,
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        20, 20, W - 40, 80, hDlg, (HMENU)2001, g_hInst, nullptr);
    SendMessage(h, WM_SETFONT, (WPARAM)hf, TRUE);

    /* Exit button */
    h = CreateWindowW(L"BUTTON",
        (g_lang == Lang::CN) ? L"退出" : L"Exit",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        80, 120, 90, 34, hDlg, (HMENU)IDYES, g_hInst, nullptr);
    SendMessage(h, WM_SETFONT, (WPARAM)hf, TRUE);
    SetWindowTheme(h, L"", L"");

    /* Cancel button */
    h = CreateWindowW(L"BUTTON",
        (g_lang == Lang::CN) ? L"取消" : L"Cancel",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        210, 120, 90, 34, hDlg, (HMENU)IDNO, g_hInst, nullptr);
    SendMessage(h, WM_SETFONT, (WPARAM)hf, TRUE);
    SetWindowTheme(h, L"", L"");

    ShowWindow(hDlg, SW_SHOW);
    SetForegroundWindow(hDlg);
    EnableWindow(parent, FALSE);

    MSG m;
    while (GetMessageW(&m, nullptr, 0, 0)) {
        if (m.message == WM_QUIT) break;
        if (!IsDialogMessage(hDlg, &m)) {
            TranslateMessage(&m);
            DispatchMessageW(&m);
        }
        if (!IsWindow(hDlg)) break;
    }

    if (IsWindow(hDlg)) DestroyWindow(hDlg);

    EnableWindow(parent, TRUE);
    SetForegroundWindow(parent);
    return (g_exitResult == IDYES);
}

/* ── Create tray menu ────────────────────────────────────────────── */
static HMENU create_tray_menu(TrayState state) {
    g_menuItems.clear();
    HMENU hMenu = CreatePopupMenu();

    const wchar_t* statusText = L"\x25CF Status (Unknown)";
    if (g_lang == Lang::CN) {
        statusText = L"\x25CF OpenClaw \x2014 \x672A\x77E5";
        switch (state) {
            case TrayState::Green:  statusText = L"\x25CF OpenClaw \x2014 \x8FD0\x884C\x4E2D"; break;
            case TrayState::Yellow: statusText = L"\x25CF OpenClaw \x2014 \x68C0\x6D4B\x4E2D..."; break;
            case TrayState::Red:    statusText = L"\x25CF OpenClaw \x2014 \x5F02\x5E38"; break;
            case TrayState::Gray:   statusText = L"\x25CF OpenClaw \x2014 \x672A\x914D\x7F6E"; break;
        }
    } else {
        switch (state) {
            case TrayState::Green:  statusText = L"\x25CF Status (Running)"; break;
            case TrayState::Yellow: statusText = L"\x25CF Status (Checking...)"; break;
            case TrayState::Red:    statusText = L"\x25CF Status (Error)"; break;
            case TrayState::Gray:   statusText = L"\x25CF Status (Not Configured)"; break;
        }
    }
    append_owner_item(hMenu, IDM_TRAY_BASE, statusText, true);
    append_owner_sep(hMenu);
    append_owner_item(hMenu, IDM_OPEN_SETTINGS,
        g_lang == Lang::CN ? L"\x6253\x5F00\x8BBE\x7F6E" : L"Settings");
    append_owner_item(hMenu, IDM_RESTART_OC,
        g_lang == Lang::CN ? L"\x91CD\x542F OpenClaw" : L"Restart OpenClaw");
    append_owner_item(hMenu, IDM_VIEW_LOGS,
        g_lang == Lang::CN ? L"\x67E5\x770B\x65E5\x5FD7" : L"Logs");
    append_owner_sep(hMenu);
    append_owner_item(hMenu, IDM_AUTO_START,
        g_lang == Lang::CN ? L"\x5F00\x673A\x81EA\x542F" : L"Auto Start",
        false, g_autoStartChecked);
    append_owner_sep(hMenu);
    append_owner_item(hMenu, IDM_ABOUT,
        g_lang == Lang::CN ? L"\x5173\x4E8E" : L"About");
    append_owner_item(hMenu, IDM_EXIT,
        g_lang == Lang::CN ? L"\x9000\x51FA" : L"Exit");

    return hMenu;
}

/* ── Tray window procedure ───────────────────────────────────────── */
static LRESULT CALLBACK tray_wndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_TRAYICON:
        switch (LOWORD(lParam)) {
        case WM_RBUTTONUP: {
            g_autoStartChecked = is_auto_start();
            POINT pt;
            GetCursorPos(&pt);
            if (g_hMenu) { DestroyMenu(g_hMenu); g_hMenu = nullptr; }
            g_menuItems.clear();
            g_hMenu = create_tray_menu(g_currentState);
            SetForegroundWindow(hwnd);
            UINT flags = TPM_RIGHTALIGN | TPM_BOTTOMALIGN | TPM_RETURNCMD;
            int cmd = TrackPopupMenu(g_hMenu, flags, pt.x, pt.y, 0, hwnd, nullptr);
            PostMessage(hwnd, WM_NULL, 0, 0);
            if (cmd > 0) SendMessage(hwnd, WM_COMMAND, cmd, 0);
            return 0;
        }
        case WM_LBUTTONDBLCLK:
            tray_show_window(true);
            return 0;
        }
        return 0;

    case WM_MEASUREITEM:
        on_measure_item((LPMEASUREITEMSTRUCT)lParam);
        return TRUE;

    case WM_DRAWITEM:
        on_draw_item((LPDRAWITEMSTRUCT)lParam);
        return TRUE;

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
            tray_balloon("OpenClaw",
                (g_lang == Lang::CN) ? "\xE6\xAD\xA3\xE5\x9C\xA8\xE9\x87\x8D\xE5\x90\xAF Gateway..." : "Restarting Gateway...", 3000);
            break;
        case IDM_VIEW_LOGS:
            tray_show_window(true);
            break;
        case IDM_AUTO_START:
            g_autoStartChecked = !g_autoStartChecked;
            set_auto_start(g_autoStartChecked);
            break;
        case IDM_ABOUT:
            show_about_dialog(hwnd);
            break;
        case IDM_EXIT: {
            extern bool is_exit_confirmation_enabled();
            if (is_exit_confirmation_enabled()) {
                if (!show_exit_dialog(hwnd)) return 0;
            }
            g_shouldQuit = true;
            return 0;
        }
        }
        return 0;

    case WM_SHOW_TRAY_MENU: {
        POINT pt;
        GetCursorPos(&pt);
        g_autoStartChecked = is_auto_start();
        if (g_hMenu) { DestroyMenu(g_hMenu); g_hMenu = nullptr; }
        g_menuItems.clear();
        g_hMenu = create_tray_menu(g_currentState);
        SetForegroundWindow(hwnd);
        UINT fl = TPM_RIGHTALIGN | TPM_BOTTOMALIGN | TPM_RETURNCMD;
        int cmd = TrackPopupMenu(g_hMenu, fl, pt.x, pt.y, 0, hwnd, nullptr);
        PostMessage(hwnd, WM_NULL, 0, 0);
        if (cmd > 0) SendMessage(hwnd, WM_COMMAND, cmd, 0);
        return 0;
    }

    case WM_DESTROY:
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

/* ── Public API ──────────────────────────────────────────────────── */
bool tray_init(HINSTANCE hInstance) {
    g_hInst = hInstance;

    WNDCLASSW wc = {};
    wc.lpfnWndProc = tray_wndproc;
    wc.hInstance = hInstance;
    wc.lpszClassName = WINDOW_CLASS;
    if (!RegisterClassW(&wc)) {
        printf("[TRAY] Failed to register window class\n");
        return false;
    }

    g_hwnd = CreateWindowExW(0, WINDOW_CLASS, L"AnyClaw_Tray",
                             0, 0, 0, 0, 0,
                             HWND_MESSAGE, nullptr, hInstance, nullptr);
    if (!g_hwnd) {
        printf("[TRAY] Failed to create message window\n");
        return false;
    }

    memset(&g_nid, 0, sizeof(g_nid));
    g_nid.cbSize = sizeof(NOTIFYICONDATAW);
    g_nid.hWnd = g_hwnd;
    g_nid.uID = TRAY_ICON_UID;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = create_color_icon(RGB(180, 180, 180));
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
    if (s_bgBrush) { DeleteObject(s_bgBrush); s_bgBrush = nullptr; }
    if (s_panelBrush) { DeleteObject(s_panelBrush); s_panelBrush = nullptr; }
    printf("[TRAY] Tray cleaned up\n");
}

void tray_set_state(TrayState state) {
    if (state == g_currentState) return;
    g_currentState = state;

    COLORREF color;
    const wchar_t* tip;
    switch (state) {
        case TrayState::Green:  color = RGB(0, 200, 0);    tip = (g_lang == Lang::CN) ? L"AnyClaw \x2014 \x8FD0\x884C\x4E2D" : L"AnyClaw \x2014 Running"; break;
        case TrayState::Yellow: color = RGB(255, 200, 0);   tip = (g_lang == Lang::CN) ? L"AnyClaw \x2014 \x68C0\x6D4B\x4E2D" : L"AnyClaw \x2014 Checking"; break;
        case TrayState::Red:    color = RGB(220, 40, 40);   tip = (g_lang == Lang::CN) ? L"AnyClaw \x2014 \x5F02\x5E38" : L"AnyClaw \x2014 Error"; break;
        case TrayState::Gray:   color = RGB(180, 180, 180); tip = (g_lang == Lang::CN) ? L"AnyClaw \x2014 \x672A\x914D\x7F6E" : L"AnyClaw \x2014 Not Configured"; break;
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
    wchar_t wtitle[256] = {0}, wmsg[512] = {0};
    MultiByteToWideChar(CP_UTF8, 0, title, -1, wtitle, 256);
    MultiByteToWideChar(CP_UTF8, 0, message, -1, wmsg, 512);
    wcscpy_s(g_nid.szInfoTitle, wtitle);
    wcscpy_s(g_nid.szInfo, wmsg);
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
    printf("[TRAY] Balloon: %s - %s\n", title, message);
}

void tray_show_window(bool show) {
    extern SDL_Window* app_get_window();
    SDL_Window* sdlWin = app_get_window();
    if (!sdlWin) return;

    HWND hwnd = (HWND)SDL_GetWindowData(sdlWin, "HWND");
    if (!hwnd) {
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

void tray_show_menu_at(int x, int y) {
    g_autoStartChecked = is_auto_start();
    if (g_hMenu) { DestroyMenu(g_hMenu); g_hMenu = nullptr; }
    g_menuItems.clear();
    g_hMenu = create_tray_menu(g_currentState);

    /* We need a message loop to handle the owner-drawn menu.
       Post a custom message to trigger menu display. */
    SetForegroundWindow(g_hwnd);

    UINT flags = TPM_RIGHTALIGN | TPM_BOTTOMALIGN | TPM_RETURNCMD;
    int cmd = TrackPopupMenu(g_hMenu, flags, x, y, 0, g_hwnd, nullptr);
    PostMessage(g_hwnd, WM_NULL, 0, 0);

    if (cmd > 0) {
        SendMessage(g_hwnd, WM_COMMAND, cmd, 0);
    }
}
