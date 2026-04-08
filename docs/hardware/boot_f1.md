# STM32F103C8T6 LED Test - Troubleshooting & Lessons Learned

**Date:** 2026-03-29
**Board:** STM32F103C8T6 (Cortex-M3, 8MHz, 64KB FLASH, 20KB RAM)
**Debugger:** ST-Link v2
**GPIO:** PB0-PB7 (8 LEDs)

---

## Issues Encountered & Solutions

### 1. ❌ Boot Files Missing (CRITICAL)
**Error Message:**
```
Error: can't open /d:/personal_project/stm32-rtos-system/boot/f1/stm.s for reading: No such file or directory
```

**Root Cause:**
- Deleted `boot/f1/` folder during project cleanup without backup
- Build script couldn't find startup assembly and linker script

**Solution:**
- Recreated `boot/f1/stm.s` (Cortex-M3 startup code with vector table)
- Recreated `boot/f1/stm.ld` (linker script: 64KB FLASH, 20KB RAM)

**Severity:** ⭐⭐⭐ CRITICAL
**Frequency:** ⭐⭐ Common in cleanup operations

**Prevention:**
```bash
# Always backup critical boot files BEFORE cleanup
cp -r boot/ boot_backup/
```

---

### 2. ❌ GPIO Port Mapping Wrong (CRITICAL)
**Error:** LEDs don't light up

**Root Cause (Iteration 1):**
- Assumed LEDs on PA0-PA7 without checking schematic
- Calculation of CRL/CRH register configuration was wrong

**Root Cause (Iteration 2):**
- Read schematic incorrectly: thought PB3-PB7 only (5 LEDs)
- Added hardcoded offset `(led_num + 3)` which broke PB0-PB2

**Correct Mapping (From Schematic):**
```
PB0 → LED0
PB1 → LED1
PB2 → LED2
PB3 → LED3
PB4 → LED4
PB5 → LED5
PB6 → LED6
PB7 → LED7
```

**Solution:**
```c
// ❌ WRONG - hardcoded offset
GPIOB_ODR |= (1 << (led_num + 3));

// ✅ CORRECT - direct mapping
GPIOB_ODR |= (1 << led_num);
```

**Severity:** ⭐⭐⭐ CRITICAL
**Frequency:** ⭐⭐⭐ Very common - must always verify with schematic first

**Prevention:**
1. Always study schematic before coding
2. Create GPIO mapping table from schematic
3. Double-check pin names in code

---

### 3. ❌ GPIO Configuration Register Bug (HIGH)
**Error:** PA4-PA7 not responding even though code compiled

**Root Cause:**
STM32F1 splits GPIO configuration into TWO registers:
- **CRL** (Control Register Low): PA0-PA7 / PB0-PB7
- **CRH** (Control Register High): PA8-PA15 / PB8-PB15

Code only configured CRL, missing CRH entirely.

**Wrong Code:**
```c
GPIOA_CRL &= 0x00000000;
GPIOA_CRL |= 0x11111111;  // Only PA0-PA3, NOT PA4-PA7!
```

**Correct Code:**
```c
// Configure PA0-PA3 via CRL
GPIOA_CRL &= 0x00000000;
GPIOA_CRL |= 0x11111111;

// Configure PA4-PA7 via CRH
GPIOA_CRH &= 0xFFFF0000;  // Clear PA4-PA7 bits
GPIOA_CRH |= 0x00001111;  // Set PA4-PA7 to output mode
```

**For PB0-PB7 (all in CRL):**
```c
GPIOB_CRL &= 0x00000000;
GPIOB_CRL |= 0x11111111;  // All 8 pins in CRL
```

**Severity:** ⭐⭐⭐ CRITICAL
**Frequency:** ⭐⭐⭐ Very common - easy to forget register split

**Prevention:**
- Read STM32F1 datasheet section on GPIO
- Know which pins are in CRL vs CRH before coding
- Make a reference table

**Reference: STM32F1 GPIO Registers**
```
Port A/B (8 pins per port):
- CRL (bits 0-31):  Control PA0-7 or PB0-7
- CRH (bits 0-31):  Control PA8-15 or PB8-15

Each pin uses 4 bits (CNF[1:0] + MODE[1:0]):
- MODE = 01 (output)
- CNF  = 00 (push-pull)
→ Value = 0x1
```

---

### 4. ❌ Delay Too Fast (MEDIUM)
**Error:** LEDs not visible - blinking way too fast

**Root Cause:**
```c
#define LED_DELAY_US 500  // 0.5 milliseconds!
```

At 0.5ms × 8 LEDs = 4ms for full cycle = 250 Hz frequency (invisible to human eye).

Human perceivable blink rate: ~10-100 Hz (minimum ~10-100ms per state).

