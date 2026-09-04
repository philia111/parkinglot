
#include "USART2.h"

uint8_t  u2_recvbuf[256] = {0};
uint32_t u2_recvcnt = 0;

void USART2_IRQHandler(void)
{
    // 判断接收中断是否发生
    if (USART_GetITStatus(USART2, USART_IT_RXNE) == SET)
    {
        // 从USART2中接收一个字节，加边界保护防止越界
        if(u2_recvcnt < sizeof(u2_recvbuf) - 2)
        {
            u2_recvbuf[u2_recvcnt++] = USART_ReceiveData(USART2);
        }
        else
        {
            USART_ReceiveData(USART2); // 丢弃数据，但必须读取以清除中断标志
        }
    }
    
    // 判断空闲中断是否发生（一帧数据接收完毕）
    if (USART_GetITStatus(USART2, USART_IT_IDLE) == SET)
    {
        // 清除IDLE中断标志：先读SR再读DR
        USART2->SR;
        USART2->DR;
        
        // 在接收数据末尾加上'\0'，使其成为合法的C字符串
        u2_recvbuf[u2_recvcnt] = '\0';
    }
}

/**
 * @brief 串口2初始化函数
 *
 * @param baud 通信波特率
 */
void USART2_Config(u32 baud)
{
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;

    // 打开了GPIO端口时钟  PA2和PA3
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

    // 打开USART2的时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    // 选择GPIO引脚的复用功能
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2);

    // 配置GPIO引脚 注意：复用模式
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 配置串口参数+对串口初始化
    USART_InitStructure.USART_BaudRate = baud;                                      // 波特率
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;                     // 数据位
    USART_InitStructure.USART_StopBits = USART_StopBits_1;                          // 停止位
    USART_InitStructure.USART_Parity = USART_Parity_No;                             // 无校验
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 无流控
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;                 // 收发模式
    USART_Init(USART2, &USART_InitStructure);

    // 配置NVIC参数 + 对NVIC初始化
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // 选择USART2的中断源，接收到数据则触发中断
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
    // 开启空闲中断，用于检测一帧数据接收完毕
    USART_ITConfig(USART2, USART_IT_IDLE, ENABLE);

    // 打开串口
    USART_Cmd(USART2, ENABLE);
}


//利用串口发送一个字符串
void USART2_SendString(const char *str)
{
    while(*str)
    {
        USART_SendData(USART2,*str++);
        while( USART_GetFlagStatus(USART2,USART_FLAG_TXE) == RESET );		
    }
}
