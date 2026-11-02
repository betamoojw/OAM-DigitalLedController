[CmdletBinding()]
param(
    [string]$Version = "4.3.12",
    [string]$Destination = ".tools\OpenKNXproducer"
)

$root = Split-Path -Parent $PSScriptRoot
$destinationPath = Join-Path $root $Destination
$cachedProducer = Get-ChildItem $destinationPath -Recurse -Filter "OpenKNXproducer-x64.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
if ($null -ne $cachedProducer) {
    Write-Output $cachedProducer.FullName
    exit 0
}

$release = Invoke-RestMethod -Headers @{ "User-Agent" = "OAM-DigitalLEDController-CI" } `
    -Uri "https://api.github.com/repos/OpenKNX/OpenKNXproducer/releases/tags/v$Version"
$asset = $release.assets | Where-Object { $_.name -match "(?i)\.zip$" } | Select-Object -First 1
if ($null -eq $asset) {
    throw "No OpenKNXproducer zip asset found for v$Version"
}

$archive = Join-Path $env:TEMP $asset.name
Invoke-WebRequest -Headers @{ "User-Agent" = "OAM-DigitalLEDController-CI" } -Uri $asset.browser_download_url -OutFile $archive
if (Test-Path $destinationPath) { Remove-Item $destinationPath -Recurse -Force }
Expand-Archive -Path $archive -DestinationPath $destinationPath -Force
$producer = Get-ChildItem $destinationPath -Recurse -Filter "OpenKNXproducer-x64.exe" | Select-Object -First 1
if ($null -eq $producer) {
    throw "OpenKNXproducer-x64.exe was not found in $destinationPath"
}
Write-Output $producer.FullName
