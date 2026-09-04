#include "USART3.h"

uint8_t  u3_recvbuf[1024] = {0};
uint32_t u3_recvcnt = 0;


//前台程序就是中断服务程序，该程序是不需要手动调用的，当中断触发之后CPU会自动跳转过来执行该函数
void USART3_IRQHandler(void)
{
	
  //判断接收中断是否发生
  if (USART_GetITStatus(USART3, USART_IT_RXNE) == SET)
  {   
		//从USART3中接收一个字节，加边界保护防止越界
		if(u3_recvcnt < sizeof(u3_recvbuf) - 2)  //预留一个位置给'\0'
		{
			u3_recvbuf[u3_recvcnt++] = USART_ReceiveData(USART3);
		}
		else
		{
			USART_ReceiveData(USART3);  //丢弃数据，但必须读取以清除中断标志
		}
  }
	
  //判断空闲中断是否发生（一帧数据接收完毕）
  if (USART_GetITStatus(USART3, USART_IT_IDLE) == SET)
  {
		//清除IDLE中断标志：先读SR再读DR
		USART3->SR;
		USART3->DR;
		
		//在接收数据末尾加上'\0'，使其成为合法的C字符串
		u3_recvbuf[u3_recvcnt] = '\0';
  }
}



void USART3_Config(u32 baud)
{
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	
	//打开了GPIO端口时钟  PB10和PB11
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	
	//打开USART3的时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
	
	//选择GPIO引脚的复用功能
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource11 , GPIO_AF_USART3);
   GPIO_PinAFConfig(GPIOB, GPIO_PinSource10 , GPIO_AF_USART3);
	
	//配置GPIO引脚 注意：复用模式
	GPIO_InitStructure.GPIO_Mode 	= GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd 	= GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Pin 	= GPIO_Pin_11|GPIO_Pin_10;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	//配置串口参数+对串口初始化
	USART_InitStructure.USART_BaudRate = baud;																		//波特率
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;										//数据位
	USART_InitStructure.USART_StopBits = USART_StopBits_1;												//停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;														//无校验
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; 				//无流控
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;									//收发模式
	USART_Init(USART3, &USART_InitStructure);

	//配置NVIC参数 + 对NVIC初始化
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
	
	//选择USART3的中断源，接收到数据则触发中断
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
	
	//开启空闲中断，用于检测一帧数据接收完毕
	USART_ITConfig(USART3, USART_IT_IDLE, ENABLE);
	
	//打开串口
	USART_Cmd(USART3, ENABLE);
}



//利用串口发送一个字符串
void  USART3_SendString(const char *str)
{
	while(*str)
	{
		 USART_SendData(USART3,*str++);
		 while( USART_GetFlagStatus(USART3,USART_FLAG_TXE) == RESET );		
	}
}

// 按长度发送二进制数据（MQTT报文包含0x00字节，不能用SendString！）
void USART3_SendBytes(const uint8_t *data, uint16_t len)
{
	uint16_t i;
	for (i = 0; i < len; i++)
	{
		USART_SendData(USART3, data[i]);
		while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
	}
}