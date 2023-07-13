#include "esp32_at.h"
#include "usart3.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "systick.h"
#include "soft_timer.h"
#include "sys.h"
#include "user_cmd.h"

#define RECV_CMD_BUF_SIZE           128
#define ESP32_RES                   "ESP32-->"
#define ESP32_SEND                  "ESP32<--"
#define TARGET                      "ESP32"
#define CMD_OK                      {esp32_flag |= BIT_3;}
#define CMD_SENDED                  {esp32_flag &= ~BIT_3;}
#define MAX_ERROR_NUM               2  // �������Դ���

static char recv_buf[RECV_CMD_BUF_SIZE];
static char send_cmd_count;        // ��������ļ���
static char esp32_flag;            // esp32�ı�־λ 
                                   // λ0 �������ӷ�����
                                   // λ1 ����
                                   // λ2 ����
                                   // λ3 0����ǰ�������ڴ��� 1���������  
                                   // λ4 ����
static char esp32_link;            // �Ѿ����ӷ�����������͸��
static char err_times;

static const char* cmd_list[]={ "ATE0","AT+CWMODE","AT+CWJAP",        
                                "AT+CIPMODE","AT+CIPSTART","AT+CIPSEND"};
char current_cmd[100]={0};

// esp32��������
static void esp32_send_cmd(const char * strbuf, unsigned short len)
{
  debug_at(ESP32_SEND"%s",strbuf);
  usart3_puts(strbuf,len);
}

// esp32 �˳�͸��ģʽ
static void esp32_exit_transparent(void)
{
  esp32_send_cmd("+++",3);
  debug_at("\r\n");
  hw_ms_delay(1000);
  esp32_link=0;
}

// ���������ϵͳ��λ
static void esp32_cmd_handle_reset()
{
  // �������λ
  esp32_flag=0;
  send_cmd_count=0;
  // ֹͣ������Ӧ��ʱ��ʱ��
  softTimer_stop(ESP32_TIMEOUT_TIMER_ID);
  memset(recv_buf,0,RECV_CMD_BUF_SIZE);
  
  // ����������
  err_times=0;
  
  // ���Է���һ������
  CMD_OK;
}

// esp32 ���ӷ�����������͸��
static int esp32_connect_server_handle(void)
{
  // ����λ 
  if(!(esp32_flag & BIT_0))return 0;
  
  // �����ܹ����ͱ�־λ
  if(!(esp32_flag & BIT_3))return 0;
  
  // ��ʼ��������
  memset(current_cmd,0,strlen(current_cmd));
  switch(send_cmd_count){
    case 0:{
      /* ATE0 */
      send_cmd_count=send_cmd_count%ARRAY_SIZE(cmd_list);
      sprintf(current_cmd,"%s\r\n",(char*)cmd_list[send_cmd_count]);
      esp32_send_cmd(current_cmd,strlen(current_cmd));
      softTimer_start(ESP32_TIMEOUT_TIMER_ID,1000);
    }break;
    case 1:{
      /* AT+CWMODE=1 */
      send_cmd_count=send_cmd_count%ARRAY_SIZE(cmd_list);
      sprintf(current_cmd,"%s=1\r\n",(char*)cmd_list[send_cmd_count]);
      esp32_send_cmd(current_cmd,strlen(current_cmd));
      softTimer_start(ESP32_TIMEOUT_TIMER_ID,1000);
    }break;
    case 2:{
      /* AT+CWJAP */
      send_cmd_count=send_cmd_count%ARRAY_SIZE(cmd_list);
      sprintf(current_cmd,"%s=\"%s\",\"%s\"\r\n",(char*)cmd_list[send_cmd_count],
              sys_parameter.wifi_ssid,sys_parameter.wifi_pwd);
      esp32_send_cmd(current_cmd,strlen(current_cmd));
      softTimer_start(ESP32_TIMEOUT_TIMER_ID,17000);    // Ĭ�����ӳ�ʱΪ15s
    }break;
    case 3:{
      /* AT+CIPMODE=1 */
      send_cmd_count=send_cmd_count%ARRAY_SIZE(cmd_list);
      sprintf(current_cmd,"%s=1\r\n",(char*)cmd_list[send_cmd_count]);
      esp32_send_cmd(current_cmd,strlen(current_cmd));
      softTimer_start(ESP32_TIMEOUT_TIMER_ID,1000);
    }break;  
    case 4:{
      /* AT+CIPSTART ����tcp���� */
      send_cmd_count=send_cmd_count%ARRAY_SIZE(cmd_list);
      sprintf(current_cmd,"%s=\"TCP\",\"%s\",%d\r\n",(char*)cmd_list[send_cmd_count]
        ,IOT_SERVER_IP,IOT_SERVER_PORT);
      esp32_send_cmd(current_cmd,strlen(current_cmd));
      softTimer_start(ESP32_TIMEOUT_TIMER_ID,2000);
    }break;
    case 5:{
      /* AT+CIPSEND ����͸��ģʽ */
      send_cmd_count=send_cmd_count%ARRAY_SIZE(cmd_list);
      sprintf(current_cmd,"%s\r\n",(char*)cmd_list[send_cmd_count]);
      esp32_send_cmd(current_cmd,strlen(current_cmd));
      softTimer_start(ESP32_TIMEOUT_TIMER_ID,1000);
    }break;
    case 6:{
      esp32_cmd_handle_reset();
      esp32_link=1;
      debug_info(INFO"CONNECT SUCCESS!\r\n");
    }return 1;
    default:{
      esp32_cmd_handle_reset();
      debug_err(ERR"instruction exceeeded!\r\n");
    }break;
  }
  CMD_SENDED;
  send_cmd_count++; 
  return 0;
}

