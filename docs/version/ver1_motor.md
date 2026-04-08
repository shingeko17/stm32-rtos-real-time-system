# VER1 — System Overview
### STM32F407VET6 DC Motor Control — Bare-metal Foundation

---

## 1. INPUT — Đề bài & Yêu cầu

### 1.1 Bài toán đặt ra

```
Bài toán:
  Điều khiển 2 động cơ DC có thể chạy 2 chiều,
  có giám sát dòng + nhiệt độ để bảo vệ phần cứng,
  có heartbeat để báo hiệu MCU còn sống,
  toàn bộ chạy trên bare-metal không OS.

Ràng buộc thiết kế:
  - Không dùng HAL/CubeIDE  → viết register trực tiếp
  - Không dùng RTOS          → single super-loop bare-metal
  - Tự viết startup, linker script, clock init từ đầu
  - Có MCU thứ 2 (STM32F1) làm watchdog giám sát MCU chính
```

### 1.2 Cần gì (Requirements)

| # | Yêu cầu | Chi tiết |
|---|---------|----------|
| F1 | Điều khiển tốc độ 2 motor | Duty 0..100%, cả 2 chiều |
| F2 | Đảo chiều quay | FORWARD / BACKWARD / BRAKE / STOP |
| F3 | Đọc encoder | Tốc độ thực tế [RPM] |
| F4 | Đọc dòng điện | ACS712, range 0–5A |
| F5 | Đọc nhiệt độ | NTC thermistor, range 0–100°C |
| F6 | Bảo vệ quá dòng | Tự brake nếu I > 3.5A |
| F7 | Bảo vệ quá nhiệt | Tự brake nếu T > 85°C |
| F8 | Heartbeat UART | Gửi 0xAA mỗi 100ms sang F1 watchdog |
| NF1 | Clock 168MHz | HSE 8MHz → PLL → Cortex-M4 max |
| NF2 | Control period 100ms | Super-loop 10Hz |
| NF3 | Build toolchain | GNU Make + arm-none-eabi-gcc |

### 1.3 Có gì (Available Resources)

```
Hardware:
  MCU chính : STM32F407VET6 (Cortex-M4, 168MHz, 512KB Flash, 128KB RAM)
  MCU backup: STM32F103    (watchdog — nhận heartbeat từ F407)
  Driver    : L298N H-Bridge (điều khiển 2 DC motor)
  Motor     : 2× DC motor brushed (up to 2A/motor)
  Sensor 1  : ACS712-5A   (current sensing, analog output)
  Sensor 2  : NTC Thermistor (temperature, analog output)
  Sensor 3  : Encoder 20PPR  (speed feedback, pulse output)
  Power     : External 12V/5V PSU

Software (tự viết từ đầu, không dùng HAL):
  stm.s              → Startup assembly (ARM Thumb)
  stm.ld             → Linker script (Flash/SRAM/CCM layout)
  system_stm32f4xx.c → Clock init (SystemInit, PLL config)
  bsp_motor.c/h      → L298N GPIO + TIM1 PWM driver
  adc_driver.c/h     → ADC1 + DMA continuous sensor read
  encoder_driver.c/h → TIM3 input capture → RPM
  uart_driver.c/h    → USART1 heartbeat TX
  motor_sensor_main.c→ Main application loop
  stm32f1_watchdog_main.c → F1 backup watchdog firmware
```

---

## 2. OUTPUT — Hệ thống giải quyết cái gì

### 2.1 Output vật lý

```
┌──────────────────────────────────────────────────────────────┐
│ OUTPUT 1: PWM → L298N → Motor A + Motor B                    │
│   Signal : TIM1_CH3 (PE13), TIM1_CH4 (PE14)                 │
│   Freq   : 1kHz PWM                                          │
│   Duty   : 0..100% → 0..100% tốc độ motor                   │
│   Chiều  : PD0/PD1 (Motor A IN1/IN2)                         │
│             PD2/PD3 (Motor B IN3/IN4)                         │
├──────────────────────────────────────────────────────────────┤
│ OUTPUT 2: UART Heartbeat → STM32F1 Watchdog                  │
│   Byte   : 0xAA mỗi 100ms                                    │
│   UART1  : PA9(TX), PA10(RX), 115200 baud 8N1               │
│   Hành động F1 khi mất heartbeat > 200ms:                    │
│     → GPIO brake motor  → LED báo lỗi PC13                   │
├──────────────────────────────────────────────────────────────┤
│ OUTPUT 3: Protection action (software)                        │
│   current_ma > 3500 mA → Motor_All_Brake()                   │
│   temp_c    > 85  °C  → Motor_All_Brake()                    │
└──────────────────────────────────────────────────────────────┘
```

### 2.2 Logic điều khiển (mỗi 100ms)

