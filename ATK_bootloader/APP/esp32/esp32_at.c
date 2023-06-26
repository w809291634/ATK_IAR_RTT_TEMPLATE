#include "esp32_at.h"
#include "usart3.h"
#include "stdio.h"
#include "string.h"

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

static void esp32_send_cmd_NOK()
{
  esp32_flag=0;
  send_cmd_count=0;
  recv_cmd_count=0;
}

static void esp32_send_cmd_reset()
{
  esp32_send_cmd_NOK();
}

static void esp32_timer_cycle()
{

}

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






void esp32_command_handle(const char* buf,unsigned short len)
{
  
}

void esp32_at_app_cycle(void)
{
  esp32_connect_ap_handle();
  esp32_usart_data_handle();
}