// ���� esp32 �Ļظ�
static void esp32_reply_analysis(char* valid_reply)
{
#define CMD_ERR_RETRY           { err_times++; \
                                send_cmd_count--; \
                                if(err_times>=MAX_ERROR_NUM)esp32_cmd_handle_reset(); \
                                else CMD_OK;}
  
  // ����"OK\r\n" 
  if(strstr(valid_reply, "OK")){   
    // ��ʾ
    debug_at(ESP32_RES"%s\r\n",valid_reply);
    // �������� 
    if(strstr(current_cmd,"AT+CIPSEND")==NULL)
      CMD_OK;
  }
  // ��ʾ����
  else if(strstr(valid_reply, "ERROR")){   
    debug_at(ESP32_RES"%s\r\n",valid_reply);
    CMD_ERR_RETRY;
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
    debug_err(ERR"AT+CWJAP:%s\r\n",info_buf);
  }
  
  /** ����������ʾ **/
  // ���� AT+CWJAP ����AP����ȷ���
  else if(strstr(valid_reply, "WIFI CONNECTED")){  
    // ��ʾ
    debug_at(ESP32_RES"%s\r\n",valid_reply);
  }
  else if(strstr(valid_reply, "WIFI GOT IP")){  
    // ��ʾ
    debug_at(ESP32_RES"%s\r\n",valid_reply);
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
  // ���� ready
  else if(strstr(valid_reply, "ready")){  
    // ��ʾ
    debug_at(ESP32_RES"%s\r\n",valid_reply);
  }
  // ���� AT+CIPSTART
  else if(strstr(valid_reply, "CONNECT")){  
    // ��ʾ
    debug_at(ESP32_RES"%s\r\n",valid_reply);
  }
  // ���� CLOSED��ʾ
  else if(strstr(valid_reply, "CLOSED")){  
    // ��ʾ
    debug_at(ESP32_RES"%s\r\n",valid_reply);
  }
  // ��ǰ�ظ�û�д���
  else{
    debug_err("valid_reply:%s not processed\r\n",valid_reply);
  }
}

// at ���� ������1
int esp32_command_handle(char* buf,unsigned short len)
{
  /** ��������͸������ **/
  if(strchr(buf,'>')){
    leftShiftCharArray(buf,len,1);
    debug_at(ESP32_RES"%c\r\n",'>');
    CMD_OK;
  }

  /** �����ж� **/
  // ��ǰ ָ����� �ռ��㹻
  if(strlen(recv_buf)+len>RECV_CMD_BUF_SIZE){
    debug_err(ERR"recv_cmd_buf not enough space\r\n");
    esp32_cmd_handle_reset();
    return -1;
  }
  strncat(recv_buf,buf,len);

  /** ������ջ��� **/
  char cycle_counts=0;
  while(1){
    char* substring=strstr(recv_buf,"\r\n");
    int vaild_len=strlen(recv_buf);
    if(substring==NULL)break;
    
    // ѭ�����
    cycle_counts++;
    if(cycle_counts > 6){
      debug_err(ERR"recv_cmd_buf processing cycles exceeded,recv_cmd_buf:%s,substring:%s\r\n",
                recv_buf,substring);
      esp32_cmd_handle_reset();
      return -1;
    }
    
    /** ȥ�������ַ� **/
    substring[0]=0;                       // ��ȡһ��ָ��
    char *valid_reply=recv_buf;           // �õ�һ����Ч�ظ�
    int offset=strlen(valid_reply)+2;      
    
    /** ������Ч�ظ� **/
    if(strlen(valid_reply)>0){
      debug("valid_reply:%s\r\n",valid_reply);
      esp32_reply_analysis(valid_reply);
    }

    /** ɾ��������Ч�ظ� **/
    leftShiftCharArray(recv_buf,vaild_len,offset);
  }
  return 0;
}

// at ���� ������2 �������ʱ
static void esp32_send_cmd_timeout()
{
  esp32_cmd_handle_reset();
  debug_war(WARNING"cmd timeout:%s\r\n",current_cmd);
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
  esp32_connect_server_handle();    // ��������esp32����AP
  esp32_usart_data_handle();        // ���ڴ���������
}

// esp32 ��ʼ���� ap
void esp32_connect_start(void)
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
  
  // �˳�͸��ģʽ
  if(esp32_link) esp32_exit_transparent();
}
