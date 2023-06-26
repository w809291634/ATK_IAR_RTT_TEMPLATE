#include "esp32_at.h"
#include "usart3.h"
#include "stdio.h"
#include "string.h"
#include "systick.h"
#include "soft_timer.h"

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

static void esp32_send_cmd_reset()
{
  esp32_flag=0;
  send_cmd_count=0;
  recv_cmd_count=0;
}

// ��������ĳ�ʱ�ص�����������
static void esp32_send_cmd_timeout()
{
  esp32_send_cmd_reset();
}

// esp32 ����AP�Ĵ�����
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

// at ����
void esp32_command_handle(const char* buf,unsigned short len)
{
  printk(buf);
}

// at Ӧ�ò��ʼ��
void esp32_at_app_init()
{
  softTimer_create(ESP32_TIMEOUT_TIMER_ID,MODE_ONE_SHOT,esp32_send_cmd_timeout);
}

// at Ӧ������ѭ������
void esp32_at_app_cycle(void)
{
  esp32_connect_ap_handle();
  esp32_usart_data_handle();
}
