#ifndef __HC05_H
#define __HC05_H

#include "stm32f4xx.h"

/* HC-05 蓝牙模块波特率，一般默认是 9600 */
#define HC05_BAUD_RATE  9600

void HC05_Init(void);
void HC05_SendString(const char *str);
void HC05_SendFormat(const char *fmt, ...);
uint8_t HC05_GetCommand(char *out_cmd);

#endif
