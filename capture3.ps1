Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
$w = [System.Windows.Forms.SystemInformation]::VirtualScreen
$b = New-Object System.Drawing.Bitmap($w.Width, $w.Height)
$g = [System.Drawing.Graphics]::FromImage($b)
$g.CopyFromScreen($w.X, $w.Y, 0, 0, $b.Size)
$b.Save('C:\Users\thundersoft\.openclaw\workspace\screenshot_p2.png')
Write-Output 'Screenshot saved'
