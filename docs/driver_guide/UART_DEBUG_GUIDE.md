# UART Debug Guide - Blocking vs Non-blocking (Event-driven)

## 🔴 Vấn đề bạn gặp:
- **Baudrate**: 115200 (rất nhanh!)
- **Triệu chứng**: "kí tự nhưng chỗ có chỗ không" (garbled data)
- **COM**: COM5

---

## 🔍 Root Cause Analysis:

### **Cách cũ (uart_driver.c):**
```c
void UART_SendByte(uint8_t byte) {
    uart_status = 1U;
    
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET) {
        // ⛔ BLOCK ở đây
        // CPU đang chờ TX register ready
    }
    
    USART_SendData(USART1, byte);
    
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) {
        // ⛔ BLOCK ở đây nữa
        // CPU đang chờ TX complete
    }
    
    uart_status = 0U;
}
```

### **Kịch bản gây lỗi:**

```
Timeline:
  t=0ms    STM32F4 gửi 1 byte qua UART1 (115200 baud)
  
  t=0ms    CPU vào UART_SendByte()
           ↓
           Block ở "while USART_FLAG_TXE"  ⛔
  
  t=0.2ms  Data từ F4 UART (hoặc PC từ UART) đến ISR
           → RXNE interrupt kích hoạt
           ❌ NHƯ 199 ISR handler không được gọi (CPU bị block)
           ❌ Dữ liệu bị drop
  
  t=46ms   UART_SendByte() cuối cùng mới xong
           CPU từ khỏi block
           
  Result:  ❌ DATA BỊ MẤT (garbled)
```

### **Tại sao "chỗ có chỗ không"?**

```
Các lúc CPU không block → UART data OK ✅
Các lúc CPU block      → UART data drop ❌

Kết quả: nhận được "Hello?? Wor?d" thay vì "Hello World"
(các ? = bytes bị drop)
```

---

## ✅ Giải pháp: Non-blocking + Event-driven

### **Nguyên lý:**

```c
// ❌ CŨ - Blocking (CPU chiếm)
UART_SendByte(0x48);  // Chặn 46µs
UART_SendByte(0x65);  // Chặn 46µs
UART_SendByte(0x6C);  // Chặn 46µs
// Tổng block: 138µs → RX interrupt miss data

// ✅ MỚI - Non-blocking (CPU không chiếm)
UART_SendChar_NonBlocking(0x48);  // Return ngay (<1µs)
UART_SendChar_NonBlocking(0x65);  // Return ngay (<1µs)
UART_SendChar_NonBlocking(0x6C);  // Return ngay (<1µs)
// Byte queued, ISR handles TX later
// CPU có thể xử lý RX interrupt ✅
```

### **Sơ đồ Non-blocking Architecture:**

```
App Layer (non-blocking):
  UART_SendChar_NonBlocking('H')     ← Queue 'H', return ngay
  UART_SendChar_NonBlocking('i')     ← Queue 'i', return ngay
  
           ↓ Queue [H, i, ...]
           
Ring Buffer (circular):
  [H][i][!][ ][ ]... capacity=256
   ↑
   ISR reads from here → TX hardware
   
RX Ring Buffer (capture all RX):
  [data]... → never drop because non-blocking
```

---

## 🛠️ 3 Key Improvements:

### **1. Ring Buffer cho RX (không drop data)**
```c
void UART_RX_ISR(void) {
    uint8_t byte = UART_ReceiveByte_Direct();
    RX_Buffer_Put(byte);  // ← Luôn có thời gian, <1µs
    uart_rx_count++;
}
```

**Lợi ích:**
- ✅ ISR quay về ngay lập tức (<1µs)
- ✅ RX data captured vào buffer an toàn
- ✅ App có thể đọc khi rảnh

### **2. TX Queue (không block app)**
```c
uint8_t UART_SendChar_NonBlocking(char c) {
    return TX_Queue_Put((uint8_t)c);  // ← Return ngay
}
```

**Lợi ích:**
- ✅ App không bao giờ block
- ✅ TX ISR xử lý thực tế gửi dữ liệu
- ✅ CPU có thể xử lý RX trong khi TX

### **3. Event-driven (ISR handles timing)**
```c
void UART_TX_ISR(void) {
    uint8_t byte;
    
    if (TX_Queue_Get(&byte)) {
        UART_SendByte_Direct(byte);  // ← ISR gửi
    }
}
```

