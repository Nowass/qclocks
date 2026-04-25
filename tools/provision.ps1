#!/usr/bin/env pwsh
# provision.ps1 – Write all NVS settings to the connected ESP32-C6 in one shot.
#
# Writes two NVS namespaces into the same partition image:
#   wifi_creds  -> ssid, password
#   qclocks     -> ota_url  (and any future app settings)
#
# Usage:
#   .\tools\provision.ps1 -SSID "MyNet" -Password "secret" -OtaUrl "https://github.com/.../releases/latest/download/qclocks.bin"
#   .\tools\provision.ps1 -SSID "MyNet" -Password "secret"          # OtaUrl optional
#   .\tools\provision.ps1 -OtaUrl "https://..."                     # WiFi optional (keeps device offline)
#
# At least one of (-SSID + -Password) or -OtaUrl must be supplied.
# Omitted sections are simply not written (the rest of the NVS partition
# is erased, so run this script with ALL parameters you care about).

[CmdletBinding()]
param(
    [string]$SSID = "",
    [string]$Password = "",
    [string]$OtaUrl = "",

    [string]$Port = "",

    [string]$IdfPath = "C:\esp\v6.0\esp-idf",

    [string]$NvsOffset = "0x9000",
    [string]$NvsSize   = "0x6000"   # 24 KB – must match partitions.csv
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# ---------------------------------------------------------------------------
# Validate
# ---------------------------------------------------------------------------
$hasWifi = ($SSID -ne "" -and $Password -ne "")
$hasOta  = ($OtaUrl -ne "")

if (-not $hasWifi -and -not $hasOta) {
    Write-Error "Supply at least (-SSID + -Password) or -OtaUrl."
    exit 1
}
if ($SSID -ne "" -and $Password -eq "") { Write-Error "-Password is required when -SSID is set."; exit 1 }
if ($SSID.Length   -gt 31) { Write-Error "SSID must be <=31 chars.";     exit 1 }
if ($Password.Length -gt 63) { Write-Error "Password must be <=63 chars."; exit 1 }
if ($OtaUrl.Length  -gt 255) { Write-Error "OtaUrl must be <=255 chars.";  exit 1 }

# ---------------------------------------------------------------------------
# Setup IDF Python
# ---------------------------------------------------------------------------
if (Test-Path "$IdfPath\tools\idf_tools.py") {
    . "$IdfPath\export.ps1" | Out-Null
}

# ---------------------------------------------------------------------------
# Build NVS CSV
# ---------------------------------------------------------------------------
$rows = @("key,type,encoding,value")

if ($hasWifi) {
    $rows += "wifi_creds,namespace,,"
    $rows += "ssid,data,string,$SSID"
    $rows += "password,data,string,$Password"
}

if ($hasOta) {
    $rows += "qclocks,namespace,,"
    $rows += "ota_url,data,string,$OtaUrl"
}

$csvContent = $rows -join "`n"

# ---------------------------------------------------------------------------
# Write temp files (no BOM – Python's nvs_partition_gen.py chokes on BOM)
# ---------------------------------------------------------------------------
$tmpDir  = New-TemporaryFile | ForEach-Object { Remove-Item $_; New-Item -ItemType Directory -Path $_.FullName }
$csvPath = Join-Path $tmpDir "nvs_provision.csv"
$binPath = Join-Path $tmpDir "nvs_provision.bin"

[System.IO.File]::WriteAllText($csvPath, $csvContent, (New-Object System.Text.UTF8Encoding $false))

Write-Host ""
Write-Host "NVS CSV:"
Write-Host $csvContent
Write-Host ""

# ---------------------------------------------------------------------------
# Generate NVS binary
# ---------------------------------------------------------------------------
$genScript = Join-Path $IdfPath "components\nvs_flash\nvs_partition_generator\nvs_partition_gen.py"
if (-not (Test-Path $genScript)) {
    Write-Error "nvs_partition_gen.py not found: $genScript"
    exit 1
}

Write-Host "Generating NVS binary..."
& python $genScript generate $csvPath $binPath $NvsSize
if ($LASTEXITCODE -ne 0) { Write-Error "nvs_partition_gen.py failed"; exit 1 }

# ---------------------------------------------------------------------------
# Flash
# ---------------------------------------------------------------------------
$portArg = if ($Port -ne "") { @("-p", $Port) } else { @() }

Write-Host "Flashing NVS partition to $NvsOffset..."
& esptool.py @portArg --chip esp32c6 write_flash $NvsOffset $binPath
if ($LASTEXITCODE -ne 0) { Write-Error "esptool.py failed"; exit 1 }

Write-Host ""
Write-Host "SUCCESS! NVS written:"
if ($hasWifi) { Write-Host "  SSID:    $SSID" }
if ($hasOta)  { Write-Host "  OtaUrl:  $OtaUrl" }
Write-Host ""
Write-Host "Reset the device to apply."

Remove-Item -Recurse -Force $tmpDir
