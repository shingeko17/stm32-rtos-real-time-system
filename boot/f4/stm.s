/*
 * Startup code for STM32F407VET6 (Cortex-M4)
 * Minimal bootstrap: vector table, reset handler, basic exception stubs
 */

    .syntax unified
    .thumb
    .thumb_func

    /* ========================================================================
       Vector Table (Must be at 0x08000000)
       ======================================================================== */
    
    .section .vectors, "ax"
    .align 2
    .global _vectors

_vectors:
    .long   _stack_end              /* 00: Stack pointer */
    .long   Reset_Handler           /* 01: Reset */
    .long   NMI_Handler             /* 02: NMI */
    .long   HardFault_Handler       /* 03: Hard Fault */
    .long   MemManage_Handler       /* 04: Memory Manage */
    .long   BusFault_Handler        /* 05: Bus Fault */
    .long   UsageFault_Handler      /* 06: Usage Fault */
    .long   0                       /* 07: Reserved */
    .long   0                       /* 08: Reserved */
    .long   0                       /* 09: Reserved */
    .long   0                       /* 10: Reserved */
    .long   SVC_Handler             /* 11: SV Call */
    .long   DebugMon_Handler        /* 12: Debug Monitor */
    .long   0                       /* 13: Reserved */
    .long   PendSV_Handler          /* 14: Pend SV */
    .long   SysTick_Handler         /* 15: System Tick */

    /* STM32F407 IRQ vector table (IRQ0-81) */
    .rept 82
    .long   Default_Handler
    .endr

    /* ========================================================================
       Reset Handler - Entry point
       ======================================================================== */
    
    .section .text, "ax"
    .align 2
    .global Reset_Handler
    .thumb_func

Reset_Handler:
    /* Enable FPU (Cortex-M4 specific) */
    ldr     r0, =0xE000ED88         /* CPACR address */
    ldr     r1, [r0]
    orr     r1, r1, #(0xF << 20)    /* CP10 + CP11 = Full access */
    str     r1, [r0]
    dsb
    isb

    /* Copy initialized data from FLASH to RAM */
    ldr     r0, =_data_load         /* Source address (FLASH) */
    ldr     r1, =_data_start        /* Destination address (RAM) */
    ldr     r2, =_data_size         /* Size in bytes */

    cmp     r2, #0
    beq     .Lzero_bss

.Lcopy_data_loop:
    ldr     r3, [r0], #4
    str     r3, [r1], #4
    subs    r2, r2, #4
    bgt     .Lcopy_data_loop

    /* Zero out BSS section */
.Lzero_bss:
    ldr     r1, =_bss_start
    ldr     r2, =_bss_end
    mov     r3, #0

    cmp     r1, r2
    beq     .Lcall_main

.Lzero_bss_loop:
    str     r3, [r1], #4
    cmp     r1, r2
    blt     .Lzero_bss_loop

    /* Call main() */
.Lcall_main:
    bl      main
    b       .Lend

.Lend:
    wfi
    b       .Lend

    /* ========================================================================
       Default Exception Handlers (Weak stubs)
       ======================================================================== */
    
    .weak   NMI_Handler
    .thumb_func
NMI_Handler:
    bx      lr

    .weak   HardFault_Handler
    .thumb_func
HardFault_Handler:
    bx      lr

    .weak   MemManage_Handler
    .thumb_func
MemManage_Handler:
    bx      lr

    .weak   BusFault_Handler
    .thumb_func
BusFault_Handler:
    bx      lr

    .weak   UsageFault_Handler
    .thumb_func
UsageFault_Handler:
    bx      lr

    .weak   SVC_Handler
    .thumb_func
SVC_Handler:
    bx      lr

    .weak   DebugMon_Handler
    .thumb_func
DebugMon_Handler:
    bx      lr

    .weak   PendSV_Handler
    .thumb_func
PendSV_Handler:
    bx      lr

    .weak   SysTick_Handler
    .thumb_func
SysTick_Handler:
    bx      lr

    .weak   Default_Handler
    .thumb_func
Default_Handler:
    bx      lr
