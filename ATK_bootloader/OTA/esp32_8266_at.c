#include "esp32_8266_at.h"
#include "usart3.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "systick.h"
#include "soft_timer.h"
#include "sys.h"
#include "user_cmd.h"
#include "led.h"
#include "ota.h"

/********************************************
* ���ļ�����ESP8266 ESP32  �Ƽ�ʹ��8266
********************************************/

// ��λ���� PF6
#define ESP_RESET_GPIO              GPIOF
#define ESP_RESET_RCC               RCC_AHB1Periph_GPIOF
#define ESP_RESET_PIN               GPIO_Pin_6
// ���ӷ�����
#define RECV_CMD_BUF_SIZE           512     // ����8266�ϵ������Ϣ
#define ESP_RES                     "ESP-->"
#define ESP_SEND                    "ESP<--"
#define CMD_OK                      {esp_flag |= BIT_3;}
#define CMD_SENDED                  {esp_flag &= ~BIT_3;}
#define MAX_ERROR_NUM               3  // �������Դ���

static char recv_buf[RECV_CMD_BUF_SIZE];
static char send_cmd_count;         // ��������ļ���
static char esp_flag;               // esp�ı�־λ 
                                    // λ0 �������ӷ�����
                                    // λ1 ����
                                    // λ2 ����
                                    // λ3 0����ǰ�������ڴ��� 1����������  
                                    // λ4 ����
char esp_link;                      // �Ѿ����ӷ�����������͸��
static char err_times;              // �������������Դ���
static const char* cmd_list[]={ "ATE0","AT+CWMODE","AT+CWJAP","AT+CIPMODE","AT+CIPSTART","AT+CIPSEND"};
static char current_cmd[100]={0};   // �洢��ǰ����

// esp��������
static void esp_send_cmd(const char * strbuf, unsigned short len)
{
  debug_at(ESP_SEND"%s",strbuf);
  usart3_puts(strbuf,len);
}

// esp �˳�͸��ģʽ
static void esp_exit_transparent(void)
{
  esp_send_cmd("+++",3);
  debug_at("\r\n");
  esp_link=0;
  LED1=1;
}

// ���������ϵͳ��λ
static void esp_cmd_handle_reset()
{
  // ��������λ
  esp_flag=0;
  send_cmd_count=0;
  // ֹͣ������Ӧ��ʱ��ʱ��
  softTimer_stop(ESP_TIMEOUT_TIMER_ID);
  memset(recv_buf,0,RECV_CMD_BUF_SIZE);
  
  // ����������
  err_times=0;
  
  // ���Է���һ������
  CMD_OK;
}

