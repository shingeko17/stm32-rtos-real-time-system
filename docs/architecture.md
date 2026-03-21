# System Architecture - Dual-MCU RTOS with Redundancy

## Overview

This project implements an **ECU-like real-time embedded system** with:
- **Primary MCU (STM32F407)**: Main RTOS controller for motor & sensors
- **Backup MCU (STM32F103)**: Watchdog redundancy with failover capability
- **Dual-channel communication**: F4 → F1 heartbeat via UART

---

## System Block Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                   STM32F407VET6 (PRIMARY)                   │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌──────────────┐  ┌────────────────┐  ┌──────────────┐   │
│  │ Motor Task   │  │  Sensor Task   │  │ Telemetry    │   │
│  │ (PWM Ctrl)   │  │ (ADC + DMA)    │  │ (Data Format)│   │
│  └──────────────┘  └────────────────┘  └──────────────┘   │
│           ↓               ↓                    ↓            │
│  ┌──────────────────────────────────────────────────────┐  │
│  │    FreeRTOS Kernel (5 Tasks)                         │  │
│  │  - Scheduler, Queue, Mutex, Semaphore               │  │
│  └──────────────────────────────────────────────────────┘  │
│           ↑                ↑                    ↑            │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Heartbeat Task                    System Monitor   │  │
│  │  (Sends 0xAA via UART to F1)       (CPU Load, etc)  │  │
│  └──────────────────────────────────────────────────────┘  │
│                                                              │
└─────────────────────────────────────────────────────────────┘
                            ↓ UART
                    (50-100ms Heartbeat)
                            ↓
┌─────────────────────────────────────────────────────────────┐
│                   STM32F103C8 (BACKUP)                      │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌──────────────────────────────────────────────────────┐  │
│  │    Watchdog & Failover Logic (Interrupt-Driven)     │  │
│  │  - UART RX ISR: Listen for 0xAA pulses              │  │
│  │  - Watchdog Timer: 100-200ms timeout                │  │
│  │  - On Timeout: Activate motor control, safe default │  │
│  └──────────────────────────────────────────────────────┘  │
│                           ↓                                  │
│                    Motor Driver F1                          │
│                                                              │
└─────────────────────────────────────────────────────────────┘
                            ↓
                       [MOTOR]
```

---

## Data Flow - Normal Operation (F4 Active)

```
Sensors (ADC) 
    ↓
DMA Ring Buffer
    ↓
Sensor Task (reads DMA)
    ↓
RTOS Queue (sensor_queue)
    ↓
Motor Task (motor_speed = queue data)
    ↓
PWM Driver (update TIM1 duty)
    ↓
Motor Output
    ↓
Telemetry Task (collect speed + sensor)
    ↓
UART → Host PC (115200 baud)
```

---

## Failover Scenario (F4 Crashes)

```
Time T0:   F4 runs normally, sends heartbeat (0xAA) to F1
Time T1:   F4 hangs/crashes (watchdog not sent)
Time T2:   F1 detects missing heartbeat (> 200ms elapsed)
Time T3:   F1 activates motor control
           - Motor speed → safe default (coast or slow)
           - F1 motor driver takes over
           - System continues in degraded mode
Time T4+:  If F4 recovers, sends heartbeat again
           - F1 detects pulse, deactivates failover
           - System back to normal
```

---

## Task Layout (F4)

| Task | Priority | Period | Purpose |
|------|----------|--------|---------|
| Heartbeat | 4 (HIGH) | 50-100ms | Send 0xAA to F1 |
| Motor Control | 3 | 10ms | Update PWM, read encoder |
| Sensor Task | 3 | 10ms | ADC + DMA acquisition |
| Telemetry | 2 | 100ms | Format + send data |
| System Monitor | 2 | 500ms | CPU load, stack check |

---

## Hardware Interfaces

### F4 Connections
- **Motor PWM**: TIM1 (PA8-PA9) → Motor Driver
- **Encoder**: TIM3 (PA6-PA7) → Counter capture
- **ADC**: ADC1 (PA0-PA3) → Sensor inputs
- **UART Heartbeat**: UART6 (PC6) TX → F1 RX
- **Telemetry UART**: UART1 (PA9) TX → Host PC

### F1 Connections
- **UART Listener**: UART2 (PA2) RX ← F4 TX
- **Motor Control**: GPIO (motor outputs) 
- **Failover Motor**: L298N or same driver as F4

---

## Clock Configuration

- **F4**: HSE 8MHz → PLL → 168MHz (SYSCLK)
  - AHB: 168MHz
  - APB1: 42MHz (Timers, UARTs 2-5)
  - APB2: 84MHz (TIM1, UART1, SPI1)

- **F1**: HSE 8MHz → PLL → 72MHz (SYSCLK)
  - AHB: 72MHz
  - APB1: 36MHz
  - APB2: 72MHz

---

## Memory Map

### F4 (STM32F407VET6)
- **Flash**: 512 KB (0x08000000 - 0x0807FFFF)
- **SRAM**: 128 KB (0x20000000 - 0x2001FFFF)
  - Stack (bottom, grows down)
  - Heap (FreeRTOS)
  - Binary (.data)
  - Code (.text)
- **CCM**: 64 KB (0x10000000 - 0x1000FFFF) - NOT DMA-accessible

### F1 (STM32F103C8)
- **Flash**: 64 KB (compact bootloader + watchdog firmware)
- **SRAM**: 20 KB (for watchdog state machine + buffers)
