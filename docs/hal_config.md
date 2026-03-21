# HAL Configuration - STM32F407VET6

**MCU**: STM32F407VET6 (ARM Cortex-M4F, 168 MHz)  
**HAL Version**: STM32F4xx_HAL_V1.27.1 (or newer)  
**System Clock**: 168 MHz (from 8 MHz HSE with PLL)

---

## System Clock Configuration

### Clock Sources

| Source | Frequency | Usage | Status |
|--------|-----------|-------|--------|
| HSE (External) | 8 MHz | PLL input | ✅ Enabled |
| HSI (Internal) | 16 MHz | Backup | - |
| SYSCLK | 168 MHz | CPU clock | ✅ Main |
| HCLK | 168 MHz | AHB bus | - |
| PCLK1 | 42 MHz | APB1, Timers | - |
| PCLK2 | 84 MHz | APB2, Timers | - |

### PLL Configuration

```
HSE Input: 8 MHz
├─ M (Prescaler): 8       → 1 MHz
├─ N (Multiplier): 336    → 336 MHz
├─ P (SYSCLK divider): 2  → 168 MHz
└─ Q (USB/SDIO): 7        → 48 MHz
```

**Key Calculations:**
- VCO Input: 8 MHz / 8 = 1 MHz ✅ (valid 1-2 MHz)
- VCO Output: 1 MHz × 336 = 336 MHz ✅ (valid 192-432 MHz)
- SYSCLK: 336 MHz / 2 = 168 MHz
- USB Clock: 336 MHz / 7 = 48 MHz ✅ (USB requires exactly 48 MHz)

### Flash Wait States

| CPU Freq | Wait States | Voltage |
|----------|-------------|---------|
| 0-30 MHz | 0 | 2.7-3.6V |
| 30-60 MHz | 1 | 2.7-3.6V |
| 60-90 MHz | 2 | 2.7-3.6V |
| 90-120 MHz | 3 | 2.7-3.6V |
| 120-150 MHz | 4 | 2.7-3.6V |
| 150-168 MHz | 5 | 2.7-3.6V |

**Configuration**: **5 wait states** for 168 MHz operation

### RCC Initialization Code Template

```c
// Simplified clock configuration (see HAL RCC module for full implementation)
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    
    // Enable HSE oscillator
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;    // = 2
    RCC_OscInitStruct.PLL.PLLQ = 7;
    
    HAL_RCC_OscConfig(&RCC_OscInitStruct);
    
    // Configure clock distribution
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | 
                                   RCC_CLOCKTYPE_SYSCLK |
                                   RCC_CLOCKTYPE_PCLK1 | 
                                   RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;      // HCLK = 168 MHz
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;       // PCLK1 = 42 MHz
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;       // PCLK2 = 84 MHz
    
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
}
```

---

## Peripheral Clock Distribution

### AHB1 Peripherals (168 MHz)

| Peripheral | Reset | Clock | Usage |
|------------|-------|-------|-------|
| GPIOA-K | ✅ | ✅ | GPIO ports |
| CRC | ✅ | ✅ | CRC calculation |
| DMA1 | ✅ | ✅ | ADC, UART DMA |
| DMA2 | ✅ | ✅ | Ethernet DMA |
| ETH_MAC | ✅ | ✅ | Ethernet controller |
| OTG_HS | - | ✅ | USB HS (optional) |

### AHB2 Peripherals (168 MHz)

| Peripheral | Status | Notes |
|------------|--------|-------|
| DCMI | - | Not used |
| HASH | - | Not used |
| CRYP | - | Not used |
| RNG | - | Not used |

### APB1 Peripherals (42 MHz)

| Peripheral | F4 | F1 | Notes |
|------------|-----|-----|-------|
| TIM2-7 | ✅ TIM3 | ✅ | Motor encoder (F4), watchdog timer (F1) |
| USART1-5 | ✅ UART6 | ✅ UART2 | F4: Heartbeat TX, F1: Heartbeat RX |
| I2C1-3 | - | - | Not used |
| SPI1-3 | - | - | Not used |
| PWR | ✅ | ✅ | Power control |

**Timer Clock Frequency**: APB1 × 2 = 84 MHz (F4), 72 MHz (F1)

### APB2 Peripherals (84 MHz)

