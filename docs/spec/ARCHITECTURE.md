# STM32 Bare-Metal Architecture v1.0

## Layer Stack (Bottom to Top)

```
┌─────────────────────────────────────┐
│  Application Layer                  │  Business logic, policy, thresholds
│  (app/)                             │  - Control loop (motor speed, safety)
│                                     │  - Watchdog logic (failover decisions)
│                                     │  - Future: RTOS tasks
└──────────────────┬──────────────────┘
                   ↓
┌─────────────────────────────────────┐
│  Driver Layer                       │  Semantic API (device-independent)
│  (drivers/driver/)                  │  - Motor_A_Run(speed) → motor control
│                                     │  - ADC_ReadCurrent() → sensor read
│                                     │  - UART_SendHeartbeat() → comm
│                                     │  - Error codes: drv_status_t (0=OK, <0=ERR)
└──────────────────┬──────────────────┘
                   ↓
┌─────────────────────────────────────┐
│  BSP Layer (Board Support)          │  Hardware pin mapping (board-specific)
│  (drivers/bsp/)                     │  - bsp_motor.c: PD0-PD3, PE13-PE14, TIM1
│                                     │  - bsp_sensors.c: PA0-PA2 ADC channels
│                                     │  - bsp_uart_comm.c: PA9/PA10 USART1
│                                     │  - board_config.h: unified constants
└──────────────────┬──────────────────┘
                   ↓
┌─────────────────────────────────────┐
│  HAL Layer (Hardware Abstraction)   │  Register-level primitives (MCU-generic)
│  (drivers/hal/)                     │  - HAL_GPIO_SetBits() → GPIO BSRR write
│                                     │  - HAL_ADC_Read() → analogRead
│                                     │  - HAL_TIM_SetCompare() → PWM duty
│                                     │  - HAL_RCC_EnableClock() → clock tree
│                                     │  - NO state, NO logic, ONLY registers
└──────────────────┬──────────────────┘
                   ↓
┌─────────────────────────────────────┐
│  Startup + Linker                   │  CPU startup, memory layout
│  (boot/)                            │  - BOOT/startup.s (exception vectors)
│                                     │  - BOOT/linker_f4.ld (FLASH/RAM map)
└──────────────────┬──────────────────┘
                   ↓
        ┌──────────────────┐
        │   STM32 Hardware │
        │  (Registers)     │
        └──────────────────┘
```

## Responsibility Map

| Layer | Owns | Does NOT Own | Example |
|---|---|---|---|
| **HAL** | Register access primitives | State management | `HAL_GPIO_Write(port, pin, 1)` sets bit, done |
| **BSP** | Pin/clock definitions | Control logic | define `MOTOR_A_IN1_PIN = GPIO_Pin_0` |
| **Driver** | Semantic API, unit conversion | Business policy | `Motor_A_Run(75)` = 75%→999 duty→call BSP |
| **App** | Control policy, thresholds, sequencing | Hardware detail | if (current > 3500mA) brake; else run |

---

## RTOS Injection Points (Future)

When adding FreeRTOS, these are the **ONLY** parts that change:

1. **app/main()** → lightweight, creates tasks + starts scheduler
2. **app/rtos_compat.h** → enable `#define USE_FREERTOS`
3. **app/control_loop.c** → add `vTaskDelay(100)` instead of manual tick
4. **app/watchdog_logic.c** → add queue/semaphore sync
5. **app/app_f4_interrupt_handler()** → call `FromISR()` variants of RTOS API

**ZERO changes to:**
- drivers/hal/ (register primitives)
- drivers/bsp/ (pin mapping)
- drivers/driver/ (semantic API)
- boot/, stm/ (startup)

---

## Clock & Memory Model

**F4 Clock Tree:**
- HSE 8MHz (external oscillator) → PLL → SYSCLK 168MHz
- APB1 = 42MHz (peripheral clock for UART2, TIM2-7, SPI2-3, I2C1-3)
- APB2 = 84MHz (for UART1, SPI1, TIM1, ADC)

**F1 Clock Tree:**
- HSE 8MHz → PLL → SYSCLK 72MHz (F103 max)
- APB1 = 36MHz (UART2, TIM2-7)
- APB2 = 72MHz (UART1, SPI1, TIM1, ADC)

---

## Failure Modes & Safety

| Failure | Detection | Response |
|---|---|---|
| F4 lockup (watchdog timeout) | F1: no heartbeat for 200ms | F1 takeover motor |
| Motor overcurrent | App: ADC > 3500mA | Emergency brake |
| Motor overtemp | App: NTC > 85°C | Stop + cool down |
| F1 lockup (IWDG) | F1 register timeout | F1 auto-reset |

---

## Build Targets & Artifacts

```
make -f Makefile.f4 all
  → build/firmware/STM32F407_MotorControl.elf (3.6KB text)
  → build/firmware/STM32F407_MotorControl.bin (flash image)
  → build/firmware/STM32F407_MotorControl.hex (text format)

make -f Makefile.f1 all
  → build_f1/firmware/STM32F103_WatchdogRedundancy.elf (pending stm32f10x.h)
  → (same hex/bin as F4)
```

---

## File Organization Rules

- **drivers/hal/hal_*.c/h**: pure register access, no ifdef, no state
- **drivers/bsp/bsp_*.c/h**: board constants, init sequences (calls HAL)
- **drivers/driver/*_driver.c/h**: unit conversion, error codes (calls BSP)
- **app/**: orchestration, policy, thresholds (calls driver)
- **stm/stm32f4xx/, stm32f10x/**: MCU-specific system init

---

## Version Freeze (v1.0)

This document locks **ARCHITECTURE v1.0** for:
- Feature set: motor control + sensor read + watchdog failover
- Runtime: bare-metal superloop, 100ms control cycle
- Target: inheritance foundation for RTOS project

See **BARE_METAL_v1_0_SPEC.md** for feature checklist & sign-off.
