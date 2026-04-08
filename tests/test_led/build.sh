#!/usr/bin/env bash
#
# STM32F103C8T6 LED Test - Build Script
# Usage: bash build.sh [clean|rebuild]
#

set -euo pipefail

# Get project root (go up 2 levels from tests/test_led/)
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$PROJECT_ROOT"

# Check if we're in right place
if [[ ! -f "tools/stm32f1.cfg" ]] || [[ ! -d "boot/f1" ]]; then
    echo "❌ ERROR: Cannot find project files"
    exit 1
fi

# ============================================================================
# Configuration
# ============================================================================

PROJECT="F103_LED_Test"
TEST_DIR="tests/test_led"
BUILD_DIR="build/test_led"
OBJ_DIR="$BUILD_DIR/obj"
FW_DIR="$BUILD_DIR/firmware"

# Compiler settings
CC="arm-none-eabi-gcc"
AS="arm-none-eabi-as"
OBJCOPY="arm-none-eabi-objcopy"
SIZE="arm-none-eabi-size"

# ============================================================================
# Flags
# ============================================================================

CPU_FLAGS=(-mcpu=cortex-m3 -mthumb)

CFLAGS=(
    -O2
    -ffunction-sections
    -fdata-sections
    -g3
    -Wall
    -Wextra
)

LDFLAGS=(
    "-Tboot/f1/stm.ld"
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    "-Wl,-Map=$BUILD_DIR/$PROJECT.map"
    --specs=nosys.specs
)

# ============================================================================
# Source files
# ============================================================================

ASM_SOURCES=(
    "boot/f1/stm.s"
)

C_SOURCES=(
    "tests/test_led/main.c"
    "src/f1/uart2_driver.c"
)

# ============================================================================
# Helper functions
# ============================================================================

clean() {
    if [[ -d "$BUILD_DIR" ]]; then
        rm -rf "$BUILD_DIR"
        echo "✅ Clean complete"
    fi
}

build() {
    mkdir -p "$OBJ_DIR" "$FW_DIR"

    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "Building: $PROJECT"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

    # Compile assembly
    local asm_objs=()
    for src in "${ASM_SOURCES[@]}"; do
        local obj="$OBJ_DIR/$(basename "$src" .s).o"
        echo "[ASM] $src"
        $AS -mcpu=cortex-m3 -mthumb "$src" -o "$obj"
        asm_objs+=("$obj")
    done

    # Compile C sources
    local c_objs=()
    for src in "${C_SOURCES[@]}"; do
        local obj="$OBJ_DIR/$(basename "$src" .c).o"
        echo "[GCC] $src"
        $CC "${CPU_FLAGS[@]}" "${CFLAGS[@]}" -c "$src" -o "$obj"
        c_objs+=("$obj")
    done

    # Link
    echo "[LD]  Linking..."
    local elf_file="$FW_DIR/$PROJECT.elf"
    $CC "${CPU_FLAGS[@]}" "${asm_objs[@]}" "${c_objs[@]}" "${LDFLAGS[@]}" -o "$elf_file"

    # Generate hex
    echo "[HEX] Creating hex file..."
    $OBJCOPY -O ihex "$elf_file" "$FW_DIR/$PROJECT.hex"

    # Generate bin
    echo "[BIN] Creating bin file..."
    $OBJCOPY -O binary "$elf_file" "$FW_DIR/$PROJECT.bin"

    # Show size
    echo ""
    $SIZE -B "$elf_file"

    echo ""
    echo "✅ Build complete!"
    echo "   ELF:  $elf_file"
    echo "   HEX:  $FW_DIR/$PROJECT.hex"
    echo "   BIN:  $FW_DIR/$PROJECT.bin"
}

# ============================================================================
# Main
# ============================================================================

case "${1:-}" in
    clean)
        clean
        ;;
    rebuild)
        clean
        build
        ;;
    *)
        build
        ;;
esac