```
[ADC_ReadSpeed()]   → speed_request_rpm → speed_cmd (0..100%)
[ADC_ReadCurrent()] → current_ma
[ADC_ReadTemp()]    → temp_c
[Encoder_GetRPM()]  → measured_rpm (lưu, chưa dùng trong loop)

IF/ELSE đơn giản (implementation ban đầu):
  IF (current_ma ≤ 3500) AND (temp_c ≤ 85):
      Motor_A_Run(speed_cmd)
      Motor_B_Run(speed_cmd)
  ELSE:
      Motor_All_Brake()
  UART_SendHeartbeat()   ← 0xAA đến F1

→ Nên nâng cấp thành FSM (xem section 3.6):
  INIT → RUNNING → FAULT → RECOVERY → RUNNING
  Có cooldown, retry limit, hysteresis, lockout protection.
```

---

## 3. Ở GIỮA — Các hệ liên quan & Characteristics

### 3.1 HỆ CƠ KHÍ (Mechanical)

```
Đối tượng: Động cơ DC brushed (brushed DC motor)

Mô hình điện - cơ:
  V  =  L·di/dt + R·i + Ke·ω     (mạch điện phần ứng)
  J·dω/dt = Kt·i - B·ω - TL      (động lực học trục)

  V   = điện áp đặt vào [V]
  i   = dòng điện [A]
  ω   = tốc độ góc [rad/s]
  Kt  = hằng số mô-men [N·m/A]
  Ke  = hằng số sức phản điện [V·s/rad]
  TL  = tải ngoài [N·m]
  J   = mô-men quán tính [kg·m²]

Điều khiển ver1:
  Voltage control qua L298N (duty cycle ↔ điện áp)
  OPEN-LOOP: tốc độ ~ duty, khi tải tăng tốc độ giảm (không bù)

Encoder:
  20 PPR (pulses per revolution)
  TIM3 CH1 (PA6) input capture mode
  RPM = (pulse_count / 20) × 600  [mỗi 100ms window]
  Nhược điểm: 1 kênh → không biết chiều quay
```

### 3.2 HỆ ĐIỆN (Electrical)

```
L298N H-Bridge:
  ┌───────────────────────────────────────────────┐
  │ Vcc max       : 46V                           │
  │ I max         : 2A/channel, 4A peak           │
  │ Voltage drop  : ~1.5V (motor nhận ~10.5V/12V) │
  │ Efficiency    : ~87% (kém do bipolar topology) │
  │ PWM freq max  : 40kHz (dùng 1kHz trong ver1)  │
  │ Không có      : current feedback, thermal prot│
  └───────────────────────────────────────────────┘

ACS712-5A (Current Sensor):
  ┌───────────────────────────────────────────────┐
  │ Loại          : Hall-effect                   │
  │ Range         : ±5A                           │
  │ Output        : 2.5V @ 0A, 185mV/A           │
  │ Accuracy      : ±1.5% full scale              │
  │ Pin           : PA0 → ADC1_CH0               │
  └───────────────────────────────────────────────┘

NTC Thermistor (Temperature Sensor):
  ┌───────────────────────────────────────────────┐
  │ Loại          : Negative Temperature Coeff.   │
  │ Mạch          : Voltage divider + ADC         │
  │ Range         : 0–100°C                       │
  │ Accuracy      : ±5°C (linearization đơn giản) │
  │ Pin           : PA2 → ADC1_CH2               │
  │ Nhược điểm   : Nonlinear, dễ nhiễu EMI       │
  └───────────────────────────────────────────────┘

Power:
  12V external → L298N VCC → Motor winding
   5V external → STM32 Vdd, sensor supply
```

### 3.3 SENSOR CALIBRATION & SIGNAL CONDITIONING

