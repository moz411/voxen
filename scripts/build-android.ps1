param([switch]$Release)
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$android = Join-Path $root "android"
if (-not $env:JAVA_HOME) {
    $jbr = "C:\Program Files\Android\Android Studio\jbr"
    if (Test-Path (Join-Path $jbr "bin\java.exe")) { $env:JAVA_HOME = $jbr }
}
if (-not $env:ANDROID_HOME -and -not $env:ANDROID_SDK_ROOT) {
    $sdk = Join-Path $env:LOCALAPPDATA "Android\Sdk"
    if (Test-Path $sdk) { $env:ANDROID_HOME = $sdk }
}
$task = if ($Release) { "assembleRelease" } else { "assembleDebug" }
$wrapper = Join-Path $android "gradle\wrapper\gradle-wrapper.jar"
Push-Location $android
try {
    if (Test-Path $wrapper) {
        & ".\gradlew.bat" --no-daemon $task
    } elseif (Get-Command gradle -ErrorAction SilentlyContinue) {
        Write-Host "Gradle wrapper JAR absent; using Gradle from PATH."
        & gradle --no-daemon $task
    } else {
        throw "Gradle wrapper JAR is absent. Open android/ in Android Studio and use Build > Build APK(s), or install Gradle on PATH, then rerun."
    }
    if ($LASTEXITCODE -ne 0) { throw "Gradle build failed with code $LASTEXITCODE" }
    $flavor = if ($Release) { "release" } else { "debug" }
    Write-Host "APK output: $android\app\build\outputs\apk\$flavor"
} finally { Pop-Location }
