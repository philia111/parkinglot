/*
 ******************************************************************************
 * @file    STM32F407xx_FLASH.ld
 * @brief   GCC Linker Script for STM32F407xx
 *
 * STM32F407xx Memory Map:
 *   FLASH  : 1024 KB  @ 0x08000000
 *   SRAM1  :  112 KB  @ 0x20000000
 *   SRAM2  :   16 KB  @ 0x2001C000
 *   CCM    :   64 KB  @ 0x10000000  (only CPU access, no DMA)
 *
 * Modify FLASH/RAM LENGTH below to match your exact chip variant:
 *   STM32F405RG / STM32F407VG  -> FLASH 1024K, RAM 112K
 *   STM32F405RE / STM32F407VE  -> FLASH  512K, RAM 112K
 ******************************************************************************
 */

/* Entry point */
ENTRY(Reset_Handler)

/* Highest address of the user mode stack (end of SRAM1) */
_estack = ORIGIN(RAM) + LENGTH(RAM);

/* Minimum sizes for heap and stack (adjust as needed) */
_Min_Heap_Size  = 0x200;   /* 512 bytes  */
_Min_Stack_Size = 0x400;   /* 1024 bytes */

/* ============================================================
 * Memory Regions
 * ============================================================ */
MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K   /* STM32F407ZET6: 512KB Flash (suffix E) */
  RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 112K
  CCMRAM(xrw) : ORIGIN = 0x10000000, LENGTH = 64K
}

/* ============================================================
 * Section Layout
 * ============================================================ */
SECTIONS
{
  /* ---- Interrupt Vector Table → Flash ---- */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >FLASH

  /* ---- Code & Read-only Data → Flash ---- */
  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)

    KEEP(*(.init))
    KEEP(*(.fini))

    . = ALIGN(4);
    _etext = .;
  } >FLASH

  /* ---- Read-only data → Flash ---- */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } >FLASH

  /* ARM exception unwind tables */
  .ARM.extab :
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } >FLASH

  .ARM :
  {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >FLASH

  /* Constructors/Destructors */
  .preinit_array :
  {
    PROVIDE_HIDDEN(__preinit_array_start = .);
    KEEP(*(.preinit_array*))
    PROVIDE_HIDDEN(__preinit_array_end = .);
  } >FLASH

  .init_array :
  {
    PROVIDE_HIDDEN(__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array*))
    PROVIDE_HIDDEN(__init_array_end = .);
  } >FLASH

  .fini_array :
  {
    PROVIDE_HIDDEN(__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array*))
    PROVIDE_HIDDEN(__fini_array_end = .);
  } >FLASH

  /* Load address of .data in Flash */
  _sidata = LOADADDR(.data);

  /* ---- Initialized Data → RAM (loaded from Flash) ---- */
  .data :
  {
    . = ALIGN(4);
    _sdata = .;     /* startup: copy from _sidata to _sdata.._edata */
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } >RAM AT> FLASH

  /* ---- CCM data (optional, CPU-only, no DMA) ---- */
  _siccmram = LOADADDR(.ccmram);
  .ccmram :
  {
    . = ALIGN(4);
    _sccmram = .;
    *(.ccmram)
    *(.ccmram*)
    . = ALIGN(4);
    _eccmram = .;
  } >CCMRAM AT> FLASH

  /* ---- Zero-initialized Data → RAM ---- */
  .bss :
  {
    . = ALIGN(4);
    _sbss = .;      /* startup: zero-fill from _sbss to _ebss */
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
  } >RAM

  /* ---- User Heap & Stack check ---- */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE(end = .);
    PROVIDE(_end = .);
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM

  /* Remove unused debug sections to reduce size */
  /DISCARD/ :
  {
    libc.a (*)
    libm.a (*)
    libgcc.a (*)
  }

  .ARM.attributes 0 :
  {
    *(.ARM.attributes)
  }
}
