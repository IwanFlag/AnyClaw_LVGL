Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
using System.Drawing;
using System.Drawing.Imaging;

public class ScreenshotHelper {
    [DllImport("user32.dll")]
    static extern IntPtr GetDesktopWindow();
    
    [DllImport("user32.dll")]
    static extern IntPtr GetDC(IntPtr hwnd);
    
    [DllImport("user32.dll")]
    static extern int ReleaseDC(IntPtr hwnd, IntPtr hdc);
    
    [DllImport("user32.dll")]
    static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    
    [DllImport("user32.dll")]
    static extern int GetWindowText(IntPtr hWnd, System.Text.StringBuilder lpString, int nMaxCount);
    
    [DllImport("user32.dll")]
    static extern int GetWindowTextLength(IntPtr hWnd);
    
    [DllImport("user32.dll")]
    static extern bool IsWindowVisible(IntPtr hWnd);
    
    [DllImport("user32.dll")]
    static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);
    
    [DllImport("user32.dll")]
    static extern IntPtr GetWindowDC(IntPtr hwnd);
    
    [DllImport("gdi32.dll")]
    static extern bool BitBlt(IntPtr hdcDest, int xDest, int yDest, int wDest, int hDest, IntPtr hdcSrc, int xSrc, int ySrc, uint rop);
    
    public static void CaptureAllWindows(uint targetPid, string outputPath) {
        var desktop = GetDesktopWindow();
        var desktopDC = GetDC(desktop);
        
        // Capture entire desktop
        int w = System.Windows.Forms.SystemInformation.VirtualScreen.Width;
        int h = System.Windows.Forms.SystemInformation.VirtualScreen.Height;
        
        using (var bmp = new Bitmap(w, h)) {
            using (var g = Graphics.FromImage(bmp)) {
                var gDC = g.GetHdc();
                BitBlt(gDC, 0, 0, w, h, desktopDC, 0, 0, 0x00CC0020);
                g.ReleaseHdc(gDC);
            }
            bmp.Save(outputPath);
        }
        
        ReleaseDC(desktop, desktopDC);
    }
    
    public static IntPtr FindWindowByPid(uint pid) {
        IntPtr found = IntPtr.Zero;
        EnumWindows((hWnd, lParam) => {
            uint windowPid;
            GetWindowThreadProcessId(hWnd, out windowPid);
            if (windowPid == pid && IsWindowVisible(hWnd)) {
                int len = GetWindowTextLength(hWnd);
                if (len > 0) {
                    found = hWnd;
                    return false;
                }
            }
            return true;
        }, IntPtr.Zero);
        return found;
    }
}
"@ -ReferencedAssemblies System.Windows.Forms, System.Drawing

# The app PID
$targetPid = 38716
$outputPath = "C:\Users\thundersoft\.openclaw\workspace\screenshot_p2_full.png"

# Check if window exists
$hwnd = [ScreenshotHelper]::FindWindowByPid($targetPid)
Write-Output "Found window: $hwnd"

# Capture full screen
[ScreenshotHelper]::CaptureAllWindows($targetPid, $outputPath)
Write-Output "Screenshot saved to $outputPath"
