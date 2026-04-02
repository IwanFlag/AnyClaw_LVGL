using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
using System.Threading;

class Program {
    [DllImport("user32.dll")]
    static extern IntPtr FindWindow(string lpClassName, string lpWindowName);
    [DllImport("user32.dll")]
    static extern IntPtr FindWindowEx(IntPtr parent, IntPtr after, string cls, string window);
    [DllImport("user32.dll")]
    static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
    [DllImport("user32.dll")]
    static extern void mouse_event(uint dwFlags, int dx, int dy, uint dwData, IntPtr dwExtraInfo);
    [DllImport("user32.dll")]
    static extern bool SetCursorPos(int x, int y);
    [DllImport("user32.dll")]
    static extern int GetSystemMetrics(int nIndex);

    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left, Top, Right, Bottom; }

    const uint MOUSEEVENTF_RIGHTDOWN = 0x0008;
    const uint MOUSEEVENTF_RIGHTUP = 0x0010;

    static void Main(string[] args) {
        IntPtr trayWnd = FindWindow("Shell_TrayWnd", null);
        if (trayWnd == IntPtr.Zero) { Console.WriteLine("No tray"); return; }
        
        // Get screen bounds using GetSystemMetrics
        int screenW = GetSystemMetrics(0); // SM_CXSCREEN
        int screenH = GetSystemMetrics(1); // SM_CYSCREEN
        Console.WriteLine("Screen: {0}x{1}", screenW, screenH);
        
        // Find the tray notification area
        IntPtr trayNotify = FindWindowEx(trayWnd, IntPtr.Zero, "TrayNotifyWnd", null);
        RECT nr;
        GetWindowRect(trayNotify, out nr);
        Console.WriteLine("TrayNotify: {0},{1} - {2},{3}", nr.Left, nr.Top, nr.Right, nr.Bottom);
        
        // Click on the last icon (rightmost - our app)
        int clickX = nr.Right - 16;
        int clickY = (nr.Top + nr.Bottom) / 2;
        Console.WriteLine("Clicking at: {0},{1}", clickX, clickY);
        
        SetCursorPos(clickX, clickY);
        Thread.Sleep(300);
        mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, IntPtr.Zero);
        Thread.Sleep(100);
        mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, IntPtr.Zero);
        
        Thread.Sleep(800); // Wait for menu to fully appear
        
        // Capture FULL SCREEN
        Bitmap bmp = new Bitmap(screenW, screenH);
        Graphics g = Graphics.FromImage(bmp);
        g.CopyFromScreen(0, 0, 0, 0, new Size(screenW, screenH));
        
        string path = "C:\\Users\\thundersoft\\.openclaw\\workspace\\AnyClaw_LVGL\\tray_menu_full.png";
        bmp.Save(path, ImageFormat.Png);
        Console.WriteLine("Saved full screenshot: " + path);
        g.Dispose();
        bmp.Dispose();
    }
}
