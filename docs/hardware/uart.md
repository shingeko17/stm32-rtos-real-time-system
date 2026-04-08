# UART Baud Rate Bug & Fix

## Problem
- UART output méo + nhanh gấp 4.5 lần
- Serial terminal hiển thị ký tự lạ thay vì text
- Nguyên nhân: PCLK1 frequency sai trong code

## Root Cause
```c
// ❌ SAI (uart2_driver.c line 57)
#define PCLK1_FREQ  36000000  // Tính nhầm là 36MHz
```

**Sự thật:**
- STM32F103 xtal = 8MHz
- Không có PLL → PCLK1 = 8MHz (không phải 36MHz)
- Baud rate calculation sai → Output tốc độ sai 4.5 lần

## Solution
```c
// ✅ ĐÚNG
#define PCLK1_FREQ  8000000  // 8MHz APB1 clock (no PLL)
```

## Công Thức Baud Rate
```
BRR = PCLK1 / (16 × Baudrate)

Với PCLK1 = 8MHz, Baudrate = 9600:
BRR = 8,000,000 / (16 × 9600) = 52 ✅

Với PCLK1 = 36MHz (sai):
BRR = 36,000,000 / (16 × 9600) = 234 ❌ (→ output nhanh 4.5x)
```

## Cách Phát Hiện
1. Output bị méo/nhanh → nghi baud rate
2. Kiểm tra terminal: thử baud rate khác
3. Nếu fix ở terminal cũng không được → lỗi trong code
4. Trace lại clock configuration & BRR calculation

## Phòng Tránh
✅ Đọc datasheet STM32F103 (xác nhận clock tree)
✅ Tính công thức baud rate trước khi code
✅ Test UART output sớm trên terminal
✅ Giữ comment rõ ràng: "8MHz xtal, no PLL"

## File Sửa
- `src/f1/uart2_driver.c` line 57: PCLK1_FREQ = 8MHz

## Kết Quả
- ✅ Build success
- ✅ UART output @ 9600 baud sạch
- ✅ LEDs running light + serial messages đồng bộ

---

# UART Non-blocking & Event-driven Implementation

## Problem: Blocking UART at High Baudrate

### Triệu chứng (115200 baud)
```
"Hello World" → nhận PC: "H_llo Wor_d" (chỗ có chỗ không)
```

### Root Cause (UART_SendByte blocking loops)
```c
void UART_SendByte(uint8_t byte) {
    // ⛔ BLOCKING: chờ TX register ready
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET) {
        // CPU chiếm ~46µs
    }
    
    USART_SendData(USART1, byte);
    
    // ⛔ BLOCKING: chờ TX complete
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) {
        // CPU chiếm thêm ~46µs
    }
}
```

### Why It Fails at 115200
```
Timeline khi sending "ABC" (3 bytes):

t=0ms     CPU vào UART_SendByte('A')
          ├─ Block ~46µs
          └─ Data A sent
          
t=0.1ms   Data từ F4 UART RX arrives
          └─ RXNE interrupt kích hoạt
             ❌ CPU bị block ở SendByte('B')
             ❌ ISR miss! Data drop ❌
             
t=0.15ms  CPU xong SendByte('B')
          ├─ ISR handler trigger tập chậm
          └─ Data mất rồi!
```

**Tại sao?** Ở 115200 baud, data đến nhanh (mỗi ~87µs/byte). Nếu CPU block, ISR miss.

---

## Solution: Non-blocking + Event-driven + Ring Buffer

### Architecture
```
Application (non-blocking):
  UART_SendChar_NonBlocking('H')  ← Return ngay (<1µs)
  UART_SendChar_NonBlocking('e')  ← Queue data
  
        ↓ Ring Buffers ↓
        
RX Ring Buffer (capture all data):
  [data][data][data]...
  ↑ ISR writes here (non-blocking)
  
TX Queue (send when ready):
  [H][e][l][l][o]...
  ↑ App writes, ISR reads
  
        ↓ ↓
        
ISR Handlers (event-driven):
  ├─ RX_ISR: Capture byte to buffer → return
  └─ TX_ISR: Send next byte from queue → return
```

### Key Features
1. **Non-blocking**: API returns immediately (<1µs)
2. **Ring Buffer**: Never miss RX data
3. **ISR-driven**: Timing handled by hardware interrupts
4. **Queue-based TX**: App queues data, ISR sends when ready

