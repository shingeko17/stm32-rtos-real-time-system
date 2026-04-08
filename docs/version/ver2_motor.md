# VER2 — System Overview
### STM32G474RET6 FOC Motor Control — Closed-loop Embedded Foundation

---

## 1. INPUT — Đề bài & Yêu cầu

### 1.1 Bài toán đặt ra

```
Bài toán:
  Điều khiển động cơ 3-pha theo giải thuật FOC (Field-Oriented Control),
  có vòng lặp kép khép kín (dòng điện + tốc độ), giám sát encoder 2 bánh,
  phát hiện trượt bánh + lệch hướng, đọc nhiệt độ qua I2C,
  nhận lệnh tốc độ và gửi telemetry qua FDCAN,
  bảo vệ phần cứng bằng overcurrent hardware path (không qua CPU).

Ràng buộc thiết kế:
  - Không dùng HAL/CubeIDE → viết thanh ghi trực tiếp
  - Không dùng RTOS         → multi-rate bare-metal interrupt + poll scheduler
  - Tự viết startup, linker script, clock tree từ đầu
  - FOC inner loop @20kHz phải hoàn thành < 30μs
  - Overcurrent phải ngắt PWM < 100ns (hardware COMP1 → HRTIM FLT1)
  - Toàn bộ tính toán float dùng FPU cứng (Cortex-M4 FPv4-SP)
```

### 1.2 Cần gì (Requirements)

| #   | Yêu cầu                       | Chi tiết                                             |
|-----|-------------------------------|------------------------------------------------------|
| F1  | FOC 3-phase current control   | Clarke/Park/PI/SVM @ 20kHz inner loop                |
| F2  | Closed-loop speed control     | Speed PI @ 1kHz outer loop → Iq setpoint             |
| F3  | Encoder 2 bánh (quadrature)   | TIM3 (trái) + TIM4 (phải), 500PPR × 4 = 2000 cnt/rev|
| F4  | Wheel slip detection          | Slip ratio > 30% warn, > 60% fault                   |
| F5  | Straight-line deviation PI    | ΔPulse_L − ΔPulse_R → PI correction @ 100Hz          |
| F6  | Current sensing via OPAMP     | Internal PGA ×4, 2 phase, shunt 10mΩ                |
| F7  | Vbus monitoring               | ADC1 regular CH6 (PC0), resistor divider 47k/4.7k    |
| F8  | Overcurrent hardware trip     | COMP1 → HRTIM FLT1, < 100ns, không qua CPU          |
| F9  | Temperature sensor TMP112     | I2C, ±0.5°C, warn 75°C / fault 85°C                  |
| F10 | FDCAN speed command           | 0x100 setpoint; 0x101 status; 0x1FF e-stop           |
| F11 | AS5048A absolute encoder SPI  | 14-bit, SPI1, optional theta source cho FOC          |
| NF1 | Clock 170MHz boost mode       | HSE 8MHz → PLL → Cortex-M4 max (PWR Range 1 Boost)  |
| NF2 | HRTIM 20kHz center-aligned    | 184ps resolution, ADC sync tại midpoint               |
| NF3 | Multi-rate scheduler          | 20kHz ISR / 1kHz ISR / 100Hz poll / 10Hz poll        |
| NF4 | Build toolchain               | GNU Make + arm-none-eabi-gcc -O2 -mfloat-abi=hard    |

### 1.3 Có gì (Available Resources)

```
Hardware:
  MCU       : STM32G474RET6 (Cortex-M4 @ 170MHz, 512KB Flash, 96KB SRAM1,
                              32KB SRAM2, HRTIM, 5× OPAMP, 7× COMP, CORDIC, FMAC)
  Driver    : 3-phase MOSFET inverter (6× MOSFET, gate driver tách biệt)
  Motor     : BLDC / PMSM 3-pha (4 cặp cực - FOC_POLE_PAIRS = 4)
  Sensor A  : Current shunt 10mΩ × 2 (Phase A + B) → OPAMP1/OPAMP2 (internal PGA ×4)
  Sensor B  : Wheel encoder quadrature × 2 (500PPR, trái=TIM3, phải=TIM4)
  Sensor C  : TMP112 (TI) digital temperature via I2C1 (±0.5°C, 12-bit, 0x48)
  Sensor D  : AS5048A absolute magnetic encoder via SPI1 (14-bit, 10MHz, optional)
  Bus       : FDCAN1 @ 1Mbps nominal (SN65HVD230 / TCAN1044 external transceiver)
  Power     : DC bus 12-48V (Vbus via resistor divider → ADC1_CH6, PC0)

Software (tự viết từ đầu, không dùng HAL):
  startup_stm32g474.s    → Startup assembly (102 IRQ vector table G474)
  stm32g474.ld           → Linker script (Flash/SRAM1/SRAM2 layout)
  system_stm32g4xx.c     → Clock init SystemInit, PLL 170MHz, GPIO_SetAF helper
  bsp_hrtim.c/h          → HRTIM 3-phase 20kHz + ADC trigger + FLT1 overcurrent
  bsp_opamp.c/h          → OPAMP1/2/3 PGA mode ×4 internal current amplifier
  bsp_comp.c/h           → COMP1 overcurrent comparator → HRTIM FLT1 hardware path
  bsp_adc_sync.c/h       → ADC1/ADC2 injected, HRTIM triggered, Phase A/B + Vbus
  bsp_spi.c/h            → SPI1 (PB3/PB4/PB5, PC4 CS) 10MHz 16-bit Mode 1
  bsp_i2c.c/h            → I2C1 (PB6 SCL, PB7 SDA) 100kHz, TMP112 temperature
  bsp_fdcan.c/h          → FDCAN1 (PA12 TX, PB8 RX) 1Mbps, speed cmd + telemetry
  sensor_encoder.c/h     → TIM3/TIM4 quadrature + TIM2 1kHz + slip + deviation PI
  sensor_temp.c/h        → TMP112 I2C driver + threshold management
  sensor_manager.c/h     → Master SensorData_t + multi-rate scheduler dispatch
  pi_controller.c/h      → Generic PI với anti-windup (dùng cho tất cả loops)
  clarke_park.c/h        → Clarke / Park / InvPark / SVM / FastSinCos LUT
  foc_control.c/h        → FOC context, inner/outer PI, state machine
  motor_foc_main.c       → ISR @ 20kHz + 1kHz, main loop 100Hz + 10Hz
  Makefile               → GNU Make, CPU Cortex-M4 + FPU hard, OpenOCD flash
```

---

## 2. OUTPUT — Hệ thống giải quyết cái gì

### 2.1 Output vật lý

```
┌──────────────────────────────────────────────────────────────────────┐
│ OUTPUT 1: HRTIM 3-phase PWM @ 20kHz → MOSFET gate driver → Motor    │
│   Phase A: PA8 (HRTIM_TA1) + PA9  (HRTIM_TA2) — complementary       │
│   Phase B: PA10(HRTIM_TB1) + PA11 (HRTIM_TB2) — complementary       │
│   Phase C: PB12(HRTIM_TC1) + PB13 (HRTIM_TC2) — complementary       │
│   Freq   : 20kHz center-aligned (PERAR = 136,000 HR ticks)           │
│   Deadtime: 100ns (136 HR ticks @ fHRCK = 5.44GHz)                  │
│   Duty   : SVM output [5% .. 95%] → FOC torque vector               │
├──────────────────────────────────────────────────────────────────────┤
│ OUTPUT 2: FDCAN1 Telemetry → Master controller / PC                  │
│   ID 0x101: FDCAN_StatusMsg_t (speed, current, temp, fault, state)  │
│   Rate   : 10Hz (100ms)                                              │
│   PA12(TX) + PB8(RX) @ 1Mbps nominal                                │
├──────────────────────────────────────────────────────────────────────┤
│ OUTPUT 3: Overcurrent hardware shutdown (< 100ns)                    │
│   COMP1 PA0 > Vref_internal → HRTIM FLT1 → tắt TẤT CẢ 6 MOSFET    │
│   Không qua CPU, không cần ISR                                       │
├──────────────────────────────────────────────────────────────────────┤
│ OUTPUT 4: Thermal shutdown (software @ 10Hz)                         │
│   TMP112 > 85°C → FOC_SetState(FAULT) → HRTIM_DisableOutputs()      │
│   Có hysteresis 5°C để tránh oscillation                             │
├──────────────────────────────────────────────────────────────────────┤
│ OUTPUT 5: Slip + Deviation correction (feedback @ 100Hz / 1kHz)     │
│   Slip   : slip_ratio > 60% → fault → FOC dừng                      │
│   Lệch   : deviation PI → điều chỉnh V_cmd_left / V_cmd_right       │
│   → Robot đi thẳng tự động dù tải 2 bánh không đều                 │
└──────────────────────────────────────────────────────────────────────┘
```

### 2.2 Logic điều khiển — Multi-rate cascade

```
═══════════════════════════════════════════════════════════════════════
  LOOP 1 — ADC ISR @ 20kHz  (priority 0, < 30μs/lần)
  Nguồn: HRTIM Master Period event → trigger ADC1/ADC2 injected
═══════════════════════════════════════════════════════════════════════
  [ADC1 JDR1] Ia_raw → ia_amps = (raw - 2048) × 0.02 A/LSB
  [ADC2 JDR1] Ib_raw → ib_amps
  [ADC1 DR  ] Vbus_raw → vbus_volt = raw × 8.87mV/LSB

  theta_elec += omega_rps × POLE_PAIRS × dt × 2π   (dt = 50μs)

  Clarke(ia, ib)        → i_αβ    (static frame)
  Park(i_αβ, theta)     → i_dq    (rotating frame)
  PI_id(0 - id)         → Vd      (inner loop, dt=50μs)
  PI_iq(Iq_ref - iq)    → Vq
  InvPark(Vd,Vq,theta)  → v_αβ
  SVM(v_αβ, Vbus)       → duty_A, duty_B, duty_C
  HRTIM_SetDuty(A,B,C)  → CMP1R, CMP1B, CMP1C registers

═══════════════════════════════════════════════════════════════════════
  LOOP 2 — TIM2 ISR @ 1kHz  (priority 1, < 500μs/lần)
  Nguồn: TIM2 ARR=999, PSC=169 → UIF @ 1kHz
═══════════════════════════════════════════════════════════════════════
  ΔPulse_L = TIM3->CNT - prev_L    (left wheel quadrature)
  ΔPulse_R = TIM4->CNT - prev_R    (right wheel quadrature)
  V_L = ΔPulse_L × PULSE_TO_METER / 0.001   [m/s]
  V_R = ΔPulse_R × PULSE_TO_METER / 0.001
  slip_ratio = |V_cmd - V_actual| / |V_cmd|
  → slip > 30% → WARNING; > 60% fault

  linear_vel  = (V_L + V_R) / 2
  angular_vel = (V_R - V_L) / WHEEL_BASE

  Speed PI: error = speed_ref - linear_vel
  PI_speed(error) → Iq_ref    (outer loop, dt=1ms)

═══════════════════════════════════════════════════════════════════════
  LOOP 3 — main poll @ 100Hz  (every 10ms)
  Nguồn: SensorManager_RunScheduler() trong while(1)
═══════════════════════════════════════════════════════════════════════
  pulse_error = ΔPulse_L_acc - ΔPulse_R_acc
  integral_error += pulse_error × 0.01     (Ki = 0.05)
  correction = Kp×pulse_error + integral   (Kp = 0.5)
  clamp correction ±20%
  V_cmd_L -= correction × V_base
  V_cmd_R += correction × V_base
  → robot chạy thẳng tự động bù sai lệch 2 bánh

═══════════════════════════════════════════════════════════════════════
  LOOP 4 — main poll @ 10Hz  (every 100ms)
  Nguồn: SensorManager_RunScheduler() trong while(1)
═══════════════════════════════════════════════════════════════════════
  I2C → TMP112 → temperature_c
  → T > 75°C : temp_warning = 1
  → T > 85°C : FOC_SetState(FAULT), HRTIM disable outputs

  CAN → BSP_FDCAN1_ReceiveSetpoint(speed_rpm)
  → cập nhật g_sensor_data.speed_setpoint_mps

  CAN ← BSP_FDCAN1_SendStatus(speed, current, temp, fault, state)
```

