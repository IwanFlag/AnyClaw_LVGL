/* ═══════════════════════════════════════════════════════════════
 *  Garlic Dock — 窗口贴边缩为大蒜头（纯 Win32 实现）
 *
 *  拖拽窗口到屏幕边缘松开 → 隐藏主窗口 → 浮动大蒜头
 *  鼠标悬停大蒜头 → 葱头摇摆动画
 *  点击大蒜头 → 恢复主窗口
 * ═══════════════════════════════════════════════════════════════ */
#include "garlic_dock.h"
#include <SDL.h>
#include "SDL_syswm.h"
#include <windows.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include "app_log.h"

/* stb_image for PNG loading (works in Wine, unlike WIC/GDI+) */
#define STB_IMAGE_IMPLEMENTATION
#include "../../thirdparty/lvgl/src/libs/gltf/stb_image/stb_image.h"

/* ── Window dimensions ── */
#define GARLIC_W          80
#define GARLIC_H          110
#define GARLIC_BODY_SIZE  64
#define SPROUT_SIZE       72
#define EDGE_SNAP_PX      20
#define TIMER_ANIM        1
#define TIMER_HOVER_CHECK 2

/* ── Snap edge ── */
enum class SnapEdge { None, Left, Right, Top, Bottom };

/* ── State ── */
static HWND      g_dock_hwnd    = nullptr;
static HINSTANCE g_hinst        = nullptr;
static bool      g_docked       = false;
static SnapEdge  g_snap_edge    = SnapEdge::None;
static int       g_snap_pos     = 0;
static HWND      g_main_hwnd    = nullptr;
static RECT      g_main_rect    = {};
static bool      g_hovering     = false;
static DWORD     g_anim_start   = 0;
static HBITMAP   g_body_bmp     = nullptr;  /* garlic body */
static HBITMAP   g_sprout_bmp   = nullptr;  /* sprout */
static int       g_body_w = 0, g_body_h = 0;
static int       g_sprout_w = 0, g_sprout_h = 0;

/* ── Load PNG via stb_image → HBITMAP ── */
static HBITMAP load_png_bitmap(const char* path, int* out_w, int* out_h) {
    int width = 0, height = 0, channels = 0;
    unsigned char* pixels = stbi_load(path, &width, &height, &channels, 4); /* force RGBA */
    if (!pixels) {
        LOG_W("GARLIC", "stbi_load failed: %s (%s)", path, stbi_failure_reason());
        return nullptr;
    }

    /* Create DIB section (BGRA top-down) */
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = (LONG)width;
    bmi.bmiHeader.biHeight = -(LONG)height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    BYTE* dib_pixels = nullptr;
    HDC hdc = GetDC(nullptr);
    HBITMAP hBmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, (void**)&dib_pixels, nullptr, 0);
    if (hBmp && dib_pixels) {
        for (int i = 0; i < width * height; i++) {
            dib_pixels[i*4+0] = pixels[i*4+2]; /* B */
            dib_pixels[i*4+1] = pixels[i*4+1]; /* G */
            dib_pixels[i*4+2] = pixels[i*4+0]; /* R */
            dib_pixels[i*4+3] = pixels[i*4+3]; /* A */
        }
    }
    ReleaseDC(nullptr, hdc);
    stbi_image_free(pixels);

    if (out_w) *out_w = width;
    if (out_h) *out_h = height;
    return hBmp;
}

