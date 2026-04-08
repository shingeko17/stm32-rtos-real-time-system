/*
 * Linker script for STM32F407VET6 (Cortex-M4)
 * FLASH: 512KB (0x08000000 - 0x0807FFFF)
 * RAM:   128KB (0x20000000 - 0x2001FFFF)
 */

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
    RAM (rwx)   : ORIGIN = 0x20000000, LENGTH = 128K
}

ENTRY(Reset_Handler)

SECTIONS
{
    /* Code and read-only data */
    .text :
    {
        KEEP(*(.vectors))           /* Vector table must be first */
        *(.text)                    /* Code */
        *(.text*)                   /* Code continuation */
        *(.rodata)                  /* Read-only data */
        *(.rodata*)                 /* Read-only data continuation */
        *(.glue_7)
        *(.glue_7t)

        /* C++ support */
        KEEP(*(.init))
        KEEP(*(.fini))

        /* ARM EABI exception unwind tables */
        *(.ARM.extab* .gnu.linkonce.armextab.*)

        /* Symbol table */
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > FLASH

    /* Initialized data (will be copied from FLASH to RAM on startup) */
    .data :
    {
        _data_start = ABSOLUTE(.);  /* Start of data in RAM */
        *(.data)
        *(.data*)
        _data_end = ABSOLUTE(.);    /* End of data in RAM */
    } > RAM AT > FLASH

    /* Start/end addresses for data initialization */
    _data_load = LOADADDR(.data);   /* Load address in FLASH */
    _data_size = _data_end - _data_start;

    /* Uninitialized data (BSS) */
    .bss :
    {
        _bss_start = ABSOLUTE(.);
        *(.bss)
        *(.bss*)
        *(COMMON)
        _bss_end = ABSOLUTE(.);
    } > RAM

    /* Heap */
    .heap :
    {
        _heap_start = .;
        . += 0;
        _heap_end = .;
    } > RAM

    /* Stack at end of RAM */
    _stack_end = ORIGIN(RAM) + LENGTH(RAM);

    /* Symbol for newlib sbrk() */
    end = ORIGIN(RAM) + LENGTH(RAM);

    /* Remove unused sections */
    /DISCARD/ :
    {
        *(.note.GNU-stack)
        *(.gnu_attributes)
        *(.comment)
    }
}
