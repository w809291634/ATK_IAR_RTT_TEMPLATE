#include "stm32f4xx.h"
#include "systick.h"
#include "user_cmd.h"
#include "usart1.h"
#include "usart3.h"
#include "led.h"
#include "esp32_at.h"
#include "soft_timer.h"

// 硬件初始化
static void hardware_init()
{
  NVIC_PriorityGroupConfig( NVIC_PriorityGroup_4 );		// 中断分组
  led_init();                             // 初始化 LED
  systick_init();                         // 时钟初始化
  softTimer_Init();                       // 软件定时器初始化
  
  /* 初始化 shell 控制台 */
  shell_hw_init(115200);                  // 初始化 控制台串口硬件
  shell_init("shell >" ,usart1_puts);     // 初始化 控制台输出
  shell_input_init(&shell_1,usart1_puts); // 初始化 交互
  
  /* 初始化 ESP8266 WiFi at串口 */
  esp32_at_hw_init(115200);               // 初始化 ESP8266 WiFi at串口 
}

// 应用初始化
static void app_init()
{
  register_user_cmd();
  esp32_at_app_init();
  led_app_init();  
}

// 主函数
int main(void)
{
  hardware_init();
  app_init();
  while(1){
    softTimer_Update();         // 软件定时器扫描
    shell_app_cycle();          // shell 控制台应用循环 
    esp32_at_app_cycle();       // esp32 的应用循环
  }
}
