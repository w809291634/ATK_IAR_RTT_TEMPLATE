#include "ota.h"
#include "usart3.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "sys.h"
#include "user_cmd.h"
#include "jsmn.h"
#include "flash.h"

/** �û����� **/
#define OTA_TASK1                  "OTA_check_version"
#define OTA_TASK2                  "OTA_download"
#define OTA_TASK3                  "OTA_download"
#define OTA_TASK4                  "OTA_download"
#define OTA_TASK5                  "OTA_download"

#define HTTP_DATA_BUF_SIZE          550
#define HTTP_HEADER_BUF_SIZE        50
#define MAX_ERROR_NUM               3  // ���Դ���
#define MAX_TOKENS                  40
#define FRAGMENT_SIZE               512  // ���ݰ���С
#define HTTP_SEND                   "ONENET<--"
#define HTTP_RECV                   "ONENET-->"

static char http_flag;              // ��־λ 
                                    // λ0 1������һ������
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
static char current_request[30];
static char err_times;
static uint8_t* current_list;
static uint8_t post_version_list[]={5,0xff};          // ���Ͱ汾�������б�
static uint8_t ota_update_list[]={1,2,3,4,5,0xff};    // OTA �����������б�
/** ����ظ� **/
// ��ǰ��������ݻ���
static char http_buf[HTTP_DATA_BUF_SIZE]; // HTTP Э������� �� ���մ���buf
static uint16_t http_buf_len;
// �洢״̬����Ϣ
static uint16_t stats_code;               // ״̬��
static uint8_t Content_Type;              // ��������  0��δ֪ 1��json  2:octet-stream
static uint16_t Content_len;              // ���ĳ���
static char file_name[50];                // ���ĳ���
// �洢������Ϣ
static char res_msg[12];
static char target_version[24];
static uint32_t tid;
static uint32_t file_size;
// ���ط�����Ϣ
static uint32_t ota_partition_start;
static uint32_t ota_partition_size;
static uint16_t ota_fragment;
static uint16_t ota_total_fragment;
static uint32_t FlashDestination;

/** json���ݴ��� **/
jsmn_parser parser;
jsmntok_t tokens[MAX_TOKENS];

// esp32�������ݵ�������
static void esp32_send_IOT(const char * strbuf, unsigned short len)
{
  if(esp32_link){
    usart3_puts(strbuf,len);
    debug_ota(HTTP_SEND"%s\r\n",current_request);
  }
}

// ����һ�θ�λ�������ٴη�������
static void http_request_reset()
{
  // ��������
  memset(http_buf,0,HTTP_DATA_BUF_SIZE);
  http_buf_len = 0;
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
  
  // flash ���
  ota_fragment =0;
  FlashDestination =0;
  
  // ֹͣ��Ӧ��ʱ��ʱ��
  softTimer_stop(HTTP_REQUEST_ID);
  
  // ����������
  err_times=0;
  
  // ��Ǹ��½���
  updating=0;
  
  // ���Է���һ������
  REQUEST_OK;
}

// http get ����Ļ�������
// url ��ַ
// Content_Type ��������
// range ��Χ
static void http_get_request(char *url,char* Content_Type,char* range)
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
    if(range!=NULL){
      sprintf(buf,"range:%s\r\n",range);
      strcat(http_request,buf);
    }
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
  // ������������ȡ�ļ���
  else if(strstr(res,"Content-Disposition")){
    char* flag="filename=";
    char* info=strstr(res,flag)+strlen(flag);
    // ��ȡ����
    strcpy(file_name,info);
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
  // ����λ 
  if(!(http_flag & BIT_0))return 0;
  
  /** �����ж� **/
  // ��ǰ ָ����� �ռ��㹻
  if(http_buf_len+len > HTTP_DATA_BUF_SIZE){
    debug_err(ERR"http_buf not enough space\r\n");
    http_system_reset();
    return -1;
  }
  memcpy(http_buf+http_buf_len,buf,len);
  http_buf_len=http_buf_len+len;
  
  // ����һ�� ���� ������� 
  if(http_flag & BIT_4){
    if(http_buf_len == Content_len)
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
    else  http_flag|=BIT_4;               // ��ʼ��������


    /** ɾ��������Ч�ظ� **/
    leftShiftCharArray(http_buf,vaild_len,offset);
    http_buf_len = http_buf_len - offset;
  }
  return 0;
}

