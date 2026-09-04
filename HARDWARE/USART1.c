
#include "USART1.h"

// 前台程序就是中断服务程序，该程序是不需要手动调用的，当中断触发之后CPU会自动跳转过来执行该函数
/**
 * @brief 中断服务函数，如果串口1收到数据，则转发给电脑
 *
 */
void USART1_IRQHandler(void)
{
    uint8_t data;
    // 判断中断是否发生
    if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
    {
        // 从USART1中接收一个字节
        data = USART_ReceiveData(USART1); // 一次只能接收一个字节

        // 把接收到的数据转发出去
        USART_SendData(USART1, data);
    }
}

/**
 * @brief 串口1初始化函数
 *
 * @param baud 通信波特率
 */
void USART1_Config(u32 baud)
{
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;

    // 打开了GPIO端口时钟  PA9和PA10
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

    // 打开USART1的时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

    // 选择GPIO引脚的复用功能
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);

    // 配置GPIO引脚 注意：复用模式
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 配置串口参数+对串口初始化
    USART_InitStructure.USART_BaudRate = baud;                                      // 波特率
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;                     // 数据位
    USART_InitStructure.USART_StopBits = USART_StopBits_1;                          // 停止位
    USART_InitStructure.USART_Parity = USART_Parity_No;                             // 无校验
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 无流控
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;                 // 收发模式
    USART_Init(USART1, &USART_InitStructure);

    // 配置NVIC参数 + 对NVIC初始化
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // 选择USART1的中断源，接收到数据则触发中断
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    // 打开串口
    USART_Cmd(USART1, ENABLE);
}

/**
 * @brief printf重定向到串口1发送给电脑 （gcc编译器可用）
 *
 * @param file
 * @param ptr
 * @param len
 */
int _write(int file, char *ptr, int len)
{
    for (int i = 0; i < len; i++)
    {
        USART_SendData(USART1, (uint8_t)ptr[i]);
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    }
    return len;
}
