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
#include <cstdlib>
#include <algorithm>
#include "app_log.h"
#include "theme.h"

/* stb_image for PNG loading (works in Wine, unlike WIC/GDI+) */
#define STB_IMAGE_IMPLEMENTATION
#include "../../thirdparty/lvgl/src/libs/gltf/stb_image/stb_image.h"

/* ── Window dimensions ── */
#define GARLIC_W          80
#define GARLIC_H          110
#define GARLIC_BODY_SIZE  64
#define SPROUT_SIZE       72
#define EDGE_SNAP_PX      28
#define TIMER_ANIM        1
#define TIMER_HOVER_CHECK 2

/* ── Snap edge ── */
enum class SnapEdge { None, Left, Right, Top, Bottom };
enum class RandomFx { None, Fire, Jump, Nod, Flash, Bubble, Sleep };

struct DockParticle {
    bool active;
    float x;
    float y;
    float vx;
    float vy;
    float life;
    float max_life;
    COLORREF color;
};

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
static DWORD     g_last_anim_tick = 0;
static HBITMAP   g_body_bmp     = nullptr;  /* garlic body */
static HBITMAP   g_sprout_bmp   = nullptr;  /* sprout */
static int       g_body_w = 0, g_body_h = 0;
static int       g_sprout_w = 0, g_sprout_h = 0;
static RandomFx  g_random_fx = RandomFx::None;
static DWORD     g_random_fx_start = 0;
static DWORD     g_random_fx_duration = 0;
static DWORD     g_next_random_fx_tick = 0;
static DockParticle g_particles[12] = {};

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

/* ── Theme-aware mascot directory ── */
static const char* get_mascot_dir() {
    switch (g_theme) {
        case Theme::Peachy:  return "mascot\\peachy\\";
        case Theme::Mochi:   return "mascot\\mochi\\";
        default:             return "mascot\\matcha\\";
    }
}

/* ── Load textures ── */
static void load_bitmaps() {
    char exe_path[MAX_PATH];
    GetModuleFileNameA(nullptr, exe_path, MAX_PATH);
    char* p = strrchr(exe_path, '\\');

    const char* mdir = get_mascot_dir();
    /* Try theme-specific path first, then fallback to root assets */
    const char* body_names[] = {
        "assets\\%sgarlic_48.png",       /* theme-specific */
        "assets\\garlic_48.png",         /* fallback */
        "..\\assets\\garlic_48.png",
        "..\\..\\assets\\garlic_48.png"
    };
    const char* sprout_names[] = {
        "assets\\%sgarlic_sprout.png",
        "assets\\garlic_sprout.png",
        "..\\assets\\garlic_sprout.png",
        "..\\..\\assets\\garlic_sprout.png"
    };

    for (int i = 0; i < 4 && !g_body_bmp; i++) {
        if (p) {
            char dir[MAX_PATH];
            size_t dir_len = (size_t)(p - exe_path + 1);
            if (dir_len >= MAX_PATH) dir_len = MAX_PATH - 1;
            memcpy(dir, exe_path, dir_len);
            dir[dir_len] = '\0';
            char path[MAX_PATH];
            if (i == 0) snprintf(path, MAX_PATH, "%sassets\\%sgarlic_48.png", dir, mdir);
            else snprintf(path, MAX_PATH, "%s%s", dir, body_names[i]);
            g_body_bmp = load_png_bitmap(path, &g_body_w, &g_body_h);
            if (g_body_bmp) {
                LOG_I("GARLIC", "Body bitmap loaded: %dx%d from %s", g_body_w, g_body_h, path);
            }
        }
    }
    for (int i = 0; i < 4 && !g_sprout_bmp; i++) {
        if (p) {
            char dir[MAX_PATH];
            size_t dir_len = (size_t)(p - exe_path + 1);
            if (dir_len >= MAX_PATH) dir_len = MAX_PATH - 1;
            memcpy(dir, exe_path, dir_len);
            dir[dir_len] = '\0';
            char path[MAX_PATH];
            if (i == 0) snprintf(path, MAX_PATH, "%sassets\\%sgarlic_sprout.png", dir, mdir);
            else snprintf(path, MAX_PATH, "%s%s", dir, sprout_names[i]);
            g_sprout_bmp = load_png_bitmap(path, &g_sprout_w, &g_sprout_h);
            if (g_sprout_bmp) {
                LOG_I("GARLIC", "Sprout bitmap loaded: %dx%d from %s", g_sprout_w, g_sprout_h, path);
            }
        }
    }
    if (!g_body_bmp) LOG_E("GARLIC", "Body bitmap NOT FOUND in any path!");
    if (!g_sprout_bmp) LOG_E("GARLIC", "Sprout bitmap NOT FOUND in any path!");
}

