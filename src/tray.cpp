#include "tray.h"
#include "app.h"
#include "theme.h"
#include "lang.h"
#include "app_log.h"
#include "resource.h"
#include "SDL.h"
#include "SDL_syswm.h"
#include <shellapi.h>
#include <shlobj.h>
#include <cstdio>
#include <cstring>
#include <algorithm>

/* Icon resource IDI_ICON1 = 1 (defined in resource.h, used by .rc and C++) */
#include <string>
#include <list>
#include <uxtheme.h>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

#define TRAY_ICON_UID   1
#define WINDOW_CLASS    L"AnyClaw_TrayWnd"
#define ABOUT_DIALOG_CLASS L"AnyClaw_AboutDlg"
#define WM_SHOW_TRAY_MENU  (WM_USER + 100)   /* Custom: show tray menu at cursor pos */

static NOTIFYICONDATAW g_nid = {};
static HWND g_hwnd = nullptr;
static HINSTANCE g_hInst = nullptr;
static bool g_shouldQuit = false;
static TrayState g_currentState = TrayState::White;
static HMENU g_hMenu = nullptr;
static bool g_autoStartChecked = false;

/* Thread-safe state change: background thread sets pending, main thread applies */
static volatile LONG g_stateDirty = 0;
static volatile LONG g_pendingStateVal = (LONG)TrayState::White;

/* ── Owner-drawn menu item data ──────────────────────────────────── */
struct MenuItemData {
    std::wstring text;   /* Wide string for proper CJK rendering */
    bool isSeparator;
    bool isDisabled;
    bool isChecked;
    UINT id;
    COLORREF ledColor;   /* 0 = no LED, else draw colored circle */
};

static std::list<MenuItemData> g_menuItems;

/* ── Theme-aware tray subdirectory ── */
static const wchar_t* get_tray_theme_dir() {
    switch (g_theme) {
        case Theme::Peachy:  return L"tray\\peachy\\";
        case Theme::Mochi:   return L"tray\\mochi\\";
        default:             return L"tray\\matcha\\";
    }
}

/* ── Theme colors as GDI COLORREF from g_colors ── */
static COLORREF to_colorref(lv_color_t c) {
    return RGB(c.red << 3 | c.red >> 2, c.green << 2 | c.green >> 4, c.blue << 3 | c.blue >> 2);
}
static COLORREF theme_bg()       { return to_colorref(g_colors->bg); }
static COLORREF theme_panel()    { return to_colorref(g_colors->panel); }
static COLORREF theme_text_gdi() { return to_colorref(g_colors->text); }
static COLORREF theme_text_dim() { return to_colorref(g_colors->text_dim); }
static COLORREF theme_hover()    { return to_colorref(g_colors->hover_overlay); }
static COLORREF theme_sep()      { return to_colorref(g_colors->divider); }

/* ── Static brushes ──────────────────────────────────────────────── */
static HBRUSH s_bgBrush = nullptr;
static HBRUSH s_panelBrush = nullptr;

static void ensure_brushes() {
    if (!s_bgBrush) s_bgBrush = CreateSolidBrush(theme_bg());
    if (!s_panelBrush) s_panelBrush = CreateSolidBrush(theme_panel());
}

/* ── GDI+ initialization ─────────────────────────────────────────── */
static ULONG_PTR g_gdiplusToken = 0;
static void ensure_gdiplus() {
    static bool initialized = false;
    if (!initialized) {
        Gdiplus::GdiplusStartupInput si;
        Gdiplus::GdiplusStartup(&g_gdiplusToken, &si, nullptr);
        initialized = true;
    }
}

