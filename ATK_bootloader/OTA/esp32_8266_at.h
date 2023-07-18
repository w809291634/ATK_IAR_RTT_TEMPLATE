#ifndef __ESP32_8266_AT_H__
#define __ESP32_8266_AT_H__
#include "stm32f4xx_conf.h"
#include "sys.h" 

extern char esp_link;    

int esp_command_handle(char* buf,unsigned short len);
void esp_at_app_init(void);
void esp_at_app_cycle(void);
void esp_connect_start(void);
void esp_reset(void);
#endif
