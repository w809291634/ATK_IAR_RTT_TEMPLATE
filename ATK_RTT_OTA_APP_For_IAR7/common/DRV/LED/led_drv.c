#include "LED/led_drv.h"

#define LED_PINS \
	IO_PIN(F, 9) \
	IO_PIN(F, 10) \

#define   LED_RUN(PIN1,PIN2)		HAL_GPIO_WritePin(PIN1.gpio, PIN1.pin, GPIO_PIN_SET);\
																HAL_GPIO_WritePin(PIN2.gpio, PIN2.pin, GPIO_PIN_RESET)
#define   LED_RUN_N(PIN1,PIN2)	HAL_GPIO_WritePin(PIN1.gpio, PIN1.pin, GPIO_PIN_RESET);\
																HAL_GPIO_WritePin(PIN2.gpio, PIN2.pin, GPIO_PIN_SET)                     
io_pin led_pins[] = {
	LED_PINS
};
static uint8_t LED_NUMBER=sizeof(led_pins)/sizeof(led_pins[0]);

void LED_init(void)
{
	for(int t=0;t<LED_NUMBER;t++){
		pin_output_pp_Init(&led_pins[t]);
    HAL_GPIO_WritePin(led_pins[t].gpio, led_pins[t].pin, GPIO_PIN_SET);   //熄灭LED
	}
}

/*********************************************************************************************
* 名称：led_ctrl()
* 功能：LED控制
* 参数：cmd -> 控制值
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
void led_ctrl(unsigned char cmd)
{
  if(cmd & LED1_NUM)HAL_GPIO_WritePin(led_pins[0].gpio, led_pins[0].pin, GPIO_PIN_RESET);
  else HAL_GPIO_WritePin(led_pins[0].gpio, led_pins[0].pin, GPIO_PIN_SET);
  if(cmd & LED2_NUM)HAL_GPIO_WritePin(led_pins[1].gpio, led_pins[1].pin, GPIO_PIN_RESET);
  else HAL_GPIO_WritePin(led_pins[1].gpio, led_pins[1].pin, GPIO_PIN_SET);
}