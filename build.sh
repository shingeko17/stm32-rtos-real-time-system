#!/usr/bin/env bash
set -euo pipefail

PROJECT="STM32F407_MotorControl"
BUILD_DIR="build"
OBJ_DIR="$BUILD_DIR/obj"
FW_DIR="$BUILD_DIR/firmware"
MAP_FILE="$BUILD_DIR/$PROJECT.map"

CC="arm-none-eabi-gcc"
OBJCOPY="arm-none-eabi-objcopy"
SIZE="arm-none-eabi-size"

CPU_FLAGS=(
  -mcpu=cortex-m4
  -mthumb
  -mfpu=fpv4-sp-d16
  -mfloat-abi=hard
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
  -Idrivers
  -Idrivers/system_init
  -Idrivers/BSP
  -Idrivers/api
  -Ihardware_vendor_code/CMSIS/Include
  -Ihardware_vendor_code/CMSIS/Device/STM32F4xx
)

DEFINES=(
  -DUSE_STDPERIPH_DRIVER
  -D__FPU_PRESENT=1
)

ASM_SOURCES=(
  BOOT/stm.s
)

C_SOURCES=(
  application/motor_sensor_main.c
  drivers/system_init/system_stm32f4xx.c
  drivers/system_init/misc_drivers.c
  drivers/BSP/bsp_motor.c
  drivers/api/motor_driver.c
  drivers/api/uart_driver.c
  drivers/api/adc_driver.c
  drivers/api/encoder_driver.c
)

if [[ "${1:-}" == "clean" ]]; then
  rm -rf "$BUILD_DIR"
  echo "[OK] Clean complete"
  exit 0
fi

if [[ "${1:-}" == "rebuild" ]]; then
  rm -rf "$BUILD_DIR"
fi

command -v "$CC" >/dev/null || { echo "[ERROR] $CC not found"; exit 1; }
command -v "$OBJCOPY" >/dev/null || { echo "[ERROR] $OBJCOPY not found"; exit 1; }
command -v "$SIZE" >/dev/null || { echo "[ERROR] $SIZE not found"; exit 1; }

mkdir -p "$OBJ_DIR" "$FW_DIR"

objects=()

echo "[AS] Compiling startup assembly..."
for src in "${ASM_SOURCES[@]}"; do
  obj="$OBJ_DIR/${src%.s}.o"
  mkdir -p "$(dirname "$obj")"
  "$CC" "${CPU_FLAGS[@]}" "${DEFINES[@]}" -c "$src" -o "$obj"
  objects+=("$obj")
  echo "  [OK] $src"
done

echo "[CC] Compiling C sources..."
for src in "${C_SOURCES[@]}"; do
  obj="$OBJ_DIR/${src%.c}.o"
  mkdir -p "$(dirname "$obj")"
  "$CC" "${CPU_FLAGS[@]}" "${CFLAGS[@]}" "${INCLUDES[@]}" "${DEFINES[@]}" -c "$src" -o "$obj"
  objects+=("$obj")
  echo "  [OK] $src"
done

ELF="$FW_DIR/$PROJECT.elf"
HEX="$FW_DIR/$PROJECT.hex"
BIN="$FW_DIR/$PROJECT.bin"

echo "[LD] Linking firmware..."
"$CC" "${CPU_FLAGS[@]}" -TBOOT/stm.ld "-Wl,--gc-sections,--print-memory-usage,-Map=$MAP_FILE" "${objects[@]}" -lc -lm -o "$ELF"
echo "[OK] Linked: $ELF"

echo "[OBJCOPY] Generating HEX/BIN..."
"$OBJCOPY" -O ihex "$ELF" "$HEX"
"$OBJCOPY" -O binary "$ELF" "$BIN"
echo "[OK] HEX: $HEX"
echo "[OK] BIN: $BIN"

echo "[SIZE] Firmware memory usage:"
"$SIZE" --format=berkeley "$ELF"

echo "Build completed successfully."
