#include "esp32_at.h"
#include "usart3.h"
#include "stdio.h"
#include "string.h"

static char send_cmd_count;        // ��������ļ���
static char recv_cmd_count;        // ��������ļ���
static short esp32_flag;           // esp32�ı�־λ 
                                   // λ0 ��ǰ�������ڴ���
                                   // λ1 ����
                                   // λ2 ����
                                   // λ3 ����
                                   // λ4 ����

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
  // �����ܹ����ͱ�־λ
  if( esp32_flag & BIT_0 )return;
  
  const char* cmd_list[]={"ATE0\r\n","AT+CWMODE",};
  const int time_list[]={10,10,10};
  char* cmd=0;              // ��ǰ���͵�����
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



