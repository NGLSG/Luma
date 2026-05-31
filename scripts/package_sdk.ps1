# LumaEngine SDK Packaging Script
# Usage: .\scripts\package_sdk.ps1 -BuildDir <cmake-build-dir> -OutputDir <sdk-output-dir>
# Example: .\scripts\package_sdk.ps1 -BuildDir .\cmake-build-reldebug -OutputDir .\LumaEngine-SDK

param(
    [Parameter(Mandatory=$true)]
    [string]$BuildDir,

    [Parameter(Mandatory=$false)]
    [string]$OutputDir = ".\LumaEngine-SDK",

    [Parameter(Mandatory=$false)]
    [string]$SourceDir = $PSScriptRoot + "\.."
)

Write-Host "=== LumaEngine SDK Packager ===" -ForegroundColor Cyan
Write-Host "Source: $SourceDir"
Write-Host "Build:  $BuildDir"
Write-Host "Output: $OutputDir"
Write-Host ""

# Clean output
if (Test-Path $OutputDir) {
    Remove-Item -Path $OutputDir -Recurse -Force
}

# Create directory structure
$dirs = @(
    "$OutputDir/include/LumaEngine",
    "$OutputDir/lib",
    "$OutputDir/bin",
    "$OutputDir/cmake",
    "$OutputDir/examples"
)
foreach ($d in $dirs) {
    New-Item -ItemType Directory -Path $d -Force | Out-Null
}

# 1. Copy public headers
Write-Host "[1/5] Copying headers..." -ForegroundColor Yellow
Copy-Item "$SourceDir\Luma_CAPI.h" "$OutputDir\include\LumaEngine\" -Force
Copy-Item "$SourceDir\EngineEntry.h" "$OutputDir\include\LumaEngine\" -Force
New-Item -ItemType Directory -Path "$OutputDir\include\LumaEngine\Utils" -Force | Out-Null
Copy-Item "$SourceDir\Utils\Platform.h" "$OutputDir\include\LumaEngine\Utils\" -Force

# 2. Copy library files
Write-Host "[2/5] Copying libraries..." -ForegroundColor Yellow
$dllPath = Get-ChildItem -Path $BuildDir -Filter "LumaEngine.dll" -Recurse | Select-Object -First 1
$libPath = Get-ChildItem -Path $BuildDir -Filter "LumaEngine.lib" -Recurse | Select-Object -First 1
if ($dllPath) {
    Copy-Item $dllPath.FullName "$OutputDir\bin\" -Force
    Write-Host "  Found DLL: $($dllPath.FullName)"
}
if ($libPath) {
    Copy-Item $libPath.FullName "$OutputDir\lib\" -Force
    Write-Host "  Found LIB: $($libPath.FullName)"
}

# 3. Copy runtime dependencies
Write-Host "[3/5] Copying runtime dependencies..." -ForegroundColor Yellow
$depDlls = @("SDL3.dll")
foreach ($dep in $depDlls) {
    $found = Get-ChildItem -Path $BuildDir -Filter $dep -Recurse | Select-Object -First 1
    if ($found) {
        Copy-Item $found.FullName "$OutputDir\bin\" -Force
        Write-Host "  Found: $dep"
    }
}

# 4. Copy CMake config
Write-Host "[4/5] Copying CMake config..." -ForegroundColor Yellow
if (Test-Path "$SourceDir\cmake\LumaEngineConfig.cmake") {
    Copy-Item "$SourceDir\cmake\LumaEngineConfig.cmake" "$OutputDir\cmake\" -Force
}

# 5. Copy examples and docs
Write-Host "[5/5] Copying examples and docs..." -ForegroundColor Yellow
if (Test-Path "$SourceDir\SDK\examples") {
    Copy-Item "$SourceDir\SDK\examples" "$OutputDir\examples" -Recurse -Force
}
if (Test-Path "$SourceDir\SDK\README.md") {
    Copy-Item "$SourceDir\SDK\README.md" "$OutputDir\" -Force
}

# Summary
Write-Host ""
Write-Host "=== SDK Package Complete ===" -ForegroundColor Green
$totalSize = (Get-ChildItem -Path $OutputDir -Recurse | Measure-Object -Property Length -Sum).Sum / 1MB
Write-Host "Output: $OutputDir"
Write-Host "Size: $([math]::Round($totalSize, 2)) MB"
Write-Host ""
Write-Host "To create archive:" -ForegroundColor Cyan
Write-Host "  Compress-Archive -Path '$OutputDir\*' -DestinationPath 'LumaEngine-SDK-win-x64.zip'"
