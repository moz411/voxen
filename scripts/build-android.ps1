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
# Bootstrap the official wrapper JAR, which is not committed to the repository.
# Gradle's distribution service publishes this small JAR and its SHA256 digest.
if (-not (Test-Path $wrapper)) {
    $properties = Get-Content (Join-Path $android "gradle\wrapper\gradle-wrapper.properties") -Raw
    $match = [regex]::Match($properties, 'gradle-([0-9][0-9A-Za-z.\-]*)-(?:bin|all)\.zip')
    if (-not $match.Success) { throw "Cannot determine Gradle version from gradle-wrapper.properties" }
    $version = $match.Groups[1].Value
    $url = "https://services.gradle.org/distributions/gradle-$version-wrapper.jar"
    $tmp = "$wrapper.download"
    Write-Host "Downloading Gradle $version wrapper JAR..."
    try {
        Invoke-WebRequest -Uri $url -OutFile $tmp -UseBasicParsing
        $expected = (Invoke-RestMethod -Uri "$url.sha256").ToString().Trim().Split(" ")[0].ToLowerInvariant()
        $actual = (Get-FileHash -Path $tmp -Algorithm SHA256).Hash.ToLowerInvariant()
        if ($actual -ne $expected) { throw "Gradle wrapper SHA256 mismatch" }
        Move-Item -Force $tmp $wrapper
    } finally {
        if (Test-Path $tmp) { Remove-Item -Force $tmp }
    }
}
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