```
Tại sao phần Điện lại có “Toán”?
═════════════════════════════
  Sensor trả về điện áp (analog) hoặc pulse (digital).
  MCU không dùng trực tiếp giá trị rời rạc đó được.
  Cần chuyển đổi: Vật lý → Điện → Số → Đơn vị vật lý.

  Đây là Signal Conditioning — thuộc thiết kế hệ điện,
  không phải giải thuật điều khiển.
  Nếu không có bước này, phần cứng chỉ là linh kiện rời rạc,
  không cung cấp thông tin hữu ích cho bộ não điều khiển.

═══════════════════════════════════════════════════════════════
SIGNAL CONDITIONING PIPELINE
═══════════════════════════════════════════════════════════════

  Physical     Sensor      ADC         Scaling      Calibration    Usable
  Quantity  →  Element  →  12-bit   →  raw→unit  →  offset+gain →  Value
  ─────────────────────────────────────────────────────────────
  Current [A] → ACS712    → 0..4095  → mA          → zero+gain    → current_ma
  Temp [°C]   → NTC       → 0..4095  → °C (approx) → LUT/curve    → temp_c
  Speed [RPM] → Encoder   → pulses   → count/100ms → PPR factor   → rpm
  Setpoint    → Potentiom.→ 0..4095  → %           → linear       → speed_cmd

═══════════════════════════════════════════════════════════════
CALIBRATION TỪNG SENSOR (từng sensor)
═══════════════════════════════════════════════════════════════

  1. ACS712-5A (Current Sensor) — Linear, cần zero offset
  ──────────────────────────────────────────────────
    Mô hình: V_out = V_zero + Sensitivity × I
    V_zero      = 2.5V (datasheet) → ADC_zero ≈ 2048
    Sensitivity = 185 mV/A
    ADC_to_mA   = (adc_raw - ADC_zero) × (3300 / 4095) / 0.185 × 1000

    Calibration procedure:
      Bước 1: Đo ADC khi I = 0A (motor off), lấy trung bình 100 mẫu
             → ADC_zero_actual (có thể lệch vài LSB so với 2048)
      Bước 2: Đo ADC với dòng biết trước (dùng ampe kếp)
             → Tính Sensitivity_actual
      Bước 3: Hồi quy tuyến tính: y = ax + b
             y = dòng thực (ampe kếp)
             x = dòng tính từ ADC
             → a ≈ 1.0, b ≈ offset (mà không bằng 0)

    Error budget:
      ± ADC quantization   : 3300/4095 = 0.8 mV/LSB → ±4.3 mA
      ± ACS712 accuracy    : ±1.5% FS = ±75 mA
      ± Zero offset drift  : ±20 mA (ảnh hưởng nhiệt độ)
      → Tổng sai số ước tính: ±100 mA
      → Overcurrent trip 3500 mA có margin đủ (±2.9%)

  2. NTC Thermistor — Nonlinear, cần linearization
  ────────────────────────────────────────────────
    Vấn đề: NTC có đường cong R-T exponential (non-linear)
           ADC → Voltage → Resistance → Temperature: 3 bước, 2 non-linear

    Giải pháp Ver1 (simple):
      Option A: Lookup Table (LUT)
        T_table[] = { 0, 5, 10, ..., 95, 100 }  (đo thực tế)
        ADC_table[] = { adc@0°C, adc@5°C, ... }  (tương ứng)
        Nội suy tuyến tính giữa 2 điểm gần nhất

      Option B: Piecewise Linear (2-3 đoạn)
        Khoảng 25-85°C (vùng quan tâm): xấp xỉ tuyến tính
        y = a×x + b  (hồi quy tuyến tính cho từng đoạn)

    Calibration procedure:
      Bước 1: Đặt NTC trong nước, đo ADC tại nhiều nhiệt độ biết trước
             (nhiệt kế tham chiếu)
      Bước 2: Lập bảng {ADC, T_actual}
      Bước 3: Fit hồi quy: T_measured vs T_actual
             y = ax + b  (giống slide: y = 0.97x + 1.71)
      Bước 4: Sai số cực đại = max |T_measured - T_actual|

    Error budget:
      ± NTC tolerance      : ±5% R → ±2-3°C
      ± ADC quantization   : ±0.5°C
      ± Linearization error: ±2°C (LUT 20 điểm) hoặc ±5°C (linear)
      → Tổng sai số ước tính: ±5°C
      → Overtemp trip 85°C, hysteresis 5°C → margin vừa đủ
      → Ver2 dùng TMP112 digital (±0.5°C) → loại bỏ vấn đề này

  3. Encoder 20PPR — Discrete, cần windowing
  ───────────────────────────────────────────────
    Mô hình: RPM = (pulse_count / PPR) × (60 / T_window)
    PPR = 20, T_window = 100ms
    RPM = pulses / 20 × 600

    Vấn đề lượng tử hóa:
      Resolution = 600/20 = 30 RPM/step
      → Tốc độ < 30 RPM không phân biệt được (0 hoặc 30)
      → Đây là giới hạn của phần cứng, không khắc phục bằng software
      → Ver2 dùng 500PPR × 4 = 2000 cnt/rev → resolution 0.3 RPM

    Calibration:
      Đo với tachometer tham chiếu → so sánh
      Sai số chủ yếu do: PPR không đều (cơ khí), noise edge

═══════════════════════════════════════════════════════════════
TỔNG SAI SỐ HỆ THỐNG (Error Budget Summary)
═══════════════════════════════════════════════════════════════

  Sensor      │ Range       │ Error       │ % FS    │ Đủ cho Ver1?
  ────────────│─────────────│─────────────│─────────│───────────
  ACS712      │ 0-5000 mA   │ ±100 mA     │ ±2%     │ ✓ (protection)
  NTC         │ 0-100 °C    │ ±5 °C       │ ±5%     │ ✓ (margin 15°C)
  Encoder     │ 0-500 RPM   │ ±30 RPM     │ ±6%     │ ✗ (chưa dùng CL)
  Potentiom.  │ 0-100 %     │ ±1 %        │ ±1%     │ ✓ (setpoint)

  Kết luận:
  → Ver1 open-loop: sai số sensor đủ dùng cho protection (OC/OT)
  → Nhưng không đủ cho closed-loop control (±30 RPM encoder)
  → Đây là động lực nâng cấp sensor trong Ver2

```

