# L298 Driver — Kỹ Thuật C Nâng Cao (Theo Embedded Architecture Model)

> Tài liệu này tổ chức các kỹ thuật C theo **3-layer architecture**
> thay vì liệt kê phẳng. Mỗi concept thuộc đúng 1 layer trong pipeline:
> **Event → State → Action → Hardware**.

---

## Architecture Overview

```
┌─────────────────────────────────────────────────┐
│  LAYER 1: LOGIC (State Machine + Event)         │
│  enum, volatile, error codes                    │
│  → Quyết định hành vi hệ thống                 │
├─────────────────────────────────────────────────┤
│  LAYER 2: DRIVER (Execution / Abstraction)      │
│  typedef struct, function pointer, const,       │
│  inline, pointer deref, bit macros, union,      │
│  memcpy/memset                                  │
│  → Interface giữa logic và hardware             │
├─────────────────────────────────────────────────┤
│  LAYER 3: MEMORY (Storage Layout)               │
│  static/global, stack, heap                     │
│  → Nơi lưu trữ dữ liệu                        │
└─────────────────────────────────────────────────┘

Pipeline: [ISR/DMA] → volatile flag → [State Machine] → [Driver API] → [Hardware Register]
```

---

## Mục Lục (grouped by layer)

**Layer 1 — Logic:**
1. [enum Types](#1-enum-types)
2. [volatile Keyword](#2-volatile-keyword)
3. [Error Code Returns](#3-error-code-returns)

**Layer 2 — Driver:**
4. [typedef struct](#4-typedef-struct)
5. [Pointer Dereferencing](#5-pointer-dereferencing)
6. [Function Pointers](#6-function-pointers)
7. [const Correctness](#7-const-correctness)
8. [inline Functions](#8-inline-functions)
9. [Bit Manipulation Macros](#9-bit-manipulation-macros)
10. [union & Bit Fields](#10-union--bit-fields)
11. [memset & memcpy](#11-memset--memcpy)

**Layer 3 — Memory:**
12. [static Keyword & Memory Layout](#12-static-keyword--memory-layout)

---

# LAYER 1 — LOGIC (State Machine + Event)

> Concept ở layer này quyết định **hành vi** hệ thống:
> hệ thống đang ở trạng thái nào, event nào xảy ra, lỗi gì cần xử lý.

## 1. enum Types

`enum` = type-safe named constants, thay magic numbers.

```c
/* ✗ Magic numbers — compiler không bắt lỗi */
int motor_state = 5;  /* 5 là gì? Invalid nhưng compiler OK */

/* ✓ Enum — compiler warning nếu invalid */
typedef enum {
    MOTOR_FREE_RUN = 0,
    MOTOR_FORWARD  = 1,
    MOTOR_REVERSE  = 2,
    MOTOR_BRAKE    = 3
} L298_MotorState;

L298_MotorState state = MOTOR_FORWARD;  /* Rõ ràng, debugger hiển thị tên */
```

**Vai trò trong pipeline:** State Machine dùng enum để biểu diễn trạng thái hiện tại (IDLE → RUNNING → ERROR).

---

## 2. volatile Keyword

`volatile` = "giá trị này thay đổi bên ngoài control flow bình thường, compiler không được cache."

**Chỉ dùng tại boundary:** ISR ↔ main, DMA ↔ CPU, hardware register.

```c
/* ✗ Không volatile — compiler cache giá trị cũ, loop vô hạn */
uint32_t count = 0;
while (count == 0) { }  /* ISR set count=1 nhưng compiler không đọc lại */

/* ✓ Volatile — compiler buộc đọc từ memory mỗi lần */
volatile uint32_t count = 0;
while (count == 0) { }  /* ISR set count=1 → loop thoát */
```

**Pattern trong driver:**
```c
typedef struct {
    volatile uint32_t current_value;   /* ISR/DMA cập nhật */
    uint32_t configuration;            /* Chỉ main code đọc/ghi → không cần volatile */
} HardwareMonitor;
```

---

## 3. Error Code Returns

C không có exceptions → dùng return code để báo lỗi cho caller.

```c
typedef enum {
    L298_OK         = 0,
    L298_ERROR      = 1,
    L298_NOT_INIT   = 2,
    L298_INVALID_CH = 3,
} L298_Status;

L298_Status L298_SetSpeed(L298_Driver *driver, L298_ChannelID channel, int16_t speed)
{
    if (driver == NULL)        return L298_ERROR;
    if (!driver->initialized)  return L298_NOT_INIT;
    if (channel > L298_CHANNEL_B) return L298_INVALID_CH;
    /* ... */
    return L298_OK;
}
```

**Vai trò trong pipeline:** Error code điều khiển flow — nếu lỗi thì State Machine chuyển sang ERROR state, không gọi tiếp driver.

---

# LAYER 2 — DRIVER (Execution / Abstraction)

> Concept ở layer này là **interface giữa logic và hardware**.
> Logic không truy cập register trực tiếp — phải qua driver.
> Driver không chứa logic điều khiển — chỉ execute.

## 4. typedef struct

### Tạo kiểu dữ liệu, nhóm data liên quan.

```c
/* typedef tạo alias → khai báo ngắn gọn */
typedef struct {
    volatile L298_MotorState state;
    volatile uint16_t current_duty;
    volatile uint16_t current_speed;
    volatile uint32_t timestamp_ms;
} L298_ChannelState;

L298_ChannelState state_a;  /* Dùng type alias thay vì struct L298_ChannelState */
```

**Memory layout:**
```c
/* L298_ChannelState ch_a; tạo biến ch_a trên stack */
/* ch_a.current_speed truy cập field bên trong */
/* sizeof(L298_ChannelState) ≈ 12 bytes (có padding) */
```

---

## 5. Pointer Dereferencing

### Syntax

```c
int x = 42;
int *ptr = &x;          /* ptr = address của x */
int value = *ptr;       /* Dereference: value = 42 */

/* Với structs */
typedef struct { int a; int b; } Point;
Point p = {1, 2};

Point *pp = &p;
int a_value = pp->a;    /* Dereference & access field */
/* Tương đương với: int a_value = (*pp).a; */
```

### Ví Dụ L298

```c
/* Hàm nhận driver pointer */
L298_Status L298_SetSpeed(L298_Driver *driver,  /* Pointer */
                           L298_ChannelID channel,
                           int16_t speed)
{
    /* Dereference để truy cập members */
    if (!driver->initialized) {
        return L298_NOT_INIT;
    }
    
    L298_ChannelState *state = &driver->state_a;
    state->current_speed = 50;  /* Access through pointer */
}
```

### Lợi Ích

| Lợi ích | Giải thích |
|---------|-----------|
| **Flexibility** | Hàm có thể modify data caller |
| **Efficiency** | Pass address, không copy entire struct |
| **Multiple Returns** | Modify input parameters |

---

## 6. Function Pointers

### Biến chứa địa chỉ function → callback, dynamic dispatch.

```c
typedef void (*L298_EventCallback)(L298_Driver *driver, 
                                    L298_ChannelID channel, 
                                    void *user_ctx);

void on_speed_changed(L298_Driver *driver, L298_ChannelID channel, void *ctx)
{
    printf("Speed changed!\n");
}

L298_EventCallback callback = &on_speed_changed;
callback(my_driver, L298_CHANNEL_A, NULL);
```

### Real-world: Timer ISR callback

```c
typedef void (*TimerCallback)(void *ctx);

struct Timer {
    TimerCallback callback;
    void *user_context;
};

void timer_interrupt_handler(void) {
    struct Timer *timer = &g_timer;
    if (timer->callback != NULL) {
        timer->callback(timer->user_context);
    }
}
```

---

## 7. const Correctness

### `const` báo dữ liệu không được modify qua variable này.

```c
const int *ptr1;              /* Không modify *ptr1, có thể move ptr1 */
int * const ptr2;             /* Có thể modify *ptr2, không thể move ptr2 */
const int * const ptr3;       /* Immutable everything */
```

### Ví Dụ L298

```c
/* Read-only: const prevents modification */
uint16_t L298_GetSpeed(const L298_Driver *driver, L298_ChannelID channel)
{
    return driver->state_a.current_speed;
}

/* Read-write: no const → can modify */
L298_Status L298_SetSpeed(L298_Driver *driver, L298_ChannelID channel, int16_t speed)
{
    driver->state_a.current_speed = 50;
}
```

---

## 8. Inline Functions

### `inline` hướng compiler nhúng function code trực tiếp, bỏ call overhead.

```c
static inline bool l298_is_valid_channel(L298_ChannelID channel)
{
    return (channel <= L298_CHANNEL_B);
}

/* Compiler inline thành: */
if ((ch <= L298_CHANNEL_B)) { ... }
```

---

## 9. Bit Manipulation Macros

### Set, clear, toggle, read individual bits.

```c
#define BIT_SET(byte, bit_pos)   ((byte) |= (1 << (bit_pos)))
#define BIT_CLR(byte, bit_pos)   ((byte) &= ~(1 << (bit_pos)))
#define BIT_TOG(byte, bit_pos)   ((byte) ^= (1 << (bit_pos)))
#define BIT_READ(byte, bit_pos)  (((byte) >> (bit_pos)) & 1)

uint8_t reg = 0x00;
BIT_SET(reg, 2);         /* reg = 0x04 */
BIT_CLR(reg, 2);         /* reg = 0x00 */
BIT_TOG(reg, 2);         /* reg = 0x04 */
```

### Memory Diagram

```
Byte: [7 6 5 4 3 2 1 0] = 0x04

BIT_SET(byte, 2):   0x04 | 0x04 = 0x04
BIT_CLR(byte, 2):   0x04 & 0xFB = 0x00
BIT_TOG(byte, 2):   0x00 ^ 0x04 = 0x04
BIT_READ(byte, 2):  (0x04 >> 2) & 1 = 1
```

---

## 10. union & Bit Fields

### union: nhiều cách truy cập cùng vùng nhớ.

```c
typedef union {
    uint8_t raw;
    struct {
        uint8_t direction : 1;   /* bit 0 */
        uint8_t enabled   : 1;   /* bit 1 */
        uint8_t fault     : 1;   /* bit 2 */
        uint8_t reserved  : 5;   /* bits 3-7 */
    } bits;
} L298_ControlFlags;

L298_ControlFlags flags;
flags.raw = 0x00;             /* Clear all */
flags.bits.direction = 1;     /* Set direction bit */
flags.bits.enabled = 1;       /* Set enable bit */
/* flags.raw == 0x03 (bits 0 & 1 set) */
```

### Bit Fields: compact storage, 3 booleans → 1 byte.

```c
/* Memory layout: */
/* [reserved:5][fault:1][enabled:1][direction:1] = 1 byte */
/* Thay vì 3 × uint8_t = 3 bytes */
```

---

## 11. memset & memcpy

### memset: zero-init struct. memcpy: copy config block.

```c
L298_Driver driver;
memset(&driver, 0, sizeof(L298_Driver));  /* Clear entire struct */

L298_ChannelConfig default_config = { /* ... */ };
memcpy(&driver.config_a, &default_config, sizeof(L298_ChannelConfig));
```

---

# LAYER 3 — MEMORY (Storage Layout)

> Concept ở layer này quyết định **dữ liệu sống ở đâu**:
> static/global (lifetime = chương trình), stack (lifetime = function), heap (lifetime = manual).

## 12. static Keyword & Memory Layout

### static: scope limiting + persistent storage.

```c
/* 1. static local: giữ giá trị giữa các lần gọi */
void ISR_Handler(void) {
    static uint32_t call_count = 0;  /* Không reset mỗi lần gọi */
    call_count++;
}

/* 2. static global: chỉ visible trong file hiện tại */
static L298_Driver g_driver;         /* File-scope only */

/* 3. static function: private function */
static bool l298_is_valid_channel(L298_ChannelID channel) {
    return (channel <= L298_CHANNEL_B);
}
```

### Memory Map

```
┌────────────────────┐ High address
│     Stack          │ ← Local variables (auto lifetime)
│     ↓              │
│     ...            │
│     ↑              │
│     Heap           │ ← malloc/free (manual lifetime)
├────────────────────┤
│     .bss           │ ← static/global uninitialized (= 0)
│     .data          │ ← static/global initialized
│     .text          │ ← Code + const
└────────────────────┘ Low address
```

---

## Summary — Grouped by Layer

| Layer | Kỹ Thuật | Vai Trò |
|-------|----------|---------|
| **1: Logic** | enum | Type-safe states: `MOTOR_FORWARD`, `MOTOR_BRAKE` |
| | volatile | ISR/hardware flag synchronization |
| | Error Codes | Return `L298_Status` thay vì exceptions |
| **2: Driver** | typedef struct | Nhóm data: `L298_Driver`, `L298_ChannelConfig` |
| | Pointer Deref | Pass-by-reference, modify caller data |
| | Function Pointer | Callbacks: timer ISR, event handler |
| | const | Intent: read-only vs read-write access |
| | inline | Optimize small utility functions |
| | Bit Macros | `BIT_SET`, `BIT_CLR` cho register access |
| | union + Bit Fields | Compact flags, dual-access cùng vùng nhớ |
| | memset/memcpy | Init & copy struct blocks |
| **3: Memory** | static | Scope limiting, persistent storage, memory map |

---

**End of Advanced C Guide**
