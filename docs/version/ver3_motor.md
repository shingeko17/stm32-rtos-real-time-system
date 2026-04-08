# VER3 — BLDC Motor Control System: Full Architecture + Control + Power
### Embedded R&D Level — Production-Grade Cyber-Physical System

---

## 1. INPUT — Đề bài & Yêu cầu

### 1.1 Bài toán đặt ra

```
Bài toán:
  Xây dựng HỆ THỐNG ĐIỀU KHIỂN BLDC MOTOR HOÀN CHỈNH — bao gồm:
    ✦ Power Electronics (từ nguồn vào → DC bus → inverter → motor)
    ✦ Control Firmware (closed-loop FOC cascade + protection)
    ✦ Real-time Processing (deterministic timing, safety guarantees)
    ✦ Sensor Feedback System (current, voltage, position, temperature)
    ✦ Communication Interface (CAN, UART, SPI/I2C)
  
  Đây KHÔNG phải "mạch điều khiển motor" đơn lẻ.
  → Đây là CYBER-PHYSICAL SYSTEM hoàn chỉnh.

Triết lý Ver3 (kế thừa + mở rộng):
  Ver1: "Do not block. Everything flows with time."
  Ver2: + "Every flow MUST respect its deadline."
  Ver3: + "Every subsystem must be designed, powered, protected,
           and integrated — from wall socket to shaft torque."

Ràng buộc thiết kế (kế thừa Ver2 + bổ sung):
  - Không dùng HAL/CubeIDE → viết thanh ghi trực tiếp
  - Không dùng RTOS (bare-metal) hoặc tùy chọn RTOS cho production
  - Tự viết startup, linker script, clock tree
  - FOC inner loop @10–20kHz, hoàn thành < 30μs
  - Overcurrent ngắt PWM < 100ns (hardware COMP → HRTIM FLT)
  - FPU cứng (Cortex-M4 FPv4-SP) cho tất cả floating-point
  - MỚI Ver3: Power supply stage tự thiết kế (Buck/Boost/PFC)
  - MỚI Ver3: Inverter hardware tự thiết kế (MOSFET + Gate Driver)
  - MỚI Ver3: Field Weakening, SVPWM advanced, over-modulation
  - MỚI Ver3: Fault logging persistent (Flash CRC-protected)
  - MỚI Ver3: Watchdog IWDG windowed + optional dual-MCU safety
```

### 1.2 Cần gì (Requirements) — Kế thừa Ver2 + Mở rộng

| #   | Yêu cầu                              | Chi tiết                                                | Nguồn     |
|-----|--------------------------------------|--------------------------------------------------------|-----------|
| F1  | FOC 3-phase current control          | Clarke/Park/PI/SVM @ 10–20kHz inner loop               | Ver2      |
| F2  | Closed-loop speed control            | Speed PI @ 1kHz outer loop → Iq setpoint               | Ver2      |
| F3  | Encoder 2 bánh (quadrature)          | TIM3 (trái) + TIM4 (phải), 500PPR × 4 = 2000 cnt/rev  | Ver2      |
| F4  | Wheel slip detection                 | Slip ratio > 30% warn, > 60% fault                     | Ver2      |
| F5  | Straight-line deviation PI           | ΔPulse_L − ΔPulse_R → PI correction @ 100Hz            | Ver2      |
| F6  | Current sensing via OPAMP            | Internal PGA ×4, 2 phase, shunt 10mΩ                   | Ver2      |
| F7  | Vbus monitoring                      | ADC regular CH, resistor divider                        | Ver2      |
| F8  | Overcurrent hardware trip            | COMP → HRTIM FLT, < 100ns, không qua CPU               | Ver2      |
| F9  | Temperature sensor TMP112            | I2C, ±0.5°C, warn 75°C / fault 85°C                    | Ver2      |
| F10 | FDCAN speed command                  | 0x100 setpoint; 0x101 status; 0x1FF e-stop              | Ver2      |
| F11 | AS5048A absolute encoder SPI         | 14-bit, SPI1, theta source cho FOC                      | Ver2      |
| **F12** | **Power supply stage (DC bus)**  | **Buck/Boost/Buck-Boost, voltage regulation loop**      | **Ver3 MỚI** |
| **F13** | **3-Phase inverter hardware**    | **6× MOSFET + gate driver IC, self-designed**           | **Ver3 MỚI** |
| **F14** | **Field Weakening (Id < 0)**     | **Mở rộng vùng tốc độ trên base speed**                | **Ver3 MỚI** |
| **F15** | **SVPWM advanced + over-modulation** | **Tăng utilization Vbus, 6-step transition**       | **Ver3 MỚI** |
| **F16** | **Persistent fault logging**     | **Flash CRC-protected log, syslog over CAN**            | **Ver3 MỚI** |
| **F17** | **IWDG windowed watchdog**       | **Heartbeat, dual-MCU safety option**                   | **Ver3 MỚI** |
| **F18** | **Over-voltage protection**      | **Regen braking → Vbus spike → brake resistor**         | **Ver3 MỚI** |
| **F19** | **MTPA optimization**            | **Maximum Torque Per Ampere lookup**                    | **Ver3 MỚI** |
| **F20** | **Soft-start / pre-charge**      | **Gradual DC bus charge, inrush prevention**            | **Ver3 MỚI** |
| NF1 | Clock 170MHz boost mode              | HSE 8MHz → PLL → Cortex-M4 max                         | Ver2      |
| NF2 | HRTIM 20kHz center-aligned           | 184ps resolution, ADC sync tại midpoint                 | Ver2      |
| NF3 | Multi-rate scheduler                 | 20kHz ISR / 1kHz ISR / 100Hz poll / 10Hz poll           | Ver2      |
| NF4 | Build toolchain                      | GNU Make + arm-none-eabi-gcc -O2 -mfloat-abi=hard       | Ver2      |
| **NF5** | **Deterministic real-time**      | **WCET analysis, deadline guarantee**                   | **Ver3 MỚI** |
| **NF6** | **Safety integrity (optional)**  | **Toward SIL2/ISO 26262 ASIL-B compliance**             | **Ver3 MỚI** |

### 1.3 Có gì (Available Resources) — Mở rộng với Power + Hardware

```
Hardware (kế thừa Ver2 + bổ sung):
  MCU       : STM32G474RET6 (Cortex-M4 @ 170MHz, 512KB Flash, 96KB SRAM1,
                              32KB SRAM2, HRTIM, 5× OPAMP, 7× COMP, CORDIC, FMAC)
  
  ===== VER3 MỚI: POWER STAGE =====
  Power In  : Battery (24V/48V/72V) hoặc AC-DC rectified
  PSU Stage : Buck/Boost/Buck-Boost converter (tự thiết kế)
              + Optional PFC front-end (cho AC input systems)
  DC Bus    : Large electrolytic capacitors (ripple reduction + transient)
              + Pre-charge circuit (soft-start inrush prevention)
              + TVS diodes / MOV (surge protection)
              + Brake resistor + MOSFET (regenerative over-voltage)
  
  ===== VER3 MỚI: INVERTER STAGE =====
  Inverter  : 3-phase full-bridge (6× MOSFET, tự chọn + layout)
              Gate driver IC (e.g. IR2110, DRV8302, IR2103)
              Bootstrap capacitors (high-side drive)
              Snubber circuits (dV/dt protection)
  
  ===== KẾ THỪA VER2 =====
  Motor     : BLDC / PMSM 3-pha (4 cặp cực - FOC_POLE_PAIRS = 4)
  Sensor A  : Current shunt 10mΩ × 2 (Phase A + B) → OPAMP1/OPAMP2 (PGA ×4)
  Sensor B  : Wheel encoder quadrature × 2 (500PPR × 4 = 2000 cnt/rev)
  Sensor C  : TMP112 digital temperature via I2C1 (±0.5°C)
  Sensor D  : AS5048A absolute magnetic encoder via SPI1 (14-bit)
  Sensor E  : Hall sensors × 3 (Ver3 mới: coarse rotor position for commutation)
  Bus       : FDCAN1 @ 1Mbps (SN65HVD230 / TCAN1044 transceiver)
  Power     : DC bus 12–72V (Vbus via resistor divider → ADC)

Software (kế thừa Ver2 + bổ sung):
  ===== PLATFORM LAYER (Ver2) =====
  startup_stm32g474.s    → Startup assembly (102 IRQ vector table G474)
  stm32g474.ld           → Linker script (Flash/SRAM1/SRAM2 layout)
  system_stm32g4xx.c     → Clock init SystemInit, PLL 170MHz

  ===== BSP LAYER (Ver2 + Ver3 bổ sung) =====
  bsp_hrtim.c/h          → HRTIM 3-phase 20kHz + ADC trigger + FLT overcurrent
  bsp_opamp.c/h          → OPAMP1/2/3 PGA mode ×4 internal
  bsp_comp.c/h           → COMP1 overcurrent + COMP2 overvoltage (Ver3 mới)
  bsp_adc_sync.c/h       → ADC1/ADC2 injected, HRTIM triggered
  bsp_spi.c/h            → SPI1 (AS5048A)
  bsp_i2c.c/h            → I2C1 (TMP112)
  bsp_fdcan.c/h          → FDCAN1 speed cmd + telemetry
  bsp_iwdg.c/h           → VER3 MỚI: IWDG windowed watchdog
  bsp_flash_log.c/h      → VER3 MỚI: Persistent fault logging (CRC)
  bsp_brake.c/h          → VER3 MỚI: Brake resistor MOSFET control

  ===== SENSOR LAYER (Ver2) =====
  sensor_encoder.c/h     → TIM3/TIM4 quadrature + velocity + slip + deviation
  sensor_hall.c/h        → VER3 MỚI: Hall sensor commutation (3× GPIO / TIM IC)
  sensor_temp.c/h        → TMP112 I2C driver
  sensor_manager.c/h     → Master SensorData_t + multi-rate dispatch

  ===== CONTROL LAYER (Ver2 + Ver3 advanced) =====
  pi_controller.c/h      → Generic PI with anti-windup
  clarke_park.c/h        → Clarke / Park / InvPark / SVM / FastSinCos LUT
  foc_control.c/h        → FOC context, inner/outer PI, state machine
  field_weakening.c/h    → VER3 MỚI: Id < 0 flux weakening control
  svpwm_advanced.c/h     → VER3 MỚI: Over-modulation + 6-step transition
  mtpa_lut.c/h           → VER3 MỚI: Maximum Torque Per Ampere table

  ===== APPLICATION LAYER =====
  motor_foc_main.c       → ISR @ 20kHz + 1kHz, main loop 100Hz + 10Hz
  power_manager.c/h      → VER3 MỚI: DC bus voltage regulation + soft-start
  safety_monitor.c/h     → VER3 MỚI: Watchdog + dual-MCU heartbeat
  Makefile               → GNU Make, CPU Cortex-M4 + FPU hard, OpenOCD flash
```

---

## 2. SYSTEM OVERVIEW — Kiến trúc tổng thể

### 2.1 High-level Architecture (Power → Control → Motor)

```
═══════════════════════════════════════════════════════════════════════
  VER3 FULL SYSTEM ARCHITECTURE (End-to-End)
═══════════════════════════════════════════════════════════════════════

  ┌──────────┐    ┌──────────────────┐    ┌────────┐    ┌────────────────────┐    ┌───────┐
  │ Power    │    │ Power Supply     │    │ DC Bus │    │ 3-Phase Inverter   │    │ BLDC  │
  │ Input    │───→│ (Buck/Boost/PFC) │───→│ (Caps) │───→│ (6× MOSFET +       │───→│ Motor │
  │ (Battery/│    │ Voltage Reg Loop │    │ Filter │    │  Gate Driver)      │    │       │
  │  AC-DC)  │    │ Current Limiting │    │ TVS    │    │  PWM from MCU      │    │       │
  └──────────┘    └──────────────────┘    └───┬────┘    └─────────┬──────────┘    └───┬───┘
                                              │                   │                    │
                                              │                   │    ┌───────────────┘
                                              │                   │    │
                                              │              ┌────┴────┴──────────────┐
                                              │              │       MCU              │
                                              │              │  (STM32G474 @ 170MHz)  │
                                              │              │                        │
                                              └──────────────┤  ADC: Vbus monitoring  │
                                                             │  ADC: Phase current    │
                                              ┌──────────────┤  PWM: HRTIM → Inverter│
                                              │              │  PI:  FOC cascade      │
                                              │              │  CAN: Command/Telemetry│
                                              │              │  WDG: Safety monitor   │
                                              │              └────────────────────────┘
                                              │
                                        ┌─────┴──────┐
                                        │  Sensors   │
                                        │ Hall / Enc │
                                        │ Current    │
                                        │ Temp / Vbus│
                                        └────────────┘

  FEEDBACK LOOP (liên tục, closed-loop):
    [Motor] → [Sensors] → [MCU ADC] → [FOC Algorithm] → [PWM] → [Inverter] → [Motor]
                                ↑                            ↑
                            position                     voltage/duty
                            current                      3-phase
                            temperature
                            bus voltage

  System type:
    ✦ Embedded Control System
    ✦ Closed-loop Motor Drive
    ✦ Real-time System (hard timing constraints)
    ✦ Cyber-Physical System (firmware + power electronics + mechanics)
```

### 2.2 System Type Definition

```
  Đây KHÔNG chỉ là "firmware".
  Đây KHÔNG chỉ là "mạch điện".
  → Đây là CYBER-PHYSICAL SYSTEM (CPS).

  Full system = tổ hợp của:
    ① Power Electronics    — chuyển đổi năng lượng, bảo vệ phần cứng
    ② Control Firmware     — giải thuật FOC, PI cascade, state machine
    ③ Real-time Processing — deadline guarantee, multi-rate scheduling
    ④ Feedback System      — sensor, ADC sync, signal conditioning
    ⑤ Communication        — CAN bus, UART debug, SPI/I2C sensor
    ⑥ Mechanical Load      — motor + gearbox + wheel + inertia + friction

  Mỗi subsystem ảnh hưởng lẫn nhau:
    Power supply ripple → ADC noise → control jitter
    Motor inductance    → current loop bandwidth
    Inverter deadtime   → torque distortion
    Sensor latency      → phase lag → instability
    → PHẢI thiết kế ĐỒNG THỜI, không thể tách rời
```

---

## 3. POWER SYSTEM — Nền tảng quan trọng (VER3 MỚI)

> **Ver2 bỏ qua Power System — giả định DC bus sẵn có.**
> **Ver3 thiết kế từ nguồn vào đến DC bus, bao gồm bảo vệ.**
> Đây là phần CỐT LÕI mà engineer thực tế PHẢI hiểu.

### 3.1 Power Input

```
Nguồn cấp cho hệ thống motor:

  ┌───────────────────────────────────────────────────────────────────┐
  │ Option A: Battery (DC trực tiếp)                                 │
  │   Voltage range: 12V, 24V, 48V, 72V tùy ứng dụng              │
  │   Đặc tính: điện áp thay đổi theo SoC (State of Charge)        │
  │   Ví dụ: LiPo 12S = 50.4V full → 36V empty                     │
  │   → Cần Buck/Boost để ổn định DC bus nếu voltage swing lớn     │
  │                                                                   │
  │ Option B: AC-DC Rectified                                        │
  │   AC wall → bridge rectifier → DC raw                            │
  │   220VAC → ~310VDC (sau rectify, chưa filter)                    │
  │   → Cần PFC (Power Factor Correction) cho compliance EMI        │
  │   → Sau đó Buck xuống Vbus mong muốn (48V, 72V...)             │
  └───────────────────────────────────────────────────────────────────┘
```

