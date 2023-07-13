#include "systick.h"
#include "soft_timer.h"

static volatile uint32_t tick = 0;

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

// 微秒延时函数
// us不要超过32位数值的范围
void hw_us_delay(unsigned int us)
{
  unsigned int start=0, now=0, reload=0, us_tick=0 ,tcnt=0 ;
  start = SysTick->VAL;                       //systick的当前计数值（起始值）
  reload = SysTick->LOAD;                     //systick的重载值
  us_tick = SystemCoreClock / 1000000UL;   //1us下的systick计数值，SystemCoreClock为1s下的计数值，需要配置systick时钟源为HCLK
  do {
    now = SysTick->VAL;                       //systick的当前计数值
    if(now!=start){
      tcnt += now < start ? start - now : reload - now + start;    //获取当前systick经过的tick数量，统一累加到tcnt中
      start=now;                              //重新获取systick的当前计数值（起始值）
    }
  } while(tcnt < us_tick * us);               //如果超出，延时完成
}

// 毫秒延时
void hw_ms_delay(unsigned int ms)
{
  uint32_t start_ms= millis();
  while(millis()- start_ms < ms);
}
