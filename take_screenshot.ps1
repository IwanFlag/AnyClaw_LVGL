Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
$bounds = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
$bmp = New-Object System.Drawing.Bitmap($bounds.Width, $bounds.Height)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen(0, 0, 0, 0, $bounds.Size)
$bmp.Save("C:\Users\thundersoft\.openclaw\workspace\AnyClaw_LVGL\debug_check.png")
$g.Dispose()
$bmp.Dispose()
Write-Host "Screenshot saved to debug_check.png"
