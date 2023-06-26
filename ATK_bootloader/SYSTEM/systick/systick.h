#ifndef __SYSTICK_H__
#define __SYSTICK_H__		   
#include "sys.h"

#define TICK_PER_SECOND 1000

void systick_init(void);
uint32_t tick_get(void);
uint32_t micros(void);
uint32_t millis(void);
#endif
