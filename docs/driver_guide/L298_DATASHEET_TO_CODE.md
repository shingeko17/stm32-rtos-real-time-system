# L298 — Từ Datasheet Đến Code: Quy Trình Tư Duy

> Tài liệu này dạy bạn **đọc datasheet L298 của ST** rồi suy nghĩ từng bước
> để viết ra các struct, union, truth table, và chọn kỹ thuật C phù hợp.

---

## Bước 0: Tổng Quan Datasheet L298

Nguồn: [STMicroelectronics L298 Datasheet](https://www.st.com/resource/en/datasheet/l298.pdf)

### Datasheet nói gì?

```
L298 = Dual Full-Bridge Driver (IC có 2 cầu H độc lập)
├─ Mỗi cầu H: 2 input (INx) + 1 enable (ENx) + 2 output (OUTx) + 1 sense (SENSEx)
├─ Motor supply: lên tới 46V (pin VS)
├─ Logic supply: 5V riêng (pin VSS) — tách logic khỏi power
├─ Dòng DC mỗi cầu: tối đa 2A (peak 3A)
├─ Input logic level: TTL compatible (LOW ≤ 1.5V, HIGH ≥ 2.3V)
└─ Bảo vệ: Overtemperature protection tích hợp
```

---

## Bước 1: Đọc Pin Description → Xác Định Hardware Interface

### Datasheet Section: "Pin Functions"

```
Pin  │ Tên       │ Chức năng (từ datasheet)
─────┼───────────┼──────────────────────────────────────────
 1   │ SENSE A   │ Nối resistor đo dòng motor A (between OUT & GND)
 2   │ OUT 1     │ Output bridge A — nối motor A
 3   │ OUT 2     │ Output bridge A — nối motor A
 4   │ VS        │ Motor supply voltage (lên tới 46V)
 5   │ IN 1      │ Input 1 — điều khiển transistor cầu A
 6   │ EN A      │ Enable bridge A (TTL: HIGH=on, LOW=off)
 7   │ IN 2      │ Input 2 — điều khiển transistor cầu A
 8   │ GND       │ Ground (logic + power chung)
 9   │ VSS       │ Logic supply (5V) — cấp nguồn cho logic bên trong
10   │ IN 3      │ Input 3 — điều khiển transistor cầu B
11   │ EN B      │ Enable bridge B
12   │ IN 4      │ Input 4 — điều khiển transistor cầu B
13   │ OUT 3     │ Output bridge B — nối motor B
14   │ OUT 4     │ Output bridge B — nối motor B
15   │ SENSE B   │ Nối resistor đo dòng motor B
```

### Tư duy Step 1: Từ pin description → cần lưu gì trong code?

```
Đọc datasheet → thấy MỖI MOTOR cần:
  ├─ 2 GPIO output (IN1, IN2) → điều khiển hướng
  ├─ 1 PWM output  (EN)      → điều khiển tốc độ
  ├─ 1 ADC input   (SENSE)   → đo dòng điện
  └─ 2 output pins (OUT1,2)  → nối motor (không cần config trong code)

Vậy mỗi channel (motor) cần lưu: pin IN1, pin IN2, pin EN, pin SENSE
→ Đây là lý do tạo L298_ChannelConfig struct!
```

**Đây là suy nghĩ đầu tiên**: datasheet nói IC có bao nhiêu pin, pin nào cần MCU
điều khiển → gom lại thành 1 struct.

```c
/* Mỗi dòng trong struct = 1 pin trên datasheet cần MCU control */
typedef struct {
    GPIO_TypeDef *in1_port;     /* ← Datasheet pin 5 (IN1) hoặc pin 10 (IN3) */
    uint16_t      in1_pin;
    GPIO_TypeDef *in2_port;     /* ← Datasheet pin 7 (IN2) hoặc pin 12 (IN4) */
    uint16_t      in2_pin;
    TIM_TypeDef  *pwm_timer;    /* ← Datasheet pin 6 (ENA) hoặc pin 11 (ENB) */
    uint32_t      pwm_channel;
    uint32_t      pwm_frequency_hz;  /* ← User chọn, ko có trong datasheet */
    uint16_t      pwm_max_duty;      /* ← Tính từ timer config */
} L298_ChannelConfig;
```

**Tại sao typedef struct?** Vì datasheet cho thấy L298 có **2 channels giống nhau**.
Nếu dùng biến rời (`uint16_t in1_pin_a, in1_pin_b, in2_pin_a, in2_pin_b...`)
→ code lộn xộn, dễ nhầm.
Struct = gom tất cả pin của 1 channel vào 1 "gói".

---

## Bước 2: Đọc Truth Table → Xây Control Logic

### Datasheet Section: "Input/Output Function Table"

Datasheet L298 có bảng này (trích nguyên văn):

```
┌──────┬──────┬──────┬───────────────────┐
│  EN  │ IN1  │ IN2  │  Motor Action     │
├──────┼──────┼──────┼───────────────────┤
│  0   │  X   │  X   │  Free Run (coast) │
│  1   │  0   │  0   │  Brake (fast stop) │
│  1   │  1   │  0   │  Forward           │
│  1   │  0   │  1   │  Reverse           │
│  1   │  1   │  1   │  Brake (fast stop) │
└──────┴──────┴──────┴───────────────────┘
X = don't care (0 hoặc 1 đều được)
```

### Tư duy Step 2: Truth table → enum + logic code

**Câu hỏi**: Có 5 hàng trong bảng, nhưng motor chỉ có mấy *trạng thái thật sự*?

```
Phân tích:
  Free Run  → motor trôi tự do (EN=0)
  Brake     → motor dừng gấp    (cả 2 input cùng mức)
  Forward   → motor quay thuận  (IN1=1, IN2=0)
  Reverse   → motor quay nghịch (IN1=0, IN2=1)

→ 4 trạng thái → dùng enum (Layer 1: Logic)
```

```c
/* Mỗi giá trị enum = 1 hàng trong truth table của datasheet */
typedef enum {
    MOTOR_STOP     = 0,  /* ← EN=0 hoặc IN1=IN2 → datasheet gọi "Fast Motor Stop" */
    MOTOR_FORWARD  = 1,  /* ← EN=1, IN1=1, IN2=0 */
    MOTOR_REVERSE  = 2,  /* ← EN=1, IN1=0, IN2=1 */
    MOTOR_BRAKE    = 3,  /* ← EN=1, IN1=0, IN2=0 → datasheet gọi "Fast Motor Stop" */
} L298_MotorState;
```

**Tại sao enum, không dùng #define?**
- `#define MOTOR_FORWARD 1` → compiler xem nó là `int`, không kiểm tra type
- `enum` → compiler biết đây là nhóm liên quan, cảnh báo nếu bạn assign sai giá trị
- Khi debug: debugger hiển thị tên `MOTOR_FORWARD` thay vì số `1`

### Truth Table → Code điều khiển GPIO

```c
/* Hàm này "dịch" truth table thành GPIO commands */
static void l298_set_direction(L298_ChannelConfig *cfg, L298_MotorState state)
{
    switch (state) {
        case MOTOR_FORWARD:
            /* Datasheet: IN1=HIGH, IN2=LOW → Forward */
            cfg->in1_port->BSRR = cfg->in1_pin;          /* Set IN1 */
            cfg->in2_port->BSRR = cfg->in2_pin << 16;    /* Clear IN2 */
            break;

        case MOTOR_REVERSE:
            /* Datasheet: IN1=LOW, IN2=HIGH → Reverse */
            cfg->in1_port->BSRR = cfg->in1_pin << 16;    /* Clear IN1 */
            cfg->in2_port->BSRR = cfg->in2_pin;           /* Set IN2 */
            break;

        case MOTOR_BRAKE:
            /* Datasheet: IN1=LOW, IN2=LOW → Fast Motor Stop */
            cfg->in1_port->BSRR = cfg->in1_pin << 16;
            cfg->in2_port->BSRR = cfg->in2_pin << 16;
            break;

        case MOTOR_STOP:
            /* Datasheet: EN=0 → Free Run (disable PWM) */
            cfg->pwm_timer->CCR1 = 0;  /* Duty = 0% */
            break;
    }
}
```

> **Chú ý**: Dùng `BSRR` register (Bit Set/Reset Register) thay vì `ODR`
> vì `BSRR` là **atomic** — set hoặc clear 1 pin mà không ảnh hưởng pin khác.
> Đây là cách register-level, không dùng HAL_GPIO_WritePin().

---

## Bước 3: Đọc "Sensing Resistor" → Hiểu Tại Sao Cần ADC + volatile

### Datasheet Section: "Current Sensing"

```
Datasheet nói:
"The emitters of the lower transistors of each bridge are connected
 together and the corresponding external terminal can be used for the
 connection of an external sensing resistor."

Pin SENSE A (pin 1) và SENSE B (pin 15):
  ├─ Nối 1 resistor nhỏ (ví dụ 0.5Ω) giữa pin SENSE và GND
  ├─ Dòng motor chạy qua resistor → tạo điện áp V = I × R
  └─ MCU đọc điện áp này bằng ADC → tính ra dòng motor
```

### Tư duy Step 3: Current sensing = ADC → cần volatile

```
ADC đo dòng motor liên tục (thường qua DMA hoặc ISR)
→ Giá trị thay đổi BẤT CỨ LÚC NÀO, không phải do code main gọi
→ Nếu không dùng volatile, compiler có thể CACHE giá trị cũ

Ví dụ KHÔNG volatile:
  uint16_t adc_value = read_adc();
  while (adc_value < threshold) {  /* Compiler cache → vòng lặp vô tận! */
      /* adc_value never re-read from memory */
  }

Ví dụ CÓ volatile:
  volatile uint16_t adc_value;     /* ADC ISR cập nhật biến này */
  while (adc_value < threshold) {  /* Compiler đọc lại memory mỗi lần */
      /* OK: always reads fresh value */
  }
```

### Tại sao volatile cho state machine?

```
State machine chạy trong main loop.
ISR (ngắt timer, UART, ADC) có thể thay đổi state BẤT CỨ LÚC NÀO.

volatile L298_MotorState current_state;  /* ISR có thể thay đổi */

Nếu không volatile → compiler optimize, main loop không thấy state mới
→ Motor vẫn quay dù ISR đã set MOTOR_BRAKE
→ NGUY HIỂM! (safety issue)
```

Đây là lý do trong `L298_ChannelState`, mọi field đều là `volatile`:

```c
typedef struct {
    volatile L298_MotorState state;       /* ISR có thể thay đổi */
    volatile uint16_t current_duty;       /* PWM duty — ISR update */
    volatile uint16_t current_speed;      /* Speed % — ISR update */
    volatile uint32_t timestamp_ms;       /* Thời điểm thay đổi cuối */
} L298_ChannelState;
```

---

## Bước 4: Đọc "Enable Pin" → Hiểu PWM Speed Control

### Datasheet Section: "Enable Inputs"

```
Datasheet nói:
"Two enable inputs are provided to enable or disable the device
 independently of the input signals."

Enable pin:
  ├─ HIGH (logic 1) → Bridge hoạt động, motor chạy theo IN1/IN2
  ├─ LOW  (logic 0) → Bridge tắt, motor coast (free run)
  └─ PWM  → điều khiển PHẦN TRĂM thời gian bridge ON → speed control
```

### Tư duy Step 4: Enable + PWM → duty cycle = speed

```
Datasheet KHÔNG nói trực tiếp "dùng PWM cho speed".
Nhưng từ truth table + enable logic:
  EN = HIGH(liên tục) → motor chạy max speed
  EN = LOW(liên tục)  → motor dừng
  EN = PWM(50% duty)  → motor nhận điện 50% thời gian → ~50% speed

STM32 Timer peripheral tạo ra PWM signal.
  TIM1->CCR1 = duty_value;   /* So sánh với ARR để tạo waveform */
  duty = 0   → EN luôn LOW → motor dừng
  duty = ARR → EN luôn HIGH → max speed
  duty = ARR/2 → 50% speed
```

### Tại sao uint16_t cho duty, không phải uint8_t?

```
Datasheet context:
  PWM resolution phụ thuộc vào timer (ARR register = 16-bit trên STM32)
  Nếu ARR = 999 (PWM 1kHz) → duty range: 0-999 → uint8_t max 255 KHÔNG ĐỦ!
  uint16_t range: 0-65535 → đủ cho mọi ARR value

  uint8_t  → max 255   → chỉ 256 mức speed (thô)
  uint16_t → max 65535 → 1000+ mức speed (mịn hơn, phù hợp với timer)

→ Chọn uint16_t vì phù hợp với hardware register size
```

### Tại sao uint32_t cho timestamp?

```
timestamp_ms đếm milliseconds từ khi hệ thống bật.
  1 ngày  = 86,400,000 ms
  uint16_t max = 65,535 → tràn sau 65 giây!
  uint32_t max = 4,294,967,295 → tràn sau ~49 ngày → đủ cho hầu hết ứng dụng

→ Chọn uint32_t vì range phù hợp với real-time tracking
```

---

## Bước 5: Đọc "Dual Bridge" → Hiểu Tại Sao Cần Main Driver Struct

### Datasheet Section: "Description"

```
"The L298 is an integrated monolithic circuit...
 It is a high-voltage, high-current DUAL full-bridge driver..."

DUAL = 2 bridges = 2 motors independent
→ Code phải quản lý 2 channels, mỗi channel có config + state riêng
```

### Tư duy Step 5: Dual channel → gom vào 1 driver struct

```c
typedef struct {
    /* Channel A — Bridge A trên datasheet */
    L298_ChannelConfig config_a;      /* Pin mapping từ Bước 1 */
    L298_ChannelState  state_a;       /* Runtime state từ Bước 3 */

    /* Channel B — Bridge B trên datasheet */
    L298_ChannelConfig config_b;
    L298_ChannelState  state_b;

    /* Driver-level status */
    volatile bool initialized;        /* Đã init chưa? */
    volatile bool error_occurred;     /* Có lỗi xảy ra? */
    volatile uint32_t error_code;     /* Mã lỗi cụ thể */

    /* Statistics */
    volatile uint32_t total_commands; /* Đếm số lệnh đã gửi */
    volatile uint32_t total_errors;   /* Đếm số lỗi */
} L298_Driver;
```

### Tại sao `volatile bool initialized`?

```
bool:
  Chỉ có 2 giá trị: true/false → bool phù hợp (1 byte)
  uint32_t cho true/false → lãng phí 3 byte

volatile:
  ISR có thể set error_occurred = true khi phát hiện overcurrent
  Main loop phải thấy sự thay đổi NGAY LẬP TỨC
  → volatile bắt buộc cho biến shared giữa ISR và main
```

---

## Bước 6: Control Bits → Tại Sao Dùng union

### Datasheet: 3 control signals mỗi channel (IN1, IN2, EN)

```
Mỗi channel cần 3 bit để biểu diễn trạng thái hiện tại:
  bit 0 = IN1 state (0 hoặc 1)
  bit 1 = IN2 state (0 hoặc 1)
  bit 2 = EN state  (0 hoặc 1)

Cách 1: Dùng 3 biến bool riêng = 3 bytes → lãng phí
Cách 2: Dùng 1 uint8_t + bit fields = 1 byte → compact

union cho phép truy cập CÙNG VÙNG NHỚ bằng 2 cách:
  - .raw = đọc/ghi cả byte 1 lần (nhanh, dùng khi so sánh)
  - .bits.in1 = đọc/ghi từng bit riêng (rõ ràng, dùng trong logic)
```

```c
typedef union {
    uint8_t raw;                /* Cách 1: truy cập cả byte */
    struct {
        uint8_t in1       : 1;  /* Bit 0 — datasheet pin IN1 */
        uint8_t in2       : 1;  /* Bit 1 — datasheet pin IN2 */
        uint8_t en        : 1;  /* Bit 2 — datasheet pin EN  */
        uint8_t fault     : 1;  /* Bit 3 — software flag: overcurrent? */
        uint8_t reserved  : 4;  /* Bit 4-7 — chưa dùng */
    } bits;
} L298_ControlBits;
```

### Ví dụ sử dụng — map trực tiếp từ truth table:

```c
L298_ControlBits ctrl;

/* FORWARD: EN=1, IN1=1, IN2=0 (truth table row 3) */
ctrl.bits.en  = 1;
ctrl.bits.in1 = 1;
ctrl.bits.in2 = 0;
/* ctrl.raw == 0x05 (binary: 00000101) */

/* BRAKE: EN=1, IN1=0, IN2=0 (truth table row 2) */
ctrl.raw = 0x04;  /* Nhanh hơn: set cả 3 bit 1 lần */
/* ctrl.bits.en == 1, ctrl.bits.in1 == 0, ctrl.bits.in2 == 0 */

/* So sánh trạng thái nhanh */
if (ctrl.raw == prev_ctrl.raw) {
    /* Không thay đổi → skip GPIO write */
}
```

---

## Tổng Kết: Quy Trình Tư Duy Đọc Datasheet → Code

```
┌──────────────────────────────────────────────────────────────┐
│  STEP 1: Đọc Pin Description                                │
│  → Xác định: MCU cần nối bao nhiêu pin? Loại gì?            │
│  → Output: L298_ChannelConfig (typedef struct)               │
├──────────────────────────────────────────────────────────────┤
│  STEP 2: Đọc Truth Table                                     │
│  → Xác định: Có bao nhiêu trạng thái? Tổ hợp input nào?    │
│  → Output: L298_MotorState (enum) + l298_set_direction()     │
├──────────────────────────────────────────────────────────────┤
│  STEP 3: Đọc Current Sensing / Electrical Specs              │
│  → Xác định: Giá trị nào thay đổi bất ngờ? (ISR, DMA, ADC) │
│  → Output: volatile keyword cho shared variables             │
├──────────────────────────────────────────────────────────────┤
│  STEP 4: Đọc Enable / Speed Control                          │
│  → Xác định: PWM resolution cần bao nhiêu bit?              │
│  → Output: uint16_t (khớp timer register), duty calculation  │
├──────────────────────────────────────────────────────────────┤
│  STEP 5: Đọc "Dual" / "Multi-channel"                       │
│  → Xác định: Bao nhiêu channel giống nhau?                  │
│  → Output: L298_Driver (struct chứa 2 channel A + B)        │
├──────────────────────────────────────────────────────────────┤
│  STEP 6: Nhóm control signals                                │
│  → Xác định: Có nên gom bits lại?                            │
│  → Output: L298_ControlBits (union + bit fields)             │
└──────────────────────────────────────────────────────────────┘
```

### Data Type Selection Cheat Sheet

| Cần gì? | Chọn kiểu | Tại sao? | Datasheet cơ sở |
|----------|-----------|----------|-----------------|
| Trạng thái motor (4 giá trị) | `enum` | Type-safe, debugger friendly | Truth table: 4 actions |
| Pin config (nhiều field) | `typedef struct` | Gom pin liên quan | Pin description: IN1,IN2,EN,SENSE |
| Shared ISR ↔ Main variable | `volatile` | Chống optimizer cache | Current sensing, ISR flags |
| PWM duty (0-999) | `uint16_t` | Timer CCR = 16-bit | Enable pin + ARR register |
| Timestamp (ms) | `uint32_t` | 49 ngày không tràn | Real-time requirement |
| Init flag (true/false) | `bool` | 2 giá trị → 1 byte đủ | Initialization sequence |
| Control pins (3 bits) | `union + bit fields` | 1 byte, 2 cách truy cập | 3 control signals/channel |

---

## Reference

- [STMicroelectronics L298 Product Page](https://www.st.com/en/motor-drivers/l298.html)
- [L298 Datasheet (PDF)](https://www.st.com/resource/en/datasheet/l298.pdf)
- [L298_ADVANCED_C_GUIDE.md](L298_ADVANCED_C_GUIDE.md) — Syntax + ví dụ mỗi kỹ thuật C
- [L298_QUICK_REFERENCE.md](L298_QUICK_REFERENCE.md) — Quick reference API + truth table
