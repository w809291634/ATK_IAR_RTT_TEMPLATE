#include "esp32_at.h"
#include "usart3.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "systick.h"
#include "soft_timer.h"
#include "sys.h"
#include "user_cmd.h"

#define RECV_CMD_BUF_SIZE           256
#define ESP32_RES                   "ESP32-->"
#define ESP32_SEND                  "ESP32<--"
#define TARGET                      "ESP32"
#define CMD_OK                      {esp32_flag |= BIT_3;}
#define CMD_SENDED                  {esp32_flag &= ~BIT_3;}
static char recv_cmd_buf[RECV_CMD_BUF_SIZE];
static char send_cmd_count;        // ��������ļ���
static short esp32_flag;           // esp32�ı�־λ 
                                   // λ0 ��������AP
                                   // λ1 ����
                                   // λ2 ����
                                   // λ3 0����ǰ�������ڴ��� 1���������  
                                   // λ4 ����
static const char* cmd_list[]={"ATE0\r\n","AT+CWSTATE?\r\n","AT+CWMODE","AT+CWJAP",};
static const char* ReplyOk_cmd_list[]={"ATE0","AT+CWMODE",};
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
  // �������λ
  esp32_flag=0;
  send_cmd_count=0;
  // ֹͣ������Ӧ��ʱ��ʱ��
  softTimer_stop(ESP32_TIMEOUT_TIMER_ID);
  memset(recv_cmd_buf,0,RECV_CMD_BUF_SIZE);
  
  // ���Է���һ������
  CMD_OK;
}