**Solution:**
```c
// ❌ WRONG - too fast
#define LED_DELAY_US 500

// ✅ CORRECT - human perceivable
#define LED_DELAY_MS 500  // 500 milliseconds
```

**Delay Implementation:**
```c
static void delay_ms(uint32_t ms)
{
    while (ms--) {
        /* Delay ~1ms at 8MHz */
        uint32_t loops = 8000 / 3;
        while (loops--) {
            __asm volatile("nop");
        }
    }
}
```

**Severity:** ⭐⭐ MEDIUM (affects UX, not critical)
**Frequency:** ⭐⭐ Common in embedded timing

**Prevention:**
- Test timing visually first
- Remember human eye can't see below ~10Hz
- For precise timing, use SysTick timer

---

### 5. ❌ Debugger Interface Wrong (CRITICAL)
**Error Message:**
```
Warn : Failed to open device: LIBUSB_ERROR_NOT_SUPPORTED
Error: No J-Link device found
```

**Root Cause:**
Configuration file set to J-Link, but physically connected ST-Link v2.

```c
// stm32f1.cfg - WRONG setting
source [find interface/jlink.cfg]
```

**Solution:**
```c
// stm32f1.cfg - CORRECT
source [find interface/stlink.cfg]
```

**Severity:** ⭐⭐⭐ CRITICAL
**Frequency:** ⭐⭐⭐ Very common - many debugger types exist

**Debugger Types & OpenOCD Config:**
```
ST-Link v2          → interface/stlink.cfg
J-Link              → interface/jlink.cfg
FT2232H (FTDI)      → interface/ftdi/um232h.cfg
CMSIS-DAP generic   → interface/cmsis-dap.cfg
```

**Prevention:**
1. Know your debugger type (check label on device)
2. Match config file to debugger
3. Test with: `openocd -f interface/xxx.cfg -f target/stm32f1x.cfg -c "init"`

---

### 6. ❌ Debugger Connection Failed (CRITICAL)
**Error Message:**
```
Info : STLINK V2J37 (API v2) VID:PID 0483:3748
Info : Target voltage: 3.03231V
Error: init mode failed (unable to connect to the target)
```

**Root Cause:**
Multiple possible causes:
1. **Wiring:** SWDIO/SWCLK/GND pins sloppy or disconnected
2. **Power:** Board not powered or voltage too low
3. **Mode:** Board in wrong mode (not "Program: STLink")
4. **Chip State:** STM32 locked or in stop mode

**Solution Checklist:**
```
□ Check ST-Link LED (red = power, green = communication)
□ Verify cable connections:
  - SWDIO (PA13) → ST-Link SWDIO pin
  - SWCLK (PA14) → ST-Link SWCLK pin
  - GND → ST-Link GND
□ Verify board power supply: 3.3V on VCC
□ Set board to "Program: STLink" mode (if selectable)
□ Reset board (power cycle)
□ Measure voltage between pins with multimeter
```

**Severity:** ⭐⭐⭐ CRITICAL
**Frequency:** ⭐⭐⭐ Very common - hardware issue

**Prevention:**
1. Triple-check wiring before first flash
2. Use quality jumper cables
3. Verify board gets power (LED check)

---

### 7. ❌ Pin Offset Calculation (MEDIUM)
**Error:** When mapping PB3-PB7, tried `(led_num + 3)` offset

```c
// ❌ WRONG - arbitrary offset breaks PB0-PB2
for (i = 0; i < 5; i++) {
    GPIOB_ODR |= (1 << (i + 3));  // Sets PB3-PB7 only
}

// ✅ CORRECT - direct mapping
for (i = 0; i < 8; i++) {
    GPIOB_ODR |= (1 << i);  // Sets PB0-PB7
}
```

**Severity:** ⭐⭐ MEDIUM
**Frequency:** ⭐⭐ Common - off-by-one errors

**Prevention:**
- Create explicit pin maps:
```c
#define LED0_PIN  0   // PB0
#define LED1_PIN  1   // PB1
// ... etc
```

---

### 8. ❌ Delay Loop Inaccuracy (LOW)
**Error:** Timing not precise - depends on compiler optimization

```c
static void delay_us(uint32_t us)
{
    uint32_t loops = us * CLK_FREQ_MHZ / 3;
    while (loops--) {
        __asm volatile("nop");  // ← volatile prevents optimization away
    }
}
```

**Issue:**
- Compiler optimizations can remove/reorder instructions
- Pipeline effects not fully accounted
- Interrupt latency affects timing

**Solution:**
Use SysTick timer for accurate timing (not just loops).

**Severity:** ⭐ LOW (for LED blinking, OK)
**Frequency:** ⭐⭐ Common in embedded code

