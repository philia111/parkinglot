/**
 * @file beep.c
 * @author philia
 * @brief  实现蜂鸣器的控制
 * @version 0.1
 * @date 2026-07-14
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#include "beep.h"
#include "delay.h"



/**
 * @brief 蜂鸣器初始化函数 PF8
 * 
 */
void Beep_Init()
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Low_Speed;

    GPIO_Init(GPIOF, &GPIO_InitStructure);

    GPIO_ResetBits(GPIOF, GPIO_Pin_8);
}

/**
 * @brief 蜂鸣器打开
 * 
 */
void Beep_On()
{
   
}

/**
 * @brief 蜂鸣器关闭
 * 
 */
void Beep_Off()
{
    GPIO_ResetBits(GPIOF, GPIO_Pin_8);
}

void Beep(uint32_t ms)
{
    GPIO_SetBits(GPIOF, GPIO_Pin_8);
    delay_ms(ms);
    GPIO_ResetBits(GPIOF, GPIO_Pin_8);

}