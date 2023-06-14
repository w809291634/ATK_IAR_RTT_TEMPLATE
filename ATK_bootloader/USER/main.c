#include "stm32f4xx.h"
#include "delay.h"

#include "usart1.h"

static void hardware_init()
{
  NVIC_PriorityGroupConfig( NVIC_PriorityGroup_4 );		// 中断分组
  delay_init(84);

  // 初始化 shell 控制台
  shell_hw_init(115200);
  shell_init("shell >" ,usart_puts);          // 初始化 控制台输出
  shell_input_init(&shell_1,usart_puts);      // 初始化 交互
}

int main(void)
{
  hardware_init();
  while(1){
    shell_hw_input();         // shell 控制台应用循环          
  }
}
