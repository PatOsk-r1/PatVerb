# Builds the shareable installer dist\PatVerb-<version>-Setup.exe:
#   1. collects your own presets into presets/ (they ship inside the plugin),
#   2. builds the optimised VST3 + Standalone,
#   3. wraps them with Inno Setup.
#   powershell -ExecutionPolicy Bypass -File tools/package.ps1
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root
& powershell -ExecutionPolicy Bypass -File tools/collect-presets.ps1 -Plugin PatVerb -Extension pvpreset
if ($LASTEXITCODE) { throw 'Collecting presets failed' }
& powershell -ExecutionPolicy Bypass -File tools/build.ps1 -Preset dist -Target PatVerb_All -CMakeArgs '-DPATVERB_COPY_VST3=OFF'
if ($LASTEXITCODE) { throw 'Build failed' }
$version = (Select-String -Path CMakeLists.txt -Pattern 'project\(PatVerb VERSION ([0-9.]+)').Matches[0].Groups[1].Value
$iscc = @("$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe", "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe", "$env:ProgramFiles\Inno Setup 6\ISCC.exe") |
    Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $iscc) { throw 'Inno Setup 6 not found' }
& $iscc /Qp "/DAppVersion=$version" "/DSourceDir=$root\build\dist\PatVerb_artefacts\Release" installer\PatVerb.iss
if ($LASTEXITCODE) { throw 'Installer build failed' }
Get-ChildItem dist\*.exe | ForEach-Object { "{0}  ({1:N1} MB)" -f $_.FullName, ($_.Length / 1MB) }