---

## 3. Ở GIỮA — Các hệ liên quan & Characteristics

### 3.1 HỆ CƠ KHÍ (Mechanical)

```
Đối tượng: BLDC / PMSM (Brushless DC / Permanent Magnet Synchronous Motor)

Mô hình điện - cơ (trong tọa độ dq):
  Vd = R×Id + Ld×dId/dt - ωe×Lq×Iq
  Vq = R×Iq + Lq×dIq/dt + ωe×(Ld×Id + λpm)

  J×dω/dt = p×(λpm×Iq + (Ld - Lq)×Id×Iq) - B×ω - TL

  p    = số đôi cực (FOC_POLE_PAIRS = 4)
  λpm  = từ thông rotor [Wb]
  Ld,Lq= điện cảm trục d, q [H]
  ωe   = tốc độ điện [rad/s] = ω_mech × p

FOC principle:
  Id = 0 (Maximum Torque Per Ampere strategy)
  → Torque = p × λpm × Iq   (tuyến tính với Iq)
  → Điều khiển torque = điều khiển Iq = điều khiển 1 biến vô hướng

Cơ học robot 2 bánh:
  WHEEL_DIAMETER_MM  = 65mm
  WHEEL_BASE_MM      = 150mm
  PULSE_TO_METER     = π × 65mm / (500PPR × 4) = 0.000102 m/pulse
  Vmax (ước tính)    = 5 rev/s × π × 0.065m ≈ 1.02 m/s

Hệ bánh xe:
  ΔPulse_L vs ΔPulse_R → kiểm tra đồng bộ
  V_linear  = (V_L + V_R) / 2   [m/s]
  V_angular = (V_R - V_L) / WHEEL_BASE_MM × 1000  [rad/s]
```

### 3.2 HỆ ĐIỆN (Electrical)

```
3-Phase MOSFET Inverter:
  ┌───────────────────────────────────────────────────────┐
  │ Topology   : 3-phase full bridge (6× MOSFET)         │
  │ Vbus range : 12V - 48V (qua resistor divider → ADC)  │
  │ Gate driver: External IC (IR2103 / DRV8302 tương tự) │
  │ Deadtime   : 100ns hardware (HRTIM DTxR = 136 ticks) │
  │ PWM freq   : 20kHz (switching loss vs THD trade-off) │
  └───────────────────────────────────────────────────────┘

Current Sensing (Internal OPAMP):
  ┌───────────────────────────────────────────────────────┐
  │ Shunt      : 10mΩ (pha A, pha B) — 3 pha sum = 0    │
  │ Amplifier  : OPAMP1 (PA1 in) + OPAMP2 (PA7 in) PGA  │
  │ Gain       : ×4 (VMSEL_PGA + PGA_GAIN_4)            │
  │ Mode       : High-speed internal (no external op-amp)│
  │ Resolution : 1 LSB = 0.02A (12-bit ADC, 3.3V ref)   │
  │ Range      : ±10A (ADC_CURRENT_ZERO = 2048)          │
  │ Ic         : = -(Ia + Ib) — Clarke, không cần đo    │
  └───────────────────────────────────────────────────────┘

Overcurrent Protection (COMP1 Hardware):
  ┌───────────────────────────────────────────────────────┐
  │ Input+     : PA0 (current sense analog)               │
  │ Input-     : Vref_internal (programmable threshold)   │
  │ Hysteresis : 10mV (HYST_10MV)                        │
  │ Output     : internally routed → HRTIM FLT1          │
  │ Latency    : < 100ns (hardware, no ISR needed)       │
  │ vs software: software path ~2-5μs @ 170MHz           │
  └───────────────────────────────────────────────────────┘

Vbus Monitoring:
  ┌───────────────────────────────────────────────────────┐
  │ Divider    : 47kΩ + 4.7kΩ → ratio 1/11              │
  │ Pin        : PC0 → ADC1 regular CH6                   │
  │ Scale      : 8.87mV/LSB (sau quy đổi divider)        │
  │ Dùng trong : SVM normalization (Vα/Vbus, Vβ/Vbus)   │
  └───────────────────────────────────────────────────────┘

Temperature (TMP112 Digital):
  ┌───────────────────────────────────────────────────────┐
  │ Giao thức  : I2C (PB6 SCL, PB7 SDA) @ 100kHz        │
  │ Địa chỉ   : 0x48 (ADD0 = GND)                       │
  │ Accuracy   : ±0.5°C (-25 to +85°C)                   │
  │ Resolution : 12-bit, 0.0625°C/LSB                    │
  │ Config     : 0x60E0 → continuous 8Hz conversion      │
  │ Warning    : 75°C, Fault: 85°C, Hysteresis: 5°C     │
  └───────────────────────────────────────────────────────┘

Encoder (AS5048A - optional absolute angle):
  ┌───────────────────────────────────────────────────────┐
  │ Giao thức  : SPI1 (PB3 SCK, PB4 MISO, PB5 MOSI)    │
  │ CS         : PC4 (GPIO out)                           │
  │ Speed      : fPCLK/16 ≈ 10MHz (APB2 @ 170MHz)       │
  │ Frame      : 16-bit, Mode 1 (CPHA=1, CPOL=0)        │
  │ Resolution : 14-bit (16384 steps/revolution)         │
  │ Dùng cho   : theta_elec chính xác (khi có lắp)      │
  └───────────────────────────────────────────────────────┘

FDCAN Bus:
  ┌───────────────────────────────────────────────────────┐
  │ Chuẩn      : ISO 11898-1 CAN FD                      │
  │ Transceiver: SN65HVD230 / TCAN1044 (external)        │
  │ Pin        : PA12(TX AF9), PB8(RX AF9)               │
  │ Nominal    : 1Mbps (NBTP = 0x00050403)               │
  │ RAM        : 0x4000A400 (dedicated message RAM G474) │
  │ Messages   : 0x100 (cmd), 0x101 (status), 0x1FF (stop)│
  └───────────────────────────────────────────────────────┘
```

### 3.3 SENSOR CALIBRATION & SIGNAL CONDITIONING