// esp ���ӷ�����������͸��
static int esp_connect_server_handle(void)
{
  // ����λ 
  if(!(esp_flag & BIT_0))return 0;
  
  // �����ܹ����ͱ�־λ
  if(!(esp_flag & BIT_3))return 0;
  
  // ��ʼ��������
  memset(current_cmd,0,strlen(current_cmd));
  switch(send_cmd_count){
    /* ATE0 */
    case 0:{ 
      send_cmd_count=send_cmd_count%ARRAY_SIZE(cmd_list);
      sprintf(current_cmd,"%s\r\n",(char*)cmd_list[send_cmd_count]);
      esp_send_cmd(current_cmd,strlen(current_cmd));
      softTimer_start(ESP_TIMEOUT_TIMER_ID,1000);
    }break;
    /* AT+CWMODE=1 */
    case 1:{
      send_cmd_count=send_cmd_count%ARRAY_SIZE(cmd_list);
      sprintf(current_cmd,"%s=1\r\n",(char*)cmd_list[send_cmd_count]);
      esp_send_cmd(current_cmd,strlen(current_cmd));
      softTimer_start(ESP_TIMEOUT_TIMER_ID,1000);
    }break;
    /* AT+CWJAP */
    case 2:{
      send_cmd_count=send_cmd_count%ARRAY_SIZE(cmd_list);
      sprintf(current_cmd,"%s=\"%s\",\"%s\"\r\n",(char*)cmd_list[send_cmd_count],
              sys_parameter.wifi_ssid,sys_parameter.wifi_pwd);
      esp_send_cmd(current_cmd,strlen(current_cmd));
      softTimer_start(ESP_TIMEOUT_TIMER_ID,17000);    // Ĭ�����ӳ�ʱΪ15s
    }break;
    /* AT+CIPMODE=1 */
    case 3:{
      send_cmd_count=send_cmd_count%ARRAY_SIZE(cmd_list);
      sprintf(current_cmd,"%s=1\r\n",(char*)cmd_list[send_cmd_count]);
      esp_send_cmd(current_cmd,strlen(current_cmd));
      softTimer_start(ESP_TIMEOUT_TIMER_ID,1000);
    }break;  
    /* AT+CIPSTART ����tcp���� */
    case 4:{
      send_cmd_count=send_cmd_count%ARRAY_SIZE(cmd_list);
      sprintf(current_cmd,"%s=\"TCP\",\"%s\",%d\r\n",(char*)cmd_list[send_cmd_count]
        ,IOT_SERVER_IP,IOT_SERVER_PORT);
      esp_send_cmd(current_cmd,strlen(current_cmd));
      softTimer_start(ESP_TIMEOUT_TIMER_ID,17000);
    }break;
    /* AT+CIPSEND ����͸��ģʽ */
    case 5:{
      send_cmd_count=send_cmd_count%ARRAY_SIZE(cmd_list);
      sprintf(current_cmd,"%s\r\n",(char*)cmd_list[send_cmd_count]);
      esp_send_cmd(current_cmd,strlen(current_cmd));
      softTimer_start(ESP_TIMEOUT_TIMER_ID,1000);
    }break;
    /* ϵͳ��λ */
    case 6:{
      esp_cmd_handle_reset();
      esp_link=1;
      LED1=0;
      debug_info(INFO"Connected to the server successfully\r\n");
      
      // ���Կ�ʼHTTP���� Ĭ���ϱ����ŷ�����Ӧ�÷�����
      download_part=DEFAULT_PARTITION;
      OTA_mission_start(0);
    }return 1;
    default:{
      esp_cmd_handle_reset();
      debug_err(ERR"instruction exceeeded!\r\n");
    }break;
  }
  CMD_SENDED;
  send_cmd_count++; 
  return 0;
}

// ���� esp �Ļظ�
static void esp_reply_analysis(char* valid_reply)
{
#define CMD_ERR_RETRY           { err_times++; \
                                send_cmd_count--; \
                                if(err_times>=MAX_ERROR_NUM){ \
                                  debug_err(ERR"Attempts Exceeded\r\n"); \
                                  esp_cmd_handle_reset(); \
                                } \
                                else CMD_OK;}
  
  // ����"OK\r\n" 
  if(strstr(valid_reply, "OK")){   
    // ��ʾ
    debug_at(ESP_RES"%s\r\n",valid_reply);
    // �������� 
    if(strstr(current_cmd,"AT+CIPSEND")==NULL){
      CMD_OK;
      err_times=0;
    } 
  }
  // ��ʾ����
  else if(strstr(valid_reply, "ERROR")){   
    debug_at(ESP_RES"%s\r\n",valid_reply);
    CMD_ERR_RETRY;
  }
  // ���� AT+CWJAP ����AP�Ĵ������
  else if(strstr(valid_reply, "+CWJAP:")){   
    // ��ʾ
    debug_at(ESP_RES"%s\r\n",valid_reply);
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
    debug_at(ESP_RES"%s\r\n",valid_reply);
  }
  else if(strstr(valid_reply, "WIFI GOT IP")){  
    // ��ʾ
    debug_at(ESP_RES"%s\r\n",valid_reply);
  }
  else if(strstr(valid_reply, "WIFI DISCONNECT")){  
    // ��ʾ
    debug_at(ESP_RES"%s\r\n",valid_reply);
  }
  // ���� ATE0
  else if(strstr(valid_reply, "ATE0")){  
    // ��ʾ
    debug_at(ESP_RES"%s\r\n",valid_reply);
  }
  // ���� ready
  else if(strstr(valid_reply, "ready")){  
    // ��ʾ
    debug_at(ESP_RES"%s\r\n",valid_reply);
    esp_connect_start();        // ������������
  }
  // ���� AT+CIPSTART
  else if(strstr(valid_reply, "CONNECT")){  
    // ��ʾ
    debug_at(ESP_RES"%s\r\n",valid_reply);
  }
  // ���� CLOSED��ʾ
  else if(strstr(valid_reply, "CLOSED")){  
    // ��ʾ
    debug_at(ESP_RES"%s\r\n",valid_reply);
  }
  // ��ǰ�ظ�û�д���
  else{
//    debug_err("reply:%s not process\r\n",valid_reply);
  }
}

