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
    [DllImport("user32.dll")]
    static extern IntPtr SendMessage(IntPtr h, uint m, IntPtr w, IntPtr l);

    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int L, T, R, B; }

    const uint MOUSEEVENTF_RIGHTDOWN = 0x0008;
    const uint MOUSEEVENTF_RIGHTUP = 0x0010;
    const uint KEYEVENTF_KEYUP = 0x0002;
    const uint TB_BUTTONCOUNT = 0x0418;
    const uint TB_GETRECT = 0x041D;

    [StructLayout(LayoutKind.Sequential)]
    public struct RECT2 { public int Left, Top, Right, Bottom; }

    static void Main() {
        int screenW = GetSystemMetrics(0);
        int screenH = GetSystemMetrics(1);

        // Show desktop
        byte VK_LWIN = 0x5B;
        keybd_event(VK_LWIN, 0, 0, IntPtr.Zero);
        keybd_event(0x44, 0, 0, IntPtr.Zero);
        Thread.Sleep(50);
        keybd_event(0x44, 0, KEYEVENTF_KEYUP, IntPtr.Zero);
        keybd_event(VK_LWIN, 0, KEYEVENTF_KEYUP, IntPtr.Zero);
        Thread.Sleep(800);

        // Find the toolbar with tray icons
        IntPtr trayWnd = FindWindow("Shell_TrayWnd", null);
        IntPtr trayNotify = FindWindowEx(trayWnd, IntPtr.Zero, "TrayNotifyWnd", null);
        IntPtr sysPager = FindWindowEx(trayNotify, IntPtr.Zero, "SysPager", null);
        IntPtr toolbar = FindWindowEx(sysPager, IntPtr.Zero, "ToolbarWindow32", null);

        if (toolbar == IntPtr.Zero) {
            Console.WriteLine("Toolbar not found, trying alternative...");
            // Try direct toolbar search
            IntPtr overflow = FindWindowEx(trayNotify, IntPtr.Zero, "Button", null);
            if (overflow != IntPtr.Zero) {
                toolbar = overflow;
            } else {
                Console.WriteLine("No toolbar found at all");
            }
        }

        if (toolbar != IntPtr.Zero) {
            RECT tr;
            GetWindowRect(toolbar, out tr);
            int btnCount = (int)SendMessage(toolbar, TB_BUTTONCOUNT, IntPtr.Zero, IntPtr.Zero);
            Console.WriteLine("Toolbar: {0},{1}-{2},{3}, Buttons: {4}",
                tr.L, tr.T, tr.R, tr.B, btnCount);

            // Try clicking each button from right to left until we see our menu
            for (int i = 0; i < Math.Min(btnCount, 10); i++) {
                int btnX = tr.R - (i * 24) - 12;
                int btnY = (tr.T + tr.B) / 2;
                Console.WriteLine("Trying button {0} at {1},{2}", i, btnX, btnY);

                SetCursorPos(btnX, btnY);
                Thread.Sleep(200);
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, IntPtr.Zero);
                Thread.Sleep(80);
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, IntPtr.Zero);
                Thread.Sleep(800);

                // Check if a menu appeared by looking for #32768 (menu class)
                IntPtr menuWnd = FindWindow("#32768", null);
                if (menuWnd != IntPtr.Zero) {
                    Console.WriteLine("Menu found! At button {0}", i);
                    Thread.Sleep(500);

                    // Capture full screen
                    Bitmap bmp = new Bitmap(screenW, screenH);
                    Graphics g = Graphics.FromImage(bmp);
                    g.CopyFromScreen(0, 0, 0, 0, new Size(screenW, screenH));

                    string path = "C:\\Users\\thundersoft\\.openclaw\\workspace\\AnyClaw_LVGL\\tray_full.png";
                    bmp.Save(path, ImageFormat.Png);
                    Console.WriteLine("Saved: " + path);
                    g.Dispose();
                    bmp.Dispose();
                    return;
                }

                // Close any popup that might have appeared
                keybd_event(0x1B, 0, 0, IntPtr.Zero); // Escape
                keybd_event(0x1B, 0, KEYEVENTF_KEYUP, IntPtr.Zero);
                Thread.Sleep(200);
            }
        }

        // Fallback: just capture the screen as is
        Console.WriteLine("No menu found, capturing as-is");
        Bitmap bmp2 = new Bitmap(screenW, screenH);
        Graphics g2 = Graphics.FromImage(bmp2);
        g2.CopyFromScreen(0, 0, 0, 0, new Size(screenW, screenH));
        string path2 = "C:\\Users\\thundersoft\\.openclaw\\workspace\\AnyClaw_LVGL\\tray_full.png";
        bmp2.Save(path2, ImageFormat.Png);
        Console.WriteLine("Saved fallback: " + path2);
        g2.Dispose();
        bmp2.Dispose();
    }
}
