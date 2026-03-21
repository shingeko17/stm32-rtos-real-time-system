# CMSIS - Cortex Microcontroller Software Interface Standard

**Purpose**: Standardized interface for ARM Cortex-M microcontroller hardware  
**MCU**: STM32F407VET6 (ARM Cortex-M4F)  
**CMSIS Version**: 5.x

---

## CMSIS Components

### 1. Core Peripherals

**Location**: `CMSIS/Include/core_cm4.h`

| Component | Features | STM32F407 Support |
|-----------|----------|-------------------|
| Systick Timer | System tick generation | ✅ Yes |
| NVIC | Interrupt controller | ✅ Yes |
| MPU | Memory protection | ✅ Yes |
| FPU | Floating-point unit | ✅ Yes (M4F) |
| SysTick | System tick timer | ✅ Yes |
| ITCM/DTCM | Code/data caches | ⚠️ Partial |

### 2. Device Files

**Location**: `CMSIS/Device/ST/STM32F4xx`

| File | Purpose |
|------|---------|
| `stm32f407xx.h` | Register definitions, memory layout |
| `system_stm32f4xx.h` | System initialization functions |
| `system_stm32f4xx.c` | SystemInit() implementation |
| `startup_stm32f407xx.s` | Vector table, boot code |

### 3. Device Specific Register Definitions

```c
// Example from stm32f407xx.h
typedef struct {
    __IO uint32_t CR;           // Control Register
    __IO uint32_t PLLCFGR;      // PLL Configuration Register
    __IO uint32_t CFGR;         // Clock Configuration Register
    __IO uint32_t CIR;          // Clock Interrupt Register
    // ... more registers
} RCC_TypeDef;

#define RCC ((RCC_TypeDef *) RCC_BASE)
```

---

## System Initialization (system_stm32f4xx.c)

### Key Functions

#### 1. SystemInit()
Called at startup before main():
- Configures flash wait states
- Sets clock source (HSE/HSI)
- Configures system clock frequency
- Initializes peripheral clocks

```c
void SystemInit(void)
{
    // 1. Reset RCC registers
    RCC->CR |= (uint32_t)0x00000001;      // Enable HSI
    RCC->CFGR = 0x00000000;               // Use HSI as clock
    RCC->CR &= (uint32_t)0xFEF6FFFF;      // Disable HSE, PLL
    RCC->PLLCFGR = 0x24003010;            // Reset PLL config
    RCC->CIR = 0x00000000;                // Disable clock interrupts
    
    // 2. Configure flash wait states for 168 MHz
    FLASH->ACR = (FLASH->ACR & (~FLASH_ACR_LATENCY)) | FLASH_ACR_LATENCY_5WS;
    
    // 3. Enable DCache, ICache
    // (typically enabled by HAL)
    
    // 4. Configure Vector Table location
    SCB->VTOR = FLASH_BASE | 0x0000; // Application at 0x08000000 or 0x08010000
}
```

#### 2. SystemCoreClockUpdate()
Updates `SystemCoreClock` variable after clock changes:
```c
void SystemCoreClockUpdate(void)
{
    uint32_t tmp = 0, pllvco = 0, pllp = 2, pllsource = 0, pllm = 2;
    
    // Read clock configuration
    tmp = RCC->CFGR & RCC_CFGR_SWS;
    
    switch (tmp) {
        case 0x00:  // HSI
            SystemCoreClock = HSI_VALUE;
            break;
        case 0x04:  // HSE
            SystemCoreClock = HSE_VALUE;
            break;
        case 0x08:  // PLL
            // Calculate from PLL config
            // ... complex calculation ...
            break;
    }
}
```

#### 3. SystemCoreClockConst
```c
const uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9, 10, 11, 12, 13};
const uint8_t APBPrescTable[8] = {0, 0, 0, 0, 1, 2, 3, 4};
```

---

## NVIC (Nested Vectored Interrupt Controller)

### NVIC Configuration

```c
// Set priority grouping (NVIC_PRIORITYGROUP_4: 4 bits preemption, 0 bits sub)
NVIC_SetPriorityGrouping(NVIC_PRIORITY_GROUP_4);

// Enable and configure interrupt
NVIC_SetPriority(USART1_IRQn, NVIC_EncodePriority(NVIC_PRIORITY_GROUP_4, 7, 0));
NVIC_EnableIRQ(USART1_IRQn);

// Disable interrupt
NVIC_DisableIRQ(UART4_IRQn);

// Check if interrupt is pending
if (NVIC_GetPendingIRQ(TIM2_IRQn)) {
    NVIC_ClearPendingIRQ(TIM2_IRQn);
}
```

### IRQn Numbers (from stm32f4xx.h)