/* ── Load textures ── */
static void load_bitmaps() {
    char exe_path[MAX_PATH];
    GetModuleFileNameA(nullptr, exe_path, MAX_PATH);
    char* p = strrchr(exe_path, '\\');

    const char* body_names[] = { "assets\\garlic_48.png", "..\\assets\\garlic_48.png", "..\\..\\assets\\garlic_48.png" };
    const char* sprout_names[] = { "assets\\garlic_sprout.png", "..\\assets\\garlic_sprout.png", "..\\..\\assets\\garlic_sprout.png" };

    for (auto& rel : body_names) {
        if (p && !g_body_bmp) {
            char dir[MAX_PATH];
            size_t dir_len = (size_t)(p - exe_path + 1);
            if (dir_len >= MAX_PATH) dir_len = MAX_PATH - 1;
            memcpy(dir, exe_path, dir_len);
            dir[dir_len] = '\0';
            char path[MAX_PATH];
            snprintf(path, MAX_PATH, "%s%s", dir, rel);
            g_body_bmp = load_png_bitmap(path, &g_body_w, &g_body_h);
            if (g_body_bmp) {
                LOG_I("GARLIC", "Body bitmap loaded: %dx%d from %s", g_body_w, g_body_h, path);
            } else {
                LOG_W("GARLIC", "Body bitmap failed: %s", path);
            }
        }
    }
    for (auto& rel : sprout_names) {
        if (p && !g_sprout_bmp) {
            char dir[MAX_PATH];
            size_t dir_len = (size_t)(p - exe_path + 1);
            if (dir_len >= MAX_PATH) dir_len = MAX_PATH - 1;
            memcpy(dir, exe_path, dir_len);
            dir[dir_len] = '\0';
            char path[MAX_PATH];
            snprintf(path, MAX_PATH, "%s%s", dir, rel);
            g_sprout_bmp = load_png_bitmap(path, &g_sprout_w, &g_sprout_h);
            if (g_sprout_bmp) {
                LOG_I("GARLIC", "Sprout bitmap loaded: %dx%d from %s", g_sprout_w, g_sprout_h, path);
            } else {
                LOG_W("GARLIC", "Sprout bitmap failed: %s", path);
            }
        }
    }
    if (!g_body_bmp) LOG_E("GARLIC", "Body bitmap NOT FOUND in any path!");
    if (!g_sprout_bmp) LOG_E("GARLIC", "Sprout bitmap NOT FOUND in any path!");
}

/* ── Render ── */
static void paint_dock(HDC hdc) {
    /* Fill with transparent key color (magenta) */
    HBRUSH bg_brush = CreateSolidBrush(RGB(1, 0, 1));
    RECT rc = {0, 0, GARLIC_W, GARLIC_H};
    FillRect(hdc, &rc, bg_brush);
    DeleteObject(bg_brush);

    HDC mem_dc = CreateCompatibleDC(hdc);

    /* Draw garlic body (centered, near bottom) */
    if (g_body_bmp) {
        HGDIOBJ old = SelectObject(mem_dc, g_body_bmp);
        int bx = (GARLIC_W - GARLIC_BODY_SIZE) / 2;
        int by = GARLIC_H - GARLIC_BODY_SIZE - 2;
        StretchBlt(hdc, bx, by, GARLIC_BODY_SIZE, GARLIC_BODY_SIZE,
                   mem_dc, 0, 0, g_body_w, g_body_h, SRCCOPY);
        SelectObject(mem_dc, old);
    } else {
        /* Fallback: draw a purple circle */
        HBRUSH circle_brush = CreateSolidBrush(RGB(130, 80, 180));
        HPEN pen = CreatePen(PS_SOLID, 2, RGB(180, 130, 230));
        HGDIOBJ old_brush = SelectObject(hdc, circle_brush);
        HGDIOBJ old_pen = SelectObject(hdc, pen);
        int cx = GARLIC_W / 2, cy = GARLIC_H - GARLIC_BODY_SIZE / 2 - 4;
        Ellipse(hdc, cx - 24, cy - 24, cx + 24, cy + 24);
        SelectObject(hdc, old_brush);
        SelectObject(hdc, old_pen);
        DeleteObject(circle_brush);
        DeleteObject(pen);
    }

    /* Draw sprout with sway rotation */
    if (g_sprout_bmp) {
        float angle = 0.0f;
        if (g_hovering) {
            DWORD elapsed = GetTickCount() - g_anim_start;
            angle = 12.0f * sinf((float)elapsed * 0.0094f);
        }

        int pivot_x = GARLIC_W / 2;
        int pivot_y = GARLIC_H - GARLIC_BODY_SIZE;
        int sx = (GARLIC_W - SPROUT_SIZE) / 2;
        int sy = pivot_y - SPROUT_SIZE + 8;

        if (fabsf(angle) < 0.5f) {
            HGDIOBJ old = SelectObject(mem_dc, g_sprout_bmp);
            StretchBlt(hdc, sx, sy, SPROUT_SIZE, SPROUT_SIZE,
                       mem_dc, 0, 0, g_sprout_w, g_sprout_h, SRCCOPY);
            SelectObject(mem_dc, old);
        } else {
            float rad = angle * 3.14159265f / 180.0f;
            float cos_a = cosf(rad);
            float sin_a = sinf(rad);

            POINT pts[3];
            float dx = (float)(sx - pivot_x);
            float dy = (float)(sy - pivot_y);
            pts[0].x = (LONG)(pivot_x + dx * cos_a - dy * sin_a);
            pts[0].y = (LONG)(pivot_y + dx * sin_a + dy * cos_a);
            dx = (float)(sx + SPROUT_SIZE - pivot_x);
            pts[1].x = (LONG)(pivot_x + dx * cos_a - dy * sin_a);
            pts[1].y = (LONG)(pivot_y + dx * sin_a + dy * cos_a);
            dx = (float)(sx - pivot_x);
            dy = (float)(sy + SPROUT_SIZE - pivot_y);
            pts[2].x = (LONG)(pivot_x + dx * cos_a - dy * sin_a);
            pts[2].y = (LONG)(pivot_y + dx * sin_a + dy * cos_a);

            HGDIOBJ old = SelectObject(mem_dc, g_sprout_bmp);
            PlgBlt(hdc, pts, mem_dc, 0, 0, g_sprout_w, g_sprout_h, nullptr, 0, 0);
            SelectObject(mem_dc, old);
        }
    } else {
        /* Fallback: draw a green line as sprout */
        HPEN sprout_pen = CreatePen(PS_SOLID, 3, RGB(80, 200, 80));
        HGDIOBJ old_pen = SelectObject(hdc, sprout_pen);
        int cx = GARLIC_W / 2;
        int top_y = 8;
        int bot_y = GARLIC_H - GARLIC_BODY_SIZE + 4;
        MoveToEx(hdc, cx, bot_y, nullptr);
        LineTo(hdc, cx, top_y);
        MoveToEx(hdc, cx, top_y + 10, nullptr);
        LineTo(hdc, cx + 10, top_y + 4);
        SelectObject(hdc, old_pen);
        DeleteObject(sprout_pen);
    }

    DeleteDC(mem_dc);
}

