param(
    [switch]$Clean = $false,
    [switch]$Rebuild = $false,
    [switch]$VerboseBuild = $false
)

$ErrorActionPreference = "Stop"

$PROJECT = "STM32F407_MotorControl"
$BUILD_DIR = "build"
$OBJ_DIR = Join-Path $BUILD_DIR "obj"
$FIRMWARE_DIR = Join-Path $BUILD_DIR "firmware"
$MAP_FILE = Join-Path $BUILD_DIR "$PROJECT.map"

$ASM_SOURCES = @(
    "BOOT/stm.s"
)

$C_SOURCES = @(
    "application/minimal_led_blink.c"
)

$CPU_FLAGS = @(
    "-mcpu=cortex-m4"
    "-mthumb"
    "-mfpu=fpv4-sp-d16"
    "-mfloat-abi=hard"
)

$CFLAGS = @(
    "-O2"
    "-ffunction-sections"
    "-fdata-sections"
    "-g3"
    "-Wall"
    "-Wextra"
)

$INCLUDES = @(
    "-I."
    "-Idrivers"
    "-Idrivers/system_init"
    "-Idrivers/BSP"
    "-Idrivers/api"
    "-Ihardware_vendor_code/CMSIS/Include"
    "-Ihardware_vendor_code/CMSIS/Device/STM32F4xx"
)

$DEFINES = @(
    "-DUSE_STDPERIPH_DRIVER"
    "-D__FPU_PRESENT=1"
)

$LINKER_FLAGS = @(
    "-TBOOT/stm.ld"
    "-Wl,--gc-sections,--print-memory-usage,-Map=$MAP_FILE"
)

function Write-Step([string]$Text)    { Write-Host $Text -ForegroundColor Yellow }
function Write-Info([string]$Text)    { Write-Host $Text -ForegroundColor Cyan }
function Write-Ok([string]$Text)      { Write-Host $Text -ForegroundColor Green }
function Write-Fail([string]$Text)    { Write-Host $Text -ForegroundColor Red }

function Require-Tool([string]$ToolName) {
    if (-not (Get-Command $ToolName -ErrorAction SilentlyContinue)) {
        throw "Missing required tool: $ToolName"
    }
}

function Ensure-Directory([string]$Path) {
    New-Item -ItemType Directory -Path $Path -Force | Out-Null
}

function Get-ObjectPath([string]$SourcePath) {
    return Join-Path $OBJ_DIR ([System.IO.Path]::ChangeExtension($SourcePath, ".o"))
}

function Invoke-CommandChecked([string]$Exe, [string[]]$CommandArgs) {
    if ($VerboseBuild) {
        Write-Host "$Exe $($CommandArgs -join ' ')" -ForegroundColor DarkGray
    }

    & $Exe @CommandArgs
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed ($LASTEXITCODE): $Exe $($CommandArgs -join ' ')"
    }
}

try {
    Write-Info "STM32 build automation (PowerShell)"
    Write-Info "Project: $PROJECT"
    Write-Host ""

    Require-Tool "arm-none-eabi-gcc"
    Require-Tool "arm-none-eabi-objcopy"
    Require-Tool "arm-none-eabi-size"

    if ($Clean -or $Rebuild) {
        Write-Step "[CLEAN] Removing build directory..."
        if (Test-Path $BUILD_DIR) {
            Remove-Item -Recurse -Force $BUILD_DIR
        }
        Write-Ok "[OK] Clean complete"
        if ($Clean -and -not $Rebuild) {
            exit 0
        }
    }

    Write-Step "[SETUP] Preparing build directories..."
    Ensure-Directory $OBJ_DIR
    Ensure-Directory $FIRMWARE_DIR

    $objects = New-Object System.Collections.Generic.List[string]

    Write-Step "[AS] Compiling startup assembly..."
    foreach ($src in $ASM_SOURCES) {
        if (-not (Test-Path $src)) {
            throw "Missing source file: $src"
        }
        $obj = Get-ObjectPath $src
        Ensure-Directory (Split-Path -Parent $obj)
        Invoke-CommandChecked "arm-none-eabi-gcc" ($CPU_FLAGS + $DEFINES + @("-c", $src, "-o", $obj))
        $objects.Add($obj)
        Write-Ok "  [OK] $src"
    }

    Write-Step "[CC] Compiling C sources..."
    foreach ($src in $C_SOURCES) {
        if (-not (Test-Path $src)) {
            throw "Missing source file: $src"
        }
        $obj = Get-ObjectPath $src
        Ensure-Directory (Split-Path -Parent $obj)
        Invoke-CommandChecked "arm-none-eabi-gcc" ($CPU_FLAGS + $CFLAGS + $INCLUDES + $DEFINES + @("-c", $src, "-o", $obj))
        $objects.Add($obj)
        Write-Ok "  [OK] $src"
    }

    $elf = Join-Path $FIRMWARE_DIR "$PROJECT.elf"
    $hex = Join-Path $FIRMWARE_DIR "$PROJECT.hex"
    $bin = Join-Path $FIRMWARE_DIR "$PROJECT.bin"

    Write-Step "[LD] Linking firmware..."
    Invoke-CommandChecked "arm-none-eabi-gcc" ($CPU_FLAGS + $LINKER_FLAGS + $objects + @("-lc", "-lm", "-o", $elf))
    Write-Ok "[OK] Linked: $elf"

    Write-Step "[OBJCOPY] Generating HEX/BIN..."
    Invoke-CommandChecked "arm-none-eabi-objcopy" @("-O", "ihex", $elf, $hex)
    Invoke-CommandChecked "arm-none-eabi-objcopy" @("-O", "binary", $elf, $bin)
    Write-Ok "[OK] HEX: $hex"
    Write-Ok "[OK] BIN: $bin"

    Write-Step "[SIZE] Firmware memory usage:"
    Invoke-CommandChecked "arm-none-eabi-size" @("--format=berkeley", $elf)

    Write-Host ""
    Write-Ok "Build completed successfully."
}
catch {
    Write-Host ""
    Write-Fail "[ERROR] $($_.Exception.Message)"
    exit 1
}
