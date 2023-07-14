#include "ota.h"
#include "usart3.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "sys.h"
#include "user_cmd.h"
#include "jsmn.h"

/** �û����� **/
#define HTTP_TASK1                  "OTA_check_version"
#define HTTP_TASK2                  "OTA_download"

#define HTTP_DATA_BUF_SIZE          550
#define HTTP_HEADER_BUF_SIZE        50
#define MAX_ERROR_NUM               3  // ���Դ���
#define MAX_TOKENS                  40


static char http_flag;              // ��־λ 
                                    // λ0 ��ʼһ�δ���
                                    // λ1 ����
                                    // λ2 ����
                                    // λ3 0����ǰ���������ڴ��� 1���������  
                                    // λ4 1����ʼ�������� 
                                    // λ5 1�����Ľ������ 
#define REQUEST_OK                  {http_flag |= BIT_3;}
#define REQUEST_SENDED              {http_flag &= ~BIT_3;}
static char report_ver;           

/** ���Ʒ����߼� **/
static char request_index;          // ���������
static char* current_list;
static char post_version_list[]={5};          // ���Ͱ汾�������б�
static char ota_update_list[]={1,2,3,4,5};    // OTA �����������б�
static char current_request[30];

static char err_times;

/** ����ظ� **/
// ��ǰ��������ݻ���
static char http_buf[HTTP_DATA_BUF_SIZE]; // HTTP Э������� �� ���մ���buf
// �洢״̬����Ϣ
static uint16_t stats_code;               // ״̬��
static uint8_t Content_Type;              // ��������  0��δ֪ 1��json  2:octet-stream
static uint16_t Content_len;              // ���ĳ���
// �洢������Ϣ
static char res_msg[12];
static char target_version[24];
static uint32_t tid;
static uint32_t file_size;

/** json���ݴ��� **/
jsmn_parser parser;
jsmntok_t tokens[MAX_TOKENS];



// ����һ�θ�λ�������ٴη�������
static void http_request_reset()
{
  // ��������
  memset(http_buf,0,HTTP_DATA_BUF_SIZE);
  stats_code = 0;
  Content_Type =0;
  Content_len =0;
  
  // �����Ҫ�ı�־λ
  http_flag &= ~BIT_4;
  http_flag &= ~BIT_5;
}

// ����ϵͳ��λ
static void http_system_reset()
{
  // �������buf �� ����
  http_request_reset();
  
  // �������λ
  http_flag=0;
  request_index =0;
  
  // ֹͣ��Ӧ��ʱ��ʱ��
  softTimer_stop(HTTP_REQUEST_ID);
  
  // ����������
  err_times=0;
  
  // ���Է���һ������
  REQUEST_OK;
}

// http get ����Ļ�������
static void http_get_request(char *url,char* Content_Type)
{
  http_request_reset();
  if(esp32_link){
    char http_request[512]={0};
    char buf[256];
    // ������
    sprintf(buf,"GET %s HTTP/1.1\r\n",url);
    strcat(http_request,buf);
    // Э��ͷ
    sprintf(buf,"Content-Type:%s\r\n",Content_Type);
    strcat(http_request,buf);
    sprintf(buf,"Authorization:%s\r\n",IOT_AUTHORIZATION);
    strcat(http_request,buf);
    sprintf(buf,"host:%s\r\n",IOT_HOST);
    strcat(http_request,buf);
    // ����
    strcat(http_request,"\r\n");
    
    // ���������������
    esp32_send_IOT(http_request,strlen(http_request));
  }else{
    debug_info(INFO"Need to connect server");
  }
}

// http post ����Ļ�������
static void http_post_request(char *url)
{
  http_request_reset();
  if(esp32_link){
  
  }else{
    debug_info(INFO"Need to connect server");
  }
}

