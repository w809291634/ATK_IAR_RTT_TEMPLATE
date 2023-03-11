#ifndef __LED_DRV_H__
#define __LED_DRV_H__
#include "appdrv_common.h"

#define LED1_NUM 1
#define LED2_NUM 2
void LED_init(void);
void led_ctrl(unsigned char cmd);
#endif