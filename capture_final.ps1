Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class WH {
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT r);
    [DllImport("user32.dll")] public static extern IntPtr FindWindow(string c, string t);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L,T,R,B; }
}
"@
$hwnd = [WH]::FindWindow($null, "AnyClaw LVGL v2.0 - Desktop Manager")
if ($hwnd -eq [IntPtr]::Zero) { Write-Output "Window not found"; exit }
$r = New-Object WH+RECT
[WH]::GetWindowRect($hwnd, [ref]$r)

$x = $r.L; $y = $r.T; $w = $r.R - $r.L; $h = $r.B - $r.T
Write-Output "Window: ($x,$y) size ${w}x${h}"

$bmp = New-Object System.Drawing.Bitmap($w, $h)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($x, $y, 0, 0, (New-Object System.Drawing.Size $w, $h))
$bmp.Save("C:\Users\thundersoft\.openclaw\workspace\AnyClaw_LVGL\full_window.png")
Write-Output "Saved ${w}x${h}"