### 3.2 Power Supply Stage

```
═══════════════════════════════════════════════════════════════════════
  DC-DC CONVERTER — Ổn định DC Bus Voltage
═══════════════════════════════════════════════════════════════════════

  Topology tùy theo Vin vs Vbus_target:

  ┌────────────────────────────────────────────────────────────────┐
  │ Trường hợp     │ Vin         │ Vbus target │ Topology         │
  │────────────────│─────────────│─────────────│──────────────────│
  │ Battery > Vbus │ 48-72V      │ 48V         │ Buck (step-down) │
  │ Battery < Vbus │ 12-24V      │ 48V         │ Boost (step-up)  │
  │ Battery ≈ Vbus │ 36-50V      │ 48V         │ Buck-Boost       │
  │ AC input       │ 310VDC raw  │ 48V         │ PFC + Buck       │
  └────────────────────────────────────────────────────────────────┘

  Buck converter (phổ biến nhất cho robot battery):
    ┌─────────────────────────────────────────────────────┐
    │                                                     │
    │   Vin ──┬── [Q_high] ──┬── [L] ── ┬── Vbus_out    │
    │         │              │          │               │
    │         │        [Q_low/Diode]    [C_out]         │
    │         │              │          │               │
    │         └──────────────┴──────────┴── GND         │
    │                                                     │
    │   Q_high: PWM switching (100-500kHz)               │
    │   L: energy storage inductor                       │
    │   C_out: output filter capacitor                   │
    │   Duty = Vbus_out / Vin                            │
    └─────────────────────────────────────────────────────┘

  Optional PFC (cho AC systems):
    AC → bridge rectifier → PFC boost → 400VDC regulated
    → Sau đó Buck xuống Vbus motor
    → Đảm bảo Power Factor > 0.95, EMI compliance
```

### 3.3 DC Bus

```
═══════════════════════════════════════════════════════════════════════
  DC BUS — Backbone năng lượng của hệ thống
═══════════════════════════════════════════════════════════════════════

  Thành phần chính:

  ┌───────────────────────────────────────────────────────────────┐
  │ Large Electrolytic Capacitors                                 │
  │   Mục đích:                                                   │
  │   ✦ Giảm ripple voltage (từ switching converter)             │
  │   ✦ Cung cấp transient current (khi motor tăng tốc đột ngột)│
  │   ✦ Hấp thụ energy regenerative (khi motor giảm tốc)        │
  │                                                               │
  │   Sizing: C ≥ I_peak × dt / ΔV_allowed                      │
  │   Ví dụ: 20A × 50μs / 1V = 1000μF                           │
  │   Thực tế: 470μF~2200μF, 63V rated (48V bus + margin)       │
  │   ESR quan trọng: low-ESR cho high-frequency ripple          │
  └───────────────────────────────────────────────────────────────┘

  ┌───────────────────────────────────────────────────────────────┐
  │ Pre-charge Circuit (VER3 MỚI — Soft-start)                   │
  │                                                               │
  │   Vấn đề: Khi bật nguồn, capacitor = 0V                     │
  │   → Dòng inrush = Vin / R_wire → hàng trăm Ampere           │
  │   → Nổ relay, cháy connector, trigger fuse                   │
  │                                                               │
  │   Giải pháp:                                                  │
  │   Vin ──[R_precharge]──[C_bus]                               │
  │           ↕                                                   │
  │   Vin ──[Relay_main]───[C_bus]                               │
  │                                                               │
  │   Sequence:                                                   │
  │   1. Đóng relay qua R_precharge (dòng nhỏ, charge chậm)     │
  │   2. Chờ Vbus ≥ 90% Vin (ADC monitor)                       │
  │   3. Đóng relay main (bypass R_precharge)                    │
  │   4. Hệ thống sẵn sàng                                      │
  │                                                               │
  │   MCU control:                                                │
  │     GPIO → relay driver (MOSFET / optocoupler)               │
  │     ADC → monitor Vbus during charge                         │
  │     Timeout: nếu Vbus không đạt 90% trong 5s → fault        │
  └───────────────────────────────────────────────────────────────┘

  ┌───────────────────────────────────────────────────────────────┐
  │ Protection Components                                         │
  │                                                               │
  │   TVS Diode: clamp voltage spike > Vbus_max                  │
  │   MOV (Metal Oxide Varistor): absorb high-energy transients  │
  │   Fuse: last-resort overcurrent protection (sau tất cả)     │
  │   Reverse polarity: P-MOSFET hoặc diode (cho battery input)  │
  └───────────────────────────────────────────────────────────────┘
```

### 3.4 Power Control Algorithm (QUAN TRỌNG)

```
═══════════════════════════════════════════════════════════════════════
  VOLTAGE REGULATION LOOP — Ổn định DC Bus
═══════════════════════════════════════════════════════════════════════

  Mục tiêu: Vbus = Vbus_ref (e.g. 48V) bất kể tải motor thay đổi

  Control loop (chạy riêng, tần số thấp hơn FOC):

    V_error = V_ref - V_bus_measured
    
    PI controller:
      duty_psu = Kp_v × V_error + Ki_v × ∫V_error
    
    duty_psu → PWM switching element (Buck/Boost Q_high)
    → Vbus ổn định

  Frequency: 10–50kHz (PSU switching, KHÁC với FOC 20kHz)
  Bandwidth: ~1kHz (chậm hơn FOC current loop 10×)
  → Không xung đột timing với FOC loop

═══════════════════════════════════════════════════════════════════════
  CURRENT LIMITING — Bảo vệ nguồn + dây dẫn
═══════════════════════════════════════════════════════════════════════

  Nếu I_bus > I_max (đo qua shunt + ADC trên đường DC bus):
    → Giảm duty NGAY LẬP TỨC (không chờ PI settle)
    → Override voltage loop output
    → Ưu tiên: protection > regulation

  Implementation:
    if (I_bus_measured > I_BUS_MAX) {
        duty_psu = duty_psu × 0.5;   // giảm 50% ngay
        fault_flag |= PSU_OVERCURRENT;
    }

═══════════════════════════════════════════════════════════════════════
  SOFT-START SEQUENCE — Tăng Vbus dần dần
═══════════════════════════════════════════════════════════════════════

  Tại sao cần: tránh inrush + cho capacitor charge ổn định

  Algorithm:
    V_ref_ramp = 0;
    while (V_ref_ramp < V_ref_target) {
        V_ref_ramp += V_STEP;          // +1V mỗi 10ms
        run_voltage_PI(V_ref_ramp);
        delay_ms(10);
    }
    // Vbus đã đạt target → enable inverter

  Timeline:
    t=0     : Power ON, pre-charge relay đóng
    t=0~2s  : Capacitor charge qua R_precharge
    t=2s    : Main relay đóng, Vbus ≈ Vin
    t=2~3s  : Soft-start ramp Vbus_ref (nếu có Buck stage)
    t=3s    : Vbus stable → enable HRTIM → FOC ready

═══════════════════════════════════════════════════════════════════════
  OVER-VOLTAGE PROTECTION — Regenerative Braking (VER3 MỚI)
═══════════════════════════════════════════════════════════════════════

  Vấn đề: Khi motor giảm tốc, nó trở thành GENERATOR
    → Năng lượng quay → điện → nạp ngược vào DC bus
    → Vbus TĂNG cao hơn nguồn cung cấp
    → Capacitor + MOSFET có thể CHÁY nếu Vbus > rating

  Giải pháp: Brake Resistor + Chopper MOSFET

    Vbus ──┬── [DC Bus]
           │
           ├── [Q_brake] ── [R_brake] ── GND
           │
           └── [Inverter] → [Motor]

  Control logic:
    if (V_bus > V_BRAKE_THRESHOLD) {      // e.g. 52V cho bus 48V
        GPIO_Set(Q_BRAKE, ON);            // dump energy vào R_brake
    }
    if (V_bus < V_BRAKE_THRESHOLD - 2V) { // hysteresis 2V
        GPIO_Set(Q_BRAKE, OFF);
    }

  Hardware backup:
    COMP2 → comparator, Vbus > absolute max → hardware latch Q_brake
    → Không cần CPU, giống COMP1 overcurrent
    → Latency < 200ns

  R_brake sizing:
    P_brake = (V_bus_max)² / R_brake
    Ví dụ: 55V² / 10Ω = 302W → cần resistor ≥ 300W (có tản nhiệt)
    Duty cycle thấp → average power thấp hơn nhiều
```

---

## 4. INVERTER — Power Stage (VER3 MỚI — Tự thiết kế)

### 4.1 Topology

```
═══════════════════════════════════════════════════════════════════════
  3-PHASE HALF-BRIDGE INVERTER (6 MOSFETs)
═══════════════════════════════════════════════════════════════════════

       Vbus (+)
        │
    ┌───┴───────────┬───────────────┬───────────────┐
    │               │               │               │
  [Q1_H]          [Q3_H]          [Q5_H]           │
  (High-A)        (High-B)        (High-C)          │
    │               │               │               │
    ├── Phase A     ├── Phase B     ├── Phase C     │
    │   → Motor     │   → Motor     │   → Motor     │
    │               │               │               │
  [Q2_L]          [Q4_L]          [Q6_L]           │
  (Low-A)         (Low-B)         (Low-C)           │
    │               │               │               │
    └───┬───────────┴───────────────┴───────────────┘
        │
       GND

  Mỗi half-bridge = 1 high-side + 1 low-side MOSFET
  3 half-bridges = 6 MOSFETs tổng cộng
  
  QUAN TRỌNG: High-side và low-side CÙNG 1 PHA không bao giờ ON đồng thời
  → Nếu vi phạm → SHOOT-THROUGH → dòng ngắn mạch Vbus → nổ MOSFET
  → Dead-time insertion BẮT BUỘC (100ns minimum)
```

### 4.2 Gate Driver IC

```
═══════════════════════════════════════════════════════════════════════
  GATE DRIVER — Giao tiếp MCU PWM → MOSFET Gate
═══════════════════════════════════════════════════════════════════════

  Tại sao cần Gate Driver:
    MCU GPIO = 3.3V, max ~20mA → KHÔNG ĐỦ để drive MOSFET gate
    MOSFET gate cần: 10-12V, peak 1-2A (capacitive load, fast switching)
    High-side MOSFET cần floating supply (Vgs referenced to source, not GND)

  ┌───────────────────────────────────────────────────────────┐
  │ Gate Driver IC phổ biến:                                  │
  │                                                           │
  │ IC          │ Channels  │ Vboot  │ Features              │
  │─────────────│───────────│────────│───────────────────────│
  │ IR2110      │ 1H + 1L   │ 600V   │ Classic, cheap        │
  │ IR2103      │ 1H + 1L   │ 600V   │ + deadtime built-in   │
  │ DRV8302     │ 3H + 3L   │ 60V    │ + current sense amp   │
  │ DRV8353     │ 3H + 3L   │ 100V   │ + SPI config          │
  │ FD6288      │ 3H + 3L   │ 250V   │ Cheap, compact        │
  │                                                           │
  │ Lựa chọn phụ thuộc Vbus:                                 │
  │   Vbus ≤ 60V  → DRV8302/DRV8353 (all-in-one, SPI)       │
  │   Vbus ≤ 100V → IR2110 × 3 (discrete, flexible)          │
  │   Vbus > 100V → gate driver module chuyên dụng           │
  └───────────────────────────────────────────────────────────┘

  Bootstrap circuit (cho high-side drive):
    ┌────────────────────────────────────────────────────────┐
    │   VCC ──[D_boot]──┬── C_boot ──┬── Vb (bootstrap)    │
    │                   │            │                      │
    │                   │      [Gate Driver IC]             │
    │                   │            │                      │
    │                   └──── Vs (source of high MOSFET)    │
    │                                                        │
    │   C_boot charge khi low-side ON (Vs = GND)            │
    │   Khi high-side ON: Vb = Vs + V_Cboot ≈ Vbus + 12V   │
    │   → Đủ Vgs cho high-side MOSFET                       │
    │                                                        │
    │   C_boot sizing: 0.1μF ~ 1μF ceramic                  │
    │   D_boot: fast recovery diode (UF4007 hoặc Schottky)  │
    └────────────────────────────────────────────────────────┘
```

### 4.3 PWM Types (Từ cơ bản → nâng cao)

```
═══════════════════════════════════════════════════════════════════════
  PWM STRATEGIES CHO BLDC MOTOR
═══════════════════════════════════════════════════════════════════════

  ┌─────────────────────────────────────────────────────────────┐
  │ 1. 6-STEP COMMUTATION (Trapezoidal — Basic)                │
  │                                                             │
  │   Mỗi thời điểm: chỉ 2/3 pha active                       │
  │   6 bước commutation mỗi electrical revolution             │
  │   PWM = ON/OFF đơn giản trên 1 pha                         │
  │                                                             │
  │   Step │ Phase A  │ Phase B  │ Phase C  │ Hall (HBA/HBB/HBC)│
  │   ─────│──────────│──────────│──────────│───────────────────│
  │   1    │ +PWM     │ -ON      │ Float    │ 1 0 1             │
  │   2    │ +PWM     │ Float    │ -ON      │ 0 0 1             │
  │   3    │ Float    │ +PWM     │ -ON      │ 0 1 1             │
  │   4    │ -ON      │ +PWM     │ Float    │ 0 1 0             │
  │   5    │ -ON      │ Float    │ +PWM     │ 1 1 0             │
  │   6    │ Float    │ -ON      │ +PWM     │ 1 0 0             │
  │                                                             │
  │   Ưu điểm: đơn giản, ít tính toán, dùng Hall sensor       │
  │   Nhược: torque ripple cao, hiệu suất thấp hơn FOC        │
  │   Dùng khi: prototype nhanh, ứng dụng low-cost            │
  └─────────────────────────────────────────────────────────────┘

  ┌─────────────────────────────────────────────────────────────┐
  │ 2. SINUSOIDAL PWM (SPWM)                                   │
  │                                                             │
  │   3 pha PWM duty biến thiên hình sin:                       │
  │     duty_A = 0.5 + 0.5 × m × sin(θ)                       │
  │     duty_B = 0.5 + 0.5 × m × sin(θ - 120°)               │
  │     duty_C = 0.5 + 0.5 × m × sin(θ - 240°)               │
  │   m = modulation index [0, 1]                               │
  │                                                             │
  │   Ưu điểm: sóng hài thấp hơn 6-step                       │
  │   Nhược: max Vbus utilization = 86.6% (√3/2)              │
  │   Dùng khi: cần smooth torque nhưng không cần tối ưu Vbus │
  └─────────────────────────────────────────────────────────────┘

  ┌─────────────────────────────────────────────────────────────┐
  │ 3. SPACE VECTOR PWM — SVPWM (Advanced, Ver2+Ver3 dùng)     │
  │                                                             │
  │   Tối ưu Vbus utilization lên 100% (linear region)         │
  │   15.5% voltage headroom hơn SPWM                          │
  │                                                             │
  │   Algorithm:                                                │
  │     Vref = (Vα, Vβ) → xác định sector (1-6)               │
  │     Tính T1, T2, T0 switching times                        │
  │     Symmetric 7-segment pattern                             │
  │                                                             │
  │   6 active vectors + 2 zero vectors:                        │
  │     V0(000), V1(100), V2(110), V3(010),                    │
  │     V4(011), V5(001), V6(101), V7(111)                     │
  │                                                             │
  │   Sector determination:                                     │
  │     Vref1 = Vβ                                              │
  │     Vref2 = (√3·Vα - Vβ) / 2                               │
  │     Vref3 = -(√3·Vα + Vβ) / 2                              │
  │     sector = lookup[sign(Vref1), sign(Vref2), sign(Vref3)] │
  │                                                             │
  │   Ver2 implementation: duty clamped [5%, 95%]              │
  │   Ver3 mở rộng: over-modulation (xem section 6.5)          │
  └─────────────────────────────────────────────────────────────┘

  So sánh:
  ┌──────────────┬───────────────┬───────────────┬───────────────┐
  │ Thuộc tính   │ 6-Step        │ SPWM          │ SVPWM         │
  │──────────────│───────────────│───────────────│───────────────│
  │ Complexity   │ Low           │ Medium        │ High          │
  │ Vbus util.   │ 100% (block)  │ 86.6%         │ 100% (linear) │
  │ THD          │ High          │ Medium        │ Low           │
  │ Torque ripple│ High          │ Medium        │ Low           │
  │ MCU load     │ Minimal       │ sin() calls   │ Sector + calc │
  │ Sensor need  │ Hall (coarse) │ Encoder/Est.  │ Encoder/Est.  │
  └──────────────┴───────────────┴───────────────┴───────────────┘
```

