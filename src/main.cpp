#include "lv_conf.h"
/* Increase keyboard buffer for IME (Chinese input sends multi-byte sequences) */
#define KEYBOARD_BUFFER_SIZE 256
#include "lvgl.h"
#include "app.h"
#include "resource.h"
#include "app_config.h"
#include "paths.h"
#include "model_manager.h"
#include "tray.h"
#include "async_task.h"
#include "health.h"
#include "garlic_dock.h"
#include "license.h"
#include "workspace.h"
#include "permissions.h"
#include "feature_flags.h"
#include "app_log.h"
#include "SDL.h"
#include "SDL_syswm.h"
#include <cstring>
#include <windows.h>
#include "drivers/sdl/lv_sdl_window.h"
#include "drivers/sdl/lv_sdl_mouse.h"
#include "drivers/sdl/lv_sdl_keyboard.h"
#include "drivers/sdl/lv_sdl_private.h"
#include "widgets/textarea/lv_textarea.h"
#include "widgets/textarea/lv_textarea_private.h"
extern const lv_obj_class_t lv_textarea_class;
#include <cstdio>

/* Track last clicked textarea for clipboard operations (settings tabs etc.) */
static lv_obj_t* g_last_clicked_ta = nullptr;
#include <string>
#include <fstream>
#include <vector>
#include <algorithm>
#include <atomic>
#include <thread>
#include <ctime>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <dbghelp.h>
#include "async_task.h"
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "comctl32.lib")
/* dbghelp linked via CMake (MinGW doesn't support #pragma comment) */

/* Log system globals */
bool g_log_enabled = true;
int  g_log_level   = LOG_DEBUG;

/* ═══════════════════════════════════════════════════════════════
 *  Crash Handler — E-01-2: 崩溃日志
 *  崩溃时自动生成日志文件到 %APPDATA%\AnyClaw_LVGL\crash_logs\
 *  保留最近 10 个崩溃日志
 * ═══════════════════════════════════════════════════════════════ */
static std::string get_crash_log_dir() {
    const char* appdata = std::getenv("APPDATA");
    std::string dir;
    if (appdata) dir = std::string(appdata) + "\\" + APP_NAME + "\\" + DIR_CRASH_LOGS;
    else dir = "crash_logs";
    CreateDirectoryA(dir.c_str(), NULL);
    return dir;
}

static void cleanup_old_crash_logs(const std::string& dir) {
    WIN32_FIND_DATAA fd;
    std::string pattern = dir + "\\crash_*.log";
    HANDLE hFind = FindFirstFileA(pattern.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    /* Collect all crash log files */
    std::vector<std::string> files;
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            files.push_back(fd.cFileName);
        }
    } while (FindNextFileA(hFind, &fd));
    FindClose(hFind);

    /* Sort alphabetically (timestamps sort naturally) */
    std::sort(files.begin(), files.end());

    /* Delete oldest files, keep last 10 */
    while (files.size() > 10) {
        DeleteFileA((dir + "\\" + files.front()).c_str());
        files.erase(files.begin());
    }
}