```c
typedef enum {
    // Cortex-M4 exceptions
    NonMaskableInt_IRQn = -14,
    MemoryManagement_IRQn = -12,
    BusFault_IRQn = -11,
    UsageFault_IRQn = -10,
    SVCall_IRQn = -5,
    PendSV_IRQn = -2,
    SysTick_IRQn = -1,
    
    // STM32F407 external interrupts
    WWDG_IRQn = 0,
    USART1_IRQn = 37,
    USART2_IRQn = 38,
    // ... 97 total interrupts
} IRQn_Type;
```

---

## SysTick Timer

### SysTick Configuration for FreeRTOS

```c
void SysTick_Config_RTOS(void)
{
    // Configure SysTick for 1 ms tick (168 MHz clock)
    SysTick_Config(SystemCoreClock / 1000);
    
    // Set SysTick priority (lowest)
    NVIC_SetPriority(SysTick_IRQn, NVIC_EncodePriority(NVIC_PRIORITY_GROUP_4, 15, 0));
}
```

### SysTick Handler

```c
void SysTick_Handler(void)
{
    // This is called every 1 ms
    // FreeRTOS hook - increment tick counter
    xTaskIncrementTick();
    
    // Optional: call FreeRTOS scheduler
    vPortYieldFromISR();
}
```

---

## Memory Layout (CMSIS Perspective)

```
Flash Memory (512KB):
├─ 0x08000000 - 0x0800FFFF: Bootloader reserved (64KB)
├─ 0x08010000 - 0x0807FFFF: Application (448KB)
│  ├─ Vector Table
│  ├─ Code (.text)
│  └─ Constants (.rodata)
└─ .data init values

RAM (128KB):
├─ 0x20000000: Stack (grows down)
├─ 0x20002000: Task stacks
├─ 0x20006000: FreeRTOS heap
├─ 0x20008000: DMA buffers
└─ 0x2000C000: Global/static vars

CCM (64KB):
├─ 0x10000000: Critical data
└─ 0x10004000: Reserved
```

---

## SCB (System Control Block)

### Key Registers

```c
// Vector Table Offset
SCB->VTOR = FLASH_BASE + 0x10000;  // Application at 0x08010000

// System Control Register
SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;  // Enable deep sleep

// System Handler Control
if (SCB->ICSR & SCB_ICSR_PENDSVSET_Msk) {
    // PendSV exception is pending
}
```

---

## FPU Configuration

The STM32F407 includes FPU (Cortex-M4F):

```c
// Enable FPU in privileged modes
SCB->CPACR |= ((3UL << (10*2)) | (3UL << (11*2))); // CP10, CP11 full access

// Use hardware FPU in calculations
float result = sqrtf(123.45f);  // Uses FPU

// Note: FreeRTOS context switch must save/restore FPU registers
// Enable in FreeRTOSConfig.h: #define configUSE_TASK_FPU_SUPPORT 1
```

---

## Performance Measurement (DWT)

Use Data Watchpoint and Trace (DWT) for cycle counting:

```c
#define DWT_CYCCNT_ENABLE() do { \
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk; \
} while(0)

#define DWT_CYCCNT_RESET() do { \
    DWT->CYCCNT = 0; \
} while(0)

#define DWT_CYCCNT_GET() (DWT->CYCCNT)

// Usage
DWT_CYCCNT_RESET();
// your code here
uint32_t cycles = DWT_CYCCNT_GET();  // CPU cycles taken
uint32_t us = cycles / 168;          // Convert to microseconds (168 MHz)
```

---

## Linker Symbol Requirements

CMSIS expects these symbols from linker script:

```
_estack             // End of stack (stack grows down)
__bss_start__       // Start of BSS
__bss_end__         // End of BSS
__text_start__      // Start of code
__data_start__      // Start of initialized data in RAM
__data_load__       // Load address of data in Flash
__data_end__        // End of data
```

---

## CMSIS Best Practices

1. **Always use bitfield macros** instead of magic numbers:
   ```c
   // Bad
   RCC->CR = 0x00000001;
   
   // Good
   RCC->CR |= RCC_CR_HSION;
   ```

2. **Verify Vector Table location** after bootloader changes:
   ```c
   assert(SCB->VTOR == FLASH_BASE + 0x10000);
   ```

3. **Cache peripheral pointers** for high-frequency access:
   ```c
   RCC_TypeDef *rcc = RCC;  // Instead of RCC->xxx repeatedly
   ```

4. **Enable code/data cache** for better performance:
   ```c
   FLASH->ACR |= FLASH_ACR_DCEN;  // Enable D-cache
   FLASH->ACR |= FLASH_ACR_ICEN;  // Enable I-cache
   ```

5. **Validate clock configuration** after SystemInit():
   ```c
   assert(SystemCoreClock == 168000000UL);
   ```
