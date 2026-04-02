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
    static extern bool SetCursorPos(int x, int y);
    [DllImport("user32.dll")]
    static extern int GetSystemMetrics(int n);
    [DllImport("user32.dll")]
    static extern IntPtr PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
    [DllImport("user32.dll")]
    static extern bool SetForegroundWindow(IntPtr h);

    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int L, T, R, B; }

    const uint KEYEVENTF_KEYUP = 0x0002;
    const uint WM_SHOW_TRAY_MENU = 0x0400 + 100;

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
        Thread.Sleep(800);

        // Step 2: Find tray window
        IntPtr trayWnd = FindWindow("AnyClaw_TrayWnd", null);
        if (trayWnd == IntPtr.Zero) {
            Console.WriteLine("Tray window not found!");
            return;
        }
        Console.WriteLine("Found tray window: " + trayWnd);

        // Step 3: Set cursor position to a good spot (bottom-right, away from any windows)
        // Menu will appear at cursor position per our code: GetCursorPos(&pt)
        SetCursorPos(900, 500);
        Thread.Sleep(200);

        // Step 4: Post the message to show menu
        Console.WriteLine("Posting WM_SHOW_TRAY_MENU...");
        PostMessage(trayWnd, WM_SHOW_TRAY_MENU, IntPtr.Zero, IntPtr.Zero);

        // Step 5: Poll for menu appearance (up to 3 seconds)
        IntPtr menuWnd = IntPtr.Zero;
        for (int i = 0; i < 30; i++) {
            Thread.Sleep(100);
            menuWnd = FindWindow("#32768", null);
            if (menuWnd != IntPtr.Zero) break;
        }

        if (menuWnd != IntPtr.Zero) {
            RECT mr;
            GetWindowRect(menuWnd, out mr);
            Console.WriteLine("Menu found! " + mr.L + "," + mr.T + "-" + mr.R + "," + mr.B);

            // Wait a bit for it to fully render
            Thread.Sleep(1500);
            SaveScreenshot("tray_menu_final");

            // Close with Escape
            keybd_event(0x1B, 0, 0, IntPtr.Zero);
            Thread.Sleep(50);
            keybd_event(0x1B, 0, KEYEVENTF_KEYUP, IntPtr.Zero);
            Console.WriteLine("Menu closed");
        } else {
            Console.WriteLine("Menu NOT found after 3 seconds");
            SaveScreenshot("tray_menu_final");
        }

        Thread.Sleep(300);
        Console.WriteLine("Done!");
    }
}
