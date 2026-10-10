param(
    [string]$OpenXRRoot = $env:OPENXR_SDK_ROOT,
    [ValidateSet("Debug","Release")][string]$Configuration = "Release"
)
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    throw "cmake.exe not found. Install CMake or add Android Studio SDK cmake bin directory to PATH."
}
if (-not $env:VULKAN_SDK -or -not (Test-Path (Join-Path $env:VULKAN_SDK "Bin\glslc.exe"))) {
    throw "Install Vulkan SDK and set VULKAN_SDK (must contain Bin\glslc.exe)."
}
if (-not $OpenXRRoot) {
    throw "Provide -OpenXRRoot pointing to an installed Khronos OpenXR-SDK (include/ and lib/), or set OPENXR_SDK_ROOT."
}
$includes = @("$OpenXRRoot\include", "$OpenXRRoot\Include") | Where-Object { Test-Path (Join-Path $_ "openxr\openxr.h") }
$libs = @("$OpenXRRoot\lib\openxr_loader.lib", "$OpenXRRoot\Lib\openxr_loader.lib", "$OpenXRRoot\lib\Release\openxr_loader.lib", "$OpenXRRoot\Lib\Release\openxr_loader.lib") | Where-Object { Test-Path $_ }
if (-not $includes -or -not $libs) { throw "OpenXR headers or openxr_loader.lib not found under $OpenXRRoot" }
$build = Join-Path $root "build\windows"
& cmake -S $root -B $build -A x64 "-DOPENXR_INCLUDE_DIR=$($includes[0])" "-DOPENXR_LOADER_LIBRARY=$($libs[0])"
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }
& cmake --build $build --config $Configuration --target voxen_windows --parallel
if ($LASTEXITCODE -ne 0) { throw "Windows build failed" }
Write-Host "EXE output: $build\$Configuration\voxen_windows.exe"
