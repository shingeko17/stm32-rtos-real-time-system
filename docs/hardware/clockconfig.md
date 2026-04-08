# STM32F407VET6 Clock Configuration Guide

## Current Clock Configuration (168MHz)

```
┌─────────────────────────────────────────────────────────────────┐
│                    CLOCK TREE - 168MHz                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  HSE (8MHz External)                                            │
│  Crystal (from XTAL1/XTAL2)                                     │
│          │                                                      │
│          ├─→ HSI (16MHz Internal) [backup]                      │
│          │                                                      │
│          ├─→ PLL INPUT STAGE                                    │
│             ├─ PLLM = 8 divider                                 │
│             └─ 8MHz / 8 = 1MHz                                  │
│                   │                                             │
│          ┌────────┴────────┐                                    │
│          │   PLL VCO       │  (336MHz intermediate)             │
│          │  PLLN × 336     │  (1MHz × 336 = 336MHz)             │
│          └────────┬────────┘                                    │
│                   │                                             │
│          ┌────────┴────────┐                                    │
│          │  OUTPUT STAGE   │                                    │
│          │  PLLP = 2       │                                    │
│          │  336MHz / 2     │                                    │
│          └────────┬────────┘                                    │
│                   │                                             │
│          ┌────────▼────────┐  ← SYSCLK = 168MHz                 │
│          │   CPU/CORTEX    │    (Cortex-M4 max)                 │
│          │     AHB Bus     │                                    │
│          └────────┬────────┘                                    │
│                   │                                             │
│          ┌────────┴────────────┬──────────────┐                 │
│          │                     │              │                 │
│      (AHB /1)             (APB2 /2)      (APB1 /4)             │
│      168MHz               84MHz          42MHz                  │
│          │                   │              │                  │
│          ▼                   ▼              ▼                  │
│  ┌──────────────┐    ┌──────────────┐  ┌──────────────┐       │
│  │ GPIO, DMA    │    │ TIM1, SPI1   │  │ TIM2-7       │       │
│  │ Flash, USB   │    │ USART1, ADC  │  │ USART2-5     │       │
│  │ OTG, Memory  │    │              │  │ SPI2-3       │       │
│  └──────────────┘    └──────────────┘  └──────────────┘       │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## Frequency Calculation

**Formula:**
```
SYSCLK = (HSE / PLLM) × PLLN / PLLP

Where:
  HSE    = 8 MHz (external crystal)
  PLLM   = 8 (input prescaler)
  PLLN   = 336 (multiplier)
  PLLP   = 2 (output divider)
```

**Current:** `(8 / 8) × 336 / 2 = 168 MHz` ✅

---

## Clock Prescalers

| Bus | Current | Register | Formula | Max Freq | Use Case |
|-----|---------|----------|---------|----------|----------|
| **AHB** | 168MHz | CFGR[7:4] | SYSCLK / 1 | 168MHz | Memory, GPIO, DMA |
| **APB2** | 84MHz | CFGR[15:13] | AHB / 2 | 84MHz | TIM1, USART1, SPI1 |
| **APB1** | 42MHz | CFGR[12:10] | AHB / 4 | 42MHz | TIM2-7, USART2-5 |

---

## Common Clock Configurations

### Option 1: HSI 16MHz (Lowest Power - No PLL)
```c
/* Config file: drivers/system_init/system_stm32f4xx.c */
/* Set SYSCLK = 16 MHz (HSI only) */
PLLM = disable
SYSCLK = 16 MHz
Power consumption: ~2mA @ 3.3V
Best for: Low-power sleep, standby modes
```

### Option 2: Current - 168MHz (Full Performance)
```c
PLLM = 8
PLLN = 336
PLLP = 2
SYSCLK = 168 MHz (Cortex-M4 maximum)
Power consumption: ~30-40mA @ 3.3V
Best for: Maximum speed, real-time control
```

### Option 3: 120MHz (Balanced - Lower Power)
```c
PLLM = 8
PLLN = 240
PLLP = 2
SYSCLK = 120 MHz
Power consumption: ~25-30mA @ 3.3V
Best for: Motor control, sensor processing
```

### Option 4: 84MHz (Medium Power)
```c
PLLM = 8
PLLN = 168
PLLP = 2
SYSCLK = 84 MHz
Power consumption: ~20-25mA @ 3.3V
Best for: General applications
```

---

## How to Change Clock Frequency

### Step 1: Edit Configuration File
**File:** [drivers/system_init/system_stm32f4xx.c](../drivers/system_init/system_stm32f4xx.c)

### Step 2: Modify PLLN Value
Look for line ~180 in `system_stm32f4xx.c`:
```c
/* Current (168MHz) */
reg_value |= (uint32_t)0x0000A000;  /* PLLN = 336 */