```
Ver2 nâng cấp toàn bộ signal conditioning so với Ver1:
  Ver1: analog sensor + manual calibration + software linearization
  Ver2: digital sensor + hardware amplification + auto-calibration

═══════════════════════════════════════════════════════════════
SIGNAL CONDITIONING PIPELINE (Ver2)
═══════════════════════════════════════════════════════════════

  Physical    Transducer   Conditioning    ADC          Calibration    Usable
  Quantity →  Element   →  (HW stage)  →  12-bit    →  (SW/HW)    →  Value
  ───────────────────────────────────────────────────────────────────────
  Phase I [A] → Shunt 10mΩ → OPAMP PGA ×4 → ADC1 inj  → zero+gain   → ia_amps
  Phase I [A] → Shunt 10mΩ → OPAMP PGA ×4 → ADC2 inj  → zero+gain   → ib_amps
  Vbus [V]    → R-divider  → (passive)    → ADC1 reg  → ratio       → vbus_volt
  Temp [°C]   → TMP112     → (internal)   → I2C 12bit → digital     → temp_c
  Angle [°]   → AS5048A    → (internal)   → SPI 14bit → zero offset → theta_elec
  Velocity    → Encoder    → TIM quad×4   → 16-bit CNT→ pulse2meter → V_L, V_R

═══════════════════════════════════════════════════════════════
CALIBRATION TẪNg SENSOR (từng sensor)
═══════════════════════════════════════════════════════════════

  1. Phase Current (OPAMP + Shunt) — Hardware-conditioned
  ────────────────────────────────────────────────────────
    Signal chain: Shunt → OPAMP (PGA ×4 internal) → ADC injected
    V_adc = (I × R_shunt) × PGA_gain + V_offset
    V_adc = (I × 0.010) × 4 + 1.65V
    → Sensitivity = 40 mV/A
    → ADC_to_A = (adc_raw - ADC_CURRENT_ZERO) × 0.02

    Calibration:
      Bước 1: ADC auto-calibration (hardware G474 ADCAL bit)
             → Loại bỏ offset nội bộ ADC tự động
      Bước 2: Đo ADC khi I = 0A (motor off, HRTIM disabled)
             → ADC_CURRENT_ZERO actual (≈ 2048 ± vài LSB)
      Bước 3: Đo với dòng tham chiếu (current probe / shunt meter)
             → Tính gain_correction
      Bước 4: Hồi quy: I_actual = a × I_measured + b

    Error budget:
      ± ADC quantization    : 3300/4095 × 1/0.04 = 0.02 A/LSB
      ± OPAMP offset (auto) : < 2 mV → < 50 mA
      ± Shunt tolerance     : ±1% → ±0.1 A @ 10A
      → Tổng sai số: ±200 mA @ 10A range (±2%)
      → FOC hoạt động ổn vì PI loop bù sai số liên tục

  2. Vbus Voltage (Resistor Divider) — Passive conditioning
  ─────────────────────────────────────────────────────
    Divider: 47kΩ + 4.7kΩ → ratio = 4.7/(47+4.7) = 1/11
    V_adc = Vbus / 11
    Vbus = adc_raw × (3.3/4095) × 11 = adc_raw × 8.87 mV/LSB

    Calibration:
      Đo Vbus bằng đồng hồ đo, so với giá trị ADC
      Resistor tolerance ±1% → ratio error < 0.2%
      → Vbus accuracy: ±0.5V @ 48V (đủ cho SVM normalization)

  3. TMP112 Temperature — Digital, factory-calibrated
  ──────────────────────────────────────────────────────
    Output: 12-bit digital, 0.0625°C/LSB, I2C
    Factory accuracy: ±0.5°C (-25 to +85°C)
    → KHÔNG cần calibration thủ công!
    → Đây là lý do chính chuyển từ NTC (Ver1) sang TMP112 (Ver2)

  4. Wheel Encoder (Quadrature) — Hardware counting
  ───────────────────────────────────────────────────
    500PPR × 4 (quadrature) = 2000 counts/revolution
    PULSE_TO_METER = π × 65mm / 2000 = 0.000102 m/pulse
    V = Δpulse × PULSE_TO_METER / dt

    Calibration:
      Bước 1: Xoay bánh xe 10 vòng, đếm pulse tổng
             → PPR_actual (có thể lệch do cơ khí lắp encoder)
      Bước 2: Đo quãng đường thực tế vs tính toán
             → Diámeter_actual (bánh xe mòn → D giảm)
      Bước 3: Hồi quy: distance_actual = a × distance_measured + b
             → Điều chỉnh PULSE_TO_METER

  5. AS5048A Absolute Angle (SPI) — Optional FOC theta
  ─────────────────────────────────────────────────────
    14-bit (16384 steps/rev) → 0.022°/step
    theta_elec = (raw_angle × POLE_PAIRS) mod 360°

    Calibration:
      Zero offset: đặt rotor về vị trí biết trước
      (Phase A aligned) → đọc angle → lưu offset
      theta_corrected = theta_raw - theta_offset
      → Quan trọng: nếu offset sai 10° → FOC torque giảm ~15%

═══════════════════════════════════════════════════════════════
TỔNG SAI SỐ HỆ THỐNG (Error Budget Summary)
═══════════════════════════════════════════════════════════════

  Sensor       │ Range       │ Error       │ % FS    │ Vs Ver1
  ─────────────│─────────────│─────────────│─────────│───────────────
  Phase I (OPA)│ ±10 A       │ ±0.2 A      │ ±2%     │ 3× better
  Vbus         │ 12-48 V     │ ±0.5 V      │ ±1%     │ N/A (new)
  TMP112       │ -25..+85°C  │ ±0.5 °C     │ ±0.5%   │ 10× better
  Wheel enc    │ 0-5 rev/s   │ ±0.5 mm/s   │ ±0.05%  │ 100× better
  AS5048A      │ 0-360°      │ ±0.022°     │ ±0.006% │ N/A (new)

═══════════════════════════════════════════════════════════════
VER1 → VER2: SIGNAL CONDITIONING EVOLUTION
═══════════════════════════════════════════════════════════════

  Khâu           │ Ver1              │ Ver2                    │ Tại sao
  ───────────────│───────────────────│─────────────────────────│───────────────
  Current sense  │ ACS712 (analog)   │ Shunt + OPAMP PGA       │ BW, noise
  Amplification  │ Không có           │ Internal OPAMP ×4       │ SNR, BOM
  ADC sync       │ DMA continuous    │ HRTIM trigger midpoint  │ Switching noise
  ADC calibrate  │ Không có           │ Auto-cal (ADCAL bit)    │ Offset cancel
  Temperature    │ NTC analog (±5°C) │ TMP112 digital (±0.5°C) │ Accuracy ×10
  Linearization  │ LUT/piecewise     │ Không cần (digital)     │ Complexity
  Encoder        │ 20PPR single-ch   │ 500PPR quadrature ×4   │ Resolution
  Angle          │ Không có           │ AS5048A 14-bit absolute │ FOC theta
  OC protection  │ SW 100ms          │ HW COMP1 < 100ns       │ Safety
```

### 3.4 HỆ DRIVER / BSP

```
BSP HRTIM (bsp_hrtim.h / bsp_hrtim.c):
  ┌───────────────────────────────────────────────────────┐
  │ fHRTIM  = 170MHz                                     │
  │ fHRCK   = 170MHz × 32 (DLL) = 5.44GHz               │
  │ PERAR   = 136,000 → fPWM = 5.44GHz/(2×136000)=20kHz │
  │ Deadtime= 136 ticks @ fHRCK/8 = 1.36GHz → 100ns     │
  │ Duty min= 5% (6,800 ticks), max = 95% (129,200 ticks)│
  │                                                       │
  │ Phase A: PA8  (TA1 AF13) + PA9  (TA2 AF13)           │
  │ Phase B: PA10 (TB1 AF13) + PA11 (TB2 AF13)           │
  │ Phase C: PB12 (TC1 AF13) + PB13 (TC2 AF13)           │
  │                                                       │
  │ ADC trigger: CMN.ADC1R bit[4] = Master Period event  │
  │ → ADC sample tại đỉnh PWM (minimum switching noise)  │
  │                                                       │
  │ FLT1 (overcurrent): CMN.FLTINR1 FLT1EN              │
  │ → COMP1 output → tắt 6 PWM outputs < 100ns          │
  │                                                       │
  │ API:                                                  │
  │   BSP_HRTIM_Init()              → khởi tạo full     │
  │   BSP_HRTIM_SetDuty(A, B, C)   → cập nhật CMP1xR   │
  │   BSP_HRTIM_EnableOutputs()    → bật sau align      │
  │   BSP_HRTIM_DisableOutputs()   → emergency stop     │
  │   BSP_HRTIM_ClearFault()       → sau khi OC cleared │
  └───────────────────────────────────────────────────────┘

BSP OPAMP (bsp_opamp.h / bsp_opamp.c):
  ┌───────────────────────────────────────────────────────┐
  │ OPAMP1: VP = PA1, PGA ×4, High-speed internal        │
  │ OPAMP2: VP = PA7, PGA ×4, High-speed internal        │
  │ OPAMP3: VP = PB0 (tham chiếu / Ic optional)          │
  │ Output → internally connected to ADC1_IN13/ADC2_IN16 │
  │ Không cần IC khuếch đại ngoài                        │
  └───────────────────────────────────────────────────────┘

BSP COMP (bsp_comp.h / bsp_comp.c):
  ┌───────────────────────────────────────────────────────┐
  │ COMP1 IN+: PA0 (current sense raw)                   │
  │ COMP1 IN-: VREFINT (internal, programmable)          │
  │ Hysteresis: 10mV, BRGEN+SCALEN (chính xác)           │
  │ Output: HRTIM FLT1 (internal routing, không cần dây) │
  └───────────────────────────────────────────────────────┘

BSP ADC Sync (bsp_adc_sync.h / bsp_adc_sync.c):
  ┌───────────────────────────────────────────────────────┐
  │ ADC1/ADC2 injected mode, HRTIM_ADC_TRG1 trigger      │
  │ Clock: CCR CKMODE = HCLK/4 = 42.5MHz (synchronous)  │
  │ ADC1 injected CH13: OPAMP1 OUT (Phase A)             │
  │ ADC2 injected CH16: OPAMP2 OUT (Phase B)             │
  │ ADC1 regular  CH6:  Vbus (PC0)                       │
  │ 12-bit, auto-calibration trước khi enable            │
  │                                                       │
  │ Conversion:                                           │
  │   ia_amps  = (ia_raw  - 2048) × 0.02 A/LSB          │
  │   ib_amps  = (ib_raw  - 2048) × 0.02 A/LSB          │
  │   vbus_volt= vbus_raw × 8.87mV/LSB × 11 (divider)   │
  │                                                       │
  │ ISR: ADC1_2_IRQHandler → JEOS flag → ReadResults()  │
  └───────────────────────────────────────────────────────┘

Sensor Encoder (sensor_encoder.h / sensor_encoder.c):
  ┌───────────────────────────────────────────────────────┐
  │ TIM3 encoder mode: PC6(CH1 AF2) + PC7(CH2 AF2)       │
  │   → Left wheel, auto count up/down với chiều quay    │
  │ TIM4 encoder mode: PD12(CH1 AF2) + PD13(CH2 AF2)    │
  │   → Right wheel                                      │
  │ TIM2 @ 1kHz: PSC=169, ARR=999 → velocity sampling   │
  │                                                       │
  │ PPR = 500, 4× quadrature = 2000 counts/rev           │
  │ PULSE_TO_METER = 204mm / 2000 = 0.000102 m/pulse    │
  │ WHEEL_BASE_MM  = 150mm                               │
  │                                                       │
  │ Slip detection:                                       │
  │   slip_ratio = |V_cmd - V_actual| / |V_cmd|          │
  │   > 30% → warn, > 60% + 500ms → fault               │
  │                                                       │
  │ Deviation PI:                                         │
  │   error = ΔPulse_L - ΔPulse_R                       │
  │   Kp=0.5, Ki=0.05, anti-windup ±500, output ±20%    │
  └───────────────────────────────────────────────────────┘

Sensor Manager (sensor_manager.h / sensor_manager.c):
  ┌───────────────────────────────────────────────────────┐
  │ g_sensor_data: SensorData_t (single source of truth) │
  │   Fast  fields (20kHz): ia/ib/ic/vbus                │
  │   Medium fields (1kHz): V_L, V_R, linear, angular   │
  │   Slow  fields (100Hz): deviation correction        │
  │   Slow  fields (10Hz) : temperature, CAN setpoint   │
  │                                                       │
  │ SensorManager_Init()       → khởi tạo theo thứ tự:  │
  │   OPAMP → COMP → SPI → I2C → Encoder → ADC → CAN   │
  │                                                       │
  │ SensorManager_RunScheduler() → main while(1):        │
  │   if (tick - last_10ms  >= 10)  → Update_100Hz()    │
  │   if (tick - last_100ms >= 100) → Update_10Hz()     │
  └───────────────────────────────────────────────────────┘
```

### 3.5 HỆ FIRMWARE / CONTROL