---

## 5. MCU / CONTROL UNIT — Trung tâm xử lý

### 5.1 PWM Generation

```
═══════════════════════════════════════════════════════════════════════
  HRTIM PWM — Center-aligned, Deadtime, ADC Sync
═══════════════════════════════════════════════════════════════════════

  Kế thừa VER2 (chi tiết đầy đủ):

  fHRTIM  = 170MHz
  fHRCK   = 170MHz × 32 (DLL) = 5.44GHz
  PERAR   = 136,000 → fPWM = 5.44GHz / (2 × 136000) = 20kHz
  
  Center-aligned mode:
    Counter: 0 → PERAR → 0 → PERAR → ...  (up-down counting)
    Ưu điểm: tần số switching harmonics xuất hiện ở 2×fPWM
    → Giảm EMI, giảm ripple current
    → Quan trọng: ADC trigger tại đỉnh (midpoint) = switching noise thấp nhất

  Dead-time insertion (BẮT BUỘC):
    DTxR = 136 ticks @ fHRCK/8 = 1.36GHz → 100ns
    Rising deadtime : high-side OFF → delay 100ns → low-side ON
    Falling deadtime: low-side OFF → delay 100ns → high-side ON
    → Tránh shoot-through (cả 2 MOSFET cùng ON = short circuit)

  ADC trigger synchronization:
    CMN.ADC1R bit[4] = Master Period event
    → ADC conversion bắt đầu tại đỉnh PWM carrier
    → Tất cả MOSFET ở trạng thái ổn định → sampling sạch nhất
    → Phase current = trung bình của chu kỳ PWM

  Outputs:
    Phase A: PA8  (HRTIM_TA1, AF13) + PA9  (HRTIM_TA2, AF13)
    Phase B: PA10 (HRTIM_TB1, AF13) + PA11 (HRTIM_TB2, AF13)
    Phase C: PB12 (HRTIM_TC1, AF13) + PB13 (HRTIM_TC2, AF13)

  Duty range: [5%, 95%] trong Ver2
  Ver3 mở rộng: over-modulation cho phép > 95% (xem section 6.5)
```

### 5.2 ADC + DMA

```
═══════════════════════════════════════════════════════════════════════
  ADC SYNCHRONIZED SAMPLING — HRTIM Triggered
═══════════════════════════════════════════════════════════════════════

  Đo đạc:
    ① Phase current (Ia, Ib) — qua shunt + OPAMP → ADC injected
    ② DC bus voltage (Vbus) — qua resistor divider → ADC regular

  ADC1/ADC2 dual mode:
    ADC1 injected CH13 : OPAMP1 OUT (Phase A current)
    ADC2 injected CH16 : OPAMP2 OUT (Phase B current)
    ADC1 regular CH6   : Vbus (PC0)
    
  Trigger: HRTIM_ADC_TRG1 (Master Period event)
    → ADC bắt đầu conversion sync với PWM midpoint
    → Giảm switching noise trong mẫu ADC

  Clock: HCLK/4 = 42.5MHz (synchronous mode)
  Resolution: 12-bit, auto-calibration (ADCAL bit)
  
  Conversion pipeline:
    HRTIM period event → trigger ADC1+ADC2 injected simultaneously
    → ~2μs conversion (12-bit @ 42.5MHz = ~240ns/sample + settling)
    → JEOS interrupt → ISR đọc JDR1 registers
    → FOC pipeline starts

  DMA option (Ver3):
    Regular Vbus có thể dùng DMA để giảm CPU load
    Injected current: đọc trực tiếp (ISR context, cần data ngay)
    → DMA cho Vbus continuous sampling, ISR cho phase current
```

### 5.3 Interrupt System

```
═══════════════════════════════════════════════════════════════════════
  INTERRUPT ARCHITECTURE — Multi-rate Deterministic
═══════════════════════════════════════════════════════════════════════

  Control loop chạy trong Timer ISR:

  ┌─────────────────────────────────────────────────────────────┐
  │ Priority │ Source              │ Rate    │ Task              │
  │──────────│─────────────────────│─────────│───────────────────│
  │ 0 (max)  │ ADC1_2 JEOS → ISR  │ 10-20kHz│ FOC current loop  │
  │          │ (HRTIM triggered)  │         │ (PID: Clarke/Park │
  │          │                    │         │  /PI/SVM) < 30μs  │
  │──────────│─────────────────────│─────────│───────────────────│
  │ 1        │ TIM2 UIF → ISR     │ 1kHz    │ Speed PI          │
  │          │                    │         │ Encoder velocity   │
  │          │                    │         │ Slip detection     │
  │──────────│─────────────────────│─────────│───────────────────│
  │ 2        │ COMP1 → HRTIM FLT1 │ async   │ Overcurrent HW    │
  │          │ (no ISR needed)    │         │ (auto disable PWM)│
  │──────────│─────────────────────│─────────│───────────────────│
  │ 3+       │ FDCAN1 RX/TX       │ async   │ CAN messages      │
  │          │ USART (debug)      │         │ Debug output       │
  └─────────────────────────────────────────────────────────────┘

  QUAN TRỌNG:
    ADC ISR (prio 0) có thể preempt TIM2 ISR (prio 1)
    → Current loop deadline > speed loop
    → Không share writable data giữa 2 ISR

  Frequency selection guidelines:
    FOC current loop: 10–20kHz (trade-off: CPU load vs ripple)
    Speed PI:         ~1kHz  (cơ khí chậm, không cần nhanh hơn)
    Deviation PI:     100Hz  (poll in main loop)
    Telemetry/Temp:   10Hz   (poll in main loop)
```

### 5.4 Protection Logic (MCU-driven + Hardware)

```
═══════════════════════════════════════════════════════════════════════
  PROTECTION HIERARCHY — Hardware First, Software Second
═══════════════════════════════════════════════════════════════════════

  ┌─────────────────────────────────────────────────────────────────┐
  │ OVERCURRENT (highest priority):                                 │
  │   Hardware path: COMP1 PA0 > Vref → HRTIM FLT1                │
  │   → Shutdown tất cả 6 PWM outputs < 100ns                     │
  │   → KHÔNG qua CPU, KHÔNG cần ISR                              │
  │   → MOSFET + motor an toàn dù firmware bị treo                │
  │                                                                 │
  │   Software path (backup): ADC Ia/Ib > I_SOFTWARE_LIMIT        │
  │   → ISR 20kHz context: HRTIM_DisableOutputs() + fault state   │
  │   → Latency ~50μs (chậm hơn HW 500×, nhưng có thể cấu hình) │
  └─────────────────────────────────────────────────────────────────┘

  ┌─────────────────────────────────────────────────────────────────┐
  │ UNDERVOLTAGE:                                                   │
  │   Vbus < Vbus_min (e.g. < 10V cho bus 48V)                    │
  │   → Disable system (motor không chạy được, FOC diverge)       │
  │   → CAN report undervoltage fault                              │
  │   → Chờ Vbus recover → soft-start lại                         │
  └─────────────────────────────────────────────────────────────────┘

  ┌─────────────────────────────────────────────────────────────────┐
  │ OVERVOLTAGE (VER3 MỚI):                                        │
  │   Vbus > Vbus_max (e.g. > 55V cho bus 48V)                    │
  │   → Bật brake resistor (dissipate regen energy)               │
  │   → Hardware: COMP2 → latch brake MOSFET < 200ns              │
  │   → Software: ADC ISR check + GPIO brake control              │
  │   → Nếu vẫn tăng > absolute max → shutdown inverter           │
  └─────────────────────────────────────────────────────────────────┘

  ┌─────────────────────────────────────────────────────────────────┐
  │ OVERTEMPERATURE:                                                │
  │   TMP112 > 85°C → fault → HRTIM disable                       │
  │   Hysteresis: recovery tại 75°C (10°C gap)                    │
  │   → Reduce output trước khi shutdown: torque derating          │
  │     T > 70°C: Iq_limit = Iq_max × (85 - T) / 15              │
  │     T > 85°C: full shutdown                                    │
  │   → Tránh on/off oscillation                                   │
  └─────────────────────────────────────────────────────────────────┘
```

---

## 6. SENSOR SYSTEM

### 6.1 Hall Sensor (VER3 MỚI — Coarse Rotor Position)

```
═══════════════════════════════════════════════════════════════════════
  HALL SENSORS — Vị trí rotor thô (3 sensor, 6 sectors)
═══════════════════════════════════════════════════════════════════════

  Mục đích:
    ✦ Commutation cơ bản (6-step trapezoidal)
    ✦ Startup FOC: xác định sector ban đầu khi chưa có encoder data
    ✦ Backup position source nếu encoder/SPI fail

  Cấu hình:
    3× Hall Effect sensor (A49E, SS443A, hoặc tích hợp trong motor)
    Đặt cách nhau 120° điện trên stator
    Output: digital HIGH/LOW → 6 trạng thái cho 1 electrical revolution

  Kết nối STM32:
    Hall A → GPIO Px (Input, pull-up, interrupt on edge)
    Hall B → GPIO Py
    Hall C → GPIO Pz
    Hoặc: TIM8 CH1/CH2/CH3 Hall sensor mode (hardware decoding)

  Decode table (4 pole pairs → 24 transitions / mech revolution):
    ┌──────────────┬──────────┬─────────────────────────────┐
    │ Hall C B A   │ Sector   │ θ_electrical (approximate)  │
    │──────────────│──────────│─────────────────────────────│
    │ 1 0 1        │ 1        │ 0° ~ 60°                    │
    │ 0 0 1        │ 2        │ 60° ~ 120°                  │
    │ 0 1 1        │ 3        │ 120° ~ 180°                 │
    │ 0 1 0        │ 4        │ 180° ~ 240°                 │
    │ 1 1 0        │ 5        │ 240° ~ 300°                 │
    │ 1 0 0        │ 6        │ 300° ~ 360°                 │
    │ 0 0 0        │ INVALID  │ Sensor fault!               │
    │ 1 1 1        │ INVALID  │ Sensor fault!               │
    └──────────────┴──────────┴─────────────────────────────┘

  FOC startup sequence (Ver3):
    1. Đọc Hall → xác định sector → θ_initial ≈ sector × 60° + 30°
    2. Bắt đầu FOC với θ_initial
    3. Sau 1-2 electrical revolutions: chuyển sang encoder/observer
    → Tránh alignment torque spike khi startup
```

### 6.2 Current Sensing

```
═══════════════════════════════════════════════════════════════════════
  PHASE CURRENT — Shunt Resistor + Op-amp (kế thừa Ver2)
═══════════════════════════════════════════════════════════════════════

  Signal chain:
    Motor phase current → Shunt 10mΩ → OPAMP (PGA ×4 internal) → ADC

  Thông số:
    Shunt resistance  : 10mΩ (R010)
    PGA gain           : ×4 (OPAMP1 + OPAMP2 internal, high-speed mode)
    V_adc = I × 0.010 × 4 + 1.65V = I × 0.040 + 1.65V
    Sensitivity        : 40 mV/A
    ADC resolution     : 0.02 A/LSB (12-bit, 3.3V ref)
    Range              : ±10A (ADC_CURRENT_ZERO = 2048)
    Phase C            : Ic = -(Ia + Ib) — Clarke, không cần đo riêng

  Yêu cầu cho FOC:
    ✦ Torque control: Iq tỉ lệ trực tiếp với torque
    ✦ Protection: phát hiện quá dòng trước khi cháy MOSFET
    ✦ Sampling sync: PHẢI đo tại HRTIM midpoint (low switching noise)

  Error budget (kế thừa Ver2):
    ± ADC quantization : 0.02 A/LSB
    ± OPAMP offset     : < 50 mA (after auto-cal)
    ± Shunt tolerance  : ±1% → ±0.1 A @ 10A
    → Tổng: ±200 mA @ 10A range (±2%) — PI loop bù được
```

### 6.3 Voltage Sensing

```
═══════════════════════════════════════════════════════════════════════
  DC BUS VOLTAGE MONITORING
═══════════════════════════════════════════════════════════════════════

  Resistor divider: 47kΩ + 4.7kΩ → ratio = 1/11
  V_adc = Vbus / 11
  Vbus = adc_raw × (3.3/4095) × 11 = adc_raw × 8.87 mV/LSB

  Dùng trong:
    ✦ SVM normalization: duty = Vα/Vbus, Vβ/Vbus
    ✦ Undervoltage detection: Vbus < Vbus_min → shutdown
    ✦ Overvoltage detection: Vbus > Vbus_max → brake resistor
    ✦ Power supply regulation loop (Ver3): Vbus_ref tracking

  Calibration:
    Đo Vbus bằng đồng hồ, so sánh với ADC reading
    Resistor tolerance ±1% → ratio error < 0.2%
    Accuracy: ±0.5V @ 48V (đủ cho mọi chức năng)
```

### 6.4 Encoder (Optional: Absolute + Quadrature)

```
  Quadrature (wheel odometry):
    TIM3/TIM4 encoder mode, 500PPR × 4 = 2000 cnt/rev
    → Velocity, slip detection, deviation PI
    (Không thay đổi so với Ver2)

  AS5048A (absolute angle for FOC):
    SPI1, 14-bit (16384 steps/revolution), 0.022°/step
    theta_elec = (raw_angle × POLE_PAIRS) mod 360°
    
    Ver3 upgrade: tích hợp CHÍNH THỨC vào FOC pipeline
    (Ver2 để optional, Ver3 dùng làm primary theta source)
    → Loại bỏ tích lũy sai số từ open-loop theta integration
```

### 6.5 Temperature (TMP112 — Không thay đổi)

```
  I2C1 @ 100kHz, address 0x48
  13-bit, 0.0625°C/LSB, ±0.5°C accuracy
  Factory calibrated → không cần calibration thủ công
  Warning 75°C, Fault 85°C, Hysteresis 10°C (Ver3 tăng từ 5°C Ver2)
```

