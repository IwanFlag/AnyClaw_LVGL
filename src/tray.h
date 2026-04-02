#ifndef TRAY_H
#define TRAY_H

#include <windows.h>

/* Tray icon color states */
enum class TrayState {
    Gray,       /* 未配置 / Not configured */
    Yellow,     /* 检测中 / Checking */
    Green,      /* 运行中 / Running */
    Red         /* 异常 / Error */
};

/* Menu item IDs */
#define IDM_TRAY_BASE       1000
#define IDM_OPEN_SETTINGS   1001
#define IDM_RESTART_OC      1002
#define IDM_VIEW_LOGS       1003
#define IDM_AUTO_START      1004
#define IDM_ABOUT           1005
#define IDM_EXIT            1006

/* Custom tray message */
#define WM_TRAYICON         (WM_USER + 1)

/* Initialize tray icon and hidden message window.
   Returns true on success. Must be called from main thread. */
bool tray_init(HINSTANCE hInstance);

/* Cleanup tray icon and destroy hidden window */
void tray_cleanup();

/* Update tray icon color */
void tray_set_state(TrayState state);

/* Show balloon notification */
void tray_balloon(const char* title, const char* message, int timeout_ms = 3000);

/* Show/hide the main SDL window */
void tray_show_window(bool show);

/* Process Windows messages (call from main loop, non-blocking) */
void tray_process_messages();

/* Check if the app should quit (set by Exit menu) */
bool tray_should_quit();

/* Get the hidden window handle */
HWND tray_get_hwnd();

/* [Debug] Programmatically show the tray context menu at a given screen position */
void tray_show_menu_at(int x, int y);

#endif /* TRAY_H */
