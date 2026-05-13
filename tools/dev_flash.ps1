#!/usr/bin/env pwsh
# dev_flash.ps1 - Flash app + reset OTA boot pointer to ota_0.
#
# Flashes two partitions only (faster than full flash):
#   0xF000   ota_data_initial.bin  - resets bootloader to boot from ota_0
#   0x20000  qclocks.bin           - the freshly built app
#
# This is needed because OTA updates write to ota_1 and update otadata to
# "boot from ota_1".  A plain "Flash app only" writes to ota_0 but leaves
# otadata unchanged, so the device keeps booting the old ota_1 firmware.
#
# Usage:
#   .\tools\dev_flash.ps1
#   .\tools\dev_flash.ps1 -Port COM5
#   .\tools\dev_flash.ps1 -BuildDir build -Port COM5

[CmdletBinding()]
param(
    [string]$Port = "",
    [string]$BuildDir = "build",
    [string]$IdfPath = "C:\esp\v6.0\esp-idf"
)

$ErrorActionPreference = "Stop"

# -- resolve paths ----------------------------------------------------------
$Root = Split-Path $PSScriptRoot -Parent
$OtaData = Join-Path (Join-Path $Root $BuildDir) "ota_data_initial.bin"
$AppBin = Join-Path (Join-Path $Root $BuildDir) "qclocks.bin"

foreach ($f in @($OtaData, $AppBin)) {
    if (-not (Test-Path $f)) {
        Write-Error "File not found: $f`nRun a build first."
    }
}

# -- activate IDF Python env ------------------------------------------------
$exportScript = Join-Path $IdfPath "export.ps1"
if (Test-Path $exportScript) {
    . $exportScript | Out-Null
}

# -- build esptool args -----------------------------------------------------
$portArgs = if ($Port) { @("--port", $Port) } else { @() }

$flashArgs = $portArgs + @(
    "--chip", "esp32c6",
    "--baud", "460800",
    "write_flash",
    "--flash-mode", "dio",
    "--flash-freq", "80m",
    "--flash-size", "2MB",
    "0xf000", $OtaData,
    "0x20000", $AppBin
)

Write-Host ""
Write-Host "dev_flash: resetting OTA pointer + flashing app" -ForegroundColor Cyan
Write-Host "  otadata  -> 0xF000  ($OtaData)" -ForegroundColor DarkGray
Write-Host "  app      -> 0x20000 ($AppBin)" -ForegroundColor DarkGray
if ($Port) { Write-Host "  port     -> $Port" -ForegroundColor DarkGray }
Write-Host ""

& python -m esptool @flashArgs

if ($LASTEXITCODE -ne 0) {
    Write-Error "esptool failed (exit $LASTEXITCODE)"
}

Write-Host ""
Write-Host "dev_flash: done - device should boot freshly flashed firmware" -ForegroundColor Green
