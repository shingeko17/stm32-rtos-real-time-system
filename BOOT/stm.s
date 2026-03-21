/*
 * STM32F407VET6 Startup Assembly Code
 * ARM Cortex-M4 processor
 * 
 * This file:
 * 1. Initializes the processor and memory
 * 2. Sets up the stack and heap
 * 3. Copies initialized data from Flash to RAM
 * 4. Zeros uninitialized data (BSS)
 * 5. Calls the main() function
 * 
 * Default stack and heap sizes can be overridden at link time
 */

    .syntax unified
    .cpu cortex-m4
    .fpu softvfp
    .thumb

/* Memory organization symbols (defined in linker script) */
    .extern _sidata        /* Start address of initialization data (Flash) */
    .extern _sdata         /* Start address of .data section (RAM) */
    .extern _edata         /* End address of .data section (RAM) */
    .extern _sbss          /* Start address of .bss section (RAM) */
    .extern _ebss          /* End address of .bss section (RAM) */
    .extern _estack        /* End of stack address */
    .extern _StackTop      /* Top of stack */

/* Declare main function as external */
    .extern main
    .extern SystemInit     /* System initialization function (optional HAL call) */

/* ===== INTERRUPT VECTOR TABLE ===== */
    .section .isr_vector, "a", %progbits
    .align 2
    .globl __isr_vector