/* ── PNG → HICON loader (GDI+) ──────────────────────────────────── */
/* Load icon from embedded EXE resource (standard Windows ICON) */
static HICON load_icon_from_resource(int size = 32) {
    HINSTANCE hInst = GetModuleHandle(nullptr);
    HICON hIcon = nullptr;

    /* Try loading from resource using MAKEINTRESOURCE(1) — the standard ID */
    hIcon = (HICON)LoadImageW(hInst, MAKEINTRESOURCEW(IDI_ICON1), IMAGE_ICON, size, size, LR_DEFAULTCOLOR);
    if (hIcon) {
        LOG_I("ICON", "load_icon_from_resource: OK size=%d handle=%p (IDI_ICON1)", size, hIcon);
        return hIcon;
    }

    DWORD err = GetLastError();
    LOG_W("ICON", "load_icon_from_resource: LoadImageW(IDI_ICON1) FAILED size=%d err=%lu", size, err);

    /* Try loading from resource using RT_GROUP_ICON */
    HRSRC hRes = FindResourceW(hInst, MAKEINTRESOURCEW(IDI_ICON1), (LPCWSTR)RT_GROUP_ICON);
    if (hRes) {
        HGLOBAL hGlobal = LoadResource(hInst, hRes);
        if (hGlobal) {
            LPVOID pResData = LockResource(hGlobal);
            DWORD resSize = SizeofResource(hInst, hRes);
            /* Create icon from resource data using CreateIconFromResourceEx */
            HICON hFromRes = CreateIconFromResourceEx(
                (PBYTE)pResData, resSize, TRUE, 0x00030000, size, size, LR_DEFAULTCOLOR);
            if (hFromRes) {
                LOG_I("ICON", "load_icon_from_resource: OK via CreateIconFromResourceEx size=%d", size);
                return hFromRes;
            }
            LOG_W("ICON", "load_icon_from_resource: CreateIconFromResourceEx failed err=%lu", GetLastError());
        }
    }

    /* Try numeric ID 1 with RT_GROUP_ICON */
    hRes = FindResourceW(hInst, MAKEINTRESOURCEW(IDI_ICON1), (LPCWSTR)RT_GROUP_ICON);
    if (hRes) {
        HGLOBAL hGlobal = LoadResource(hInst, hRes);
        if (hGlobal) {
            LPVOID pResData = LockResource(hGlobal);
            DWORD resSize = SizeofResource(hInst, hRes);
            HICON hFromRes = CreateIconFromResourceEx(
                (PBYTE)pResData, resSize, TRUE, 0x00030000, size, size, LR_DEFAULTCOLOR);
            if (hFromRes) {
                LOG_I("ICON", "load_icon_from_resource: OK via numeric GROUP_ICON IDI_ICON1 size=%d", size);
                return hFromRes;
            }
        }
    }

    LOG_W("ICON", "load_icon_from_resource: ALL resource methods failed size=%d", size);
    return nullptr;
}

/* Load PNG icon from file (fallback for development) */

static HICON load_png_icon(const wchar_t* path, int targetSize = 32) {
    ensure_gdiplus();
    LOG_D("ICON", "load_png_icon: trying path=%ls size=%d", path, targetSize);
    Gdiplus::Bitmap* bmp = Gdiplus::Bitmap::FromFile(path);
    if (!bmp || bmp->GetLastStatus() != Gdiplus::Ok) {
        int status = bmp ? (int)bmp->GetLastStatus() : -1;
        LOG_W("ICON", "load_png_icon: Bitmap::FromFile failed status=%d", status);
        delete bmp;
        return nullptr;
    }

    /* Create a DC and bitmap to draw into */
    HDC hdcScreen = GetDC(nullptr);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = targetSize;
    bmi.bmiHeader.biHeight = -targetSize; /* top-down */
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    BYTE* pixels = nullptr;
    HBITMAP hBmp = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, (void**)&pixels, nullptr, 0);
    HGDIOBJ hOld = SelectObject(hdcMem, hBmp);

    /* Fill with black + transparent */
    memset(pixels, 0, targetSize * targetSize * 4);

    /* Draw PNG onto the bitmap using GDI+ */
    Gdiplus::Graphics graphics(hdcMem);
    graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    Gdiplus::Rect destRect(0, 0, targetSize, targetSize);
    graphics.DrawImage(bmp, destRect);

    /* Fix alpha: GDI+ draws with premultiplied alpha, convert to straight */
    for (int y = 0; y < targetSize; y++) {
        for (int x = 0; x < targetSize; x++) {
            int idx = (y * targetSize + x) * 4;
            BYTE a = pixels[idx + 3];
            if (a > 0 && a < 255) {
                /* Premultiplied → straight alpha */
                pixels[idx + 0] = (BYTE)std::min(255, pixels[idx + 0] * 255 / a);
                pixels[idx + 1] = (BYTE)std::min(255, pixels[idx + 1] * 255 / a);
                pixels[idx + 2] = (BYTE)std::min(255, pixels[idx + 2] * 255 / a);
            }
        }
    }

    SelectObject(hdcMem, hOld);
    DeleteDC(hdcMem);
    ReleaseDC(nullptr, hdcScreen);
    delete bmp;

    /* Create mask: 0 = opaque, 1 = transparent */
    int maskBytes = (targetSize * targetSize + 7) / 8;
    BYTE* maskBits = new BYTE[maskBytes];
    memset(maskBits, 0xFF, maskBytes); /* all transparent by default */

    /* For opaque pixels, clear the mask bit */
    /* Note: mask bitmap is monochrome, uses AND logic */
    /* We'll use a simple approach: all transparent (mask=0xFF), color bitmap has alpha */

    ICONINFO ii = {};
    ii.fIcon = TRUE;
    ii.hbmMask = CreateBitmap(targetSize, targetSize, 1, 1, maskBits);
    ii.hbmColor = hBmp;
    HICON hIcon = CreateIconIndirect(&ii);
    DeleteObject(ii.hbmMask);
    /* hBmp is owned by the icon now, don't delete it */
    delete[] maskBits;

    if (hIcon) {
        LOG_I("ICON", "load_png_icon: OK handle=%p size=%d", hIcon, targetSize);
    } else {
        LOG_E("ICON", "load_png_icon: CreateIconIndirect failed");
    }
    return hIcon;
}