```
Platform Layer:
  ┌───────────────────────────────────────────────────────┐
  │ startup_stm32g474.s                                   │
  │   1. Copy .data Flash → SRAM1                        │
  │   2. Zero .bss                                       │
  │   3. Gọi SystemInit() → main()                       │
  │   4. Vector table 102 IRQ (G474 full)                │
  │      Bao gồm: HRTIM1_Master, HRTIM_TIMA-F           │
  │               ADC1_2, COMP1_2_3, FDCAN1/2/3          │
  │               CORDIC, FMAC                           │
  │   5. Default_Handler: infinite loop                  │
  │                                                       │
  │ stm32g474.ld (linker script)                         │
  │   FLASH  : 512KB @ 0x08000000                       │
  │   SRAM1  :  96KB @ 0x20000000                       │
  │   SRAM2  :  32KB @ 0x20018000                       │
  │   _estack = ORIGIN(SRAM1) + LENGTH(SRAM1)           │
  │                                                       │
  │ system_stm32g4xx.c (clock init)                      │
  │   Bước 1: PWR.CR5 &= ~R1MODE → Boost mode ON        │
  │   Bước 2: FLASH latency = 4WS, prefetch + cache ON  │
  │   Bước 3: HSE 8MHz bật, chờ HSERDY                  │
  │   Bước 4: APB1=/2=85MHz, APB2=/1=170MHz             │
  │   Bước 5: PLL: PLLM=/2 → 4MHz → ×85 → 340MHz →/2  │
  │           → SYSCLK = 170MHz                         │
  │   Bước 6: SW → PLL, chờ SWS confirm                 │
  └───────────────────────────────────────────────────────┘

Control Layer:
  ┌───────────────────────────────────────────────────────┐
  │ PI Controller (pi_controller.h / pi_controller.c)    │
  │   PI_Controller_t: kp, ki, integral, limits, dt      │
  │   Anti-windup: clamp integral → clamp output        │
  │   PI_Update(pi, error)   → output                   │
  │   PI_Reset(pi)           → clear integral           │
  │   Dùng cho: pi_id, pi_iq (20kHz), pi_speed (1kHz)  │
  │             deviation PI (100Hz)                     │
  │                                                       │
  │ Clarke / Park / SVM (clarke_park.h / clarke_park.c) │
  │   Clarke:   Iα = Ia                                  │
  │             Iβ = (Ia + 2×Ib) / √3                   │
  │   Park:     Id =  Iα·cos(θ) + Iβ·sin(θ)            │
  │             Iq = -Iα·sin(θ) + Iβ·cos(θ)            │
  │   InvPark:  Vα = Vd·cos(θ) - Vq·sin(θ)            │
  │             Vβ = Vd·sin(θ) + Vq·cos(θ)             │
  │   SVM 6-sector:                                      │
  │     Vref1 = Vβ                                       │
  │     Vref2 = (√3·Vα - Vβ)/2                          │
  │     Vref3 = -(√3·Vα + Vβ)/2                         │
  │     sector từ sign(Vref1/2/3) → T1, T2, T0          │
  │     duty clamped [5%, 95%]                           │
  │   FastSinCos: LUT 256 entry float, [0, 2π)          │
  │                                                       │
  │ FOC Control (foc_control.h / foc_control.c)         │
  │   FOC_Context_t: pi_id, pi_iq, pi_speed             │
  │                  i_ab, i_dq, v_dq, v_ab             │
  │                  theta_elec, omega_rps               │
  │                  id_ref=0, iq_ref, speed_ref_rps    │
  │                                                       │
  │   State machine: (xem chi tiết section 3.6 FSM)      │
  │     IDLE → ALIGN: inject Id, xác định rotor pos    │
  │     ALIGN → RUN: enable HRTIM, current loop active  │
  │     RUN → FAULT: disable HRTIM, PWM off            │
  │     RUN → BRAKE: active braking                    │
  │     FAULT → IDLE: clear fault, PI reset            │
  │                                                       │
  │   FOC_CurrentLoop_20kHz(ia, ib, theta, vbus)        │
  │     → full Clarke/Park/PI/SVM/HRTIM @ 50μs/call    │
  │                                                       │
  │   FOC_SpeedLoop_1kHz(speed_actual_rps)              │
  │     → PI_speed → Iq_ref cho current loop           │
  └───────────────────────────────────────────────────────┘

Application Layer:
  ┌───────────────────────────────────────────────────────┐
  │ motor_foc_main.c                                      │
  │   Architecture : Multi-rate bare-metal               │
  │                                                       │
  │   ADC1_2_IRQHandler (priority 0 — highest):          │
  │     → SensorManager_Update_20kHz()                   │
  │     → theta integration (omega × dt × pole_pairs)   │
  │     → FOC_CurrentLoop_20kHz()                        │
  │     → fault state update                             │
  │     Timing: < 30μs (< 5100 cycles @ 170MHz)         │
  │                                                       │
  │   TIM2_IRQHandler (priority 1):                      │
  │     → SensorManager_Update_1kHz()   (encoder)        │
  │     → FOC_SpeedLoop_1kHz()          (speed PI)       │
  │     Có thể bị preempt bởi ADC ISR (không share data)│
  │                                                       │
  │   main while(1):                                      │
  │     → SensorManager_RunScheduler()  (100Hz + 10Hz)  │
  │     → System state machine (RUN/FAULT/IDLE)         │
  │                                                       │
  │   Init sequence:                                      │
  │     delay 100ms → SensorManager_Init →              │
  │     BSP_HRTIM_Init → FOC_Init →                     │
  │     SetSpeedRef → delay 500ms → FOC_STATE_RUN       │
  └───────────────────────────────────────────────────────┘
```

### 3.6 SYSTEM STATE MACHINE (FSM) — FOC Motor Controller

