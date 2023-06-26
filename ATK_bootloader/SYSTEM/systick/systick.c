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

//��ȡϵͳ������ʱ�䣬��λus
uint32_t micros(void)
{
  uint32_t usTick = (SysTick->LOAD+1UL)/(1000000/TICK_PER_SECOND);        //1us�µ�tick����F1 SYSTICK HCLKΪʱ��Դ��72mhz��1usΪ72��
  uint32_t tk = SysTick->VAL;            //��ǰ����ֵ��������
  uint32_t us = (SysTick->LOAD-tk)/usTick;        //������usʱ��
    
  return (tick_get()*(1000000/TICK_PER_SECOND))+us;
}

//��ȡϵͳ������ʱ�䣬��λms
uint32_t millis(void)
{
  return (tick_get()*(1000/TICK_PER_SECOND));
}