/* ── Window proc ── */
static LRESULT CALLBACK dock_wndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            paint_dock(hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_TIMER:
            if (wParam == TIMER_ANIM) {
                InvalidateRect(hwnd, nullptr, FALSE);
            } else if (wParam == TIMER_HOVER_CHECK) {
                POINT pt;
                GetCursorPos(&pt);
                RECT rc;
                GetWindowRect(hwnd, &rc);
                bool now_hovering = PtInRect(&rc, pt) != 0;
                if (now_hovering != g_hovering) {
                    g_hovering = now_hovering;
                    if (g_hovering) g_anim_start = GetTickCount();
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
            }
            return 0;
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
            garlic_restore_from_dock();
            return 0;
        case WM_RBUTTONDOWN:
            garlic_restore_from_dock();
            return 0;
        case WM_DESTROY:
            KillTimer(hwnd, TIMER_ANIM);
            KillTimer(hwnd, TIMER_HOVER_CHECK);
            return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

/* ── Register window class ── */
static void register_dock_class() {
    WNDCLASSA wc = {};
    wc.lpfnWndProc = dock_wndproc;
    wc.hInstance = g_hinst;
    wc.lpszClassName = "AnyClawGarlicDock";
    wc.hCursor = LoadCursor(nullptr, IDC_HAND);
    RegisterClassA(&wc);
}

/* ── Position dock at snap edge ── */
static void position_dock() {
    if (!g_dock_hwnd) return;
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    int x = 0, y = 0;
    switch (g_snap_edge) {
        case SnapEdge::Left:   x = 0;            y = (g_snap_pos > 0) ? g_snap_pos : (sh - GARLIC_H) / 2; break;
        case SnapEdge::Right:  x = sw - GARLIC_W; y = (g_snap_pos > 0) ? g_snap_pos : (sh - GARLIC_H) / 2; break;
        case SnapEdge::Top:    x = (g_snap_pos > 0) ? g_snap_pos : (sw - GARLIC_W) / 2; y = 0; break;
        case SnapEdge::Bottom: x = (g_snap_pos > 0) ? g_snap_pos : (sw - GARLIC_W) / 2; y = sh - GARLIC_H; break;
        default: break;
    }
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x + GARLIC_W > sw) x = sw - GARLIC_W;
    if (y + GARLIC_H > sh) y = sh - GARLIC_H;
    SetWindowPos(g_dock_hwnd, HWND_TOPMOST, x, y, GARLIC_W, GARLIC_H, SWP_SHOWWINDOW);
    LOG_I("GARLIC", "Dock positioned at (%d,%d) screen=%dx%d", x, y, sw, sh);
}

/* ── Helpers ── */
static void get_primary_monitor_rect(RECT* r) {
    r->left = 0; r->top = 0;
    r->right = GetSystemMetrics(SM_CXSCREEN);
    r->bottom = GetSystemMetrics(SM_CYSCREEN);
}

/* ═══ Public API ═══ */

void garlic_dock_init() {
    g_hinst = GetModuleHandle(nullptr);
    register_dock_class();
    load_bitmaps();
    LOG_I("GARLIC", "Garlic dock module initialized (Win32)");
}

void garlic_dock_cleanup() {
    if (g_dock_hwnd) {
        DestroyWindow(g_dock_hwnd);
        g_dock_hwnd = nullptr;
    }
    if (g_body_bmp)   { DeleteObject(g_body_bmp);   g_body_bmp = nullptr; }
    if (g_sprout_bmp) { DeleteObject(g_sprout_bmp); g_sprout_bmp = nullptr; }
    g_docked = false;
}

void garlic_snap_to_edge() {
    if (g_docked) {
        LOG_D("GARLIC", "snap_to_edge: already docked, skip");
        return;
    }

    extern SDL_Window* app_get_window();
    SDL_Window* main_win = app_get_window();
    if (!main_win) { LOG_W("GARLIC", "snap_to_edge: no main window"); return; }

    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (!SDL_GetWindowWMInfo(main_win, &wmInfo)) {
        LOG_W("GARLIC", "snap_to_edge: SDL_GetWindowWMInfo failed");
        return;
    }
    g_main_hwnd = wmInfo.info.win.window;

    RECT win_rect;
    GetWindowRect(g_main_hwnd, &win_rect);
    g_main_rect = win_rect;

    int wx = win_rect.left;
    int wy = win_rect.top;
    int ww = win_rect.right - win_rect.left;
    int wh = win_rect.bottom - win_rect.top;

    RECT mon;
    get_primary_monitor_rect(&mon);

    int d_left   = wx - mon.left;
    int d_right  = mon.right - win_rect.right;
    int d_top    = wy - mon.top;
    int d_bottom = mon.bottom - win_rect.bottom;

    LOG_I("GARLIC", "snap_to_edge: rect=(%d,%d)-(%d,%d) size=%dx%d dist L=%d R=%d T=%d B=%d threshold=%d",
          wx, wy, win_rect.right, win_rect.bottom, ww, wh,
          d_left, d_right, d_top, d_bottom, EDGE_SNAP_PX);

    int min_d = d_left;
    SnapEdge edge = SnapEdge::Left;
    int pos = wy;
    if (d_right < min_d)  { min_d = d_right;  edge = SnapEdge::Right;  pos = wy; }
    if (d_top < min_d)    { min_d = d_top;    edge = SnapEdge::Top;    pos = wx; }
    if (d_bottom < min_d) { min_d = d_bottom; edge = SnapEdge::Bottom; pos = wx; }

    if (min_d > EDGE_SNAP_PX) {
        LOG_D("GARLIC", "snap_to_edge: too far from edge (min_d=%d > %d), skip", min_d, EDGE_SNAP_PX);
        return;
    }

    g_snap_edge = edge;
    g_snap_pos = pos;
    g_docked = true;
    g_hovering = false;

    ShowWindow(g_main_hwnd, SW_HIDE);

    g_dock_hwnd = CreateWindowExA(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        "AnyClawGarlicDock", "AnyClaw Dock",
        WS_POPUP,
        0, 0, GARLIC_W, GARLIC_H,
        nullptr, nullptr, g_hinst, nullptr);

    if (!g_dock_hwnd) {
        LOG_E("GARLIC", "Failed to create dock window");
        ShowWindow(g_main_hwnd, SW_SHOW);
        g_docked = false;
        return;
    }

    SetLayeredWindowAttributes(g_dock_hwnd, RGB(1, 0, 1), 0, LWA_COLORKEY);
    position_dock();
    SetTimer(g_dock_hwnd, TIMER_ANIM, 33, nullptr);
    SetTimer(g_dock_hwnd, TIMER_HOVER_CHECK, 100, nullptr);

    LOG_I("GARLIC", "Snapped to edge=%d pos=%d (Win32)", (int)edge, pos);
}

void garlic_restore_from_dock() {
    if (!g_docked) return;
    LOG_I("GARLIC", "Restoring from dock (Win32)");

    if (g_dock_hwnd) {
        DestroyWindow(g_dock_hwnd);
        g_dock_hwnd = nullptr;
    }

    if (g_main_hwnd) {
        SetWindowPos(g_main_hwnd, HWND_NOTOPMOST,
                     g_main_rect.left, g_main_rect.top,
                     g_main_rect.right - g_main_rect.left,
                     g_main_rect.bottom - g_main_rect.top,
                     SWP_SHOWWINDOW | SWP_FRAMECHANGED);
        ShowWindow(g_main_hwnd, SW_SHOW);
        SetForegroundWindow(g_main_hwnd);
        BringWindowToTop(g_main_hwnd);
    }

    g_docked = false;
    g_snap_edge = SnapEdge::None;
}

bool garlic_is_docked() {
    return g_docked;
}