static LONG WINAPI crash_handler(EXCEPTION_POINTERS* ex) {
    /* Open log file */
    std::string dir = get_crash_log_dir();
    SYSTEMTIME st;
    GetLocalTime(&st);
    char filename[512];
    snprintf(filename, sizeof(filename),
             "%s\\crash_%04d%02d%02d_%02d%02d%02d.log",
             dir.c_str(), st.wYear, st.wMonth, st.wDay,
             st.wHour, st.wMinute, st.wSecond);

    FILE* f = fopen(filename, "w");
    if (!f) return EXCEPTION_EXECUTE_HANDLER;

    fprintf(f, "=== AnyClaw LVGL Crash Report ===\n");
    fprintf(f, "Time: %04d-%02d-%02d %02d:%02d:%02d\n",
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    fprintf(f, "Version: AnyClaw LVGL v2.0\n\n");

    if (ex && ex->ExceptionRecord) {
        EXCEPTION_RECORD* rec = ex->ExceptionRecord;
        fprintf(f, "Exception Code: 0x%08lX\n", rec->ExceptionCode);
        fprintf(f, "Exception Address: 0x%p\n", (void*)rec->ExceptionAddress);
        fprintf(f, "Exception Flags: 0x%08lX\n\n", rec->ExceptionFlags);

        /* Exception name */
        switch (rec->ExceptionCode) {
            case EXCEPTION_ACCESS_VIOLATION:
                fprintf(f, "Type: ACCESS_VIOLATION\n");
                if (rec->NumberParameters >= 2) {
                    fprintf(f, "Operation: %s at 0x%p\n",
                            rec->ExceptionInformation[0] ? "WRITE" : "READ",
                            (void*)rec->ExceptionInformation[1]);
                }
                break;
            case EXCEPTION_STACK_OVERFLOW:
                fprintf(f, "Type: STACK_OVERFLOW\n"); break;
            case EXCEPTION_ILLEGAL_INSTRUCTION:
                fprintf(f, "Type: ILLEGAL_INSTRUCTION\n"); break;
            case EXCEPTION_INT_DIVIDE_BY_ZERO:
                fprintf(f, "Type: DIVIDE_BY_ZERO\n"); break;
            default:
                fprintf(f, "Type: UNKNOWN (0x%08lX)\n", rec->ExceptionCode); break;
        }

        /* Stack trace using dbghelp */
        fprintf(f, "\n--- Stack Trace ---\n");
        HANDLE process = GetCurrentProcess();
        HANDLE thread = GetCurrentThread();

        SymInitialize(process, NULL, TRUE);

        CONTEXT* ctx = ex->ContextRecord;
        STACKFRAME64 stack = {};
        DWORD machine = IMAGE_FILE_MACHINE_AMD64;

#ifdef _M_X64
        stack.AddrPC.Offset = ctx->Rip;
        stack.AddrFrame.Offset = ctx->Rbp;
        stack.AddrStack.Offset = ctx->Rsp;
#else
        stack.AddrPC.Offset = ctx->Eip;
        stack.AddrFrame.Offset = ctx->Ebp;
        stack.AddrStack.Offset = ctx->Esp;
#endif
        stack.AddrPC.Mode = AddrModeFlat;
        stack.AddrFrame.Mode = AddrModeFlat;
        stack.AddrStack.Mode = AddrModeFlat;

        for (int i = 0; i < 64; i++) {
            if (!StackWalk64(machine, process, thread, &stack,
                             ctx, NULL, SymFunctionTableAccess64,
                             SymGetModuleBase64, NULL)) break;
            if (stack.AddrPC.Offset == 0) break;

            /* Get symbol name */
            char symbol_buf[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
            SYMBOL_INFO* symbol = (SYMBOL_INFO*)symbol_buf;
            symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
            symbol->MaxNameLen = MAX_SYM_NAME;

            DWORD64 displacement = 0;
            if (SymFromAddr(process, stack.AddrPC.Offset, &displacement, symbol)) {
                fprintf(f, "  #%d  0x%p  %s + 0x%llX\n",
                        i, (void*)stack.AddrPC.Offset,
                        symbol->Name, displacement);
            } else {
                fprintf(f, "  #%d  0x%p  <unknown>\n",
                        i, (void*)stack.AddrPC.Offset);
            }
        }

        SymCleanup(process);
    }

    fprintf(f, "\n--- End Crash Report ---\n");
    fclose(f);

    /* Cleanup old logs */
    cleanup_old_crash_logs(dir);

    /* Also write a marker file for recovery */
    std::string marker = dir + "\\last_crash.txt";
    FILE* mf = fopen(marker.c_str(), "w");
    if (mf) {
        fprintf(mf, "%s\n", filename);
        fclose(mf);
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

static void install_crash_handler() {
    SetUnhandledExceptionFilter(crash_handler);
    LOG_I("MAIN", "Crash handler installed");
}


/* Expose for ui_main.cpp window drag */
static SDL_Window* g_window = nullptr;

/* Startup error message - shown as LVGL modal in app_ui_init() */
std::string g_startup_error;
SDL_Window* app_get_window() { return g_window; }

/* ── Window topmost state ───────────────────────────────────────── */
static bool g_topmost = false;

void app_set_topmost(bool topmost) {
    g_topmost = topmost;
    if (!g_window) return;
    /* Get HWND via SDL_SysWMinfo */
    HWND hwnd = nullptr;
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (SDL_GetWindowWMInfo(g_window, &info)) {
        hwnd = info.info.win.window;
    }
    if (!hwnd) return;
    if (topmost) {
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    } else {
        SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
    LOG_I("MAIN", "Window topmost: %s", topmost ? "ON" : "OFF");
}

bool app_is_topmost() {
    return g_topmost;
}

/* Expose runtime CJK font for ui_settings.cpp */
extern lv_font_t* g_cjk_font;
extern lv_font_t* g_cjk_font_small;
lv_font_t* app_get_cjk_font() { 
    static int printed = 0;
    if (!printed) {
        LOG_D("FONT", "app_get_cjk_font() called, g_cjk_font=%p", (void*)g_cjk_font);
        printed = 1;
    }
    return g_cjk_font; 
}
lv_font_t* app_get_cjk_font_small() { 
    static int printed = 0;
    if (!printed) {
        LOG_D("FONT", "app_get_cjk_font_small() called");
        printed = 1;
    }
    return g_cjk_font_small; 
}

/* Get HWND from SDL_Window */
static HWND getNativeWindowHandle(SDL_Window* window) {
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (SDL_GetWindowWMInfo(window, &info)) {
        return info.info.win.window;
    }
    return nullptr;
}


/* P2: Multi-instance protection via Windows Mutex */
static HANDLE g_instanceMutex = nullptr;

static bool acquire_instance_mutex() {
    g_instanceMutex = ::CreateMutexA(nullptr, TRUE, APP_MUTEX_NAME);
    if (::GetLastError() == ERROR_ALREADY_EXISTS) {
        return false;
    }
    return true;
}

static void release_instance_mutex() {
    if (g_instanceMutex) {
        ::ReleaseMutex(g_instanceMutex);
        ::CloseHandle(g_instanceMutex);
        g_instanceMutex = nullptr;
    }
}

/* ═══ Title bar drag via Win32 WM_NCHITTEST ═══ */
#define TITLE_BAR_HEIGHT 48
/* Window control buttons on title bar: right side, exclude from drag */
/* Updated dynamically from actual button positions in ui_main.cpp */
static RECT g_btn_exclude = {0, 0, 0, 0};
void app_update_btn_exclude(int window_w) {
    /* 3 buttons (min/max/close) on right side of title bar, last ~120px */
    g_btn_exclude.right = window_w;
    g_btn_exclude.left = window_w - 120;
    g_btn_exclude.top = 0;
    g_btn_exclude.bottom = TITLE_BAR_HEIGHT;
}

static LRESULT CALLBACK titlebar_subclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                          UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    (void)uIdSubclass; (void)dwRefData;
    if (msg == WM_NCHITTEST) {
        LRESULT hit = DefSubclassProc(hwnd, msg, wParam, lParam);
        if (hit == HTCLIENT) {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(hwnd, &pt);
            if (pt.y >= 0 && pt.y < TITLE_BAR_HEIGHT &&
                !(pt.x >= g_btn_exclude.left && pt.x < g_btn_exclude.right &&
                  pt.y >= g_btn_exclude.top && pt.y < g_btn_exclude.bottom)) {
                return HTCAPTION;
            }
        }
        return hit;
    }
    /* Handle double-click on title bar → maximize/restore */
    if (msg == WM_NCLBUTTONDBLCLK && wParam == HTCAPTION) {
        WINDOWPLACEMENT wp = { sizeof(wp) };
        GetWindowPlacement(hwnd, &wp);
        if (wp.showCmd == SW_MAXIMIZE)
            ShowWindow(hwnd, SW_RESTORE);
        else
            ShowWindow(hwnd, SW_MAXIMIZE);
        return 0;
    }
    if (msg == WM_SETCURSOR) {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(hwnd, &pt);
        if (pt.y >= 0 && pt.y < TITLE_BAR_HEIGHT &&
            !(pt.x >= g_btn_exclude.left && pt.x < g_btn_exclude.right &&
              pt.y >= g_btn_exclude.top && pt.y < g_btn_exclude.bottom)) {
            SetCursor(LoadCursor(nullptr, IDC_ARROW));
            return TRUE;
        }
    }
    /* Handle end of window drag → check for edge snap */
    if (msg == WM_EXITSIZEMOVE) {
        LOG_I("GARLIC", "WM_EXITSIZEMOVE received (hwnd=%p)", hwnd);
        extern bool garlic_is_docked();
        extern void garlic_restore_from_dock();
        if (garlic_is_docked()) {
            /* Already docked — user dragged again, restore instead of skip */
            LOG_I("GARLIC", "WM_EXITSIZEMOVE: already docked, restoring");
            garlic_restore_from_dock();
        } else {
            /* Only snap if not maximized */
            WINDOWPLACEMENT wp = { sizeof(wp) };
            GetWindowPlacement(hwnd, &wp);
            if (wp.showCmd != SW_MAXIMIZE) {
                extern void garlic_snap_to_edge();
                garlic_snap_to_edge();
            } else {
                LOG_D("GARLIC", "WM_EXITSIZEMOVE: maximized, skip snap");
            }
        }
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }
    /* Safety: if main window gets focus while docked, auto-restore */
    if (msg == WM_ACTIVATE && LOWORD(wParam) != WA_INACTIVE) {
        extern bool garlic_is_docked();
        if (garlic_is_docked()) {
            LOG_I("GARLIC", "WM_ACTIVATE: main window focused while docked, auto-restore");
            extern void garlic_restore_from_dock();
            garlic_restore_from_dock();
        }
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

static void setup_title_drag(HWND hwnd) {
    if (hwnd) {
        SetWindowSubclass(hwnd, titlebar_subclass, 1001, 0);
        LOG_I("MAIN", "Title bar drag subclass installed");
    }
}

/* ═══════════════════════════════════════════════════════════════
 *  Command-line interface
 *  --show-wizard   Force the setup wizard to appear (bypasses wizard_completed)
 *  --reset-wizard  Reset wizard_completed=false so wizard auto-shows on next start
 *  --help          Show available options
 * ═══════════════════════════════════════════════════════════════ */
static void handle_cli_args(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        const char* arg = argv[i];
        if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) {
            printf("AnyClaw LVGL v2.0 — Command-line options:\n");
            printf("  --show-wizard   Force the setup wizard to appear\n");
            printf("  --reset-wizard  Reset wizard_completed so wizard shows on next start\n");
            printf("  --help, -h      Show this help message\n");
            exit(0);
        } else if (strcmp(arg, "--reset-wizard") == 0) {
            /* Reset wizard_completed in config so it auto-shows next launch */
            const char* appdata = std::getenv("APPDATA");
            if (!appdata) {
                printf("ERROR: APPDATA not set, cannot reset wizard.\n");
                exit(1);
            }
            std::string cfg_path = std::string(appdata) + "\\AnyClaw_LVGL\\config.json";
            std::ifstream in(cfg_path);
            if (!in.is_open()) {
                printf("AnyClaw config not found at %s (no reset needed)\n", cfg_path.c_str());
                exit(0);
            }
            std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            in.close();
            /* Replace "wizard_completed": true with false */
            std::string modified;
            modified.reserve(content.size());
            bool found = false;
            for (size_t p = 0; p < content.size();) {
                size_t pos = content.find("\"wizard_completed\": true", p);
                if (pos != std::string::npos) {
                    modified += content.substr(p, pos - p);
                    modified += "\"wizard_completed\": false";
                    p = pos + strlen("\"wizard_completed\": true");
                    found = true;
                } else {
                    modified += content.substr(p);
                    break;
                }
            }
            if (found) {
                std::ofstream out(cfg_path);
                if (out.is_open()) {
                    out << modified;
                    out.close();
                    printf("Wizard reset: wizard_completed=false. Restart AnyClaw to see the wizard.\n");
                } else {
                    printf("ERROR: Could not write config at %s\n", cfg_path.c_str());
                    exit(1);
                }
            } else {
                printf("Wizard was already not completed.\n");
            }
            exit(0);
        }
    }
}

/* Exposed by ui_main.cpp: force wizard to show regardless of wizard_completed */
extern void ui_show_setup_wizard_forced(void);

/* Flag: user passed --show-wizard */
static bool g_force_show_wizard = false;

int main(int argc, char* argv[]) {
    handle_cli_args(argc, argv);

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--show-wizard") == 0)
            g_force_show_wizard = true;
    }

    /* E-01-2: Install crash handler as early as possible */
    install_crash_handler();

    /* Initialize application logging */
    app_log_init();
    LOG_I("MAIN", "=== AnyClaw LVGL v2.0 starting ===");
    LOG_I("MAIN", "Log file: %s", app_log_file().c_str());
    LOG_I("MAIN", "Log enabled: %d  level: %d", g_log_enabled, g_log_level);

    /* P2: Prevent multiple instances.
     * Do not show a blocking message box (looks like app freeze to users).
     * Instead, try to bring the existing instance to front and exit quietly. */
    if (!acquire_instance_mutex()) {
        const char* titles[] = {
            "AnyClaw LVGL v2.0 - Desktop Manager",
            "AnyClaw LVGL",
            nullptr
        };
        HWND existing = nullptr;
        for (int i = 0; titles[i]; ++i) {
            existing = FindWindowA(nullptr, titles[i]);
            if (existing) break;
        }
        if (existing) {
            ShowWindow(existing, SW_RESTORE);
            SetForegroundWindow(existing);
            FlashWindow(existing, TRUE);
            return 0;
        }
        LOG_W("MAIN", "Another instance is running but no main window was found; exit silently to avoid blocking popup");
        return 0;
    }

    /* Console hidden in release build (WIN32 subsystem) — use anyclaw_app.log for debug */

    HINSTANCE hInstance = GetModuleHandle(nullptr);

    /* ═══ DPI Awareness: declare BEFORE any SDL/GDI calls ═══
     * Without this, Windows auto-scales the window bitmap AND our manual
     * SCALE() macro causes double-scaling (125% × 125% = 156%).
     * With DPI awareness, SDL/LVGL work in physical pixels and SCALE()
     * correctly converts base layout to DPI-scaled layout. */
    {
        typedef BOOL(WINAPI *SetProcessDpiAwarenessContext_fn)(HANDLE);
        HMODULE user32 = GetModuleHandleA("user32.dll");
        bool dpi_set = false;
        if (user32) {
            auto fn = (SetProcessDpiAwarenessContext_fn)GetProcAddress(user32, "SetProcessDpiAwarenessContext");
            if (fn) {
                /* DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 = (HANDLE)-4 */
                dpi_set = fn((HANDLE)-4) != FALSE;
                if (dpi_set) LOG_I("DPI", "SetProcessDpiAwarenessContext(PER_MONITOR_V2) OK");
            }
        }
        if (!dpi_set) {
            /* Fallback for older Windows */
            typedef BOOL(WINAPI *SetProcessDPIAware_fn)(void);
            if (user32) {
                auto fn2 = (SetProcessDPIAware_fn)GetProcAddress(user32, "SetProcessDPIAware");
                if (fn2) {
                    dpi_set = fn2() != FALSE;
                    if (dpi_set) LOG_I("DPI", "SetProcessDPIAware() OK (fallback)");
                }
            }
        }
        if (!dpi_set) LOG_W("DPI", "Warning: could not set DPI awareness");
    }

    /* Init system tray */
    if (!tray_init(hInstance)) {
        LOG_E("MAIN", "tray_init failed!");
        LOG_W("MAIN", "Warning: tray init failed");
    }

    /* Initialize garlic dock (edge snap) */
    garlic_dock_init();

    /* Detect system DPI scaling — app is DPI-aware, so SDL/LVGL work in
     * physical pixels. SCALE() converts base(96dpi) layout to current DPI. */
    int detected_dpi = 100;
    {
        typedef UINT(WINAPI *GetDpiForSystem_fn)(void);
        HMODULE user32 = GetModuleHandleA("user32.dll");
        if (user32) {
            auto fn = (GetDpiForSystem_fn)GetProcAddress(user32, "GetDpiForSystem");
            if (fn) {
                UINT dpi = fn();
                if (dpi > 0) detected_dpi = (int)(dpi * 100 / 96);
            }
        }
        if (detected_dpi == 100) {
            HDC hdc = GetDC(NULL);
            if (hdc) {
                int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
                if (dpi > 0 && dpi != 96) detected_dpi = dpi * 100 / 96;
                ReleaseDC(NULL, hdc);
            }
        }
        LOG_I("WIN", "System DPI scale: %d%%", detected_dpi);
    }
    app_set_dpi_scale(detected_dpi);

    /* Enable IME (Input Method Editor) for Chinese/Japanese input */
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
    SDL_SetHint(SDL_HINT_IME_SUPPORT_EXTENDED_TEXT, "1");

    /* Init LVGL first (lv_sdl_window_create calls SDL_Init internally) */
    lv_init();

    /* Detect actual screen resolution using Win32 (SDL may return wrong value)
     * Use SM_CXVIRTUALSCREEN for the virtual desktop, falling back to
     * SM_CXFULLSCREEN (work area), then SM_CXSCREEN (full screen).
     *
     * CRITICAL: On Windows with Per-Monitor DPI Awareness V2 + DPI virtualization
     * (e.g. 2560x1600 physical @200% scaling shown as 1280x800 virtual),
     * GetSystemMetrics may return PHYSICAL pixels even for SM_CXVIRTUALSCREEN.
     * We detect this by comparing screen size against the DPI scale:
     *   - At 200% DPI (detected_dpi=208): virtual = physical / 2.08
     *   - If physical / dpi_ratio is meaningfully smaller → use virtual
     *
     * SDL2 with Per-Monitor DPI awareness creates windows in physical pixels,
     * while our LVGL layout uses virtual pixels. We must keep both in sync.
     */
    int raw_screen_w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int raw_screen_h = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    if (raw_screen_w <= 0) {
        raw_screen_w = GetSystemMetrics(SM_CXFULLSCREEN);
        raw_screen_h = GetSystemMetrics(SM_CYFULLSCREEN);
    }
    if (raw_screen_w <= 0) {
        raw_screen_w = GetSystemMetrics(SM_CXSCREEN);
        raw_screen_h = GetSystemMetrics(SM_CYSCREEN);
    }

    /* Use raw screen pixels directly - SDL2 handles DPI awareness */
    int screen_w = raw_screen_w;
    int screen_h = raw_screen_h;
    if (screen_w <= 0) screen_w = 1280;
    if (screen_h <= 0) screen_h = 800;

    /* Default window size: 75% of screen. */
    int win_w = screen_w * 75 / 100;
    int win_h = screen_h * 75 / 100;

    /* Load saved window size from config (before window creation) */
    load_window_config(&win_w, &win_h);

    /* Clamp to screen (with margins for taskbar + window borders) */
    int max_w = screen_w - 60;  /* 30px each side */
    int max_h = screen_h - 80;  /* title bar + taskbar + borders */
    if (win_w > max_w) win_w = max_w;
    if (win_h > max_h) win_h = max_h;

    /* Clamp to reasonable minimum (fixed physical pixels, not DPI-scaled) */
    if (win_w < 800) win_w = 800;
    if (win_h < 500) win_h = 500;

    /* Load exit confirmation config */
    load_exit_confirm_config();

    /* Create SDL window using LVGL's official driver (EGL preferred, software fallback) */
    lv_display_t* disp = lv_sdl_window_create(win_w, win_h);
    if (!disp) {
        LOG_E("MAIN", "lv_sdl_window_create failed; abort startup to avoid invalid display access");
        app_log_shutdown();
        release_instance_mutex();
        return 1;
    }

    /* Bug 1: Enable window resizing so maximize/restore works */
    lv_sdl_window_set_resizeable(disp, true);

    /* Set window title and position */
    lv_sdl_window_set_title(disp, "AnyClaw LVGL v2.0 - Desktop Manager");
    g_window = lv_sdl_window_get_window(disp);
    /* Read back actual window size */
    if (g_window) SDL_GetWindowSize(g_window, &win_w, &win_h);

    /* Set window icon IMMEDIATELY after creation (before SDL overrides it) */
    {
        HWND hwndEarly = getNativeWindowHandle(g_window);
        if (hwndEarly) {
            HINSTANCE hInst = GetModuleHandle(nullptr);
            HICON hIcon96 = (HICON)LoadImageW(hInst, MAKEINTRESOURCEW(IDI_ICON1), IMAGE_ICON, 96, 96, LR_DEFAULTCOLOR);
            HICON hIcon64 = (HICON)LoadImageW(hInst, MAKEINTRESOURCEW(IDI_ICON1), IMAGE_ICON, 64, 64, LR_DEFAULTCOLOR);
            HICON hIcon32 = (HICON)LoadImageW(hInst, MAKEINTRESOURCEW(IDI_ICON1), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
            if (hIcon96) SendMessage(hwndEarly, WM_SETICON, ICON_BIG, (LPARAM)hIcon96);
            else if (hIcon64) SendMessage(hwndEarly, WM_SETICON, ICON_BIG, (LPARAM)hIcon64);
            if (hIcon64) SendMessage(hwndEarly, WM_SETICON, ICON_SMALL, (LPARAM)hIcon64);
            else if (hIcon32) SendMessage(hwndEarly, WM_SETICON, ICON_SMALL, (LPARAM)hIcon32);
            LOG_D("ICON", "Early window icon set: 96=%p 64=%p 32=%p", hIcon96, hIcon64, hIcon32);
        }
    }

    /* Position window: use saved position or default centered-left */
    if (g_window) {
        /* Check for saved position in config */
        int saved_x = -1, saved_y = -1;
        {
            std::string cfg_path;
            const char* appdata = std::getenv("APPDATA");
            if (appdata) cfg_path = std::string(appdata) + "\\" + APP_NAME + "\\" + FILE_CONFIG;
            else cfg_path = "AnyClaw_config.json";
            std::ifstream cf(cfg_path);
            if (cf.is_open()) {
                std::string content((std::istreambuf_iterator<char>(cf)), std::istreambuf_iterator<char>());
                cf.close();
                /* FIX 1: Use robust JSON extraction instead of hand-written lambda */
                saved_x = json_extract_int(content.c_str(), "window_x", -1);
                saved_y = json_extract_int(content.c_str(), "window_y", -1);
            }
        }
        if (saved_x >= 0 && saved_y >= 0 && saved_x < 3000 && saved_y < 2000) {
            /* Clamp position: ensure window stays fully on screen */
            int wx = saved_x;
            int wy = saved_y;
            if (wx + win_w > screen_w) wx = screen_w - win_w - 10;
            if (wy + win_h > screen_h) wy = screen_h - win_h - 10;
            if (wx < 0) wx = 0;
            if (wy < 0) wy = 0;
            SDL_SetWindowPosition(g_window, wx, wy);
            LOG_I("MAIN", "Restored window position: (%d, %d) [saved (%d,%d) clamped]", wx, wy, saved_x, saved_y);
        } else {
            SDL_SetWindowPosition(g_window, (screen_w - win_w) / 2, (screen_h - win_h) / 2);
        }
        SDL_RaiseWindow(g_window);
        
        /* Fix DWM extended frame covering custom title bar */
        HWND hwnd = getNativeWindowHandle(g_window);
        if (hwnd) {
            MARGINS margins = {0, 0, 0, 0};  /* No extended frame */
            DwmExtendFrameIntoClientArea(hwnd, &margins);
        }
        
        int ww, wh;
        SDL_GetWindowSize(g_window, &ww, &wh);
        LOG_I("SDL", "Window: %dx%d (screen: %dx%d)", ww, wh, screen_w, screen_h);
    }

    /* Create input devices */
    lv_indev_t* mouse = lv_sdl_mouse_create();
    lv_indev_t* keyboard = lv_sdl_keyboard_create();
    (void)mouse;
    (void)keyboard;

    /* Create input group for keyboard -> textarea focus */
    lv_group_t* grp = lv_group_create();
    lv_group_set_default(grp);
    lv_indev_set_group(keyboard, grp);

    /* Initialize async task queue (2 worker threads for HTTP/Exec) */
    async_init(2);

    /* Enable SDL text input for text areas */
    SDL_StartTextInput();
    LOG_I("MAIN", "SDL text input enabled");

    /* Initialize model list (defaults, async API fetch later) */
    model_manager_init();

    /* Initialize model failover system */
    failover_init();
    failover_start_health_thread();

    /* Create UI */
    DWORD tick_ui = GetTickCount();
    app_ui_init();
    LOG_I("MAIN", "UI initialized in %lums", GetTickCount() - tick_ui);

    /* --show-wizard: force the setup wizard to appear immediately */
    if (g_force_show_wizard) {
        lv_timer_create([](lv_timer_t* t) {
            LOG_I("MAIN", "--show-wizard: forcing setup wizard display");
            ui_show_setup_wizard_forced();
            lv_timer_del(t);
        }, 300, nullptr);
    }

    /* Set window icon from garlic_icon.png */
    tray_set_window_icon();

    /* Setup Win32 title bar drag after UI is created (buttons exist) */
    setup_title_drag(getNativeWindowHandle(g_window));
    /* Update button exclude region based on actual window width */
    {
        int ww = 1080;
        if (g_window) SDL_GetWindowSize(g_window, &ww, nullptr);
        app_update_btn_exclude(ww);
    }

    LOG_I("MAIN", "GUI initialized. System tray active.");

    static std::atomic<bool> s_need_license_dialog(false);

    /* Defer heavy startup work to worker thread so UI stays responsive. */
    lv_timer_create([](lv_timer_t* t) {
        std::thread([]() {
            ui_progress_begin("Startup", "Initializing UI...", 5);
            /* Gateway auto-start DISABLED for testing */
            ui_progress_update("Startup", "GUI ready (gateway autostart disabled)", 30);

            health_start();
            ui_progress_update("Startup", "Health monitor online", 50);tray_set_state(TrayState::Yellow);
            tray_balloon("AnyClaw", "已启动，正在检测 OpenClaw 状态...", 3000);

            ui_progress_update("Startup", "Loading license", 46);
            license_init();
            ui_progress_update("Startup", "License loaded", 56);

            {
                std::string ws_root = workspace_get_root();
                if (!ws_root.empty()) {
                    ui_progress_update("Startup", "Workspace init", 66);
                    workspace_init(ws_root.c_str());
                    LOG_I("MAIN", "Workspace: %s", ws_root.c_str());

                    if (!workspace_lock_acquire()) {
                        LOG_W("MAIN", "Workspace lock acquire failed, continuing in read-mostly mode");
                    }

                    permissions().set_workspace_root(ws_root.c_str());
                    bool loaded = permissions().load();
                    if (!loaded) {
                        permissions().save();
                    }
                    ui_progress_update("Startup", "Permissions ready", 80);
                    workspace_sync_managed_sections();
                    workspace_sync_runtime_config_from_permissions();
                    feature_flags_init();
                    ui_progress_update("Startup", "Feature flags loaded", 90);
                } else {
                    ui_progress_update("Startup", "Workspace not configured", 72);
                }
            }

            if (!license_is_valid()) {
                LOG_W("MAIN", "License expired, dialog will be shown on UI thread");
                s_need_license_dialog.store(true);
                ui_progress_finish("Startup", false, "Startup ready (license dialog required)");
            } else {
                char remain[64];
                license_get_remaining_str(remain, sizeof(remain));
                LOG_I("MAIN", "License valid: %s", remain);
                ui_progress_finish("Startup", true, "Background startup ready");
            }
        }).detach();
        lv_timer_del(t);
    }, 50, nullptr);

    lv_timer_create([](lv_timer_t* t) {
        (void)t;
        if (!s_need_license_dialog.exchange(false)) return;
        extern void ui_show_license_dialog();
        ui_show_license_dialog();
    }, 300, nullptr);

    /* Register SDL event watch — fires BEFORE LVGL's sdl_event_handler consumes events */
    extern void ui_process_wheel_scroll();
    SDL_AddEventWatch([](void* userdata, SDL_Event* event) -> int {
        (void)userdata;

        /* Wheel scroll — consume to prevent LVGL's built-in crown scroll */
        if (event->type == SDL_MOUSEWHEEL) {
            extern int g_wheel_diff;
            g_wheel_diff += event->wheel.y;
            return 1;
        }

        /* Window resize — recalculate layout proportions */
        if (event->type == SDL_WINDOWEVENT && event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
            extern void ui_relayout_all();
            ui_relayout_all();
            return 0;
        }

        /* Display changed (dragged to different DPI monitor) — re-read DPI and relayout */
        if (event->type == SDL_WINDOWEVENT && event->window.event == SDL_WINDOWEVENT_DISPLAY_CHANGED) {
            int new_dpi = 100;
            {
                HDC hdc = GetDC(NULL);
                if (hdc) {
                    int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
                    if (dpi > 0) new_dpi = dpi * 100 / 96;
                    ReleaseDC(NULL, hdc);
                }
            }
            LOG_I("WIN", "Display changed, new DPI scale: %d%%", new_dpi);
            app_set_dpi_scale(new_dpi);
            extern void ui_relayout_all();
            ui_relayout_all();
            return 0;
        }

        /* Window expose/move — force LVGL refresh to prevent ghost images during drag */
        if (event->type == SDL_WINDOWEVENT &&
            (event->window.event == SDL_WINDOWEVENT_EXPOSED ||
             event->window.event == SDL_WINDOWEVENT_MOVED)) {
            lv_obj_invalidate(lv_screen_active());
            return 0;
        }

        /* Mouse click — double-click title bar + track clicked textarea */
        if (event->type == SDL_MOUSEBUTTONDOWN && event->button.button == SDL_BUTTON_LEFT) {
            int mx = event->button.x, my = event->button.y;

            /* Manual double-click detection (SDL clicks unreliable in event watch) */
            static Uint32 g_last_click_ms = 0;
            static int g_last_click_x = 0, g_last_click_y = 0;
            Uint32 now = SDL_GetTicks();
            bool is_dbl_click = (now - g_last_click_ms < DOUBLE_CLICK_MS) &&
                                (abs(mx - g_last_click_x) < 10) &&
                                (abs(my - g_last_click_y) < 10);
            g_last_click_ms = now;
            g_last_click_x = mx;
            g_last_click_y = my;

            /* Double-click title bar (y < 48) → maximize/restore */
            if (is_dbl_click && my < 48) {
                /* Use Win32 native maximize/restore — more reliable than SDL in Wine */
                HWND hwnd = getNativeWindowHandle(g_window);
                if (hwnd) {
                    WINDOWPLACEMENT wp = { sizeof(wp) };
                    GetWindowPlacement(hwnd, &wp);
                    if (wp.showCmd == SW_MAXIMIZE)
                        ShowWindow(hwnd, SW_RESTORE);
                    else
                        ShowWindow(hwnd, SW_MAXIMIZE);
                } else {
                    /* Fallback to SDL */
                    SDL_Window* win = app_get_window();
                    if (win) {
                        if (SDL_GetWindowFlags(win) & SDL_WINDOW_MAXIMIZED)
                            SDL_RestoreWindow(win);
                        else
                            SDL_MaximizeWindow(win);
                    }
                }
                return 0;
            }

            /* Track last clicked textarea for clipboard fallback */
            extern lv_obj_t* chat_input;
            lv_obj_t* hit = nullptr;
            /* Check chat_input */
            if (chat_input && lv_obj_is_valid(chat_input)) {
                lv_area_t a;
                lv_obj_get_coords(chat_input, &a);
                if (mx >= a.x1 && mx <= a.x2 && my >= a.y1 && my <= a.y2)
                    hit = chat_input;
            }
            /* Check settings textareas if chat_input not hit */
            if (!hit) {
                extern const lv_obj_class_t lv_textarea_class;
                lv_obj_t* scr = lv_scr_act();
                if (scr) {
                    /* BFS search for textarea under click */
                    lv_obj_t* stack[64];
                    int top = 0;
                    stack[top++] = scr;
                    while (top > 0 && !hit) {
                        lv_obj_t* cur = stack[--top];
                        uint32_t cnt = lv_obj_get_child_count(cur);
                        for (uint32_t i = 0; i < cnt && top < 63; i++) {
                            lv_obj_t* ch = lv_obj_get_child(cur, (int32_t)i);
                            if (!ch) continue;
                            /* Skip invisible */
                            if (lv_obj_has_flag(ch, LV_OBJ_FLAG_HIDDEN)) continue;
                            lv_area_t a;
                            lv_obj_get_coords(ch, &a);
                            if (mx >= a.x1 && mx <= a.x2 && my >= a.y1 && my <= a.y2) {
                                if (lv_obj_check_type(ch, &lv_textarea_class)) {
                                    hit = ch;
                                    break;
                                }
                                stack[top++] = ch;
                            }
                        }
                    }
                }
            }
            if (hit) {
                g_last_clicked_ta = hit;
                /* Focus in group */
                lv_group_t* grp = lv_group_get_default();
                if (grp) lv_group_focus_obj(hit);
                /* Explicitly position cursor at click location */
                lv_area_t lbl_area;
                lv_obj_get_coords(((lv_textarea_t*)hit)->label, &lbl_area);
                lv_point_t local = { mx - lbl_area.x1, my - lbl_area.y1 };
                if (local.x >= 0 && local.y >= 0) {
                    uint32_t ch = lv_label_get_letter_on(((lv_textarea_t*)hit)->label, &local, true);
                    lv_textarea_set_cursor_pos(hit, (int32_t)ch);
                }
            }
            return 1; /* keep in queue — let LVGL also process the click */
        }

        /* Ctrl+C/V/X/A — LVGL SDL driver drops these, handle ourselves */
        if (event->type == SDL_KEYDOWN && (event->key.keysym.mod & KMOD_CTRL)) {
            /* Find focused textarea: group focus first, fallback to last clicked */
            lv_group_t* grp = lv_group_get_default();
            lv_obj_t* focused = grp ? lv_group_get_focused(grp) : nullptr;
            extern const lv_obj_class_t lv_textarea_class;
            bool is_ta = focused && lv_obj_check_type(focused, &lv_textarea_class);
            if (!is_ta && g_last_clicked_ta && lv_obj_is_valid(g_last_clicked_ta)) {
                focused = g_last_clicked_ta;
                is_ta = true;
            }

            if (!is_ta) {
                /* No textarea focused — try label Ctrl+C */
                if (event->key.keysym.sym == SDLK_c) {
                    extern void global_ctrl_c_copy();
                    global_ctrl_c_copy();
                }
                return 0;
            }

            lv_textarea_t* ta = (lv_textarea_t*)focused;
            bool has_sel = ta->sel_start != ta->sel_end;

            if (event->key.keysym.sym == SDLK_c) {
                if (has_sel) {
                    const char* full = lv_textarea_get_text(focused);
                    uint32_t s = ta->sel_start < ta->sel_end ? ta->sel_start : ta->sel_end;
                    uint32_t e = ta->sel_start < ta->sel_end ? ta->sel_end : ta->sel_start;
                    if (full && e <= strlen(full)) {
                        char* sel = (char*)malloc(e - s + 1);
                        if (sel) {
                            memcpy(sel, full + s, e - s);
                            sel[e - s] = '\0';
                            extern void clipboard_copy_to_win(const char*);
                            clipboard_copy_to_win(sel);
                            free(sel);
                        }
                    }
                } else {
                    extern void global_ctrl_c_copy();
                    global_ctrl_c_copy();
                }
            }
            else if (event->key.keysym.sym == SDLK_v) {
                char* text = SDL_GetClipboardText();
                if (text && text[0]) {
                    if (has_sel) {
                        uint32_t s = ta->sel_start < ta->sel_end ? ta->sel_start : ta->sel_end;
                        uint32_t e = ta->sel_start < ta->sel_end ? ta->sel_end : ta->sel_start;
                        lv_textarea_set_cursor_pos(focused, (int32_t)e);
                        for (uint32_t i = 0; i < e - s; i++) lv_textarea_delete_char(focused);
                    }
                    lv_textarea_add_text(focused, text);
                }
                if (text) SDL_free(text);
            }
            else if (event->key.keysym.sym == SDLK_x) {
                if (has_sel) {
                    const char* full = lv_textarea_get_text(focused);
                    uint32_t s = ta->sel_start < ta->sel_end ? ta->sel_start : ta->sel_end;
                    uint32_t e = ta->sel_start < ta->sel_end ? ta->sel_end : ta->sel_start;
                    if (full && e <= strlen(full)) {
                        char* sel = (char*)malloc(e - s + 1);
                        if (sel) {
                            memcpy(sel, full + s, e - s);
                            sel[e - s] = '\0';
                            extern void clipboard_copy_to_win(const char*);
                            clipboard_copy_to_win(sel);
                            free(sel);
                        }
                        lv_textarea_set_cursor_pos(focused, (int32_t)e);
                        for (uint32_t i = 0; i < e - s; i++) lv_textarea_delete_char(focused);
                    }
                }
            }
            else if (event->key.keysym.sym == SDLK_a) {
                const char* text = lv_textarea_get_text(focused);
                if (text) {
                    ta->sel_start = 0;
                    ta->sel_end = (uint32_t)strlen(text);
                    lv_obj_invalidate(focused);
                }
            }
            return 0;
        }

        /* Handle space/backspace/delete on selected text in textareas */
        if (event->type == SDL_KEYDOWN && !(event->key.keysym.mod & KMOD_CTRL) &&
            !(event->key.keysym.mod & KMOD_ALT) && !(event->key.keysym.mod & KMOD_GUI)) {
            lv_group_t* grp = lv_group_get_default();
            lv_obj_t* focused = grp ? lv_group_get_focused(grp) : nullptr;
            if (!focused && g_last_clicked_ta && lv_obj_is_valid(g_last_clicked_ta))
                focused = g_last_clicked_ta;
            if (focused && lv_obj_check_type(focused, &lv_textarea_class)) {
                /* Handle Delete key explicitly (LVGL SDL driver may not map it correctly in Wine) */
                if (event->key.keysym.sym == SDLK_DELETE) {
                    lv_textarea_t* ta = (lv_textarea_t*)focused;
                    if (ta->sel_start != ta->sel_end) {
                        /* Delete selection */
                        uint32_t s = ta->sel_start < ta->sel_end ? ta->sel_start : ta->sel_end;
                        uint32_t e = ta->sel_start < ta->sel_end ? ta->sel_end : ta->sel_start;
                        lv_textarea_set_cursor_pos(focused, (int32_t)s);
                        for (uint32_t i = 0; i < e - s; i++) lv_textarea_delete_char(focused);
                    } else {
                        /* Delete char at cursor (forward delete) */
                        lv_textarea_delete_char_forward(focused);
                    }
                    return 1; /* consume — prevent LVGL from also handling */
                }
                /* NOTE: Backspace is handled by LVGL natively (not event watch).
                 * Event watch Backspace handling caused cursor desync with IME:
                 * - Event watch fires before LVGL's sdl_event_handler
                 * - After TEXTINPUT (IME commit), LVGL updates cursor pos internally
                 * - Event watch's delete_char uses stale cursor → corrupts state
                 * - Result: cursor jumps to pos 1, can't move, can't delete */
                /* Handle selection replacement for non-backspace keys */
                if (lv_textarea_get_text_selection(focused)) {
                    lv_textarea_t* ta = (lv_textarea_t*)focused;
                    uint32_t text_len = (uint32_t)strlen(lv_textarea_get_text(focused));
                    /* Check for actual selection (both sel indices within text bounds) */
                    if (ta->sel_start != ta->sel_end &&
                        ta->sel_start < text_len && ta->sel_end <= text_len &&
                        ta->sel_start != LV_LABEL_TEXT_SELECTION_OFF &&
                        ta->sel_end != LV_LABEL_TEXT_SELECTION_OFF) {
                        uint32_t s = ta->sel_start < ta->sel_end ? ta->sel_start : ta->sel_end;
                        uint32_t e = ta->sel_start < ta->sel_end ? ta->sel_end : ta->sel_start;
                        lv_textarea_set_cursor_pos(focused, (int32_t)s);
                        for (uint32_t i = 0; i < e - s; i++) lv_textarea_delete_char(focused);
                        /* For space, insert a space and consume */
                        if (event->key.keysym.sym == SDLK_SPACE) {
                            lv_textarea_add_char(focused, ' ');
                            return 1;
                        }
                        /* For other printable keys, add the char and consume */
                        if (event->key.keysym.sym >= 0x20 && event->key.keysym.sym < 0x7F) {
                            lv_textarea_add_char(focused, event->key.keysym.sym);
                            return 1;
                        }
                    }
                }
            }
        }

        /* SDL_QUIT — handle window close */
        if (event->type == SDL_QUIT) {
            tray_show_window(false);
            return 0;
        }

        return 0; /* keep all other events in queue for LVGL to process */
    }, nullptr);

    /* Main loop — LVGL timer handles SDL events via sdl_event_handler */
    LOG_I("MAIN", "Entering main loop");
    int loop_count = 0;
    auto last_log_time = std::chrono::steady_clock::now();
    while (!tray_should_quit()) {
        auto loop_start = std::chrono::steady_clock::now();
        lv_timer_handler();
        auto after_lvt = std::chrono::steady_clock::now();
        loop_count++;

        if (tray_should_quit()) break;

        auto before_wheel = std::chrono::steady_clock::now();
        ui_process_wheel_scroll();
        auto after_wheel = std::chrono::steady_clock::now();

        auto before_tray = std::chrono::steady_clock::now();
        tray_process_messages();
        auto after_tray = std::chrono::steady_clock::now();

        /* Log first iteration and every 100th to stderr (bypasses log buffer) */
        auto dt_lvt = std::chrono::duration_cast<std::chrono::milliseconds>(after_lvt - loop_start).count();
        auto dt_wheel = std::chrono::duration_cast<std::chrono::milliseconds>(after_wheel - before_wheel).count();
        auto dt_tray = std::chrono::duration_cast<std::chrono::milliseconds>(after_tray - before_tray).count();
        auto dt_loop = std::chrono::duration_cast<std::chrono::milliseconds>(after_tray - loop_start).count();

        /* Stall watchdog: log only when a stage blocks abnormally long. */
        if (dt_lvt > 1500 || dt_wheel > 1500 || dt_tray > 1500 || dt_loop > 2000) {
            LOG_W("LOOP", "UI stall detected: loop=%lldms lvt=%lldms wheel=%lldms tray=%lldms count=%d",
                  (long long)dt_loop, (long long)dt_lvt, (long long)dt_wheel, (long long)dt_tray, loop_count);
        }

        if (loop_count == 1 || loop_count % 100 == 0) {
            fprintf(stderr, "[LOOP] #%d lvt=%lldms wheel=%lldms tray=%lldms\n",
                    loop_count, (long long)dt_lvt,
                    (long long)dt_wheel,
                    (long long)dt_tray);
            fflush(stderr);
        }
        if (loop_count == 10) {
            fprintf(stderr, "[LOOP] #10 REACHED\n");
            fflush(stderr);
        }

        SDL_Delay(5);

        /* Every 2s write a raw heartbeat to stderr (bypasses log buffer).
           Also log first iteration and every 10th so we can see timing. */
        auto now = std::chrono::steady_clock::now();
        long long elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_log_time).count();
        if (elapsed_ms >= 2000) {
            fprintf(stderr, "[HEARTBEAT] loop_count=%d elapsed=%lldms\n", loop_count, elapsed_ms);
            fflush(stderr);
            last_log_time = now;
        }
    }
    LOG_I("MAIN", "Main loop exited after %d iterations", loop_count);

    LOG_I("MAIN", "Shutting down...");

    /* P2-03/P2-04: Save all config before exit */
    extern void save_theme_config();
    save_theme_config();

    /* Check if we should stop Gateway on exit */
    {
        bool close_gw = true; /* default: close */
        const char* userprofile = std::getenv("USERPROFILE");
        if (userprofile) {
            char path[512];
            snprintf(path, sizeof(path), "%s\\AppData\\Roaming\\AnyClaw_LVGL\\config.json", userprofile);
            FILE* cf = fopen(path, "r");
            if (cf) {
                fseek(cf, 0, SEEK_END); long sz = ftell(cf); fseek(cf, 0, SEEK_SET);
                if (sz > 0 && sz < 16384) {
                    char* buf = new char[sz + 1]; fread(buf, 1, sz, cf); buf[sz] = '\0';
                    if (strstr(buf, "\"close_gateway_on_exit\": false")) close_gw = false;
                    delete[] buf;
                }
                fclose(cf);
            }
        }
        if (close_gw) {
            LOG_I("MAIN", "Stopping OpenClaw Gateway on exit...");
            app_stop_gateway();
        } else {
            LOG_I("MAIN", "Keeping OpenClaw Gateway running (setting disabled)");
        }
    }

    /* FIX 3: Clear sensitive data from memory */
    app_secure_zero_sensitive();

    /* Cleanup */
    workspace_lock_release();
    garlic_dock_cleanup();
    failover_stop_health_thread();
    health_stop();
    async_shutdown();
    app_log_shutdown();  /* 强制刷日志缓冲到磁盘，然后退出刷盘线程 */
    tray_cleanup();
    lv_sdl_quit();

    /* P2: Release the instance mutex */
    release_instance_mutex();

    return 0;
}