// ����������ͼ��汾���� ����1
static void OTA_check_version(void)
{
  if(sys_parameter.parameter_flag!=FLAG_OK){
    debug_err(ERR"sys_parameter error!\r\n");
    return;
  }
  
  char url[150];
  sprintf(url,"http://iot-api.heclouds.com/fuse-ota/%s/check?type=1&version=%s",
      IOT_PRO_ID_NAME,sys_parameter.app2_fw_version);
  http_get_request(url,"application/json",NULL);
}

// �����������ȡ���� ����2
static void OTA_download_package(char* range)
{
  char url[150];
  sprintf(url,"http://iot-api.heclouds.com/fuse-ota/%s/%d/download",
      IOT_PRO_ID_NAME,tid);
  http_get_request(url,"application/json",range);
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
  if(esp32_link!=1) return 0;
  
  // ����λ 
  if(!(http_flag & BIT_0))return 0;
  
  // �����ܹ����ͱ�־λ
  if(!(http_flag & BIT_3))return 0;
  
  uint8_t cmd_id = current_list[request_index];
  char temp[50];
  // ��ʼ��������
  switch(cmd_id){
    case 1:{
      /* OTA_check_version */
      // ��Ƿ���״̬
      if(download_part==1) sys_parameter.app1_flag=APP_ERR;
      else if(download_part==2) sys_parameter.app2_flag=APP_ERR;
      write_sys_parameter();
      
      strcpy(current_request,OTA_TASK1);
      OTA_check_version();
      softTimer_start(HTTP_REQUEST_ID,5000);
    }break;
    case 2:{
      /* ���з������� */
      sprintf(temp,"%d-%d",0+512*ota_fragment,511+512*ota_fragment);
      sprintf(current_request,"%s range:%s",OTA_TASK2,temp);
      OTA_download_package(temp);
      softTimer_start(HTTP_REQUEST_ID,5000);
    }break;
    case 3:{
      /*  */
      
      softTimer_start(HTTP_REQUEST_ID,5000);
    }break;  
    case 4:{
      /*  */

      softTimer_start(HTTP_REQUEST_ID,5000);
    }break;
    case 5:{

      softTimer_start(HTTP_REQUEST_ID,5000);
    }break;
    case 0xff:{
      /* �б�������� */

      debug_info(INFO"CONNECT SUCCESS!\r\n");
    }return 1;
    
    default:{
      http_system_reset();
      debug_err(ERR"instruction exceeeded!\r\n");
    }break;
  }
  REQUEST_SENDED;
  return 0;
}