### Implementation Files
- `drivers/api/uart_driver_eventdriven.c` - Implementation
- `drivers/api/uart_driver_eventdriven.h` - Public API
- `drivers/api/uart_debug_test_code.c` - Test functions

---

## Blocking vs Non-blocking Comparison

### Blocking (Old - DON'T USE at high baudrate)
```c
UART_SendByte(0x48);   // Block ~46µs
UART_SendByte(0x65);   // Block ~46µs
UART_SendByte(0x6C);   // Block ~46µs

Total block: 138µs → RX ISR miss data ❌
```

### Non-blocking (New - CORRECT)
```c
UART_SendChar_NonBlocking(0x48);  // Return <1µs + queue
UART_SendChar_NonBlocking(0x65);  // Return <1µs + queue
UART_SendChar_NonBlocking(0x6C);  // Return <1µs + queue

ISR handles TX later ✅
CPU available for RX ISR ✅
```

### Performance Table
| Metric | Blocking | Non-blocking |
|--------|----------|--------------|
| UART_SendByte() | ~46µs/byte | <1µs/byte |
| Total block time (3 bytes) | 138µs | 0µs |
| RX ISR response | ⛔ Slow (miss data) | ✅ Fast |
| CPU available | ❌ No | ✅ Yes |
| Data loss @ 115200 | ⚠️ Likely | ✅ None |
| RTOS task delay | ⚠️ Yes | ✅ No |

---

## Non-blocking API

### Send Functions
```c
// Queue character (non-blocking)
uint8_t UART_SendChar_NonBlocking(char c);
// Returns: 1 if queued, 0 if queue full

// Queue string (non-blocking)
uint16_t UART_SendString_NonBlocking(const char *str);
// Returns: number of characters queued

// Blocking send (emergency only!)
void UART_SendChar_Blocking(char c);
// ⚠️ USE WITH CAUTION - can block
```

### Receive Functions
```c
// Get character if available (non-blocking)
uint8_t UART_RecvChar_NonBlocking(void);
// Returns: character or 0 if none

// Check if data available
uint8_t UART_DataAvailable(void);
// Returns: 1 if data, 0 if empty

// Get RX buffer level
uint16_t UART_RXCount(void);
```

### Debug Functions
```c
// Print statistics
void UART_PrintStats(void);
// Shows: RX count, TX count, drop count, buffer usage

// Reset counters
void UART_ResetStats(void);
```

---

## Event-driven ISR Handlers

### RX Interrupt Handler
```c
void UART_RX_ISR(void) {
    uint8_t byte = UART_ReceiveByte_Direct();
    RX_Buffer_Put(byte);  // ← Non-blocking put
    uart_rx_count++;      // Statistics
}
// Execution: <1µs (just copy data to buffer)
```

### TX Interrupt Handler
```c
void UART_TX_ISR(void) {
    uint8_t byte;
    
    if (TX_Queue_Get(&byte)) {
        // Queue has data - send it
        UART_SendByte_Direct(byte);
    } else {
        // Queue empty - disable TX interrupt
        UART_DisableTXInterrupt();
    }
}
// Execution: <1µs (just send next byte or disable)
```

### Integration (in your ISR)
```c
void USART1_IRQHandler(void) {
    uint32_t sr = *(volatile uint32_t *)(USART1_BASE + 0x00);
    
    if (sr & (1 << 5)) {  // RXNE (RX data available)
        UART_RX_ISR();
    }
    
    if (sr & (1 << 6)) {  // TC (TX complete)
        UART_TX_ISR();
    }
}
```

---

## Timing Reference @ 115200 baud

### Baudrate Calculations
```
Baudrate formula:
  Time per bit = 1 / Baudrate
  
@ 115200 baud:
  Time/bit = 1 / 115,200 = 8.68 microseconds

1 byte:      10 bits × 8.68µs = 86.8µs ≈ 87µs
10 bytes:    8.7 milliseconds
100 bytes:   87 milliseconds
1 KB:        8.7 seconds
```

### BRR Calculation for STM32F4
```
Formula: BRR = PCLK1 / (16 × Baudrate)

For STM32F4 with PCLK1 = 42MHz, Baudrate = 115200:
  BRR = 42,000,000 / (16 × 115,200)
  BRR = 42,000,000 / 1,843,200
  BRR = 22.78 ≈ 23

Verify actual baudrate:
  Actual = 42,000,000 / (16 × 23) = 114,130 baud
  Error = (115,200 - 114,130) / 115,200 × 100% = 0.93%
  ✅ ACCEPTABLE (<3% error tolerance)
```