### 3.4 HỆ DRIVER / BSP

```
BSP Motor (bsp_motor.h / bsp_motor.c):
  ┌───────────────────────────────────────────────┐
  │ TIM1 APB2 @ 84MHz                             │
  │ PSC = 83  → fTIM = 1MHz                       │
  │ ARR = 999 → fPWM = 1kHz                       │
  │ CCR = duty (0..999 = 0..100%)                 │
  │                                               │
  │ Motor A: IN1=PD0, IN2=PD1, EN=PE13(TIM1_CH3) │
  │ Motor B: IN3=PD2, IN4=PD3, EN=PE14(TIM1_CH4) │
  │                                               │
  │ IN1│IN2 → Chế độ                             │
  │  0 │ 0  → STOP    (coasting)                 │
  │  1 │ 0  → FORWARD                            │
  │  0 │ 1  → BACKWARD                           │
  │  1 │ 1  → BRAKE   (dynamic braking)          │
  └───────────────────────────────────────────────┘

ADC Driver (adc_driver.h / adc_driver.c):
  ┌───────────────────────────────────────────────┐
  │ ADC1 + DMA2 Stream0, circular mode            │
  │ 3 channels: CH0(current) CH1(speed) CH2(temp) │
  │ 12-bit, scan + continuous, ~1kHz/channel      │
  │ DMA buffer[3] → CPU đọc bất kỳ lúc nào       │
  │                                               │
  │ ADC_ReadCurrent() → 0–5000 mA                │
  │ ADC_ReadSpeed()   → 0–500 RPM                │
  │ ADC_ReadTemp()    → 0–100 °C                 │
  └───────────────────────────────────────────────┘

Encoder Driver (encoder_driver.h / encoder_driver.c):
  ┌───────────────────────────────────────────────┐
  │ TIM3 CH1 (PA6) input capture, rising edge     │
  │ PPR = 20, sampling window = 100ms             │
  │ RPM = pulses / 20 × 600                       │
  │                                               │
  │ Encoder_GetRPM()    → uint16_t RPM            │
  │ Encoder_IsStuck()   → 1 nếu không có pulse   │
  └───────────────────────────────────────────────┘

UART Driver (uart_driver.h / uart_driver.c):
  ┌───────────────────────────────────────────────┐
  │ USART1: PA9(TX), PA10(RX)                     │
  │ Baud: 115200, 8N1                             │
  │ UART_SendHeartbeat() → gửi 0xAA              │
  └───────────────────────────────────────────────┘
```

### 3.5 HỆ FIRMWARE

```
Platform Layer:
  ┌───────────────────────────────────────────────┐
  │ stm.s  (startup assembly)                     │
  │   1. Copy .data: Flash → SRAM                 │
  │   2. Zero .bss                                │
  │   3. Gọi SystemInit() → main()               │
  │   4. Vector table 82 IRQ (F407 full)          │
  │   5. Default_Handler: infinite loop           │
  │                                               │
  │ stm.ld  (linker script)                       │
  │   FLASH : 512KB @ 0x08000000                 │
  │   SRAM  : 128KB @ 0x20000000                 │
  │   CCM   :  64KB @ 0x10000000 (CPU only)      │
  │                                               │
  │ system_stm32f4xx.c  (clock init)             │
  │   HSE 8MHz / PLLM=8 → VCO input = 1MHz      │
  │   × PLLN=336 → VCO = 336MHz                  │
  │   / PLLP=2   → SYSCLK = 168MHz               │
  │   AHB /1=168MHz, APB2 /2=84MHz, APB1 /4=42MHz│
  │   Flash latency = 5 wait states              │
  └───────────────────────────────────────────────┘

Application Layer:
  ┌───────────────────────────────────────────────┐
  │ motor_sensor_main.c                           │
  │   Architecture : Super-loop bare-metal        │
  │   Period       : 100ms (SysTick 1kHz counter) │
  │   Không có RTOS, không có scheduler           │
  └───────────────────────────────────────────────┘

F1 Watchdog Firmware:
  ┌───────────────────────────────────────────────┐
  │ stm32f1_watchdog_main.c                       │
  │   Nhận 0xAA từ F407 mỗi 100ms               │
  │   Timeout > 200ms → F407 crash detected       │
  │   Hành động: GPIO brake L298N, LED on (PC13) │
  │   Hoàn toàn độc lập với F407                 │
  └───────────────────────────────────────────────┘
```

