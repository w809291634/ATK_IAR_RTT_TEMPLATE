#include "esp32_at.h"
#include "usart3.h"
#include "stdio.h"
#include "string.h"
#include "systick.h"
#include "soft_timer.h"

#define RECV_CMD_BUF_SIZE           256
#define ESP32_RES                   "ESP32-->"
#define ESP32_SEND                  "ESP32<--"
#define CMD_OK                      {esp32_flag |= BIT_0;}
#define CMD_SENDED                  {esp32_flag &= ~BIT_0;}
static char recv_cmd_buf[RECV_CMD_BUF_SIZE];
static char send_cmd_count;        // ��������ļ���
//static char recv_cmd_count;        // ��������ļ���
static short esp32_flag;           // esp32�ı�־λ 
                                   // λ0 0����ǰ�������ڴ��� 1���������  
                                   // λ1 ����
                                   // λ2 ����
                                   // λ3 ����
                                   // λ4 ����
static const char* cmd_list[]={"ATE0\r\n","AT+CWMODE",};
static const char* only_ok_cmd_list[]={"ATE0","AT+CWMODE",};
static char current_cmd[100]={0};

// esp32��������
static void esp32_send_cmd(const char * strbuf, unsigned short len)
{
  debug_at(ESP32_SEND"%s",strbuf);
  usart3_puts(strbuf,len);
}

// ���������ϵͳ��λ
static void esp32_cmd_handle_reset()
{
  esp32_flag=0;
  send_cmd_count=0;
//  recv_cmd_count=0;
  softTimer_stop(ESP32_TIMEOUT_TIMER_ID);
  memset(recv_cmd_buf,0,RECV_CMD_BUF_SIZE);
  
  // �ͷ�һ�����Է�������
  CMD_OK;
}

// esp32 ����AP�Ĵ�����
static void esp32_connect_ap_handle(void)
{
  // �����ܹ����ͱ�־λ
  if( !esp32_flag & BIT_0 )return;
//  if( recv_cmd_count!=send_cmd_count)return;
  
  // ��ʼ��������
  memset(current_cmd,0,strlen(current_cmd));
  switch(send_cmd_count){
    case 0:{
      send_cmd_count=send_cmd_count%ARRAY_SIZE(cmd_list);
      strcpy(current_cmd,(char*)cmd_list[send_cmd_count]);
      esp32_send_cmd(current_cmd,strlen(current_cmd));
      softTimer_start(ESP32_TIMEOUT_TIMER_ID,1000);
    }break;
    case 1:{
      send_cmd_count=send_cmd_count%ARRAY_SIZE(cmd_list);
      sprintf(current_cmd,"%s=1\r\n",(char*)cmd_list[send_cmd_count]);
      esp32_send_cmd(current_cmd,strlen(current_cmd));
      softTimer_start(ESP32_TIMEOUT_TIMER_ID,1000);
    }break;
    case 2:{
      softTimer_start(ESP32_TIMEOUT_TIMER_ID,1000);
    }break;
    default:{
      softTimer_start(ESP32_TIMEOUT_TIMER_ID,1000);
    }break;
  }
  CMD_SENDED;
  send_cmd_count++; 
}


// �б����Ƿ�����ַ���
// obj Ŀ���ַ���  list �ַ����б�  len �б���
static int list_contains_str(char* str,char** list,int len)
{
  for(int i=0;i<len;i++){
    if(strstr(str,list[i]))
      return 1;
  }
  return 0;
}

// at ���� ������1
int esp32_command_handle(const char* buf,unsigned short len)
{
#define REPLY_PROCESSED { char* __new_buf=substring+flag_len; \
                          int __new_len=strlen(__new_buf); \
                          my_memcpy(recv_cmd_buf,__new_buf,__new_len);\
                          char* __remain_buf=recv_cmd_buf+__new_len; \
                          memset(__remain_buf,0,strlen(__remain_buf)); \
                        }
  /** �����ж� **/
  // ��ǰ ָ����� �ռ��㹻
  if(strlen(recv_cmd_buf)+len>RECV_CMD_BUF_SIZE){
    debug_err(ERR"recv_cmd_buf not enough space\r\n");
    esp32_cmd_handle_reset();
    return -1;
  }
  
  /** ׷�ӵ������� **/
  strncat(recv_cmd_buf,buf,len);
  
  /** ������ջ��� **/
  char cycle_counts=0;
  while(1){
    /** ��¼��Ϣ **/
    char* reply_flag="\r\n";
    int flag_len=strlen(reply_flag);  
    char* substring=strstr(recv_cmd_buf,reply_flag);
    debug("recv_cmd_buf:%s\r\n",recv_cmd_buf);
    if(substring==NULL)break;
    // ѭ�����
    cycle_counts++;
    if(cycle_counts > 2){
      debug_err(ERR"recv_cmd_buf processing cycles exceeded \r\n");
      esp32_cmd_handle_reset();
      return -1;
    }
    
    /** ȥ�������ַ� **/
    memset(substring,0,flag_len);             // ɾ�� recv_cmd_buf �е�һ������"\r\n"
    char * valid_reply=recv_cmd_buf;          // �õ�һ����Ч�ظ�
    // �յ�\r\n
    if(strlen(valid_reply)==0){
      REPLY_PROCESSED;
      continue;
    }
    
    /** ������Ч�ظ� **/
    debug("valid_cmd:%s\r\n",valid_reply);
    // ����"OK\r\n" 
    if(strstr(valid_reply, "OK")){   
      // ��ʾ
      debug_at(ESP32_RES"%s\r\n",valid_reply);
      if(list_contains_str(current_cmd,(char**)only_ok_cmd_list,
            ARRAY_SIZE(only_ok_cmd_list)))
      {
        CMD_OK;
//        recv_cmd_count++;
      }
      REPLY_PROCESSED;
    }
    // ��ǰ�ظ�û�д���
    else{
      debug_err(ERR"valid_cmd:%s not processed\r\n",valid_reply);
      REPLY_PROCESSED;
    }
  }
  return 0;
}

// at ���� ������2 �������ʱ
static void esp32_send_cmd_timeout()
{
  esp32_cmd_handle_reset();
}

// at Ӧ�ò��ʼ��
void esp32_at_app_init()
{
  softTimer_create(ESP32_TIMEOUT_TIMER_ID,MODE_ONE_SHOT,esp32_send_cmd_timeout);
  esp32_cmd_handle_reset();   // �״θ�λ
}

// at Ӧ������ѭ������
void esp32_at_app_cycle(void)
{
  esp32_connect_ap_handle();
  esp32_usart_data_handle();    // ���ڴ���������
}