__isr_vector:
    /* Core Exceptions */
    .word  _estack                     /* 0 : Stack Pointer Initial Value */
    .word  Reset_Handler                   /* 1 : Reset Handler */
    .word  NMI_Handler                     /* 2 : Non-Maskable Interrupt */
    .word  HardFault_Handler               /* 3 : Hard Fault */
    .word  MemManage_Handler               /* 4 : Memory Management Fault */
    .word  BusFault_Handler                /* 5 : Bus Fault */
    .word  UsageFault_Handler              /* 6 : Usage Fault */
    .word  0                               /* 7 : Reserved */
    .word  0                               /* 8 : Reserved */
    .word  0                               /* 9 : Reserved */
    .word  0                               /* 10 : Reserved */
    .word  SVC_Handler                     /* 11 : Supervisor Call (SVC) */
    .word  DebugMon_Handler                /* 12 : Debug Monitor */
    .word  0                               /* 13 : Reserved */
    .word  PendSV_Handler                  /* 14 : Pend Service Call */
    .word  SysTick_Handler                 /* 15 : System Tick Timer */
    
    /* External Interrupts (STM32F407 has 82 external interrupts) */
    .word  WWDG_IRQHandler                 /* 16 : Window Watchdog */
    .word  PVD_IRQHandler                  /* 17 : PVD through EXTI line */
    .word  TAMP_STAMP_IRQHandler           /* 18 : Tamper/Timestamp */
    .word  RTC_WKUP_IRQHandler             /* 19 : RTC Wakeup timer */
    .word  FLASH_IRQHandler                /* 20 : Flash memory */
    .word  RCC_IRQHandler                  /* 21 : Reset and Clock Config */
    .word  EXTI0_IRQHandler                /* 22 : EXTI Line 0 */
    .word  EXTI1_IRQHandler                /* 23 : EXTI Line 1 */
    .word  EXTI2_IRQHandler                /* 24 : EXTI Line 2 */
    .word  EXTI3_IRQHandler                /* 25 : EXTI Line 3 */
    .word  EXTI4_IRQHandler                /* 26 : EXTI Line 4 */
    .word  DMA1_Stream0_IRQHandler         /* 27 : DMA1 Stream 0 */
    .word  DMA1_Stream1_IRQHandler         /* 28 : DMA1 Stream 1 */
    .word  DMA1_Stream2_IRQHandler         /* 29 : DMA1 Stream 2 */
    .word  DMA1_Stream3_IRQHandler         /* 30 : DMA1 Stream 3 */
    .word  DMA1_Stream4_IRQHandler         /* 31 : DMA1 Stream 4 */
    .word  DMA1_Stream5_IRQHandler         /* 32 : DMA1 Stream 5 */
    .word  DMA1_Stream6_IRQHandler         /* 33 : DMA1 Stream 6 */
    .word  ADC_IRQHandler                  /* 34 : ADC1, ADC2, ADC3 */
    .word  CAN1_TX_IRQHandler              /* 35 : CAN1 TX */
    .word  CAN1_RX0_IRQHandler             /* 36 : CAN1 RX0 */
    .word  CAN1_RX1_IRQHandler             /* 37 : CAN1 RX1 */
    .word  CAN1_SCE_IRQHandler             /* 38 : CAN1 SCE */
    .word  EXTI9_5_IRQHandler              /* 39 : EXTI Lines 5-9 */
    .word  TIM1_BRK_TIM9_IRQHandler        /* 40 : TIM1 Break / TIM9 */
    .word  TIM1_UP_TIM10_IRQHandler        /* 41 : TIM1 Update / TIM10 */
    .word  TIM1_TRG_COM_TIM11_IRQHandler   /* 42 : TIM1 Trigger/Commutation / TIM11 */
    .word  TIM1_CC_IRQHandler              /* 43 : TIM1 Capture Compare */
    .word  TIM2_IRQHandler                 /* 44 : TIM2 */
    .word  TIM3_IRQHandler                 /* 45 : TIM3 */
    .word  TIM4_IRQHandler                 /* 46 : TIM4 */
    .word  I2C1_EV_IRQHandler              /* 47 : I2C1 Event */
    .word  I2C1_ER_IRQHandler              /* 48 : I2C1 Error */
    .word  I2C2_EV_IRQHandler              /* 49 : I2C2 Event */
    .word  I2C2_ER_IRQHandler              /* 50 : I2C2 Error */
    .word  SPI1_IRQHandler                 /* 51 : SPI1 */
    .word  SPI2_IRQHandler                 /* 52 : SPI2 */
    .word  USART1_IRQHandler               /* 53 : USART1 */
    .word  USART2_IRQHandler               /* 54 : USART2 */
    .word  USART3_IRQHandler               /* 55 : USART3 */
    .word  EXTI15_10_IRQHandler            /* 56 : EXTI Lines 10-15 */
    .word  RTC_Alarm_IRQHandler            /* 57 : RTC Alarm EXTI */
    .word  OTG_FS_WKUP_IRQHandler          /* 58 : USB OTG FS Wakeup */
    .word  TIM8_BRK_TIM12_IRQHandler       /* 59 : TIM8 Break / TIM12 */
    .word  TIM8_UP_TIM13_IRQHandler        /* 60 : TIM8 Update / TIM13 */
    .word  TIM8_TRG_COM_TIM14_IRQHandler   /* 61 : TIM8 Trigger/Commutation / TIM14 */
    .word  TIM8_CC_IRQHandler              /* 62 : TIM8 Capture Compare */
    .word  DMA1_Stream7_IRQHandler         /* 63 : DMA1 Stream 7 */
    .word  FSMC_IRQHandler                 /* 64 : FSMC */
    .word  SDIO_IRQHandler                 /* 65 : SDIO */
    .word  TIM5_IRQHandler                 /* 66 : TIM5 */
    .word  SPI3_IRQHandler                 /* 67 : SPI3 */
    .word  UART4_IRQHandler                /* 68 : UART4 */
    .word  UART5_IRQHandler                /* 69 : UART5 */
    .word  TIM6_DAC_IRQHandler             /* 70 : TIM6 / DAC */
    .word  TIM7_IRQHandler                 /* 71 : TIM7 */
    .word  DMA2_Stream0_IRQHandler         /* 72 : DMA2 Stream 0 */
    .word  DMA2_Stream1_IRQHandler         /* 73 : DMA2 Stream 1 */
    .word  DMA2_Stream2_IRQHandler         /* 74 : DMA2 Stream 2 */
    .word  DMA2_Stream3_IRQHandler         /* 75 : DMA2 Stream 3 */
    .word  DMA2_Stream4_IRQHandler         /* 76 : DMA2 Stream 4 */
    .word  ETH_IRQHandler                  /* 77 : Ethernet */
    .word  ETH_WKUP_IRQHandler             /* 78 : Ethernet Wakeup */
    .word  CAN2_TX_IRQHandler              /* 79 : CAN2 TX */
    .word  CAN2_RX0_IRQHandler             /* 80 : CAN2 RX0 */
    .word  CAN2_RX1_IRQHandler             /* 81 : CAN2 RX1 */
    .word  CAN2_SCE_IRQHandler             /* 82 : CAN2 SCE */
    .word  OTG_FS_IRQHandler               /* 83 : USB OTG FS */
    .word  DMA2_Stream5_IRQHandler         /* 84 : DMA2 Stream 5 */
    .word  DMA2_Stream6_IRQHandler         /* 85 : DMA2 Stream 6 */
    .word  DMA2_Stream7_IRQHandler         /* 86 : DMA2 Stream 7 */
    .word  USART6_IRQHandler               /* 87 : USART6 */
    .word  I2C3_EV_IRQHandler              /* 88 : I2C3 Event */
    .word  I2C3_ER_IRQHandler              /* 89 : I2C3 Error */
    .word  OTG_HS_EP1_OUT_IRQHandler       /* 90 : USB OTG HS EP1 OUT */
    .word  OTG_HS_EP1_IN_IRQHandler        /* 91 : USB OTG HS EP1 IN */
    .word  OTG_HS_WKUP_IRQHandler          /* 92 : USB OTG HS Wakeup */
    .word  OTG_HS_IRQHandler               /* 93 : USB OTG HS */
    .word  DCMI_IRQHandler                 /* 94 : DCMI */
    .word  CRYP_IRQHandler                 /* 95 : Crypto processor */
    .word  HASH_RNG_IRQHandler             /* 96 : Hash processor / RNG */
    .word  FPU_IRQHandler                  /* 97 : Floating Point Unit */

    .size __isr_vector, . - __isr_vector

