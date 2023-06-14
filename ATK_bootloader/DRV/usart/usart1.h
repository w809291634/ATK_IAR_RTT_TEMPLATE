#ifndef __USART1_H
#define __USART1_H
#include "stdio.h"	
#include "stm32f4xx_conf.h"
#include "sys.h" 
#include "shell.h"

extern shellinput_t shell_1;

void shell_hw_init(u32 bound);
void usart_puts(const char * strbuf, unsigned short len);
void shell_hw_input(void);
#endif
