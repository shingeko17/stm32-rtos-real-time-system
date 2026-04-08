# Failover Integration Test Procedure v1.0

**Date:** 2026-03-29  
**Status:** Ready for Hardware Test  
**Target:** F4 ↔ F1 Heartbeat + Failover Validation

---

## 1. Hardware Setup

### Requirements
- STM32F407VET6 board (F4) with motor control circuit
- STM32F103T8C6 board (F1) with status LED
-ST-Link v2 programmer (×2 if simultaneous programming needed, or 1 shared)
- USB power supply for both boards
- UART connection F4→F1 (PA9 TX → PA3 RX @ 115200 baud)
- Oscilloscope or logic analyzer (optional, for latency measurement)

### Physical Connections
```
F4 (STM32F407):                F1 (STM32F103):
  PA9 (USART1 TX) ─────────→ PA3 (USART2 RX)
  GND ─────────────────────→ GND
  Power ─────────────────→ Power
```

---

## 2. Pre-Flight Checks

### Software Prerequisites
- ✅ `./build.sh rebuild` completed (F4 firmware ready)
- ✅ `./build_f1.sh` completed (F1 firmware ready)
- ✅ OpenOCD installed: `openocd --version`
- ✅ `stm32f4.cfg` and `stm32f1.cfg` present in repo root

### Firmware Files
```bash
build/firmware/STM32F407_MotorControl.hex       # F4 application
build_f1/firmware/STM32F103_WatchdogRedundancy.hex  # F1 watchdog
```

---

## 3. Phase 1: Individual Board Testing

### 3.1 Flash F4 Firmware
```bash
./flash.sh
```

**Expected Output:**
```
[SUCCESS] Firmware flashed and verified successfully!
```

**Verify:** Motor runs open-loop at fixed duty when board powers up (check with scope/motor load)

---

### 3.2 Flash F1 Firmware
```bash
./flash_f1.sh
```

**Expected Output:**
```
[SUCCESS] F1 Firmware flashed and verified successfully!
```

**Verify:** 
- PC13 LED ON (F1 starts assuming F4 is alive)
- F1 IWDG fed every loop (F1 main loop is running)

---

## 4. Phase 2: Heartbeat Communication Test

### 4.1 Connect Boards

Connect UART between boards:
```
F4 PA9 (USART1 TX) → F1 PA3 (USART2 RX)
F4 GND → F1 GND
```

### 4.2 Power On Both Boards

1. Power F4 first
2. Power F1 within 1 second
3. Observe F1 LED status:
   - **LED ON** = F4 heartbeat received (expected ✅)
   - **LED OFF** = F4 heartbeat timeout (unexpected ❌)

### 4.3 Monitor UART Traffic (Optional)

Use serial terminal @ 115200 baud:
```bash
# On F1 USART2 (PA2/PA3)
minicom /dev/ttyUSB0 -b 115200
```

**Expected:** 0xAA byte every 100±20ms

---

## 5. Phase 3: F4 Failure Injection Test (Critical)

### 5.1 Force F4 Heartbeat Stop

Method: Halt F4 CPU via debugger
```bash
openocd -f stm32f4.cfg -c "init" -c "targets" -c "halt" -c "shutdown"
```

**During Halt:**
- Initial LED state: ON (0-100ms)
- At 200ms: LED transitions to OFF (watchdog timeout triggered)
- Expected: F1 LED OFF continuously

### 5.2 Resume F4

```bash
openocd -f stm32f4.cfg -c "init" -c "targets" -c "resume" -c "shutdown"
```

**Expected:**
- F1 LED turns ON within 200ms (heartbeat resumed)
- Motor returns to controlled state (coast mode)

### 5.3 Measure Failover Latency

**Goal:** < 200ms (requirement from spec)

**Measurement Setup:**
1. Use oscilloscope
2. Trigger on F4 TX stop (CH1 = F4 PA9)
3. Measure to F1 LED change (CH2 = F1 PC13)

**Calculation:**
```
Latency = (F1 LED transition time) - (F4 TX stop time)
Expected: < 200ms
```