```
Ver2 FSM phức tạp hơn Ver1 vì:
  - Có FOC pipeline (Clarke/Park/PI/SVM) cần enable/disable đúng thứ tự
  - HRTIM output phải tắt TRƯỚC khi clear fault (nếu không → shoot-through)
  - PI integral phải reset khi chuyển state (tránh windup burst)
  - Hardware fault (COMP1 → FLT1) có thể xảy ra BẤT KỲ LÚC NÀO
  - Nhiều nguồn fault: overcurrent HW, overtemp SW, slip, CAN e-stop

═══════════════════════════════════════════════════════════════════════
  STATE DIAGRAM — 2 FSM tách biệt trong code
═══════════════════════════════════════════════════════════════════════

  ┌─────────────────────────────────────────────────────────────────┐
  │ SYSTEM-LEVEL FSM (SystemState_t — motor_foc_main.c)            │
  │ 6 states: SYS_BOOT → SYS_INIT → SYS_IDLE → SYS_RUN           │
  │                                   ↑          ↓                 │
  │                                   └── SYS_FAULT ── SYS_BRAKE  │
  └─────────────────────────────────────────────────────────────────┘

  ┌─────────────────────────────────────────────────────────────────┐
  │ FOC-LEVEL FSM (FOC_State_t — foc_control.h)                    │
  │ 5 states: IDLE → ALIGN → RUN → FAULT / BRAKE                  │
  └─────────────────────────────────────────────────────────────────┘

  System FSM chi tiết:

  ┌──────────┐   SystemInit    ┌──────────┐  BSP_Init   ┌──────────┐
  │ SYS_BOOT │───────────────→│ SYS_INIT │────────────→│ SYS_IDLE │
  │(power-on)│                │(HW setup)│             │(standby) │
  └──────────┘                └──────────┘             └────┬─────┘
                                                           │    ↑
                                                CAN 0x100  │    │
                                                speed cmd  │    │ fault
                                                 received  │    │ cleared
                                                           ▼    │
                                                     ┌──────────┐
                                           ┌────────→│ SYS_RUN  │
                                           │         │ (FOC     │
                                           │         │  active) │
                                           │         └───┬──┬───┘
                                           │             │  │
                                  safe     │     HW OC   │  │ SW fault
                                  again    │  (COMP1→FLT)│  │ (OT/slip)
                                           │             ▼  ▼
                                      ┌────┴────┐   ┌──────────┐
                                      │SYS_BRAKE│   │SYS_FAULT │
                                      │(active  │←──│(shutdown)│
                                      │ braking)│   └──────────┘
                                      └─────────┘

  FOC FSM chi tiết:

  ┌──────────┐   inject Id    ┌──────────┐  alignment  ┌──────────┐
  │FOC_IDLE  │───────────────→│FOC_ALIGN │────────────→│ FOC_RUN  │
  │(PWM off) │                │(rotor    │  done       │(FOC loop)│
  └──────────┘                │ position)│             └───┬──┬───┘
       ↑                      └──────────┘                 │  │
       │                                           fault   │  │ brake
       │ cleared                                           ▼  ▼
       │                                          ┌─────┐ ┌─────┐
       └──────────────────────────────────────────│FAULT│ │BRAKE│
                                                  └─────┘ └─────┘

═══════════════════════════════════════════════════════════════════════
  TRANSITION TABLE (chi tiết)
═══════════════════════════════════════════════════════════════════════

  ─── SYSTEM FSM (SystemState_t — motor_foc_main.c) ───

  From       │ To         │ Guard                       │ Action
  ───────────│────────────│─────────────────────────────│──────────────────────
  SYS_BOOT   │ SYS_INIT   │ Power-on, SystemInit()      │ Clock, Flash config
  ───────────│────────────│─────────────────────────────│──────────────────────
  SYS_INIT   │ SYS_IDLE   │ BSP_*_Init() + FOC_Init()   │ HRTIM outputs OFF
             │            │ all OK                      │ PI reset, CAN ready
  ───────────│────────────│─────────────────────────────│──────────────────────
  SYS_IDLE   │ SYS_RUN    │ CAN 0x100 speed setpoint    │ FOC_SetState(RUN)
             │            │ received, no active faults  │ HRTIM_EnableOutputs()
             │            │                             │ PI_Reset(id,iq,speed)
  ───────────│────────────│─────────────────────────────│──────────────────────
  SYS_RUN    │ SYS_FAULT  │ COMP1 hardware trip (<100ns)│ HRTIM auto-disable
             │ (HW path)  │ → HRTIM FLT1 active        │ fault_code = OC
  ───────────│────────────│─────────────────────────────│──────────────────────
  SYS_RUN    │ SYS_FAULT  │ temp_c > 85°C               │ HRTIM_DisableOutputs()
             │ (SW path)  │ OR slip_ratio > 60% + 500ms │ PI_Reset(all)
             │            │ OR Vbus out of range        │ fault_code = OT/OV
  ───────────│────────────│─────────────────────────────│──────────────────────
  SYS_RUN    │ SYS_BRAKE  │ Brake command received      │ Active braking
             │            │                             │ HRTIM controlled stop
  ───────────│────────────│─────────────────────────────│──────────────────────
  SYS_FAULT  │ SYS_IDLE   │ Fault cleared (safe values) │ HRTIM_ClearFault()
             │            │ AND temp < 80°C (hyst 5°C)  │ PI_Reset(all)
  ───────────│────────────│─────────────────────────────│──────────────────────
  SYS_BRAKE  │ SYS_IDLE   │ Motor stopped               │ HRTIM outputs OFF
  ───────────│────────────│─────────────────────────────│──────────────────────

  ─── FOC FSM (FOC_State_t — foc_control.h) ───

  From       │ To         │ Guard                       │ Action
  ───────────│────────────│─────────────────────────────│──────────────────────
  FOC_IDLE   │ FOC_ALIGN  │ FOC_SetState(RUN) called    │ Inject Id, lock rotor
  ───────────│────────────│─────────────────────────────│──────────────────────
  FOC_ALIGN  │ FOC_RUN    │ Alignment done (rotor pos)  │ Enable current loop
  ───────────│────────────│─────────────────────────────│──────────────────────
  FOC_RUN    │ FOC_FAULT  │ Overcurrent/overtemp        │ Disable HRTIM outputs
  ───────────│────────────│─────────────────────────────│──────────────────────
  FOC_RUN    │ FOC_BRAKE  │ Brake command               │ Active braking mode
  ───────────│────────────│─────────────────────────────│──────────────────────
  FOC_FAULT  │ FOC_IDLE   │ Fault cleared               │ PI_Reset(all)
  ───────────│────────────│─────────────────────────────│──────────────────────

═══════════════════════════════════════════════════════════════════════
  FAULT SUB-TYPES & PRIORITY
═══════════════════════════════════════════════════════════════════════

  Code trong bsp_fdcan.h: 0=OK, 1=OC, 2=OT, 3=OV (4 loại)

  Priority│ Code │ Source              │ Path     │ Latency  │ Auto-recover?
  ────────│──────│─────────────────────│──────────│──────────│──────────────
  0 (max) │ OC(1)│ COMP1 → HRTIM FLT1 │ Hardware │ < 100ns  │ Yes (w/ cool)
  1       │ OT(2)│ TMP112 > 85°C       │ Poll 10Hz│ 100ms    │ Yes (hyst 5°C)
  2       │ OV(3)│ Vbus out of range   │ ISR 20kHz│ 50μs     │ Yes
  3       │ SLIP │ slip > 60% + 500ms  │ ISR 1kHz │ 500ms    │ Yes

═══════════════════════════════════════════════════════════════════════
  IMPLEMENTATION (pseudo-code)
═══════════════════════════════════════════════════════════════════════

  // ─── FOC_State_t (foc_control.h) — FOC-level FSM ───
  typedef enum {
      FOC_STATE_IDLE   = 0,   /* Motor dừng, PWM off */
      FOC_STATE_ALIGN  = 1,   /* Alignment: inject Id để biết rotor position */
      FOC_STATE_RUN    = 2,   /* FOC chạy bình thường */
      FOC_STATE_FAULT  = 3,   /* Fault: overcurrent/overtemp → PWM off */
      FOC_STATE_BRAKE  = 4,   /* Active braking */
  } FOC_State_t;

  // ─── SystemState_t (motor_foc_main.c) — System-level FSM ───
  typedef enum {
      SYS_BOOT  = 0,   /* Power-on, SystemInit */
      SYS_INIT  = 1,   /* Hardware BSP init */
      SYS_IDLE  = 2,   /* Motor stopped, waiting CAN cmd */
      SYS_RUN   = 3,   /* Normal FOC operation */
      SYS_FAULT = 4,   /* Fault detected (OC/OT/OV) */
      SYS_BRAKE = 5,   /* Active braking */
  } SystemState_t;

  // Trong main while(1), 10Hz context:
  switch (s_sys_state) {
      case SYS_RUN:
          // FOC chạy trong ADC ISR 20kHz (tự động)
          // Speed PI chạy trong TIM2 ISR 1kHz
          // Ở đây chỉ check slow faults:
          if (g_sensor_data.any_fault) {
              BSP_HRTIM_DisableOutputs();
              PI_Reset(&pi_id); PI_Reset(&pi_iq); PI_Reset(&pi_speed);
              s_sys_state = SYS_FAULT;
          }
          // Note: HW_OC (COMP1→FLT1) tự disable HRTIM, ISR set fault flag
          if (hrtim_fault_flag) {
              s_sys_state = SYS_FAULT;
          }
          break;

      case SYS_FAULT:
          // Motor OFF, kiểm tra điều kiện safe
          // temp < 80°C (85 - 5°C hysteresis) AND current safe AND vbus OK
          if (current_safe && temp_c < 80.0f && vbus_ok) {
              BSP_HRTIM_ClearFault();
              PI_Reset(&pi_id); PI_Reset(&pi_iq); PI_Reset(&pi_speed);
              s_sys_state = SYS_IDLE;
          }
          // CAN telemetry vẫn gửi (fault status)
          break;

      case SYS_IDLE:
          // HRTIM outputs OFF, FOC không chạy
          // Chờ CAN speed command
          if (can_speed_cmd_received) {
              PI_Reset(&pi_id); PI_Reset(&pi_iq); PI_Reset(&pi_speed);
              BSP_HRTIM_EnableOutputs();
              s_sys_state = SYS_RUN;
          }
          break;
  }

═══════════════════════════════════════════════════════════════════════
  VER1 vs VER2 FSM COMPARISON
═══════════════════════════════════════════════════════════════════════

  Thuộc tính       │ Ver1 FSM             │ Ver2 FSM
  ─────────────────│──────────────────────│───────────────────────────
  Số states         │ 4 (INIT/RUN/FAULT/   │ System: 6 (BOOT/INIT/IDLE/
                    │    RECOVERY)         │   RUN/FAULT/BRAKE)
                    │                      │ FOC: 5 (IDLE/ALIGN/RUN/
                    │                      │   FAULT/BRAKE)
  Trigger nguồn     │ Software only        │ Hardware (COMP) + Software
  Fault types       │ OC, OT, heartbeat    │ OC(1), OT(2), OV(3)
                    │                      │ + slip detection
  Recovery          │ Check safe → run     │ FAULT → IDLE (safe values
                    │                      │ + HRTIM clear + PI reset)
  Lockout           │ fault_count > 3      │ N/A (simple FAULT→IDLE)
  Run context       │ Main loop 10Hz       │ ISR 20kHz + 1kHz + poll
  HRTIM mgmt        │ N/A (L298N GPIO)     │ Enable/Disable/ClearFault
  PI management     │ N/A (no PI)          │ Reset integral mỗi transition
  CAN integration   │ N/A                  │ Cmd start/stop + telemetry
  Hysteresis        │ OT: 85→80°C (5°C)   │ OT: 85→80°C (5°C)
```

---

## 4. ARCHITECTURE & DESIGN PHILOSOPHY (Closed-loop / Multi-rate Bare-metal)

### 4.1 Core Principles

```
Nguyên tắc thiết kế (kế thừa Ver1 + nâng cấp):
  1. Timer-driven system      → HRTIM + TIM2 đa tần số, deterministic
  2. Non-blocking execution   → ISR chain + polling scheduler, không delay
  3. Event-driven data flow   → ADC trigger bởi HRTIM, DMA/interrupt flag
  4. Pipeline processing      → sense → transform → control → actuate

System type (Ver2):
  ✦ CLOSED-LOOP CONTROL (FOC cascade: current + speed PI)
  ✦ Feedback bù liên tục → tốc độ ổn định bất kể tải
  ✦ Deterministic timing: mỗi loop phải hoàn thành đúng deadline
  ✦ Phù hợp: motor control, robotics, PID systems, production
```

### 4.2 System Pipeline Overview

```
HRTIM Master Period (20kHz) → trigger ADC1/ADC2 injected @ midpoint
  ↓
ADC ISR (priority 0):
  read Ia, Ib, Vbus → Clarke → Park → PI_id/PI_iq → InvPark → SVM → HRTIM duty
  ↓
TIM2 ISR (1kHz, priority 1):
  read encoder L/R → velocity → slip check → Speed PI → Iq_ref
  ↓
Main loop poll (100Hz):
  deviation PI → correction L/R
  ↓
Main loop poll (10Hz):
  I2C temp → CAN rx/tx → fault management

```

### 4.3 Closed-loop System Characteristics

```
Đặc trưng closed-loop vs open-loop (Ver1):

  ┌──────────────────────┬──────────────────────┬──────────────────────┐
  │ Thuộc tính           │ Ver1 (Open-loop)     │ Ver2 (Closed-loop)   │
  │──────────────────────│──────────────────────│──────────────────────│
  │ Timing               │ Flexible (10Hz)      │ Fixed, deterministic │
  │                      │                      │ (20kHz / 1kHz)       │
  │ Feedback             │ Không có bù          │ Cascade PI feedback  │
  │ Delay tolerance      │ 100ms chấp nhận      │ > 50μs = instability │
  │ Sampling sync        │ ADC async (DMA auto) │ ADC sync HRTIM mid   │
  │ Jitter               │ Không quan trọng     │ Must minimize        │
  │ Protection           │ Software 100ms       │ Hardware < 100ns     │
  │ Stability            │ Phụ thuộc tải        │ Ổn định bất kể tải  │
  │ Throughput           │ Ưu tiên data volume  │ Ưu tiên latency      │
  │ Ứng dụng            │ Logging, IoT, proto  │ Motor, PID, robotics │
  └──────────────────────┴──────────────────────┴──────────────────────┘
```

### 4.4 Multi-rate Scheduler Design

```
Scheduler đa tần số (không cần RTOS):

  ┌─────────────────────────────────────────────────────────────┐
  │ Rate    │ Trigger          │ Tasks                          │
  │─────────│──────────────────│────────────────────────────────│
  │ 20kHz   │ HRTIM → ADC ISR  │ FOC current loop (Clarke/     │
  │         │ priority 0       │ Park/PI/SVM), < 30μs          │
  │─────────│──────────────────│────────────────────────────────│
  │ 1kHz    │ TIM2 UIF ISR     │ Encoder velocity, slip detect,│
  │         │ priority 1       │ speed PI → Iq_ref, < 500μs    │
  │─────────│──────────────────│────────────────────────────────│
  │ 100Hz   │ Main loop poll   │ Deviation PI, correction L/R  │
  │         │ (tick mod 10)    │                                │
  │─────────│──────────────────│────────────────────────────────│
  │ 10Hz    │ Main loop poll   │ TMP112 I2C, CAN rx/tx,        │
  │         │ (tick mod 100)   │ fault management, telemetry    │
  └─────────────────────────────────────────────────────────────┘

  ISR preemption chain:
    ADC ISR (prio 0) có thể preempt TIM2 ISR (prio 1)
    → Không share data giữa 2 ISR → không cần mutex
    → Main loop chỉ đọc g_sensor_data (atomic writes từ ISR)

  Tương đương RTOS nhưng nhẹ hơn:
    ISR 20kHz     ≈  Highest-priority task + hardware timer
    ISR 1kHz      ≈  Medium-priority task
    Main 100Hz    ≈  Low-priority task (cooperative)
    Main 10Hz     ≈  Background/idle task
```