---

## 7. CONTROL ALGORITHMS — Từ cơ bản đến nâng cao

### 7.1 Open-loop (Basic — Chỉ để tham khảo)

```
  Fixed PWM duty → motor quay
  KHÔNG có feedback → KHÔNG bù tải
  → KHÔNG KHUYẾN KHÍCH cho bất kỳ ứng dụng thực tế nào
  → Chỉ dùng để test hardware ban đầu (motor có quay không?)
```

### 7.2 Closed-loop Speed Control (PID)

```
═══════════════════════════════════════════════════════════════════════
  SPEED PID — Outer Loop (1kHz)
═══════════════════════════════════════════════════════════════════════

  speed_error = speed_ref - speed_measured

  PID:
    output = Kp × error + Ki × ∫error × dt + Kd × d(error)/dt

  output → PWM duty (nếu standalone)
  output → Iq_ref  (nếu cascade với current loop — VER2/VER3)

  Anti-windup: clamp integral, clamp output
  dt = 1ms (1kHz loop)

  Tuning (kế thừa Ver2):
    Kp = 0.1, Ki = 0.5, output limit = ±10A (Iq)
```

### 7.3 Torque / Current Control (Inner Loop)

```
═══════════════════════════════════════════════════════════════════════
  CURRENT PI — Inner Loop (10–20kHz)
═══════════════════════════════════════════════════════════════════════

  Inner loop (fast):
    I_error = I_ref - I_measured
    PI: V_output = Kp_i × I_error + Ki_i × ∫I_error
    V_output → PWM (through SVM)

  Outer loop (slow):
    Speed PID → generates I_ref (Iq_ref)

  Cascade structure:
    ┌──────────┐    Iq_ref    ┌──────────┐    Vq    ┌──────┐    ┌───────┐
    │ Speed PI │──────────────│Current PI│──────────│ SVM  │───→│Inverter│
    │ (1kHz)   │              │ (20kHz)  │          │      │    │       │
    └────┬─────┘              └────┬─────┘          └──────┘    └───┬───┘
         │                        │                                 │
         │ speed_measured         │ Iq_measured                     │
         └────────────────────────┴──────────── [Motor] ←───────────┘

  Tại sao cascade:
    Current loop phản hồi NHANH (20kHz) → bảo vệ MOSFET + motor
    Speed loop phản hồi CHẬM (1kHz) → ổn định cơ khí
    → Inner loop coi outer loop output là "DC setpoint"
    → Bandwidth separation: f_inner >> f_outer (tỉ lệ ≥ 10×)
```

### 7.4 Field-Oriented Control — FOC (Advanced)

```
═══════════════════════════════════════════════════════════════════════
  FOC — FIELD-ORIENTED CONTROL (Cốt lõi Ver2 + Ver3)
═══════════════════════════════════════════════════════════════════════

  FOC pipeline (chạy mỗi 50μs trong ADC ISR):

  Step 1: Measure phase currents
    [ADC1 JDR1] → Ia_raw → ia_amps = (raw - 2048) × 0.02
    [ADC2 JDR1] → Ib_raw → ib_amps = (raw - 2048) × 0.02

  Step 2: Clarke Transform (abc → αβ, static frame)
    Iα = Ia
    Iβ = (Ia + 2×Ib) / √3

  Step 3: Park Transform (αβ → dq, rotating frame)
    Id =  Iα·cos(θe) + Iβ·sin(θe)
    Iq = -Iα·sin(θe) + Iβ·cos(θe)

  Step 4: PI Controllers (d-axis and q-axis)
    Id control: Id_ref → PI → Vd
      Ver2: Id_ref = 0 (MTPA, maximum torque per ampere)
      Ver3: Id_ref ≤ 0 (field weakening cho high-speed)
    
    Iq control: Iq_ref → PI → Vq
      Iq_ref đến từ Speed PI outer loop

  Step 5: Inverse Park Transform (dq → αβ)
    Vα = Vd·cos(θe) - Vq·sin(θe)
    Vβ = Vd·sin(θe) + Vq·cos(θe)

  Step 6: SVM → PWM Duty
    SVM(Vα, Vβ, Vbus) → duty_A, duty_B, duty_C
    HRTIM_SetDuty(A, B, C) → CMP1R registers

  ┌─────────────────────────────────────────────────────────────────┐
  │                       FOC Block Diagram                         │
  │                                                                 │
  │  Speed_ref ──→ [Speed PI] ──→ Iq_ref ──→ [Iq PI] ──→ Vq ──┐  │
  │                  (1kHz)                    (20kHz)          │  │
  │                                                             │  │
  │  Id_ref=0 ──────────────────→ [Id PI] ──→ Vd ──┐          │  │
  │  (hoặc <0                      (20kHz)         │          │  │
  │   field weak)                                   │          │  │
  │                                                 ▼          ▼  │
  │                                           [Inverse Park]      │
  │                                                 │              │
  │                                                 ▼              │
  │  Ia,Ib ──→ [Clarke] ──→ [Park] ──→ Id,Iq    [SVM]            │
  │    ↑                       ↑                   │              │
  │    │                       │                   ▼              │
  │  [ADC]                   [θe]          [HRTIM PWM]            │
  │    ↑                       ↑                   │              │
  │    │                       │                   ▼              │
  │  [Shunt+OPA]         [Encoder/Hall]      [Inverter]          │
  │    ↑                       ↑                   │              │
  │    └───────────────── [BLDC Motor] ←───────────┘              │
  └─────────────────────────────────────────────────────────────────┘

═══════════════════════════════════════════════════════════════════════
  FOC MATH — Mô hình điện-cơ trong tọa độ dq
═══════════════════════════════════════════════════════════════════════

  Phương trình điện áp dq:
    Vd = R×Id + Ld×(dId/dt) - ωe×Lq×Iq
    Vq = R×Iq + Lq×(dIq/dt) + ωe×(Ld×Id + λpm)

  Phương trình mô-men:
    J×(dω/dt) = p×(λpm×Iq + (Ld - Lq)×Id×Iq) - B×ω - TL

  Trong đó:
    p    = pole pairs (FOC_POLE_PAIRS = 4)
    λpm  = permanent magnet flux linkage [Wb]
    Ld,Lq= d-axis, q-axis inductance [H]
    ωe   = electrical angular velocity [rad/s] = ω_mech × p
    R    = stator resistance [Ω]
    J    = moment of inertia [kg·m²]
    B    = viscous friction coefficient [N·m·s/rad]
    TL   = load torque [N·m]

  FOC principle (MTPA — Maximum Torque Per Ampere):
    Id = 0 → Torque = p × λpm × Iq (tuyến tính với Iq)
    → Điều khiển torque = điều khiển Iq = điều khiển 1 biến vô hướng
    → Đây là lý do FOC hiệu quả: decoupling flux và torque
```

### 7.5 Field Weakening (VER3 MỚI)

```
═══════════════════════════════════════════════════════════════════════
  FIELD WEAKENING — Mở rộng vùng tốc độ (Id < 0)
═══════════════════════════════════════════════════════════════════════

  Ver2 giới hạn: Id_ref = 0 cố định
    → Base speed = Vbus / (ωe × λpm)
    → Trên base speed: Vq bão hòa, không tăng tốc được

  Ver3 giải pháp: Id < 0 (flux weakening)
    → Giảm từ thông rotor hiệu dụng
    → Back-EMF giảm → còn headroom cho Vq
    → Tốc độ tăng được TRÊN base speed

  Algorithm:
    Vs_max = Vbus / √3    // max voltage vector magnitude
    Vs_actual = √(Vd² + Vq²)

    if (Vs_actual > Vs_max × 0.95) {
        // Entering field weakening region
        Id_ref -= FW_STEP;           // giảm Id (âm hơn)
        Id_ref = max(Id_ref, ID_FW_MIN);  // giới hạn âm
    } else if (Id_ref < 0 && Vs_actual < Vs_max × 0.85) {
        // Exiting field weakening
        Id_ref += FW_STEP;           // trả Id về 0
        Id_ref = min(Id_ref, 0);
    }

  Ràng buộc:
    Is = √(Id² + Iq²) ≤ Is_max   // current magnitude limit
    Vs = √(Vd² + Vq²) ≤ Vs_max   // voltage magnitude limit
    → 2 ràng buộc tạo FEASIBLE REGION (hình ellipse + circle trên Id-Iq plane)

  Operating regions:
    Region 1 (ω < ω_base)  : Id = 0, Iq tự do → MTPA (max torque)
    Region 2 (ω > ω_base)  : Id < 0, Iq giảm  → constant power
    Region 3 (ω >> ω_base) : deep flux weakening → torque giảm mạnh

  Trade-off:
    ✓ Mở rộng speed range 2-3× so với MTPA only
    ✗ Torque giảm khi field weakening (P = T × ω ≈ constant)
    ✗ Efficiency giảm (Id tạo loss nhưng không tạo torque)
    ✗ Risk: nếu Id quá âm → demagnetization vĩnh viễn
```

### 7.6 Over-modulation + SVPWM Advanced (VER3 MỚI)

```
═══════════════════════════════════════════════════════════════════════
  SVPWM OVER-MODULATION — Tăng Vbus Utilization
═══════════════════════════════════════════════════════════════════════

  SVPWM linear region: 
    Modulation index m ∈ [0, 1]
    Max Vout = Vbus / √3 ≈ 0.577 × Vbus

  Over-modulation Region I (m ∈ [1, 1.15]):
    Voltage vector trajectory bị cắt bởi hexagon boundary
    → Clamping ở boundary, distortion nhỏ
    → Vout tăng lên ~0.605 × Vbus

  Over-modulation Region II (m ∈ [1.15, π/4 ≈ 1.27]):
    Trajectory bị lock vào vertex hexagon
    → Distortion lớn hơn, harmonics tăng
    → Vout tiến đến 0.637 × Vbus (6-step equivalent)

  6-step transition (m > 1.27):
    → Full 6-step commutation (mỗi pha = ±Vbus/2)
    → Max Vout = 2/π × Vbus ≈ 0.637 × Vbus
    → THD cao nhất nhưng utilization Vbus cao nhất

  Implementation:
    if (m <= 1.0) {
        // Normal SVPWM (Ver2 algorithm)
        standard_svm(Valpha, Vbeta, Vbus);
    } else if (m <= 1.15) {
        // Over-modulation I: clamp to hexagon boundary
        clamp_to_hexagon(Valpha, Vbeta);
        standard_svm(Valpha_clamped, Vbeta_clamped, Vbus);
    } else {
        // Over-modulation II → approaching 6-step
        six_step_transition(theta_elec, m);
    }

  Dùng khi: 
    High-speed operation + field weakening → cần tận dụng hết Vbus
    Kết hợp: field weakening (Id < 0) + over-modulation → max speed
```

### 7.7 MTPA Optimization (VER3 MỚI)

```
═══════════════════════════════════════════════════════════════════════
  MTPA — Maximum Torque Per Ampere (cho IPM motor)
═══════════════════════════════════════════════════════════════════════

  Với surface-mount PM (Ld ≈ Lq):
    MTPA = Id = 0 (đã dùng trong Ver2)

  Với interior PM — IPM (Ld ≠ Lq, Ld < Lq — saliency):
    Torque = p × (λpm × Iq + (Ld - Lq) × Id × Iq)
    → Reluctance torque component (Ld - Lq) × Id × Iq ≠ 0
    → Id ≠ 0 tối ưu cho mỗi giá trị Is

  MTPA trajectory (lookup table):
    Is = √(Id² + Iq²)  // total current magnitude
    
    For each Is, optimal Id:
      Id_mtpa = (λpm - √(λpm² + 8×(Lq-Ld)²×Is²)) / (4×(Lq-Ld))
      Iq_mtpa = √(Is² - Id_mtpa²)
    
    → Pre-compute LUT: Is → (Id_mtpa, Iq_mtpa)
    → Runtime: lookup + interpolation

  Implementation:
    // MTPA LUT (pre-computed offline)
    const float mtpa_id_table[MTPA_TABLE_SIZE];  // Id vs Is
    const float mtpa_iq_table[MTPA_TABLE_SIZE];  // Iq vs Is

    // Runtime
    Is_cmd = sqrt(Iq_ref * Iq_ref);  // from speed PI
    Id_ref = mtpa_lookup(Is_cmd);     // table interpolation
    Iq_ref = sqrt(Is_cmd*Is_cmd - Id_ref*Id_ref);
```

### 7.8 Control Loop Structure — Tổng hợp

```
═══════════════════════════════════════════════════════════════════════
  VER3 COMPLETE CONTROL HIERARCHY
═══════════════════════════════════════════════════════════════════════

  Outer loop (SLOW — 1kHz):
    Speed PID → Iq_ref
    (+ MTPA lookup → Id_ref nếu IPM motor)
    (+ Field Weakening → điều chỉnh Id_ref nếu high-speed)

  Inner loop (FAST — 10–20kHz):
    Id PI → Vd
    Iq PI → Vq
    Inverse Park → Vαβ
    SVPWM (+ over-modulation nếu cần) → duty A, B, C
    → HRTIM PWM → Inverter → Motor

  Protection layer (HARDWARE — async, < 100ns):
    COMP1 → HRTIM FLT: overcurrent instant shutdown
    COMP2 → Brake: overvoltage energy dump

  Protection layer (SOFTWARE — 10Hz):
    Temperature derating / shutdown
    Slip detection / fault
    CAN e-stop
    Watchdog refresh
```

### 7.9 TRANSFER FUNCTION DESIGN — Hàm truyền toàn hệ thống (VER3 MỚI)

> **Sai lầm phổ biến:** Chỉ xây hàm truyền cho motor, bỏ qua sensor + inverter.
> MCU **KHÔNG** điều khiển output thật — MCU điều khiển **tín hiệu đo được**.
> Cái gì nằm trong vòng feedback loop → **PHẢI** có mặt trong hàm truyền.

#### 7.9.1 Tại sao PHẢI tính sensor vào hàm truyền

```
═══════════════════════════════════════════════════════════════════════
  BẠN TƯỞNG (sai):
═══════════════════════════════════════════════════════════════════════

  Setpoint ──→ [Controller] ──→ [Motor] ──→ Output thật
                    ↑                           │
                    └───────────────────────────┘
                         feedback = output thật

═══════════════════════════════════════════════════════════════════════
  THỰC TẾ BẠN CÓ (đúng):
═══════════════════════════════════════════════════════════════════════

  Setpoint ──→ [Controller] ──→ [Inverter] ──→ [Motor] ──→ Output thật
                    ↑               delay          RL         (dòng, tốc độ)
                    │                                              │
                    │                                              ▼
                    │                                         [Sensor]
                    │                                          gain +
                    │                                          delay +
                    │                                          noise
                    │                                              │
                    └──────────────────────────────────────────────┘
                              feedback = output ĐO ĐƯỢC
                              (≠ output thật!)

  MCU chỉ thấy "Output đo được", KHÔNG BAO GIỜ thấy "Output thật".
  → Hàm truyền PHẢI bao gồm TẤT CẢ khâu trong vòng loop.
```

#### 7.9.2 Open-loop Transfer Function — Đầy đủ

