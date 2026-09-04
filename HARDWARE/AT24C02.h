#ifndef AT24C02_H
#define AT24C02_H

#include "stm32f4xx.h"
#include <stdbool.h>


void IIC_SCLConfig(void);
void IIC_SDAConfig(GPIOMode_TypeDef GPIO_Mode);
void IIC_Config(void);
void IIC_Start(void);
void IIC_SendByte(uint8_t Byte);
bool  IIC_IsSlaveACK(void);
uint8_t IIC_ReadByte(void);
void IIC_MasterACK(uint8_t ack);
void IIC_Stop(void);
void AT24C02_Config(void);
bool AT24C02_ByteWrite(uint8_t Addr,uint8_t Byte);
uint8_t  AT24C02_CurrentAddressRead(void);
uint8_t  AT24C02_RandomAddressRead(uint8_t DestAddr);




#endif