### 4.5 Timing Design & Analysis (Closed-loop Critical)

```
Timing budget — MUST NOT exceed:

  ┌───────────────────────────────────────────────────────┐
  │ Loop          │ Period  │ Budget    │ Actual   │ OK?  │
  │───────────────│─────────│───────────│──────────│──────│
  │ FOC 20kHz ISR │ 50μs    │ < 30μs    │ ~25μs    │ ✓    │
  │ Speed 1kHz ISR│ 1ms     │ < 500μs   │ ~100μs   │ ✓    │
  │ Deviation poll│ 10ms    │ < 5ms     │ ~50μs    │ ✓    │
  │ Temp/CAN poll │ 100ms   │ < 50ms    │ ~2ms     │ ✓    │
  └───────────────────────────────────────────────────────┘

  Điều kiện bắt buộc:
  ✦ ADC conversion hoàn thành trước ISR đọc (HRTIM trigger timing)
  ✦ FOC ISR exit trước khi HRTIM period tiếp theo trigger ADC
  ✦ DMA / ISR không overwrite data đang xử lý
  ✦ Jitter < 1μs cho FOC loop (timer hardware đảm bảo)
  ✦ Interrupt latency < 12 cycles (Cortex-M4 auto stacking)

  Deterministic timing:
    Open-loop (Ver1): processing time << period → luôn OK, không strict
    Closed-loop (Ver2): processing PHẢI < period, nếu không → instability
    → Đây là khác biệt CỐT LÕI giữa data acquisition vs control
```

### 4.6 Buffer & Data Integrity

```
Closed-loop yêu cầu data consistency cao hơn open-loop:

  ADC data (20kHz):
    ADC1/ADC2 injected → ISR đọc trực tiếp JDR1 register
    Không cần buffer (đọc ngay, dùng ngay trong cùng ISR)

  Encoder data (1kHz):
    TIM3/TIM4 CNT → delta calculation trong TIM2 ISR
    prev_L / prev_R lưu giá trị trước → atomic (single writer)

  Sensor data shared (g_sensor_data):
    ISR 20kHz ghi ia/ib/vbus → main loop đọc (display only)
    ISR 1kHz ghi velocity → main loop 100Hz đọc (deviation PI)
    → Dùng volatile + aligned write → no tearing trên 32-bit ARM

  CAN data:
    RX → FIFO hardware FDCAN (message RAM)
    TX → ghi trực tiếp vào TX FIFO element
    → Hardware buffer, không cần software double-buffer
```

### 4.7 Common Mistakes (Closed-loop Bare-metal)

```
✗ Dùng delay() trong control loop
    → Mất deadline → PID oscillation hoặc instability
    → Timer ISR tự trigger, không cần delay

✗ ADC không sync với PWM
    → Sample trúng lúc MOSFET switching → nhiễu dòng
    → PHẢI trigger ADC tại midpoint (HRTIM Master Period)

✗ Processing nặng trong ISR
    → FOC > 30μs → miss next ADC trigger → alias, instability
    → Tối ưu: LUT sin/cos, không malloc, không printf trong ISR

✗ Không phân biệt open-loop vs closed-loop
    → Closed-loop PHẢI có deadline guarantee
    → Timing violation = system failure (không chỉ mất data)

✗ Shared data không protect
    → ISR ghi + main loop đọc cùng struct → torn read
    → Dùng atomic/volatile, hoặc tách field 32-bit aligned

✗ PID integral windup
    → Khi motor saturate, integral tích lũy không giới hạn
    → Anti-windup PHẢI có: clamp integral + clamp output

✗ Encoder overflow không xử lý
    → TIM CNT 16-bit overflow → delta sai
    → Dùng signed cast: (int16_t)(CNT - prev) → auto wrap

✗ Ignoring interrupt priority
    → Nếu slow ISR chặn fast ISR → FOC miss deadline
    → Priority: ADC=0 (highest) >> TIM2=1 >> others

✗ Overcurrent protection bằng software
    → ADC → ISR → GPIO = vài μs, MOSFET có thể cháy trong 1μs
    → PHẢI dùng hardware comparator → fault input (< 100ns)
```

### 4.8 Important Insights

```
Mental Model — Closed-loop Bare-metal ↔ Digital Control System:

  HRTIM 20kHz        ≈  Master sampling clock
  ADC sync trigger   ≈  Sample & Hold circuit
  Clarke/Park        ≈  Coordinate transform (DFT-like)
  PI controller      ≈  Digital compensator (IIR filter)
  SVM                ≈  Digital-to-Analog modulator (PWM DAC)
  ISR priority chain ≈  Pipeline with priority arbitration
  g_sensor_data      ≈  Shared register file
  State machine      ≈  FSM (IDLE/RUN/FAULT)

Key principle (kế thừa Ver1):
  "Do not block. Everything flows with time."
  + "Every flow MUST respect its deadline."
```

---

## 5. Thông số tổng hợp

> **Lưu ý:** Architecture type = Closed-loop FOC, Multi-rate ISR, Deterministic timing

| Thông số | Giá trị |
|----------|---------|
| MCU | STM32G474RET6, Cortex-M4 @ 170MHz (Boost mode) |
| Flash / RAM | 512KB Flash / 96KB SRAM1 + 32KB SRAM2 |
| Motor type | BLDC / PMSM 3-phase, 4 pole pairs |
| PWM peripheral | HRTIM với DLL ×32 |
| PWM frequency | 20kHz center-aligned |
| PWM resolution | 184ps (fHRCK = 5.44GHz) / PERAR = 136,000 ticks |
| Deadtime | 100ns hardware (HRTIM DTxR = 136 ticks) |
| Overcurrent trip | COMP1 → HRTIM FLT1, < 100ns (hardware, no CPU) |
| ADC sampling | ADC1/ADC2 injected, triggered bởi HRTIM midpoint |
| ADC resolution | 12-bit, 42.5MHz (HCLK/4, synchronous mode) |
| Current sensing | OPAMP1/2 internal PGA ×4, shunt 10mΩ, ±10A |
| Current resolution | 0.02 A/LSB |
| Vbus sensing | ADC1 CH6 (PC0), divider 47k/4.7k, 8.87mV/LSB |
| Temperature | TMP112, I2C @ 100kHz, ±0.5°C, 12-bit |
| Temp warning | 75°C, Temp fault: 85°C (5°C hysteresis) |
| Encoder (wheel) | 2× Quadrature TIM3/TIM4, 500PPR × 4 = 2000 cnt/rev |
| Encoder (angle) | AS5048A SPI1 14-bit (optional FOC theta source) |
| Wheel diameter | 65mm, PULSE_TO_METER = 0.000102 m/pulse |
| Wheel base | 150mm |
| Slip threshold | Warn 30%, Fault 60% (500ms duration) |
| Deviation PI | Kp=0.5, Ki=0.05, correction ±20% |
| Control type | **Closed-loop FOC** (cascade current + speed PI) |
| Current loop | 20kHz (dt=50μs), Kp=0.5, Ki=50, limit ±24V |
| Speed loop | 1kHz (dt=1ms), Kp=0.1, Ki=0.5, limit ±10A Iq |
| Deviation loop | 100Hz (dt=10ms), PI anti-windup ±500 |
| Thermal loop | 10Hz (dt=100ms), I2C blocking OK ở slow context |
| CAN protocol | FDCAN1, 1Mbps nominal, 3 message IDs |
| CAN command | ID 0x100 speed setpoint, ID 0x1FF e-stop |
| CAN telemetry | ID 0x101 status (speed, current, temp, fault) |
| Build tool | GNU Make + arm-none-eabi-gcc, -O2, -mfloat-abi=hard |
| Flash tool | ST-Link + OpenOCD, stm32g4x.cfg |
| Code style | Register-level, zero HAL, full FPU float |
| Source files | 15 C files + 1 ASM + 1 LD + 1 Makefile |

---

## 6. Điểm mạnh & Giới hạn

### Điểm mạnh
```
✓ Closed-loop FOC thực sự
    Torque tuyến tính với Iq, tốc độ không bị ảnh hưởng bởi tải
    Cascade: current (20kHz) >> speed (1kHz) → ổn định tốt

✓ Timer-driven + deterministic architecture
    Kế thừa Ver1: non-blocking, event-driven, pipeline
    Nâng cấp: deadline guarantee, multi-rate ISR priority chain

✓ HRTIM 184ps resolution
    Khả năng điều chỉnh duty cực mịn → sóng hài thấp hơn TIM thường

✓ ADC synchronized với HRTIM midpoint
    Sample tại đỉnh carrier = thời điểm tất cả MOSFET ổn định
    → loại bỏ nhiễu switching hoàn toàn

✓ Overcurrent hardware path < 100ns
    COMP1 → HRTIM FLT1 không đi qua CPU
    Motor và MOSFET an toàn dù ISR bị disable

✓ Internal OPAMP (không cần chip khuếch đại ngoài)
    Giảm component count, BOM, noise pickup ngoại vi
    PGA ×4 calibrated internal

✓ FDCAN (CAN FD)
    Multi-node robust, hardware CRC
    Phù hợp cho robot đa motor, CNC, AGV

✓ Encoder quadrature 4× (2000 cnt/rev vs 20 cnt ver1)
    Resolution tăng 100×, chiều quay tự động, sai số nhỏ

✓ Slip detection + Deviation PI
    Bảo vệ bánh xe slipping
    Robot đi thẳng tự động trên mặt không đều

✓ Multi-rate scheduler không cần RTOS
    4 tần số khác nhau, không block, không race condition
    ISR priority chain (ADC=0 >> TIM2=1 >> main)
    Pipeline: sense → transform → control → actuate

✓ Toàn bộ float dùng FPU cứng (FPv4-SP)
    FOC computation < 30μs/cycle @ 170MHz
    Không cần thư viện CORDIC (LUT đủ cho 20kHz)

✓ Anti-windup PI controller
    Tất cả PI (id, iq, speed, deviation) đều có clamp
    Tránh integral saturation khi motor/actuator saturate
```