/* Change to desired frequency:
   - 120MHz:  PLLN = 240 → 0x0000F000
   - 84MHz:   PLLN = 168 → 0x0000A800
   - 60MHz:   PLLN = 120 → 0x00007800
*/
```

### Step 3: Update Flash Latency
Look for line ~100 in `system_stm32f4xx.c`:
```c
/* Flash latency cycles based on frequency */
Flash Latency = 5 cycles for 150-180MHz (current - 168MHz)
Flash Latency = 4 cycles for 120-150MHz
Flash Latency = 3 cycles for 90-120MHz
Flash Latency = 2 cycles for 60-90MHz
```

### Step 4: Rebuild and Flash
```bash
cd /path/to/stm32-rtos-system
./build.sh          # Compile with new clock config
bash flash_latest.sh # Flash to board
```

---

## PLL Register Reference

### PLLCFGR - PLL Configuration Register (Offset: 0x04)

| Field | Bits | Current Value | Description |
|-------|------|---------------|-------------|
| **PLLSRC** | 22 | 1 | 0=HSI, 1=HSE (currently HSE) |
| **PLLQ** | 27:24 | 7 | USB/SDIO clock = VCO/PLLQ |
| **PLLP** | 17:16 | 0 | SYSCLK divider (00=/2, 01=/4, 10=/6, 11=/8) |
| **PLLN** | 14:6 | 336 | VCO multiplier (50-432) |
| **PLLM** | 5:0 | 8 | Input prescaler (2-63) |

### CFGR - Clock Configuration Register (Offset: 0x08)

| Field | Bits | Current Value | Description |
|-------|------|---------------|-------------|
| **SW** | 1:0 | 10 | Sysclk source (10=PLL) |
| **SWS** | 3:2 | - | Sysclk source status (read-only) |
| **HPRE** | 7:4 | 0 | AHB prescaler (0=/1, 1=/2, ..., F=/512) |
| **PPRE1** | 12:10 | 5 | APB1 prescaler (101=/4) |
| **PPRE2** | 15:13 | 4 | APB2 prescaler (100=/2) |

---

## Timing-dependent Code Update

When changing clock frequency, **timer-based delays must be recalibrated**:

### SysTick Calculation (in `minimal_led_blink.c`)
```c
/* SysTick interrupt every 1ms */
/* Formula: reload_value = (SYSCLK_HZ / 1000) - 1 */

/* 168MHz: reload = (168,000,000 / 1000) - 1 = 167,999 */
SysTick_Config(168000);

/* If changing to 120MHz: reload = (120,000,000 / 1000) - 1 = 119,999 */
SysTick_Config(120000);
```

### Busy-Wait Delays (in `minimal_led_blink.c`)
```c
/* Busy-wait approximation */
/* At 168MHz: ~168 iterations ≈ 1 microsecond */
/* At 120MHz: ~120 iterations ≈ 1 microsecond */

void delay_ms(uint32_t ms)
{
    /* Adjust multiplier based on SYSCLK */
    uint32_t cycles_per_us = SystemCoreClock / 1000000;
    for (uint32_t i = 0; i < ms * 1000 * cycles_per_us; i++) {
        __NOP();  /* No operation - prevents compiler optimization */
    }
}
```

---

## Verification After Clock Change

1. **Check SystemCoreClock variable**
   ```c
   /* Should be updated by SystemInit() */
   printf("SYSCLK = %lu Hz\n", SystemCoreClock);
   ```

2. **Verify LED Blink Rate**
   - Change frequency and rebuild
   - Flash to board
   - LED blink rate should change proportionally

3. **Use OpenOCD to Read Registers**
   ```bash
   openocd -f interface/stlink.cfg -f target/stm32f4x.cfg
   ```
   Then in telnet:
   ```
   mdw 0x40023804  # Read RCC->CFGR (clock status)
   mdw 0x40023808  # Read RCC->PLLCFGR (PLL config)
   ```

---

## Files Involved in Clock Configuration

| File | Purpose | Edit For |
|------|---------|----------|
| [system_stm32f4xx.c](../drivers/system_init/system_stm32f4xx.c) | PLL + prescaler config | Change frequency (PLLN, flash latency) |
| [system_stm32f4xx.h](../drivers/system_init/system_stm32f4xx.h) | Clock variable declarations | Rarely edit |
| [minimal_led_blink.c](../application/minimal_led_blink.c) | Busy-wait delays | Update if changing frequency |
| [stm.s](../BOOT/stm.s) | SysTick init | Update SysTick reload value |

---

## Quick Reference: Change 168MHz → 120MHz

```bash
# Edit this file:
# drivers/system_init/system_stm32f4xx.c

# Line ~100: Flash latency
# Change from: reg_value |= (uint32_t)0x00000005;  (5 cycles)
# Change to:   reg_value |= (uint32_t)0x00000004;  (4 cycles)

# Line ~180: PLLN value
# Change from: reg_value |= (uint32_t)0x0000A000;  (PLLN=336)
# Change to:   reg_value |= (uint32_t)0x0000F000;  (PLLN=240)

# Then:
./build.sh
bash flash_latest.sh
```

---

## Troubleshooting

| Issue | Cause | Solution |
|-------|-------|----------|
| Board unresponsive after clock change | Flash latency too low | Increase Flash->ACR cycles |
| LED blinks too fast/slow | SysTick not updated | Recalculate SysTick reload value |
| USB not working | APB2 freq wrong for USB | Verify PLLQ=7 (USB=48MHz) |
| Processor hangs | Frequency too high for CPU | Verify SYSCLK ≤ 168MHz |

---

## Reference Links
- **STM32F4 Reference Manual** (RM0090): Section 5 (Reset and Clock Control)
- **Cortex-M4 Technical Reference**: Max frequency = 168MHz
- **Flash Timing Requirements**: See datasheet Section 3.8
