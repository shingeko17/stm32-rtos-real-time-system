#!/usr/bin/env bash
#
# Build script for STM32F1 Watchdog Redundancy firmware
# Updated for modular directory structure:
# boot/f1/, src/f1/, drivers/, middleware/, etc.
#

set -euo pipefail

PROJECT="STM32F103_WatchdogRedundancy"
BUILD_DIR="build/f1"
FW_DIR="$BUILD_DIR/firmware"
OBJ_DIR="$BUILD_DIR/obj"

CC="arm-none-eabi-gcc"
OBJCOPY="arm-none-eabi-objcopy"
SIZE="arm-none-eabi-size"

CPU_FLAGS=(
  -mcpu=cortex-m3
  -mthumb
)

CFLAGS=(
  -O2
  -ffunction-sections
  -fdata-sections
  -g3
  -Wall
  -Wextra
)

INCLUDES=(
  -I.
  -Isrc
  -Idrivers
  -Idrivers/system_init
  -Idrivers/BSP
  -Idrivers/api
  -Ihardware_vendor_code/CMSIS/Include
  -Ihardware_vendor_code/CMSIS/Device/STM32F1xx
)

DEFINES=(
  -DUSE_STDPERIPH_DRIVER
  -DSTM32F10X_MD
)

LDFLAGS=(
  "-Tboot/f1/stm.ld"
  -Wl,--gc-sections
  -Wl,--print-memory-usage
  "-Wl,-Map=$BUILD_DIR/$PROJECT.map"
)

ASM_SOURCES=(
  boot/f1/stm.s
)

C_SOURCES=(
  src/f1/watchdog_main.c
)

if [[ "${1:-}" == "clean" ]]; then
  rm -rf "$BUILD_DIR"
  echo "[OK] Clean complete"
  exit 0
fi

if [[ "${1:-}" == "rebuild" ]]; then
  rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR" "$FW_DIR" "$OBJ_DIR"

echo "=========================================="
echo "Building STM32F1 Watchdog Redundancy"
echo "=========================================="

# Compile ASM sources (using as directly for better compatibility)
ASM_OBJECTS=()
AS="arm-none-eabi-as"
for src in "${ASM_SOURCES[@]}"; do
    obj="$OBJ_DIR/$(basename "${src%.*}").o"
    mkdir -p "$(dirname "$obj")"
    echo "[ASM] $src → $obj"
    $AS -mcpu=cortex-m3 -mthumb "$src" -o "$obj"
    ASM_OBJECTS+=("$obj")
done

# Compile C sources
C_OBJECTS=()
for src in "${C_SOURCES[@]}"; do
    obj="$OBJ_DIR/$(basename "${src%.c}").o"
    mkdir -p "$(dirname "$obj")"
    echo "[C] $src → $obj"
    $CC "${CPU_FLAGS[@]}" "${CFLAGS[@]}" "${INCLUDES[@]}" "${DEFINES[@]}" -c "$src" -o "$obj"
    C_OBJECTS+=("$obj")
done

# Link
ELF_FILE="$FW_DIR/$PROJECT.elf"
echo ""
echo "[LINK] Linking..."
$CC "${CPU_FLAGS[@]}" "${LDFLAGS[@]}" "${ASM_OBJECTS[@]}" "${C_OBJECTS[@]}" -o "$ELF_FILE"

# Generate HEX and BIN
HEX_FILE="$FW_DIR/$PROJECT.hex"
BIN_FILE="$FW_DIR/$PROJECT.bin"

echo "[OBJCOPY] Generating HEX..."
$OBJCOPY -O ihex "$ELF_FILE" "$HEX_FILE"

echo "[OBJCOPY] Generating BIN..."
$OBJCOPY -O binary "$ELF_FILE" "$BIN_FILE"

# Print size
echo ""
echo "=========================================="
echo "Build Complete!"
echo "=========================================="
$SIZE "$ELF_FILE"
echo ""
echo "Output files:"
echo "  ELF: $ELF_FILE"
echo "  HEX: $HEX_FILE"
echo "  BIN: $BIN_FILE"
echo ""