### Giới hạn → Dẫn đến Ver3
```
✗ Không có RTOS
    Multi-rate bằng polling → không có deadline guarantee
    Phù hợp thesis; không đủ cho safety-critical (ISO 26262)

✗ Theta từ encoder tích phân (không phải đo trực tiếp)
    Tích lũy sai số theo thời gian
    AS5048A lắp sẵn nhưng chưa integrate vào FOC pipeline chính

✗ Chưa có Field Weakening
    id_ref = 0 cố định → giới hạn tốc độ tối đa bởi Vbus
    Ver3: id < 0 để mở rộng tốc độ (flux weakening region)

✗ Không có fault logging
    Fault code đơn giản, không có persistent log, timestamp
    Ver3: CRC-protected flash log, syslog over CAN

✗ Không có Over-voltage protection
    Braking regenerative → Vbus có thể spike
    Ver3: hardware OV comparator + braking resistor control

✗ Watchdog đơn giản (SysTick, không có IWDG/WWDG)
    Ver3: IWDG với windowed mode, heartbeat từ RTOS task

✗ Không có dual-MCU safety
    Ver1 có F1 watchdog độc lập; ver2 đi một mình
    Ver3: co-processor hoặc SIL2 companion MCU

✗ Không có over-modulation / MTPA optimization
    SVM clamp 95% → headroom bị giới hạn
    Ver3: overmodulation + MTPA lookup table
```

---

## 7. Kết luận

> **Ver2 = Closed-loop FOC embedded system đúng nghĩa.**
> Hiểu được toàn bộ chuỗi: hardware timer (HRTIM) → ADC sync →
> Clarke/Park → PI cascade → SVM → 3-phase output.
> Kết hợp 4 sensor (ADC/Encoder/I2C/CAN) vào scheduler đa tần số
> không cần RTOS, bảo vệ phần cứng bằng hardware comparator.
>
> **Triết lý kiến trúc kế thừa từ Ver1:**
> Timer-driven, non-blocking, event-driven, pipeline processing.
> Bổ sung thêm: deterministic deadline, multi-rate ISR priority,
> synchronized ADC, hardware fault path.
>
> **"Do not block. Everything flows with time.
> Every flow MUST respect its deadline."**
>
> **Ver2 đủ điều kiện làm đồ án tốt nghiệp kỹ thuật.**
> Thể hiện hiểu biết sâu về: clock tree, HRTIM, FPU, FOC algorithm,
> multi-rate control, sensor fusion, và communication protocol.
>
> Mọi giới hạn của Ver2 (RTOS, functional safety, field weakening,
> dual-MCU) là nền tảng để phát triển lên Ver3 — cấp độ production.

---

## 8. ALTERNATIVE — Diymore BLDC Module (SNR8503M)

> **Mục đích section này:**
> FOC từ đầu (section 1-7) là con đường ACADEMIC — hiểu sâu.
> Section 8 là con đường PRACTICAL — dùng module có sẵn, viết
> closed-loop speed PID đơn giản, chạy được robot ngay.
> Hai hướng KHÔNG loại trừ nhau: prototype với module → sau đó
> migrate sang custom FOC khi cần tối ưu.

### 8.1 MODULE SPECIFICATION

```
═══════════════════════════════════════════════════════════════════════
  Diymore BLDC Sensorless Motor Driver — SNR8503M
═══════════════════════════════════════════════════════════════════════

  Chip điều khiển  : SNR8503M (tích hợp sensorless commutation)
  Điện áp vào      : DC 6-80V
  Dòng liên tục    : 20A (cần heatsink, module kèm tản nhiệt)
  Dòng đỉnh        : 50A
  Công suất max    : 1600W
  Commutation      : Sensorless (back-EMF zero-crossing, chip xử lý)
  Hall sensor      : KHÔNG CẦN — hall-less
  Điều khiển tốc độ: PWM (1-20kHz, 10-100%) hoặc Analog (0.5-5V)
  Giao tiếp        : UART (5V TTL)
  Open/Closed loop : Open-loop mặc định (module KHÔNG có speed PID)
  Bảo vệ tích hợp  : quá dòng, locked-rotor, đảo nguồn, quá áp, quá nhiệt MOS
  Kích thước        : 78 × 57mm
  Nhiệt độ         : -40°C ~ 105°C hoạt động

  Thermal guideline:
    < 5A  : bare board, thông gió tự nhiên
    5-20A : heatsink (kèm theo module), gắn sau MOS tube
    > 20A : heatsink lớn hơn hoặc quạt cưỡng bức
```

### 8.2 PINOUT — Module Interface

```
═══════════════════════════════════════════════════════════════════════
  MOTOR PORT (3 pha)
═══════════════════════════════════════════════════════════════════════
  Pin 1-2 : Power +  (DC 6-80V)
  Pin 3   : GND
  Pin 4   : Phase U  (motor wire)
  Pin 5   : Phase V  (motor wire)
  Pin 6   : Phase W  (motor wire)
  → Nếu chiều quay sai → hoán đổi 2 dây bất kỳ

═══════════════════════════════════════════════════════════════════════
  CONTROL PORT
═══════════════════════════════════════════════════════════════════════
  Pin 6   : GND
  Pin 7   : 5V (output từ module, cấp cho sensor/logic)
  Pin 8   : VSP/PWM — Speed command input
              Analog: 0.5V-5V (0.5V = min, 5V = max)
              PWM   : 1-20kHz, duty 10-100%
              → ĐÂY LÀ INPUT CHÍNH từ STM32
  Pin 9   : FG — Speed feedback output
              Open-drain, 5V logic
              RPM = FG_frequency_Hz × 60 / pole_number
              → ĐÂY LÀ FEEDBACK CHÍNH về STM32
  Pin 10  : GND
  Pin 11  : CW/CCW — Direction control
              Floating / High = CW (clockwise)
              GND              = CCW (counter-clockwise)
              → GPIO output từ STM32

═══════════════════════════════════════════════════════════════════════
  UART PORT (5V TTL)
═══════════════════════════════════════════════════════════════════════
  Pin 16  : GND
  Pin 17  : RX (module nhận lệnh)
  Pin 18  : TX (module gửi status)
  Pin 19  : 5V
  → CẦN level shifter 3.3V ↔ 5V (STM32 GPIO = 3.3V)

═══════════════════════════════════════════════════════════════════════
  ICP BURN PORT (lập trình chip SNR8503M — KHÔNG DÙNG)
═══════════════════════════════════════════════════════════════════════
  Pin 12-16: ICP programming — for factory firmware only, bỏ qua
```

### 8.3 LED FAULT CODES (built-in diagnostics)

```
  Module State              │ LED Pattern             │ FG Output
  ──────────────────────────│─────────────────────────│──────────────────
  Standby (no speed cmd)    │ Blink 1Hz               │ Low level
  Normal running            │ Steady ON               │ Frequency ∝ RPM
  Short-circuit fault       │ 1× blink + 2s off       │ 1× 200ms low + 2s high
  Undervoltage fault        │ 2× blink + 2s off       │ 2× 200ms low + 2s high
  Overvoltage fault         │ 3× blink + 2s off       │ 3× 200ms low + 2s high
  Locked-rotor fault        │ 4× blink + 2s off       │ 4× 200ms low + 2s high
  System bias fault         │ 5× blink + 2s off       │ 5× 200ms low + 2s high
  MOS overtemperature       │ 6× blink + 2s off       │ 6× 200ms low + 2s high
  Overcurrent fault         │ 10× blink + 2s off      │ 10× 200ms low + 2s high
  MOS self-test fail        │ 12× blink + 2s off      │ 12× 200ms low + 2s high

  → Module tự bảo vệ, STM32 KHÔNG CẦN xử lý overcurrent/OT
  → Đọc FG pattern để detect fault state (optional advanced feature)
```

### 8.4 WIRING — STM32G474 ↔ Diymore Module

```
═══════════════════════════════════════════════════════════════════════
  STM32G474 PIN ASSIGNMENT (dùng module thay FOC custom)
═══════════════════════════════════════════════════════════════════════

  ┌─────────────┐                    ┌──────────────────┐
  │ STM32G474   │                    │ Diymore Module   │
  │             │                    │ (SNR8503M)       │
  │  PA8 (TIM1) ├───── PWM 10kHz ──→│ Pin 8 (VSP/PWM)  │
  │  (AF6)      │                    │                  │
  │             │                    │                  │
  │  PA6 (TIM3) ├←── FG feedback ───│ Pin 9 (FG)       │
  │  (AF2, IC)  │  (5V open-drain)  │ (cần pull-up 5V) │
  │             │  level shift 5→3.3│                  │
  │             │                    │                  │
  │  PB0 (GPIO) ├────── CW/CCW ────→│ Pin 11 (CW/CCW)  │
  │  (push-pull)│                    │                  │
  │             │                    │                  │
  │  PA2 (UART2 ├←──── TX ─────────→│ Pin 18 (TX)      │
  │   RX, AF7)  │  level shift 5↔3.3│                  │
  │  PA3 (UART2 ├───── RX ─────────→│ Pin 17 (RX)      │
  │   TX, AF7)  │                    │                  │
  │             │                    │                  │
  │  GND ───────┼────────────────────│ Pin 6/10 (GND)   │
  └─────────────┘                    └──────────────────┘

  QUAN TRỌNG:
  ① FG output = 5V open-drain → cần voltage divider hoặc level shifter
    Đơn giản: 2.2kΩ + 3.3kΩ divider → ~3V cho STM32 input
    Hoặc: BSS138 MOSFET level shifter module
  ② CW/CCW pin = 5V input nhưng threshold thấp
    STM32 3.3V output → kiểm tra module có nhận không (thường OK)
    Nếu không → thêm NPN transistor pull-down
  ③ UART = 5V TTL → BẮT BUỘC level shifter (MAX3232 hoặc BSS138)
    STM32 UART pins KHÔNG 5V tolerant trên G474

  Peripheral dùng:
    TIM1 CH1 (PA8)  : PWM output 10kHz → speed command
    TIM3 CH1 (PA6)  : Input Capture → đo FG frequency → RPM
    USART2 (PA2/PA3): UART 9600/115200 (nếu dùng serial control)
    GPIO PB0        : CW/CCW direction toggle

  Peripheral KHÔNG CẦN (so với FOC custom):
    ✗ HRTIM         → module tự commutate
    ✗ OPAMP1/2      → module tự sense current
    ✗ COMP1         → module có built-in overcurrent
    ✗ ADC injected  → không cần synchronized current sampling
    ✗ SVM/Clarke/Park → module tự xử lý
    ✗ CORDIC/FMAC   → không cần tính sin/cos
```

