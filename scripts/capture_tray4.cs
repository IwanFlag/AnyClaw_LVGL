using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
using System.Threading;

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

    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int L, T, R, B; }

    const uint MOUSEEVENTF_RIGHTDOWN = 0x0008;
    const uint MOUSEEVENTF_RIGHTUP = 0x0010;
    const uint KEYEVENTF_KEYUP = 0x0002;
    const byte VK_LWIN = 0x5B;

    static void Main() {
        int screenW = GetSystemMetrics(0);
        int screenH = GetSystemMetrics(1);
        Console.WriteLine("Screen: {0}x{1}", screenW, screenH);

        // Step 1: Show desktop with Win+D
        keybd_event(VK_LWIN, 0, 0, IntPtr.Zero);
        keybd_event(0x44, 0, 0, IntPtr.Zero);
        Thread.Sleep(50);
        keybd_event(0x44, 0, KEYEVENTF_KEYUP, IntPtr.Zero);
        keybd_event(VK_LWIN, 0, KEYEVENTF_KEYUP, IntPtr.Zero);
        Thread.Sleep(800);

        // Step 2: Find taskbar and right-click in the notification area
        IntPtr trayWnd = FindWindow("Shell_TrayWnd", null);
        if (trayWnd == IntPtr.Zero) { Console.WriteLine("No tray"); return; }

        IntPtr trayNotify = FindWindowEx(trayWnd, IntPtr.Zero, "TrayNotifyWnd", null);
        RECT nr;
        if (trayNotify != IntPtr.Zero) {
            GetWindowRect(trayNotify, out nr);
            Console.WriteLine("TrayNotify: {0},{1}-{2},{3}", nr.L, nr.T, nr.R, nr.B);
        } else {
            Console.WriteLine("No TrayNotifyWnd, using tray fallback");
            GetWindowRect(trayWnd, out nr);
            nr.L = nr.R - 200;
            Console.WriteLine("Tray fallback: {0},{1}-{2},{3}", nr.L, nr.T, nr.R, nr.B);
        }

        // Click on the rightmost icon area (our app should be there)
        int clickX = nr.R - 16;
        int clickY = (nr.T + nr.B) / 2;
        Console.WriteLine("Clicking tray at: {0},{1}", clickX, clickY);

        SetCursorPos(clickX, clickY);
        Thread.Sleep(300);
        mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, IntPtr.Zero);
        Thread.Sleep(80);
        mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, IntPtr.Zero);

        // Step 3: Wait for menu to appear
        Thread.Sleep(1200);

        // Step 4: Capture full screen
        Bitmap bmp = new Bitmap(screenW, screenH);
        Graphics g = Graphics.FromImage(bmp);
        g.CopyFromScreen(0, 0, 0, 0, new Size(screenW, screenH));

        string path = "C:\\Users\\thundersoft\\.openclaw\\workspace\\AnyClaw_LVGL\\tray_full.png";
        bmp.Save(path, ImageFormat.Png);
        Console.WriteLine("Saved: " + path);
        g.Dispose();
        bmp.Dispose();
    }
}
