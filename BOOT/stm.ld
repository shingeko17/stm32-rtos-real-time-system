/*
 * STM32F407VET6 linker script
 * Flash: 512KB @ 0x08000000
 * SRAM : 128KB @ 0x20000000
 * CCM  : 64KB  @ 0x10000000 (CPU only, no DMA)
 */

ENTRY(Reset_Handler)

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
  RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 128K
  CCM   (xrw) : ORIGIN = 0x10000000, LENGTH = 64K
}

/* Startup-visible symbols */
_estack = ORIGIN(RAM) + LENGTH(RAM);

/* Tunable minimum runtime regions */
_Min_Heap_Size  = 0x2000;  /* 8KB */
_Min_Stack_Size = 0x2000;  /* 8KB */

SECTIONS
{
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } > FLASH

  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.rodata)
    *(.rodata*)
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)
    . = ALIGN(4);
    _etext = .;
  } > FLASH

  .ARM.extab :
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } > FLASH

  .ARM.exidx :
  {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } > FLASH

  _sidata = LOADADDR(.data);
  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } > RAM AT > FLASH

  .bss (NOLOAD) :
  {
    . = ALIGN(4);
    _sbss = .;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
  } > RAM

  .ccm_data (NOLOAD) :
  {
    . = ALIGN(4);
    *(.ccm_data)
    *(.ccm_data*)
    . = ALIGN(4);
  } > CCM

  .dma_buffers (NOLOAD) :
  {
    . = ALIGN(4);
    *(.dma_buffers)
    *(.dma_buffers*)
    . = ALIGN(4);
  } > RAM

  ._user_heap_stack (NOLOAD) :
  {
    . = ALIGN(8);
    _heap_start = .;
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
    _heap_end = .;
  } > RAM

  .ARM.attributes 0 : { *(.ARM.attributes) }

  /DISCARD/ :
  {
    *(.note.GNU-stack)
    *(.gnu_debuglink)
    *(.gnu.lto_*)
  }
}

PROVIDE(__StackTop = _estack);
PROVIDE(__StackLimit = _estack - _Min_Stack_Size);
PROVIDE(__heap_base = _heap_start);
PROVIDE(__heap_limit = _heap_end);
PROVIDE(__etext = _etext);
PROVIDE(__sdata = _sdata);
PROVIDE(__edata = _edata);
PROVIDE(__sbss = _sbss);
PROVIDE(__ebss = _ebss);