// ����һ����Ч�Ļظ�
static void http_response_analysis(char *res)
{
  // ״̬��
  if(strstr(res,"HTTP/1.1")){
    char *token = strtok(res, " ");
    unsigned char k = 0;
    while(token != NULL)
    {
      k++;
      switch(k)
      {
        case 1: break;    // Э��
        case 2: stats_code = atoi(token); break;
        case 3: break;
      }
      token = strtok(NULL, " ");
    }
  }
  // ��������
  else if(strstr(res,"Content-Type")){
    char* flag="Content-Type";
    char* info=strstr(res,flag)+strlen(flag)+1;
    // ��ȡ����
    if(strstr(info,"application/json"))Content_Type=1;
    else if(strstr(info,"application/octet-stream"))Content_Type=2;
    else Content_Type=0;
  }
  // ���ĳ��� Content-Length: 183
  else if(strstr(res,"Content-Length")){
    char* flag="Content-Length";
    char* info=strstr(res,flag)+strlen(flag)+1;
    // ��ȡ����
    Content_len = atoi(info);
  }
  else{
    debug("res:%s not process\r\n",res);
  }
}

// http��������ݽ��
// ����������� ������Ϣ
// ��ESP32 ��Ӧ��ѭ���лص�
int http_data_handle(char *buf,uint16_t len)
{
  /** �����ж� **/
  // ��ǰ ָ����� �ռ��㹻
  if(strlen(http_buf)+len>HTTP_DATA_BUF_SIZE){
    debug_err(ERR"http_buf not enough space\r\n");
    http_request_reset();
    return -1;
  }
  strncat(http_buf,buf,len);
  
  // ����һ�� ���� ������� 
  if(Content_len){
    if(strlen(http_buf)==Content_len)
      http_flag |= BIT_5;
  }
  
  /** �������� **/
  if(http_flag & BIT_4)return 0;
  
  /** ����״̬�к�Э��ͷ **/
  char cycle_counts=0;
  while(!(http_flag & BIT_4)){
    char* substring=strstr(http_buf,"\r\n");
    int vaild_len=strlen(http_buf);
    if(substring==NULL)break;

    /** ѭ����� **/
    cycle_counts++;
    if(cycle_counts > 6){
      debug_err(ERR"cycles exceeded,buf:%s,sub:%s\r\n",
                http_buf,substring);
      http_request_reset();
      return -1;
    }
    
    /** ȥ�������ַ� **/
    substring[0]=0;                       // ��ȡһ��ָ��
    char *valid_reply=http_buf;           // �õ�һ����Ч�ظ�
    int offset=strlen(valid_reply)+2;      

    /** ����״̬�к�Э��ͷ **/
    if(strlen(valid_reply)>0)
      http_response_analysis(valid_reply);
    else http_flag|=BIT_4;               // ��ʼ��������

    /** ɾ��������Ч�ظ� **/
    leftShiftCharArray(http_buf,vaild_len,offset);
  }
  return 0;
}

// ����������ͼ��汾���� ����1
void OTA_check_version(void)
{
  if(sys_parameter.parameter_flag!=FLAG_OK){
    debug_err(ERR"sys_parameter error!");
    return;
  }
  char url[150];
  sprintf(url,"http://iot-api.heclouds.com/fuse-ota/%s/check?type=1&version=%s",
      IOT_PRO_ID_NAME,sys_parameter.app2_fw_version);
  http_get_request(url,"application/json");
  strcpy(current_request,HTTP_TASK1);
}

// ����������͵�ǰ�汾
void OTA_POST_version(void)
{
//  if(sys_parameter.parameter_flag!=FLAG_OK){
//    debug_err(ERR"sys_parameter error!");
//    return;
//  }
//  char url[150];
//  sprintf(url,"http://iot-api.heclouds.com/fuse-ota/%s/check?type=1&version=%s",
//      IOT_PRO_ID_NAME,sys_parameter.app2_fw_version);
//  http_get_request(url,"application/json");
}

