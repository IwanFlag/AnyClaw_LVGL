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

    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left, Top, Right, Bottom; }

    const uint MOUSEEVENTF_RIGHTDOWN = 0x0008;
    const uint MOUSEEVENTF_RIGHTUP = 0x0010;

    static void Main(string[] args) {
        IntPtr trayWnd = FindWindow("Shell_TrayWnd", null);
        if (trayWnd == IntPtr.Zero) { Console.WriteLine("No tray"); return; }
        
        RECT trayRect;
        GetWindowRect(trayWnd, out trayRect);
        Console.WriteLine("Tray: {0},{1} - {2},{3}", trayRect.Left, trayRect.Top, trayRect.Right, trayRect.Bottom);
        
        // Find the tray icons area (near the clock, bottom-right)
        IntPtr trayNotify = FindWindowEx(trayWnd, IntPtr.Zero, "TrayNotifyWnd", null);
        RECT nr;
        GetWindowRect(trayNotify, out nr);
        Console.WriteLine("TrayNotify: {0},{1} - {2},{3}", nr.Left, nr.Top, nr.Right, nr.Bottom);
        
        // Click on the first icon from the right (our app icon)
        int clickX = nr.Right - 16;
        int clickY = (nr.Top + nr.Bottom) / 2;
        
        SetCursorPos(clickX, clickY);
        Thread.Sleep(200);
        mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, IntPtr.Zero);
        Thread.Sleep(100);
        mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, IntPtr.Zero);
        
        Thread.Sleep(500); // Wait for menu to appear
        
        // Capture screenshot of the menu area
        int captureX = clickX - 160;
        int captureY = clickY - 200;
        int captureW = 360;
        int captureH = 420;
        
        if (captureX < 0) captureX = 0;
        if (captureY < 0) captureY = 0;
        
        Bitmap bmp = new Bitmap(captureW, captureH);
        Graphics g = Graphics.FromImage(bmp);
        g.CopyFromScreen(captureX, captureY, 0, 0, new Size(captureW, captureH));
        
        string path = "C:\\Users\\thundersoft\\.openclaw\\workspace\\AnyClaw_LVGL\\tray_menu.png";
        bmp.Save(path, ImageFormat.Png);
        Console.WriteLine("Saved: " + path);
        g.Dispose();
        bmp.Dispose();
    }
}
