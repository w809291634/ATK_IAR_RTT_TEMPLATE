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

// ΢����ʱ����
// us��Ҫ����32λ��ֵ�ķ�Χ
void hw_us_delay(unsigned int us)
{
  unsigned int start=0, now=0, reload=0, us_tick=0 ,tcnt=0 ;
  start = SysTick->VAL;                       //systick�ĵ�ǰ����ֵ����ʼֵ��
  reload = SysTick->LOAD;                     //systick������ֵ
  us_tick = SystemCoreClock / 1000000UL;   //1us�µ�systick����ֵ��SystemCoreClockΪ1s�µļ���ֵ����Ҫ����systickʱ��ԴΪHCLK
  do {
    now = SysTick->VAL;                       //systick�ĵ�ǰ����ֵ
    if(now!=start){
      tcnt += now < start ? start - now : reload - now + start;    //��ȡ��ǰsystick������tick������ͳһ�ۼӵ�tcnt��
      start=now;                              //���»�ȡsystick�ĵ�ǰ����ֵ����ʼֵ��
    }
  } while(tcnt < us_tick * us);               //�����������ʱ���
}

// ������ʱ
void hw_ms_delay(unsigned int ms)
{
  uint32_t start_ms= millis();
  while(millis()- start_ms < ms);
}