### 3.6 SYSTEM STATE MACHINE (FSM)

```
Ver1 dù open-loop vẫn CẦN FSM.
Lý do: IF/ELSE thuần không trả lời được:
  - Sau fault, có tự restart không? Sau bao lâu?
  - Fault liên tục xảy ra → có lockout không?
  - Power-on → chạy motor ngay hay chờ init xong?

═══════════════════════════════════════════════════════════════
  STATE DIAGRAM (4 states)
═══════════════════════════════════════════════════════════════

  ┌──────────┐    init done     ┌──────────┐
  │   INIT   │─────────────────→│  RUNNING  │
  │ (power-on│                  │ (normal)  │←─────────────┐
  └──────────┘                  └────┬──────┘              │
                                     │                     │
                           overcurrent│                    │ cooldown
                           overtemp   │                    │ elapsed
                           heartbeat  │                    │ + safe
                             fail     │                    │
                                     ▼                     │
                                ┌──────────┐    timer     ┌┴─────────┐
                                │  FAULT   │─────────────→│ RECOVERY │
                                │ (protect)│  cooldown    │ (waiting)│
                                └──────────┘  start       └──────────┘
                                     ▲                         │
                                     │    still unsafe         │
                                     └────────────────────────┘

═══════════════════════════════════════════════════════════════
  TRANSITION TABLE
═══════════════════════════════════════════════════════════════

  From      │ To        │ Guard (điều kiện)          │ Action
  ──────────│───────────│────────────────────────────│──────────────────────
  INIT      │ RUNNING   │ Clock + GPIO + ADC + DMA   │ Motor_Stop() (safe)
            │           │ + UART + Encoder init OK   │ heartbeat_start()
  ──────────│───────────│────────────────────────────│──────────────────────
  RUNNING   │ FAULT     │ current_ma > 3500          │ Motor_All_Brake()
            │           │ OR temp_c > 85             │ fault_code = OC/OT
            │           │ OR heartbeat timeout(F1)   │ fault_timestamp = tick
  ──────────│───────────│────────────────────────────│──────────────────────
  FAULT     │ RECOVERY  │ cooldown_ms elapsed        │ fault_count++
            │           │ (e.g., 2000ms)             │ reset cooldown timer
  ──────────│───────────│────────────────────────────│──────────────────────
  RECOVERY  │ RUNNING   │ current_ma ≤ 3500          │ Motor ramp-up (soft)
            │           │ AND temp_c ≤ 80 (hyst 5°C) │ clear fault_code
  ──────────│───────────│────────────────────────────│──────────────────────
  RECOVERY  │ FAULT     │ still unsafe after check   │ Motor_All_Brake()
            │           │ OR fault_count > 3 → LOCK  │ lockout nếu quá 3 lần
  ──────────│───────────│────────────────────────────│──────────────────────

═══════════════════════════════════════════════════════════════
  IMPLEMENTATION (pseudo-code cho super-loop)
═══════════════════════════════════════════════════════════════

  typedef enum {
      SYS_STATE_INIT,
      SYS_STATE_RUNNING,
      SYS_STATE_FAULT,
      SYS_STATE_RECOVERY
  } SystemState_t;

  typedef struct {
      SystemState_t state;
      uint8_t  fault_code;       // 0=none, 1=OC, 2=OT, 3=heartbeat
      uint8_t  fault_count;      // số lần fault liên tiếp
      uint32_t fault_timestamp;  // tick khi vào FAULT
      uint32_t cooldown_ms;      // thời gian chờ trước RECOVERY
  } SystemFSM_t;

  // Trong main loop (mỗi 100ms):
  switch (fsm.state) {
      case SYS_STATE_INIT:
          // init peripherals...
          fsm.state = SYS_STATE_RUNNING;
          break;

      case SYS_STATE_RUNNING:
          if (current_ma > 3500 || temp_c > 85) {
              Motor_All_Brake();
              fsm.fault_code = (current_ma > 3500) ? 1 : 2;
              fsm.fault_timestamp = tick;
              fsm.state = SYS_STATE_FAULT;
          } else {
              Motor_A_Run(speed_cmd);
              Motor_B_Run(speed_cmd);
          }
          UART_SendHeartbeat();
          break;

      case SYS_STATE_FAULT:
          Motor_All_Brake();  // giữ brake
          if (tick - fsm.fault_timestamp >= fsm.cooldown_ms) {
              fsm.fault_count++;
              fsm.state = SYS_STATE_RECOVERY;
          }
          UART_SendHeartbeat();  // vẫn gửi heartbeat
          break;

      case SYS_STATE_RECOVERY:
          if (fsm.fault_count > 3) {
              // LOCKOUT — cần power cycle hoặc manual reset
              Motor_All_Brake();
              break;
          }
          if (current_ma <= 3500 && temp_c <= 80) {
              fsm.fault_code = 0;
              fsm.state = SYS_STATE_RUNNING;
          } else {
              fsm.state = SYS_STATE_FAULT;
              fsm.fault_timestamp = tick;
          }
          break;
  }

```

