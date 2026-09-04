#ifndef __USART3_H
#define __USART3_H


/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"


/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
extern uint8_t  u3_recvbuf[1024];
extern uint32_t u3_recvcnt;

void USART3_IRQHandler(void);
void USART3_Config(u32 baud);
void USART3_SendString(const char *str);
void USART3_SendBytes(const uint8_t *data, uint16_t len);


#endif 


