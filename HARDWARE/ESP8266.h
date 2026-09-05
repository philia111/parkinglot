#ifndef __ESP8266_H__
#define __ESP8266_H__

#include "stm32f4xx.h"


#define WIFI_SSID       "2302"
#define WIFI_PSWD		"13638065697"

uint8_t ESP8266_Send_Cmd(char* cmd , const char* rsp, u32 timeoutms);
void ESP8266_Init(void);
uint8_t ESP8266_Connect_WIFI(void);
void ESP8266_Reset(void);
void ESP8266_Exit_Transparent_Transmission (void);
uint8_t  ESP8266_Entry_Transparent_Transmission(void);
int8_t ESP8266_Connect_Server(char *ip, uint16_t port);
int8_t ESP8266_Disconnect_Server(void);
uint8_t ESP8266_Get_Network_Time(char *datetime_buf);


#endif




