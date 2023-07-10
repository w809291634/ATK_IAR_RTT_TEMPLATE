#ifndef __USART_H
#define __USART_H
#include "stdio.h"	
#include "stm32f4xx_conf.h"

typedef struct {
  GPIO_TypeDef* gpio;
  uint8_t PinSource;
	uint16_t pin;
}uart_config;

#define IO_PIN(gpio, pin) {GPIO##gpio, GPIO_PinSource##pin ,GPIO_Pin_##pin}

void uart3_init(u32 bound);
void USART3_putstring(char *data);
#endif


