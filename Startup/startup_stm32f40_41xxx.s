/**
 ******************************************************************************
 * @file    startup_stm32f40_41xxx.s  (GCC / GNU AS version)
 * @brief   STM32F40x/41xxx startup file for arm-none-eabi-gcc toolchain.
 *
 * IMPORTANT: Uses _estack from the linker script as initial stack pointer.
 *   _estack = ORIGIN(RAM) + LENGTH(RAM)  (defined in .ld file)
 ******************************************************************************
 */

  .syntax unified
  .cpu cortex-m4
  .fpu softvfp
  .thumb

/* _estack is defined in the linker script as top of RAM */
  .globl _estack

/* ============================================================
 * Vector Table  — placed in .isr_vector (→ Flash by linker)
 * ============================================================ */
  .section .isr_vector, "a", %progbits
  .type  __isr_vector, %object
  .align 2
  .globl __isr_vector
__isr_vector:
  .word  _estack                            /* Initial Stack Pointer     */
  .word  Reset_Handler                      /* Reset Handler             */
  .word  NMI_Handler                        /* NMI Handler               */
  .word  HardFault_Handler                  /* Hard Fault Handler        */
  .word  MemManage_Handler                  /* MPU Fault Handler         */
  .word  BusFault_Handler                   /* Bus Fault Handler         */
  .word  UsageFault_Handler                 /* Usage Fault Handler       */
  .word  0                                  /* Reserved                  */
  .word  0                                  /* Reserved                  */
  .word  0                                  /* Reserved                  */
  .word  0                                  /* Reserved                  */
  .word  SVC_Handler                        /* SVCall Handler            */
  .word  DebugMon_Handler                   /* Debug Monitor Handler     */
  .word  0                                  /* Reserved                  */
  .word  PendSV_Handler                     /* PendSV Handler            */
  .word  SysTick_Handler                    /* SysTick Handler           */

  /* External Interrupts */
  .word  WWDG_IRQHandler
  .word  PVD_IRQHandler
  .word  TAMP_STAMP_IRQHandler
  .word  RTC_WKUP_IRQHandler
  .word  FLASH_IRQHandler
  .word  RCC_IRQHandler
  .word  EXTI0_IRQHandler
  .word  EXTI1_IRQHandler
  .word  EXTI2_IRQHandler
  .word  EXTI3_IRQHandler
  .word  EXTI4_IRQHandler
  .word  DMA1_Stream0_IRQHandler
  .word  DMA1_Stream1_IRQHandler
  .word  DMA1_Stream2_IRQHandler
  .word  DMA1_Stream3_IRQHandler
  .word  DMA1_Stream4_IRQHandler
  .word  DMA1_Stream5_IRQHandler
  .word  DMA1_Stream6_IRQHandler
  .word  ADC_IRQHandler
  .word  CAN1_TX_IRQHandler
  .word  CAN1_RX0_IRQHandler
  .word  CAN1_RX1_IRQHandler
  .word  CAN1_SCE_IRQHandler
  .word  EXTI9_5_IRQHandler
  .word  TIM1_BRK_TIM9_IRQHandler
  .word  TIM1_UP_TIM10_IRQHandler
  .word  TIM1_TRG_COM_TIM11_IRQHandler
  .word  TIM1_CC_IRQHandler
  .word  TIM2_IRQHandler
  .word  TIM3_IRQHandler
  .word  TIM4_IRQHandler
  .word  I2C1_EV_IRQHandler
  .word  I2C1_ER_IRQHandler
  .word  I2C2_EV_IRQHandler
  .word  I2C2_ER_IRQHandler
  .word  SPI1_IRQHandler
  .word  SPI2_IRQHandler
  .word  USART1_IRQHandler
  .word  USART2_IRQHandler
  .word  USART3_IRQHandler
  .word  EXTI15_10_IRQHandler
  .word  RTC_Alarm_IRQHandler
  .word  OTG_FS_WKUP_IRQHandler
  .word  TIM8_BRK_TIM12_IRQHandler
  .word  TIM8_UP_TIM13_IRQHandler
  .word  TIM8_TRG_COM_TIM14_IRQHandler
  .word  TIM8_CC_IRQHandler
  .word  DMA1_Stream7_IRQHandler
  .word  FSMC_IRQHandler
  .word  SDIO_IRQHandler
  .word  TIM5_IRQHandler
  .word  SPI3_IRQHandler
  .word  UART4_IRQHandler
  .word  UART5_IRQHandler
  .word  TIM6_DAC_IRQHandler
  .word  TIM7_IRQHandler
  .word  DMA2_Stream0_IRQHandler
  .word  DMA2_Stream1_IRQHandler
  .word  DMA2_Stream2_IRQHandler
  .word  DMA2_Stream3_IRQHandler
  .word  DMA2_Stream4_IRQHandler
  .word  ETH_IRQHandler
  .word  ETH_WKUP_IRQHandler
  .word  CAN2_TX_IRQHandler
  .word  CAN2_RX0_IRQHandler
  .word  CAN2_RX1_IRQHandler
  .word  CAN2_SCE_IRQHandler
  .word  OTG_FS_IRQHandler
  .word  DMA2_Stream5_IRQHandler
  .word  DMA2_Stream6_IRQHandler
  .word  DMA2_Stream7_IRQHandler
  .word  USART6_IRQHandler
  .word  I2C3_EV_IRQHandler
  .word  I2C3_ER_IRQHandler
  .word  OTG_HS_EP1_OUT_IRQHandler
  .word  OTG_HS_EP1_IN_IRQHandler
  .word  OTG_HS_WKUP_IRQHandler
  .word  OTG_HS_IRQHandler
  .word  DCMI_IRQHandler
  .word  CRYP_IRQHandler
  .word  HASH_RNG_IRQHandler
  .word  FPU_IRQHandler

  .size  __isr_vector, . - __isr_vector

