#ifndef __USART1_H
#define __USART1_H
#include "stdio.h"	
#include "stm32f4xx_conf.h"
#include "sys.h" 
#include "shell.h"

extern char usart1_mode;
extern shellinput_t shell_1;

void shell_hw_init(u32 bound);
int usart1_getchar(uint8_t* data);
void usart1_put(const char data);
void usart1_puts(const char * strbuf, unsigned short len);
void shell_app_cycle(void);
#endif
