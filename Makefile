# ============================================================================
# STM32F407VET6 + L298N Motor Control - Makefile
# Compiler: ARM GCC (arm-none-eabi-gcc)
# Author: STM32 Motor Control Team
# Date: 2026-03-14
# 
# Usage:
#   make              - Build project
#   make clean        - Clean build artifacts
#   make flash        - Flash to board (requires OpenOCD)
#   make size         - Show firmware size
# ============================================================================

# ============================================================================
# PROJECT CONFIGURATION
# ============================================================================

# Project name
PROJECT = STM32F407_MotorControl

# Build output directory
BUILD_DIR = build

# Source directories
SOURCE_DIRS = application drivers/system_init drivers/BSP drivers/api

# ============================================================================
# COMPILER TOOLS
# ============================================================================

# ARM GCC toolchain - Using full path to Downloads folder
TOOLCHAIN_PATH = c:\Users\Admin\Downloads\stm32f4\bin
CC      = $(TOOLCHAIN_PATH)\arm-none-eabi-gcc.exe
AS      = $(TOOLCHAIN_PATH)\arm-none-eabi-as.exe
LD      = $(TOOLCHAIN_PATH)\arm-none-eabi-ld.exe
OBJCOPY = $(TOOLCHAIN_PATH)\arm-none-eabi-objcopy.exe
OBJDUMP = $(TOOLCHAIN_PATH)\arm-none-eabi-objdump.exe
SIZE    = $(TOOLCHAIN_PATH)\arm-none-eabi-size.exe
GDB     = $(TOOLCHAIN_PATH)\arm-none-eabi-gdb.exe

# ============================================================================
# COMPILER FLAGS
# ============================================================================

# CPU flags
CPU_FLAGS = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

# Optimization flags
OPT_FLAGS = -O2 -ffunction-sections -fdata-sections

# Warning flags
WARN_FLAGS = -Wall -Wextra -Wshadow -Wundef -Wno-packed-bitfield-compat

# Debug flags
DEBUG_FLAGS = -g3 -gdwarf-2

# All compiler flags
CFLAGS = $(CPU_FLAGS) $(OPT_FLAGS) $(WARN_FLAGS) $(DEBUG_FLAGS) -c

# ============================================================================
# INCLUDE PATHS
# ============================================================================

INCLUDE_PATHS = \
	-I. \
	-Idrivers \
	-Idrivers/system_init \
	-Idrivers/BSP \
	-Idrivers/api \
	-Ihardware_vendor_code/CMSIS/Include \
	-Ihardware_vendor_code/CMSIS/Device/STM32F4xx \
	-Ihardware_vendor_code/STM32F4xx_StdPeriph_Driver/inc \
	-ISTM32CubeF4_source/Drivers/CMSIS/Core/Include \
	-ISTM32CubeF4_source/Drivers/CMSIS/Device/ST/STM32F4xx/Include

# ============================================================================
# DEFINES
# ============================================================================

# Microcontroller defines
DEFINES = \
	-DSTM32F407xx \
	-DUSE_STDPERIPH_DRIVER \
	-D__FPU_PRESENT=1 \
	-D__FPU_USED=1

# ============================================================================
# LINKER FLAGS
# ============================================================================

# Linker script
LINKER_SCRIPT = BOOT/stm.ld

# Linker flags
LDFLAGS = $(CPU_FLAGS) \
	-T$(LINKER_SCRIPT) \
	-Wl,--gc-sections,--print-memory-usage,--cref,-Map=$(BUILD_DIR)/$(PROJECT).map

# ============================================================================
# SOURCE FILES
# ============================================================================

# C source files
C_SOURCES = \
	application/main.c \
	drivers/system_init/system_stm32f4xx.c \
	drivers/system_init/misc_drivers.c \
	drivers/BSP/bsp_motor.c \
	drivers/api/motor_driver.c

# Assembly startup file
ASM_SOURCES = BOOT/stm.s

# Object files
OBJECTS = $(addprefix $(BUILD_DIR)/,$(C_SOURCES:.c=.o) $(ASM_SOURCES:.s=.o))

