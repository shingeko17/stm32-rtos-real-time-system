# Error Handling Convention v1.0

## Core Principle

**All driver functions return status codes. Application layer checks them.**

---

## Error Code Definitions

```c
// drivers/driver/driver_types.h
typedef int16_t drv_status_t;

#define DRV_OK           0      // Success, no error
#define DRV_ERR_INIT    -1      // Initialization failed (clock not enabled, etc.)
#define DRV_ERR_INVALID -2      // Invalid parameter (speed > 100%, etc.)
#define DRV_ERR_TIMEOUT -3      // Operation timeout (ADC failed to convert, etc.)
#define DRV_ERR_HW_FAULT -4     // Hardware fault (sensor out of range, etc.)
```

---

## Error Handling Pattern

### Driver Functions (errors should be rare)
```c
// drivers/driver/motor_driver.c
drv_status_t Motor_A_Run(int16_t speed) {
    if (speed < -100 || speed > 100) {
        return DRV_ERR_INVALID;  // Invalid range
    }
    // ... execute ...
    return DRV_OK;
}
```

### Application Handling (errors are FATAL)
```c
// app/control_loop.c
void Control_MotorLoop(void) {
    drv_status_t status = Motor_A_Run(75);
    if (status != DRV_OK) {
        // STOP: cannot continue
        Motor_All_Brake();
        // Log error or reboot
        while(1);
    }
}
```

### Initialization (must succeed)
```c
// app/app_f4_main.c
int main(void) {
    drv_status_t status;
    
    status = BSP_Motor_Init();
    if (status != DRV_OK) {
        __BKPT(0);  // Breakpoint for debug
        return 1;
    }
    
    status = ADC_Init();
    if (status != DRV_OK) {
        Motor_All_Brake();
        return 2;
    }
    
    // ... continue only if all OK ...
}
```

---

## ISR & HAL Layer (No Error Codes)

**Interrupt handlers and HAL primitives do NOT return errors.**

Instead, use **panic/assert** model:

```c
// drivers/hal/hal_gpio.c
void HAL_GPIO_Write(GPIO_TypeDef* port, uint16_t pin, uint8_t value) {
    if (port == NULL) {
        while(1);  // Hard fault: invalid pointer
    }
    // Execute atomically, no error path
    if (value) {
        port->BSRR = (uint32_t)pin;
    } else {
        port->BSRR = (uint32_t)pin << 16;
    }
}

// drivers/driver/encoder_driver.c - ISR
void TIM3_IRQHandler(void) {
    if (TIM3->SR & TIM_SR_CC1IF) {
        uint16_t capture = TIM3->CCR1;  // Atomic read, no error
        encoder_pulse_count++;
        TIM3->SR &= ~TIM_SR_CC1IF;  // No error path
    }
}
```

---

## Safety Policy (Application Decides)

| Error | Detection | Response | Responsibility |
|---|---|---|---|
| Parameter out of range | Driver: DRV_ERR_INVALID | App: may retry or halt | App logic |
| Overcurrent | App: ADC > 3500mA | App: Motor_All_Brake() | App policy |
| Sensor timeout | Driver: DRV_ERR_TIMEOUT | App: retry or fallback | App choice |
| Hardware fault | HAL: invalid pointer | HAL: assert/panic | HAL safety |

---

## Error Propagation Example

```c
// Scenario: ADC fails to initialize
// → HAL_ADC_Init() cannot turn on clock
// → Returns DRV_ERR_INIT

drv_status_t ADC_Init(void) {
    drv_status_t status;
    status = BSP_ADC_EnableClock();  // Calls HAL_RCC_Enable()
    if (status != DRV_OK) {
        return DRV_ERR_INIT;  // Propagate up
    }
    // Continue init...
    return DRV_OK;
}

// In main():
status = ADC_Init();
if (status == DRV_ERR_INIT) {
    // Cannot proceed without sensors
    Motor_All_Brake();
    return 1;  // Exit, reboot manually
}
```

---

## Debug Assertions (Development Only)

For development, use lightweight asserts:

```c
// drivers/driver/driver_types.h
#ifdef DEBUG
  #define ASSERT(cond) do { if (!(cond)) while(1); } while(0)
#else
  #define ASSERT(cond) ((void)0)
#endif

// Usage in driver:
void Motor_A_Run(int16_t speed) {
    ASSERT(speed >= -100 && speed <= 100);  // Debug only
    if (speed < -100 || speed > 100) {
        return DRV_ERR_INVALID;  // Production fallback
    }
    // ...
}
```

---

## Logging Strategy (Future RTOS)

When RTOS integrates, add logging to error codes:

```c
// With RTOS + UART logging:
if (status != DRV_OK) {
    printf("ERROR: Motor_A_Run() returned %d\n", status);
    // UART queue write, non-blocking
}
```

Bare-metal has no logging (no memory), RTOS will add it.

---

## Version History

- **v1.0** (2026-03-25): Initial error handling convention
  - 4 error codes (OK, INIT, INVALID, TIMEOUT, HW_FAULT)
  - Driver returns, app decides
  - HAL/ISR panic model