// ���� http ����Ĵ�����
// �������� �汾�ϴ� �� OTA���¹̼�
static int http_request_handle(void)
{
  // ����λ 
  if(!(http_flag & BIT_0))return 0;
  
  // �����ܹ����ͱ�־λ
  if(!(http_flag & BIT_3))return 0;
  
  // �ж�������ȷ�� �� ��ȡ
  if(request_index >= strlen(current_list)){
    debug_err(ERR"index error!\r\n");
    return -1;
  }
  char cmd_id = current_list[request_index];
  // ��ʼ��������
  switch(cmd_id){
    case 0:{
      /* OTA_check_version */
      OTA_check_version();
      softTimer_start(HTTP_REQUEST_ID,2000);
    }break;
    case 1:{
      /*  */

      softTimer_start(HTTP_REQUEST_ID,2000);
    }break;
    case 2:{
      /*  */

      softTimer_start(HTTP_REQUEST_ID,2000);    // Ĭ�����ӳ�ʱΪ15s
    }break;
    case 3:{
      /* AT+CIPMODE=1 */
      
      softTimer_start(HTTP_REQUEST_ID,2000);
    }break;  
    case 4:{
      /*  */

      softTimer_start(HTTP_REQUEST_ID,2000);
    }break;
    case 5:{

      softTimer_start(HTTP_REQUEST_ID,2000);
    }break;
    case 6:{

      debug_info(INFO"CONNECT SUCCESS!\r\n");
    }return 1;
    default:{
      http_system_reset();
      debug_err(ERR"instruction exceeeded!\r\n");
    }break;
  }
  REQUEST_SENDED;
  request_index++; 
  return 0;
}

// ��������
static void http_request_retry(void)
{
  err_times++; 
  request_index--; 
  if(err_times>=MAX_ERROR_NUM){ 
    debug_err(ERR"Attempts Exceeded\r\n"); 
    http_system_reset(); 
  } 
  else REQUEST_OK;
}


// ��д��ԭʼjson�����е��ַ����ȽϺ���
static int jsoneq(const char *json, jsmntok_t *tok, const char *s) 
{
  if(tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
    strncmp(json + tok->start, s, tok->end - tok->start) == 0) return 0;
  return -1;
}

// ���� http ����Ӧ
// http ��������1
static void http_respond_handle(void)
{
  if(!(http_flag & BIT_5)) return ;
  
  char temp[50]={0};
  /* ��ʼ������Ӧ���ݴ��� */
  if(strstr(current_request,HTTP_TASK1)){
    // ���� �����������
    int r = jsmn_parse(&parser, http_buf, strlen(http_buf), tokens, MAX_TOKENS);
    if(r>1){
      // ��ȡJSON�еĶ�������
      for (int i = 1; i < r; i++) {
        // ��ȡ JSON ����λ����Ϣ
        char *p =http_buf + tokens[i + 1].start;
        int len = tokens[i + 1].end - tokens[i + 1].start;
        if (jsoneq(http_buf, &tokens[i], "msg") == 0) {
          if(len < sizeof(res_msg)){
            memcpy(res_msg, p, len);res_msg[len]='\0';
          }
          else debug_err(ERR"buf not space\r\n");
          i++;
        } else if (jsoneq(http_buf, &tokens[i], "target") == 0) {
          if(len < sizeof(target_version)){
            memcpy(target_version, p, len);target_version[len]='\0';
          }
          else debug_err(ERR"buf not space\r\n");
          i++;
        } else if (jsoneq(http_buf, &tokens[i], "tid") == 0) {
          memcpy(temp, p, len);temp[len]='\0';
          tid = atoi(temp);
          i++;
        } else if (jsoneq(http_buf, &tokens[i], "size") == 0) {
          memcpy(temp, p, len);temp[len]='\0';
          file_size = atoi(temp);
          i++;
        } 
      }
      // ������Ϣ������һ��
      printf("%s\r\n",res_msg);
      printf("%s\r\n",target_version);
      printf("%d\r\n",tid);
      printf("%d\r\n",file_size);
    }
    else http_request_retry();
  }
  else if(strstr(current_request,HTTP_TASK2)){
  
  
  }
  
  
  // һ�����������
  http_request_reset();
}

// http ����ʱ
// ͨ��һ��������Ӧ��ȽϿ죬��ʱ���������
// http ��������2
static void http_request_timeout()
{
  debug_war(WARNING"<%s> request timeout:\r\n",current_request);
  http_request_retry();
}


// ���� ota ����ϵͳ�ĳ�ʼ��
void OTA_system_init(void)
{
  jsmn_init(&parser);
  softTimer_create(HTTP_REQUEST_ID,MODE_ONE_SHOT,http_request_timeout);
  http_system_reset();     // �״θ�λ
}

// Ӧ�ô���ѭ��
void OTA_system_loop(void)
{
  http_request_handle();
  http_respond_handle();
}

// �����ϱ�һ�ΰ汾
void OTA_report_hw_version(void)
{
  if(!report_ver){
  
    
  }
}
