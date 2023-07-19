#ifndef __CPU_USAGE_TIMER_H__
#define __CPU_USAGE_TIMER_H__
#include "FreeRTOS.h"

extern volatile uint32_t CPU_RunTime ;
void cpu_usage_timer_Init(unsigned short psc,unsigned short arr);
#endif