/* ===== RESET HANDLER ===== */
    .section .text.startup, "ax", %progbits
    .align 2
    .weak Reset_Handler
    .type  Reset_Handler, %function

Reset_Handler:
    /* Step 1: Disable all interrupts (set PRIMASK bit) */
    cpsid i                           /* Disable all interrupts */
    
    /* Step 2: Copy initialized data from Flash to RAM */
    /* Source (_sidata in Flash), Destination (_sdata in RAM), End (_edata) */
    ldr    r0, =_sidata              /* Load source address (Flash) */
    ldr    r1, =_sdata               /* Load destination address (RAM) */
    ldr    r2, =_edata               /* Load end of data address (RAM) */
    
    /* Check if there's data to copy */
    cmp    r1, r2                     /* Compare destination start with end */
    beq    .zero_bss                  /* If equal, skip data copy */
    
.copy_data_loop:
    ldr    r3, [r0], #4               /* Load 4 bytes from Flash, increment source */
    str    r3, [r1], #4               /* Store 4 bytes to RAM, increment destination */
    cmp    r1, r2                     /* Compare current destination with end */
    bne    .copy_data_loop            /* Loop if not done */
    
    /* Step 3: Zero initialize the BSS (uninitialized data) section */
.zero_bss:
    ldr    r0, =_sbss                /* Load start of BSS section */
    ldr    r1, =_ebss                /* Load end of BSS section */
    
    /* Check if there's BSS to zero */
    cmp    r0, r1                     /* Compare start with end */
    beq    .run_libc_init             /* If equal, skip BSS zeroing */
    
    /* Zero fill BSS */
    movs   r2, #0                     /* Load 0 for zeroing */
    
