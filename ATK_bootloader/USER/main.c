#include "stm32f4xx.h"
#include "systick.h"
#include "user_cmd.h"
#include "usart1.h"
#include "usart3.h"
#include "led.h"
#include "esp32_at.h"
#include "soft_timer.h"
#include "config.h"
#include "flash.h"
#include "download.h"
#include "app_start.h"

// 系统参数保存区域
sys_parameter_t sys_parameter;

// 系统初始化
static void system_init()
{
  /* 初始化基本硬件 */
  NVIC_SetVectorTable(NVIC_VectTab_FLASH,0);          // 重映射中断向量表
  NVIC_PriorityGroupConfig( NVIC_PriorityGroup_4 );		// 中断分组
  shell_hw_init(115200);                              // 初始化 控制台串口硬件
  /* 启动 app */
  SYS_PARAMETER_READ;
  start_app_partition(sys_parameter.current_part);
  
  /* 初始化 shell 控制台 */
  shell_init("shell >" ,usart1_puts);     // 初始化 控制台输出
  shell_input_init(&shell_1,usart1_puts); // 初始化 交互
  welcome_gets(&shell_1,0,0);             // 主动显示 welcome'
  printf(INFO"Entered the bootloader program!\r\n");
  cmdline_gets(&shell_1,"\r",1);          // 一次换行
}

// 硬件初始化
static void hardware_init()
{
  led_init();                             // 初始化 LED
  systick_init();                         // 时钟初始化
  softTimer_Init();                       // 软件定时器初始化
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
  system_init();
  hardware_init();
  app_init();
  while(1){
    softTimer_Update();         // 软件定时器扫描
    esp32_at_app_cycle();       // esp32 的应用循环
    if(usart1_mode==0)
      shell_app_cycle();          // shell 控制台应用循环 
    else{
      IAP_download();
    }
  }
}
