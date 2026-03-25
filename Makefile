# ============================================================================
# STM32F407VET6 + L298N Motor Control - Makefile
# Compiler: ARM GCC (arm-none-eabi-gcc) - from system PATH
# Author: STM32 Motor Control Team
# Date: 2026-03-24
# 
# Usage:
#   make              - Build firmware (.elf, .hex, .bin)
#   make clean        - Remove build artifacts
#   make flash        - Flash to STM32 via ST-Link
#   make size         - Show firmware memory usage
#   make help         - Show this menu
# ============================================================================

# ============================================================================
# PROJECT CONFIGURATION
# ============================================================================

PROJECT = STM32F407_MotorControl
BUILD_DIR = build
FIRMWARE_DIR = $(BUILD_DIR)/firmware

# ============================================================================
# COMPILER TOOLS (uses system PATH)
# ============================================================================

# ARM GCC toolchain - automatically found from system PATH
CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE    = arm-none-eabi-size
GDB     = arm-none-eabi-gdb

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
# SOURCE FILES - ALL PROJECT FILES
# ============================================================================

# C source files - application (LED TEST VERSION)
C_SOURCES = application/minimal_led_blink.c

# C source files - system initialization
C_SOURCES += \
	drivers/system_init/system_stm32f4xx.c \
	drivers/system_init/misc_drivers.c

# C source files - Board Support Package
C_SOURCES += drivers/BSP/bsp_motor.c

# C source files - Driver APIs (high-level)
C_SOURCES += \
	drivers/api/motor_driver.c \
	drivers/api/uart_driver.c \
	drivers/api/adc_driver.c \
	drivers/api/encoder_driver.c \
	drivers/api/telemetry_driver.c \
	drivers/api/sysmon_driver.c

# Assembly startup file (if available)
# ASM_SOURCES = BOOT/stm.s

# Object files
OBJECTS = $(addprefix $(BUILD_DIR)/,$(C_SOURCES:.c=.o))

# ============================================================================
# BUILD TARGETS
# ============================================================================

# Help menu
.PHONY: help
help:
	@echo =====================================================================
	@echo STM32F407 Motor Control Build System
	@echo =====================================================================
	@echo.
	@echo Available commands:
	@echo   make              - Build all (firmware.elf, .hex, .bin)
	@echo   make clean        - Delete build directory
	@echo   make size         - Show firmware memory usage
	@echo   make flash        - Flash to STM32F407 via ST-Link/OpenOCD
	@echo   make reset        - Reset STM32 device
	@echo   make help         - Show this menu
	@echo.
	@echo Configuration:
	@echo   Compiler:  $(CC)
	@echo   Toolchain: ARM GCC (cortex-m4, hard FPU)
	@echo   Optimize:  -O2 (balanced)
	@echo   Debug:     -g3 (full symbols)
	@echo.
	@echo Build output location:
	@echo   $(FIRMWARE_DIR)/
	@echo =====================================================================
	@echo.

# Default target
.PHONY: all
all: $(FIRMWARE_DIR)/$(PROJECT).elf $(FIRMWARE_DIR)/$(PROJECT).bin $(FIRMWARE_DIR)/$(PROJECT).hex size

# Create build directory - Windows compatible
$(BUILD_DIR):
	@if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"
	@if not exist "$(FIRMWARE_DIR)" mkdir "$(FIRMWARE_DIR)"
	@if not exist "$(BUILD_DIR)\application" mkdir "$(BUILD_DIR)\application"
	@if not exist "$(BUILD_DIR)\drivers" mkdir "$(BUILD_DIR)\drivers"
	@if not exist "$(BUILD_DIR)\drivers\system_init" mkdir "$(BUILD_DIR)\drivers\system_init"
	@if not exist "$(BUILD_DIR)\drivers\BSP" mkdir "$(BUILD_DIR)\drivers\BSP"
	@if not exist "$(BUILD_DIR)\drivers\api" mkdir "$(BUILD_DIR)\drivers\api"

# Compile C files to object files
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@echo [CC] Compiling: $<
	@$(CC) $(CFLAGS) $(INCLUDE_PATHS) $(DEFINES) -o $@ $<

# Assemble ASM files (if needed)
$(BUILD_DIR)/%.o: %.s | $(BUILD_DIR)
	@echo [AS] Assembling: $<
	@$(CC) $(CPU_FLAGS) $(DEBUG_FLAGS) -c -o $@ $<

# Link all object files into ELF
$(FIRMWARE_DIR)/$(PROJECT).elf: $(OBJECTS)
	@echo [LD] Linking: $@
	@$(CC) $(LDFLAGS) $(OBJECTS) -lc -lm -o $@
	@echo [SUCCESS] Firmware: $@