// ��������
static void http_request_retry(void)
{
  // ����λ 
  if(!(http_flag & BIT_0))return;
  
  err_times++; 
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
  // ����λ 
  if(!(http_flag & BIT_0)) return;
  
  if(!(http_flag & BIT_5)) return;
  
  char temp[50]={0};
  /* ��ʼ������Ӧ���ݴ��� */
  // ���� �����������
  if(strstr(current_request,OTA_TASK1)){
    // ���״̬��
    if(stats_code!=200 || Content_Type!=1){
      http_request_retry(); 
      debug_err(ERR"status error\r\n");
    }
    
    // ��ȡJSON�еĶ�������
    int r = jsmn_parse(&parser, http_buf, strlen(http_buf), tokens, MAX_TOKENS);
    if(r>0){
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
        } 
        else if (jsoneq(http_buf, &tokens[i], "target") == 0) {
          if(len < sizeof(target_version)){
            memcpy(target_version, p, len);target_version[len]='\0';
          }
          else debug_err(ERR"buf not space\r\n");
          i++;
        } 
        else if (jsoneq(http_buf, &tokens[i], "tid") == 0) {
          memcpy(temp, p, len);temp[len]='\0';
          tid = atoi(temp);
          i++;
        } 
        else if (jsoneq(http_buf, &tokens[i], "size") == 0) {
          memcpy(temp, p, len);temp[len]='\0';
          file_size = atoi(temp);
          i++;
        } 
      }
      // ��ʾ������Ϣ
      debug_ota(HTTP_RECV"msg:%s\r\n",res_msg);
      
      // ������Ϣ������һ��
      if(strstr(res_msg,"succ")) {
        /* ��ȡ��Ϣ */
        FlashDestination = ota_partition_start;
        ota_total_fragment = file_size/FRAGMENT_SIZE + ((file_size%FRAGMENT_SIZE)?1:0);
        
        /* ����ַ */
        u32 addrx=ota_partition_start;           // д����ʼ��ַ
        u32 end_addr=ota_partition_start+file_size;   // �����ļ���д����ֹ��ַ����1
        
        if (file_size > ota_partition_size  || 
            addrx < STM32_FLASH_BASE ||       // ��ʼ��ַ���
            addrx % 4 )
        {
          debug_err(ERR"Programming address error\r\n");
          http_system_reset();
        }

        /* ������Ҫ��flash���� */
        FLASH_Unlock();
        FLASH_DataCacheCmd(DISABLE);
        
        while(addrx < end_addr)	
        {
          if(*(vu32*)addrx!=0XFFFFFFFF)           // �з�0XFFFFFFFF�ĵط�,Ҫ�����������,ͬʱ����У��
          {   
            uint16_t Sector_num = STMFLASH_GetFlashSector(addrx);
            if(FLASH_EraseSector(Sector_num,VoltageRange_3)!=FLASH_COMPLETE){   //VCC=2.7~3.6V֮��!!
              FLASH_DataCacheCmd(ENABLE);	
              FLASH_Lock();
              
              debug_err(ERR"Erase flash error!\r\n");
              http_system_reset();
            }
          }else addrx+=4;
        } 
        FLASH_DataCacheCmd(ENABLE);	              // FLASH��������,�������ݻ���
        FLASH_Lock();                             // ����
        
        // ���Է�����һ������
        err_times=0;
        REQUEST_OK;
        request_index++; 
      }else http_request_retry();    
    }else http_request_retry();
  }
  // ������������
  else if(strstr(current_request,OTA_TASK2)){
    if(stats_code!=206 || Content_Type!=2 || FlashDestination==0){
      http_request_retry(); 
      debug_err(ERR"status error\r\n");
    }

    uint32_t RamSource = (uint32_t)http_buf;
    /* ���ݱ�� */
    FLASH_Unlock();	
    FLASH_DataCacheCmd(DISABLE); 
    for (int j = 0;(j < Content_len) && (FlashDestination <  ota_partition_start + file_size);j += 4)
    {
      FLASH_ProgramWord(FlashDestination, *(uint32_t*)RamSource);

      if (*(uint32_t*)FlashDestination != *(uint32_t*)RamSource)
      {
        FLASH_DataCacheCmd(ENABLE);	
        FLASH_Lock();
        // ��̴������¿�ʼ
        http_system_reset();
      }
      FlashDestination += 4;
      RamSource += 4;
    }
    FLASH_DataCacheCmd(ENABLE);	
    FLASH_Lock();
    
    // ������
    if(FlashDestination >= ota_partition_start + file_size){
      debug_ota(HTTP_RECV"Progress 100%%\r\n");
      debug_ota("OTA update Successfully!FW version:%s\r\n",target_version);
      debug("Name: %s \r\n",(char*)file_name);
      debug("Size: %d Bytes\r\n",file_size);
      // д��־
      if(download_part==1) {
        strcpy(sys_parameter.app1_fw_version,target_version);
        sys_parameter.app1_flag=APP_OK;
      }
      else if(download_part==2) {
        sys_parameter.app2_flag=APP_OK;
      }
      write_sys_parameter();
      
      // �������
      REQUEST_OK;
      request_index++; 
      http_system_reset();
    }else{
      // ��������
      ota_fragment++;
      debug_ota(HTTP_RECV"Progress %.2f%%\r\n",(double)ota_fragment / (double)ota_total_fragment * 100);
      REQUEST_OK;
      err_times=0;
    }
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
    http_system_reset();     // �״θ�λ
    current_list=post_version_list;
    http_flag |= BIT_0;
  }
}

// ����һ�� OTA ����
void OTA_update_start(void)
{
  if(esp32_link!=1)return ;
    
  // �ж��Ƿ����ڽ��и���
  if(updating==1){
    debug_war(WARNING"updating in progress!\r\n");
    return ;
  }
  
  if(!(http_flag & BIT_0)){
    http_system_reset();     // �״θ�λ
    current_list=ota_update_list;
    
    // ���з����ж�
    if(download_part==1){
      /* ����1 */
      FlashDestination = APP1_START_ADDR;
      ota_partition_start=APP1_START_ADDR;
      ota_partition_size=APP1_SIZE;
    }
    else if(download_part==2){
      /* ����2 */
      FlashDestination = APP2_START_ADDR;
      ota_partition_start=APP2_START_ADDR;
      ota_partition_size=APP2_SIZE;
    }else {
      debug_err(ERR"Please set partition to 1 or 2!\r\n");
      return ;
    }
    
    // ��������
    printf("download partition is app%d,start:0x%08x,size:%d Bytes\r\n",
    download_part,ota_partition_start,ota_partition_size);
    updating =1;
    http_flag |= BIT_0;
  }else{
    debug_info(INFO"HTTP requesting\r\n");
  }
}
