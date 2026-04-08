# Driver API Reference v1.0

## Motor Control API

### Motor_A_Run(speed)
```c
void Motor_A_Run(int16_t speed);
```
**Purpose:** Run motor A at specified speed and direction.

**Parameters:**
- `speed`: Speed from -100 to +100 (% power)
  - Positive: forward direction
  - Negative: backward direction
  - 0: stop (coast, no power)

**Return:** void

**Side Effects:**
- Sets TIM1_CH3 PWM duty cycle
- Sets PD0/PD1 direction pins
- Duration: immediate

**Example:**
```c
Motor_A_Run(75);    // Run motor A forward at 75% power
Motor_A_Run(-50);   // Run motor A backward at 50% power
Motor_A_Run(0);     // Stop motor A (coast)
```

### Motor_B_Run(speed)
Same as Motor_A_Run but controls motor B (TIM1_CH4, PD2/PD3).

### Motor_All_Stop(void)
```c
void Motor_All_Stop(void);
```
**Purpose:** Stop all motors (coasting mode).

**Return:** void

### Motor_All_Brake(void)
```c
void Motor_All_Brake(void);
```
**Purpose:** Emergency brake all motors (short-circuit mode).

**Return:** void

**Note:** Consumes power, faster stop than coast.

---

## Encoder API

### Encoder_GetRPM(void)
```c
uint16_t Encoder_GetRPM(void);
```
**Purpose:** Get motor RPM from encoder feedback.

**Return:** RPM (0-32767)

**Accuracy:** ±5% typical

**Example:**
```c
uint16_t rpm = Encoder_GetRPM();
if (rpm > 500) Motor_All_Brake();  // Overspeed protect
```

---

## ADC Sensor API

### ADC_ReadCurrent(void)
```c
uint16_t ADC_ReadCurrent(void);
```
**Purpose:** Read motor current from ACS712 sensor (PA0).

**Return:** Current in milliamps (0-5000 mA)

**Calibration:** 0mA = 0V, 2500mA = 2.5V (center), 5000mA = 5V

**Example:**
```c
uint16_t current = ADC_ReadCurrent();
if (current > 3500) Motor_All_Brake();  // Overcurrent protect
```

### ADC_ReadSpeed(void)
```c
uint16_t ADC_ReadSpeed(void);
```
**Purpose:** Read motor speed command from potentiometer (PA1).

**Return:** Speed request in RPM (0-500)

### ADC_ReadTemp(void)
```c
uint16_t ADC_ReadTemp(void);
```
**Purpose:** Read motor temperature from NTC sensor (PA2).

**Return:** Temperature in °C (0-100)

---

## UART Heartbeat API

### UART_SendHeartbeat(void)
```c
void UART_SendHeartbeat(void);
```
**Purpose:** Send heartbeat pulse (0xAA) to F1 via USART1 PA9.

**Return:** void

**Timing:** Must call every ~100ms for F1 watchdog.

**Example:**
```c
// In main loop:
if ((now - last_heartbeat) >= 100) {
    UART_SendHeartbeat();  // Send 0xAA
    last_heartbeat = now;
}
```

---

## Error Code Convention

**All driver functions return `drv_status_t` (int16_t):**

```c
#define DRV_OK          0
#define DRV_ERR_INIT   -1   // Initialization failed
#define DRV_ERR_INVALID -2  // Invalid parameter
#define DRV_ERR_TIMEOUT -3  // Wait timeout
#define DRV_ERR_HW_FAULT -4 // Hardware error (e.g., ADC out of range)
```

**Application must check return codes:**
```c
drv_status_t status = ADC_Init();
if (status != DRV_OK) {
    // Handle error: log, retry, or abort
    Motor_All_Brake();
    while(1);  // halt
}
```

---

## Interrupt Handlers (ISR)

### TIM3_IRQHandler (Encoder)
Automatically called on encoder pulse. Increments pulse counter.
**User:** Do NOT call directly.

### USART2_IRQHandler (F1 Heartbeat RX)
Automatically called on incoming heartbeat byte.
**User:** Do NOT call directly.

### SysTick_Handler
Called every 1ms by SysTick timer.
**User:** Do NOT call directly.

---

## Initialization Sequence

Recommended order in main():
```c
int main(void) {
    SystemCoreClockUpdate();           // Set SYSCLK
    SysTick_Config(168000);             // 1ms tick
    BSP_Motor_Init();                   // GPIO + PWM
    ADC_Init();                         // ADC + DMA
    Encoder_Init();                     // TIM3 capture
    UART_Init();                        // USART1 + interrupt
    Motor_All_Stop();                   // Safe state
    // Ready to run...
}
```

---

## Timing Guarantees

| Operation | Latency | Notes |
|---|---|---|
| Motor speed change | <1ms | PWM updates immediately |
| ADC read | 1-16ms | Circular buffer, ~100Hz sample rate |
| Encoder RPM calc | 100ms | Calculated every 100ms cycle |
| Heartbeat TX | 1ms | Blocking serial transmit |

---

## Version History

- **v1.0** (2026-03-25): Initial bare-metal API
  - Motor run/stop/brake
  - Encoder RPM feedback
  - ADC multi-channel (current, speed, temp)
  - UART heartbeat

See BARE_METAL_v1_0_SPEC.md for feature freeze.
