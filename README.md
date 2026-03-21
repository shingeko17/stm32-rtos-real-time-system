# STM32 RTOS Real-Time Motor System with Dual-MCU Redundancy

## Overview

This project implements an **ECU-like black box system** with real-time motor control on STM32F407 (primary) and redundancy watchdog on STM32F103 (backup).

### System Goals
- **Real-time RTOS architecture** with multi-task scheduling
- **Motor control** with PWM and encoder feedback
- **Sensor telemetry** system (ADC + DMA data acquisition)
- **Dual-MCU redundancy** with heartbeat-based failover mechanism
- **Modular firmware design** for scalability and maintainability

The system evaluates **timing predictability**, **task scheduling performance**, and **fault tolerance** in embedded real-time systems.

---

## Hardware Architecture

### Primary MCU: STM32F407VET6 (F4)
- **Role**: Main system controller, motor driver, sensor data processor
- **Features**: 
  - PWM motor control with feedback
  - ADC + DMA for sensor data pipeline
  - UART heartbeat sender to F1
  - 5 concurrent RTOS tasks
  - ~65% CPU utilization target

### Backup MCU: STM32F103C8 (F1)
- **Role**: Watchdog monitor, failover controller
- **Features**:
  - Monitors heartbeat from F4 (50-100ms intervals)
  - Detects F4 crash/hang
  - Automatic motor control takeover if F4 fails
  - Independent motor drivers

### Communication Link
- **Protocol**: Simple UART-based heartbeat
- **Timing**: F4 sends pulse every 50-100ms
- **Failover**: F1 activates motor control if 2+ missing heartbeats (100-200ms timeout)
- **Connector**: UART TX/RX + GND

### Peripherals
- **Motor**: DC motor with quadrature encoder
- **Sensors**: Analog sensors (current, temperature, speed)
- **Telemetry**: UART output for data logging / external console
- **Power**: Dual supply (one per MCU for isolation)

---

## Software Architecture

### F4 (Primary) - RTOS Task Structure

**5 RTOS Tasks** running on FreeRTOS:

1. **Motor Control Task** (Priority 3)
   - PWM duty cycle adjustment
   - Encoder feedback processing
   - Speed/torque regulation

2. **Sensor Reading Task** (Priority 3)
   - ADC + DMA data acquisition
   - Analog filter/averaging
   - Current/temperature monitoring

3. **Telemetry Task** (Priority 2)
   - Collects motor + sensor data
   - Formats data for UART output
   - Logs system state to external console

4. **Watchdog Heartbeat Task** (Priority 4 - highest)
   - Sends periodic pulse to F1 via UART
   - Interval: 50-100ms configurable
   - Indicates F4 is alive and responsive

5. **System Monitor Task** (Priority 2)
   - Tracks CPU load, timing violations
   - Monitors task stack usage
   - Detects anomalies (watchdog failures, sensor errors)

### F1 (Backup) - Watchdog Task

**Watchdog & Failover Logic** (non-RTOS, interrupt-driven):

1. **Heartbeat Listener**
   - UART interrupt detects pulses from F4
   - Resets watchdog timer on each pulse
   - Timeout window: 100-200ms

2. **Failover Handler**
   - If watchdog timer expires → F4 assumed dead
   - Activates F1's motor drivers
   - Maintains last-known motor state (safe default)
   - Optional: log failover event to external storage

---

## Communication Protocol: F4 ↔ F1 Heartbeat

### Message Format (Simple)
```
F4 → F1: Single byte pulse (0xAA) every 50-100ms
         via UART @ 115200 baud

F1 Logic:
  On byte receive: Reset watchdog timer to T_max (200ms)
  On timer expire: F4 is dead → activate failover
  On receive resume: F4 recovered → deactivate failover
```

### Rationale (Simple Protocol)
- **Low bandwidth**: Single byte per cycle
- **Low latency**: No protocol negotiation overhead
- **Robust**: Easy to validate (byte = pulse, no checksum needed)
- **Scalable**: Can add optional ACK later if needed

---

## File Structure & Implementation Roadmap

### Current Status
- ✅ Motor control driver (PWM + encoder reading)
- ✅ RTOS base structure
- ✅ System init framework
- 🟡 Driver abstraction layer (partial)
- ❌ Sensor ADC driver (not started)
- ❌ DMA configuration (not started)
- ❌ Telemetry data collection (not started)
- ❌ UART heartbeat task (not started)
- ❌ F1 failover firmware (not started)

### Key Files to Modify / Create

