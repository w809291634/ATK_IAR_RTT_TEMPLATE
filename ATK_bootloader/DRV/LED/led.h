#ifndef __LED_H
#define __LED_H
#include "sys.h"

//LED端口定义
#define LED0 PFout(9)	  // DS0
#define LED1 PFout(10)	// DS1	 

void led_init(void);//初始化		 	
void led_app_init(void);
#endif
