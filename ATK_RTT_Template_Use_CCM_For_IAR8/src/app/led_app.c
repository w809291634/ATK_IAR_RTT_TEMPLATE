#include "LED/led_drv.h"
#include <rtthread.h>
#include <stdio.h>
#include <string.h>

static rt_mutex_t mutex = RT_NULL;

void led_thread_entry(void *param)
{
  mutex = rt_mutex_create("mutex", RT_IPC_FLAG_PRIO);
  while(1)
  {
    rt_mutex_take(mutex, RT_WAITING_FOREVER);     // 测试使用
    led_ctrl(1);
    rt_thread_mdelay(50);
    led_ctrl(0);
    rt_thread_mdelay(3000);
    rt_mutex_release(mutex);
  }
}
