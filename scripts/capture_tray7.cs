using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
using System.Threading;
using System.Diagnostics;

class Program {
    [DllImport("user32.dll")]
    static extern IntPtr FindWindow(string cls, string win);
    [DllImport("user32.dll")]
    static extern IntPtr FindWindowEx(IntPtr p, IntPtr a, string cls, string win);
    [DllImport("user32.dll")]
    static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")]
    static extern void keybd_event(byte vk, byte scan, uint flags, IntPtr ex);
    [DllImport("user32.dll")]
    static extern void mouse_event(uint f, int dx, int dy, uint d, IntPtr e);
    [DllImport("user32.dll")]
    static extern bool SetCursorPos(int x, int y);
    [DllImport("user32.dll")]
    static extern int GetSystemMetrics(int n);
    [DllImport("user32.dll")]
    static extern IntPtr WindowFromPoint(POINT p);
    [DllImport("user32.dll")]
    static extern IntPtr SendMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
    [DllImport("user32.dll")]
    static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);

    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int L, T, R, B; }
    [StructLayout(LayoutKind.Sequential)]
    public struct POINT { public int X, Y; }

    const uint MOUSEEVENTF_RIGHTDOWN = 0x0008;
    const uint MOUSEEVENTF_RIGHTUP = 0x0010;
    const uint KEYEVENTF_KEYUP = 0x0002;

    static void SaveScreenshot(string name) {
        int sw = GetSystemMetrics(0);
        int sh = GetSystemMetrics(1);
        Bitmap bmp = new Bitmap(sw, sh);
        Graphics g = Graphics.FromImage(bmp);
        g.CopyFromScreen(0, 0, 0, 0, new Size(sw, sh));
        string path = "C:\\Users\\thundersoft\\.openclaw\\workspace\\AnyClaw_LVGL\\" + name + ".png";
        bmp.Save(path, ImageFormat.Png);
        Console.WriteLine("Saved: " + path);
        g.Dispose();
        bmp.Dispose();
    }

    static void Main() {
        // First, let's just show the tray menu by programmatically sending
        // the message to the tray window
        IntPtr trayWnd = FindWindow("Shell_TrayWnd", null);
        if (trayWnd == IntPtr.Zero) { Console.WriteLine("No tray"); return; }

        // Find our app's window - it's an SDL window
        IntPtr sdlWnd = FindWindow("SDL_app", null);
        if (sdlWnd == IntPtr.Zero) {
            // Try finding by the window title
            sdlWnd = FindWindow(null, "AnyClaw LVGL");
        }
        Console.WriteLine("SDL window: " + sdlWnd);

        // Actually, let's try a different approach - use the taskbar's
        // notification area directly. On Windows 10/11 the toolbar is in:
        // Shell_TrayWnd > TrayNotifyWnd > SysPager > ToolbarWindow32
        // OR it might be directly in TrayNotifyWnd

        IntPtr notify = FindWindowEx(trayWnd, IntPtr.Zero, "TrayNotifyWnd", null);
        RECT nr;
        GetWindowRect(notify, out nr);
        Console.WriteLine("TrayNotifyWnd: {0},{1}-{2},{3}", nr.L, nr.T, nr.R, nr.B);

        // Let's try left-clicking to open the overflow panel first
        int overflowX = nr.L + 10;
        int overflowY = (nr.T + nr.B) / 2;
        Console.WriteLine("Clicking overflow arrow at: " + overflowX + "," + overflowY);
        SetCursorPos(overflowX, overflowY);
        Thread.Sleep(200);
        mouse_event(0x0002, 0, 0, 0, IntPtr.Zero); // left down
        Thread.Sleep(50);
        mouse_event(0x0004, 0, 0, 0, IntPtr.Zero); // left up
        Thread.Sleep(1000);

        // Screenshot to see what happened
        SaveScreenshot("tray_overflow");
        
        // Now find any popup that appeared
        // Try various popup window classes
        string[] popupClasses = {"#32768", "ComboLBox", "ToolbarWindow32", "SysListView32"};
        foreach (string cls in popupClasses) {
            IntPtr popup = FindWindow(cls, null);
            if (popup != IntPtr.Zero) {
                RECT pr;
                GetWindowRect(popup, out pr);
                Console.WriteLine("Found " + cls + ": " + pr.L + "," + pr.T + "-" + pr.R + "," + pr.B);
            }
        }

        // Close the overflow
        keybd_event(0x1B, 0, 0, IntPtr.Zero);
        keybd_event(0x1B, 0, KEYEVENTF_KEYUP, IntPtr.Zero);
        Thread.Sleep(300);

        // Now try right-clicking at the SAME position where our app icon would be
        // Since our app is the only one we started, it should be one of the icons
        // Let's try clicking 16 pixels from the RIGHT edge (rightmost icon)
        // and also 40, 64, 88, 112 pixels from right
        for (int offset = 16; offset <= 112; offset += 24) {
            int cx = nr.R - offset;
            int cy = (nr.T + nr.B) / 2;
            Console.WriteLine("Right-click at offset -" + offset + ": " + cx + "," + cy);
            SetCursorPos(cx, cy);
            Thread.Sleep(200);
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, IntPtr.Zero);
            Thread.Sleep(80);
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, IntPtr.Zero);
            Thread.Sleep(800);

            // Check for ANY popup menu
            IntPtr menu = FindWindow("#32768", null);
            if (menu != IntPtr.Zero) {
                RECT mr;
                GetWindowRect(menu, out mr);
                Console.WriteLine("FOUND MENU at offset " + offset + ": " + mr.L + "," + mr.T + "-" + mr.R + "," + mr.B);
                Thread.Sleep(800);
                SaveScreenshot("tray_menu_found");
                return;
            }

            // Also check for any new window
            IntPtr popup = FindWindow("#32768", null);
            if (popup != IntPtr.Zero) {
                RECT pr;
                GetWindowRect(popup, out pr);
                Console.WriteLine("Popup found: " + pr.L + "," + pr.T);
            }

            keybd_event(0x1B, 0, 0, IntPtr.Zero);
            keybd_event(0x1B, 0, KEYEVENTF_KEYUP, IntPtr.Zero);
            Thread.Sleep(200);
        }

        // Check if maybe our app is not running or icon is not visible
        Console.WriteLine("Checking if AnyClaw process is running...");
        Process[] procs = Process.GetProcessesByName("AnyClaw_LVGL");
        if (procs.Length == 0) {
            Console.WriteLine("WARNING: AnyClaw_LVGL is not running!");
        } else {
            Console.WriteLine("AnyClaw_LVGL is running, PID: " + procs[0].Id);
        }

        SaveScreenshot("tray_final");
    }
}