### 3.7 SAFETY ANALYSIS (FMEA)

```
═══════════════════════════════════════════════════════════════
  FAILURE MODE & EFFECTS ANALYSIS — Ver1
═══════════════════════════════════════════════════════════════

  ID │ Failure Mode           │ Effect              │ Sev│ Det│ Action / Mitigation
  ───│────────────────────────│─────────────────────│────│────│─────────────────────────
  F1 │ ACS712 output stuck    │ OC not detected     │ H  │ M  │ Plausibility: if motor ON
     │ at 2.5V (zero)         │ motor may burn      │    │    │ but ADC=2048 → fault
  ───│────────────────────────│─────────────────────│────│────│─────────────────────────
  F2 │ NTC open circuit       │ ADC reads 0 or 4095 │ H  │ H  │ Range check: if ADC < 50
     │ (wire broken)          │ temp unknown         │    │    │ or > 4000 → sensor fault
  ───│────────────────────────│─────────────────────│────│────│─────────────────────────
  F3 │ NTC short circuit      │ ADC reads ~0        │ H  │ H  │ Same range check
     │                        │ fake cold reading    │    │    │ → trigger fault state
  ───│────────────────────────│─────────────────────│────│────│─────────────────────────
  F4 │ Encoder no pulses      │ RPM = 0 always      │ M  │ H  │ Encoder_IsStuck(): if
     │ (wire broken/stuck)    │ can't detect speed   │    │    │ motor ON + RPM=0 → warn
  ───│────────────────────────│─────────────────────│────│────│─────────────────────────
  F5 │ L298N one channel dead │ One motor stops     │ H  │ L  │ Compare duty vs encoder:
     │ (driver IC failure)    │ robot turns instead  │    │    │ if duty>50% but RPM=0→F
  ───│────────────────────────│─────────────────────│────│────│─────────────────────────
  F6 │ UART TX stuck          │ F1 timeout → brake  │ M  │ H  │ F1 watchdog handles this
     │ (F407 frozen)          │ system safe-stop     │    │    │ by design (200ms timeout)
  ───│────────────────────────│─────────────────────│────│────│─────────────────────────
  F7 │ ADC DMA stopped        │ All sensor data     │ H  │ M  │ Watchdog: if DMA TC count
     │ (DMA config error)     │ stale/frozen         │    │    │ not incrementing → fault
  ───│────────────────────────│─────────────────────│────│────│─────────────────────────
  F8 │ SysTick ISR disabled   │ No timing, no       │ H  │ L  │ F1 watchdog catches this
     │ (priority bug)         │ control, no HB       │    │    │ (heartbeat stops)
  ───│────────────────────────│─────────────────────│────│────│─────────────────────────
  F9 │ 5V rail brownout       │ MCU reset, motor    │ H  │ M  │ BOD (Brown-out Detect)
     │                        │ coasting (no brake)  │    │    │ → POR → re-init → safe
  ───│────────────────────────│─────────────────────│────│────│─────────────────────────
  F10│ 12V rail spike         │ Motor overspeed,    │ H  │ L  │ TVS diode on 12V input
     │ (motor back-EMF)       │ L298N damage         │    │    │ + flyback diode on motor
  ───│────────────────────────│─────────────────────│────│────│─────────────────────────

  Severity: H=High, M=Medium, L=Low
  Detectability: H=Easy detect, M=Moderate, L=Hard to detect

  Phân tích coverage:
    Hardware mitigation   : F1 watchdog (F6,F8), BOD (F9), TVS (F10)
    Software mitigation   : Range check (F2,F3), plausibility (F1,F5),
                           stale data detect (F4,F7)
    Không có mitigation   : F5 (driver IC fail) → cần hardware feedback
                           F10 (voltage spike) → cần TVS on PCB

  [NOTE: FMEA đầy đủ cần RPN (Risk Priority Number) = Sev × Occ × Det
   và action plan cho mọi RPN > threshold. Bảng trên là simplified.]
```

### 3.8 PROTOCOL SPECIFICATION — UART Heartbeat

