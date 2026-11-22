param(
    [Parameter(Mandatory=$true)][string]$BuildDirectory,
    [Parameter(Mandatory=$true)][string]$OutputDirectory
)
$ErrorActionPreference = "Stop"
$source = Split-Path $PSScriptRoot -Parent
$package = Join-Path $OutputDirectory "process-dispatcher-windows-x64"
New-Item -ItemType Directory -Force $package | Out-Null
Copy-Item "$BuildDirectory/Release/process_dispatcher.exe", "$BuildDirectory/Release/dispatcher_unit_tests.exe" $package
Copy-Item "$source/README.md", "$PSScriptRoot/*.cmd" $package
foreach ($directory in @("configs", "docs", "tools", "tests", "observability", "reports")) {
    Copy-Item "$source/$directory" $package -Recurse
}
$licenses = New-Item -ItemType Directory -Force "$package/licenses"
Get-ChildItem "$BuildDirectory/vcpkg_installed/x64-windows-static-release/share/*/copyright" | ForEach-Object {
    Copy-Item $_.FullName (Join-Path $licenses ($_.Directory.Name + ".txt"))
}
if (!(Test-Path "$licenses/curl.txt") -or !(Test-Path "$licenses/boost-process.txt")) {
    throw "Dependency license files missing"
}
# Validate the actual extracted distribution, including spaces and Unicode in its path.
$archive = Join-Path $OutputDirectory "process-dispatcher-windows-x64.zip"
Compress-Archive -Path $package -DestinationPath $archive -Force
$validation = Join-Path $OutputDirectory "Проверка архива с пробелами"
Expand-Archive $archive -DestinationPath $validation -Force
$extracted = Join-Path $validation "process-dispatcher-windows-x64"
& "$extracted/tests.cmd"
if ($LASTEXITCODE -ne 0) { throw "Packaged tests failed: $LASTEXITCODE" }
& "$extracted/run.cmd"
if ($LASTEXITCODE -ne 0) { throw "Packaged demo failed: $LASTEXITCODE" }
$hash = (Get-FileHash $archive -Algorithm SHA256).Hash.ToLowerInvariant()
"$hash  process-dispatcher-windows-x64.zip" | Set-Content "$OutputDirectory/SHA256SUMS.txt" -Encoding ascii
