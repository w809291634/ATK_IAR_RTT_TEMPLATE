#include "LED/led_drv.h"
#include <rtthread.h>
#include <stdio.h>
#include <string.h>

void led_thread_entry(void *param)
{
  while(1)
  {
    led_ctrl(1);
    rt_thread_mdelay(50);
    led_ctrl(0);
    rt_thread_mdelay(3000);
  }
}