```
═══════════════════════════════════════════════════════════════════════
  G_open(s) = G_controller(s) × G_inverter(s) × G_motor(s) × G_sensor(s)
                  PI/PID           PWM delay       plant        đo lường
═══════════════════════════════════════════════════════════════════════

  KHÔNG PHẢI:
    G_open(s) = G_controller(s) × G_motor(s)    ← THIẾU 2 KHÂU!

  4 khâu trong loop, mỗi khâu có gain + dynamics riêng:
  ┌────────────────────────────────────────────────────────────────────┐
  │ Khâu             │ Hàm truyền         │ Tham số chính             │
  │──────────────────│─────────────────────│───────────────────────────│
  │ Controller (PI)  │ Kp + Ki/s           │ Kp, Ki (tunable)          │
  │ Inverter (PWM)   │ K_inv / (1 + s×Td)  │ Td ≈ 0.5×Tpwm = 25μs    │
  │ Motor (plant)    │ xem 7.9.3           │ R, L, J, B, λpm, p       │
  │ Sensor (feedback)│ K_s / (1 + s×τ_s)   │ K_s = gain, τ_s = delay  │
  └────────────────────────────────────────────────────────────────────┘
```

#### 7.9.3 G_motor(s) — Hàm truyền Motor (d-q frame)

```
═══════════════════════════════════════════════════════════════════════
  A. CURRENT LOOP — Motor electrical model (q-axis)
═══════════════════════════════════════════════════════════════════════

  Từ phương trình Vq:
    Vq = R×Iq + Lq×(dIq/dt) + ωe×(Ld×Id + λpm)
    
  Với Id = 0 (MTPA), bỏ qua back-EMF coupling (inner loop nhanh):
    Vq ≈ R×Iq + Lq×(dIq/dt)

  Laplace:
    Vq(s) = (R + s×Lq) × Iq(s)

  ┌─────────────────────────────────────────────────────────┐
  │                    1                                     │
  │ G_motor_i(s) = ─────────                                │
  │                 Lq×s + R                                 │
  │                                                          │
  │              1/R                                          │
  │           = ──────────────                               │
  │             (Lq/R)×s + 1                                 │
  │                                                          │
  │           = K_m / (τ_e × s + 1)                          │
  │                                                          │
  │ Trong đó:                                                │
  │   K_m = 1/R        [A/V]  — DC gain (static)            │
  │   τ_e = Lq/R       [s]   — Hằng số thời gian điện      │
  │                                                          │
  │ Ví dụ: R = 0.5Ω, Lq = 1mH                              │
  │   K_m = 2 A/V                                           │
  │   τ_e = 1mH / 0.5Ω = 2ms                               │
  │   Bandwidth tự nhiên = 1/(2π×τ_e) = 80Hz                │
  │   → PI current loop PHẢI nhanh hơn nhiều (target: 2kHz) │
  └─────────────────────────────────────────────────────────┘

  D-axis tương tự:
    G_motor_id(s) = 1 / (Ld×s + R) = (1/R) / ((Ld/R)×s + 1)

═══════════════════════════════════════════════════════════════════════
  B. SPEED LOOP — Motor mechanical model
═══════════════════════════════════════════════════════════════════════

  Phương trình cơ:
    J×(dω/dt) = T_em - B×ω - T_load
    T_em = (3/2) × p × λpm × Iq  (với Id = 0)

  Laplace (bỏ T_load, coi là disturbance):
    (J×s + B) × ω(s) = K_t × Iq(s)

  ┌─────────────────────────────────────────────────────────┐
  │                  K_t                                     │
  │ G_motor_w(s) = ─────────                                │
  │                 J×s + B                                  │
  │                                                          │
  │              K_t/B                                       │
  │           = ──────────────                               │
  │             (J/B)×s + 1                                  │
  │                                                          │
  │           = K_mech / (τ_m × s + 1)                       │
  │                                                          │
  │ Trong đó:                                                │
  │   K_t   = (3/2)×p×λpm  [N·m/A] — Torque constant       │
  │   K_mech= K_t/B        [rad/s/A] — DC gain mech         │
  │   τ_m   = J/B          [s]   — Hằng số thời gian cơ     │
  │                                                          │
  │ Ví dụ: J = 0.001 kg·m², B = 0.005 N·m·s/rad            │
  │   τ_m = 0.001/0.005 = 200ms                             │
  │   → Cơ khí CHẬM hơn điện 100× (2ms vs 200ms)           │
  │   → Speed loop 1kHz hoàn toàn đủ (thừa bandwidth)      │
  └─────────────────────────────────────────────────────────┘
```

#### 7.9.4 G_inverter(s) — Hàm truyền Inverter (PWM delay)

```
═══════════════════════════════════════════════════════════════════════
  INVERTER = PWM TRANSPORT DELAY
═══════════════════════════════════════════════════════════════════════

  PWM center-aligned: lệnh duty được MCU ghi vào → có hiệu lực
  sau trung bình 0.5 chu kỳ PWM (worst case 1 chu kỳ).

  Delay chính xác:
    Td = 0.5 × Tpwm = 0.5 × (1/20000) = 25μs

  Hàm truyền delay thuần túy:
    G_inv_exact(s) = e^(-s × Td)

  Xấp xỉ Padé bậc 1 (dùng cho tính toán PI):
    G_inv(s) ≈ 1 / (1 + s × Td)

  ┌─────────────────────────────────────────────────────────┐
  │                    1                                     │
  │ G_inverter(s) ≈ ─────────       Td = 25μs (@ 20kHz)    │
  │                  1 + s×Td                                │
  │                                                          │
  │ Ảnh hưởng:                                               │
  │   @ 1kHz : |G_inv| = 0.99, ∠ = -9°     → gần như không │
  │   @ 5kHz : |G_inv| = 0.85, ∠ = -38°    → đáng kể       │
  │   @ 10kHz: |G_inv| = 0.54, ∠ = -57°    → rất lớn       │
  │                                                          │
  │ → Giới hạn bandwidth current loop < ~fPWM/10 = 2kHz    │
  │ → Nếu cố tune PI nhanh hơn → oscillation               │
  └─────────────────────────────────────────────────────────┘

  Tại sao KHÔNG THỂ bỏ qua:
    Ở 20kHz FOC, mỗi μs delay = 7.2° phase lag
    25μs delay = 180° phase lag @ 20kHz → UNSTABLE nếu gain > 1
    → Đây là LÝ DO current loop bandwidth bị giới hạn
```

#### 7.9.5 G_sensor(s) — Hàm truyền Sensor (PHẦN HAY BỎ QUA)

```
═══════════════════════════════════════════════════════════════════════
  SENSOR TRANSFER FUNCTION — Gain + Delay + Noise
═══════════════════════════════════════════════════════════════════════

  Mỗi sensor KHÔNG phải "transparent window" nhìn thẳng vào motor.
  Sensor có:
    ① Gain (K_s)    — tỉ lệ giữa đại lượng vật lý ↔ giá trị digital
    ② Delay (τ_s)   — thời gian từ lúc sự kiện xảy ra → MCU đọc được
    ③ Noise / Quant — sàn nhiễu, resolution hữu hạn

  Hàm truyền tổng quát:
  ┌─────────────────────────────────────────────────────────┐
  │              K_s                                         │
  │ G_sensor(s) = ──────────────                             │
  │               1 + s × τ_s                                │
  │                                                          │
  │ K_s = sensor gain (V/A, counts/rad, LSB/°C...)          │
  │ τ_s = tổng delay (conversion + filter + latency)        │
  └─────────────────────────────────────────────────────────┘

═══════════════════════════════════════════════════════════════════════
  TỪNG SENSOR TRONG HỆ THỐNG — Chi tiết hàm truyền
═══════════════════════════════════════════════════════════════════════

  ┌─────────────────────────────────────────────────────────────────┐
  │ A. CURRENT SENSOR (Shunt + OPAMP + ADC)                        │
  │                                                                 │
  │ Signal chain:                                                   │
  │   I_phase [A] → Shunt 10mΩ → V_shunt [V]                     │
  │   V_shunt → OPAMP PGA ×4 → V_adc [V]                         │
  │   V_adc → ADC 12-bit → raw [LSB]                              │
  │   raw → software scale → I_measured [A]                        │
  │                                                                 │
  │ Gain tổng hợp:                                                  │
  │   K_shunt = R_shunt = 0.010 V/A                                │
  │   K_opamp = PGA gain = 4                                       │
  │   K_adc   = 4095 / 3.3 = 1241 LSB/V                           │
  │   K_sw    = 1 / (K_shunt × K_opamp × K_adc) = 0.02 A/LSB     │
  │                                                                 │
  │   K_s_current = K_shunt × K_opamp × K_adc × K_sw              │
  │              = 1.0 (dimensionless, vì sw đã normalize về Amp)  │
  │   → NẾU calibration ĐÚNG, K_s = 1.0 (transparent)            │
  │   → NẾU calibration SAI (offset, gain error), K_s ≠ 1.0      │
  │     → Kp hiệu dụng sai → performance lệch khỏi thiết kế     │
  │                                                                 │
  │ Delay:                                                          │
  │   τ_adc     = 2μs    (12-bit conversion @ 42.5MHz)            │
  │   τ_opamp   = 0.5μs  (OPAMP settling, high-speed mode)        │
  │   τ_sample  = 0.1μs  (sampling capacitor charge)              │
  │   τ_isr     = 1μs    (ISR entry + register read)              │
  │   ─────────────────                                             │
  │   τ_s_current ≈ 3.5μs total                                   │
  │                                                                 │
  │ Hàm truyền:                                                    │
  │                1.0                                              │
  │   G_cs(s) = ────────────      τ = 3.5μs                       │
  │              1 + s × 3.5μs                                     │
  │                                                                 │
  │ Impact @ 20kHz FOC (period = 50μs):                            │
  │   Phase lag = arctan(2π × 20000 × 3.5e-6) = 24°               │
  │   → 24° phase margin bị mất DO SENSOR THÔI!                   │
  │   → Nếu bỏ qua: PI gains quá cao → oscillation                │
  │                                                                 │
  │ So sánh: nếu τ_s = 0 (sensor lý tưởng):                       │
  │   → Phase margin tăng 24° → có thể tune PI aggressive hơn     │
  │   → Bandwidth lý thuyết: 5kHz                                  │
  │   → Bandwidth thực (có sensor): 3kHz (giảm 40%!)              │
  └─────────────────────────────────────────────────────────────────┘

  ┌─────────────────────────────────────────────────────────────────┐
  │ B. VOLTAGE SENSOR (Resistor Divider + ADC)                     │
  │                                                                 │
  │ Gain:                                                           │
  │   K_divider = 4.7 / (47 + 4.7) = 1/11                         │
  │   K_adc     = 1241 LSB/V                                      │
  │   K_sw      = 11 × 3.3/4095 = 8.87 mV/LSB (quy ngược về V)  │
  │   K_s_vbus  = 1.0 (sau software normalization)                 │
  │                                                                 │
  │ Delay:                                                          │
  │   τ_s_vbus ≈ 2μs (ADC regular conversion)                     │
  │   + nếu có DMA + averaging: τ ≈ 10-50μs                       │
  │                                                                 │
  │ Hàm truyền:                                                    │
  │                1.0                                              │
  │   G_vs(s) = ────────────                                       │
  │              1 + s × 2μs                                       │
  │                                                                 │
  │ Impact: Vbus dùng trong SVM normalization (Vα/Vbus)           │
  │   Nếu Vbus đo chậm → SVM output sai tỉ lệ → duty sai        │
  │   Nhưng Vbus thay đổi CHẬM (ms~s) vs ADC sample (μs):        │
  │   → Ảnh hưởng nhỏ TRỪKHI Vbus thay đổi đột ngột (regen)     │
  └─────────────────────────────────────────────────────────────────┘

  ┌─────────────────────────────────────────────────────────────────┐
  │ C. ENCODER / POSITION SENSOR                                    │
  │                                                                 │
  │ C1. Quadrature Encoder (Wheel velocity @ 1kHz)                 │
  │   Gain: K_s = PULSE_TO_METER / dt = 0.000102 / 0.001          │
  │        = 0.102 (m/s) / (pulse/sample)                          │
  │   Delay: τ ≈ 0 (TIM hardware, counter read = instant)         │
  │   Noise: quantization = ±1 count → ±0.102 m/s @ 1kHz          │
  │     → Ở tốc độ thấp (< 0.5 m/s): noise = ±20% giá trị       │
  │     → Cần filter (moving average) → thêm delay                │
  │     → Filter N=4: τ_filter = N × dt / 2 = 2ms                 │
  │                                                                 │
  │   G_enc(s) = 1.0 / (1 + s × 2ms)   (với filter)              │
  │                                                                 │
  │   Impact @ speed loop 1kHz:                                    │
  │     Phase lag = arctan(2π × 1000 × 0.002) = 51°               │
  │     → 51° phase margin mất bởi encoder + filter!              │
  │     → Speed PI PHẢI tune conservative                          │
  │                                                                 │
  │ C2. AS5048A SPI (FOC theta @ 20kHz)                            │
  │   Gain: K_s = 2π / 16384 = 0.000384 rad/LSB                   │
  │   Delay:                                                        │
  │     SPI transfer: 16-bit @ 10MHz = 1.6μs                      │
  │     CS setup + hold: 0.2μs                                     │
  │     Processing: 0.5μs                                          │
  │     τ_s_angle ≈ 2.3μs                                         │
  │                                                                 │
  │   G_angle(s) = 1.0 / (1 + s × 2.3μs)                         │
  │                                                                 │
  │   Impact: θ_elec error → Park transform sai                    │
  │     @ 3000 RPM, ωe = 3000/60 × 4 × 2π = 1257 rad/s           │
  │     θ_error = ωe × τ = 1257 × 2.3e-6 = 0.003 rad = 0.17°    │
  │     → Rất nhỏ → chấp nhận được                                │
  │     @ 10000 RPM: θ_error = 0.56° → vẫn OK (FOC tolerant ~5°) │
  └─────────────────────────────────────────────────────────────────┘

  ┌─────────────────────────────────────────────────────────────────┐
  │ D. TEMPERATURE SENSOR (TMP112 — Không ảnh hưởng control)       │
  │                                                                 │
  │   Delay: 125ms (I2C read + conversion time)                    │
  │   → NHƯNG: temperature loop = 10Hz (100ms period)              │
  │   → Temperature thay đổi theo phút, không theo ms              │
  │   → Delay 125ms / τ_thermal(~60s) = 0.2% → BỎ QUA ĐƯỢC       │
  │   → Không cần tính vào hàm truyền control loop                │
  └─────────────────────────────────────────────────────────────────┘
```

#### 7.9.6 Hàm truyền tổng hợp — CURRENT LOOP (Quan trọng nhất)

