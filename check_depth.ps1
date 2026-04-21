$lines = Get-Content "D:\workspace\AnyClaw\AnyClaw_LVGL_Code\AnyClaw_LVGL\src\ui_main.cpp" -Encoding UTF8
$depth = 0
for ($i = 11314; $i -lt 11650; $i++) {
    $line = $lines[$i]
    # strip line comments
    $idx = $line.IndexOf("//")
    if ($idx -ge 0) { $line = $line.Substring(0, $idx) }
    # count braces
    foreach ($ch in $line.ToCharArray()) {
        if ($ch -eq '{') { $depth++ }
        elseif ($ch -eq '}') { $depth-- }
    }
    if ($i -ge 11636 -and $i -le 11685) {
        $prefix = " " * [Math]::Max(0, 4 - $depth.ToString().Length)
        Write-Host "$prefix depth=$($depth): line $($i+1): $($lines[$i].Substring(0, [Math]::Min(70, $lines[$i].Length)))"
    }
    if ($depth -lt 0) {
        Write-Host "UNDERFLOW at line $($i+1): depth=$depth"
        break
    }
}
