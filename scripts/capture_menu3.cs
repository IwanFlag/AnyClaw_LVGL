using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
using System.Threading;

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
    static extern IntPtr WindowFromPoint(POINT p);
    [DllImport("user32.dll")]
    static extern IntPtr GetDesktopWindow();

    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int L, T, R, B; }
    [StructLayout(LayoutKind.Sequential)]
    public struct POINT { public int X, Y; }

    const uint KEYEVENTF_KEYUP = 0x0002;
    const uint WM_SHOW_TRAY_MENU = 0x0400 + 100;

    static void Main() {
        // Show desktop
        byte VK_LWIN = 0x5B;
        keybd_event(VK_LWIN, 0, 0, IntPtr.Zero);
        keybd_event(0x44, 0, 0, IntPtr.Zero);
        Thread.Sleep(50);
        keybd_event(0x44, 0, KEYEVENTF_KEYUP, IntPtr.Zero);
        keybd_event(VK_LWIN, 0, KEYEVENTF_KEYUP, IntPtr.Zero);
        Thread.Sleep(800);

        IntPtr trayWnd = FindWindow("AnyClaw_TrayWnd", null);
        if (trayWnd == IntPtr.Zero) {
            Console.WriteLine("Tray window not found!");
            return;
        }
        Console.WriteLine("Found tray window: " + trayWnd);

        // Set cursor to a position where menu should be fully visible
        // Use center-ish of screen so menu doesn't go off-screen
        SetCursorPos(640, 400);
        Thread.Sleep(200);

        // Post message
        Console.WriteLine("Posting WM_SHOW_TRAY_MENU...");
        PostMessage(trayWnd, WM_SHOW_TRAY_MENU, IntPtr.Zero, IntPtr.Zero);

        // Poll rapidly for menu with immediate screenshot when found
        for (int attempt = 0; attempt < 50; attempt++) {
            Thread.Sleep(50);
            IntPtr menuWnd = FindWindow("#32768", null);
            if (menuWnd != IntPtr.Zero) {
                RECT mr;
                GetWindowRect(menuWnd, out mr);
                Console.WriteLine("Menu found at attempt {0}: {1},{2}-{3},{4}",
                    attempt, mr.L, mr.T, mr.R, mr.B);

                // Take screenshot IMMEDIATELY
                int sw = GetSystemMetrics(0);
                int sh = GetSystemMetrics(1);

                // First, let's also check what window is at the menu position
                POINT mp = new POINT { X = (mr.L + mr.R) / 2, Y = (mr.T + mr.B) / 2 };
                IntPtr winAtMenu = WindowFromPoint(mp);
                Console.WriteLine("Window at menu center: " + winAtMenu);
                Console.WriteLine("Desktop window: " + GetDesktopWindow());

                // Capture screenshot right now!
                Bitmap bmp = new Bitmap(sw, sh);
                Graphics g = Graphics.FromImage(bmp);
                g.CopyFromScreen(0, 0, 0, 0, new Size(sw, sh));
                string path = "C:\\Users\\thundersoft\\.openclaw\\workspace\\AnyClaw_LVGL\\tray_menu_final.png";
                bmp.Save(path, ImageFormat.Png);
                Console.WriteLine("Saved: " + path + " (" + sw + "x" + sh + ")");
                g.Dispose();
                bmp.Dispose();

                // Also capture just the menu area
                int mw = mr.R - mr.L;
                int mh = mr.B - mr.T;
                if (mw > 0 && mh > 0 && mr.L >= 0 && mr.T >= 0 && mr.R <= sw && mr.B <= sh) {
                    Bitmap mbmp = new Bitmap(mw, mh);
                    Graphics mg = Graphics.FromImage(mbmp);
                    mg.CopyFromScreen(mr.L, mr.T, 0, 0, new Size(mw, mh));
                    string mpath = "C:\\Users\\thundersoft\\.openclaw\\workspace\\AnyClaw_LVGL\\tray_menu_crop.png";
                    mbmp.Save(mpath, ImageFormat.Png);
                    Console.WriteLine("Saved crop: " + mpath);
                    mg.Dispose();
                    mbmp.Dispose();
                }

                // Close menu
                keybd_event(0x1B, 0, 0, IntPtr.Zero);
                Thread.Sleep(30);
                keybd_event(0x1B, 0, KEYEVENTF_KEYUP, IntPtr.Zero);
                return;
            }
        }

        Console.WriteLine("Menu not found after 50 attempts");
        // Save screenshot anyway
        int sw2 = GetSystemMetrics(0);
        int sh2 = GetSystemMetrics(1);
        Bitmap bmp2 = new Bitmap(sw2, sh2);
        Graphics g2 = Graphics.FromImage(bmp2);
        g2.CopyFromScreen(0, 0, 0, 0, new Size(sw2, sh2));
        string path2 = "C:\\Users\\thundersoft\\.openclaw\\workspace\\AnyClaw_LVGL\\tray_menu_final.png";
        bmp2.Save(path2, ImageFormat.Png);
        Console.WriteLine("Saved fallback: " + path2);
        g2.Dispose();
        bmp2.Dispose();
    }
}