static void clear_particles() {
    for (auto& p : g_particles) p.active = false;
}

static void emit_particles(bool fire_variant) {
    const int count = 3 + (rand() % 3);
    for (int i = 0; i < count; i++) {
        for (auto& p : g_particles) {
            if (p.active) continue;
            p.active = true;
            p.x = (float)(GARLIC_W / 2 + (rand() % 7) - 3);
            p.y = (float)(GARLIC_H - GARLIC_BODY_SIZE - 8);
            const float spread = ((float)(rand() % 31) - 15.0f) / 100.0f;
            p.vx = fire_variant ? spread * 65.0f : spread * 22.0f;
            p.vy = fire_variant ? (-85.0f - (float)(rand() % 35)) : (-38.0f - (float)(rand() % 18));
            p.life = 0.0f;
            p.max_life = fire_variant ? (0.55f + (float)(rand() % 35) / 100.0f)
                                      : (0.70f + (float)(rand() % 35) / 100.0f);
            p.color = fire_variant ? RGB(61, 214, 140) : RGB(180, 235, 205);
            break;
        }
    }
}

static void schedule_next_random_fx() {
    g_next_random_fx_tick = GetTickCount() + 30000 + (DWORD)(rand() % 90001);
}

static void start_random_fx() {
    int r = rand() % 100;
    if (r < 20) g_random_fx = RandomFx::Fire;
    else if (r < 40) g_random_fx = RandomFx::Jump;
    else if (r < 55) g_random_fx = RandomFx::Nod;
    else if (r < 70) g_random_fx = RandomFx::Flash;
    else if (r < 85) g_random_fx = RandomFx::Bubble;
    else g_random_fx = RandomFx::Sleep;

    g_random_fx_start = GetTickCount();
    switch (g_random_fx) {
        case RandomFx::Fire:   g_random_fx_duration = 1400; emit_particles(true); break;
        case RandomFx::Jump:   g_random_fx_duration = 1100; break;
        case RandomFx::Nod:    g_random_fx_duration = 1400; break;
        case RandomFx::Flash:  g_random_fx_duration = 700; break;
        case RandomFx::Bubble: g_random_fx_duration = 1500; emit_particles(false); break;
        case RandomFx::Sleep:  g_random_fx_duration = 1700; break;
        default:               g_random_fx_duration = 0; break;
    }
}

static void tick_random_fx(float dt_sec) {
    if (dt_sec <= 0.0f) return;
    for (auto& p : g_particles) {
        if (!p.active) continue;
        p.life += dt_sec;
        if (p.life >= p.max_life) {
            p.active = false;
            continue;
        }
        p.x += p.vx * dt_sec;
        p.y += p.vy * dt_sec;
        p.vy += 26.0f * dt_sec;
    }
}

static void draw_particles(HDC hdc) {
    for (const auto& p : g_particles) {
        if (!p.active) continue;
        float t = p.life / p.max_life;
        int size = (int)(3.0f + (1.0f - t) * 3.0f);
        int x = (int)p.x;
        int y = (int)p.y;
        HBRUSH b = CreateSolidBrush(p.color);
        HGDIOBJ old = SelectObject(hdc, b);
        Ellipse(hdc, x - size, y - size, x + size, y + size);
        SelectObject(hdc, old);
        DeleteObject(b);
    }
}