.zero_bss_loop:
    str    r2, [r0], #4               /* Store 0 to memory, increment address */
    cmp    r0, r1                     /* Compare current address with end */
    bne    .zero_bss_loop             /* Loop if not done */
    
    /* Step 4: Call optional SystemInit function (if using ST HAL) */
.run_libc_init:
    /* Uncomment to call SystemInit if using ST HAL libraries
    bl     SystemInit
    */
    
    /* Step 5: Jump to main function */
    bl     main                       /* Call main() - should not return */
    
    /* If main returns, hang in loop */
.infinite_loop:
    b      .infinite_loop             /* Infinite loop - should never reach here */
    
    .size  Reset_Handler, . - Reset_Handler

/* ===== WEAK FAULT HANDLERS ===== */
/* These are default handlers that can be overridden by user code */
    .section .text, "ax", %progbits
    
    /* Non-Maskable Interrupt - cannot be disabled */
    .weak  NMI_Handler
    .type  NMI_Handler, %function
NMI_Handler:
    b      .infinite_loop
    .size  NMI_Handler, . - NMI_Handler
    
    /* Hard Fault - processor error */
    .weak  HardFault_Handler
    .type  HardFault_Handler, %function
HardFault_Handler:
    b      .infinite_loop
    .size  HardFault_Handler, . - HardFault_Handler
    
    /* Memory Management Fault */
    .weak  MemManage_Handler
    .type  MemManage_Handler, %function
MemManage_Handler:
    b      .infinite_loop
    .size  MemManage_Handler, . - MemManage_Handler
    
    /* Bus Fault */
    .weak  BusFault_Handler
    .type  BusFault_Handler, %function
BusFault_Handler:
    b      .infinite_loop
    .size  BusFault_Handler, . - BusFault_Handler
    
    /* Usage Fault - invalid instruction or state */
    .weak  UsageFault_Handler
    .type  UsageFault_Handler, %function
UsageFault_Handler:
    b      .infinite_loop
    .size  UsageFault_Handler, . - UsageFault_Handler
    
    /* Supervisor Call (SVC) - used by FreeRTOS for context switch */
    .weak  SVC_Handler
    .type  SVC_Handler, %function
SVC_Handler:
    b      .infinite_loop
    .size  SVC_Handler, . - SVC_Handler
    
    /* Debug Monitor */
    .weak  DebugMon_Handler
    .type  DebugMon_Handler, %function
DebugMon_Handler:
    b      .infinite_loop
    .size  DebugMon_Handler, . - DebugMon_Handler
    
    /* Pend Service Call (PendSV) - used by FreeRTOS for context switch */
    .weak  PendSV_Handler
    .type  PendSV_Handler, %function
PendSV_Handler:
    b      .infinite_loop
    .size  PendSV_Handler, . - PendSV_Handler
    
    /* System Tick Timer interrupt */
    .weak  SysTick_Handler
    .type  SysTick_Handler, %function
SysTick_Handler:
    b      .infinite_loop
    .size  SysTick_Handler, . - SysTick_Handler

/* ===== WEAK INTERRUPT HANDLERS ===== */
/* All 82 external interrupts have weak default implementations */
    .macro  weakhandler name
    .weak   \name
    .type   \name, %function
