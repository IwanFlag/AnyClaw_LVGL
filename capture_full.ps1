Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class WH {
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT r);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern IntPtr FindWindow(string c, string t);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L,T,R,B; }
}
"@
$hwnd = [IntPtr]::new(5378208)
$r = New-Object WH+RECT
[WH]::GetWindowRect($hwnd, [ref]$r)
[WH]::SetForegroundWindow($hwnd)
Start-Sleep -Milliseconds 300

$x = $r.L; $y = $r.T; $w = $r.R - $r.L; $h = $r.B - $r.T
# Use the actual window dimensions
$bmp = New-Object System.Drawing.Bitmap($w, $h)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$size = New-Object System.Drawing.Size -ArgumentList $w, $h
$g.CopyFromScreen($x, $y, 0, 0, $size)
$bmp.Save("C:\Users\thundersoft\.openclaw\workspace\AnyClaw_LVGL\full_window.png")
Write-Output "Full window ${w}x${h} saved"
