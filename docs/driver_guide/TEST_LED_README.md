# STM32F103C8T6 LED Running Light Test

Complete test project for LED running light effect on STM32F103C8T6 Cortex-M3 board.

## Hardware Setup

**Board:** STM32F103C8T6 (8MHz, 64KB FLASH, 20KB RAM)

**LED Connections (GPIO PA0-PA7):**
```
L1 → PA0 (LED 0)
L2 → PA1 (LED 1)
L3 → PA2 (LED 2)
L4 → PA3 (LED 3)
L5 → PA4 (LED 4)
L6 → PA5 (LED 5)
L7 → PA6 (LED 6)
L8 → PA7 (LED 7)
```

All LEDs active HIGH (3.3V logic), common GND.

## Project Structure

```
tests/test_led/
├── main.c           ← LED test application (register-level GPIO control)
├── build.sh         ← Build script
├── flash.sh         ← Flash script
└── README.md        ← This file
```

**External dependencies (referenced but not copied):**
- `boot/f1/stm.s` - Cortex-M3 startup code
- `boot/f1/stm.ld` - Linker script (64KB FLASH, 20KB RAM)
- `stm32f1.cfg` - OpenOCD configuration

## Build

From project root directory:

```bash
# Normal build (fast, only changed files)
bash tests/test_led/build.sh

# Clean build (remove old build, rebuild everything)
bash tests/test_led/build.sh rebuild

# Clean only
bash tests/test_led/build.sh clean
```

**Output files:**
- `build/test_led/firmware/F103_LED_Test.elf` - Executable
- `build/test_led/firmware/F103_LED_Test.hex` - Hex for flashing
- `build/test_led/firmware/F103_LED_Test.bin` - Binary

## Flash to Board

From project root directory:

```bash
bash tests/test_led/flash.sh
```

**Requirements:**
- OpenOCD installed (`openocd` available in PATH)
- ST-Link or similar debugger connected to PC
- Board connected via debugger

## LED Effect

**Running Light (Chase Pattern):**
1. **Phase 1 - Sáng dồn (Sequential ON):** LEDs turn ON sequentially L1→L8
2. **Phase 2 - Tắt dồn (Sequential OFF):** LEDs turn OFF sequentially L1→L8
3. Each transition: **0.5ms delay** between LEDs
4. Repeats continuously

**Visual:** Looks like a light chasing along the LED strip, then chasing back off.

## Firmware Details

**Clock:** 8MHz HSI (internal)
**Startup:** Minimal, register-level only (no HAL/LL libraries)

**Code Files:**
- `main.c` (~250 lines)
  - GPIO initialization (PA0-PA7 outputs)
  - Microsecond delayer
  - LED control functions
  - Running light effect function
  - Exception handler stubs

**Code Size:** ~1-2 KB (very small)

## Troubleshooting

### Build fails: "Cannot find project files"
- Make sure you're in the project root directory
- Check that `boot/f1/` and `stm32f1.cfg` exist

### Build fails: "arm-none-eabi-gcc: command not found"
- STM32 ARM toolchain not installed
- Install: `sudo apt-get install arm-none-eabi-gcc` (Linux)
- Or: `brew install arm-embedded-toolchain` (macOS)

### Flash fails: "OpenOCD not found"
- Install OpenOCD: `sudo apt-get install openocd` (Linux)
- Or: `brew install openocd` (macOS)

### Flash fails: "Debugger not found"
- ST-Link not connected or not recognized
- Check USB cable to debugger
- Linux: May need udev rules for ST-Link

### LEDs not lighting up
- Check physical connections (PA0-PA7 to LEDs)
- Verify LED polarity (anode → GPIO, cathode → GND via resistor)
- Try individual LED test in `main.c` (modify to call `LED_On(0)` only, etc.)

## Next Steps

- **Modify effect:** Edit `LED_Running_Light()` in `main.c` for different patterns
- **Add timing:** Implement SysTick timer for more accurate delays
- **Button input:** Add PA15 button to change effects on press
- **Integrate RTOS:** Move to main project middleware folder when ready

## Author
Generated: 2026-03-29

## License
Test code - Feel free to modify for learning/testing purposes
