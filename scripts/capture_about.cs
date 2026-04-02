using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
using System.Threading;

class Program {
    [DllImport("user32.dll")]
    static extern IntPtr FindWindow(string cls, string win);
    [DllImport("user32.dll")]
    static extern void keybd_event(byte vk, byte scan, uint flags, IntPtr ex);
    [DllImport("user32.dll")]
    static extern int GetSystemMetrics(int n);
    [DllImport("user32.dll")]
    static extern IntPtr PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
    [DllImport("user32.dll")]
    static extern bool SetForegroundWindow(IntPtr h);

    const uint WM_COMMAND = 0x0111;
    const uint KEYEVENTF_KEYUP = 0x0002;

    static void Main() {
        // IDM_ABOUT = 404, IDM_EXIT = 405 (from tray.h)
        // Find the tray window and send IDM_ABOUT command
        IntPtr trayWnd = FindWindow("AnyClaw_TrayWnd", null);
        if (trayWnd == IntPtr.Zero) {
            Console.WriteLine("Tray not found");
            return;
        }
        
        // Post IDM_ABOUT command (ID 404) 
        // In the tray proc, IDM_ABOUT is handled in WM_COMMAND
        // We can send WM_COMMAND with wParam = IDM_ABOUT
        Console.WriteLine("Sending IDM_ABOUT...");
        PostMessage(trayWnd, WM_COMMAND, (IntPtr)404, IntPtr.Zero);
        
        // Wait for the dialog to appear
        Thread.Sleep(2000);
        
        // Find the About dialog
        IntPtr aboutWnd = FindWindow("AnyClaw_AboutDlg", null);
        if (aboutWnd == IntPtr.Zero) {
            Console.WriteLine("About dialog not found, checking for any dialog...");
            // Try alternate: it might be a standard dialog
            aboutWnd = FindWindow("#32770", null); // dialog class
        }
        
        if (aboutWnd != IntPtr.Zero) {
            Console.WriteLine("About dialog found: " + aboutWnd);
        } else {
            Console.WriteLine("About dialog still not found");
        }
        
        // Capture the screen
        int sw = GetSystemMetrics(0);
        int sh = GetSystemMetrics(1);
        Bitmap bmp = new Bitmap(sw, sh);
        Graphics g = Graphics.FromImage(bmp);
        g.CopyFromScreen(0, 0, 0, 0, new Size(sw, sh));
        string path = "C:\\Users\\thundersoft\\.openclaw\\workspace\\AnyClaw_LVGL\\about_dialog.png";
        bmp.Save(path, ImageFormat.Png);
        Console.WriteLine("Saved: " + path);
        g.Dispose();
        bmp.Dispose();
        
        // Close dialog with Escape
        keybd_event(0x1B, 0, 0, IntPtr.Zero);
        Thread.Sleep(50);
        keybd_event(0x1B, 0, KEYEVENTF_KEYUP, IntPtr.Zero);
    }
}