| Peripheral | F4 | F1 | Notes |
|------------|-----|-----|-------|
| TIM1 | ✅ | ✅ | Motor PWM control |
| USART1 | ✅ UART1 | - | F4: Telemetry to Host PC |
| ADC1-3 | ✅ ADC1 | - | F4: Sensor inputs (3 channels) |
| SDIO | - | - | Not used |
| SYSCFG | ✅ | ✅ | System config |
| EXTI | ✅ | ✅ | External interrupts |

**Timer Clock Frequency**: APB2 × 2 = 168 MHz (F4), 72 MHz (F1)

---

## GPIO Configuration

### F4 (STM32F407VET6) - Pin Usage

```c
// Motor Control
PA8  → TIM1_CH1 (AF1)   = Motor PWM A
PA9  → TIM1_CH2 (AF1)   = Motor PWM B
PD0  → GPIO OUT         = Motor A direction 1
PD1  → GPIO OUT         = Motor A direction 2
PD2  → GPIO OUT         = Motor B direction 1
PD3  → GPIO OUT         = Motor B direction 2

// Encoder Feedback
PA6  → TIM3_CH1 (AF2)   = Encoder A
PA7  → TIM3_CH2 (AF2)   = Encoder B

// ADC Sensors
PA0  → ADC1_IN0         = Current sense A
PA1  → ADC1_IN1         = Current sense B
PA2  → ADC1_IN2         = Temperature
PA3  → ADC1_IN3         = Speed reference

// UART Communications
PC6  → UART6_TX (AF8)   = Heartbeat to F1
PA9  → UART1_TX (AF7)   = Telemetry to Host
PA10 → UART1_RX (AF7)   = Host PC debug

// Status
PC13 → GPIO OUT         = LED status (optional)
```

### F1 (STM32F103C8) - Pin Usage

```c
// Watchdog Receiver
PA2  → UART2_RX         = Heartbeat from F4

// Backup Motor Control
PA8  → TIM1_CH1         = Backup Motor PWM
PB0-PB3 → GPIO OUT      = Backup motor direction

// Status
PC13 → GPIO OUT         = Failover LED indicator
```

---

## Interrupt Priority Configuration

### NVIC Priority Grouping

```
Priority Grouping: NVIC_PRIORITYGROUP_4
├─ Preemption Level: 0-15 (4 bits)
└─ Sub-priority: 0 (0 bits)
```

### Recommended Priority Levels

| Device | Priority | Critical | Notes |
|--------|----------|----------|-------|
| SysTick | 15 | ✅ | RTOS scheduler tick (FreeRTOS) |
| UART6 RX | 4 | ✅ | Heartbeat receiver (F1 uses same) |
| UART1 TX/RX | 5 | - | Telemetry to host (F4 only) |
| DMA2_Stream4 | 6 | ✅ | ADC data acquisition (F4 only) |
| TIM1 | 3 | ✅ | Motor PWM (both F4 & F1) |
| TIM3 | 7 | - | Encoder counter (F4 only) |
| GPIO EXTI | 8 | - | External interrupts |

**Notes on F1 Watchdog:**
- Uses Timer2 (APB1) as watchdog counter
- No FreeRTOS (bare-metal interrupt-driven)
- Simple state machine: listening → timeout → failover

---

## HAL Initialization Sequence

### F4 Startup (with FreeRTOS)

```c
1. SystemInit()           → Configure clock to 168 MHz, flash
2. HAL_Init()             → Initialize HAL, SysTick
3. SystemClock_Config()   → Verify clock setup
4. MX_GPIO_Init()         → Configure all GPIO pins
5. MX_DMA_Init()          → DMA2_Stream4 for ADC
6. MX_ADC1_Init()         → ADC1 (3 channels, continuous)
7. MX_TIM1_Init()         → TIM1 PWM (1kHz, 8-bit)
8. MX_TIM3_Init()         → TIM3 encoder counter
9. MX_UART6_Init()        → UART6 for heartbeat TX
10. MX_UART1_Init()        → UART1 for telemetry TX
11. HAL_DMA_Start_IT()      → Start ADC DMA transfers
12. HAL_ADC_Start_DMA()     → Begin ADC conversions
13. xTaskCreate()           → Create 5 RTOS tasks
14. vTaskStartScheduler()   → Start FreeRTOS
```

### F1 Startup (Bare Metal)