```
═══════════════════════════════════════════════════════════════
  UART HEARTBEAT PROTOCOL (F407 → F1)
═══════════════════════════════════════════════════════════════

  Physical layer:
    Interface : USART1 (PA9 TX, PA10 RX)
    Baud      : 115200
    Format    : 8N1 (8 data, no parity, 1 stop)
    Level     : 3.3V TTL (direct connect, same GND)

  Message format:
    ┌──────┬──────┬──────────┬──────────┬──────┐
    │ Byte │ Name │ Value    │ Type     │ Note │
    │──────│──────│──────────│──────────│──────│
    │  0   │ SYNC │ 0xAA     │ fixed    │ magic│
    └──────┴──────┴──────────┴──────────┴──────┘

    Hiện tại: single byte 0xAA (simplest possible)
    Không CRC, không sequence number, không payload

  Timing:
    TX period    : 100ms (10Hz)
    TX duration  : 1 byte / 115200 baud ≈ 87μs
    F1 timeout   : 200ms (2× period)
    Jitter       : < 10ms (super-loop, non-deterministic)

  F1 Receiver behavior:
    ┌──────────────────────────────────────────────────┐
    │ Event                │ F1 Action                 │
    │──────────────────────│───────────────────────────│
    │ Received 0xAA        │ Reset watchdog timer      │
    │ Received != 0xAA     │ Ignore (noise/garbage)    │
    │ No byte > 200ms      │ → F407 crash assumed      │
    │                      │ → GPIO brake L298N        │
    │                      │ → LED ON (PC13)           │
    │ Byte received again  │ → Clear brake? (TBD)      │
    │ after timeout        │ → Or require power cycle  │
    └──────────────────────────────────────────────────┘

  Limitations:
    ✗ No authentication → any 0xAA on wire resets timer
    ✗ No state info → F1 doesn't know if F407 is in FAULT/RUN
    ✗ No bidirectional → F407 doesn't know if F1 is alive
    → Ver2 dùng FDCAN với message ID + status payload → giải quyết

  [NOTE: Nếu nâng cấp, có thể thêm:
   - 2-byte frame: [0xAA, state_code] + simple checksum
   - Bidirectional: F1 gửi ACK về F407
   - Nhưng Ver1 deliberately simple — đủ dùng]
```

---

## 4. ARCHITECTURE & DESIGN PHILOSOPHY (Open-loop / Bare-metal)

### 4.1 Core Principles

```
Nguyên tắc thiết kế:
  1. Timer-driven system    → mọi hoạt động gắn với timer, không dùng delay
  2. Non-blocking execution → main loop không bao giờ bị chặn
  3. Event-driven data flow → interrupt/DMA báo khi data sẵn sàng
  4. Pipeline processing    → acquire → process → transmit, tách tầng rõ ràng

System type (Ver1):
  ✦ OPEN-LOOP (Data Acquisition / Voltage Control)
  ✦ Chưa có feedback bù → tải thay đổi, tốc độ thay đổi
  ✦ Phù hợp: sensor logging, IoT gateway, prototype ban đầu
```

### 4.2 Execution Flow (Concise)

```
SysTick 1kHz (1ms tick)
  ↓
Mỗi 100ms trigger control cycle (10Hz)
  ↓
ADC1 + DMA circular cập nhật liên tục 3 kênh
  ↓
Main loop:
  - đọc sensor đã scale
  - check protection (OC/OT)
  - update PWM motor
  - gửi heartbeat 0xAA cho F1

Nguyên tắc: không delay, không blocking, ISR chỉ set flag.
```

### 4.3 Timing Summary

```
Control period: 100ms (10Hz)
ADC: DMA continuous (CPU không polling)
Heartbeat: 0xAA mỗi 100ms, timeout phía F1 = 200ms

Encoder 20PPR với cửa sổ 100ms:
  RPM = pulses / 20 × 600
  Resolution xấp xỉ 30 RPM/step

Kết luận timing:
  - Ver1 đủ cho open-loop + protection
  - Không phù hợp cho closed-loop tốc độ cao
```

### 4.4 Open-loop Characteristics

```
Ưu điểm:
  ✓ Kiến trúc đơn giản, dễ debug
  ✓ Phù hợp học register-level + DMA + timer
  ✓ Có lớp bảo vệ cơ bản (OC/OT + watchdog F1)

Giới hạn:
  ✗ Tốc độ phụ thuộc tải, không có bù closed-loop
  ✗ Latency 100ms còn lớn cho control chính xác
  ✗ Encoder 20PPR chưa đủ độ phân giải cho regulation mượt
```

---

## 5. Thông số tổng hợp

> **Lưu ý:** Architecture type = Open-loop, Pipeline processing, Timer-driven