/* ============================================================
 * Reset Handler
 * ============================================================ */
  .section .text.Reset_Handler
  .weak   Reset_Handler
  .type   Reset_Handler, %function
Reset_Handler:
  /* Copy .data section (initialized vars) from Flash to SRAM */
  ldr   r0, =_sdata
  ldr   r1, =_edata
  ldr   r2, =_sidata
  movs  r3, #0
  b     LoopCopyDataInit

CopyDataInit:
  ldr   r4, [r2, r3]
  str   r4, [r0, r3]
  adds  r3, r3, #4

LoopCopyDataInit:
  adds  r4, r0, r3
  cmp   r4, r1
  bcc   CopyDataInit

  /* Zero-fill .bss section */
  ldr   r2, =_sbss
  ldr   r4, =_ebss
  movs  r3, #0
  b     LoopFillZerobss

FillZerobss:
  str   r3, [r2]
  adds  r2, r2, #4

LoopFillZerobss:
  cmp   r2, r4
  bcc   FillZerobss

  /* Call SystemInit (clock setup) */
  bl    SystemInit

  /* Call main */
  bl    main

  /* Should never reach here */
Hang:
  b     Hang

  .size  Reset_Handler, . - Reset_Handler

/* ============================================================
 * Default weak exception / IRQ handlers → infinite loop
 * ============================================================ */
  .section .text.Default_Handler, "ax", %progbits
