# L298 Driver - Quick Reference & Flow Chart

## File Structure

```
drivers/api/
├── l298_driver.h          # Header file (public API + types)
├── l298_driver.c          # Implementation (helper functions)
├── l298_example.c         # Example usage
└── ../docs/
    └── L298_ADVANCED_C_GUIDE.md  # C techniques (grouped by 3-layer model)
```

---

## Architecture Overview (3-Layer Model)

```
 Pipeline: [ISR/DMA] → volatile flag → [State Machine] → [Driver API] → [Hardware Register]

┌─────────────────────────────────────────────────┐
│  LAYER 1: LOGIC (State Machine + Event)         │
│  enum states, volatile flags, error codes       │
│  → Quyết định hành vi hệ thống                 │
├─────────────────────────────────────────────────┤
│  LAYER 2: DRIVER (Execution / Abstraction)      │
│  L298_Init(), L298_SetSpeed(), l298_set_pins()  │
│  typedef struct, function pointer, const,       │
│  bit macros, union, memcpy                      │
│  → Interface giữa logic và hardware             │
├─────────────────────────────────────────────────┤
│  LAYER 3: MEMORY (Storage Layout)               │
│  static/global (.bss/.data), stack, heap        │
│  → Nơi lưu trữ dữ liệu                        │
└─────────────────────────────────────────────────┘
                 │
                 ▼  Direct register access (no HAL)
┌─────────────────────────────────────────────────┐
│  STM32F4 Hardware Registers                     │
│  GPIOx->ODR, TIMx->CCRx, ADCx->DR              │
└─────────────────────────────────────────────────┘
```

---

## Key Data Structures (Memory Layout)

### L298_Driver (Main Control Structure)

```c
typedef struct {
    // Channel A
    L298_ChannelConfig config_a;      // @offset 0,   size ~20 bytes
    L298_ChannelState  state_a;       // @offset 20,  size ~12 bytes
    
    // Channel B
    L298_ChannelConfig config_b;      // @offset 32,  size ~20 bytes
    L298_ChannelState  state_b;       // @offset 52,  size ~12 bytes
    
    // Driver Status
    volatile bool initialized;        // @offset 64,  size 1 byte
    volatile bool error_occurred;     // @offset 65,  size 1 byte
    volatile uint32_t error_code;     // @offset 68,  size 4 bytes
    
    // Statistics
    volatile uint32_t total_commands; // @offset 72,  size 4 bytes
    volatile uint32_t total_errors;   // @offset 76,  size 4 bytes
    
    // Total size: ~80 bytes
} L298_Driver;
```

### L298_ControlBits (Union Example)

```c
typedef union {
    uint8_t byte;              // Access as 8-bit value
    struct {                   // OR access as individual bits
        uint8_t in1 : 1;       // Bit 0
        uint8_t in2 : 1;       // Bit 1
        uint8_t en  : 1;       // Bit 2
        uint8_t reserved : 5;  // Bits 3-7
    } bits;
} L298_ControlBits;

// Memory layout:
// ┌─────────────────────────────┐
// │     L298_ControlBits        │  Size: 1 byte
// ├────┬───┬───┬───────────┬────┤
// │ en │in2│in1│ reserved  │    │
// │ 2  │ 1 │ 0 │   3-7     │    │
// └────┴───┴───┴───────────┴────┘
```

---

## Control Logic Table (H-Bridge)

| EN | IN1 | IN2 | Motor State | Output |
|----|-----|-----|-------------|--------|
| 0  | X   | X   | **FREE RUN** | Coasts freely (no torque) |
| 1  | 0   | 0   | **BRAKE**    | Motor stalls (max torque, zero speed) |
| 1  | 1   | 0   | **FORWARD**  | Motor rotates forward |
| 1  | 0   | 1   | **REVERSE**  | Motor rotates backward |
| 1  | 1   | 1   | **BRAKE**    | Motor stalls (alternative) |

---

## State Machine (Speed Setting)

