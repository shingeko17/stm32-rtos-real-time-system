/**
 * @file LOGIC_ANALYZER_CALCULATIONS.md
 * @brief Baudrate calculations and expected timing values
 */

# Logic Analyzer - Baudrate Calculations & Timing

## 📐 Baudrate Timing Reference

### **Formula:**
```
Time per bit = 1 / Baudrate

Example: 115200 baud
  Time/bit = 1 / 115200 = 8.68 microseconds
```

### **Common Baudrates (Time per bit):**

| Baudrate | Time/Bit | Notes |
|----------|----------|-------|
| 9600 | 104.17µs | Very slow (legacy) |
| 19200 | 52.08µs | |
| 38400 | 26.04µs | |
| 57600 | 17.36µs | |
| 115200 | **8.68µs** | ← Your target |
| 230400 | 4.34µs | 2x faster |
| 460800 | 2.17µs | 4x faster |
| 921600 | 1.09µs | Needs good hardware |

---

## 🔧 STM32 Baudrate Configuration

### **BRR Calculation:**

```
For STM32 (not F4 APB2, use APB1 or APB2 based on UART):

BRR = PCLK_FREQ / (16 * Baudrate)

Example: STM32F103 with 8MHz, UART2 on APB1 (8MHz):
  BRR = 8,000,000 / (16 * 115200)
  BRR = 8,000,000 / 1,843,200
  BRR = 4.34 (round to 4)
  
Actual baudrate = 8,000,000 / (16 * 4) = 125,000 baud ❌ WRONG!

Let's try BRR = 5:
  Actual = 8,000,000 / (16 * 5) = 100,000 baud ❌ Still wrong

Better: Need 8MHz / (16 * 115200) = 4.34
        Round to nearest integer carefully
        
If you don't have exact match:
  Measurement = 8,000,000 / (16 * desired_BRR)
  
For 8MHz and want 115200:
  Need: BRR such that 8,000,000 / (16*BRR) ~ 115,200
  → No exact match! Need PLL or different clock.
```

### **Better Example: STM32F4 with 42MHz APB1:**

```
Baudrate = 115200 baud
PCLK1 = 42,000,000 Hz

BRR = 42,000,000 / (16 * 115,200)
BRR = 42,000,000 / 1,843,200
BRR = 22.78 (round to 23)

Verify actual baudrate:
  Actual = 42,000,000 / (16 * 23)
  Actual = 42,000,000 / 368
  Actual = 114,130 baud

Error = (115,200 - 114,130) / 115,200 × 100%
      = 0.93% error ✅ ACCEPTABLE (<3%)
```

---

## 📊 What to Expect on Logic Analyzer

### **Test: Send 'A' (0x41) repeatedly**

**Setup:**
```c
void test_send_a(void) {
    while (1) {
        UART_SendByte(0x41);   // Send 'A'
        delay_ms(200);          // 200ms delay
    }
}
```

**Expected on Logic Analyzer @ 115200:**

```
Time:       0µs      100µs     200µs
            |---------|---------|
TX line:    __________╱╲╱╲╱╲╱╲╱╲╲╱╲╱╲__________

            |← 1 byte (87µs) →|← 200ms gap →|

1 byte @ 115200 = 10 bits × 8.68µs = 86.8µs ≈ 87µs
```

### **Byte Structure for 0x41 (binary: 0100 0001):**

```
Bit#  0  1  2  3  4  5  6  7  8  9
      S  D0 D1 D2 D3 D4 D5 D6 D7 P
      0  1  0  0  0  0  1  0  0  1
      
      └─ START (always 0)
               └─ Data bits (LSB first)
                              └─ STOP (always 1)

Timing:
  0µs   ───┐
          │ START bit (0)
  8.68µs  │
         ─┘───┐
           │ D0=1
  17.36µs  │
         ─┤───┐
           │ D1=0
  26.04µs  │
         ──┘───┐
           │ D2=0
  34.73µs  │
         ──┤───┐
           │ D3=0
  43.41µs  │
         ──┤───┐
           │ D4=0
  52.09µs  │
         ──┤───┐
           │ D5=1
  60.77µs  │
         ─┘───┐
           │ D6=0
  69.45µs  │
         ──┤───┐
           │ D7=0
  78.13µs  │
         ──┤───┐
           │ STOP (1)
  86.81µs  │
         ─┘
```

---

## 🔴 Red Flags on Logic Analyzer

### **Flag 1: Bits too narrow (faster than expected)**

```
Expected (115200): 8.68µs per bit
Actual:            4.34µs per bit
                   ↓
Actual baudrate = 230,400 (2x too fast)
                   ↓
Problem: BRR set to half the value it should be
         OR clock frequency 2x higher than expected
```

