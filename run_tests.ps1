#Requires -Version 5.1
<#
.SYNOPSIS
    Run PlatformIO unit tests for GUI-Generic project.

.PARAMETER Environment
    PlatformIO environment to test against. Default: esp32_test

.PARAMETER Port
    Serial port of the connected device. Default: COM3

.PARAMETER Verbose
    Enable verbose PlatformIO output (-vvv).

.PARAMETER Filter
    Run only tests matching this pattern (e.g. "test_sharky*").

.EXAMPLE
    .\run_tests.ps1
    .\run_tests.ps1 -Port COM5
    .\run_tests.ps1 -Environment esp32_test -Verbose
    .\run_tests.ps1 -Filter test_sharky774
#>
param(
    [string]$Environment = "esp32_test",
    [string]$Port        = "COM3",
    [switch]$Verbose,
    [string]$Filter      = ""
)

$pio = "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe"

if (-not (Test-Path $pio)) {
    Write-Error "PlatformIO not found at: $pio"
    exit 1
}

$args = @(
    "test"
    "--environment", $Environment
    "--upload-port",  $Port
    "--test-port",    $Port
)

if ($Verbose)       { $args += "-vvv" }
if ($Filter -ne "") { $args += "--filter", $Filter }

Write-Host ""
Write-Host "Running tests..." -ForegroundColor Cyan
Write-Host "  Environment : $Environment" -ForegroundColor Gray
Write-Host "  Port        : $Port" -ForegroundColor Gray
if ($Filter) {
    Write-Host "  Filter      : $Filter" -ForegroundColor Gray
}
Write-Host ""

& $pio @args
$exitCode = $LASTEXITCODE

Write-Host ""
if ($exitCode -eq 0) {
    Write-Host "All tests PASSED." -ForegroundColor Green
} else {
    Write-Host "Tests FAILED (exit code $exitCode)." -ForegroundColor Red
}

exit $exitCode