**On F4 (Primary):**
```
drivers/
  ├── system_init/
  │   ├── misc_drivers.h       [MODIFY] Add ADC, UART, DMA init
  │   └── misc_drivers.c
  ├── api/
  │   ├── motor_driver.h        [MODIFY] Add encoder reading
  │   ├── motor_driver.c
  │   ├── sensor_driver.h       [CREATE] ADC + DMA interface
  │   ├── sensor_driver.c
  │   ├── uart_driver.h         [CREATE] Heartbeat UART
  │   ├── uart_driver.c
  │   └── telemetry_driver.h    [CREATE] Data formatting
  ├── BSP/
  │   └── bsp_motor.h/c         [MODIFY] Multi-driver support
  │
application/
  ├── main.c                    [MODIFY] Task creation (5 tasks)
  ├── tasks/
  │   ├── motor_control_task.c  [CREATE/MODIFY]
  │   ├── sensor_read_task.c    [CREATE]
  │   ├── telemetry_task.c      [CREATE]
  │   ├── heartbeat_task.c      [CREATE]
  │   └── system_monitor_task.c [CREATE]
  │
  └── config/
      └── rtos_config.h         [CREATE/MODIFY] Task priorities, timings
```

**On F1 (Backup):**
```
application_f1/
  ├── main.c                    [CREATE] F1 firmware entry
  ├── watchdog/
  │   ├── failover.c            [CREATE] Heartbeat logic
  │   └── failover.h
  ├── drivers/
  │   ├── uart_listener.c       [CREATE] UART RX interrupt
  │   └── motor_driver_f1.c     [CREATE] Backup motor control
  │
  └── config/
      └── timing_config.h       [CREATE] Heartbeat interval, timeout
```

---

## Implementation Challenges & Considerations

### 1. Timing & Real-Time Constraints
- **Heartbeat must not block** motor control on F4 (use interrupt-driven or low-priority task)
- **ADC + DMA** must not interfere with motor PWM timing (separate DMA channels)
- **Telemetry** should not cause CPU spikes (rate-limit or use circular buffer)
- **Challenge**: CPU load ~65% already; adding 5 tasks + telemetry may exceed headroom

### 2. Failover Transition
- **Challenge**: What motor state does F1 adopt when taking over? (coast, brake, last command?)
- **Race condition**: If F1 activates during F4 crash, motor command might be inconsistent
- **Solution**: F1 reads motor state on takeover or uses safe default (slow speed first, ramp up)

### 3. Data Consistency (Multi-task Access)
- **Sensor data**: Motor task + Telemetry task both read sensor values
- **Motor command**: Heartbeat task must not interrupt motor duty cycle update
- **Solution**: Use RTOS queues/mutexes, or atomic read/write patterns

### 4. UART Communication Reliability
- **F4 → F1**: Heartbeat via UART (single cable, no redundancy)
- **Risk**: Cable disconnect = false failover on F1
- **Mitigation**: Monitor F1's motor output for anomalies on F4 side (optional feedback channel)

### 5. F1 Firmware Complexity
- **F1 logic** must be bulletproof (no RTOS, direct interrupt handling)
- **Edge case**: What if F4 sends garbage on UART during power-up noise?
- **Solution**: UART frame validation, timeout-based state machine

---

## Feature Checklist

### Current (Baseline)
- [x] PWM motor control (F4)
- [x] Basic RTOS startup
- [x] System initialization

### This Sprint (5 Tasks + Telemetry + Redundancy)
- [ ] Expand RTOS to 5 concurrent tasks
- [ ] ADC + DMA sensor data pipeline (F4)
- [ ] Telemetry data collection & formatting (F4)
- [ ] UART heartbeat task (F4)
- [ ] Watchdog failover firmware (F1)
- [ ] F1 backup motor control
- [ ] Cross-MCU coordination logic

### Future Enhancements
- [ ] CAN bus instead of UART (if reliability needed)
- [ ] EEPROM logging for crash diagnostics
- [ ] Web dashboard for telemetry
- [ ] Parameter tuning interface (speed, acceleration)

---

## Building & Running

### Prerequisites
- STM32CubeIDE or ARM GCC toolchain
- STM32F4 and STM32F1 HAL libraries installed
- FreeRTOS kernel
- JLink / ST-Link debugger for flashing F4 + F1

### Build Steps
```bash
make clean
make all          # Builds both F4 and F1 firmware
make flash_f4     # Flash primary MCU
make flash_f1     # Flash backup MCU
```

### Debugging & Verification
- **Serial Terminal**: Monitor telemetry output from F4 (115200 baud, /dev/ttyUSB0)
- **Heartbeat Test**: Watch UART traffic between F4 & F1 with logic analyzer
- **Failover Test**: Disconnect F4's heartbeat UART, verify F1 activates
- **Load Test**: Run motor at different speeds, check CPU % in System Monitor task output

---

## References & Documentation

- `docs/architecture.md` - System design diagrams
- `docs/hal_config.md` - Hardware abstraction layer configuration
- `docs/peripheral_drivers.md` - Detailed driver documentation
- `docs/memory_map.md` - Memory layout & section definitions
- FreeRTOS documentation: https://www.freertos.org/

---

**Project Status**: Active Development (Phase 2: Multi-task RTOS + Redundancy)  
**Last Updated**: March 22, 2026
