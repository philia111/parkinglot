#ifndef RTC_H
#define RTC_H

#include "stm32f4xx.h"

void RTC_Config(void);
uint8_t RTC_Set_Calendar_String(const char *time_str);
void RTC_Get_Calendar_String(char *time_str);

#endif