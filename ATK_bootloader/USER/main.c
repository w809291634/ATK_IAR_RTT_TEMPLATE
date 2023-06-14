#include "stm32f4xx.h"
#include "systick.h"
#include "usart1.h"
#include "led.h"
#include "user_cmd.h"

static void hardware_init()
{
  NVIC_PriorityGroupConfig( NVIC_PriorityGroup_4 );		// 中断分组
  led_init();                                         // 初始化 LED
  systick_init();                                     // 时钟初始化
  /* 初始化 shell 控制台 */
  shell_hw_init(115200);                              // 初始化 控制台串口硬件
  shell_init("shell >" ,usart_puts);                  // 初始化 控制台输出
  shell_input_init(&shell_1,usart_puts);              // 初始化 交互
}

static void app_init()
{
  register_user_cmd();
}

int main(void)
{
  hardware_init();
  app_init();
  while(1){
    led_app();                // led 指示灯
    shell_hw_input();         // shell 控制台应用循环          
  }
}
