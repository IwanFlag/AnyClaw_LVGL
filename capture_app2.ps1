Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class WinHelper {
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern IntPtr FindWindow(string cls, string title);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L,T,R,B; }
}
"@
# Use the HWND we found
$hwnd = [IntPtr]::new(5378208)
$rect = New-Object WinHelper+RECT
[WinHelper]::GetWindowRect($hwnd, [ref]$rect)
[WinHelper]::SetForegroundWindow($hwnd)
Start-Sleep -Milliseconds 500

$x = $rect.L
$y = $rect.T
$w = $rect.R - $rect.L
$h = $rect.B - $rect.T
Write-Output "Window: ($x,$y) size ${w}x${h}"

$bmp = New-Object System.Drawing.Bitmap($w, $h)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($x, $y, 0, 0, $bmp.Size)
$bmp.Save("C:\Users\thundersoft\.openclaw\workspace\AnyClaw_LVGL\app_window.png")
Write-Output "App window screenshot saved"