```
═══════════════════════════════════════════════════════════════════════
  CURRENT LOOP OPEN-LOOP TRANSFER FUNCTION
═══════════════════════════════════════════════════════════════════════

  Block diagram:

  Iq_ref ──(+)──→ [PI: Kp + Ki/s] ──→ [Inverter] ──→ [Motor RL] ──→ Iq
             ↑(-)                     1/(1+sTd)     1/(Lq·s+R)     │
             │                                                      │
             │                          [Current Sensor]            │
             └────────────────────── 1/(1+s·τ_cs) ←────────────────┘


  G_open_i(s) = [Kp + Ki/s] × [1/(1 + s×Td)] × [1/(Lq×s + R)] × [1/(1 + s×τ_cs)]
                 controller      inverter          motor             sensor

  Thay số (ví dụ):
    Kp_i = 0.5,  Ki_i = 50
    Td   = 25μs   (inverter delay @ 20kHz)
    Lq   = 1mH,   R = 0.5Ω
    τ_cs = 3.5μs  (current sensor delay)

  Nếu BỎ QUA sensor + inverter delay:
    G_simplified(s) = [Kp + Ki/s] × [1/(Lq×s + R)]
    → Bandwidth tính ra ≈ 5kHz
    → Phase margin ≈ 75° (thoải mái)

  Nếu TÍNH ĐẦY ĐỦ:
    G_full(s) = G_simplified(s) × [1/(1+s×25μs)] × [1/(1+s×3.5μs)]
    → Extra phase lag @ crossover:
      Inverter: -arctan(2π × 3000 × 25e-6) = -28°
      Sensor:   -arctan(2π × 3000 × 3.5e-6) = -4°
      Tổng extra: -32° phase margin mất!
    → Bandwidth thực ≈ 3kHz
    → Phase margin thực ≈ 43° (vẫn stable, nhưng ít headroom)

  ┌───────────────────────────────────────────────────────────────┐
  │ KẾT LUẬN: Bỏ qua sensor delay                                │
  │   → Phase margin tưởng 75° → thực tế chỉ 43°               │
  │   → Tưởng bandwidth 5kHz → thực tế chỉ 3kHz                │
  │   → Sai 40%! Đủ để hệ thống oscillate nếu tune aggressive  │
  └───────────────────────────────────────────────────────────────┘

═══════════════════════════════════════════════════════════════════════
  PI TUNING TỪ HÀM TRUYỀN — Phương pháp Technical Optimum
═══════════════════════════════════════════════════════════════════════

  Target: triệt tiêu pole chậm nhất của motor bằng zero của PI.

  PI: C(s) = Kp × (1 + 1/(Ti×s)) = Kp × (Ti×s + 1) / (Ti×s)

  Đặt Ti = τ_e = Lq/R  (triệt tiêu pole motor)
  → G_open(s) = Kp/(Ti×s) × 1/(R) × 1/((1+s×Td)(1+s×τ_cs))

  Xấp xỉ 2 delay nhỏ thành 1:
    τ_Σ = Td + τ_cs = 25 + 3.5 = 28.5μs

  → G_open(s) ≈ Kp/(R × Ti × s) × 1/(1 + s×τ_Σ)

  Technical Optimum (Betragsoptimum):
    Chọn Kp sao cho crossover frequency ω_c = 1/(2×τ_Σ):
    
    Kp = R × Ti / (2 × τ_Σ)
       = 0.5 × 0.002 / (2 × 28.5e-6)
       = 17.5

    Ki = Kp / Ti = 17.5 / 0.002 = 8750

  Kết quả:
    Bandwidth:    ω_c = 1/(2 × 28.5μs) = 17.5 krad/s ≈ 2.8 kHz
    Phase margin: 65° (standard for Technical Optimum)
    Overshoot:    4.3% (step response)
    Settling time: ~4 × τ_Σ = 114μs (2-3 FOC cycles)

  → Nếu KHÔNG tính τ_cs vào τ_Σ:
    τ_Σ_wrong = 25μs (chỉ inverter)
    Kp_wrong  = 0.5 × 0.002 / (2 × 25e-6) = 20 (cao hơn 14%)
    → Bandwidth = 3.2kHz (quá cao cho τ thực)
    → Phase margin = 55° (thấp hơn target 65°)
    → Overshoot = 12% (3× cao hơn!)
    → Có thể trigger overcurrent protection bất ngờ
```

#### 7.9.7 Hàm truyền tổng hợp — SPEED LOOP

```
═══════════════════════════════════════════════════════════════════════
  SPEED LOOP — Cascade trên nền current loop đã tune
═══════════════════════════════════════════════════════════════════════

  Current loop (đã đóng) coi như một khâu:
    Inner-loop closed = 1 / (1 + s × τ_cl)
    τ_cl ≈ 2 × τ_Σ = 2 × 28.5μs = 57μs  (Technical Optimum result)

  Block diagram:

  ω_ref ──(+)──→ [PI speed] ──→ [Current loop] ──→ [Mech] ──→ ω
            ↑(-)                 1/(1+s×τ_cl)     Kt/(Js+B)   │
            │                                                   │
            │                     [Encoder + filter]            │
            └───────────────── 1/(1+s×τ_enc) ←─────────────────┘

  G_open_w(s) = [Kp_w + Ki_w/s] × [1/(1+s×τ_cl)] × [K_t/(J×s+B)] × [1/(1+s×τ_enc)]
                  speed PI       current loop CL    mechanical       encoder sensor

  Delay budget speed loop:
    τ_cl  = 57μs   (current loop bandwidth)
    τ_enc = 2ms    (encoder + moving average filter)
    τ_Σ_speed = τ_cl + τ_enc = 57μs + 2ms ≈ 2.06ms

  → τ_enc CHIẾM CHỦ ĐẠO! (2ms >> 57μs)
  → Chính SENSOR (encoder filter) giới hạn speed loop, KHÔNG phải motor!

  Technical Optimum cho speed loop:
    Ti_w = τ_m = J/B = 200ms  (triệt tiêu pole cơ)
    Kp_w = J / (2 × K_t × τ_Σ_speed)
         = 0.001 / (2 × 0.12 × 0.00206)
         = 2.02
    Ki_w = Kp_w / Ti_w = 2.02 / 0.2 = 10.1

  Bandwidth speed loop:
    ω_c = 1/(2 × τ_Σ_speed) = 1/(2 × 0.00206) = 243 rad/s ≈ 39 Hz

  ┌───────────────────────────────────────────────────────────────┐
  │ KẾT LUẬN:                                                     │
  │   Nếu bỏ encoder filter (τ_enc = 0):                         │
  │     τ_Σ = 57μs → bandwidth = 1.4 kHz (speed PI)              │
  │   Nếu tính encoder filter (τ_enc = 2ms):                     │
  │     τ_Σ = 2.06ms → bandwidth = 39 Hz                         │
  │                                                                │
  │   SAI LỆCH 36× !!!                                            │
  │   → Encoder filter là BOTTLENECK, không phải motor             │
  │   → Muốn tăng speed bandwidth: GIẢM encoder filter trước     │
  │   → Ví dụ: N=2 filter → τ_enc = 1ms → BW = 77Hz (tăng 2×)  │
  └───────────────────────────────────────────────────────────────┘
```

#### 7.9.8 Tổng hợp — Bảng delay budget toàn hệ thống

```
═══════════════════════════════════════════════════════════════════════
  DELAY BUDGET — Mỗi khâu đóng góp bao nhiêu vào tổng delay
═══════════════════════════════════════════════════════════════════════

  ┌──────────────────────────────────────────────────────────────────┐
  │                    CURRENT LOOP (20kHz)                          │
  │                                                                  │
  │ Khâu              │ Delay    │ Phase lag   │ % tổng   │ Bỏ qua?│
  │                   │          │ @ 3kHz      │ delay    │        │
  │───────────────────│──────────│─────────────│──────────│────────│
  │ Inverter (PWM)    │ 25μs     │ -28°        │ 88%      │ KHÔNG  │
  │ Current sensor    │ 3.5μs    │ -4°         │ 12%      │ Tùy!   │
  │ Motor RL (pole)   │ cancel   │ (PI triệt)  │ —        │ —      │
  │───────────────────│──────────│─────────────│──────────│────────│
  │ TỔNG              │ 28.5μs   │ -32°        │ 100%     │        │
  │                                                                  │
  │ Kết luận: Ở current loop, inverter delay chiếm chủ đạo.        │
  │ Sensor delay = 12% — ảnh hưởng nhỏ nhưng KHÔNG nên bỏ          │
  │ nếu tune aggressive (phase margin < 50°).                       │
  └──────────────────────────────────────────────────────────────────┘

  ┌──────────────────────────────────────────────────────────────────┐
  │                    SPEED LOOP (1kHz)                              │
  │                                                                  │
  │ Khâu              │ Delay    │ Phase lag   │ % tổng   │ Bỏ qua?│
  │                   │          │ @ 39Hz      │ delay    │        │
  │───────────────────│──────────│─────────────│──────────│────────│
  │ Current loop CL   │ 57μs     │ -0.8°       │ 2.8%     │ có thể│
  │ Encoder + filter  │ 2ms      │ -27°        │ 97%      │ KHÔNG  │
  │ Motor mech (pole) │ cancel   │ (PI triệt)  │ —        │ —      │
  │───────────────────│──────────│─────────────│──────────│────────│
  │ TỔNG              │ 2.06ms   │ -28°        │ 100%     │        │
  │                                                                  │
  │ Kết luận: Ở speed loop, ENCODER + FILTER chiếm chủ đạo (97%).  │
  │ Current loop delay gần như không đáng kể ở bandwidth này.       │
  │ → SENSOR LÀ BOTTLENECK CỦA SPEED LOOP, KHÔNG PHẢI MOTOR.      │
  └──────────────────────────────────────────────────────────────────┘

═══════════════════════════════════════════════════════════════════════
  QUY TẮC THỰC HÀNH — Khi nào sensor delay QUAN TRỌNG
═══════════════════════════════════════════════════════════════════════

  ┌──────────────────────────────────────────────────────────────────┐
  │ Điều kiện                     │ Sensor delay quan trọng?        │
  │───────────────────────────────│─────────────────────────────────│
  │ τ_sensor / T_loop < 1%       │ BỎ QUA ĐƯỢC (ví dụ: temp 10Hz) │
  │ τ_sensor / T_loop = 1-10%    │ Nên tính nếu tune aggressive   │
  │ τ_sensor / T_loop > 10%      │ BẮT BUỘC tính (ví dụ: enc@1kHz)│
  │ τ_sensor ≈ τ_plant           │ SENSOR LÀ BOTTLENECK!          │
  └──────────────────────────────────────────────────────────────────┘

  Trong hệ thống Ver3:
    Current sensor (3.5μs / 50μs = 7%)   → NÊN tính
    Encoder filter (2ms / 1ms = 200%)     → BẮT BUỘC — nó LỚN HƠN loop period!
    Vbus sensor (2μs / slow loop)         → bỏ qua được
    Temp sensor (125ms / thermal ~60s)    → bỏ qua hoàn toàn
    AS5048A angle (2.3μs / 50μs = 4.6%)  → nên tính nếu high-speed FOC
```

---

## 8. TIMING ARCHITECTURE

### 8.1 Multi-rate Schedule

```
═══════════════════════════════════════════════════════════════════════
  DETERMINISTIC TIMING — Ver3 (kế thừa Ver2 + WCET analysis)
═══════════════════════════════════════════════════════════════════════

  ┌─────────────────────────────────────────────────────────────────┐
  │ Rate    │ Trigger           │ Tasks                    │ Budget │
  │─────────│───────────────────│──────────────────────────│────────│
  │ 10-20kHz│ HRTIM → ADC ISR   │ FOC current loop         │ < 30μs│
  │         │ (priority 0)      │ Clarke/Park/PI/SVM       │        │
  │─────────│───────────────────│──────────────────────────│────────│
  │ 1kHz    │ TIM2 UIF ISR      │ Encoder velocity         │ <500μs│
  │         │ (priority 1)      │ Slip detection           │        │
  │         │                   │ Speed PI → Iq_ref        │        │
  │─────────│───────────────────│──────────────────────────│────────│
  │ 100Hz   │ Main loop poll    │ Deviation PI correction  │ < 5ms │
  │         │ (tick mod 10)     │                          │        │
  │─────────│───────────────────│──────────────────────────│────────│
  │ 10Hz    │ Main loop poll    │ TMP112 I2C read          │ <50ms │
  │         │ (tick mod 100)    │ CAN rx/tx telemetry      │        │
  │         │                   │ Fault management         │        │
  │         │                   │ Flash log (nếu fault)    │        │
  │         │                   │ Watchdog refresh         │        │
  └─────────────────────────────────────────────────────────────────┘

  Timing constraints (Ver3 QUAN TRỌNG):
    ✦ PWM frequency: 10–20kHz (configurable)
    ✦ ADC trigger: PHẢI synchronized với PWM midpoint
    ✦ Control ISR: runs every PWM cycle (hoặc N cycles nếu CPU load cao)
    ✦ Deterministic: mỗi ISR PHẢI hoàn thành trước deadline
    ✦ Low latency: interrupt entry < 12 cycles (Cortex-M4 auto stacking)
    ✦ Jitter: < 1μs cho FOC loop (hardware timer guarantee)

  WCET Analysis (Ver3 mới):
    FOC ISR worst-case:
      ADC read      : 0.5μs
      Clarke/Park   : 2.0μs (LUT sin/cos)
      PI × 2        : 1.5μs
      InvPark       : 1.5μs
      SVM + duty set: 3.0μs
      Field Weakening: 1.0μs (conditional)
      Over-modulation: 1.5μs (conditional)
      Overhead (ISR) : 1.0μs
      ─────────────────────
      WCET total    : ~12μs → margin 60% (budget 30μs)
      → AN TOÀN cho 20kHz operation
```

### 8.2 ISR Preemption & Data Integrity

```
  ISR preemption chain:
    ADC ISR (prio 0) → có thể interrupt TIM2 ISR (prio 1)
    TIM2 ISR (prio 1) → có thể interrupt main loop (background)
    → Không share writable data giữa ISR cùng mức
    → Main loop chỉ đọc g_sensor_data (ISR atomic 32-bit writes)

  Data integrity (kế thừa Ver2):
    ADC: đọc JDR1 trực tiếp trong ISR (no buffer needed)
    Encoder: prev_L/prev_R lưu giá trị trước (single writer)
    g_sensor_data: volatile + 32-bit aligned → no tearing on ARM
    CAN: hardware FIFO (message RAM) → no software buffer needed
    
  Ver3 addition:
    Flash log: chỉ ghi từ main loop context (10Hz, non-time-critical)
    Watchdog: refresh trong main loop 10Hz (IWDG window)
```

---

## 9. COMMUNICATION / INTERFACE

### 9.1 UART (Debug)

```
  USART2 (PA2 TX, PA3 RX) @ 115200 baud
  Mục đích: debug output (printf-style), parameter tuning
  KHÔNG dùng trong control loop (blocking I/O quá chậm)
  Chỉ dùng từ main loop context hoặc off khi production
```

### 9.2 CAN (Industrial — Kế thừa Ver2)

```
  FDCAN1 @ 1Mbps nominal
  PA12(TX, AF9) + PB8(RX, AF9)
  External transceiver: SN65HVD230 / TCAN1044

  Messages:
    ID 0x100 : Speed setpoint command (from master)
    ID 0x101 : Status telemetry (to master, 10Hz)
               [speed, current, temp, fault_code, state]
    ID 0x1FF : Emergency stop (broadcast)

  Ver3 additions:
    ID 0x102 : Fault log dump (on request)
    ID 0x103 : Parameter write (PI gains, limits — runtime tuning)
    ID 0x104 : Power status (Vbus, Ibus, PSU state)
```

### 9.3 SPI / I2C (Sensor — Kế thừa Ver2)

