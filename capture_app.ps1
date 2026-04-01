Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
# Find the app window
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class WinHelper {
    [DllImport("user32.dll")] public static extern IntPtr FindWindow(string cls, string title);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L,T,R,B; }
}
"@
$hwnd = [WinHelper]::FindWindow($null, "AnyClaw LVGL v2.0 - Desktop Manager")
if ($hwnd -eq [IntPtr]::Zero) { Write-Output "Window not found"; exit }
$rect = New-Object WinHelper+RECT
[WinHelper]::GetWindowRect($hwnd, [ref]$rect)
[WinHelper]::SetForegroundWindow($hwnd)
Start-Sleep -Milliseconds 500

$x = $rect.L
$y = $rect.T
$w = $rect.R - $rect.L
$h = $rect.B - $rect.T
Write-Output "Window: ($x,$y) size ${w}x${h}"

# Capture just the app window area with some margin
$bmp = New-Object System.Drawing.Bitmap($w, $h)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($x, $y, 0, 0, $bmp.Size)
$bmp.Save("C:\Users\thundersoft\.openclaw\workspace\AnyClaw_LVGL\app_window.png")
Write-Output "App window screenshot saved"
