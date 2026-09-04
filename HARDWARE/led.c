
#include "led.h"
#include "stm32f4xx.h"

/**
 * @brief LED初始化函数 D1 PF9 ,D2 PF10 ,D3 PE13 ,D4 PE14
 *        注意：GPIO_Init函数初始化后，输出数据寄存器（ODR）的值是0，LED低电平电亮
 *        所以上电瞬间会闪一下再熄灭。在配置成输出模式之前，先把默认电平设置好！
 */
void LED_Init()
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);

 
    GPIO_SetBits(GPIOF, GPIO_Pin_9 | GPIO_Pin_10);
    GPIO_SetBits(GPIOE, GPIO_Pin_13 | GPIO_Pin_14);

    
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Low_Speed;
    GPIO_Init(GPIOF, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14;
    GPIO_Init(GPIOE, &GPIO_InitStructure);
}


void Led1_On()
{
    GPIO_ResetBits(GPIOF, GPIO_Pin_9 );
}

void Led2_On()
{
    GPIO_ResetBits(GPIOF, GPIO_Pin_10 );
}

void Led3_On()
{
    GPIO_ResetBits(GPIOE, GPIO_Pin_13 );
}

void Led4_On()
{
     GPIO_ResetBits(GPIOE, GPIO_Pin_14 );
}

void Led1_Off()
{
    GPIO_SetBits(GPIOF, GPIO_Pin_9 );
}

void Led2_Off()
{
    GPIO_SetBits(GPIOF, GPIO_Pin_10 );
}

void Led3_Off()
{
    GPIO_SetBits(GPIOE, GPIO_Pin_13 );
}

void Led4_Off()
{
    GPIO_SetBits(GPIOE, GPIO_Pin_14 );
}


