/*
 * STM32F407VET6 Linker Script
 * 
 * Memory Layout:
 * - Flash: 512KB (0x08000000 - 0x0807FFFF)
 * - SRAM:  128KB (0x20000000 - 0x2001FFFF)
 * - CCM:   64KB  (0x10000000 - 0x1000FFFF) - NOT accessible by DMA
 * 
 * Application starts at: 0x08010000 (Sector 4)
 * This reserves Sectors 0-3 (64KB) for future bootloader
 */

/* Define memory regions */
MEMORY
{
  /* Flash memory - Application code starts at 0x08010000 */
  FLASH (rx)  : ORIGIN = 0x08010000, LENGTH = 448K    /* 448KB for application */
  
  /* Main SRAM - 128KB total */
  RAM (rwx)   : ORIGIN = 0x20000000, LENGTH = 128K    /* Full 128KB main SRAM */
  
  /* CCM (Core Coupled Memory) - 64KB, NOT DMA-accessible */
  CCM (rwx)   : ORIGIN = 0x10000000, LENGTH = 64K     /* 64KB CCM for critical data */
}

/* Define the entry point symbol */
ENTRY(Reset_Handler)

/* Section definitions and placement */
SECTIONS
{
  /* ===== FLASH SECTIONS ===== */
  
  /* Vector Table - Must be first in Flash at application start address */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))      /* Interrupt Service Routine table */
    . = ALIGN(4);
  } > FLASH
  
  /* Executable code section */
  .text :
  {
    . = ALIGN(4);
    *(.text)                  /* Code from .text sections */
    *(.text*)                 /* Code from .text* sections (e.g., .text.startup) */
    *(.rodata)                /* Read-only data (constants, strings) */
    *(.rodata*)               /* Read-only data variants */
    
    /* C++ static destructors */
    KEEP(*(.dtors))
    KEEP(*(.dt_begin))
    KEEP(*(.dt_end))
    
    /* C++ static constructors */
    KEEP(*(.ctors))
    KEEP(*(.ct_begin))
    KEEP(*(.ct_end))
    
    . = ALIGN(4);
    _etext = .;               /* End of code section marker */
  } > FLASH
  
  /* Initialized data that will be copied to RAM at startup */
  .data : 
  {
    . = ALIGN(4);
    _sdata = .;               /* Start address of .data in Flash */
    *(.data)                  /* Initialized data */
    *(.data*)                 /* Initialized data variants */
    . = ALIGN(4);
    _edata = .;               /* End address of .data in Flash */
  } > FLASH
  
  /* Uninitialized data section (will be zeroed at startup) */
  .bss :
  {
    . = ALIGN(4);
    _sbss = .;                /* Start of BSS section */
    *(.bss)                   /* Uninitialized data */
    *(.bss*)                  /* Uninitialized data variants */
    *(COMMON)                 /* Common symbols */
    . = ALIGN(4);
    _ebss = .;                /* End of BSS section */
  } > FLASH
  
  /* ===== RAM SECTIONS (copied from Flash) ===== */
  
  /* RAM copy of initialized data */
  .data_ram (NOLOAD) :
  {
    . = ALIGN(4);
    _data_in_ram = .;         /* Start of initialized data in RAM */
  } > RAM AT > FLASH
  
  /* Stack section - for Main Stack Pointer (MSP) from interrupts */
  .stack :
  {
    . = ALIGN(4);
    . += 0x2000;              /* 8KB main stack for interrupts and startup */
    _estack = .;              /* End of stack (grows downward) */
  } > RAM
  
  /* ===== HEAP SECTION ===== */
  
  .heap :
  {
    . = ALIGN(4);
    _heap_start = .;          /* Start of heap for malloc/calloc */
    . += 0x2000;              /* 8KB FreeRTOS heap */
    . = ALIGN(4);
    _heap_end = .;            /* End of heap */
  } > RAM
  
  /* ===== CCM SECTION (Core Coupled Memory) ===== */
  /* 
   * WARNING: CCM is NOT accessible by DMA!
   * Use for: Critical control data, PID coefficients, TCBs
   * Do NOT use for: DMA buffers, peripheral read/write data
   */
  
  .ccm_data :
  {
    . = ALIGN(4);
    *(.ccm_data)              /* Variables marked for CCM storage */
    *(.ccm_data*)
    . = ALIGN(4);
    _ccm_data_start = ADDR(.ccm_data);  /* CCM start for copy routine */
    _ccm_data_end = .;        /* CCM end marker */
  } > CCM
  
  /* DMA buffer sections (placed in main RAM, NOT CCM) */
  .dma_buffers (NOLOAD) :
  {
    . = ALIGN(4);
    *(.dma_buffers)           /* DMA-accessible buffers */
    . = ALIGN(4);
  } > RAM
  
  /* Configuration/Debugging sections (optional) */
  .ARM.attributes 0 : { *(.ARM.attributes) }
  
  /* Remove unused sections to save space */
  /DISCARD/ :
  {
    *(.note.GNU-stack)        /* Remove GNU stack info */
    *(.gnu_debuglink)
    *(.gnu.lto_*)
  }
}

/* ===== Symbol Definitions for Startup Code ===== */

/* Flash to RAM copy symbols */
_sidata = LOADADDR(.data);    /* Source address of .data in Flash */

/* These symbols are referenced by startup code in stm.s */
/* Do not remove or modify */
__etext = _etext;
__sdata = _sdata;
__edata = _edata;
__sbss = _sbss;
__ebss = _ebss;
__StackTop = _estack;
__StackLimit = _estack - 0x2000;
__heap_base = _heap_start;
__heap_limit = _heap_end;