# ============================================================================
# BUILD TARGETS
# ============================================================================

# Default target
.PHONY: all
all: $(BUILD_DIR)/$(PROJECT).elf $(BUILD_DIR)/$(PROJECT).bin $(BUILD_DIR)/$(PROJECT).hex size

# Create build directory (Windows-compatible)
$(BUILD_DIR):
	@if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"
	@if not exist "$(BUILD_DIR)\application" mkdir "$(BUILD_DIR)\application"
	@if not exist "$(BUILD_DIR)\drivers" mkdir "$(BUILD_DIR)\drivers"
	@if not exist "$(BUILD_DIR)\drivers\system_init" mkdir "$(BUILD_DIR)\drivers\system_init"
	@if not exist "$(BUILD_DIR)\drivers\BSP" mkdir "$(BUILD_DIR)\drivers\BSP"
	@if not exist "$(BUILD_DIR)\drivers\api" mkdir "$(BUILD_DIR)\drivers\api"

# Compile C files
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@echo [CC] $<
	@$(CC) $(CFLAGS) $(INCLUDE_PATHS) $(DEFINES) -o $@ $<

# Assemble ASM files
$(BUILD_DIR)/%.o: %.s | $(BUILD_DIR)
	@echo [AS] $<
	@$(CC) $(CPU_FLAGS) $(DEBUG_FLAGS) -c -o $@ $<

# Link ELF file
$(BUILD_DIR)/$(PROJECT).elf: $(OBJECTS)
	@echo [LD] Linking...
	@$(CC) $(LDFLAGS) $(OBJECTS) -lc -lm -o $@
	@echo [OK] $@

# Generate BIN file
$(BUILD_DIR)/$(PROJECT).bin: $(BUILD_DIR)/$(PROJECT).elf
	@echo [OBJCOPY] Generating .bin...
	@$(OBJCOPY) -O binary $< $@
	@echo [OK] $@

# Generate HEX file
$(BUILD_DIR)/$(PROJECT).hex: $(BUILD_DIR)/$(PROJECT).elf
	@echo [OBJCOPY] Generating .hex...
	@$(OBJCOPY) -O ihex $< $@
	@echo [OK] $@

# Show firmware size
.PHONY: size
size: $(BUILD_DIR)/$(PROJECT).elf
	@echo.
	@echo [SIZE] Firmware size:
	@$(SIZE) $(BUILD_DIR)/$(PROJECT).elf
	@echo.

# Clean build artifacts
.PHONY: clean
clean:
	@echo [CLEAN] Removing build directory...
	@if exist $(BUILD_DIR) rmdir /s /q $(BUILD_DIR)
	@echo [OK] Cleaned

# ============================================================================
# FLASHING TARGETS (requires OpenOCD)
# ============================================================================

# OpenOCD configuration
OPENOCD = openocd
OPENOCD_INTERFACE = interface/stlink-v2.cfg
OPENOCD_TARGET = target/stm32f4x.cfg

# Flash firmware
.PHONY: flash
flash: $(BUILD_DIR)/$(PROJECT).bin
	@echo [FLASH] Programming STM32F407...
	@$(OPENOCD) -f $(OPENOCD_INTERFACE) -f $(OPENOCD_TARGET) \
		-c "init" \
		-c "reset halt" \
		-c "flash write_image erase $(BUILD_DIR)/$(PROJECT).bin 0x08000000" \
		-c "reset run" \
		-c "shutdown"
	@echo [OK] Flash complete

# ============================================================================
# UTILITY TARGETS
# ============================================================================

# Show help
.PHONY: help
help:
	@echo STM32F407VET6 + L298N Motor Control - Makefile Help
	@echo.
	@echo Usage: make [target]
	@echo.
	@echo Targets:
	@echo  all          - Build project (default)
	@echo  clean        - Clean build artifacts
	@echo  size         - Show firmware size
	@echo  flash        - Flash to board (requires OpenOCD)
	@echo  help         - Show this help
	@echo.

# Phony targets (not files)
.PHONY: all clean help size flash