### Expected on Logic Analyzer
```
Bit timing should be: 8.68µs ± 2% (±0.17µs)

If you measure:
  ✅ 8.68µs per bit   → Baudrate correct
  ❌ 4.34µs per bit   → 2x too fast (BRR error)
  ❌ 17.36µs per bit  → 2x too slow (BRR error)
  ❌ Irregular timing → Clock instability
```

---

## Debug with Logic Analyzer

### Quick Diagnosis Flowchart
```
Start: UART data "chỗ có chỗ không"

├─ Is baudrate correct?
│  ├─ NO (bits too fast/slow)
│  │  └─ Fix: Recalculate BRR value
│  │
│  └─ YES (8.68µs per bit)
│     └─ Are TX bytes all sent?
│        ├─ NO → Check app code
│        │
│        └─ YES (TX clean)
│           └─ Are RX bytes all received?
│              ├─ NO (RX gaps while TX OK)
│              │  └─ FIX: Use non-blocking UART ✅
│              │  └─ CAUSE: Blocking delays miss ISR
│              │
│              └─ YES → Problem elsewhere
```

### Test Procedures
```c
// Test 1: Verify baudrate
void test_single_byte_timing(void) {
    while (1) {
        UART_SendByte(0x41);  // Send 'A'
        delay_ms(200);        // Repeat every 200ms
    }
}
// Expected: Each byte = ~87µs on logic analyzer

// Test 2: Check for ISR miss
void test_rapid_transmission(void) {
    while (1) {
        UART_SendString("ABCDEFGH");
        delay_ms(100);
    }
}
// Expected: TX = RX (no gaps)
// If RX has gaps → ISR missing data (blocking confirmed!)

// Test 3: Loopback test (PA9→PA10)
void test_loopback_rx(void) {
    while (1) {
        UART_SendString("TEST123\r\n");
        delay_ms(200);
    }
}
// Expected: TX and RX lines identical
```

See `uart_debug_test_code.c` for 7 ready-to-use tests.

---

## When to Use What

### Use Blocking UART_SendByte()
```
❌ NEVER at high baudrate (>57600)
❌ NEVER in RTOS tasks
❌ NEVER when receiving data simultaneously

✅ ONLY if:
   - Low baudrate (9600-19200)
   - Single-threaded code
   - Not timing-critical
```

### Use Non-blocking UART_SendChar_NonBlocking()
```
✅ ALWAYS at high baudrate (115200+)
✅ ALWAYS in RTOS (multiple tasks)
✅ ALWAYS with simultaneous TX/RX
✅ ALWAYS for responsive systems
```

---

## FSM Context (Optional Application Layer)

While UART driver doesn't need FSM, the UART application layer might:

```c
// Application FSM above driver layer
typedef enum {
    APP_STATE_IDLE,
    APP_STATE_WAITING_COMMAND,
    APP_STATE_PROCESSING,
    APP_STATE_SENDING_RESPONSE
} app_fsm_state_t;

// Use non-blocking UART within FSM:
void app_fsm_execute(void) {
    switch (app_state) {
        case APP_STATE_WAITING_COMMAND:
            if (UART_DataAvailable()) {
                char cmd = UART_RecvChar_NonBlocking();
                app_state = APP_STATE_PROCESSING;
            }
            break;
        case APP_STATE_SENDING_RESPONSE:
            UART_SendString_NonBlocking("OK\r\n");
            // ← Non-blocking, returns immediately
            break;
    }
}
```

---

## Best Practices

### ✅ DO
- Use non-blocking API for SYS/UART operations
- Enable RX interrupt before sending
- Check buffer levels with UART_RXCount()
- Test at maximum baudrate
- Use logic analyzer for verification
- Handle queue full gracefully
- Reset statistics periodically for monitoring

### ❌ DON'T
- Use blocking while loops for UART timing
- Disable interrupts over UART operations
- Ignore overflow/drop statistics
- Assume "it works" without testing
- Mix blocking and non-blocking calls
- Send >256 bytes without checking queue

---

## References
- `uart_driver_eventdriven.c` - Non-blocking implementation
- `LOGIC_ANALYZER_DEBUG_GUIDE.md` - Debugging with hardware
- `LOGIC_ANALYZER_CALCULATIONS.md` - Timing calculations
- `uart_debug_test_code.c` - 7 test functions
- `UART_DEBUG_GUIDE.md` - Comprehensive troubleshooting