```
  SPI1 : AS5048A absolute encoder (PB3/PB4/PB5, PC4 CS)
  I2C1 : TMP112 temperature sensor (PB6 SCL, PB7 SDA)
```

### 9.4 Human Interface (Optional)

```
  Potentiometer → ADC → speed setpoint (simple)
  Rotary encoder knob → speed adjustment
  OLED / LCD display → show RPM, current, temp, state
  LED indicators → state machine (IDLE=green, RUN=blue, FAULT=red)
  → Không critical cho control, chỉ UX
```

---

## 10. SYSTEM STATE MACHINE (FSM) — Ver3 Extended

### 10.1 State Diagram (6 states — thêm PRECHARGE)

```
═══════════════════════════════════════════════════════════════════════
  VER3 STATE DIAGRAM (6 states)
═══════════════════════════════════════════════════════════════════════

                 power on
  ┌──────────┐  (reset)     ┌────────────┐   Vbus OK      ┌──────────┐
  │   INIT   │─────────────→│ PRECHARGE  │───────────────→│   IDLE   │←─────┐
  │(power-on)│              │  (Ver3 mới)│  all BSP done  │(standby) │      │
  └──────────┘              │ Vbus ramp  │                └────┬─────┘      │
                            │ capacitor  │                     │            │
                            │ charge     │                     │            │
                            └──────┬─────┘              CAN   │            │
                                   │                    speed  │            │
                         timeout   │                    cmd    │            │
                         (Vbus     │                           ▼            │
                          fail)    │                   ┌──────────┐         │
                                   │         ┌────────→│   RUN    │         │
                                   ▼         │        │  (FOC    │         │
                            ┌──────────┐     │        │  active) │         │
                            │  FAULT   │     │        └───┬──┬───┘         │
                            │(shutdown)│←────┼────────────┘  │             │
                            └──┬──┬────┘     │               │             │
                               │  │          │    HW OC      │  SW fault   │
                     lockout   │  │ cooldown │    or SW      │             │
                     (max      │  │ elapsed  │    fault      │             │
                      retry)   │  │          │               │             │
                               │  ▼          │               │             │
                               │ ┌──────────┐│               │             │
                               │ │ RECOVERY ├┘               │             │
                               │ │(cooldown)│                │             │
                               │ └──────────┘                │             │
                               │                              │             │
                               └──────────────────────────────┴─────────────┘
                                     lockout / CAN reset

═══════════════════════════════════════════════════════════════════════
  TRANSITION TABLE (Ver3 — bổ sung PRECHARGE)
═══════════════════════════════════════════════════════════════════════

  From       │ To         │ Guard                       │ Action
  ───────────│────────────│─────────────────────────────│──────────────────
  INIT       │ PRECHARGE  │ Power ON, SystemInit OK     │ Close pre-charge relay
             │            │                             │ Start Vbus ADC monitor
  ───────────│────────────│─────────────────────────────│──────────────────
  PRECHARGE  │ IDLE       │ Vbus ≥ 90% Vin + BSP init  │ Close main relay
             │            │ within 5s timeout           │ Open pre-charge relay
             │            │                             │ BSP all init complete
  ───────────│────────────│─────────────────────────────│──────────────────
  PRECHARGE  │ FAULT      │ Timeout (Vbus not reaching) │ fault_code = PRECHARGE_FAIL
             │            │ OR Vbus overshoot           │ All relays open
  ───────────│────────────│─────────────────────────────│──────────────────
  IDLE       │ RUN        │ CAN 0x100 speed setpoint    │ FOC_SetState(RUN)
             │            │ received, no active faults  │ HRTIM_EnableOutputs()
             │            │                             │ PI_Reset(all)
  ───────────│────────────│─────────────────────────────│──────────────────
  RUN        │ FAULT(HW)  │ COMP1 trip (< 100ns)       │ HRTIM auto-disable
             │            │                             │ fault_code = HW_OC
  ───────────│────────────│─────────────────────────────│──────────────────
  RUN        │ FAULT(SW)  │ temp>85°C / slip>60%+500ms │ HRTIM_DisableOutputs()
             │            │ / CAN e-stop / Vbus fault  │ PI_Reset(all)
             │            │ / Vbus overvoltage persist  │ Flash log write
  ───────────│────────────│─────────────────────────────│──────────────────
  FAULT      │ RECOVERY   │ cooldown 2000ms elapsed    │ fault_count++
             │            │ AND fault_count ≤ MAX(3)    │ Start sensor check
  ───────────│────────────│─────────────────────────────│──────────────────
  RECOVERY   │ RUN        │ All sensors safe            │ HRTIM_ClearFault()
             │            │ Vbus in range, temp OK      │ PI_Reset(all)
             │            │                             │ Speed ramp (gradual)
  ───────────│────────────│─────────────────────────────│──────────────────
  RECOVERY   │ FAULT      │ Still unsafe after check    │ Reset cooldown timer
  ───────────│────────────│─────────────────────────────│──────────────────
  FAULT      │ IDLE       │ fault_count > MAX           │ LOCKOUT state
             │            │ OR CAN reset command        │ Need power cycle
             │            │ OR ESTOP received           │ Flash log: lockout
  ───────────│────────────│─────────────────────────────│──────────────────
```

### 10.2 Fault Sub-types & Priority (Ver3 Extended)

```
  Priority│ Code     │ Source                │ Path     │ Latency │ Auto-recover?
  ────────│──────────│───────────────────────│──────────│─────────│──────────
  0 (max) │ HW_OC    │ COMP1 → HRTIM FLT1   │ Hardware │ < 100ns │ Yes (cool)
  1       │ HW_OV    │ COMP2 → brake latch   │ Hardware │ < 200ns │ Yes (dump)
  2       │ SW_OC    │ ADC Ia/Ib > limit     │ ISR 20kHz│ 50μs    │ Yes
  3       │ SW_UV    │ Vbus < Vbus_min       │ ISR 20kHz│ 50μs    │ Yes
  4       │ OT       │ TMP112 > 85°C         │ Poll 10Hz│ 100ms   │ Yes (hyst)
  5       │ SLIP     │ slip > 60% + 500ms    │ ISR 1kHz │ 500ms   │ Yes
  6       │ PRECHARGE│ Vbus not reaching     │ Startup  │ 5s      │ No (power)
  7       │ ESTOP    │ CAN 0x1FF received    │ Poll 10Hz│ 100ms   │ No (manual)
  8       │ WDG      │ IWDG timeout          │ Hardware │ auto    │ No (reset)

  Ver3 additions vs Ver2:
    + HW_OV (over-voltage hardware comparator + brake)
    + SW_UV (undervoltage software detection)
    + PRECHARGE (pre-charge timeout failure)
    + WDG (watchdog timeout → hardware reset)
```

### 10.3 Persistent Fault Logging (VER3 MỚI)

```
═══════════════════════════════════════════════════════════════════════
  FLASH FAULT LOG — CRC-Protected Persistent Storage
═══════════════════════════════════════════════════════════════════════

  Ver2: fault_code trong RAM → mất khi power cycle
  Ver3: fault log ghi vào Flash → tồn tại qua power cycle

  Log entry format:
    typedef struct {
        uint32_t timestamp_ms;     // SysTick counter
        uint8_t  fault_code;       // enum fault type
        uint8_t  state_before;     // FSM state trước fault
        int16_t  ia_amps_x100;    // current at fault (×100 for fixed-point)
        int16_t  ib_amps_x100;
        uint16_t vbus_mv;         // Vbus in millivolts
        int16_t  temp_x10;        // temperature ×10
        uint16_t speed_rpm;       // speed at fault
        uint32_t crc32;           // CRC32 of above fields
    } FaultLog_Entry_t;           // 20 bytes + 4 CRC = 24 bytes

  Flash layout:
    Flash sector cuối (hoặc SRAM2 backed by VBAT):
    ┌───────────────┬─────────────────────────────┐
    │ Header (16B)  │ Magic + entry_count + CRC   │
    │ Entry[0] 24B  │ First fault                 │
    │ Entry[1] 24B  │ Second fault                │
    │ ...           │ ...                         │
    │ Entry[N] 24B  │ Last fault (N ≈ 100 max)   │
    └───────────────┴─────────────────────────────┘

  Write policy:
    Chỉ ghi từ main loop context (10Hz, non-ISR)
    Erase khi sector full → ring buffer (oldest overwritten)
    CRC verify trước khi trust entry

  Read via CAN:
    CAN ID 0x102 → dump fault log entries to master
    → Factory diagnostics, field debugging
```

---

## 11. SAFETY & WATCHDOG (VER3 MỚI)

### 11.1 IWDG Windowed Watchdog

```
═══════════════════════════════════════════════════════════════════════
  IWDG — Independent Watchdog (Hardware, không disable được)
═══════════════════════════════════════════════════════════════════════

  Ver2: Chỉ có SysTick counter, không có hardware watchdog
  Ver3: IWDG với windowed mode

  Configuration:
    IWDG prescaler : /64 → IWDG clock = 32kHz / 64 = 500Hz
    IWDG reload    : 500 → timeout = 500/500 = 1000ms
    Window         : 250 → earliest refresh = 500ms
    → Refresh phải xảy ra trong window [500ms, 1000ms]
    → Quá sớm (firmware loop quá nhanh) = reset
    → Quá muộn (firmware treo)           = reset

  Refresh logic:
    Main loop 10Hz (100ms period):
      Mỗi 5-10 iterations → refresh IWDG key
      → Nằm trong window [500ms, 1000ms]
    Nếu FOC ISR treo → main loop bị block → WDG timeout → reset
    Nếu main loop treo → không refresh → WDG timeout → reset

  Sau reset:
    Check RCC_CSR IWDGRSTF flag → biết reset do WDG
    Log vào Flash → "WDG_RESET" entry
    Re-enter PRECHARGE state (safe startup)
```

### 11.2 Dual-MCU Safety (Optional — Production)

```
═══════════════════════════════════════════════════════════════════════
  OPTIONAL: Co-processor Safety Monitor
═══════════════════════════════════════════════════════════════════════

  Concept (Ver1 đã có F1 watchdog, Ver3 nâng cấp):
    MCU_main (G474) : chạy FOC, control, mọi thứ
    MCU_safety (F0/L0): giám sát MCU_main, emergency shutdown

  Heartbeat protocol:
    MCU_main → UART/SPI → MCU_safety: heartbeat mỗi 100ms
    Nếu MCU_safety không nhận heartbeat trong 500ms:
      → MCU_safety kéo HRTIM FLT2 xuống → tắt inverter NGAY
      → Hoàn toàn independent từ MCU_main firmware
      → Bảo vệ dù MCU_main crash hoàn toàn

  MCU_safety functions (minimal firmware):
    ① Heartbeat monitor (timeout → shutdown)
    ② Vbus analog check (independent ADC)
    ③ Temperature analog check (NTC backup)
    ④ LED fault indicator
    ⑤ CAN fault report to master
    → Firmware < 1KB, rất khó crash

  Hướng tới: SIL2 / ISO 26262 ASIL-B
    Dual-channel redundancy
    Independent power supply (LDO riêng)
    Diverse implementation (khác MCU, khác compiler)
```

---

## 12. SYSTEM INTEGRATION

### 12.1 Full System = Combination of Everything

```
═══════════════════════════════════════════════════════════════════════
  VER3 INTEGRATION MAP
═══════════════════════════════════════════════════════════════════════

  ┌─────────────────────────────────────────────────────────────────┐
  │                     POWER DOMAIN                                │
  │  [Power In] → [PSU Buck/Boost] → [DC Bus (caps + protection)] │
  │                                        │                       │
  │  [Brake Resistor + Chopper] ← OV ──────┤                       │
  │  [Pre-charge Circuit] ← startup ───────┤                       │
  └────────────────────────────────────────┼───────────────────────┘
                                           │
  ┌────────────────────────────────────────┼───────────────────────┐
  │                  INVERTER DOMAIN        │                       │
  │  [Gate Driver IC] ← PWM from MCU       │                       │
  │  [6× MOSFET Half-bridges] ←────────────┘                       │
  │  [Deadtime: hardware HRTIM]                                    │
  │  [Snubber / Bootstrap]                                         │
  │        │ │ │                                                    │
  │        A B C  (3-phase output to motor)                        │
  └────────┼─┼─┼───────────────────────────────────────────────────┘
           │ │ │
  ┌────────┼─┼─┼───────────────────────────────────────────────────┐
  │        ▼ ▼ ▼         MOTOR + SENSOR DOMAIN                     │
  │    [BLDC Motor]                                                 │
  │        │                                                        │
  │    [Hall Sensors] → GPIO/TIM → MCU                             │
  │    [Shunt Resistors] → OPAMP → ADC → MCU                      │
  │    [Encoder Quadrature] → TIM3/TIM4 → MCU                     │
  │    [AS5048A SPI Encoder] → SPI1 → MCU                         │
  │    [TMP112 Temperature] → I2C1 → MCU                          │
  └────────────────────────────────────────────────────────────────┘
           │
  ┌────────┼───────────────────────────────────────────────────────┐
  │        ▼            MCU DOMAIN (STM32G474)                     │
  │                                                                 │
  │  ┌─────────────┐  ┌─────────────┐  ┌──────────────┐           │
  │  │ FOC Control │  │ Speed PI    │  │ Fault Mgmt   │           │
  │  │ (20kHz ISR) │  │ (1kHz ISR)  │  │ (FSM + Log)  │           │
  │  └──────┬──────┘  └──────┬──────┘  └──────┬───────┘           │
  │         │                │                 │                    │
  │  ┌──────┴────────────────┴─────────────────┴───────┐           │
  │  │              Sensor Manager                      │           │
  │  │  (multi-rate scheduler: 20k/1k/100/10 Hz)       │           │
  │  └─────────────────────┬───────────────────────────┘           │
  │                        │                                        │
  │  ┌─────────────────────┴───────────────────────────┐           │
  │  │              BSP Layer                           │           │
  │  │  HRTIM / OPAMP / COMP / ADC / SPI / I2C / CAN  │           │
  │  │  IWDG / Flash Log / Brake GPIO                  │           │
  │  └─────────────────────────────────────────────────┘           │
  │                                                                 │
  │  ┌─────────────────────────────────────────────────┐           │
  │  │              Communication                       │           │
  │  │  FDCAN1 (cmd/telemetry) + USART2 (debug)       │           │
  │  └─────────────────────────────────────────────────┘           │
  └────────────────────────────────────────────────────────────────┘
```

### 12.2 Key Integration Principle

```
  Đây KHÔNG phải "mạch điện":
  → Đây là CYBER-PHYSICAL SYSTEM (CPS)

  Mỗi subsystem ảnh hưởng lẫn nhau:
    ✦ Power supply ripple      → ADC noise → FOC jitter
    ✦ Inverter deadtime        → torque distortion → vibration
    ✦ Motor inductance         → current loop bandwidth limit
    ✦ Sensor latency           → phase lag → potential instability
    ✦ Bus capacitor sizing     → transient response → Vbus droop
    ✦ Gate driver propagation  → effective deadtime → shoot-through risk
    ✦ PCB layout (EMI)         → ADC noise floor → control resolution

  → PHẢI thiết kế ĐỒNG THỜI (co-design), không thể tách rời
  → Firmware tuning cần hardware understanding (và ngược lại)
```

---

## 13. DEVELOPMENT ROADMAP (5 Stages)

