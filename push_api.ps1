# Push via Git Data API using curl for lower memory
param()

$ErrorActionPreference = "Continue"
$token = $env:GITHUB_TOKEN
$baseUrl = "https://api.github.com/repos/IwanFlag/AnyClaw_LVGL"
$basePath = "C:\Users\thundersoft\.openclaw\workspace\AnyClaw_LVGL"

# Get HEAD
$r = curl.exe -s -H "Authorization: token $token" -H "Accept: application/vnd.github.v3+json" "$baseUrl/git/ref/heads/main"
$headSha = ($r | ConvertFrom-Json).object.sha
Write-Host "HEAD: $headSha"

# Create a temporary file to store blob SHAs
$blobFile = "$env:TEMP\blobs.jsonl"
if (Test-Path $blobFile) { Remove-Item $blobFile }

# Collect files
$skip = @('.git', 'build', 'thirdparty\lvgl\src', 'thirdparty\lvgl\docs', 'thirdparty\lvgl\examples', 'thirdparty\lvgl\demos')
$i = 0; $ok = 0
Get-ChildItem -Path $basePath -File -Recurse | Sort-Object FullName | ForEach-Object {
    $skipIt = $false
    foreach ($s in $skip) { if ($_.FullName -match [regex]::Escape($s)) { $skipIt = $true; break } }
    if ($skipIt -or $_.Length -ge 1MB) { return }
    
    $rel = $_.FullName.Substring($basePath.Length + 1).Replace('\', '/')
    $content = [IO.File]::ReadAllText($_.FullName)
    $b64 = [Convert]::ToBase64String([Text.Encoding]::UTF8.GetBytes($content))
    $body = @{content = $b64; encoding = "base64"} | ConvertTo-Json -Compress
    $tmp = [IO.Path]::GetTempFileName()
    [IO.File]::WriteAllText($tmp, $body)
    
    $result = curl.exe -s -X POST -H "Authorization: token $token" -H "Accept: application/vnd.github.v3+json" -H "Content-Type: application/json" -d "@$tmp" "$baseUrl/git/blobs"
    Remove-Item $tmp -Force
    $sha = ($result | ConvertFrom-Json).sha
    if ($sha) {
        "$rel`t$sha" | Out-File $blobFile -Append -Encoding utf8
        $ok++
    }
    $i++
    Remove-Variable content, b64, body -ErrorAction SilentlyContinue
    if ($i % 100 -eq 0) {
        [GC]::Collect()
        Write-Host "  $i blobs [ok=$ok]"
    }
}
Write-Host "  $i blobs [ok=$ok]"

# Build tree from blob file
$treeEntries = @()
Get-Content $blobFile | ForEach-Object {
    $parts = $_ -split "`t"
    $treeEntries += @{path = $parts[0]; mode = "100644"; type = "blob"; sha = $parts[1]}
}

# Create tree in batches of 500
Write-Host "Creating tree with $($treeEntries.Count) entries..."
$currentSha = $null
for ($b = 0; $b -lt [Math]::Ceiling($treeEntries.Count / 500); $b++) {
    $start = $b * 500
    $end = [Math]::Min($start + 499, $treeEntries.Count - 1)
    $batch = $treeEntries[$start..$end]
    $body = @{tree = $batch} | ConvertTo-Json -Depth 6
    if ($currentSha) { $body = $body.TrimEnd('}') + ",`"base_tree`":`"$currentSha`"}" }
    $tmp = [IO.Path]::GetTempFileName()
    [IO.File]::WriteAllText($tmp, $body)
    $result = curl.exe -s -X POST -H "Authorization: token $token" -H "Accept: application/vnd.github.v3+json" -H "Content-Type: application/json" -d "@$tmp" "$baseUrl/git/trees"
    Remove-Item $tmp -Force
    $currentSha = ($result | ConvertFrom-Json).sha
    Write-Host "  Tree batch $b: $currentSha"
}
$treeSha = $currentSha

# Create commit
Write-Host "Creating commit..."
$commitBody = @{message = "fix: maximize/restore, window size, garbled text"; tree = $treeSha; parents = @($headSha)} | ConvertTo-Json
$tmp = [IO.Path]::GetTempFileName()
[IO.File]::WriteAllText($tmp, $commitBody)
$result = curl.exe -s -X POST -H "Authorization: token $token" -H "Accept: application/vnd.github.v3+json" -H "Content-Type: application/json" -d "@$tmp" "$baseUrl/git/commits"
Remove-Item $tmp -Force
$commitSha = ($result | ConvertFrom-Json).sha
Write-Host "Commit: $commitSha"

# Update ref
Write-Host "Updating ref..."
$refBody = @{sha = $commitSha} | ConvertTo-Json
$tmp = [IO.Path]::GetTempFileName()
[IO.File]::WriteAllText($tmp, $refBody)
$result = curl.exe -s -X PATCH -H "Authorization: token $token" -H "Accept: application/vnd.github.v3+json" -H "Content-Type: application/json" -d "@$tmp" "$baseUrl/git/refs/heads/main"
Remove-Item $tmp -Force
Write-Host "Done! Pushed $ok files."

Remove-Item $blobFile -Force -ErrorAction SilentlyContinue
