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
    [DllImport("user32.dll")]
    static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);

    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left, Top, Right, Bottom; }

    const uint MOUSEEVENTF_RIGHTDOWN = 0x0008;
    const uint MOUSEEVENTF_RIGHTUP = 0x0010;

    static void Main(string[] args) {
        // Minimize all windows first (ShowDesktop)
        IntPtr desktop = FindWindow("Progman", null);
        // Send Win+D by using keybd_event
        // VK_LWIN=0x5B, VK_D=0x44
        // Actually let's just minimize all top-level windows
        
        int screenW = GetSystemMetrics(0);
        int screenH = GetSystemMetrics(1);
        Console.WriteLine("Screen: {0}x{1}", screenW, screenH);

        // Find tray
        IntPtr trayWnd = FindWindow("Shell_TrayWnd", null);
        if (trayWnd == IntPtr.Zero) { Console.WriteLine("No tray"); return; }
        
        // The taskbar IS the tray. Find the button area for our app.
        // Our app is visible as a small icon. Let's use the overflow approach.
        // First click the overflow arrow (^) to show hidden icons
        IntPtr trayNotify = FindWindowEx(trayWnd, IntPtr.Zero, "TrayNotifyWnd", null);
        IntPtr sysPager = FindWindowEx(trayNotify, IntPtr.Zero, "SysPager", null);
        IntPtr toolbar = FindWindowEx(sysPager, IntPtr.Zero, "ToolbarWindow32", null);
        
        if (toolbar != IntPtr.Zero) {
            RECT tr;
            GetWindowRect(toolbar, out tr);
            Console.WriteLine("Toolbar: {0},{1} - {2},{3}", tr.Left, tr.Top, tr.Right, tr.Bottom);
            
            // Click on the last button (our icon) in the toolbar
            int lastBtnX = tr.Right - 6;
            int btnY = (tr.Top + tr.Bottom) / 2;
            
            SetCursorPos(lastBtnX, btnY);
            Thread.Sleep(200);
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, IntPtr.Zero);
            Thread.Sleep(100);
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, IntPtr.Zero);
        } else {
            // Fallback: try clicking near system tray clock area
            RECT nr;
            GetWindowRect(trayNotify, out nr);
            int clickX = nr.Right - 20;
            int clickY = (nr.Top + nr.Bottom) / 2;
            Console.WriteLine("Fallback click at: {0},{1}", clickX, clickY);
            SetCursorPos(clickX, clickY);
            Thread.Sleep(200);
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, IntPtr.Zero);
            Thread.Sleep(100);
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, IntPtr.Zero);
        }
        
        Thread.Sleep(1000); // Wait for menu to appear
        
        // Capture full screen
        Bitmap bmp = new Bitmap(screenW, screenH);
        Graphics g = Graphics.FromImage(bmp);
        g.CopyFromScreen(0, 0, 0, 0, new Size(screenW, screenH));
        
        string path = "C:\\Users\\thundersoft\\.openclaw\\workspace\\AnyClaw_LVGL\\tray_menu_full.png";
        bmp.Save(path, ImageFormat.Png);
        Console.WriteLine("Saved: " + path);
        g.Dispose();
        bmp.Dispose();
    }
}
