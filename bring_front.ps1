Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Win32 {
    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")]
    public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
    [DllImport("user32.dll")]
    public static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, uint uFlags);
}
"@
$proc = Get-Process -Name AnyClaw_LVGL -ErrorAction SilentlyContinue
if ($proc) {
    $hwnd = $proc.MainWindowHandle
    [Win32]::ShowWindow($hwnd, 9)  # SW_RESTORE
    [Win32]::SetWindowPos($hwnd, [IntPtr]::Zero, 0, 0, 1280, 800, 0x0040)
    [Win32]::SetForegroundWindow($hwnd)
    Start-Sleep -Seconds 1
    Write-Host "Window moved and brought to front"
} else {
    Write-Host "Process not found"
}