/* ── Tray icon: pre-combined garlic+LED PNG ──────────────────────── */
static HICON create_tray_icon(TrayState state, int size = 96) {
    /* Map state to color name for filename */
    const wchar_t* colorName;
    switch (state) {
        case TrayState::Green:  colorName = L"green";  break;
        case TrayState::Yellow: colorName = L"yellow"; break;
        case TrayState::Red:    colorName = L"red";    break;
        default:                colorName = L"white";  break;
    }

    /* Try loading pre-combined tray PNG: tray_{color}_{size}.png */
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    wchar_t iconPath[MAX_PATH];
    wcscpy_s(iconPath, exePath);
    wchar_t* p = wcsrchr(iconPath, L'\\');

    HICON hBase = nullptr;

    /* Build filename: tray_{color}_{size}.png */
    wchar_t trayFile[32];
    swprintf_s(trayFile, L"tray_%ls_%d.png", colorName, size);

    const wchar_t* themeDir = get_tray_theme_dir();

    /* Path 1: exe_dir/assets/tray/{theme}/tray_*.png */
    if (p) {
        wcscpy_s(p + 1, MAX_PATH - (p - iconPath + 1), L"assets\\");
        wcscat_s(iconPath, MAX_PATH, themeDir);
        wcscat_s(iconPath, MAX_PATH, trayFile);
        LOG_I("ICON", "create_tray_icon: trying theme=%ls", iconPath);
        hBase = load_png_icon(iconPath, size);
    }
    /* Path 2: exe_dir/assets/tray/tray_*.png (theme fallback) */
    if (!hBase && p) {
        wcscpy_s(p + 1, MAX_PATH - (p - iconPath + 1), L"assets\\tray\\");
        wcscat_s(iconPath, MAX_PATH, trayFile);
        LOG_I("ICON", "create_tray_icon: trying combined=%ls", iconPath);
        hBase = load_png_icon(iconPath, size);
    }
    /* Path 3: exe_dir/../assets/tray/{theme}/tray_*.png */
    if (!hBase && p) {
        wcscpy_s(p + 1, MAX_PATH - (p - iconPath + 1), L"..\\assets\\");
        wcscat_s(iconPath, MAX_PATH, themeDir);
        wcscat_s(iconPath, MAX_PATH, trayFile);
        hBase = load_png_icon(iconPath, size);
    }
    /* Path 4: exe_dir/../assets/tray/tray_*.png */
    if (!hBase && p) {
        wcscpy_s(p + 1, MAX_PATH - (p - iconPath + 1), L"..\\assets\\tray\\");
        wcscat_s(iconPath, MAX_PATH, trayFile);
        hBase = load_png_icon(iconPath, size);
    }
    /* Path 5: exe_dir/../../assets/tray/{theme}/tray_*.png */
    if (!hBase && p) {
        wcscpy_s(p + 1, MAX_PATH - (p - iconPath + 1), L"..\\..\\assets\\");
        wcscat_s(iconPath, MAX_PATH, themeDir);
        wcscat_s(iconPath, MAX_PATH, trayFile);
        hBase = load_png_icon(iconPath, size);
    }
    /* Path 6: exe_dir/../../assets/tray/tray_*.png */
    if (!hBase && p) {
        wcscpy_s(p + 1, MAX_PATH - (p - iconPath + 1), L"..\\..\\assets\\tray\\");
        wcscat_s(iconPath, MAX_PATH, trayFile);
        hBase = load_png_icon(iconPath, size);
    }
    /* Path 7: %APPDATA%/AnyClaw/tray/{theme}/tray_*.png */
    if (!hBase) {
        wchar_t appData[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, appData))) {
            swprintf_s(iconPath, MAX_PATH, L"%ls\\AnyClaw\\%ls%ls", appData, themeDir, trayFile);
            hBase = load_png_icon(iconPath, size);
        }
    }

    /* Found combined PNG — use it directly, no GDI overlay needed */
    if (hBase) {
        LOG_I("ICON", "create_tray_icon: loaded combined tray icon OK state=%d size=%d", (int)state, size);
        return hBase;
    }

    /* ═══ Fallback: old behavior — load base icon + GDI LED overlay ═══ */
    LOG_W("ICON", "create_tray_icon: combined PNG not found, falling back to GDI LED overlay");

    /* Status LED colors from theme */
    COLORREF ledColor;
    switch (state) {
        case TrayState::Green:  ledColor = to_colorref(g_colors->success);  break;
        case TrayState::Yellow: ledColor = to_colorref(g_colors->warning);  break;
        case TrayState::Red:    ledColor = to_colorref(g_colors->danger);   break;
        default:                ledColor = to_colorref(g_colors->text_dim);  break;
    }

    /* Load base icon: try embedded resource first, then file */
    hBase = load_icon_from_resource(size);
    if (!hBase && p) {
        /* Fallback: load app_icon.png from file */
        wcscpy_s(p + 1, MAX_PATH - (p - iconPath + 1), L"app_icon.png");
        LOG_I("ICON", "create_tray_icon: trying fallback app_icon=%ls", iconPath);
        hBase = load_png_icon(iconPath, size);
    }
    if (!hBase && p) {
        wcscpy_s(p + 1, MAX_PATH - (p - iconPath + 1), L"..\\assets\\app_icon.png");
        hBase = load_png_icon(iconPath, size);
    }
    if (!hBase && p) {
        wcscpy_s(p + 1, MAX_PATH - (p - iconPath + 1), L"..\\..\\assets\\app_icon.png");
        hBase = load_png_icon(iconPath, size);
    }
    if (!hBase) {
        LOG_W("ICON", "create_tray_icon: ALL icon sources failed, using gray circle fallback");
        /* Fallback: colored circle */
        BYTE* mask = new BYTE[size * size / 8];
        memset(mask, 0xFF, size * size / 8);
        BYTE* bits = new BYTE[size * size * 4];
        memset(bits, 0, size * size * 4);
        int cx = size/2, cy = size/2, r = size/2 - 1;
        for (int y = 0; y < size; y++)
            for (int x = 0; x < size; x++) {
                int dx = x - cx, dy = y - cy;
                if (dx*dx + dy*dy <= r*r) {
                    int idx = (y * size + x) * 4;
                    bits[idx] = 180; bits[idx+1] = 180; bits[idx+2] = 180; bits[idx+3] = 255;
                    mask[(y*size+x)/8] &= ~(1 << ((y*size+x) % 8));
                }
            }
        ICONINFO ii = {};
        ii.fIcon = TRUE;
        ii.hbmMask = CreateBitmap(size, size, 1, 1, mask);
        ii.hbmColor = CreateBitmap(size, size, 1, 32, bits);
        hBase = CreateIconIndirect(&ii);
        DeleteObject(ii.hbmMask);
        DeleteObject(ii.hbmColor);
        delete[] mask;
        delete[] bits;
    }

    /* Draw LED indicator in bottom-right corner */
    HDC hdcScreen = GetDC(nullptr);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hBmp;
    GetObject(hBase, sizeof(HBITMAP), &hBmp);
    HGDIOBJ hOld = SelectObject(hdcMem, hBmp);

    int ledR = size / 6;
    if (ledR < 2) ledR = 2;
    int ledCx = size - ledR - 1;
    int ledCy = size - ledR - 1;

    /* LED border (dark ring) */
    HPEN hBorderPen = CreatePen(PS_SOLID, 1, RGB(40, 40, 40));
    HGDIOBJ hOldPen = SelectObject(hdcMem, hBorderPen);

    /* LED fill */
    HBRUSH hLed = CreateSolidBrush(ledColor);
    HGDIOBJ hOldBrush = SelectObject(hdcMem, hLed);
    Ellipse(hdcMem, ledCx - ledR, ledCy - ledR, ledCx + ledR, ledCy + ledR);
    SelectObject(hdcMem, hOldBrush);
    SelectObject(hdcMem, hOldPen);
    DeleteObject(hLed);
    DeleteObject(hBorderPen);

    /* LED highlight (small white dot) */
    HBRUSH hHi = CreateSolidBrush(RGB(255, 255, 255));
    hOldBrush = SelectObject(hdcMem, hHi);
    int hiR = ledR / 3;
    if (hiR < 1) hiR = 1;
    Ellipse(hdcMem, ledCx - hiR, ledCy - hiR - 1, ledCx + hiR, ledCy + hiR - 1);
    SelectObject(hdcMem, hOldBrush);
    DeleteObject(hHi);

    SelectObject(hdcMem, hOld);
    DeleteDC(hdcMem);
    ReleaseDC(nullptr, hdcScreen);

    return hBase;
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
                               bool disabled = false, bool checked = false,
                               COLORREF ledColor = 0) {
    MenuItemData data;
    data.text = text ? text : L"";
    data.isSeparator = false;
    data.isDisabled = disabled;
    data.isChecked = checked;
    data.id = id;
    data.ledColor = ledColor;
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
    data.ledColor = 0;
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

    /* Draw LED indicator for status items */
    if (data->ledColor) {
        int lr = 4; /* LED radius */
        int lcx = rc.left + 9;
        int lcy = (rc.top + rc.bottom) / 2;
        HBRUSH hLedBorder = CreateSolidBrush(RGB(40, 40, 40));
        HPEN hOldPen = (HPEN)SelectObject(hdc, GetStockObject(NULL_PEN));
        HGDIOBJ hOldBrush = SelectObject(hdc, hLedBorder);
        Ellipse(hdc, lcx - lr - 1, lcy - lr - 1, lcx + lr + 1, lcy + lr + 1);
        HBRUSH hLedFill = CreateSolidBrush(data->ledColor);
        SelectObject(hdc, hLedFill);
        Ellipse(hdc, lcx - lr, lcy - lr, lcx + lr, lcy + lr);
        SelectObject(hdc, hOldBrush);
        SelectObject(hdc, hOldPen);
        DeleteObject(hLedFill);
        DeleteObject(hLedBorder);
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
    h = CreateWindowW(L"STATIC", L"Version 2.0.1", WS_CHILD | WS_VISIBLE | SS_CENTER,
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

/* ── Create tray menu ────────────────────────────────────────────── */
static HMENU create_tray_menu(TrayState state) {
    g_menuItems.clear();
    HMENU hMenu = CreatePopupMenu();

    /* Remove the Unicode bullet from text — LED will be drawn separately */
    const wchar_t* statusText = L"Status (Unknown)";
    COLORREF ledColor = RGB(150, 150, 150);
    if (g_lang == Lang::CN) {
        statusText = L"OpenClaw \x2014 \x672A\x77E5";
        switch (state) {
            case TrayState::Green:  statusText = L"OpenClaw \x2014 \x8FD0\x884C\x4E2D"; ledColor = RGB(0, 200, 60); break;
            case TrayState::Yellow: statusText = L"OpenClaw \x2014 \x68C0\x6D4B\x4E2D..."; ledColor = RGB(240, 200, 0); break;
            case TrayState::Red:    statusText = L"OpenClaw \x2014 \x5F02\x5E38"; ledColor = RGB(220, 50, 50); break;
            case TrayState::White:   statusText = L"OpenClaw \x2014 \x7A7A\u95F2"; ledColor = RGB(200, 200, 210); break;
        }
    } else {
        switch (state) {
            case TrayState::Green:  statusText = L"Status (Running)"; ledColor = RGB(0, 200, 60); break;
            case TrayState::Yellow: statusText = L"Status (Checking...)"; ledColor = RGB(240, 200, 0); break;
            case TrayState::Red:    statusText = L"Status (Error)"; ledColor = RGB(220, 50, 50); break;
            case TrayState::White:   statusText = L"Status (Idle)"; ledColor = RGB(200, 200, 210); break;
        }
    }
    append_owner_item(hMenu, IDM_TRAY_BASE, statusText, true, false, ledColor);

    /* Model name (sub-status) */
    {
        extern bool app_get_current_model(char* buf, int buf_size);
        char model_buf[128] = {0};
        app_get_current_model(model_buf, sizeof(model_buf));
        if (model_buf[0]) {
            /* Shorten: take last segment */
            const char* short_name = model_buf;
            const char* last_slash = strrchr(model_buf, '/');
            if (last_slash && strlen(last_slash + 1) > 3) short_name = last_slash + 1;
            wchar_t model_w[128] = {0};
            MultiByteToWideChar(CP_UTF8, 0, short_name, -1, model_w, 127);
            wchar_t label_w[160] = {0};
            if (g_lang == Lang::CN) swprintf_s(label_w, L"  Model: %ls", model_w);
            else swprintf_s(label_w, L"  Model: %ls", model_w);
            append_owner_item(hMenu, IDM_TRAY_BASE + 1, label_w, true, false, RGB(120, 120, 130));
        }
    }

    append_owner_sep(hMenu);

    /* Start/Stop toggle */
    bool isRunning = (state == TrayState::Green);
    append_owner_item(hMenu, IDM_START_STOP,
        isRunning
            ? (g_lang == Lang::CN ? L"停止 Gateway" : L"Stop Gateway")
            : (g_lang == Lang::CN ? L"启动 Gateway" : L"Start Gateway"));

    /* Restart OpenClaw */
    append_owner_item(hMenu, IDM_RESTART_OC,
        g_lang == Lang::CN ? L"\x91CD\x542F OpenClaw" : L"Restart OpenClaw");

    /* Refresh status */
    append_owner_item(hMenu, IDM_REFRESH_STATUS,
        g_lang == Lang::CN ? L"\u5237\u65B0\u72B6\u6001" : L"Refresh Status");

    append_owner_sep(hMenu);
    append_owner_item(hMenu, IDM_AUTO_START,
        g_lang == Lang::CN ? L"\u5F00\u673A\u81EA\u542F" : L"Boot Start",
        false, g_autoStartChecked);
    /* Window topmost toggle */
    append_owner_item(hMenu, IDM_ALWAYS_TOP,
        g_lang == Lang::CN ? L"\u7F6E\u9876\u7A97\u53E3" : L"Pin on Top",
        false, app_is_topmost());
    append_owner_sep(hMenu);
    append_owner_item(hMenu, IDM_OPEN_SETTINGS,
        g_lang == Lang::CN ? L"\u6253\u5F00\u8BBE\u7F6E" : L"Settings");
    append_owner_item(hMenu, IDM_ABOUT,
        g_lang == Lang::CN ? L"\u5173\u4E8E" : L"About");
    append_owner_item(hMenu, IDM_EXIT,
        g_lang == Lang::CN ? L"\u9000\u51FA" : L"Exit");

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
        case WM_LBUTTONUP:
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
        case IDM_START_STOP: {
            extern TrayState g_currentState; /* access tray state */
            bool isRunning = (g_currentState == TrayState::Green);
            if (isRunning) {
                app_stop_gateway();
                tray_set_state(TrayState::White);
                tray_balloon("AnyClaw",
                    (g_lang == Lang::CN) ? "Gateway 已停止" : "Gateway stopped", 2000);
            } else {
                app_start_gateway();
                tray_set_state(TrayState::Yellow);
                tray_balloon("AnyClaw",
                    (g_lang == Lang::CN) ? "正在启动 Gateway..." : "Starting Gateway...", 2000);
            }
            break;
        }
        case IDM_REFRESH_STATUS:
            app_refresh_status();
            tray_balloon("AnyClaw",
                (g_lang == Lang::CN) ? "正在刷新状态..." : "Refreshing status...", 2000);
            break;
        case IDM_OPEN_SETTINGS:
            tray_show_window(true);
            ui_settings_open();
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
        case IDM_ALWAYS_TOP: {
            bool newState = !app_is_topmost();
            app_set_topmost(newState);
            /* Persist to config */
            save_theme_config();
            tray_balloon("AnyClaw",
                (g_lang == Lang::CN)
                    ? (newState ? "窗口已置顶" : "窗口已取消置顶")
                    : (newState ? "Window pinned" : "Window unpinned"),
                2000);
            break;
        }
        case IDM_ABOUT: {
            extern void ui_show_about_dialog();
            tray_show_window(true);
            ui_show_about_dialog();
            break;
        }
        case IDM_EXIT: {
            extern bool is_exit_confirmation_enabled();
            if (is_exit_confirmation_enabled()) {
                tray_show_window(true);
                extern void ui_show_exit_dialog();
                ui_show_exit_dialog();
                return 0;
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
        /* Show exit dialog directly — no PostMessage round-trip delay */
        if (cmd == IDM_EXIT) {
            SendMessage(hwnd, WM_COMMAND, cmd, 0);
        } else if (cmd > 0) {
            SendMessage(hwnd, WM_COMMAND, cmd, 0);
        }
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
        LOG_E("TRAY", "Failed to register window class");
        return false;
    }

    g_hwnd = CreateWindowExW(0, WINDOW_CLASS, L"AnyClaw_Tray",
                             0, 0, 0, 0, 0,
                             HWND_MESSAGE, nullptr, hInstance, nullptr);
    if (!g_hwnd) {
        LOG_E("TRAY", "Failed to create message window");
        return false;
    }

    memset(&g_nid, 0, sizeof(g_nid));
    g_nid.cbSize = sizeof(NOTIFYICONDATAW);
    g_nid.hWnd = g_hwnd;
    g_nid.uID = TRAY_ICON_UID;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = create_tray_icon(TrayState::White);
    wcscpy_s(g_nid.szTip, L"AnyClaw LVGL");

    if (!Shell_NotifyIconW(NIM_ADD, &g_nid)) {
        LOG_E("TRAY", "Failed to add tray icon");
        return false;
    }

    LOG_I("TRAY", "Tray icon initialized");
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
    LOG_I("TRAY", "Tray cleaned up");
}

void tray_set_state(TrayState state) {
    if (state == (TrayState)g_pendingStateVal && g_currentState == state) return;
    /* Store pending state atomically — main thread will apply via tray_process_messages */
    InterlockedExchange(&g_pendingStateVal, (LONG)state);
    InterlockedExchange(&g_stateDirty, 1);
}

/* Apply pending tray state on the main thread (called from tray_process_messages) */
static void tray_apply_pending_state() {
    if (!InterlockedCompareExchange(&g_stateDirty, 0, 1)) return;

    TrayState state = (TrayState)g_pendingStateVal;
    if (state == g_currentState) return;
    g_currentState = state;

    const wchar_t* tip;
    switch (state) {
        case TrayState::Green:  tip = (g_lang == Lang::CN) ? L"AnyClaw \x2014 \x8FD0\x884C\x4E2D" : L"AnyClaw \x2014 Running"; break;
        case TrayState::Yellow: tip = (g_lang == Lang::CN) ? L"AnyClaw \x2014 \x68C0\x6D4B\x4E2D" : L"AnyClaw \x2014 Checking"; break;
        case TrayState::Red:    tip = (g_lang == Lang::CN) ? L"AnyClaw \x2014 \x5F02\x5E38" : L"AnyClaw \x2014 Error"; break;
        case TrayState::White:   tip = (g_lang == Lang::CN) ? L"AnyClaw \x2014 \x7A7A\u95F2" : L"AnyClaw \x2014 Idle"; break;
    }

    HICON hNewIcon = create_tray_icon(state);
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
    LOG_I("TRAY", "Balloon: %s - %s", title, message);
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
            /* Restore taskbar icon: re-add WS_EX_APPWINDOW */
            LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
            exStyle |= WS_EX_APPWINDOW;
            SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);
            if (IsIconic(hwnd)) {
                /* Window was minimized to taskbar — restore it */
                ShowWindow(hwnd, SW_RESTORE);
            } else {
                ShowWindow(hwnd, SW_SHOW);
            }
            SetForegroundWindow(hwnd);
            SDL_RaiseWindow(sdlWin);
        } else {
            /* Hide from taskbar: remove WS_EX_APPWINDOW, then hide */
            LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
            exStyle &= ~WS_EX_APPWINDOW;
            SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);
            /* Force frame change so taskbar icon is removed */
            SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
            ShowWindow(hwnd, SW_HIDE);
        }
    }
}