# Convert ELF to BIN (raw binary format)
$(FIRMWARE_DIR)/$(PROJECT).bin: $(FIRMWARE_DIR)/$(PROJECT).elf
	@echo [OBJCOPY] Converting to BIN: $@
	@$(OBJCOPY) -O binary $< $@

# Convert ELF to HEX (Intel HEX format)
$(FIRMWARE_DIR)/$(PROJECT).hex: $(FIRMWARE_DIR)/$(PROJECT).elf
	@echo [OBJCOPY] Converting to HEX: $@
	@$(OBJCOPY) -O ihex $< $@

# Show firmware size and memory usage
.PHONY: size
size: $(FIRMWARE_DIR)/$(PROJECT).elf
	@echo.
	@echo [SIZE] Firmware memory usage:
	@$(SIZE) --format=berkeley $(FIRMWARE_DIR)/$(PROJECT).elf
	@echo.
	@echo File sizes:
	@dir $(FIRMWARE_DIR)\*.elf $(FIRMWARE_DIR)\*.hex $(FIRMWARE_DIR)\*.bin 2>nul || echo Build files created

# Clean all build artifacts
.PHONY: clean
clean:
	@echo [CLEAN] Removing build directory...
	@if exist $(BUILD_DIR) rmdir /s /q $(BUILD_DIR) >nul 2>&1
	@echo [SUCCESS] Build directory cleaned

# ============================================================================
# FLASHING TARGETS (requires OpenOCD + ST-Link)
# ============================================================================

# OpenOCD configuration
OPENOCD = openocd
OPENOCD_INTERFACE = interface/stlink.cfg
OPENOCD_TARGET = target/stm32f4x.cfg

.PHONY: flash
flash: $(FIRMWARE_DIR)/$(PROJECT).hex
	@echo [FLASH] Programming STM32F407 via ST-Link...
	@echo.
	@$(OPENOCD) -f $(OPENOCD_INTERFACE) -f $(OPENOCD_TARGET) \
		-c "init" \
		-c "targets 0" \
		-c "halt" \
		-c "flash erase_all" \
		-c "program $(FIRMWARE_DIR)/$(PROJECT).hex verify reset" \
		-c "shutdown"
	@echo [SUCCESS] Flashing complete!
	@echo.

.PHONY: reset
reset:
	@echo [RESET] Resetting STM32F407...
	@$(OPENOCD) -f $(OPENOCD_INTERFACE) -f $(OPENOCD_TARGET) \
		-c "init" \
		-c "targets 0" \
		-c "reset" \
		-c "shutdown"
	@echo [SUCCESS] Reset complete
	@echo [SUCCESS] Reset complete

# ============================================================================
# REQUIREMENTS & SETUP NOTES
# ============================================================================

# BEFORE USING THIS MAKEFILE, INSTALL:
#
# 1. ARM GNU Toolchain
#    Windows: Download from https://developer.arm.com/downloads/-/gnu-rm
#             Extract and add bin/ folder to system PATH
#    Linux:   sudo apt install gcc-arm-none-eabi
#    macOS:   brew install arm-none-eabi-gcc
#
#    Verify: arm-none-eabi-gcc --version
#
# 2. OpenOCD (for flashing via ST-Link)
#    Windows: Download from https://gnutoolchains.com/arm-eabi/openocd
#    Linux:   sudo apt install openocd
#    macOS:   brew install open-ocd
#
#    Verify: openocd --version
#
# 3. (Optional) FreeRTOS kernel
#    Download from https://freertos.org/
#    Extract to: drivers/FreeRTOS/
#    Then uncomment FreeRTOS sections in this Makefile
#
# 4. Linker Script
#    Must create or provide: BOOT/stm.ld
#    See: docs/memory_map.md for STM32F407 memory layout
#
# 5. Startup Assembly (optional)
#    May need: BOOT/stm.s (ARM Thumb startup code)

# ============================================================================
# TROUBLESHOOTING
# ============================================================================

# Error: "arm-none-eabi-gcc not found"
#  → Install ARM toolchain and add to system PATH
#  → Check: where arm-none-eabi-gcc (Windows) or which arm-none-eabi-gcc (Linux)
#
# Error: "No rule to make target"
#  → Clean and rebuild: make clean && make all
#  → Check file paths are correct
#
# Error: "undefined reference to..."
#  → Check all source files are listed in C_SOURCES
#  → Verify object files compiled correctly
#
# Error: "OpenOCD not found" when running "make flash"
#  → Install OpenOCD 
#  → Check interface: stlink-v2.cfg vs stlink.cfg (edit OPENOCD_INTERFACE)

# ============================================================================
#                           END OF MAKEFILE
# ============================================================================
