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

# When the repository has no gradle-wrapper.jar, use a verified official
# Gradle distribution. Keep it in the user's Gradle cache for later builds.
$gradleExe = $null
if (Test-Path $wrapper) {
    $gradleExe = Join-Path $android "gradlew.bat"
} elseif (Get-Command gradle -ErrorAction SilentlyContinue) {
    $gradleExe = (Get-Command gradle).Source
} else {
    $properties = Get-Content (Join-Path $android "gradle\wrapper\gradle-wrapper.properties") -Raw
    $match = [regex]::Match($properties, 'gradle-([0-9][0-9A-Za-z.\-]*)-(?:bin|all)\.zip')
    if (-not $match.Success) { throw "Cannot determine Gradle version from gradle-wrapper.properties" }
    $version = $match.Groups[1].Value
    $cache = Join-Path $env:USERPROFILE ".gradle\voxen-dist"
    $gradleExe = Join-Path $cache "gradle-$version\bin\gradle.bat"
    if (-not (Test-Path $gradleExe)) {
        New-Item -ItemType Directory -Force -Path $cache | Out-Null
        $uri = "https://services.gradle.org/distributions/gradle-$version-bin.zip"
        $archive = Join-Path $cache "gradle-$version-bin.zip"
        Write-Host "Downloading verified Gradle $version distribution..."
        try {
            Invoke-WebRequest -Uri $uri -OutFile $archive -UseBasicParsing
            $sha = ((Invoke-WebRequest -Uri "$uri.sha256" -UseBasicParsing).Content).Trim().Split(" ")[0].ToLowerInvariant()
            $actual = (Get-FileHash $archive -Algorithm SHA256).Hash.ToLowerInvariant()
            if ($sha -ne $actual) { throw "Gradle distribution SHA256 mismatch" }
            Expand-Archive -Path $archive -DestinationPath $cache -Force
        } finally {
            if (Test-Path $archive) { Remove-Item -Force $archive }
        }
    }
}
Push-Location $android
try {
    & $gradleExe --no-daemon $task
    if ($LASTEXITCODE -ne 0) { throw "Gradle build failed with code $LASTEXITCODE" }
    $flavor = if ($Release) { "release" } else { "debug" }
    Write-Host "APK output: $android\app\build\outputs\apk\$flavor"
} finally { Pop-Location }