/* Set the window taskbar/titlebar icon from garlic_icon.png */
void tray_set_window_icon() {
    extern SDL_Window* app_get_window();
    SDL_Window* sdlWin = app_get_window();
    if (!sdlWin) { LOG_W("ICON", "tray_set_window_icon: no SDL window"); return; }

    HWND hwnd = nullptr;
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (SDL_GetWindowWMInfo(sdlWin, &wmInfo)) {
        hwnd = wmInfo.info.win.window;
    }
    if (!hwnd) { LOG_W("ICON", "tray_set_window_icon: no HWND"); return; }

    /* Load window icon: try embedded resource first, then file */
    HICON hIcon96 = load_icon_from_resource(96);
    HICON hIcon64 = load_icon_from_resource(64);
    HICON hIcon32 = load_icon_from_resource(32);
    if (!hIcon96 && !hIcon64) {
        /* Fallback: load from file with multiple path attempts */
        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        LOG_I("ICON", "tray_set_window_icon: resource failed, trying file fallback, exePath=%ls", exePath);
        wchar_t iconPath[MAX_PATH];
        wcscpy_s(iconPath, exePath);
        wchar_t* p = wcsrchr(iconPath, L'\\');

        /* Path 1: exe_dir/app_icon.png */
        if (p) {
            wcscpy_s(p + 1, MAX_PATH - (p - iconPath + 1), L"app_icon.png");
            hIcon96 = load_png_icon(iconPath, 96);
            hIcon64 = load_png_icon(iconPath, 64);
            hIcon32 = load_png_icon(iconPath, 32);
        }
        /* Path 2: exe_dir/../assets/app_icon.png */
        if (!hIcon96 && !hIcon64 && p) {
            wcscpy_s(p + 1, MAX_PATH - (p - iconPath + 1), L"..\\assets\\app_icon.png");
            hIcon96 = load_png_icon(iconPath, 96);
            hIcon64 = load_png_icon(iconPath, 64);
            hIcon32 = load_png_icon(iconPath, 32);
        }
        /* Path 3: exe_dir/../../assets/app_icon.png */
        if (!hIcon96 && !hIcon64 && p) {
            wcscpy_s(p + 1, MAX_PATH - (p - iconPath + 1), L"..\\..\\assets\\app_icon.png");
            hIcon96 = load_png_icon(iconPath, 96);
            hIcon64 = load_png_icon(iconPath, 64);
            hIcon32 = load_png_icon(iconPath, 32);
        }
        /* Path 4: %APPDATA%/AnyClaw/app_icon.png */
        if (!hIcon96 && !hIcon64) {
            wchar_t appData[MAX_PATH];
            if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, appData))) {
                swprintf_s(iconPath, MAX_PATH, L"%ls\\AnyClaw\\app_icon.png", appData);
                hIcon96 = load_png_icon(iconPath, 96);
                hIcon64 = load_png_icon(iconPath, 64);
                hIcon32 = load_png_icon(iconPath, 32);
            }
        }
    }
    if (hIcon96) {
        SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon96);
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)(hIcon64 ? hIcon64 : hIcon96));
    } else if (hIcon64) {
        SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon64);
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon64);
    }
    if (hIcon32) {
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon32);
    }
    /* Also set the class icon so child windows/dialogs inherit it */
    if (hIcon96 || hIcon64) {
        SetClassLongPtr(hwnd, GCLP_HICON, (LONG_PTR)(hIcon96 ? hIcon96 : hIcon64));
        SetClassLongPtr(hwnd, GCLP_HICONSM, (LONG_PTR)(hIcon32 ? hIcon32 : (hIcon64 ? hIcon64 : hIcon96)));
    }
}