\name:
    b       .infinite_loop
    .endm
    
    weakhandler WWDG_IRQHandler
    weakhandler PVD_IRQHandler
    weakhandler TAMP_STAMP_IRQHandler
    weakhandler RTC_WKUP_IRQHandler
    weakhandler FLASH_IRQHandler
    weakhandler RCC_IRQHandler
    weakhandler EXTI0_IRQHandler
    weakhandler EXTI1_IRQHandler
    weakhandler EXTI2_IRQHandler
    weakhandler EXTI3_IRQHandler
    weakhandler EXTI4_IRQHandler
    weakhandler DMA1_Stream0_IRQHandler
    weakhandler DMA1_Stream1_IRQHandler
    weakhandler DMA1_Stream2_IRQHandler
    weakhandler DMA1_Stream3_IRQHandler
    weakhandler DMA1_Stream4_IRQHandler
    weakhandler DMA1_Stream5_IRQHandler
    weakhandler DMA1_Stream6_IRQHandler
    weakhandler ADC_IRQHandler
    weakhandler CAN1_TX_IRQHandler
    weakhandler CAN1_RX0_IRQHandler
    weakhandler CAN1_RX1_IRQHandler
    weakhandler CAN1_SCE_IRQHandler
    weakhandler EXTI9_5_IRQHandler
    weakhandler TIM1_BRK_TIM9_IRQHandler
    weakhandler TIM1_UP_TIM10_IRQHandler
    weakhandler TIM1_TRG_COM_TIM11_IRQHandler
    weakhandler TIM1_CC_IRQHandler
    weakhandler TIM2_IRQHandler
    weakhandler TIM3_IRQHandler
    weakhandler TIM4_IRQHandler
    weakhandler I2C1_EV_IRQHandler
    weakhandler I2C1_ER_IRQHandler
    weakhandler I2C2_EV_IRQHandler
    weakhandler I2C2_ER_IRQHandler
    weakhandler SPI1_IRQHandler
    weakhandler SPI2_IRQHandler
    weakhandler USART1_IRQHandler
    weakhandler USART2_IRQHandler
    weakhandler USART3_IRQHandler
    weakhandler EXTI15_10_IRQHandler
    weakhandler RTC_Alarm_IRQHandler
    weakhandler OTG_FS_WKUP_IRQHandler
    weakhandler TIM8_BRK_TIM12_IRQHandler
    weakhandler TIM8_UP_TIM13_IRQHandler
    weakhandler TIM8_TRG_COM_TIM14_IRQHandler
    weakhandler TIM8_CC_IRQHandler
    weakhandler DMA1_Stream7_IRQHandler
    weakhandler FSMC_IRQHandler
    weakhandler SDIO_IRQHandler
    weakhandler TIM5_IRQHandler
    weakhandler SPI3_IRQHandler
    weakhandler UART4_IRQHandler
    weakhandler UART5_IRQHandler
    weakhandler TIM6_DAC_IRQHandler
    weakhandler TIM7_IRQHandler
    weakhandler DMA2_Stream0_IRQHandler
    weakhandler DMA2_Stream1_IRQHandler
    weakhandler DMA2_Stream2_IRQHandler
    weakhandler DMA2_Stream3_IRQHandler
    weakhandler DMA2_Stream4_IRQHandler
    weakhandler ETH_IRQHandler
    weakhandler ETH_WKUP_IRQHandler
    weakhandler CAN2_TX_IRQHandler
    weakhandler CAN2_RX0_IRQHandler
    weakhandler CAN2_RX1_IRQHandler
    weakhandler CAN2_SCE_IRQHandler
    weakhandler OTG_FS_IRQHandler
    weakhandler DMA2_Stream5_IRQHandler
    weakhandler DMA2_Stream6_IRQHandler
    weakhandler DMA2_Stream7_IRQHandler
    weakhandler USART6_IRQHandler
    weakhandler I2C3_EV_IRQHandler
    weakhandler I2C3_ER_IRQHandler
    weakhandler OTG_HS_EP1_OUT_IRQHandler
    weakhandler OTG_HS_EP1_IN_IRQHandler
    weakhandler OTG_HS_WKUP_IRQHandler
    weakhandler OTG_HS_IRQHandler
    weakhandler DCMI_IRQHandler
    weakhandler CRYP_IRQHandler
    weakhandler HASH_RNG_IRQHandler
    weakhandler FPU_IRQHandler
    
    .end