### Stage 1: Basic Motor Running

```
  Mục tiêu: Motor quay được, đọc được sensor
  Tasks:
    ✦ Run motor using gate driver + PWM (6-step commutation)
    ✦ Read Hall sensors → commutation table
    ✦ Verify phase sequence (đảo dây nếu sai chiều)
    ✦ PWM duty control → speed varies
  
  Hardware: MCU + gate driver + motor + Hall
  Firmware: GPIO PWM + Hall read + commutation lookup
  Estimated effort: 1-2 tuần
  Deliverable: Motor quay theo Hall, speed ~ duty
```

### Stage 2: Close the Loop (PID)

```
  Mục tiêu: Closed-loop speed control hoạt động
  Tasks:
    ✦ Implement Speed PID (outer loop, 1kHz)
    ✦ Add current sensing (shunt + op-amp + ADC)
    ✦ Current limit protection (software)
    ✦ Encoder feedback cho velocity measurement
    ✦ PID tuning (Ziegler-Nichols hoặc manual)
  
  Hardware: + shunt resistors + ADC wiring
  Firmware: + PID controller + ADC driver + encoder driver
  Estimated effort: 1-2 tuần
  Deliverable: Tốc độ ổn định bất kể tải thay đổi
```

### Stage 3: Optimize & Protect

```
  Mục tiêu: Timing deterministic, protection hardware
  Tasks:
    ✦ Optimize timing: ADC sync with PWM (DMA + ISR)
    ✦ HRTIM center-aligned PWM (thay TIM basic)
    ✦ Add hardware overcurrent protection (COMP → HRTIM FLT)
    ✦ Add temperature monitoring (TMP112 I2C)
    ✦ Implement multi-rate scheduler (20kHz / 1kHz / 100Hz / 10Hz)
    ✦ IWDG watchdog setup
    ✦ CAN bus communication
  
  Hardware: + COMP wiring + I2C sensor + CAN transceiver
  Firmware: + HRTIM driver + COMP driver + I2C driver + CAN driver
  Estimated effort: 2-3 tuần
  Deliverable: Timing deterministic, hardware fault protection
```

### Stage 4: FOC Implementation

```
  Mục tiêu: Full FOC running
  Tasks:
    ✦ Implement FOC algorithm:
      - Clarke transform
      - Park transform
      - PI controllers (Id, Iq)
      - Inverse Park transform
      - SVPWM
    ✦ Sin/Cos LUT (256 entries)
    ✦ OPAMP PGA setup (internal current amplification)
    ✦ ADC injected mode (synchronized sampling)
    ✦ θ_elec from encoder/observer
    ✦ Cascade: Speed PI → Iq_ref → Current PI → Vdq → SVM → PWM
    ✦ Field Weakening (Id < 0)
    ✦ Over-modulation
    ✦ MTPA lookup (nếu IPM motor)
  
  Firmware: + FOC pipeline + SVPWM + field weakening
  Estimated effort: 3-4 tuần
  Deliverable: Smooth torque, efficient operation, high-speed capable
```

### Stage 5: Custom Power Stage & Production

```
  Mục tiêu: Toàn bộ phần cứng tự thiết kế
  Tasks:
    ✦ Design own inverter PCB:
      - MOSFET selection (Rds_on, Vds, Id)
      - Gate driver IC selection + bootstrap
      - Current shunt placement + routing
      - Thermal management (heatsink, copper pour)
      - EMI consideration (layout, decoupling)
    ✦ Design power supply stage:
      - Buck/Boost converter
      - Pre-charge circuit
      - Brake resistor + chopper
      - Input protection (reverse polarity, fuse)
    ✦ PCB layout:
      - Power plane separation
      - Analog / digital ground split
      - ADC routing (away from switching noise)
      - Gate driver trace length matching
    ✦ Persistent fault logging (Flash CRC)
    ✦ Optional: dual-MCU safety (SIL2 target)
    ✦ Optional: RTOS migration (FreeRTOS / Zephyr)
  
  Deliverable: Production-grade motor drive system
```

---

## 14. Thông số tổng hợp (Ver3)

> **Architecture type:** Cyber-Physical System — Closed-loop FOC + Power Stage + Safety

| Thông số | Giá trị | Nguồn |
|----------|---------|-------|
| MCU | STM32G474RET6, Cortex-M4 @ 170MHz (Boost mode) | Ver2 |
| Flash / RAM | 512KB Flash / 96KB SRAM1 + 32KB SRAM2 | Ver2 |
| Motor type | BLDC / PMSM 3-phase, 4 pole pairs | Ver2 |
| **Power input** | **Battery 12-72V hoặc AC-DC** | **Ver3** |
| **Power supply** | **Buck/Boost/Buck-Boost + optional PFC** | **Ver3** |
| **DC Bus** | **Electrolytic caps + pre-charge + TVS + brake** | **Ver3** |
| **Inverter** | **6× MOSFET full-bridge, self-designed** | **Ver3** |
| **Gate driver** | **IR2110/DRV8302 + bootstrap** | **Ver3** |
| PWM peripheral | HRTIM với DLL ×32 | Ver2 |
| PWM frequency | 10–20kHz center-aligned (configurable) | Ver2+ |
| PWM resolution | 184ps (fHRCK = 5.44GHz) / PERAR = 136,000 ticks | Ver2 |
| PWM types | **6-step / SPWM / SVPWM + over-modulation** | **Ver3** |
| Deadtime | 100ns hardware (HRTIM DTxR = 136 ticks) | Ver2 |
| Overcurrent trip | COMP1 → HRTIM FLT1, < 100ns (hardware) | Ver2 |
| **Overvoltage trip** | **COMP2 → Brake MOSFET, < 200ns (hardware)** | **Ver3** |
| ADC sampling | ADC1/ADC2 injected, HRTIM triggered midpoint | Ver2 |
| ADC resolution | 12-bit, 42.5MHz synchronous | Ver2 |
| Current sensing | OPAMP1/2 PGA ×4, shunt 10mΩ, ±10A, 0.02 A/LSB | Ver2 |
| Vbus sensing | ADC CH, divider 47k/4.7k, 8.87mV/LSB | Ver2 |
| **Hall sensor** | **3× digital, 6 sectors, startup + backup** | **Ver3** |
| Temperature | TMP112, I2C, ±0.5°C, 75°C warn / 85°C fault | Ver2 |
| Encoder (wheel) | 2× Quadrature TIM3/TIM4, 500PPR × 4 | Ver2 |
| Encoder (angle) | AS5048A SPI 14-bit (**primary theta source**) | Ver3+ |
| Control type | **Closed-loop FOC + Field Weakening + MTPA** | **Ver3** |
| Current loop | 10–20kHz, PI with anti-windup | Ver2+ |
| Speed loop | 1kHz, PI with anti-windup | Ver2 |
| **Field Weakening** | **Id < 0, extend speed above base speed** | **Ver3** |
| **Over-modulation** | **Region I + II + 6-step transition** | **Ver3** |
| **MTPA** | **Lookup table for IPM motors (Ld ≠ Lq)** | **Ver3** |
| CAN protocol | FDCAN1, 1Mbps, **6 message IDs (Ver3 expanded)** | Ver3+ |
| **Fault logging** | **Flash CRC-protected, ring buffer, CAN dump** | **Ver3** |
| **Watchdog** | **IWDG windowed [500ms, 1000ms]** | **Ver3** |
| **Brake resistor** | **Regen over-voltage dissipation (Vbus > threshold)** | **Ver3** |
| **Pre-charge** | **Soft-start capacitor charge, inrush prevention** | **Ver3** |
| **Safety option** | **Dual-MCU heartbeat, toward SIL2/ASIL-B** | **Ver3** |
| Build tool | GNU Make + arm-none-eabi-gcc, -O2, -mfloat-abi=hard | Ver2 |
| Flash tool | ST-Link + OpenOCD, stm32g4x.cfg | Ver2 |
| Code style | Register-level, zero HAL, full FPU float | Ver2 |
| Source files | **~25 C files + 1 ASM + 1 LD + 1 Makefile** | Ver3 |

---

## 15. Điểm mạnh & Giới hạn

### Điểm mạnh (Ver3)

```
✓ FULL SYSTEM — từ nguồn vào đến trục motor
    Power supply → DC bus → inverter → motor → feedback → control
    Hiểu toàn bộ chuỗi năng lượng, không "giả định DC bus có sẵn"

✓ Power stage tự thiết kế
    Buck/Boost converter, pre-charge, brake resistor
    Hiểu trade-off: efficiency, ripple, component sizing, thermal

✓ Inverter tự thiết kế
    MOSFET selection, gate driver, bootstrap, snubber, deadtime
    Hiểu shoot-through risk, dV/dt, EMI, PCB layout

✓ Field Weakening mở rộng vùng tốc độ
    Id < 0 → constant power region → 2-3× speed above base
    Kết hợp over-modulation → tận dụng Vbus tối đa

✓ SVPWM advanced + over-modulation
    Linear SVPWM + Region I/II + 6-step transition
    15.5% voltage headroom hơn SPWM, smooth transition

✓ MTPA cho IPM motor (Ld ≠ Lq)
    Reluctance torque contribution tận dụng
    Efficiency tối ưu ở mọi operating point

✓ Persistent fault logging
    Flash CRC-protected → survive power cycle
    CAN dump → factory diagnostics, field debugging
    Timestamp + full context (current, voltage, temp, speed)

✓ IWDG windowed watchdog
    Hardware independent → detect ISR hang, main loop hang
    Windowed → detect cả "too fast" và "too slow"
    WDG reset → auto-detected, logged

✓ Over-voltage protection (regenerative braking)
    Brake resistor + chopper MOSFET
    Hardware comparator backup (< 200ns)
    Bảo vệ capacitor + MOSFET khỏi Vbus spike

✓ Pre-charge / soft-start
    Inrush prevention → bảo vệ relay, connector, fuse
    ADC-monitored ramp → FSM PRECHARGE state

✓ Kế thừa TOÀN BỘ điểm mạnh Ver2:
    FOC cascade PI, HRTIM 184ps, ADC sync midpoint,
    Hardware overcurrent < 100ns, multi-rate bare-metal,
    FPU hard, encoder quadrature 4×, slip/deviation PI,
    CAN telemetry, anti-windup PI, zero HAL
```

### Giới hạn → Dẫn đến Ver4

```
✗ Chưa có sensorless observer
    → Hiện tại cần encoder/Hall vật lý cho rotor position
    → Ver4: Back-EMF observer, sliding mode observer, Luenberger
    → Chạy FOC KHÔNG CẦN encoder (cost reduction)

✗ Chưa có auto-tuning PI gains
    → PI gains phải tune thủ công (Kp, Ki)
    → Ver4: auto-ID (motor R, L, λpm) + auto-calculate gains

✗ Chưa có regenerative braking energy recovery
    → Brake resistor chỉ dissipate (nhiệt)
    → Ver4: battery charging path (bidirectional DC-DC)

✗ Chưa có acoustic noise optimization
    → PWM frequency cố định → tonal noise ở fPWM
    → Ver4: random PWM / spread-spectrum modulation

✗ Chưa có vibration suppression
    → Torque ripple compensation chưa có
    → Ver4: harmonic current injection (6th, 12th)

✗ Chưa có model predictive control (MPC)
    → PI cascade là "classical" approach
    → Ver4: MPC cho multi-objective optimization (speed + efficiency + NVH)

✗ Sản xuất hàng loạt chưa sẵn sàng
    → Cần: DFM review, FMEA, EMC test, safety certification
    → Ver4: full production documentation + test coverage
```

---

## 16. VER1 → VER2 → VER3 EVOLUTION SUMMARY

```
═══════════════════════════════════════════════════════════════════════
  EVOLUTION TABLE
═══════════════════════════════════════════════════════════════════════

  Thuộc tính           │ VER1 (Open-loop)     │ VER2 (Closed-loop)   │ VER3 (Full System)
  ─────────────────────│──────────────────────│──────────────────────│──────────────────────
  System type          │ Data acquisition     │ FOC motor control    │ Cyber-physical system
  Control              │ Open-loop duty       │ FOC cascade PI       │ FOC + FW + MTPA + OvMod
  Power stage          │ N/A (assumed)        │ N/A (assumed)        │ Self-designed (Buck/Boost)
  Inverter             │ L298N (H-bridge)     │ External driver      │ Self-designed (6× MOSFET)
  PWM                  │ TIM basic 10Hz       │ HRTIM 20kHz          │ HRTIM 10-20kHz + SVPWM adv
  Overcurrent          │ Software 100ms       │ COMP → FLT < 100ns  │ + COMP2 overvoltage
  Overvoltage          │ N/A                  │ N/A                  │ Brake resistor + HW COMP
  Pre-charge           │ N/A                  │ N/A                  │ Relay + ADC monitor
  Watchdog             │ SysTick              │ SysTick              │ IWDG windowed
  Fault logging        │ RAM only             │ RAM only             │ Flash CRC persistent
  Safety               │ F1 watchdog MCU      │ Software FSM         │ IWDG + dual-MCU option
  Field Weakening      │ N/A                  │ N/A (Id=0 fixed)     │ Id < 0 dynamic
  Over-modulation      │ N/A                  │ N/A (95% max)        │ Region I + II + 6-step
  MTPA                 │ N/A                  │ Id=0 (SPM only)      │ LUT (IPM support)
  Hall sensors         │ N/A                  │ N/A                  │ 3× startup + backup
  Sensor count         │ 3 (basic)            │ 5 (precision)        │ 6+ (redundancy)
  LOC (est.)           │ ~500                 │ ~3000                │ ~5000+
  PCB design           │ Off-the-shelf        │ Partial custom       │ Full custom
  Ứng dụng            │ Learning, prototype  │ Thesis, advanced     │ R&D, pre-production

  Philosophy evolution:
    Ver1: "Do not block. Everything flows with time."
    Ver2: + "Every flow MUST respect its deadline."
    Ver3: + "Every subsystem must be designed, powered, protected,
             and integrated — from wall socket to shaft torque."
```

---

## 17. Kết luận

> **Ver3 = Production-grade Cyber-Physical System cho BLDC motor control.**
>
> Khác biệt cốt lõi so với Ver2:
> - **Power System**: từ nguồn vào → DC bus → inverter, tự thiết kế toàn bộ
> - **Advanced Control**: Field Weakening, Over-modulation, MTPA
> - **Safety**: IWDG watchdog, persistent fault log, overvoltage hardware
> - **Integration**: hiểu toàn bộ chuỗi năng lượng-điều khiển-cơ khí
>
> **Triết lý thiết kế:**
> ```
> "Do not block. Everything flows with time.
>  Every flow MUST respect its deadline.
>  Every subsystem must be designed, powered, protected,
>  and integrated — from wall socket to shaft torque."
> ```
>
> **System type:** Không phải "mạch điện". Không phải "firmware".
> → Đây là **CYBER-PHYSICAL SYSTEM** hoàn chỉnh.
>
> **Ver3 đủ điều kiện cho Embedded R&D position.**
> Thể hiện hiểu biết sâu về: power electronics, gate driver design,
> FOC algorithm (advanced), multi-rate real-time control,
> hardware protection, safety architecture, và system integration.
>
> Mọi giới hạn của Ver3 (sensorless observer, auto-tuning, regen recovery,
> MPC, production certification) là nền tảng cho Ver4 — production mass.
