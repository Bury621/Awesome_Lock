#ifndef __MULTI_TIMER_H
#define __MULTI_TIMER_H

#include "main.h"

typedef void (*TimerCallback)(void);

void MultiTimer_Init(void);
void MultiTimer_Start(uint32_t seconds_1, TimerCallback func_1, 
                     uint32_t seconds_2, TimerCallback func_2);
void MultiTimer_Stop(void);
uint8_t MultiTimer_IsRunning(void);
void MultiTimer_IRQ_Handler(void);  // 确保这行存在

#endif