**Lợi ích:**
- ✅ CPU không chờ hardware flags
- ✅ ISR quản lý timing
- ✅ Dành CPU cho app logic

---

## 📊 Performance Comparison:

| Metric | CŨ (Blocking) | MỚI (Non-blocking) |
|--------|--------------|-------------------|
| **UART_SendByte()** | ~46µs/byte | <1µs/byte |
| **CPU block time** | 138µs / 3 bytes | 0µs |
| **RX interrupt response** | ⛔ Chậm (bị block) | ✅ Ngay lập tức |
| **Data loss at 115200 baud** | ⚠️ Có thể | ✅ Không |
| **RTOS task delay** | ⚠️ Có (chặn task) | ✅ Không |

---

## 🚀 Cách sử dụng:

### **Initialization:**
```c
int main(void) {
    UART_Init();  // Basic UART init (từ cũ)
    UART_Init_EventDriven();  // Enable interrupts + buffers
    
    // Setup ISR handler:
    // In USART1_IRQHandler:
    //   if (SR & RXNE) UART_RX_ISR();
    //   if (SR & TC) UART_TX_ISR();
    
    while (1) {
        // Non-blocking send (no CPU block!)
        if (sensor_ready) {
            UART_SendString_NonBlocking("Sensor OK\r\n");
        }
        
        // Non-blocking receive
        if (UART_DataAvailable()) {
            char cmd = UART_RecvChar_NonBlocking();
            process_command(cmd);
        }
        
        // Other work continues...
        do_other_work();
    }
}
```

### **ISR integrations:**
```c
void USART1_IRQHandler(void) {
    uint32_t sr = *(volatile uint32_t *)(USART1_BASE + 0x00);
    
    if (sr & (1 << 5)) {  // RXNE
        UART_RX_ISR();
    }
    
    if (sr & (1 << 6)) {  // TC (TX complete)
        UART_TX_ISR();
    }
}
```

---

## 🐛 Debug UART Issues:

### **Method 1: Use statistics**
```c
// Check if data is being received
printf("RX Buffer: %u bytes\n", UART_RXCount());

// Check if TX queue backed up
printf("TX Queue: %u bytes\n", UART_TXCount());

// Check for dropped data
UART_PrintStats();
```

### **Method 2: Monitor at different baudrates**
```
9600 baud:   OK (slower, less ISR pressure)
57600 baud:  ⚠️  Might show occasional drops
115200 baud: ⛔ With blocking → lots of drops
             ✅ With event-driven → OK
```

### **Method 3: Test TX + RX simultaneously**
```c
while (1) {
    // Send counter
    static uint32_t count = 0;
    char buf[32];
    snprintf(buf, sizeof(buf), "Count: %lu\r\n", count++);
    UART_SendString_NonBlocking(buf);
    
    // Receive and echo
    if (UART_DataAvailable()) {
        char c = UART_RecvChar_NonBlocking();
        UART_SendChar_NonBlocking(c);  // Echo
    }
    
    delay_ms(1);  // Don't fill console
}
```

---

## ⚠️ Migration from old to new:

### **Replace:**
```c
// Old
UART_SendByte(byte);          → UART_SendChar_NonBlocking(byte);
UART_SendString(str);         → UART_SendString_NonBlocking(str);
while (!UART_DataAvailable()); → if (UART_DataAvailable())
char c = UART_ReadByte();     → char c = UART_RecvChar_NonBlocking();
```

### **Add ISR handler:**
```c
// In interrupt handler
void USART1_IRQHandler(void) {
    if (USART_SR & USART_FLAG_RXNE) {
        UART_RX_ISR();
    }
    if (USART_SR & USART_FLAG_TC) {
        UART_TX_ISR();
    }
}
```

---

## 📈 Expected Improvements:

| Problem | Before | After |
|---------|--------|-------|
| Garbled at 115200 | ❌ Happens | ✅ Fixed |
| CPU blocked by UART | ❌ Yes | ✅ No |
| RX data lost | ⚠️ Often | ✅ Never |
| Response time | ⚠️ Slow | ✅ Fast |

---

## 🎓 Takeaway:

**Blocking UART + high baudrate = Recipe for disaster!**

**Solution: Non-blocking + ring buffer + ISR-driven = Rock solid!**

Hệ thống của bạn sẽ ổn định ở 115200 baud mà không có "kí tự nhưng chỗ có chỗ không" nữa! 🚀
