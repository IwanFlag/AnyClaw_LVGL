$MsvcBin = "C:\VSBuildTools\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64"

$origPath = $env:Path
$origLib = $env:LIB
$origInclude = $env:INCLUDE

try {
    $env:Path = "$MsvcBin;$env:SystemRoot\System32;$env:SystemRoot"
    $env:LIB = "C:\VSBuildTools\VC\Tools\MSVC\14.50.35717\lib\x64;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\ucrt\x64;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\um\x64"
    $env:INCLUDE = "C:\VSBuildTools\VC\Tools\MSVC\14.50.35717\include;C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\ucrt;C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\shared;C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um;C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\winrt;D:\workspace\AnyClaw\AnyClaw_LVGL_Code\AnyClaw_LVGL\src;D:\workspace\AnyClaw\AnyClaw_LVGL_Code\AnyClaw_LVGL\thirdparty\lvgl\src;D:\workspace\AnyClaw\AnyClaw_LVGL_Code\AnyClaw_LVGL\thirdparty\sdl2-windows\include"

    Set-Location "D:\workspace\AnyClaw\AnyClaw_LVGL_Code\AnyClaw_LVGL"
    $cl = "$MsvcBin\cl.exe"
    & $cl /nologo /TP /c /utf-8 /std:c++17 /DNOMINMAX /EHsc /O2 /Ob2 /MD `
        /ID:\workspace\AnyClaw\AnyClaw_LVGL_Code\AnyClaw_LVGL\src `
        /ID:\workspace\AnyClaw\AnyClaw_LVGL_Code\AnyClaw_LVGL\thirdparty\lvgl\src `
        /ID:\workspace\AnyClaw\AnyClaw_LVGL_Code\AnyClaw_LVGL\thirdparty\sdl2-windows\include `
        /Fo"$env:TEMP\ui_main_quick.obj" `
        src\ui_main.cpp 2>&1 | Select-Object -First 30
}
finally {
    if ($null -ne $origPath) { $env:Path = $origPath } else { Remove-Item Env:Path -ErrorAction SilentlyContinue }
    if ($null -ne $origLib) { $env:LIB = $origLib } else { Remove-Item Env:LIB -ErrorAction SilentlyContinue }
    if ($null -ne $origInclude) { $env:INCLUDE = $origInclude } else { Remove-Item Env:INCLUDE -ErrorAction SilentlyContinue }
}