### 8.5 CLOSED-LOOP SPEED CONTROL — Simple PID

```
═══════════════════════════════════════════════════════════════════════
  ARCHITECTURE: STM32 = supervisor, Module = commutator
═══════════════════════════════════════════════════════════════════════

  Khác biệt fundamental:
    FOC custom (section 1-7): STM32 điều khiển TỪNG MOSFET, 20kHz
    Module mode (section 8) : STM32 chỉ gửi "tốc độ mong muốn", 100Hz
                               Module tự commutate, tự bảo vệ

  Control loop đơn giản:

    ┌─────────┐    PWM duty    ┌───────────┐    3-phase   ┌───────┐
    │ STM32   ├───────────────→│ Diymore   ├─────────────→│ BLDC  │
    │ PID     │    (10kHz)     │ SNR8503M  │              │ Motor │
    │ 100Hz   │                │ (sensorless│              │       │
    │         │←───────────────┤  commut.)  │←─ back-EMF ─┤       │
    └────┬────┘   FG feedback  └───────────┘              └───────┘
         │
         │ RPM = FG_freq × 60 / pole_pairs
         │
    error = target_RPM - actual_RPM
    output = Kp×error + Ki×∫error + Kd×Δerror
    PWM_duty = clamp(output, 10%, 95%)

═══════════════════════════════════════════════════════════════════════
  ALGORITHM — Pseudo-code (main loop @ 100Hz)
═══════════════════════════════════════════════════════════════════════

  // ──── INIT ────
  TIM1_PWM_Init(10000);        // 10kHz PWM on PA8
  TIM3_InputCapture_Init();    // IC on PA6 (FG input)
  GPIO_Init(PB0, OUTPUT);      // CW/CCW direction
  SysTick_Init(10ms);          // 100Hz scheduler tick

  target_rpm = 0;
  pid.Kp = 0.5f;
  pid.Ki = 0.1f;
  pid.Kd = 0.0f;
  pid.integral = 0.0f;
  pid.prev_error = 0.0f;
  pid.output_min = 10.0f;      // module minimum = 10% duty
  pid.output_max = 95.0f;

  // ──── MAIN LOOP (100Hz) ────
  while (1) {
      if (!tick_100hz) continue;
      tick_100hz = 0;

      // ① Đo tốc độ thực tế từ FG
      fg_period_us = TIM3_GetCapturePeriod();   // μs giữa 2 rising edge
      if (fg_period_us > 0) {
          fg_freq_hz = 1000000.0f / fg_period_us;
          actual_rpm = fg_freq_hz * 60.0f / POLE_PAIRS;
      } else {
          actual_rpm = 0;    // motor đứng yên hoặc FG mất
      }

      // ② PID controller
      error = target_rpm - actual_rpm;
      pid.integral += error * dt;                // dt = 0.01s
      pid.integral = clamp(pid.integral, -500, 500);  // anti-windup
      derivative = (error - pid.prev_error) / dt;
      pid.prev_error = error;

      output = pid.Kp * error
             + pid.Ki * pid.integral
             + pid.Kd * derivative;

      duty_percent = clamp(output, pid.output_min, pid.output_max);

      // ③ Ghi PWM duty
      if (target_rpm == 0) {
          TIM1_SetDuty(0);       // dừng motor
      } else {
          TIM1_SetDuty(duty_percent);
      }

      // ④ Direction (nếu cần đổi chiều)
      if (target_rpm >= 0) {
          GPIO_Write(PB0, HIGH);   // CW
      } else {
          GPIO_Write(PB0, LOW);    // CCW
          target_rpm = -target_rpm; // PID dùng magnitude
      }

      // ⑤ Safety check (optional, module tự bảo vệ)
      if (fg_period_us == 0 && duty_percent > 20) {
          // Motor không quay dù đang drive → locked rotor?
          stall_counter++;
          if (stall_counter > 200) {  // 2 giây
              TIM1_SetDuty(0);        // dừng
              state = FAULT;
          }
      } else {
          stall_counter = 0;
      }
  }

  // ──── TIM3 INPUT CAPTURE ISR ────
  void TIM3_IRQHandler(void) {
      if (TIM3->SR & TIM_SR_CC1IF) {
          uint32_t capture = TIM3->CCR1;
          fg_period_us = capture - last_capture;
          last_capture = capture;
          TIM3->SR &= ~TIM_SR_CC1IF;
      }
  }

═══════════════════════════════════════════════════════════════════════
  PID TUNING GUIDE
═══════════════════════════════════════════════════════════════════════

  Plant (module + motor combo):
    Input  : PWM duty (10-95%)
    Output : RPM (đo qua FG)
    Đặc tính: ~first-order với delay
      τ_mech ≈ 200-500ms (phụ thuộc tải + quán tính)
      Delay  ≈ 20-50ms  (module xử lý sensorless + ramp)

  Tuning strategy (Ziegler-Nichols simplified):
    1. Set Ki=0, Kd=0
    2. Tăng Kp từ 0.1 cho đến khi RPM oscillate → Kp_critical
    3. Kp = 0.5 × Kp_critical
    4. Ki = 0.05 ~ 0.2 (bắt đầu nhỏ, tăng dần)
    5. Kd = 0 (thường không cần cho speed control motor)

  Starting values (safe):
    │ Parameter │ Value  │ Ý nghĩa                    │
    │───────────│────────│────────────────────────────│
    │ Kp        │ 0.5    │ Proportional               │
    │ Ki        │ 0.1    │ Integral (bù steady-state) │
    │ Kd        │ 0.0    │ Derivative (skip)          │
    │ dt        │ 10ms   │ Control period (100Hz)     │
    │ Duty min  │ 10%    │ Module minimum             │
    │ Duty max  │ 95%    │ Safety headroom            │
    │ Anti-windup│ ±500  │ Integral clamp             │

═══════════════════════════════════════════════════════════════════════
  FG → RPM CALCULATION
═══════════════════════════════════════════════════════════════════════

  FG output: 1 pulse per electrical revolution
  Với motor 4 pole pairs (8 cực):
    1 mechanical revolution = 4 electrical revolutions = 4 FG pulses

    RPM = FG_freq_Hz × 60 / pole_pairs
        = FG_freq_Hz × 60 / 4
        = FG_freq_Hz × 15

  Ví dụ:
    FG = 100Hz → RPM = 100 × 15 = 1500 RPM
    FG = 200Hz → RPM = 200 × 15 = 3000 RPM
    FG = 10Hz  → RPM = 10  × 15 = 150  RPM

  Resolution: ở 150 RPM, FG period = 100ms → đủ cho 100Hz loop
  Ở RPM thấp (< 60 RPM, FG < 4Hz) → period > 250ms → lag lớn
  → Giới hạn min RPM controllable ≈ 100 RPM (FG ≈ 6.7Hz)

  TIM3 Input Capture setup:
    Prescaler: 169 → TIM3 clock = 1MHz (1μs resolution)
    ARR: 0xFFFF (65535) → max measurable period = 65ms → min FG ≈ 15Hz
    Nếu cần đo FG thấp hơn: PSC=1699 → 100kHz (10μs), max 655ms

═══════════════════════════════════════════════════════════════════════
  SO SÁNH: FOC Custom vs Module Mode
═══════════════════════════════════════════════════════════════════════

  │ Tiêu chí              │ FOC Custom (§1-7)    │ Module (§8)          │
  │───────────────────────│──────────────────────│──────────────────────│
  │ Motor commutation     │ STM32 tự xử lý      │ SNR8503M xử lý      │
  │ Current sensing       │ Shunt + OPAMP + ADC  │ Module tích hợp     │
  │ Overcurrent protect   │ COMP1→HRTIM (<100ns) │ Module tích hợp     │
  │ Speed control         │ Cascade PI (20kHz+   │ Simple PID (100Hz)  │
  │                       │ 1kHz)                │                      │
  │ Độ phức tạp firmware  │ ~3000 LOC            │ ~300 LOC             │
  │ Peripheral cần        │ HRTIM,OPAMP,COMP,    │ TIM1(PWM),TIM3(IC), │
  │                       │ ADC,TIM,SPI,I2C,CAN │ GPIO, UART           │
  │ Hiệu suất inverter   │ Tối ưu (SVM, FOC)   │ Phụ thuộc module    │
  │ Khả năng debug motor  │ Full (xem Id,Iq,θ)  │ Limited (RPM only)  │
  │ Giá trị học thuật     │ Rất cao              │ Trung bình           │
  │ Thời gian prototype   │ 2-4 tuần             │ 2-3 ngày             │
  │ Giá module            │ ~200k (BOM custom)   │ ~410k (Shopee)       │
  │ Regen braking         │ Cần tự thiết kế      │ Không hỗ trợ        │
  │ Field weakening       │ Có thể (Id < 0)      │ Không               │
  │ Sensorless start      │ Cần code observer    │ Module tự xử lý     │

  Recommendation:
    → Prototype nhanh, test cơ khí → dùng Module (§8)
    → Đồ án tốt nghiệp, hiểu sâu → dùng FOC Custom (§1-7)
    → Production → FOC Custom + RTOS (Ver3)
```

### 8.6 SENSORLESS STARTING — Lưu ý quan trọng

```
  Module SNR8503M dùng sensorless commutation (back-EMF zero-crossing).
  Back-EMF TỈ LỆ với tốc độ → ở tốc độ 0-thấp KHÔNG CÓ back-EMF.

  Module xử lý startup bằng chuỗi "alignment + open-loop ramp":
    1. Alignment: Lock rotor ở vị trí cố định (1 vector, ~500ms)
    2. Open-loop ramp: Tăng tần số commutation dần, duty thấp
    3. Back-EMF detect: Khi RPM đủ cao → chuyển sang closed-loop sensorless

  Factors ảnh hưởng startup:
    ✦ Phase change angle (commutation advance)
    ✦ Phase R, L của motor
    ✦ Tải khởi động (high inertia → khó start)
    ✦ Supply voltage (quá thấp → không đủ torque)

  Nếu motor KHÔNG START được:
    ① Kiểm tra wiring (hoán đổi 2 phase dây)
    ② Giảm tải (thả robot lên, chạy không tải trước)
    ③ Tăng supply voltage
    ④ Chiết áp vặn từ MIN lên từ từ (không nhảy đột ngột)

  [NOTE: PID closed-loop (§8.5) CHỈ HOẠT ĐỘNG sau khi module
   đã start thành công và có back-EMF. Ở giai đoạn startup,
   module chạy open-loop — STM32 KHÔNG CAN THIỆP.
   → Set PID output minimum = 10% để module luôn có đủ
     duty cho sensorless startup algorithm.]
```
