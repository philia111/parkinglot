#ifndef USART2_H
#define USART2_H


/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"


/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

extern uint8_t  u2_recvbuf[256];
extern uint32_t u2_recvcnt;

void USART2_IRQHandler(void);
void USART2_Config(u32 baud);
void USART2_SendString(const char *str);



#endif 


