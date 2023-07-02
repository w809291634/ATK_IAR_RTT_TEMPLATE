#include "app_start.h"

// 复位外设寄存器
static void peripheral_reset(void)
{
  // 关闭滴答定时器，复位到默认值
  SysTick->CTRL = 0;
  SysTick->LOAD = 0;
  SysTick->VAL = 0;
  
  // 使能复位
  RCC->APB2RSTR = 0xFFFFFFFF;
  RCC->AHB1RSTR = 0xFFFFFFFF;
  RCC->AHB2RSTR = 0xFFFFFFFF;
  RCC->AHB3RSTR = 0xFFFFFFFF;

  // 关闭复位
  RCC->APB2RSTR = 0x00000000;
  RCC->AHB1RSTR = 0x00000000;
  RCC->AHB2RSTR = 0x00000000;
  RCC->AHB3RSTR = 0x00000000;
}

// 复位中断控制器
void nvic_reset(void) 
{
  // 关闭滴答定时器，复位到默认值
  SysTick->CTRL = 0;
  SysTick->LOAD = 0;
  SysTick->VAL = 0;
  
  // 禁用所有中断
  NVIC_DisableIRQ(USART1_IRQn);
  NVIC_DisableIRQ(USART3_IRQn);
  // …… 根据需要禁用其他中断

  // 清除所有中断挂起标志位
  NVIC_ClearPendingIRQ(USART1_IRQn);
  NVIC_ClearPendingIRQ(USART3_IRQn);
  // …… 根据需要清除其他中断挂起标志位
}

// 复位中断控制器
static void NVIC_Reset(void) 
{
  int i;
  __set_FAULTMASK(1); // 关闭所有中端
  
  // 关闭所有中断,清除所有中断挂起标志
  for (i = 0; i < 8; i++) {
    NVIC->ICER[i] = 0xFFFFFFFF; 
    NVIC->ICPR[i] = 0xFFFFFFFF; 
  }

  // 设置所有中断优先级为默认值
  for (i = 0; i < 240; i++) {
      NVIC->IP[i] = 0;
  }
  
  __set_FAULTMASK(0); // 使能所有中端
}

// 跳转到指定地址运行
void JumpToApp(uint32_t app_addr)
{
  pFunction Jump_To_Application; 
  uint32_t JumpAddress;	
  
  /* peripheral_reset */
  NVIC_Reset();
  peripheral_reset();
  
  /* Jump to user application */
  JumpAddress = *(volatile uint32_t*)(app_addr + 4);
  Jump_To_Application = (pFunction) JumpAddress;
  
  /* Initialize user application's Stack Pointer */
  __set_PSP(*(volatile uint32_t*)app_addr);
  __set_CONTROL(0);
  __set_MSP(*(volatile uint32_t*)app_addr);
  Jump_To_Application();

  /* 跳转成功的话，不会执行到这里，用户可以在这里添加代码 */
  while (1)
  {
  }
}

// 启动app系统分区
void start_app_partition(uint8_t partition)
{
  switch(partition){
    case 1:{
      if(sys_parameter.app1_flag==APP_OK){
        printk(INFO"Starting partition 1!\r\n");
        printk(INFO"Next automatic start partition 1!\r\n");
        JumpToApp(APP1_START_ADDR);
      }else{
        printk(INFO"Starting partition 1 fail!\r\n");
      }
    }break;
    case 2:{
      if(sys_parameter.app2_flag==APP_OK){
        printk(INFO"Starting partition 2!\r\n");
        printk(INFO"Next automatic start partition 2!\r\n");
        JumpToApp(APP2_START_ADDR);
      }else{
        printk(INFO"Starting partition 2 fail!\r\n");
      }
    }break;
    case 0xff:break;
    default:{
      printk(INFO"partition %d not assigned!\r\n",partition);
    }break;
  }
}