Default_Handler:
Infinite_Loop:
  b     Infinite_Loop
  .size Default_Handler, . - Default_Handler

  .macro  def_default_handler  handler_name
  .weak   \handler_name
  .set    \handler_name, Default_Handler
  .endm

  def_default_handler  NMI_Handler
  def_default_handler  HardFault_Handler
  def_default_handler  MemManage_Handler
  def_default_handler  BusFault_Handler
  def_default_handler  UsageFault_Handler
  def_default_handler  SVC_Handler
  def_default_handler  DebugMon_Handler
  def_default_handler  PendSV_Handler
  def_default_handler  SysTick_Handler

  def_default_handler  WWDG_IRQHandler
  def_default_handler  PVD_IRQHandler
  def_default_handler  TAMP_STAMP_IRQHandler
  def_default_handler  RTC_WKUP_IRQHandler
  def_default_handler  FLASH_IRQHandler
  def_default_handler  RCC_IRQHandler
  def_default_handler  EXTI0_IRQHandler
  def_default_handler  EXTI1_IRQHandler
  def_default_handler  EXTI2_IRQHandler
  def_default_handler  EXTI3_IRQHandler
  def_default_handler  EXTI4_IRQHandler
  def_default_handler  DMA1_Stream0_IRQHandler
  def_default_handler  DMA1_Stream1_IRQHandler
  def_default_handler  DMA1_Stream2_IRQHandler
  def_default_handler  DMA1_Stream3_IRQHandler
  def_default_handler  DMA1_Stream4_IRQHandler
  def_default_handler  DMA1_Stream5_IRQHandler
  def_default_handler  DMA1_Stream6_IRQHandler
  def_default_handler  ADC_IRQHandler
  def_default_handler  CAN1_TX_IRQHandler
  def_default_handler  CAN1_RX0_IRQHandler
  def_default_handler  CAN1_RX1_IRQHandler
  def_default_handler  CAN1_SCE_IRQHandler
  def_default_handler  EXTI9_5_IRQHandler
  def_default_handler  TIM1_BRK_TIM9_IRQHandler
  def_default_handler  TIM1_UP_TIM10_IRQHandler
  def_default_handler  TIM1_TRG_COM_TIM11_IRQHandler
  def_default_handler  TIM1_CC_IRQHandler
  def_default_handler  TIM2_IRQHandler
  def_default_handler  TIM3_IRQHandler
  def_default_handler  TIM4_IRQHandler
  def_default_handler  I2C1_EV_IRQHandler
  def_default_handler  I2C1_ER_IRQHandler
  def_default_handler  I2C2_EV_IRQHandler
  def_default_handler  I2C2_ER_IRQHandler
  def_default_handler  SPI1_IRQHandler
  def_default_handler  SPI2_IRQHandler
  def_default_handler  USART1_IRQHandler
  def_default_handler  USART2_IRQHandler
  def_default_handler  USART3_IRQHandler
  def_default_handler  EXTI15_10_IRQHandler
  def_default_handler  RTC_Alarm_IRQHandler
  def_default_handler  OTG_FS_WKUP_IRQHandler
  def_default_handler  TIM8_BRK_TIM12_IRQHandler
  def_default_handler  TIM8_UP_TIM13_IRQHandler
  def_default_handler  TIM8_TRG_COM_TIM14_IRQHandler
  def_default_handler  TIM8_CC_IRQHandler
  def_default_handler  DMA1_Stream7_IRQHandler
  def_default_handler  FSMC_IRQHandler
  def_default_handler  SDIO_IRQHandler
  def_default_handler  TIM5_IRQHandler
  def_default_handler  SPI3_IRQHandler
  def_default_handler  UART4_IRQHandler
  def_default_handler  UART5_IRQHandler
  def_default_handler  TIM6_DAC_IRQHandler
  def_default_handler  TIM7_IRQHandler
  def_default_handler  DMA2_Stream0_IRQHandler
  def_default_handler  DMA2_Stream1_IRQHandler
  def_default_handler  DMA2_Stream2_IRQHandler
  def_default_handler  DMA2_Stream3_IRQHandler
  def_default_handler  DMA2_Stream4_IRQHandler
  def_default_handler  ETH_IRQHandler
  def_default_handler  ETH_WKUP_IRQHandler
  def_default_handler  CAN2_TX_IRQHandler
  def_default_handler  CAN2_RX0_IRQHandler
  def_default_handler  CAN2_RX1_IRQHandler
  def_default_handler  CAN2_SCE_IRQHandler
  def_default_handler  OTG_FS_IRQHandler
  def_default_handler  DMA2_Stream5_IRQHandler
  def_default_handler  DMA2_Stream6_IRQHandler
  def_default_handler  DMA2_Stream7_IRQHandler
  def_default_handler  USART6_IRQHandler
  def_default_handler  I2C3_EV_IRQHandler
  def_default_handler  I2C3_ER_IRQHandler
  def_default_handler  OTG_HS_EP1_OUT_IRQHandler
  def_default_handler  OTG_HS_EP1_IN_IRQHandler
  def_default_handler  OTG_HS_WKUP_IRQHandler
  def_default_handler  OTG_HS_IRQHandler
  def_default_handler  DCMI_IRQHandler
  def_default_handler  CRYP_IRQHandler
  def_default_handler  HASH_RNG_IRQHandler
  def_default_handler  FPU_IRQHandler

  .end