```
                    ┌─────────────────┐
                    │  L298_SetSpeed()│
                    └────────┬────────┘
                             │
                             ▼
                  ┌──────────────────────┐
                  │ Validate Input       │
                  │ - driver != NULL     │
                  │ - initialized == true│
                  │ - channel valid      │
                  └────────┬─────────────┘
                           │
                ┌──────────┼──────────┐
                │          │          │
         ┌──────▼──┐   ┌──▼────┐ ┌──▼────┐
         │speed > 0│   │speed=0│ │speed<0│
         │ FORWARD │   │ BRAKE │ │REVERSE│
         └──────┬──┘   └──┬────┘ └──┬────┘
                │         │        │
         ┌──────▼────────┐└────┬──┘
         │ Set IN1,IN2   │     │
         │ Based on State│     │
         └──────┬────────┘     │
                │              │
         ┌──────▼──────────────────┐
         │ Calculate Duty Cycle    │
         │ duty = (|speed| * 999)/100
         └──────┬──────────────────┘
                │
         ┌──────▼──────────────────┐
         │ Set PWM Duty via Timer  │
         │ TIMx->CCRx = duty      │
         └──────┬──────────────────┘
                │
         ┌──────▼──────────────────┐
         │ Update volatile State   │
         │ - state, speed, duty    │
         │ - timestamp             │
         └──────┬──────────────────┘
                │
         ┌──────▼──────────────────┐
         │ Return L298_Status      │
         │ L298_OK / L298_ERROR    │
         └────────────────────────┘
```

---

## C Techniques — Grouped by Layer

| Layer | Technique | Purpose |
|-------|-----------|---------|
| **1: Logic** | enum | Type-safe states: `MOTOR_FORWARD`, `MOTOR_BRAKE` |
| | volatile | ISR/hardware flag synchronization |
| | Error Codes | Return `L298_Status` thay vì exceptions |
| **2: Driver** | typedef struct | Nhóm data: `L298_Driver`, `L298_ChannelConfig` |
| | Pointer Deref | Pass-by-reference (`ptr->member`) |
| | Function Pointer | Callbacks: timer ISR, event handler |
| | const | Read-only vs read-write access |
| | inline | Optimize small utility functions |
| | Bit Macros | `BIT_SET`, `BIT_CLR` cho register access |
| | union + Bit Fields | Compact flags, dual-access cùng vùng nhớ |
| | memset/memcpy | Init & copy struct blocks |
| **3: Memory** | static | Scope limiting, persistent storage |

---

## Usage Pattern

### 1. Initialize

```c
L298_Driver driver;
L298_ChannelConfig config = { /* ... */ };
L298_Status status = L298_Init(&driver, &config, NULL);
if (status != L298_OK) handle_error();
```

### 2. Control

```c
L298_SetSpeed(&driver, L298_CHANNEL_A, 75);   // 75% forward
L298_SetSpeed(&driver, L298_CHANNEL_B, -50);  // 50% reverse
```

### 3. Query

```c
uint16_t speed = L298_GetSpeed(&driver, L298_CHANNEL_A);
L298_MotorState state = L298_GetState(&driver, L298_CHANNEL_A);
uint16_t current = L298_ReadCurrent(&driver, L298_CHANNEL_A);
```

### 4. Cleanup

```c
L298_Deinit(&driver);
```

---

## Hardware Pin Mapping (STM32F4)

```
Motor A:
  ├─ IN1 → PA0 (GPIO output)
  ├─ IN2 → PA1 (GPIO output)
  ├─ EN  → PA2 (Timer PWM)
  └─ SENS→ PA3 (ADC input)

Motor B:
  ├─ IN3 → PA4 (GPIO output)
  ├─ IN4 → PA5 (GPIO output)
  ├─ EN  → PA6 (Timer PWM)
  └─ SENS→ PA7 (ADC input)

Timer Configuration:
  ├─ Timer 2 for PWM
  ├─ Prescaler: 83 (84MHz → 1MHz)
  ├─ Period (ARR): 999 (1MHz / 1000 = 1kHz)
  └─ Channels: 1 (Motor A), 2 (Motor B)

ADC Configuration:
  ├─ ADC1 for current sensing
  ├─ Channel 3 (Motor A current)
  └─ Channel 7 (Motor B current)
```

---

## Reference

- `l298_driver.h` — Public API & types
- `l298_driver.c` — Core logic
- `l298_example.c` — Example usage
- `L298_ADVANCED_C_GUIDE.md` — C techniques chi tiết (3-layer model)
- `L298_DATASHEET_TO_CODE.md` — Quy trình tư duy: datasheet → code
