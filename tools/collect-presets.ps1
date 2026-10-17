# Copies the author's own presets (Documents\<Plugin>\Presets) into presets/ so the next build
# ships them inside the plugin for everyone. Sub-folders become categories; presets saved in
# the root go to "Community"; a sub-folder named "Privado" is never shared.
#   powershell -ExecutionPolicy Bypass -File tools/collect-presets.ps1
param([string]$Plugin = 'PatWave', [string]$Extension = 'pwpreset')
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$src = Join-Path ([Environment]::GetFolderPath('MyDocuments')) "$Plugin\Presets"
$dst = Join-Path $root 'presets'
New-Item -ItemType Directory -Force $dst | Out-Null
Get-ChildItem $dst -Recurse -File -Filter "*.$Extension" | Remove-Item -Force
Get-ChildItem $dst -Recurse -Directory | Sort-Object FullName -Descending | Where-Object { -not (Get-ChildItem $_.FullName) } | Remove-Item -Force
if (-not (Test-Path $src)) { Write-Host "No user presets yet ($src)"; return }
$count = 0
foreach ($f in Get-ChildItem $src -Recurse -File -Filter "*.$Extension") {
    $rel = $f.DirectoryName.Substring($src.Length).TrimStart('\')
    if ($rel -eq 'Privado' -or $rel.StartsWith('Privado\')) { continue }
    $category = if ($rel) { $rel.Replace('\', '/') } else { 'Community' }
    [xml]$xml = Get-Content -LiteralPath $f.FullName -Raw -Encoding UTF8
    $xml.DocumentElement.SetAttribute('category', $category)
    $outDir = if ($rel) { Join-Path $dst $rel } else { Join-Path $dst 'Community' }
    New-Item -ItemType Directory -Force $outDir | Out-Null
    $xml.Save((Join-Path $outDir $f.Name))
    $count++
}
Write-Host "Shared presets collected: $count"
