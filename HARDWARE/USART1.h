#ifndef USART1_H
#define USART1_H


/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"


/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

void USART1_IRQHandler(void);
void USART1_Config(u32 baud);
int _write(int file, char *ptr, int len);



#endif 


