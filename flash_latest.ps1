param(
    [switch]$NoReset = $false
)

$ErrorActionPreference = "Stop"

$Project = "STM32F407_MotorControl"
$FirmwareDir = "build\firmware"
$HexFile = Join-Path $FirmwareDir "$Project.hex"
$OpenOcdCfg = "stm32f4.cfg"

function Write-Step([string]$Text) { Write-Host $Text -ForegroundColor Yellow }
function Write-Ok([string]$Text) { Write-Host $Text -ForegroundColor Green }
function Write-Fail([string]$Text) { Write-Host $Text -ForegroundColor Red }

try {
    Write-Step "[CHECK] OpenOCD tool"
    if (-not (Get-Command openocd -ErrorAction SilentlyContinue)) {
        throw "openocd not found in PATH."
    }
    Write-Ok "[OK] openocd found"

    Write-Step "[CHECK] Firmware file"
    if (-not (Test-Path $HexFile)) {
        throw "Firmware not found: $HexFile. Run build.ps1 first."
    }
    Write-Ok "[OK] $HexFile"

    Write-Step "[CHECK] OpenOCD config"
    if (-not (Test-Path $OpenOcdCfg)) {
        throw "OpenOCD config not found: $OpenOcdCfg"
    }
    Write-Ok "[OK] $OpenOcdCfg"

    if (-not $NoReset) {
        Write-Step "[INFO] Connect board + ST-Link, then press ENTER to flash..."
        Read-Host | Out-Null
    }

    Write-Step "[FLASH] Programming firmware..."
    $cmd = "program `"$HexFile`" verify reset exit"
    & openocd -f $OpenOcdCfg -c $cmd
    if ($LASTEXITCODE -ne 0) {
        throw "OpenOCD returned exit code $LASTEXITCODE"
    }

    Write-Ok "[DONE] Flash + verify successful."
}
catch {
    Write-Fail "[ERROR] $($_.Exception.Message)"
    exit 1
}
