#ifndef __DRV_COMMON_APP_H__
#define __DRV_COMMON_APP_H__
#include "stm32f4xx_hal.h"

typedef struct{
	GPIO_TypeDef  * gpio;
	uint16_t pin;
} io_pin;

#define IO_PIN(gpio, pin) {GPIO##gpio, GPIO_PIN_##pin},

void pin_output_pp_Init(io_pin* io_pin);
void pin_input_nopull_Init(io_pin* io_pin);
#endif