// esp32 ����AP�Ĵ�����
static void esp32_connect_ap_handle(void)
{
  // ����λ 
  if(!(esp32_flag & BIT_0))return;
  
  // �����ܹ����ͱ�־λ
  if(!(esp32_flag & BIT_3))return;
  
  // ��ʼ��������
  memset(current_cmd,0,strlen(current_cmd));
  switch(send_cmd_count){
    case 0:{
      /* ATE0 */
      send_cmd_count=send_cmd_count%ARRAY_SIZE(cmd_list);
      strcpy(current_cmd,(char*)cmd_list[send_cmd_count]);
      esp32_send_cmd(current_cmd,strlen(current_cmd));
      softTimer_start(ESP32_TIMEOUT_TIMER_ID,1000);
    }break;
    case 1:{
      /* AT+CWSTATE? ��ѯ��ǰ��û������wifi */
      send_cmd_count=send_cmd_count%ARRAY_SIZE(cmd_list);
      strcpy(current_cmd,(char*)cmd_list[send_cmd_count]);
      esp32_send_cmd(current_cmd,strlen(current_cmd));
      softTimer_start(ESP32_TIMEOUT_TIMER_ID,1000);
    }break;
    case 2:{
      /* AT+CWMODE=1 */
      send_cmd_count=send_cmd_count%ARRAY_SIZE(cmd_list);
      sprintf(current_cmd,"%s=1\r\n",(char*)cmd_list[send_cmd_count]);
      esp32_send_cmd(current_cmd,strlen(current_cmd));
      softTimer_start(ESP32_TIMEOUT_TIMER_ID,1000);
    }break;
    case 3:{
      /* AT+CWJAP */
      send_cmd_count=send_cmd_count%ARRAY_SIZE(cmd_list);
      sprintf(current_cmd,"%s=\"%s\",\"%s\"\r\n",(char*)cmd_list[send_cmd_count],
              sys_parameter.wifi_ssid,sys_parameter.wifi_pwd);
      esp32_send_cmd(current_cmd,strlen(current_cmd));
      softTimer_start(ESP32_TIMEOUT_TIMER_ID,16000);    // Ĭ�����ӳ�ʱΪ15s
    }break;
    case 4:{
      esp32_cmd_handle_reset();
      debug_info(INFO"CONNECT AP SUCCESS!\r\n");
    }
    return;
    default:{
      esp32_cmd_handle_reset();
      debug_err(ERR"instruction exceeeded!\r\n");
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

// ����char����
void leftShiftCharArray(char* arr, int size, int shiftAmount) {
  memmove(arr, arr + shiftAmount, (size - shiftAmount) * sizeof(char));
  memset(arr + size - shiftAmount, 0, shiftAmount * sizeof(char));
}

// at ���� ������1
int esp32_command_handle(const char* buf,unsigned short len)
{
#define REPLY_PROCESSED do{ char* __new_buf=substring+flag_len; \
                          int __new_len=strlen(__new_buf); \
                          my_memcpy(recv_cmd_buf,__new_buf,__new_len);\
                          char* __remain_buf=recv_cmd_buf+__new_len; \
                          memset(__remain_buf,0,strlen(__remain_buf)); \
                        }while(0);
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
    if(cycle_counts > 5){
      debug_err(ERR"recv_cmd_buf processing cycles exceeded,recv_cmd_buf:%s,substring:%s\r\n",
                recv_cmd_buf,substring);
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
    debug("valid_reply:%s\r\n",valid_reply);
    // ����"OK\r\n" 
    if(strstr(valid_reply, "OK")){   
      // ��ʾ
      debug_at(ESP32_RES"%s\r\n",valid_reply);
      // ��������
      if(list_contains_str(current_cmd,(char**)ReplyOk_cmd_list,
            ARRAY_SIZE(ReplyOk_cmd_list)))
      {
        CMD_OK;
      }
    }
    // ��ʾ����
    else if(strstr(valid_reply, "ERROR")){   
      debug_at(ESP32_RES"%s\r\n",valid_reply);
      esp32_cmd_handle_reset();
    }
    // ���� AT+CWJAP ����AP�Ĵ������
    else if(strstr(valid_reply, "+CWJAP:")){   
      // ��ʾ
      debug_at(ESP32_RES"%s\r\n",valid_reply);
      // ������Ϣ ���� +CWJAP:4
      char* _flag="+CWJAP:";
      char info_buf[50]={0};
      int info = atoi(valid_reply+strlen(_flag));
      switch(info){
        case 1:{
          strcpy(info_buf,"connect timeout");
        }break;
        case 2:{
          strcpy(info_buf,"password error");
        }break;
        case 3:{
          strcpy(info_buf,"Unable to find target AP");
        }break;
        case 4:{
          strcpy(info_buf,"connect failed");
        }break;
        default:{
          strcpy(info_buf,"Unknown error occurred");
        }break;
      }
      esp32_cmd_handle_reset();
      debug_err(ERR"AT+CWJAP:%s\r\n",info_buf);
    }
    // ���� AT+CWJAP ����AP����ȷ���
    else if(strstr(valid_reply, "WIFI CONNECTED")){  
      // ��ʾ
      debug_at(ESP32_RES"%s\r\n",valid_reply);
    }
    else if(strstr(valid_reply, "WIFI GOT IP")){  
      // ��ʾ
      debug_at(ESP32_RES"%s\r\n",valid_reply);
      CMD_OK;
    }
    else if(strstr(valid_reply, "WIFI DISCONNECT")){  
      // ��ʾ
      debug_at(ESP32_RES"%s\r\n",valid_reply);
    }
    // ���� ATE0
    else if(strstr(valid_reply, "ATE0")){  
      // ��ʾ
      debug_at(ESP32_RES"%s\r\n",valid_reply);
    }
    // ���� AT+CWSTATE?
    else if(strstr(valid_reply, "+CWSTATE:")){  
      // ��ʾ
      debug_at(ESP32_RES"%s\r\n",valid_reply);
      // ����ظ������� +CWSTATE:2,"Xiaomi_B596"
      char* _flag="+CWSTATE:";
      char* reply = valid_reply+strlen(_flag);
      // �����Ϣ
      int state=0;
      char ssid[50]={0};
      char *token = strtok(reply, ",");
      unsigned char k = 0;
      while(token != NULL)
      {
        k++;
        switch(k)
        {
          case 1: {
            state = atoi(token);
          }break;
          case 2: {
            strcpy(ssid,token);
          }break;
        }
        token = strtok(NULL, ",");
      }
      // ������Ϣ
      if(state==2 && strstr(ssid,sys_parameter.wifi_ssid)){
        send_cmd_count=4;           // �������Ӳ���
        debug_info(INFO"%s connected to the wifi:%s\r\n",TARGET,ssid);
      }
      CMD_OK;
    }
    // ���� ready
    else if(strstr(valid_reply, "ready")){  
      // ��ʾ
      debug_at(ESP32_RES"%s\r\n",valid_reply);
    }
    // ��ǰ�ظ�û�д���
    else{
      debug_err(ERR"valid_reply:%s not processed\r\n",valid_reply);
    }
    
    /** ɾ��������Ч�ظ� **/
    REPLY_PROCESSED;
  }
  return 0;
}

// at ���� ������2 �������ʱ
static void esp32_send_cmd_timeout()
{
  esp32_cmd_handle_reset();
  debug_war(WARNING"send_cmd_timeout\r\n");
}

// esp32_at Ӧ�ò��ʼ��
void esp32_at_app_init(void)
{
  softTimer_create(ESP32_TIMEOUT_TIMER_ID,MODE_ONE_SHOT,esp32_send_cmd_timeout);
//  softTimer_create(TEST_TIMER_ID,MODE_PERIODIC,esp32_connect_ap_start);
//  softTimer_start(TEST_TIMER_ID,1000);
  esp32_cmd_handle_reset();     // �״θ�λ
}

// esp32_at Ӧ������ѭ������
void esp32_at_app_cycle(void)
{
  esp32_connect_ap_handle();    // ��������esp32����AP
  esp32_usart_data_handle();    // ���ڴ���������
}

// esp32 ��ʼ���� ap
void esp32_connect_ap_start(void)
{
  // �״�ʹ�������� wifi ���Ӳ���
  if(sys_parameter.wifi_flag!=FLAG_OK){
    debug_info(INFO"Please use %s cmd set wifi parameter!\r\n",ESP_SET_SSID_PASS_CMD);
    return;
  }
  // ����ap����
  if(!(esp32_flag & BIT_0)){
    esp32_cmd_handle_reset();
    esp32_flag|=BIT_0;
  }
  else{
    debug_info(INFO"ESP32 connecting ap\r\n");
  }
}