```c
1. SystemInit()             → Configure clock to 72 MHz
2. HAL_Init()               → Initialize HAL
3. SystemClock_Config()     → Verify clock setup
4. GPIO_Init()              → Configure PA2 (UART2 RX), PA8, PB0-PB3 (motor)
5. MX_UART2_Init()          → UART2 for heartbeat RX
6. MX_TIM1_Init()           → TIM1 PWM for backup motor
7. MX_TIM2_Init()           → TIM2 as watchdog timer
8. Watchdog_Init()          → Set parameters (200ms timeout)
9. UART2_Start_IT()         → Begin receiving heartbeats
10. while(1)                 → Simple state machine loop
    - Check watchdog timer
    - If timeout: activate motor control
    - If heartbeat received: reset watchdog
```

---

## ADC Configuration (F4 Only)

| Parameter | Value | Notes |
|-----------|-------|-------|
| Resolution | 12-bit | STM32F4 default |
| Conversion Time | ~15 ADC cycles | At 36 MHz ADC clock |
| Sample Rate | 10 kHz | Triggered by DMA |
| DMA Mode | Circular | Ring buffer pattern |
| Channels | 3 | IN0, IN1, IN2 (current_A, current_B, temp) |

---

## Timer Configuration

### TIM1 PWM (F4 & F1)

| Parameter | F4 | F1 |
|-----------|----|----|
| Frequency | 168 MHz | 72 MHz |
| Prescaler | 167 | 71 |
| AutoReload | 99 | 99 |
| PWM Frequency | 1 kHz | 1 kHz |
| Resolution | 8-bit (0-99 → 0-100%) | 8-bit |

### TIM3 Encoder (F4 Only)

| Parameter | Value |
|-----------|-------|
| Mode | Encoder mode (both edges) |
| Period | Max uint32 (32-bit counter) |
| Prescaler | 0 (no prescaling) |
| Channels | PA6, PA7 (quadrature input) |

### TIM2 Watchdog Counter (F1 Only)

| Parameter | Value | Purpose |
|-----------|-------|---------|
| Frequency | 72 MHz | APB1 timer |
| Prescaler | 7199 | → 10 kHz tick |
| AutoReload | 2000 | → 200ms period |
| Mode | Up counting | Reset on heartbeat RX |

---

## UART Configuration

### UART6 (F4 Heartbeat TX)

| Parameter | Value |
|-----------|-------|
| Baud Rate | 115200 |
| Data Bits | 8 |
| Stop Bits | 1 |
| Parity | None |
| DMA | Off (simple byte TX) |
| Trigger | Timer-based (50ms interval) |

### UART1 (F4 Telemetry)

| Parameter | Value |
|-----------|-------|
| Baud Rate | 115200 |
| Data Bits | 8 |
| Stop Bits | 1 |
| Parity | None |
| DMA | Optional (for bulk data) |
| Buffer Size | 128 bytes |
| Format | CSV or binary (TBD) |

### UART2 (F1 Heartbeat RX)

| Parameter | Value |
|-----------|-------|
| Baud Rate | 115200 |
| Data Bits | 8 |
| Stop Bits | 1 |
| Parity | None |
| Interrupt | ✅ Enabled (ISR on byte RX) |
| Action on RX | Reset watchdog timer |
10. MX_ETH_Init()         → Configure Ethernet
11. SystemClock_Config()  → Final clock validation
```

---

## Power Management

### Power Modes

| Mode | SYSCLK | Sleep Timings | Usage |
|------|--------|---------------|-------|
| Run | 168 MHz | Normal | Active operation |
| Sleep | 168 MHz | CPU off, peripherals on | Light sleep |
| Stop | Off | Peripherals on, RTC | Deep sleep |
| Standby | Off | Only RTC/WDOG | Lowest power |

### Typical Power Consumption

| Mode | Current | Wake Time |
|------|---------|-----------|
| Run 168MHz | ~150 mA | - |
| Sleep | ~100 mA | Immediate |
| Stop | ~10 µA | 50 µs |
| Standby | <1 µA | 10 ms |

---

## Debug Configuration

### JTAG/SWD Pins

| Pin | Function | Mode |
|-----|----------|------|
| PA13 | SWDIO | AF0 |
| PA14 | SWCLK | AF0 |
| PB3 | SWO | AF0 (ITM trace) |

**Configuration**: SWD mode (2 pins) recommended over JTAG (4+ pins)

---

## Next Steps

1. Implement `SystemClock_Config()` in your startup code
2. Configure all peripheral clocks in their respective `MX_*_Init()` functions
3. Set interrupt priorities before FreeRTOS initialization
4. Validate clock output with oscilloscope if available
5. Monitor power consumption in different modes
