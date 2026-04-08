# Hệ Thống Điều Khiển Motor RTOS STM32 với Dự Phòng Dual-MCU

## 1. Tổng Quan Dự Án

Dự án xây dựng một hệ thống nhúng thời gian thực theo kiến trúc ECU/black-box:
- MCU chính: **STM32F407 (F4)** xử lý điều khiển motor, sensor, telemetry.
- MCU dự phòng: **STM32F103 (F1)** giám sát heartbeat và takeover khi F4 lỗi.

Mục tiêu chính:
- Điều khiển motor open-loop với an toàn (ngưỡng dòng/nhiệt độ).
- Đọc encoder feedback (RPM) nhưng chưa dùng cho vòng kín.
- Cơ chế dự phòng/failover giữa 2 MCU.
- Chuỗi build/flash hoàn chỉnh.

## 2. Phạm Vi (Scope)

### 2.1 In-Scope (v1.0 - Bare-Metal)
- **F4 Motor Control**: Open-loop PWM + safety thresholds (current/temp)
- **F4 Sensor Pipeline**: Encoder RPM read, ADC+DMA (current/temp/speed pot)
- **F4 Heartbeat**: UART pulse to F1 (0xAA every 100ms)
- **F1 Watchdog**: Heartbeat listener + failover logic
- Chuỗi build/flash (Makefile + bash scripts)
- Hardware bring-up & validation (LED + motor + encoder)

### 2.2 Out-of-Scope (giai đoạn hiện tại - v1.0)
- **RTOS**: Bare-metal superloop only (không FreeRTOS)
- **Closed-loop PID Control**: Motor chạy open-loop, encoder feedback chỉ đọc được
- **Telemetry advanced**: Chỉ heartbeat UART đơn giản, không gửi dữ liệu sensor
- Dashboard web.
- Giao diện tuning tham số nâng cao.
- Chuẩn giao tiếp thay thế UART (CAN/Ethernet).

## 3. Yêu Cầu Đề Bài / Deliverables

| # | Task | Status | Notes |
|---|------|--------|-------|
| 1 | Write PRD | ✅ | Specification complete (v1.0 locked) |
| 2 | Write SAD | ✅ | Architecture documented |
| 3 | Write SWCDD | ✅ | Component design complete |
| 4 | Define Coding Rules | ✅ | Error handling convention v1.0 |
| 5 | F4 Motor Control | ✅ | Open-loop PWM + safety thresholds |
| 6 | F1 Watchdog | ✅ | USART2 RX ISR + IWDG + failover |
| 7 | Build System | ✅ | build.sh, build_f1.sh, flash.sh, flash_f1.sh |
| 8 | Build + Flash | ✅ | Both F4 & F1 compile successfully |
| 9 | Hardware test | ⚠️ | Ready for test (pending hardware availability) |

Ghi chú:
- Trạng thái đang để trống theo đúng yêu cầu hiện tại của bạn.
- Bring-up LED được tách riêng tại `tests/test_led/` và không thay thế mục tiêu tổng của dự án.

## 4. Kiến Trúc Hệ Thống

### 4.1 MCU Chính - STM32F407 (F4)
- Điều khiển motor PWM + encoder feedback.
- Đọc sensor qua ADC/DMA.
- Gửi telemetry UART.
- Phát heartbeat sang F1.

### 4.2 MCU Dự Phòng - STM32F103 (F1)
- Lắng nghe heartbeat từ F4.
- Timeout thì kích hoạt failover.
- Duy trì trạng thái an toàn cho motor khi takeover.

### 4.3 Kết Nối F4 ↔ F1
- UART heartbeat đơn giản (ví dụ byte pulse định kỳ).
- Mục tiêu: latency thấp, dễ kiểm chứng, đủ tin cậy cho failover cơ bản.

## 5. Thiết Kế Phần Mềm (Mức Cao)

### 5.1 Tác vụ chính phía F4 (mục tiêu)
1. Motor Control Task
2. Sensor Read Task
3. Telemetry Task
4. Heartbeat Task
5. System Monitor Task

### 5.2 Logic chính phía F1 (mục tiêu)
1. Heartbeat Listener
2. Timeout Detection
3. Failover Handler

## 6. Cấu Trúc Dự Án

```
stm32-rtos-system/
├── boot/f1/                  # Startup + linker scripts (F1)
├── drivers/
│   ├── api/                  # Driver implementations (L298, UART, ADC, encoder...)
│   ├── BSP/                  # Board Support Package
│   └── system_init/          # System clock, misc peripheral init
├── hardware_vendor_code/     # CMSIS headers (minimal, used by build)
├── middleware/
│   └── event_framework/      # Active object + event system
├── src/
│   ├── f4/                   # F4 main + event-driven main
│   └── f1/                   # F1 watchdog main + UART driver
├── tests/
│   ├── f4/                   # F4 LED test
│   └── test_led/             # Standalone LED test + flash script
├── tools/                    # Build & flash scripts (bash)
├── docs/
│   ├── spec/                 # Specs (ver1-3, architecture, API, error handling)
│   ├── build/                # Build system, boot, flash, porting guides
│   ├── hardware/             # Clock config, board layout, UART, logic analyzer
│   └── driver_guide/         # L298 guides, integration test, datasheet
├── build/                    # F4 build output (.elf, .hex, .bin)
├── build_f1/                 # F1 build output
├── stm32f1.cfg               # OpenOCD config (F1)
└── stm32f4.cfg               # OpenOCD config (F4)
```

## 7. Build & Flash

### 7.1 Build F4
```bash
bash tools/build_f4.sh
```

### 7.2 Build F1
```bash
bash tools/build_f1.sh
```

### 7.3 Flash
```bash
bash tools/flash_f4.sh    # Flash F4
bash tools/flash_f1.sh    # Flash F1
```

## 8. Tiêu Chí Hoàn Thành (Definition of Done)

- Build thành công, sinh đủ `.elf/.hex/.bin`.
- Flash thành công lên đúng địa chỉ boot của target.
- Kiểm thử phần cứng đạt theo checklist (bao gồm heartbeat/failover).
- Tài liệu PRD/SAD/SWCDD và coding rules được hoàn thiện.

## 9. Lộ Trình Gần

1. Hoàn thiện tài liệu PRD/SAD/SWCDD.
2. Chốt coding rules và workflow review.
3. Hoàn thiện chuỗi build+flash ổn định.
4. Chạy hardware test theo checklist.

## 10. Trạng Thái Dự Án

- Giai đoạn hiện tại: **Bring-up và hợp nhất nền tảng build/boot**.
- Mục tiêu kế tiếp: **tăng dần lên tích hợp RTOS đầy đủ + dự phòng F1**.
- Cập nhật lần cuối: **March 24, 2026**.