/* ── Render ── */
static void paint_dock(HDC hdc) {
    /* Fill with transparent key color (magenta) */
    HBRUSH bg_brush = CreateSolidBrush(RGB(1, 0, 1));
    RECT rc = {0, 0, GARLIC_W, GARLIC_H};
    FillRect(hdc, &rc, bg_brush);
    DeleteObject(bg_brush);

    HDC mem_dc = CreateCompatibleDC(hdc);
    DWORD now = GetTickCount();
    float elapsed = (float)(now - g_anim_start) / 1000.0f;
    float fx_elapsed = (g_random_fx != RandomFx::None) ? (float)(now - g_random_fx_start) / 1000.0f : 0.0f;
    int body_offset_y = 0;
    float body_shift_x = 0.0f;
    bool draw_flash = false;

    /* Draw garlic body (centered, near bottom) */
    if (g_body_bmp) {
        HGDIOBJ old = SelectObject(mem_dc, g_body_bmp);
        if (g_random_fx == RandomFx::Jump) {
            body_offset_y = -(int)(fabsf(sinf(fx_elapsed * 7.0f)) * 14.0f);
        }
        if (g_random_fx == RandomFx::Nod) {
            body_shift_x = sinf(fx_elapsed * 9.5f) * 6.0f;
        }
        if (g_random_fx == RandomFx::Flash) {
            draw_flash = true;
        }
        int bx = (GARLIC_W - GARLIC_BODY_SIZE) / 2 + (int)body_shift_x;
        int by = GARLIC_H - GARLIC_BODY_SIZE - 2 + body_offset_y;

        if (draw_flash) {
            HBRUSH glow = CreateSolidBrush(RGB(95, 220, 155));
            HGDIOBJ old_glow = SelectObject(hdc, glow);
            Ellipse(hdc, bx - 8, by - 8, bx + GARLIC_BODY_SIZE + 8, by + GARLIC_BODY_SIZE + 8);
            SelectObject(hdc, old_glow);
            DeleteObject(glow);
        }

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
        float angle = g_hovering ? (12.0f * sinf(elapsed * 9.4f)) : (2.0f * sinf(elapsed * 3.1f));
        if (g_random_fx == RandomFx::Nod) {
            angle += 8.0f * sinf(fx_elapsed * 10.0f);
        } else if (g_random_fx == RandomFx::Sleep) {
            float t = std::min(1.0f, fx_elapsed / 1.4f);
            angle -= 16.0f * t;
        }

        int pivot_x = GARLIC_W / 2;
        int pivot_y = GARLIC_H - GARLIC_BODY_SIZE + body_offset_y;
        int sx = (GARLIC_W - SPROUT_SIZE) / 2;
        int sy = pivot_y - SPROUT_SIZE + 8 + body_offset_y / 2;

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

    draw_particles(hdc);
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
                DWORD now = GetTickCount();
                float dt = (g_last_anim_tick > 0 && now > g_last_anim_tick)
                    ? (float)(now - g_last_anim_tick) / 1000.0f
                    : (1.0f / 30.0f);
                g_last_anim_tick = now;

                if (!g_hovering) {
                    if (g_random_fx == RandomFx::None) {
                        if (g_next_random_fx_tick == 0) schedule_next_random_fx();
                        if (now >= g_next_random_fx_tick) start_random_fx();
                    } else if (now - g_random_fx_start >= g_random_fx_duration) {
                        g_random_fx = RandomFx::None;
                        clear_particles();
                        schedule_next_random_fx();
                    }
                }
                tick_random_fx(dt);
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
                    if (g_hovering) {
                        g_random_fx = RandomFx::None;
                        clear_particles();
                        schedule_next_random_fx();
                    }
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

static void play_restore_animation() {
    if (!g_dock_hwnd) return;

    RECT rc;
    GetWindowRect(g_dock_hwnd, &rc);
    int sx = rc.left;
    int sy = rc.top;
    int sw = rc.right - rc.left;
    int sh = rc.bottom - rc.top;
    int tx = (g_main_rect.left + g_main_rect.right) / 2 - sw / 4;
    int ty = (g_main_rect.top + g_main_rect.bottom) / 2 - sh / 4;

    const int steps = 10;
    for (int i = 1; i <= steps; i++) {
        float t = (float)i / (float)steps;
        int nx = sx + (int)((tx - sx) * t);
        int ny = sy + (int)((ty - sy) * t);
        int nw = std::max(22, (int)(sw * (1.0f - 0.55f * t)));
        int nh = std::max(26, (int)(sh * (1.0f - 0.55f * t)));
        BYTE alpha = (BYTE)(255.0f * (1.0f - t));
        SetLayeredWindowAttributes(g_dock_hwnd, RGB(1, 0, 1), alpha, LWA_COLORKEY | LWA_ALPHA);
        SetWindowPos(g_dock_hwnd, HWND_TOPMOST, nx, ny, nw, nh, SWP_NOACTIVATE | SWP_SHOWWINDOW);
        UpdateWindow(g_dock_hwnd);
        Sleep(16);
    }
}

/* ═══ Public API ═══ */

void garlic_dock_init() {
    g_hinst = GetModuleHandle(nullptr);
    srand((unsigned int)GetTickCount());
    register_dock_class();
    load_bitmaps();
    schedule_next_random_fx();
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
    g_random_fx = RandomFx::None;
    g_last_anim_tick = 0;
    clear_particles();
    schedule_next_random_fx();

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

    SetLayeredWindowAttributes(g_dock_hwnd, RGB(1, 0, 1), 255, LWA_COLORKEY | LWA_ALPHA);
    position_dock();
    SetTimer(g_dock_hwnd, TIMER_ANIM, 33, nullptr);
    SetTimer(g_dock_hwnd, TIMER_HOVER_CHECK, 100, nullptr);

    LOG_I("GARLIC", "Snapped to edge=%d pos=%d (Win32)", (int)edge, pos);
}

void garlic_restore_from_dock() {
    if (!g_docked) return;
    LOG_I("GARLIC", "Restoring from dock (Win32)");

    play_restore_animation();

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
    g_random_fx = RandomFx::None;
    clear_particles();
}

bool garlic_is_docked() {
    return g_docked;
}
