#include "esp32_at.h"
#include "usart3.h"
#include "stdio.h"
#include "string.h"
#include "systick.h"
#include "soft_timer.h"

static char send_cmd_count;        // 发送命令的计数
static char recv_cmd_count;        // 接收命令的计数
static short esp32_flag;           // esp32的标志位 
                                   // 位0 当前命令正在处理
                                   // 位1 备用
                                   // 位2 备用
                                   // 位3 备用
                                   // 位4 备用

static void esp32_send_cmd_OK()
{
  esp32_flag &= ~BIT_0;
  recv_cmd_count++;
}

static void esp32_send_cmd_reset()
{
  esp32_flag=0;
  send_cmd_count=0;
  recv_cmd_count=0;
}

// 发送命令的超时回调函数，处理
static void esp32_send_cmd_timeout()
{
  esp32_send_cmd_reset();
}

// esp32 连接AP的处理函数
static void esp32_connect_ap_handle(void)
{
  // 命令能够发送标志位
  if( esp32_flag & BIT_0 )return;
  
  const char* cmd_list[]={"ATE0\r\n","AT+CWMODE",};
  const int time_list[]={10,10,10};
  char* cmd=0;              // 当前发送的命令
  switch(send_cmd_count){
    case 0:{
      send_cmd_count=send_cmd_count%ARRAY_SIZE(cmd_list);
      cmd=(char*)cmd_list[send_cmd_count];
      usart3_puts(cmd,strlen(cmd));
      softTimer_start(ESP32_TIMEOUT_TIMER_ID,1000);
    }break;
    case 1:{
    
    }break;
    case 2:{
    
    }break;
    default:{
    
    }break;
  }
  esp32_flag |= BIT_0;
  send_cmd_count++; 
}

// at 命令
void esp32_command_handle(const char* buf,unsigned short len)
{
  printk(buf);
}

// at 应用层初始化
void esp32_at_app_init()
{
  softTimer_create(ESP32_TIMEOUT_TIMER_ID,MODE_ONE_SHOT,esp32_send_cmd_timeout);
}

// at 应用周期循环函数
void esp32_at_app_cycle(void)
{
  esp32_connect_ap_handle();
  esp32_usart_data_handle();
}