**In Pulseview:**
```
Measure first bit: 4.34µs instead of 8.68µs
→ Baudrate is 2x too fast!
```

### **Flag 2: Bits too wide (slower than expected)**

```
Expected (115200): 8.68µs per bit
Actual:            17.36µs per bit
                   ↓
Actual baudrate = 57,600 (0.5x too slow)
                   ↓
Problem: BRR set to twice the value OR clock too slow
```

### **Flag 3: Irregular timing (clock drift?)**

```
Bit 1: 8.68µs
Bit 2: 8.70µs  ← Slightly off
Bit 3: 8.67µs  ← Slightly off
Bit 4: 8.65µs  ← Slightly off
       ...

If variation > ±2%: Clock might be unstable or wrong
```

### **Flag 4: Gaps on RX while TX OK**

```
TX line (what STM32 sends):
  Byte1      Byte2      Byte3
  ┌────┐     ┌────┐     ┌────┐
──┘    └─────┘    └─────┘    └────

RX line (what PC receives):
  Byte1      Byte2      (gap!)  Byte3
  ┌────┐     ┌────┐             ┌────┐
──┘    └─────┘    └─────────────┤    └────

                 ↑
         Data loss during this period
         = ISR miss = Blocking issue!
```

---

## 🧮 Quick Calculation Reference

### **115200 Baud (your case):**

```
Time per bit:        8.68µs
1 byte (10 bits):    86.8µs (rounds to ~87µs)
1 frame + stop:      100µs
10 bytes:            870µs
100 bytes:           8.7ms
1KB:                 87ms

Example test durations:
  "Hello World\r\n" (13 bytes) = ~1.13ms on the wire
  If you see >10ms gap between bytes → Blocking detected!
```

### **Other common baudrates:**

```
9600:
  Time/bit:   104.17µs
  1 byte:     1.04ms
  10 bytes:   10.4ms

57600:
  Time/bit:   17.36µs
  1 byte:     173.6µs
  10 bytes:   1.74ms

230400:
  Time/bit:   4.34µs
  1 byte:     43.4µs
  10 bytes:   434µs
```

---

## 📋 Measurement Checklist

When you capture data, check:

```
□ Bit timing correct? (should be 8.68µs ± 2%)
  - Measure from falling edge to falling edge
  - Should be exactly 8.68µs for 115200
  - If not, baudrate mismatch

□ All bytes transmitted? (no TX gaps)
  - TX line should show clean bytes
  - No unexpected pauses
  - If missing bytes → app code issue

□ All bytes received? (no RX gaps when TX OK)
  - RX line should mirror TX
  - If RX missing bytes while TX OK → ISR issue
  - Likely cause: Blocking delays

□ Data content correct? (byte values)
  - Pulseview can decode as ASCII
  - "Hello" should show as "Hello"
  - If showing garbage → possible clock/baudrate issue

□ Protocol errors shown?
  - Look for red markers (protocol violations)
  - Might indicate corruption
```

---

## 🎯 For Your Specific Debug:

### **Procedure:**

```
1. Capture "Hello World" being sent (115200, 8N1)

2. Measure first bit:
   Expected: 8.68µs
   If: 8.68µs ± 0.2µs → ✅ Baudrate correct
   If: 4.34µs       → ❌ 2x too fast
   If: 17.36µs      → ❌ 2x too slow
   If: irregular    → ❌ Clock issue

3. Check for gaps:
   TX line shows "Hello World\r\nHello World\r\n" cleanly?
   → Yes: TX OK
   → No: App not sending all bytes
   
   RX line matches TX?
   → Yes: RX OK
   → No: RX ISR missing data → BLOCKING CONFIRMED
```

### **Expected Result:**

If your issue is **Blocking (not baudrate)**:

```
TX: ────┐  ┌──┐  ┌──┐  ┌──┐  ┌──┐  ┌──┐  ┌──┐
        └──┘  └──┘  └──┘  └──┘  └──┘  └──┘  └──
   (Clean, all 7 bytes for "Hello W")

RX: ────┐  ┌──┐           ┌──┐     ┌──┐
        └──┘  └───────────┘  └─────┘  └──────
   (Gap here! Missing bytes caught by ISR)

This proves: RX ISR missing data = Blocking issue!
Fix: Use non-blocking driver (uart_driver_eventdriven.c)
```

---

## 🎓 Key Formula Reminder:

```
For your STM32F4 @ 42MHz APB1:

BRR = 42,000,000 / (16 * 115200) = 22.8 ≈ 23

Actual baudrate = 42,000,000 / (16 * 23) = 114,130
Error = 0.93% ✅ ACCEPTABLE

Expected timing on logic analyzer:
Time/bit = 1 / 114,130 ≈ 8.76µs
1 byte = 87.6µs
```

---

**Now you have everything needed to debug with logic analyzer!** 🔬📊
