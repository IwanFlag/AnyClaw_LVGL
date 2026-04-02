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

        // Find tray notification area window
        IntPtr trayWnd = FindWindow("Shell_TrayWnd", null);
        IntPtr trayNotify = FindWindowEx(trayWnd, IntPtr.Zero, "TrayNotifyWnd", null);
        if (trayNotify == IntPtr.Zero) {
            Console.WriteLine("No TrayNotifyWnd");
            return;
        }

        RECT nr;
        GetWindowRect(trayNotify, out nr);
        Console.WriteLine("TrayNotify: {0},{1} - {2},{3}", nr.L, nr.T, nr.R, nr.B);

        // First try to click the overflow arrow (^) which shows hidden icons
        // It's usually at the left edge of the tray notify area
        int overflowX = nr.L + 8;
        int overflowY = (nr.T + nr.B) / 2;
        Console.WriteLine("Clicking overflow at: {0},{1}", overflowX, overflowY);
        SetCursorPos(overflowX, overflowY);
        Thread.Sleep(200);
        mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, IntPtr.Zero);
        Thread.Sleep(80);
        mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, IntPtr.Zero);
        Thread.Sleep(1000);

        IntPtr menuWnd = FindWindow("#32768", null);
        if (menuWnd != IntPtr.Zero) {
            Console.WriteLine("Menu found from overflow!");
        } else {
            // Close any popup
            keybd_event(0x1B, 0, 0, IntPtr.Zero);
            keybd_event(0x1B, 0, KEYEVENTF_KEYUP, IntPtr.Zero);
            Thread.Sleep(300);
        }

        // Now try left-click to show overflow popup, then find our icon
        SetCursorPos(overflowX, overflowY);
        Thread.Sleep(200);
        mouse_event(0x0002, 0, 0, 0, IntPtr.Zero); // left down
        Thread.Sleep(80);
        mouse_event(0x0004, 0, 0, 0, IntPtr.Zero); // left up
        Thread.Sleep(1000);

        // Check for popup window (overflow icons panel)
        IntPtr overflowWnd = FindWindow("#32768", null);
        if (overflowWnd != IntPtr.Zero) {
            RECT or2;
            GetWindowRect(overflowWnd, out or2);
            Console.WriteLine("Overflow panel: {0},{1}-{2},{3}", or2.L, or2.T, or2.R, or2.B);

            // Scan the overflow panel for buttons, try right-clicking each
            // Since it's a popup, we need to find toolbar within it
            // Instead, let's just right-click various positions in the overflow panel
            for (int x = or2.R - 12; x > or2.L; x -= 24) {
                for (int y = or2.T + 12; y < or2.B; y += 24) {
                    SetCursorPos(x, y);
                    Thread.Sleep(100);
                    mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, IntPtr.Zero);
                    Thread.Sleep(80);
                    mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, IntPtr.Zero);
                    Thread.Sleep(600);

                    // Check for menu
                    IntPtr innerMenu = FindWindow("#32768", null);
                    if (innerMenu != IntPtr.Zero) {
                        Console.WriteLine("Menu found at {0},{1}!", x, y);
                        Thread.Sleep(500);
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
                    keybd_event(0x1B, 0, 0, IntPtr.Zero);
                    keybd_event(0x1B, 0, KEYEVENTF_KEYUP, IntPtr.Zero);
                    Thread.Sleep(100);
                }
            }
        } else {
            Console.WriteLine("No overflow panel found, trying direct tray clicks");
            // Try various positions in the tray notification area
            for (int i = 0; i < 8; i++) {
                int clickX = nr.R - 16 - (i * 24);
                int clickY = (nr.T + nr.B) / 2;
                Console.WriteLine("Trying position {0}: {1},{2}", i, clickX, clickY);
                SetCursorPos(clickX, clickY);
                Thread.Sleep(200);
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, IntPtr.Zero);
                Thread.Sleep(80);
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, IntPtr.Zero);
                Thread.Sleep(800);

                menuWnd = FindWindow("#32768", null);
                if (menuWnd != IntPtr.Zero) {
                    Console.WriteLine("Menu found!");
                    Thread.Sleep(500);
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
                keybd_event(0x1B, 0, 0, IntPtr.Zero);
                keybd_event(0x1B, 0, KEYEVENTF_KEYUP, IntPtr.Zero);
                Thread.Sleep(100);
            }
        }

        // Fallback: just capture screen
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
