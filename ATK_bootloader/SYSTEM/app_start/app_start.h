#ifndef __APP_START_H__
#define __APP_START_H__
#include "sys.h"

void nvic_reset(void) ;
void reset_peripheral_register(void);
void start_app_partition(uint8_t partition);
#endif