**For Accurate Timing (Future Enhancement):**
```c
// Use SysTick timer instead of loop-based delay
void delay_ms_systick(uint32_t ms)
{
    // Configure SysTick for 1ms intervals
    // Non-blocking or blocking wait
}
```

---

## Successful Configuration

### Final Correct Setup

**GPIO Configuration:**
```c
#define GPIOB_BASE  0x40010C00UL
#define GPIOB_CRL   (*(volatile uint32_t *)(GPIOB_BASE + 0x00))
#define GPIOB_ODR   (*(volatile uint32_t *)(GPIOB_BASE + 0x0C))

void GPIO_Init(void)
{
    RCC_APB2ENR |= (1 << 3);          // GPIOB clock
    GPIOB_CRL &= 0x00000000;          // Clear
    GPIOB_CRL |= 0x11111111;          // PB0-7 = output mode
    GPIOB_ODR = 0x00;                 // All off
}

void LED_On(uint8_t led_num)
{
    GPIOB_ODR |= (1 << led_num);      // Direct mapping
}
```

**LED Pattern (Running Light):**
```
ON sequence:  LED0 → LED1 → LED2 → ... → LED7 (500ms each)
OFF sequence: LED0 → LED1 → LED2 → ... → LED7 (500ms each)
Repeat forever
```

**OpenOCD Config:**
```ini
# stm32f1.cfg
source [find interface/stlink.cfg]    # ← Match your debugger
source [find target/stm32f1x.cfg]
adapter speed 1000
set WORKAREASIZE 0x5000
```

---

## Best Practices & Prevention

### ✅ Do's
1. **Always read schematic first** before any GPIO code
2. **Backup critical files** (boot, linker script) separately
3. **Create GPIO mapping tables** from schematic
4. **Test incrementally:** GPIO init → single LED → pattern
5. **Use debugger correctly:** Verify model matches config
6. **Check power:** Measure voltage before troubleshooting
7. **Document pin mappings** in code comments
8. **Use SysTick** for timing-critical applications

### ❌ Don'ts
1. **Don't assume GPIO ports** - always verify from schematic
2. **Don't hardcode offsets** - use direct pin mapping
3. **Don't forget register splits** (CRL/CRH for port A/B)
4. **Don't mix debugger types** - J-Link ≠ ST-Link
5. **Don't rely on loop delays** for accurate timing
6. **Don't ignore compiler warnings** - they catch bugs
7. **Don't skip hardware verification** - measure voltages
8. **Don't delete boot files** before backup

---

## Quick Reference

### STM32F1 GPIO Configuration
```c
// Enable clock
RCC_APB2ENR |= (1 << port_num);  // port_num: 2=A, 3=B, 4=C, etc

// Set pins to output (example: PB0-7)
GPIOB_CRL = 0x11111111;  // Mode=01(output), CNF=00(push-pull)

// Toggle LED
GPIOB_ODR |= (1 << pin);   // LED ON
GPIOB_ODR &= ~(1 << pin);  // LED OFF
```

### Register Bit Layout
```
CRL/CRH each pin uses 4 bits: [CNF1:CNF0][MODE1:MODE0]
- CNF = 00: Push-pull output
- MODE = 01: Output 10MHz
- → Value = 0x1 per pin

For 8 pins: 0x11111111 sets all to output mode
```

### OpenOCD Debug
```bash
# Test connection
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg -c "init"

# Flash
openocd -f stm32f1.cfg -c "init" -c "flash write_image erase firmware.hex" -c "shutdown"
```

---

## Summary Table

| Issue | Severity | Frequency | Prevention |
|-------|----------|-----------|-----------|
| Boot files missing | ⭐⭐⭐ | ⭐⭐ | Backup separately |
| GPIO port wrong | ⭐⭐⭐ | ⭐⭐⭐ | Read schematic first |
| CRL/CRH split | ⭐⭐⭐ | ⭐⭐⭐ | Study datasheet |
| Delay too fast | ⭐⭐ | ⭐⭐ | Test visually |
| Debugger wrong | ⭐⭐⭐ | ⭐⭐⭐ | Match hardware type |
| Connection failed | ⭐⭐⭐ | ⭐⭐⭐ | Verify wiring + power |
| Pin offset wrong | ⭐⭐ | ⭐⭐ | Use direct mapping |
| Loop timing inaccurate | ⭐ | ⭐⭐ | Use SysTick |

---

## Next Steps

1. **Optimize timing:** Implement SysTick-based delays for accuracy
2. **Add button input:** Read PA15 button to change LED patterns
3. **Integrate RTOS:** Move to FreeRTOS when ready
4. **Add documentation:** Update project README with GPIO map

**Status:** ✅ LED test working - Ready for production code integration