---

## 6. Phase 4: IWDG Self-Watchdog Test (F1 Robustness)

### 6.1 Force F1 Main Loop Stall

Via debugger:
```bash
openocd -f stm32f1.cfg -c "init" -c "targets" -c "halt" -c "shutdown"
```

**Expected:** F1 CPU resets within 1000ms (IWDG timeout)

**Verify:**
- PC13 LED resets (goes OFF briefly)
- F1 restarts and resumes heartbeat monitoring

---

## 7. Test Coverage Matrix

| Test Case | Precondition | Action | Expected Result | Pass/Fail |
|-----------|--------------|--------|-----------------|-----------|
| **T1** | Both boards ON | None | F1 LED ON | ☐ |
| **T2** | Both boards ON | Halt F4 | F1 LED OFF within 200ms | ☐ |
| **T3** | F1 LED OFF | Resume F4 | F1 LED ON within 200ms | ☐ |
| **T4** | Both boards ON | Halt F1 main | F1 resets in <1s (IWDG) | ☐ |
| **T5** | Both boards ON | Power cycle F4 | F1 LED OFF then ON | ☐ |
| **T6** | F1 LED OFF | Disconnect UART | F1 stays OFF (no recovery) | ☐ |
| **T7** | F1 OFF | Override UART RX | LED toggles / F1 boots | ☐ |

---

## 8. Measurement Examples

### Example 1: Normal Operation (T1)
```
Time 0ms:    F1 boots, heartbeat_timer = 200ms
Time 0-100ms: 0xAA received from F4 every 100ms
Time 100ms:  Heartbeat_timer reloads = 200ms
Time ∞:      LED stays ON
Status: ✅ PASS
```

### Example 2: F4 Failure & Recovery (T2+T3)
```
Time 0ms:    Both running, F1 LED ON
Time 10ms:   F4 halted (no more 0xAA)
Time 100ms:  Heartbeat_timer = 100ms
Time 200ms:  Heartbeat_timer = 0, timeout → LED OFF
Latency:     200-10 = 190ms → ✅ < 200ms PASS

Time 210ms:  F4 resumed, 0xAA received
Time 220ms:  Heartbeat_timer reloaded
Time 230ms:  LED turns ON
Recovery: 20ms → ✅ PASS
```

---

## 9. Troubleshooting

### F1 LED Never Changes
1. Check UART connection F4 PA9 → F1 PA3
2. Verify baud rate: F1 USART2 @ 115200
3. Check F1 GPIO PC13 for shorts/damage

### F1 Hangs / Constant Resets
- IWDG triggering every 1s = main loop stalled
- Check for infinite loops in USART2_IRQHandler
- Verify SysTick interrupt is configured

### Latency > 200ms
- F1 heartbeat_timer may be miscalibrated
- Check SysTick frequency: should be 1ms/tick
- Confirm F4 sending 0xAA every 100ms (verify USART1)

---

## 10. Sign-Off Criteria (Definition of Done)

- ✅ **T1 PASS:** F1 LED ON when both boards powered
- ✅ **T2 PASS:** F1 LED OFF within 200ms after F4 crash
- ✅ **T3 PASS:** F1 LED ON within 200ms after F4 recovery
- ✅ **T4 PASS:** F1 IWDG reset confirmed <1s
- ✅ **T5 PASS:** F1 handles F4 power cycle gracefully
- Memory usage: F4 < 5%, F1 < 2%
- No compiler warnings (-Wall -Wextra)
- All tests logged with timestamps

---

## 11. Next Steps (Post v1.0)

1. **PID Closed-Loop Control** - Use encoder RPM feedback for speed regulation
2. **Telemetry** - Log sensor data to F4 SD card
3. **CAN/Modbus** - Replace UART heartbeat with robust protocol
4. **RTOS Migration** - FreeRTOS task scheduling
5. **Dashboard** - Web UI for real-time telemetry

---

**Test Date:** _______________  
**Tester Name:** _______________  
**All Tests Passed:** ☐ YES ☐ NO  
**Notes:** _________________________________________________