void tray_process_messages() {
    /* Apply any pending tray state change on the main thread (thread-safe) */
    tray_apply_pending_state();

    /* Drain Windows messages but cap to prevent re-entrant or spinning message loops
       from blocking the main loop. Use a hard limit (256) + time budget (10ms). */
    MSG msg;
    DWORD deadline = GetTickCount() + 10;
    int count = 0;
    while (count < 256 && GetTickCount() < deadline && PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        ++count;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

bool tray_should_quit() {
    return g_shouldQuit;
}

void tray_request_quit() {
    g_shouldQuit = true;
}

HWND tray_get_hwnd() {
    return g_hwnd;
}

void tray_refresh_theme() {
    /* Destroy cached brushes so they get recreated with new theme colors */
    if (s_bgBrush) { DeleteObject(s_bgBrush); s_bgBrush = nullptr; }
    if (s_panelBrush) { DeleteObject(s_panelBrush); s_panelBrush = nullptr; }
    ensure_brushes();
    /* Re-apply current state to update icon with theme colors */
    TrayState cur = g_currentState;
    g_currentState = TrayState::White; /* force refresh */
    tray_set_state(cur);
    LOG_I("TRAY", "Theme refreshed");
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

    /* Use PostMessage for exit command — avoids blocking the main loop */
    if (cmd == IDM_EXIT) {
        PostMessage(g_hwnd, WM_COMMAND, cmd, 0);
    } else if (cmd > 0) {
        SendMessage(g_hwnd, WM_COMMAND, cmd, 0);
    }
}

/* P2-24: Window topmost delegation */
void tray_set_topmost(bool topmost) {
    app_set_topmost(topmost);
}

bool tray_is_topmost() {
    return app_is_topmost();
}
