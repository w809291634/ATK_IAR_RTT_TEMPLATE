#include "systick.h"
#include "soft_timer.h"

static uint32_t tick = 0;

void systick_init(void)
{
  SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);       
  SysTick_Config(SystemCoreClock /TICK_PER_SECOND);
  NVIC_SetPriority(SysTick_IRQn, 0xFF);
}								    

void SysTick_Handler(void)
{
  ++ tick;
  tickCnt_Update();
}

uint32_t tick_get(void)
{
    return tick;
}

//获取系统的运行时间，单位us
uint32_t micros(void)
{
  uint32_t usTick = (SysTick->LOAD+1UL)/(1000000/TICK_PER_SECOND);        //1us下的tick数，F1 SYSTICK HCLK为时钟源，72mhz，1us为72次
  uint32_t tk = SysTick->VAL;            //当前计数值，倒计数
  uint32_t us = (SysTick->LOAD-tk)/usTick;        //经过的us时间
    
  return (tick_get()*(1000000/TICK_PER_SECOND))+us;
}

//获取系统的运行时间，单位ms
uint32_t millis(void)
{
  return (tick_get()*(1000/TICK_PER_SECOND));
}
