#!/usr/bin/env pwsh
# set_wifi.ps1 – Flash Wi-Fi credentials into NVS on the connected ESP32-C6.
#
# Usage:
#   .\tools\set_wifi.ps1 -SSID "MyNetwork" -Password "MyPassword"
#   .\tools\set_wifi.ps1 -SSID "MyNetwork" -Password "MyPassword" -Port COM5
#
# The script:
#   1. Creates a temporary NVS CSV with the given credentials
#   2. Generates an NVS binary using nvs_partition_gen.py
#   3. Flashes the binary to the NVS partition at 0x9000

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$SSID,

    [Parameter(Mandatory = $true)]
    [string]$Password,

    [string]$Port = "",          # e.g. COM5; auto-detect if empty

    [string]$IdfPath = "C:\esp\v6.0\esp-idf",

    [string]$NvsOffset = "0x9000",
    [string]$NvsSize   = "0x6000"   # 24 KB
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# ---------------------------------------------------------------------------
# 1. Validate inputs
# ---------------------------------------------------------------------------
if ($SSID.Length -gt 31) {
    Write-Error "SSID must be 31 characters or fewer."
    exit 1
}
if ($Password.Length -gt 63) {
    Write-Error "Password must be 63 characters or fewer."
    exit 1
}

# ---------------------------------------------------------------------------
# 2. Locate Python from IDF environment (or system PATH)
# ---------------------------------------------------------------------------
$python = "python"
if (Test-Path "$IdfPath\tools\idf_tools.py") {
    # Source IDF to get the correct python in PATH
    . "$IdfPath\export.ps1" | Out-Null
}

# ---------------------------------------------------------------------------
# 3. Create temporary CSV file
# ---------------------------------------------------------------------------
$tmpDir = New-TemporaryFile | ForEach-Object { Remove-Item $_; New-Item -ItemType Directory -Path $_.FullName }
$csvPath = Join-Path $tmpDir "nvs_wifi.csv"
$binPath = Join-Path $tmpDir "nvs_wifi.bin"

$csvContent = @"
key,type,encoding,value
wifi_creds,namespace,,
ssid,data,string,$SSID
password,data,string,$Password
"@

# Write without BOM – PowerShell 5.x utf8 adds BOM which breaks nvs_partition_gen.py
[System.IO.File]::WriteAllText($csvPath, $csvContent, (New-Object System.Text.UTF8Encoding $false))

Write-Host ""
Write-Host "NVS CSV contents:"
Write-Host $csvContent
Write-Host ""

# ---------------------------------------------------------------------------
# 4. Generate NVS binary
# ---------------------------------------------------------------------------
$genScript = Join-Path $IdfPath "components\nvs_flash\nvs_partition_generator\nvs_partition_gen.py"
if (-not (Test-Path $genScript)) {
    Write-Error "nvs_partition_gen.py not found at: $genScript"
    exit 1
}

Write-Host "Generating NVS binary..."
& $python $genScript generate $csvPath $binPath $NvsSize
if ($LASTEXITCODE -ne 0) {
    Write-Error "nvs_partition_gen.py failed (exit code $LASTEXITCODE)"
    exit 1
}
Write-Host "Generated: $binPath"

# ---------------------------------------------------------------------------
# 5. Flash the NVS partition
# ---------------------------------------------------------------------------
$portArg = @()
if ($Port -ne "") {
    $portArg = @("-p", $Port)
}

Write-Host ""
Write-Host "Flashing NVS to $NvsOffset on the device..."
Write-Host "(Device must be connected and in normal or download mode)"
Write-Host ""

& esptool.py @portArg --chip esp32c6 write_flash $NvsOffset $binPath

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "SUCCESS! Wi-Fi credentials written to NVS."
    Write-Host "  SSID:     $SSID"
    Write-Host "  Password: $('*' * $Password.Length)"
    Write-Host ""
    Write-Host "Reset the device and it will connect automatically."
} else {
    Write-Error "esptool.py failed. Check that the device is connected and the correct port is used."
    Write-Host "To specify a port manually: .\tools\set_wifi.ps1 -SSID '...' -Password '...' -Port COM5"
}

# ---------------------------------------------------------------------------
# 6. Cleanup
# ---------------------------------------------------------------------------
Remove-Item -Recurse -Force $tmpDir