| Thông số | Giá trị |
|----------|---------|
| MCU chính | STM32F407VET6, Cortex-M4, 168MHz |
| MCU backup | STM32F103, watchdog role |
| Flash / RAM | 512KB / 128KB SRAM + 64KB CCM |
| Motor driver | L298N H-Bridge |
| PWM freq | 1kHz (TIM1_CH3/CH4) |
| PWM resolution | 10-bit (0..999 counts) |
| Max motor current | 2A/channel (L298N hardware limit) |
| Overcurrent trip | 3.5A (software, 100ms latency) |
| Overtemp trip | 85°C (software, 100ms latency) |
| Current sensor | ACS712-5A, Hall-effect, ±5A |
| Temp sensor | NTC thermistor, 0–100°C |
| Speed sensor | Encoder 20PPR, TIM3 input capture |
| ADC | 12-bit, DMA circular, 3 channel |
| Control type | **Open-loop** (voltage → duty) |
| Control period | 100ms (10Hz super-loop) |
| Heartbeat | UART 115200 baud, 0xAA mỗi 100ms |
| Build tool | GNU Make + arm-none-eabi-gcc |
| Debug | ST-Link + OpenOCD |
| Code style | Register-level, zero HAL dependency |

---

## 6. Điểm mạnh & Giới hạn

### Điểm mạnh
```
✓ Register-level hoàn toàn → hiểu hardware từ gốc
✓ Dual-MCU safety architecture (F407 + F1 watchdog)
✓ Dual-layer protection: software (OC/OT) + hardware (heartbeat timeout)
✓ Startup + linker script tự viết → hiểu memory layout đầy đủ
✓ DMA continuous ADC → CPU không block khi đọc sensor
✓ F1 watchdog hoàn toàn độc lập → fail-safe khi F407 crash
✓ Timer-driven architecture → non-blocking, event-driven
✓ Pipeline concept: acquire → process → transmit rõ ràng
✓ Sensor unified interface → dễ thêm sensor mới
✓ Không dùng delay() → main loop luôn responsive
```

### Giới hạn → Dẫn đến Ver2
```
✗ Open-loop control
    Tải thay đổi → tốc độ thay đổi, không có bù

✗ PWM 1kHz + TIM standard
    Resolution thấp, không đủ cho BLDC/FOC

✗ ADC không đồng bộ với PWM
    Sample có thể trúng lúc MOSFET switching → nhiễu

✗ Overcurrent bảo vệ bằng software (100ms)
    Motor có thể cháy trong vài ms trước khi software phản ứng

✗ Encoder 20PPR, 1 kênh
    Resolution thấp, không biết chiều, sai số lớn ở RPM thấp

✗ Super-loop 100ms
    Latency quá lớn cho control nhanh, không thể scale lên
```

---

## 7. Future Upgrades (Từ Open-loop → Closed-loop)

```
Upgrade path từ Ver1 → Ver2:

  1. Add closed-loop feedback
     Open-loop voltage control → PID / FOC closed-loop
     Encoder feedback tham gia vào control loop thực sự

  2. Multi-rate scheduler
     Super-loop 10Hz → multi-rate ISR (20kHz + 1kHz + 100Hz + 10Hz)
     Priority-based interrupt chain

  3. Double buffering everywhere
     Simple buffer → double buffer cho ADC, encoder, CAN
     Tránh race condition hoàn toàn

  4. Event-driven architecture
     Polling-based → event flag + callback pattern
     Gần hơn với RTOS model (task + queue)

  5. Timestamp synchronization
     Không có timestamp → synchronized timestamp cho sensor fusion
     Quan trọng khi tích hợp nhiều sensor khác tần số

  6. RTOS integration (Ver3+)
     Bare-metal scheduler → FreeRTOS task + queue + mutex
     Deadline guarantee, priority inversion handling

  7. Hardware overcurrent protection
     Software 100ms → hardware comparator < 100ns (COMP → HRTIM FLT)
```

---

## 8. Kết luận

> **Ver1 chốt ở phạm vi nền tảng bare-metal open-loop, ưu tiên độ rõ ràng và an toàn cơ bản.**
> Tài liệu này tập trung vào những phần cốt lõi đã triển khai thực tế: kiến trúc timer-driven,
> ADC+DMA không blocking, điều khiển PWM qua L298N, bảo vệ OC/OT, và watchdog F1 qua heartbeat UART.
>
> Ver1 **đủ để chạy ổn định ở mức prototype học thuật/kỹ thuật**, nhưng chưa phải closed-loop control:
> tốc độ vẫn phụ thuộc tải, encoder 20PPR còn thô, và chu kỳ 100ms còn chậm cho điều khiển chính xác cao.
>
> Hướng đi Ver2 là giữ lại nền tảng hiện tại và nâng cấp theo 3 trục chính:
> closed-loop control, lịch đa tốc độ (multi-rate), và tăng lớp bảo vệ phần cứng thời gian thực.
