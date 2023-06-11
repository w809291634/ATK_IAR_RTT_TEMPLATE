#include "drv_common_app.h"

void pin_output_pp_Init(io_pin* io_pin)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = io_pin->pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(io_pin->gpio, &GPIO_InitStruct);
}

void pin_input_nopull_Init(io_pin* io_pin)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = io_pin->pin;
  /* input setting: not pull. */
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(io_pin->gpio, &GPIO_InitStruct);
}