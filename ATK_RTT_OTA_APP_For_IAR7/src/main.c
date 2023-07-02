#include <rtthread.h>
#include <rtdevice.h>
#include "board.h"
#include "main.h"
#include "cm_backtrace.h"
#include "cmb_def.h"
#include "LED/led_drv.h"
#include "led_app.h"
#include "flash.h"

sys_parameter_t sys_parameter;
static  rt_thread_t  LEDThread;	
#define HARDWARE_VERSION               "V1.0.0"

void fault_test_by_unalign(void) 
{
    volatile int * SCB_CCR = (volatile int *) 0xE000ED14; // SCB->CCR
    volatile int * p;
    volatile int value;

    *SCB_CCR |= (1 << 3); /* bit3: UNALIGN_TRP. */

    p = (int *) 0x00;
    value = *p;
    rt_kprintf("addr:0x%02X value:0x%08X\r\n", (int) p, value);

    p = (int *) 0x04;
    value = *p;
    rt_kprintf("addr:0x%02X value:0x%08X\r\n", (int) p, value);

    p = (int *) 0x03;
    value = *p;
    rt_kprintf("addr:0x%02X value:0x%08X\r\n", (int) p, value);
}

static void hardware_init(void)
{
  SYS_PARAMETER_READ;
  LED_init();
#ifdef RT_USING_CMBACKTRACE
  cm_backtrace_init("CmBacktrace", HARDWARE_VERSION, CMB_SW_VERSION);       // 初始化调试栈
#endif
}

int main(void)
{
	hardware_init();
//  fault_test_by_unalign();
	LEDThread = rt_thread_create("led" ,led_thread_entry, RT_NULL,
											          1024,	10, 10);
	rt_thread_startup(LEDThread);
  
  while(1){
    rt_kprintf("this is app 1 program!\r\n");
    rt_thread_mdelay(1000);
  }
  return 0;
}


