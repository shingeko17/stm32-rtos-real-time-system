/**
 * @file LOGIC_ANALYZER_DEBUG_GUIDE.md
 * @brief How to Debug UART with Logic Analyzer
 * @description
 *   Step-by-step guide to diagnose UART issues:
 *   - Baudrate problem detection
 *   - Blocking/ISR miss detection
 *   - Data integrity verification
 */

# Logic Analyzer Debug Guide - UART Troubleshooting

## 📋 Table of Contents
1. [What is Logic Analyzer?](#1-what-is-logic-analyzer)
2. [Hardware Setup](#2-hardware-setup)
3. [Software Setup (Pulseview)](#3-software-setup)
4. [Test Scenarios](#4-test-scenarios)
5. [Reading Waveforms](#5-reading-waveforms)
6. [Diagnosis Guide](#6-diagnosis-guide)

---

## 1. What is Logic Analyzer?

### **Definition:**
Logic analyzer là thiết bị capture các tín hiệu digital (0 và 1) trên các wire theo thời gian thực.

### **For UART Debug:**
- TX wire: Thấy byte nào được gửi
- RX wire: Thấy byte nào được nhận
- Timing: Detect baudrate problems
- Data loss: Detect missing bytes

### **Ưu điểm:**
```
✅ Thấy CHÍNH XÁC cái gì truyền trên wire
✅ Không bị ảnh hưởng bởi SW bugs
✅ Timeline rõ ràng (microsecond precision)
✅ Phân biệt baudrate vs blocking
```

---

## 2. Hardware Setup

### **Thiết bị cần:**

| Item | Cost | Ghi chú |
|------|------|--------|
| Logic Analyzer | $15-30 | Saleae clone (8ch), hoặc Hantek, DSLogic |
| USB Cable | $5 | For connecting to PC |
| Jumper Wires | $2 | To probe UART TX/RX |
| Reference Ground | free | Probe GND to STM32 GND |

### **Kết nối:**

```
STM32F4 UART1:
  PA9  (TX) ────────┬──→ Logic Analyzer Channel 0
                    │
  PA10 (RX) ────────┬──→ Logic Analyzer Channel 1
                    │
  GND ──────────────┴──→ Logic Analyzer GND

(LUÔN probe GND để set reference!)
```

### **Hình vẽ kết nối:**

```
┌─────────────┐
│   STM32F4   │
│  PA9 (TX)   │────[Jumper]────→ CH0
│  PA10(RX)   │────[Jumper]────→ CH1
│  GND        │────[Jumper]────→ GND
└─────────────┘

                    ┌─────────────────┐
                    │ Logic Analyzer  │
                    │   [USB]         │
                    │  CH0, CH1, GND  │
                    └────────┬────────┘
                             │
                    ┌────────▼────────┐
                    │   PC/Laptop     │
                    │  Pulseview SW   │
                    └─────────────────┘
```

---

## 3. Software Setup (Pulseview)

### **Step 1: Install Pulseview**

```bash
# Windows
Download: https://sigrok.org/wiki/PulseView
# hoặc
winget install pulseview

# Linux (Ubuntu)
sudo apt-get install pulseview

# macOS
brew install pulseview
```

### **Step 2: Connect Logic Analyzer**

```
1. Connect Logic Analyzer to PC via USB
2. Open Pulseview
3. Click: Capture → Device → Select "Saleae Clone" (or your device)
4. Verify channels detected (usually 8ch auto-detect)
```

### **Step 3: Configure Protocol Decoder**

```
1. Go to: Decoders → Add
2. Select: UART (not RS232, just UART!)
3. Configure:
   - TX pin: Channel 0 (PA9)
   - RX pin: Channel 1 (PA10)
   - Baudrate: 115200 (match your config)
   - Data bits: 8
   - Stop bits: 1
   - Parity: None
```

### **Step 4: Set Capture Parameters**

```
1. Sample rate: ≥ 1MHz (higher = better resolution)
   Typical: 4MHz or 8MHz for 115200 baud
   
2. Record length: 1M samples (enough for ~100 bytes @ 1MHz)

3. Trigger: Optional
   - Can trigger on UART activity
   - Or just free-run capture
```

---

## 4. Test Scenarios

### **Scenario 1: Check Baudrate (TX side)**

**Code to run on STM32:**

```c
void uart_baudrate_test(void) {
    // Send 'A' (0x41) repeatedly
    while (1) {
        UART_SendByte(0x41);  // 'A'
        delay_ms(200);        // 200ms between sends
    }
}
```

**Steps:**
```
1. Start Pulseview capture
2. Run code above
3. Capture 1-2 bytes (should show clean waveform)
4. Analyze timing
```

### **Scenario 2: Check Data Integrity (TX side)**

**Code to run on STM32:**

```c
void uart_data_test(void) {
    // Send sequence: 'H', 'e', 'l', 'l', 'o'
    uint8_t data[] = {0x48, 0x65, 0x6C, 0x6C, 0x6F};
    
    UART_SendString("Hello\r\n");
    delay_ms(500);
    
    UART_SendString("World\r\n");
    delay_ms(500);
}
```

**Steps:**
```
1. Start capture
2. Run code (send "Hello\r\nWorld\r\n")
3. Capture both strings (should show clean bytes)
4. Check if any bytes are missing
```

### **Scenario 3: Check RX (Receive) - Loopback**

**Hardware setup:**
```
STM32 PA9  (TX) ────┐
                    ├──→ Connect together to loop TX back to RX
STM32 PA10 (RX) ────┘

(Self-loopback test)
```

**Code:**

```c
void uart_loopback_test(void) {
    UART_SendString("TEST123\r\n");
}
```

**What to see:**
- TX line: Shows "TEST123\r\n"
- RX line: Should see same (because looped back)
- If RX missing some bytes → Blocking issue confirmed!

---

## 5. Reading Waveforms

### **UART Bit Structure (115200, 8N1):**

```
1 byte = 1 START + 8 DATA + 1 STOP = 10 bits
Time per bit @ 115200 = 1/115200 ≈ 8.68µs

Example: Char 'A' (0x41 = 0100 0001)

    ┌─────────────────────────────────────────┐
    │ USB UART Frame (87µs total)             │
    └─────────────────────────────────────────┘
    
    S D0 D1 D2 D3 D4 D5 D6 D7 STOP
    └─┴──┴──┴──┴──┴──┴──┴──┴──┴───┘
    8.68µs per bit

Byte 'A' (0x41 binary: 0100 0001):
    START (0)
    │
    D0=1, D1=0, D2=0, D3=0
    D4=0, D5=1, D6=0, D7=0
    STOP (1)
```

### **Waveform Examples:**

#### **Good Signal (UART working fine):**
```
TX line:
    ┌─────┐       ┌─────┐       ┌─────┐
    │     │       │     │       │     │
 ──┘     └───────┘     └───────┘     └──
    
 Data1      Gap        Data2      Gap   Data3
(Clean, regular spacing, no glitches)
```

#### **Baudrate Too Fast (STM32 sending faster than PC expects):**
```
TX line:
    ┌──┐    ┌──┐    ┌──┐    ┌──┐
    │  │    │  │    │  │    │  │
 ──┘  └────┘  └────┘  └────┘  └──
    
 Too narrow pulses! (< 8.68µs)
 STM32 is sending too fast!
```

#### **Baudrate Too Slow (STM32 sending slower than PC expects):**
```
TX line:
    ┌────────────┐       ┌────────────┐
    │            │       │            │
 ──┘            └───────┘            └──
    
 Too wide pulses! (> 8.68µs)
 STM32 is sending too slow!
```

#### **Data Loss / ISR Miss (blocking issue):**
```
TX line (From STM32):
    ┌─────┐       ┌─────┐       ┌─────┐
 ──┘     └───────┘     └───────┘     └──
    Data1      Gap        Data2
    
RX line (What PC sees):
    ┌─────┐       ┌─────┐       
 ──┘     └───────┘     └────────────────
    Data1      Gap        (Missing Data2!)
    
→ Data2 lost because RX ISR missed it
```

---

## 6. Diagnosis Guide

### **Test Case 1: Does STM32 SEND the data?**

**What to check:**
- Look at TX line in Pulseview
- See if all bytes are transmitted

**Results:**

```
✅ All bytes on TX
   └─ Good: STM32 is sending correctly

❌ Some bytes missing on TX
   └─ Problem: App code issue (not sending)
```

### **Test Case 2: Are bit timings correct? (Baudrate check)**

**What to measure:**
- Distance between edges (should be ~8.68µs @ 115200)
- Use Pulseview ruler tool

**Steps:**
```
1. Capture a byte (e.g., 'A')
2. In Pulseview: Click ruler tool
3. Measure from START bit edge to next edge
4. Should be 8.68µs ± 2% (±0.17µs)
```

**Results:**

```
+--------+--------+--------+--------+---------+
| 8.68µs | 8.68µs | 8.68µs | 8.68µs | ...     |
+--------+--------+--------+--------+---------+
✅ CORRECT BAUDRATE

+------+------+------+------+
| 4µs  | 4µs  | 4µs  | 4µs |
+------+------+------+------+
❌ TOO FAST (probably 230400 baud instead)

+----------+----------+----------+
| 12.5µs   | 12.5µs   | 12.5µs   |
+----------+----------+----------+
❌ TOO SLOW (probably 76800 baud instead)
```

### **Test Case 3: PC receiving data correctly?**

**What to check:**
- Compare TX line vs RX line
- Should be identical (normally)

**Results:**

```
TX: ┌─────┐       ┌─────┐       ┌─────┐
 ──┘     └───────┘     └───────┘     └──
     Data1      Gap        Data2

RX: ┌─────┐       ┌─────┐       ┌─────┐
 ──┘     └───────┘     └───────┘     └──
     Data1      Gap        Data2

✅ TX == RX
   → Communication working fine

RX: ┌─────┐       ┌─────┐       
 ──┘     └───────┘     └────────────────
     Data1      Gap        (Missing!)

❌ TX ≠ RX (data loss)
   → Blocking ISR miss or other RX issue
```

### **Test Case 4: Is it blocking causing data loss?**

**Test procedure:**

```
1. Run UART test with BLOCKING send
   while (1) {
       UART_SendString("ABCDEFGH");
       delay_ms(10);
   }

2. Capture: F4 sending to F1 loopback

3. Observe TX line:
   ✅ Clean A,B,C,D,E,F,G,H bytes
   └─ TX side OK
   
4. If RX line shows gaps:
   ❌ "A_C_E_G" (missing B,D,F,H)
   └─ RX ISR miss confirmed!
   └─ Cause: Blocking delays
   └─ Fix: Use non-blocking + queue

5. If TX line itself shows gaps:
   ❌ "A_C_E_G" on TX also
   └─ TX app not even sending
   └─ Different issue
```

---

## 📊 Quick Diagnosis Flowchart:

```
Start: UART sending garbled/missing data

Is baudrate correct?
  ├─ NO (too fast/slow bits)
  │  └─ Fix: Calculate correct BRR value
  │     BRR = PCLK1 / (16 * 115200)
  │
  └─ YES (8.68µs per bit)
     └─ Are TX bytes all sent?
        ├─ NO (TX line gaps)
        │  └─ Fix: Check app code
        │
        └─ YES (TX clean)
           └─ Are RX bytes all received?
              ├─ NO (RX line gaps when TX clean)
              │  └─ Fix: Non-blocking UART + ISR
              │  └─ Cause: Blocking delays miss ISR
              │
              └─ YES
                 └─ No issue found!
```

---

## 🎯 For Your Specific Issue (115200 COM5):

### **Exact Steps:**

**Step 1: Hardware**
```
Connect:
  STM32F4 PA9  (TX) → Logic Analyzer CH0
  STM32F4 PA10 (RX) → Logic Analyzer CH1
  STM32F4 GND       → Logic Analyzer GND
```

**Step 2: Code**
```c
// Run this on STM32F4:
void debug_uart(void) {
    UART_Init();
    UART_Init_EventDriven();  // or old version
    
    uint32_t count = 0;
    while (1) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Test:%lu\r\n", count++);
        UART_SendString(buf);  // Old blocking version
        delay_ms(100);
    }
}
```

**Step 3: Capture**
```
1. Open Pulseview
2. Set Baudrate: 115200
3. Start capture
4. Let it run 2-3 seconds
5. Stop capture
```

**Step 4: Analyze**
```
1. Zoom in on first byte
2. Measure bit timing (should be 8.68µs ± 2%)
3. If correct → Baudrate OK
4. If wrong → Baudrate issue (fix BRR calculation)

5. Check for gaps between bytes
6. If TX has gaps when you're sending → Blocking confirmed
7. If RX has gaps while TX OK → ISR miss confirmed
```

**Step 5: Interpret Results**

```
Result A: All bits 8.68µs, clean transmission
  ✅ Baudrate correct, no blocking issue
  ❌ Problem is elsewhere (PC-side, cable, driver)

Result B: All bits 4.34µs (2x too fast)
  ❌ Baudrate divided by 2 error
  → Fix: BRR calculation or clock config

Result C: Bits correct but gaps on RX while TX clean
  ❌ ISR missing data (blocking issue confirmed)
  → Fix: Use non-blocking driver
```

---

## 💡 Quick Tips:

1. **Zoom tool**: Press 'Z' on keyboard to zoom
2. **Measure**: Pulseview has ruler/measurement tools
3. **Export**: Can export data as CSV for analysis
4. **Decode**: Protocol analyzer auto-decodes UART bytes
5. **Save**: Save captures for reference

---

## ✅ Summary:

Logic analyzer shows **exactly** what's on the wire, making it easy to:
- ✅ Confirm baudrate (measure bit time)
- ✅ Confirm data present (see bytes on wire)
- ✅ Confirm ISR issues (see gaps on RX vs clean TX)

**For your "chỗ có chỗ không" issue:**
→ Use logic analyzer to prove it's **blocking**, not baudrate! 🎯
