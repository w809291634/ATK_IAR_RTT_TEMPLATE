#include "led.h"
/* FreeRTOS+CLI includes. */
#include "FreeRTOS_CLI.h"
/* Hardware and starter kit includes. */
#include "cm_backtrace.h"
#include "serial.h"
#include "usart.h"
#include "esp32_at.h"
#include "key.h"
#include "apl_key/apl_key.h"
#include "GUI_app/gui_thread.h"
#include "TOUCH/touch.h" 
#include "LCD/lcd.h"
#include "RTC/rtc.h"
#include "TIMER/cpu_usage_timer.h"

/* config */
#include "sys_config.h"
#include "config.h"

#define HARDWARE_VERSION               "V1.0.0"
#define SOFTWARE_VERSION               "V0.1.0"
unsigned volatile char app_cpu_usage=0;
volatile uint32_t CPU_RunTime = 0UL;
sys_parameter_t sys_parameter;

void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName )
{
  xprintf( "OverflowHook in %s \r\n", pcTaskName);  
  while(1);
}

void vApplicationTickHook(void){

}

void vApplicationMallocFailedHook(void){
  xprintf( "vApplicationMallocFailedHook \r\n");  
  while(1);
}

void vApplicationIdleHook(void){

}

///*************微秒延时函数*************/
//us不要超过32位数值的范围
//该函数时候时必须在vTaskStartScheduler之后使用，否则systick没有启动，导致一直在这里循环
void hw_us_delay(unsigned int us)
{
    unsigned int start=0, now=0, reload=0, us_tick=0 ,tcnt=0 ;
    start = SysTick->VAL;                       //systick的当前计数值（起始值）
    reload = SysTick->LOAD;                     //systick的重载值
    us_tick = configCPU_CLOCK_HZ / 1000000UL;   //1us下的systick计数值，configCPU_CLOCK_HZ为1s下的计数值，需要配置systick时钟源为HCLK
    do {
      now = SysTick->VAL;                       //systick的当前计数值
      if(now!=start){
        tcnt += now < start ? start - now : reload - now + start;    //获取当前systick经过的tick数量，统一累加到tcnt中
        start=now;                              //重新获取systick的当前计数值（起始值）
      }
    } while(tcnt < us_tick * us);               //如果超出，延时完成
}

//获取系统的运行时间，单位us
unsigned int xtime(void)
{
  unsigned int time_ms=0;
  unsigned int time_us=0;
  unsigned int now=0, reload=0, us_tick=0 ,systick=0 ;
  us_tick = configCPU_CLOCK_HZ / 1000000UL;   //1us下的systick计数值
  time_ms=xTaskGetTickCount()*portTICK_PERIOD_MS;      //获取当前经过的ms时间
  reload = SysTick->LOAD;                     //systick的重载值
  now = SysTick->VAL;                         //systick的当前计数值
  systick = reload-now;                       //经过的systick值
  time_us=time_ms*1000+systick/us_tick;
  return time_us;
}

void Cpu_task(void *pvParameters)
{
  uint8_t CPU_RunInfo[512];
  vTaskDelay(pdMS_TO_TICKS(2000));
  for(;;)
  {
    memset(CPU_RunInfo,0,512);   
    vTaskGetRunTimeStats((char *)&CPU_RunInfo);
//    xprintf("CPU_RunTime:%d\r\n",CPU_RunTime);
//    xprintf("tasks         cnts    usage\r\n");
//    xprintf("%s", CPU_RunInfo);
//    xprintf("---------------------------------------------\r\n\n");
    app_cpu_usage=99-app_cpu_usage;

    xTaskResetRunTime();
    vTaskDelay(pdMS_TO_TICKS(2000));
    GPIO_ResetBits(GPIOF,GPIO_Pin_9);       //LED0对应引脚GPIOF.9拉低，亮  等同LED0=0;
    vTaskDelay(pdMS_TO_TICKS(50));
    GPIO_SetBits(GPIOF,GPIO_Pin_9);	        //LED0对应引脚GPIOF.0拉高，灭  等同LED0=1;
  }
}

static void prvSetupHardware( void )
{
	SystemInit();																				// 时钟初始化
  NVIC_SetVectorTable(NVIC_VectTab_FLASH,(APP2_START_ADDR - NVIC_VectTab_FLASH));
	NVIC_PriorityGroupConfig( NVIC_PriorityGroup_4 );		// 中断分组
	vRegisterSampleCLICommands();												// 注册命令
  
	vUARTCommandConsoleStart(1024,1);										// CLI任务 ,栈大小，优先级
	cm_backtrace_init("CmBacktrace", HARDWARE_VERSION, SOFTWARE_VERSION);		// 初始化串口后进行
  
  LED_Init();	                                        // 初始化LED控制管脚
  KEY_Init();
  uart3_init(115200);																	// 初始化ESP32 AT设备串口
#if(CPU_USAGE_USE_TIM==1)
  cpu_usage_timer_Init(83,49);
#endif
}
int main(void)
{ 
  /* Configure the hardware ready to run the test. */
	prvSetupHardware();
  xprintf("here\r\n");

  xTaskCreate( app_usart3_Task, "u3_at", 1024, NULL, ESP32_AT_APP_TASKS_PRIORITY, NULL );
	xTaskCreate( app_key_Task, "key", 1024, NULL ,KEY_APP_TASKS_PRIORITY, NULL );
  xTaskCreate( gui_thread_Task, "gui", 2048, NULL, GUI_APP_TASKS_PRIORITY, NULL );
  xTaskCreate( rtc_thread_Task, "rtc", 1024, NULL, RTC_APP_TASKS_PRIORITY, NULL );
  xTaskCreate( Cpu_task, "cpu", 1024, NULL, CPU_APP_TASKS_PRIORITY, NULL );
  
	/* Start the scheduler. */
	vTaskStartScheduler();

	for( ;; );
//	vTaskDelete(NULL);
}


