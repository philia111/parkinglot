#include "sg90.h"
#include "delay.h"
/*
 * 硬件驱动SG90:
 *   用GPIO手动翻转高低电平, 在while循环中持续产生PWM波形
 *   - PWM周期: 20ms (50Hz)
 *   - 高电平时间决定角度:
 *     0°  = 0.5ms,  90° = 1.5ms,  180° = 2.5ms
 *   - 低电平时间 = 20ms - 高电平时间
 *
 * 
 */

// 微秒级延时 (使用SysTick, 时钟源21MHz, 1个计数 ≈ 1/21 us)

// TIM9_CH2 PE6 168mhz

void SG90_Init()
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    //开启时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM9, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);

    //GPIO配置
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Fast_Speed;

    // 必须在 GPIO_Init 之后（或之前）添加复用映射：
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource6, GPIO_AF_TIM9);


    GPIO_Init(GPIOE, &GPIO_InitStructure);

    TIM_TimeBaseInitStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStructure.TIM_Period = 20000 - 1;   // ARR (总刻度数 20000，周期 20ms)
    TIM_TimeBaseInitStructure.TIM_Prescaler = 168 - 1;  // PSC (分频系数 168，1个刻度 1us)


    TIM_TimeBaseInit(TIM9, &TIM_TimeBaseInitStructure);

    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_Pulse = 1500; // 默认居中 90度 (1.5ms / 1500us)
    TIM_OC2Init(TIM9, &TIM_OCInitStructure);
    TIM_OC2PreloadConfig(TIM9, TIM_OCPreload_Enable);

    TIM_Cmd(TIM9, ENABLE);

}

void SG90_SetAngle(uint8_t Angle)
{
     // 限幅保护，防止超出物理角度范围
  if (Angle < 0.0) Angle = 0.0;
  if (Angle > 180.0) Angle = 180.0;
  
     // 0度 -> 500us, 180度 -> 2500us
  uint16_t ccr_val = 500 + (uint16_t)(((uint32_t)Angle * 2000) / 180);
  
  // 更新比较寄存器 CCR，调整占空比
  TIM_SetCompare2(TIM9, ccr_val);

}