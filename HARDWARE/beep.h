#ifndef BEEP_H
#define BEEP_H

#include "stm32f4xx.h"

void Beep_Init(void);

void Beep_On(void);

void Beep_Off(void);

void Beep(uint32_t ms);


#endif