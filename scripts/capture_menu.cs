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
    static extern IntPtr SendMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
    [DllImport("user32.dll")]
    static extern IntPtr PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);

    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int L, T, R, B; }

    const uint KEYEVENTF_KEYUP = 0x0002;
    const uint WM_SHOW_TRAY_MENU = 0x0400 + 100; // WM_USER + 100

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
        // Step 1: Show desktop
        byte VK_LWIN = 0x5B;
        keybd_event(VK_LWIN, 0, 0, IntPtr.Zero);
        keybd_event(0x44, 0, 0, IntPtr.Zero);
        Thread.Sleep(50);
        keybd_event(0x44, 0, KEYEVENTF_KEYUP, IntPtr.Zero);
        keybd_event(VK_LWIN, 0, KEYEVENTF_KEYUP, IntPtr.Zero);
        Thread.Sleep(500);

        // Step 2: Find the tray hidden window
        IntPtr trayWnd = FindWindow("AnyClaw_TrayWnd", null);
        if (trayWnd == IntPtr.Zero) {
            Console.WriteLine("Tray window not found, trying with PID...");
            Process[] procs = Process.GetProcessesByName("AnyClaw_LVGL");
            if (procs.Length == 0) {
                Console.WriteLine("App not running!");
                return;
            }
            Console.WriteLine("App running PID: " + procs[0].Id);
            return;
        }
        Console.WriteLine("Found tray window: " + trayWnd);

        // Step 3: Position cursor near the taskbar notification area
        // On 1280x800, the tray icons should be at bottom-right
        int cursorX = 1240;
        int cursorY = 776;
        SetCursorPos(cursorX, cursorY);
        Thread.Sleep(300);

        // Step 4: Send WM_SHOW_TRAY_MENU
        Console.WriteLine("Sending WM_SHOW_TRAY_MENU via PostMessage...");
        PostMessage(trayWnd, WM_SHOW_TRAY_MENU, IntPtr.Zero, IntPtr.Zero);

        // Step 5: Wait for menu to appear
        Thread.Sleep(1500);
        
        // Check for popup menu
        IntPtr menuWnd = FindWindow("#32768", null);
        if (menuWnd != IntPtr.Zero) {
            RECT mr;
            GetWindowRect(menuWnd, out mr);
            Console.WriteLine("Menu found! " + mr.L + "," + mr.T + "-" + mr.R + "," + mr.B);
        } else {
            Console.WriteLine("Menu not detected by class, trying screenshot anyway...");
        }

        // Step 6: Capture screenshot
        Thread.Sleep(500);
        SaveScreenshot("tray_menu_final");

        // Step 7: Close the menu with Escape
        keybd_event(0x1B, 0, 0, IntPtr.Zero);
        Thread.Sleep(50);
        keybd_event(0x1B, 0, KEYEVENTF_KEYUP, IntPtr.Zero);
        Thread.Sleep(200);

        Console.WriteLine("Done!");
    }
}
