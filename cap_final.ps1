Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
$code = @'
using System;
using System.Runtime.InteropServices;
public class WH {
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern IntPtr FindWindow(string c, string t);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L,T,R,B; }
}
'@
Add-Type $code
$hwnd = [IntPtr]::new(2625750)
Write-Output "Using HWND: $hwnd"
$r = New-Object WH+RECT
[WH]::GetWindowRect($hwnd, [ref]$r)
$w = $r.R - $r.L
$h = $r.B - $r.T
Write-Output "Window: $($r.L),$($r.T) ${w}x${h}"
$bmp = New-Object System.Drawing.Bitmap($w, $h)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$sz = New-Object System.Drawing.Size -ArgumentList @($w, $h)
$pt = New-Object System.Drawing.Point -ArgumentList @(0, 0)
$g.CopyFromScreen([int]$r.L, [int]$r.T, [int]0, [int]0, $sz)
$bmp.Save("C:\Users\thundersoft\.openclaw\workspace\AnyClaw_LVGL\full_window.png")
Write-Output "Saved"
