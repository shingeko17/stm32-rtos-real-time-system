# VER1 — PROGRESS TRACKER
### STM32F407 Bare-metal DC Motor Control + STM32F103 Watchdog

---

## VER1 — Bare-metal DC Motor Control

### Code đã hoàn thành

| Component | File(s) | Status |
|-----------|---------|--------|
| **Boot F1** | `boot/f1/stm.s`, `boot/f1/stm.ld` | ✅ Done |
| **Clock init** | `drivers/system_init/system_stm32f4xx.c` | ✅ Done |
| **BSP Motor** | `drivers/BSP/bsp_motor.c/h` | ✅ Done |
| **ADC + DMA** | `drivers/api/adc_driver.c/h` | ✅ Done |
| **Encoder** | `drivers/api/encoder_driver.c/h` | ✅ Done |
| **UART heartbeat** | `drivers/api/uart_driver.c/h` | ✅ Done |
| **Motor high-level** | `drivers/api/motor_driver.c/h` | ✅ Done |
| **L298N driver** | `drivers/api/l298_driver.c/h` | ✅ Done |
| **Event framework** | `middleware/event_framework/` | ✅ Done |
| **Motor controller AO** | `middleware/event_framework/motor_controller_ao.c/h` | ✅ Done |
| **Main (polling)** | `src/f4/main.c` | ✅ Done |
| **Main (event-driven)** | `src/f4/main_event_driven.c` | ✅ Done |
| **F1 watchdog** | `src/f1/watchdog_main.c` | ✅ Done |
| **Build F4** | `tools/build_f4.sh` | ✅ Done |
| **Build F1** | `tools/build_f1.sh` | ✅ Done |
| **LED test** | `tests/f4/minimal_led_test.c` | ✅ Done |

### Code partial / stub (Ver1 scope nhưng chưa cần thiết)

| Component | File(s) | Status | Ghi chú |
|-----------|---------|--------|---------|
| DMA driver | `drivers/api/dma_driver.c/h` | ⚠️ Partial | Skeleton, ADC DMA chạy qua adc_driver |
| Sysmon | `drivers/api/sysmon_driver.c/h` | ⚠️ Stub | Struct defined, init = TODO |
| Telemetry | `drivers/api/telemetry_driver.c/h` | ⚠️ Stub | Buffer defined, chưa transmit |

### Build & Validation

```
F4: 1432B text, 16400B RAM — compiles 0 warnings (-Wall -Wextra) ✅
F1:  736B text,    12B RAM — compiles 0 warnings                 ✅
```

### Documentation Ver1

| Doc | Status |
|-----|--------|
| `docs/ARCHITECTURE.md` | ✅ |
| `docs/BARE_METAL_v1_0_SPEC.md` | ✅ LOCKED |
| `docs/BUILD_SYSTEM.md` | ✅ |
| `docs/BOOT_AND_FLASH.md` | ✅ |
| `docs/ERROR_HANDLING.md` | ✅ |
| `docs/INTEGRATION_TEST.md` | ✅ |
| `docs/MOTOR_SENSOR_REDUNDANCY_IMPLEMENTATION.md` | ✅ |
| `docs/L298_QUICK_REFERENCE.md` | ✅ |
| `docs/L298_ADVANCED_C_GUIDE.md` | ✅ ← **dừng ở đây** |

---

## ĐIỂM DỪNG CUỐI CÙNG (last session)

> **Đã xong:** L298 driver hoàn chỉnh + viết L298 Advanced C Guide
> (pointer dereference, bitwise ops, static globals, encapsulation patterns).
>
> **Chưa làm tiếp:** hoàn thiện stub drivers (sysmon, telemetry) + integration test.

---

## CÒN LẠI TRONG VER1

- [ ] Integration test suite
- [ ] Hoàn thiện sysmon_driver (đang stub)
- [ ] Hoàn thiện telemetry_driver (đang stub)
- [ ] Ring buffer middleware (nếu cần cho telemetry)
