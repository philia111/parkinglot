#ifndef DHT11_H
#define DHT11_H

#include <stdbool.h>
#include "stm32f4xx.h"

void DHT11_GPIO_Out(void);
void DHT11_GPIO_In(void);
bool DHT11_Is_ACK(void);
uint8_t Read_bit(void);
uint8_t Read_Byte(void);
bool Read_Data(uint8_t buf[5]);
bool Read_DHT11Data(uint8_t buf[5]);
void DHT11Init(void);



#endif