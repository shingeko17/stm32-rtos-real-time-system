# Hệ Thống Điều Khiển Motor RTOS STM32 với Dự Phòng Dual-MCU

## 1. Tổng Quan Dự Án

Dự án xây dựng một hệ thống nhúng thời gian thực theo kiến trúc ECU/black-box:
- MCU chính: **STM32F407 (F4)** xử lý điều khiển motor, sensor, telemetry.
- MCU dự phòng: **STM32F103 (F1)** giám sát heartbeat và takeover khi F4 lỗi.

Mục tiêu chính:
- Kiến trúc RTOS đa tác vụ có tính xác định thời gian.
- Điều khiển motor có phản hồi encoder.
- Thu thập và truyền telemetry.
- Cơ chế dự phòng/failover giữa 2 MCU.

## 2. Phạm Vi (Scope)

### 2.1 In-Scope
- Firmware F4 cho control loop, sensor pipeline, telemetry, heartbeat.
- Firmware F1 cho watchdog/failover.
- Chuỗi build/flash và kiểm thử phần cứng.

### 2.2 Out-of-Scope (giai đoạn hiện tại)
- Dashboard web.
- Giao diện tuning tham số nâng cao.
- Chuẩn giao tiếp thay thế UART (CAN/Ethernet).

## 3. Yêu Cầu Đề Bài / Deliverables

| # | Task | Status |
|---|------|--------|
| 1 | Write PRD | ☐ |
| 2 | Write SAD | ☐ |
| 3 | Write SWCDD | ☐ |
| 4 | Define Coding Rules | ☐ |
| 8 | Build + Flash | ☐ |
| 9 | Hardware test | ☐ |

Ghi chú:
- Trạng thái đang để trống theo đúng yêu cầu hiện tại của bạn.
- Bring-up LED được tách riêng tại `linkled.md` và không thay thế mục tiêu tổng của dự án.

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

## 6. Cấu Trúc Mã Nguồn

- `application/`: code ứng dụng.
- `BOOT/`: startup/linker script.
- `drivers/`: BSP + API + system init.
- `hardware_vendor_code/`: CMSIS/vendor headers.
- `STM32CubeF4_source/`: bộ nguồn tham chiếu STM32CubeF4.

## 7. Build & Flash

### 7.1 Build trên Windows (khuyến nghị)
```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1 -Rebuild
```

### 7.2 Build bằng Bash
```bash
./build.sh
```

### 7.3 Flash
```powershell
powershell -ExecutionPolicy Bypass -File .\flash_latest.ps1
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
