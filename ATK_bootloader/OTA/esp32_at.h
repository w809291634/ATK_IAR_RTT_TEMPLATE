#ifndef __ESP32_AT_H__
#define __ESP32_AT_H__
#include "stm32f4xx_conf.h"
#include "sys.h" 

int esp32_command_handle(char* buf,unsigned short len);
void esp32_at_app_init(void);
void esp32_at_app_cycle(void);
void esp32_connect_start(void);
void esp32_send_IOT(const char * strbuf, unsigned short len);
void esp32_reset(void);
#endif