// at ���� �������1
int esp_command_handle(char* buf,unsigned short len)
{
  /** ��������͸������ **/
  if(strchr(buf,'>') && strstr(current_cmd,"AT+CIPSEND")){
    leftShiftCharArray(buf,len,1);
    debug_at(ESP_RES">\r\n");
    CMD_OK;
  }

  /** �����ж� **/
  // ��ǰ ָ������� �ռ��㹻
  if(strlen(recv_buf)+len>RECV_CMD_BUF_SIZE){
    debug_err(ERR"recv_cmd_buf not enough space\r\n");
    esp_cmd_handle_reset();
    return -1;
  }
  strncat(recv_buf,buf,len);

  /** �������ջ��� **/
  char cycle_counts=0;
  while(1){
    char* substring=strstr(recv_buf,"\r\n");
    int vaild_len=strlen(recv_buf);
    if(substring==NULL)break;
    
    // ѭ�����
    cycle_counts++;
    if(cycle_counts > 6){
      debug_err(ERR"cycles exceeded,buf:%s,sub:%s\r\n",
                recv_buf,substring);
      esp_cmd_handle_reset();
      return -1;
    }
    
    /** ȥ�������ַ� **/
    substring[0]=0;                       // ��ȡһ��ָ��
    char *valid_reply=recv_buf;           // �õ�һ����Ч�ظ�
    int offset=strlen(valid_reply)+2;      
    
    /** ������Ч�ظ� **/
    if(strlen(valid_reply)>0)
      esp_reply_analysis(valid_reply);

    /** ɾ��������Ч�ظ� **/
    leftShiftCharArray(recv_buf,vaild_len,offset);
  }
  return 0;
}

// at ���� �������2 �������ʱ.
// һ��һ��ATָ����лظ������û�лظ������ܵȴ�ʱ�䲻��������ͨѶ����
// ���ﲻ�ٽ������ԣ�ֻ�лظ��˲��ܹ����ԣ������� esp_reply_analysis �����д���
static void esp_send_cmd_timeout()
{
  esp_cmd_handle_reset();
  debug_war(WARNING"cmd timeout:%s\r\n",current_cmd);
}

// ��������
static void _esp_connect_start(void){
  // �״�ʹ�������� wifi ���Ӳ���
  if(sys_parameter.parameter_flag!=FLAG_OK){
    debug_err(ERR"sys_parameter error!\r\n");
    return;
  }
  
  // ����ap����
  if(!(esp_flag & BIT_0)){
    esp_cmd_handle_reset();
    esp_flag|=BIT_0;
  }
  else{
    debug_info(INFO"ESP connecting\r\n");
  }
}

// ����һ�θ�λ
void esp_reset(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure={0};
  RCC_AHB1PeriphClockCmd(ESP_RESET_RCC, ENABLE);  //ʹ��GPIOFʱ��

  //GPIOF9,F10��ʼ������
  GPIO_InitStructure.GPIO_Pin = ESP_RESET_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;     //��ͨ���ģʽ
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;    //�������
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;      //����
  GPIO_Init(ESP_RESET_GPIO, &GPIO_InitStructure); //��ʼ��GPIO
	
	GPIO_ResetBits(ESP_RESET_GPIO,ESP_RESET_PIN);   
  hw_ms_delay(100);
  GPIO_SetBits(ESP_RESET_GPIO,ESP_RESET_PIN); 
}

// esp_at Ӧ�ò��ʼ��
void esp_at_app_init(void)
{
  softTimer_create(ESP_TIMEOUT_TIMER_ID,MODE_ONE_SHOT,esp_send_cmd_timeout);
//  softTimer_create(TEST_TIMER_ID,MODE_PERIODIC,esp_connect_ap_start);
//  softTimer_start(TEST_TIMER_ID,1000);
  esp_cmd_handle_reset();     // �״θ�λ
}

// esp_at Ӧ������ѭ������
void esp_at_app_cycle(void)
{
  esp_connect_server_handle();    // ��������esp���ӷ�����
  esp_usart_data_handle();        // ���ڴ�����������
}

// esp ��ʼ���� ap
void esp_connect_start(void)
{
  softTimer_create(ESP_CONNECT_ID,MODE_ONE_SHOT,_esp_connect_start);
  
  if(esp_link){
    // �˳�͸��ģʽ,�ٴ�����
    esp_exit_transparent();
    softTimer_start(ESP_CONNECT_ID,1000);
  }else _esp_connect_start();
}