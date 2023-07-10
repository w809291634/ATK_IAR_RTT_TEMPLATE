#include "TIMER/cpu_usage_timer.h"
#include "config.h"

extern volatile uint32_t CPU_RunTime ;
#if(CPU_USAGE_USE_TIM==1)
//arr：自动重装值。
//psc：时钟预分频数
//定时器溢出时间计算方法:Tout=((arr+1)*(psc+1))/Ft us.
//定时器6使用 APB1使用源，84MHZ
//psc=83  arr 49  就是50us
void cpu_usage_timer_Init(unsigned short psc,unsigned short arr)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(CPU_USAGE_TIMER_RCC,ENABLE);  
	
  TIM_TimeBaseInitStructure.TIM_Period = arr; 	//自动重装载值
	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;  //定时器分频
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //向上计数模式
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1; //输入捕获时滤波
	
	TIM_TimeBaseInit(CPU_USAGE_TIMER,&TIM_TimeBaseInitStructure);
	
	TIM_ITConfig(CPU_USAGE_TIMER,TIM_IT_Update,ENABLE); 
	TIM_Cmd(CPU_USAGE_TIMER,ENABLE); 
	
	NVIC_InitStructure.NVIC_IRQChannel=CPU_USAGE_TIMER_IRQ; 
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=CPU_USAGE_TIMER_PRIORITY; //抢占优先级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0; 
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

void TIM6_DAC_IRQHandler(void)
{
  if(TIM_GetITStatus(CPU_USAGE_TIMER,TIM_IT_Update)==SET) //溢出中断
  {
    CPU_RunTime++;
  }
  TIM_ClearITPendingBit(CPU_USAGE_TIMER,TIM_IT_Update);  //清除中断标志位
}
#endif
