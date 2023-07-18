#include "ota.h"
#include "usart3.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "sys.h"
#include "user_cmd.h"
#include "jsmn.h"
#include "flash.h"

/** 任务名配置 **/
#define OTA_TASK1                  "OTA_check_update"
#define OTA_TASK2                  "OTA_download"
#define OTA_TASK3                  "OTA_POST_status"
#define OTA_TASK4                  "OTA_check_status"
#define OTA_TASK5                  "OTA_check_version"
#define OTA_TASK6                  "OTA_POST_version"

/** 用户配置 **/
#define HTTP_DATA_BUF_SIZE          550       // 接收 HTTP 数据的缓存区
#define MAX_ERROR_NUM               3         // 重试次数
#define MAX_TOKENS                  40        // 用于解析 json 字符串
#define FRAGMENT_SIZE               512       // 下载固件时，每个数据包大小
#define HTTP_SEND                   "ONENET<--"
#define HTTP_RECV                   "ONENET-->"
#define HTTP_RESPOND                "ONENET responding..."

static char http_flag;              // 标志位 
                                    // 位0 1：进行一次任务
                                    // 位1 备用
                                    // 位2 备用
                                    // 位3 0：当前有请求正在处理 1：处理结束  
                                    // 位4 1：开始接收正文 
                                    // 位5 1：正文接收完成 
#define REQUEST_OK                  {http_flag |= BIT_3;}
#define REQUEST_SENDED              {http_flag &= ~BIT_3;}  

/** 服务器 **/
char IOT_PRO_ID_NAME[]="m5005irgK9/stm32_temperature";
char IOT_SERVER_IP[] ="iot-api.heclouds.com";
char IOT_AUTHORIZATION[]="version=2022-05-01&res=userid%2F315714&et=1720619752&method=sha1&sign=GG%2FUnpuqy6GC4L%2B45enSav3y3jA%3D";
char IOT_HOST[]= "iot-api.heclouds.com";
char IOT_API_URL[]= "https://iot-api.heclouds.com/fuse-ota";

/** 控制发送逻辑 **/
static char request_index;                              // 请求的列表索引，获取不同的执行序列
static char current_request[40];
static char err_times;
static uint8_t* current_list;
static uint8_t post_version_list[]={6,0xff};            // 发送版本的请求列表
static uint8_t ota_update_list[]={1,2,3,4,5,6,0xff};    // OTA 升级的请求列表
static uint8_t onenet_res;                              // 标记 ONENET 处于响应状态
/** 处理回复 **/
// 当前请求的数据缓存
static char http_buf[HTTP_DATA_BUF_SIZE]; // HTTP 协议的正文 和 接收处理buf
static uint16_t http_buf_len;
// 存储状态行信息
static uint16_t stats_code;               // 状态码
static uint8_t Content_Type;              // 正文类型  0：未知 1：json  2:octet-stream
static uint16_t Content_len;              // 正文长度
static char file_name[50];                // 正文长度
// 存储正文信息
static char res_msg[12];                  // 返回的消息
static char fw_version[24];               // 当前的版本
static char target_version[24];           // 当前的升级版本
static uint32_t tid;                      // tid 号码
static uint32_t file_size;                // 文件大小
// 下载分区信息
static uint32_t ota_partition_start;      // 分区起始地址
static uint32_t ota_partition_size;       // 当前分区大小
static uint16_t ota_fragment;             // 当前请求的片段号，一个片段大小为 FRAGMENT_SIZE
static uint16_t ota_total_fragment;       // 总的片段号码
static uint32_t FlashDestination;

/** json数据处理 **/
static jsmn_parser parser;
static jsmntok_t tokens[MAX_TOKENS];

// esp发送数据到服务器
static void esp_send_IOT(const char * strbuf, unsigned short len)
{
  if(esp_link){
    usart3_puts(strbuf,len);
    debug_ota(HTTP_SEND"%s\r\n",current_request);
    onenet_res=1;
  }
}

// 进行一次复位，可以再次发送请求
static void http_request_reset()
{
  // 缓存清理
  memset(http_buf,0,HTTP_DATA_BUF_SIZE);
  http_buf_len = 0;
  stats_code = 0;
  Content_Type =0;
  Content_len =0;
  
  // 清理必要的标志位
  http_flag &= ~BIT_4;
  http_flag &= ~BIT_5;
}

// 请求系统复位
static void http_system_reset()
{
  // 清除接收buf 和 数据
  http_request_reset();
  
  // 清理控制位
  http_flag=0;
  request_index =0;
  
  // flash 编程
  ota_fragment =0;
  FlashDestination =0;
  
  // 停止响应超时定时器
  softTimer_stop(HTTP_REQUEST_ID);
  
  // 清除错误次数
  err_times=0;
  
  // 标记更新结束
  updating=0;
  
  // 可以发送一次请求
  REQUEST_OK;
}

// http get 请求的基础函数
// url 地址
// Content_Type 正文类型
// range 范围
static void http_get_request(char *url,char* Content_Type,char* range)
{
  http_request_reset();
  if(esp_link){
    char http_request[512]={0};
    char buf[256];
    // 请求行
    sprintf(buf,"GET %s HTTP/1.1\r\n",url);
    strcat(http_request,buf);
    // 协议头
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
    // 空行
    strcat(http_request,"\r\n");
    
    // 向服务器发送请求
    esp_send_IOT(http_request,strlen(http_request));
  }else{
    debug_info(INFO"Need to connect server");
  }
}

// http post 请求的基础函数
// Content_Type 正文类型  Content 正文
static void http_post_request(char *url,char* Content_Type,char* Content,int Content_len)
{
  http_request_reset();
  if(esp_link){
    char http_request[512]={0};
    char buf[256];
    // 请求行
    sprintf(buf,"POST %s HTTP/1.1\r\n",url);
    strcat(http_request,buf);
    // 协议头
    sprintf(buf,"Content-Type:%s\r\n",Content_Type);
    strcat(http_request,buf);
    sprintf(buf,"Authorization:%s\r\n",IOT_AUTHORIZATION);
    strcat(http_request,buf);
    sprintf(buf,"host:%s\r\n",IOT_HOST);
    strcat(http_request,buf);
    sprintf(buf,"Content-Length:%d\r\n",Content_len);
    strcat(http_request,buf);
    
    // 空行
    strcat(http_request,"\r\n");
    
    // 正文
    strcat(http_request,Content);
    
    // 向服务器发送请求
    esp_send_IOT(http_request,strlen(http_request));
  }else{
    debug_info(INFO"Need to connect server");
  }
}

/***********************************************
*  http 的请求返回数据处理，处理状态行和接收正文
***********************************************/
// 处理 HTTP 的返回的状态行
static void http_response_analysis(char *res)
{
  // 状态行
  if(strstr(res,"HTTP/1.1")){
    char *token = strtok(res, " ");
    unsigned char k = 0;
    while(token != NULL)
    {
      k++;
      switch(k)
      {
        case 1: break;    // 协议
        case 2: stats_code = atoi(token); break;
        case 3: break;
      }
      token = strtok(NULL, " ");
    }
  }
  // 正文类型
  else if(strstr(res,"Content-Type")){
    char* flag="Content-Type";
    char* info=strstr(res,flag)+strlen(flag)+1;
    // 获取内容
    if(strstr(info,"application/json"))Content_Type=1;
    else if(strstr(info,"application/octet-stream"))Content_Type=2;
    else Content_Type=0;
  }
  // 正文长度 Content-Length: 183
  else if(strstr(res,"Content-Length")){
    char* flag="Content-Length";
    char* info=strstr(res,flag)+strlen(flag)+1;
    // 获取内容
    Content_len = atoi(info);
  }
  // 正文描述,获取文件名
  else if(strstr(res,"Content-Disposition")){
    char* flag="filename=";
    char* info=strstr(res,flag)+strlen(flag);
    // 获取内容
    strcpy(file_name,info);
  }
  else{
    debug("res:%s not process\r\n",res);
  }
}

// http请求的数据结果
// 这里仅仅缓存 返回信息
// 在ESP 的应用循环中回调
int http_data_handle(char *buf,uint16_t len)
{
  if(onenet_res==1){
    onenet_res=0;
    debug_ota(HTTP_RESPOND"\r\n");
  }
  // 控制位 
  if(!(http_flag & BIT_0))return 0;
  
  /** 条件判断 **/
  // 当前 指令处理缓存 空间足够
  if(http_buf_len+len > HTTP_DATA_BUF_SIZE){
    debug_err(ERR"http_buf not enough space\r\n");
    http_system_reset();
    return -1;
  }
  memcpy(http_buf+http_buf_len,buf,len);
  http_buf_len=http_buf_len+len;
  
  // 产生一次 请求 接收完成 
  if(http_flag & BIT_4){
    if(http_buf_len >= Content_len) http_flag |= BIT_5; 
  }
  
  /** 接收正文 **/
  if(http_flag & BIT_4)return 0;
  
  /** 处理状态行和协议头 **/
  char cycle_counts=0;
  while(!(http_flag & BIT_4)){
    char* substring=strstr(http_buf,"\r\n");
    int vaild_len=strlen(http_buf);
    if(substring==NULL)break;

    /** 循环检测 **/
    cycle_counts++;
    if(cycle_counts > 6){
      debug_err(ERR"cycles exceeded,buf:%s,sub:%s\r\n",
                http_buf,substring);
      http_request_reset();
      return -1;
    }
    
    /** 去除多余字符 **/
    substring[0]=0;                       // 获取一个指令
    char *valid_reply=http_buf;           // 得到一条有效回复
    int offset=strlen(valid_reply)+2;      
    
    /** 处理状态行和协议头 **/
    if(strlen(valid_reply)>0)
      http_response_analysis(valid_reply);
    else  http_flag|=BIT_4;               // 开始接收正文

    /** 删除此条有效回复 **/
    leftShiftCharArray(http_buf,vaild_len,offset);
    http_buf_len = http_buf_len - offset;
    
    // 进行一次判断
    if(http_buf_len >= Content_len && http_flag & BIT_4) http_flag |= BIT_5;
  }
  return 0;
}

/***********************************************
*  ONENET 的升级请求任务
***********************************************/
// 向服务器发送检查版本请求 任务1
static void OTA_check_update(void)
{
  char url[150];
  sprintf(url,"%s/%s/check?type=1&version=%s",
      IOT_API_URL,IOT_PRO_ID_NAME,fw_version);
  http_get_request(url,"application/json",NULL);
}

// 向服务器上拉取镜像 任务2
static void OTA_download_package(char* range)
{
  char url[150];
  sprintf(url,"%s/%s/%d/download",
      IOT_API_URL,IOT_PRO_ID_NAME,tid);
  http_get_request(url,"application/json",range);
}

// 向服务器上报升级状态 任务3
static void OTA_POST_status(void)
{
  char url[150];
  char Content[20];
  sprintf(url,"%s/%s/%d/status",
      IOT_API_URL,IOT_PRO_ID_NAME,tid);

  strcpy(Content,"{\"step\":201}");     // 201	升级成功，此时会把设备的版本号修改为任务的目标版本
  http_post_request(url,"application/json",Content,strlen(Content));
}

// 检测升级状态 任务4
static void OTA_check_status(void)
{
  char url[150];
  sprintf(url,"%s/%s/%d/check",
      IOT_API_URL,IOT_PRO_ID_NAME,tid);
  http_get_request(url,"application/json",NULL);
}

// 查看设备版本号 任务5
static void OTA_check_version(void)
{
  char url[150];
  sprintf(url,"%s/%s/version",
      IOT_API_URL,IOT_PRO_ID_NAME);
  http_get_request(url,"application/json",NULL);
}

// 上报设备版本号 任务6
static void OTA_POST_version(void)
{
  char url[150];
  char Content[70];
  // url
  sprintf(url,"%s/%s/version",IOT_API_URL,IOT_PRO_ID_NAME);
  // 正文
  sprintf(Content,"{\"f_version\":\"%s\",\"s_version\":\"V1.0\"}",fw_version);
  http_post_request(url,"application/json",Content,strlen(Content));
}

/***********************************************
*  应用中处理发送 升级 请求的序列
***********************************************/
// 进行 http 请求的处理函数
// 包含进行 版本上传 和 OTA更新固件
static int http_request_handle(void)
{
  if(esp_link!=1) return 0;
  
  // 控制位 
  if(!(http_flag & BIT_0))return 0;
  
  // 命令能够发送标志位
  if(!(http_flag & BIT_3))return 0;
  
  uint8_t cmd_id = current_list[request_index];
  char temp[50];
  // 开始发送命令
  switch(cmd_id){
    /* 检查升级 */
    case 1:{
      // 标记分区状态
      if(download_part==1){
        sprintf(current_request,"%s(%s)",OTA_TASK1,sys_parameter.app1_fw_version);
        sys_parameter.app1_flag=APP_ERR;
      }
      else if(download_part==2){
        sprintf(current_request,"%s(%s)",OTA_TASK1,sys_parameter.app2_fw_version);
        sys_parameter.app2_flag=APP_ERR;
      }
      write_sys_parameter();
      
      OTA_check_update();
      softTimer_start(HTTP_REQUEST_ID,5000);
    }break;
    /* 进行分区下载 */
    case 2:{
      sprintf(temp,"%d-%d",0+512*ota_fragment,511+512*ota_fragment);
      sprintf(current_request,"%s range:%s",OTA_TASK2,temp);
      OTA_download_package(temp);
      softTimer_start(HTTP_REQUEST_ID,5000);
    }break;
    /* 上报升级状态 */
    case 3:{
      strcpy(current_request,OTA_TASK3);
      OTA_POST_status();
      softTimer_start(HTTP_REQUEST_ID,5000);
    }break;  
    /* 检测升级状态 */
    case 4:{
      strcpy(current_request,OTA_TASK4);
      OTA_check_status();
      softTimer_start(HTTP_REQUEST_ID,5000);
    }break;
    /* 查看设备版本号 */
    case 5:{
      strcpy(current_request,OTA_TASK5);
      OTA_check_version();
      softTimer_start(HTTP_REQUEST_ID,5000);
    }break;
    /* 上报设备版本号 */
    case 6:{
      strcpy(current_request,OTA_TASK6);
      OTA_POST_version();
      softTimer_start(HTTP_REQUEST_ID,5000);
    }break;
    /* 列表请求完成 */
    case 0xff:{
      http_system_reset();
      debug_info(INFO"HTTP request sequence completed\r\n");
    }return 1;
    
    default:{
      http_system_reset();
      debug_err(ERR"instruction exceeeded!\r\n");
    }break;
  }
  REQUEST_SENDED;
  return 0;
}

// 进行重试
static void http_request_retry(void)
{
  // 控制位 
  if(!(http_flag & BIT_0))return;
  
  err_times++; 
  if(err_times>=MAX_ERROR_NUM){ 
    debug_err(ERR"Attempts Exceeded\r\n"); 
    http_system_reset(); 
  } 
  else REQUEST_OK;
}

/***********************************************
*  http 请求的正文处理
***********************************************/
// 编写在原始json数据中的字符串比较函数
static int jsoneq(const char *json, jsmntok_t *tok, const char *s) 
{
  if(tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
    strncmp(json + tok->start, s, tok->end - tok->start) == 0) return 0;
  return -1;
}

// 处理 http 的响应
// http 请求的情况1
static void http_respond_handle(void)
{
  /* 条件判断 */
  if(!(http_flag & BIT_0)) return;
  
  if(!(http_flag & BIT_5)) return;
  
  char temp[50]={0};
  
  /* 开始进行响应数据处理 */
  // 处理 检查升级任务
  if(strstr(current_request,OTA_TASK1)){
    // 检查状态行
    if(stats_code!=200 || Content_Type!=1){
      http_request_retry(); 
      debug_err(ERR"status error,code:%d \r\n",stats_code);
    }
    
    // 获取JSON中的对象数据
    jsmn_init(&parser);
    int r = jsmn_parse(&parser, http_buf, http_buf_len, tokens, MAX_TOKENS);
    if(r>0){
      for (int i = 1; i < r; i++) {
        // 获取 JSON 对象位置信息
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
      // 显示接收信息
      debug_ota(HTTP_RECV"msg:%s\r\n",res_msg);
      
      // 根据信息处理下一步
      if(strstr(res_msg,"succ")) {
        /* 获取信息 */
        FlashDestination = ota_partition_start;
        ota_total_fragment = file_size/FRAGMENT_SIZE + ((file_size%FRAGMENT_SIZE)?1:0);
        
        /* 检查地址 */
        u32 addrx=ota_partition_start;                // 写入起始地址
        u32 end_addr=ota_partition_start+file_size;   // 接收文件的写入终止地址，加1
        
        if (file_size > ota_partition_size  || 
            addrx < STM32_FLASH_BASE ||       // 起始地址检查
            addrx % 4 )
        {
          debug_err(ERR"Programming address error\r\n");
          http_system_reset();
        }

        /* 擦除需要的flash部分 */
        FLASH_Unlock();
        FLASH_DataCacheCmd(DISABLE);
        
        while(addrx < end_addr)	
        {
          if(*(vu32*)addrx!=0XFFFFFFFF)           // 有非0XFFFFFFFF的地方,要擦除这个扇区,同时进行校验
          {   
            uint16_t Sector_num = STMFLASH_GetFlashSector(addrx);
            if(FLASH_EraseSector(Sector_num,VoltageRange_3)!=FLASH_COMPLETE){   //VCC=2.7~3.6V之间!!
              FLASH_DataCacheCmd(ENABLE);	
              FLASH_Lock();
              
              debug_err(ERR"Erase flash error!\r\n");
              http_system_reset();
            }
          }else addrx+=4;
        } 
        FLASH_DataCacheCmd(ENABLE);	              // FLASH擦除结束,开启数据缓存
        FLASH_Lock();                             // 上锁
        
        // 可以发送下一次请求
        err_times=0;
        REQUEST_OK;
        request_index++; 
      }
      else if(strstr(res_msg,"not exist"))http_system_reset();
      else http_request_retry();    
    }
  }
  // 处理下载任务
  else if(strstr(current_request,OTA_TASK2)){
    if(stats_code!=206 || Content_Type!=2 || FlashDestination==0){
      http_request_retry(); 
      debug_err(ERR"status error,code:%d \r\n",stats_code);
    }

    uint32_t RamSource = (uint32_t)http_buf;
    /* 数据编程 */
    FLASH_Unlock();	
    FLASH_DataCacheCmd(DISABLE); 
    for (int j = 0;(j < Content_len) && (FlashDestination <  ota_partition_start + file_size);j += 4)
    {
      FLASH_ProgramWord(FlashDestination, *(uint32_t*)RamSource);

      if (*(uint32_t*)FlashDestination != *(uint32_t*)RamSource)
      {
        FLASH_DataCacheCmd(ENABLE);	
        FLASH_Lock();
        // 编程错误，重新开始
        http_system_reset();
      }
      FlashDestination += 4;
      RamSource += 4;
    }
    FLASH_DataCacheCmd(ENABLE);	
    FLASH_Lock();
    
    // 处理结果
    if(FlashDestination >= ota_partition_start + file_size){
      debug_ota(HTTP_RECV"Progress 100%%\r\n");
      debug_info(INFO"APP%d OTA update Successfully!FW version:%s\r\n",download_part,target_version);
      debug("Name: %s \r\n",(char*)file_name);
      debug("Size: %d Bytes\r\n",file_size);
      // 写标志
      if(download_part==1) {
        strcpy(sys_parameter.app1_fw_version,target_version);
        sys_parameter.app1_flag=APP_OK;
      }
      else if(download_part==2) {
        strcpy(sys_parameter.app2_fw_version,target_version);
        sys_parameter.app2_flag=APP_OK;
      }
      write_sys_parameter();
      strcpy(fw_version,target_version);
      
      // 更新完成
      err_times=0;
      request_index++; 
      REQUEST_OK;
    }else{
      // 继续下载
      ota_fragment++;
      debug_ota(HTTP_RECV"Progress %.2f%%\r\n",(double)ota_fragment / (double)ota_total_fragment * 100);
      REQUEST_OK;
      err_times=0;
    }
  }
  // 处理上报升级任务
  else if(strstr(current_request,OTA_TASK3))
  {
    // 检查状态行
    if(stats_code!=200 || Content_Type!=1){
      http_request_retry(); 
      debug_err(ERR"status error,code:%d \r\n",stats_code);
    }
    
    // 获取JSON中的对象数据
    jsmn_init(&parser);
    int r = jsmn_parse(&parser, http_buf, strlen(http_buf), tokens, MAX_TOKENS);
    if(r>0){
      for (int i = 1; i < r; i++) {
        // 获取 JSON 对象位置信息
        char *p =http_buf + tokens[i + 1].start;
        int len = tokens[i + 1].end - tokens[i + 1].start;
        if (jsoneq(http_buf, &tokens[i], "msg") == 0) {
          if(len < sizeof(res_msg)){
            memcpy(res_msg, p, len);res_msg[len]='\0';
          }
          else debug_err(ERR"buf not space\r\n");
          i++;
        } 
      }
      // 显示接收信息
      debug_ota(HTTP_RECV"msg:%s\r\n",res_msg);
      // 根据信息处理下一步
      if(strstr(res_msg,"succ")) {
        // 可以发送下一次请求
        err_times=0;
        request_index++; 
        REQUEST_OK;
      }else http_request_retry();
    }else http_request_retry();
  }
  // 处理 检测升级 任务
  else if(strstr(current_request,OTA_TASK4))
  {
    // 检查状态行
    if(stats_code!=200 || Content_Type!=1){
      http_request_retry(); 
      debug_err(ERR"status error,code:%d \r\n",stats_code);
    }
    
    // 获取JSON中的对象数据
    int _status_code;
    jsmn_init(&parser);
    int r = jsmn_parse(&parser, http_buf, strlen(http_buf), tokens, MAX_TOKENS);
    if(r>0){
      for (int i = 1; i < r; i++) {
        // 获取 JSON 对象位置信息
        char *p =http_buf + tokens[i + 1].start;
        int len = tokens[i + 1].end - tokens[i + 1].start;
        if (jsoneq(http_buf, &tokens[i], "msg") == 0) {
          if(len < sizeof(res_msg)){
            memcpy(res_msg, p, len);res_msg[len]='\0';
          }
          else debug_err(ERR"buf not space\r\n");
          i++;
        } 
        else if (jsoneq(http_buf, &tokens[i], "status") == 0) {
          memcpy(temp, p, len);temp[len]='\0';
          _status_code = atoi(temp);
          i++;
        } 
      }
      // 显示接收信息
      debug_ota(HTTP_RECV"msg:%s\r\n",res_msg);
      // 根据信息处理下一步
      if(strstr(res_msg,"succ") && _status_code==4 ) {
        // 可以发送下一次请求
        err_times=0;
        REQUEST_OK;
        request_index++; 
      }else http_request_retry();
    }else http_request_retry();
  }
  // 处理 查看设备版本号 任务
  else if(strstr(current_request,OTA_TASK5))
  {
    // 检查状态行
    if(stats_code!=200 || Content_Type!=1){
      http_request_retry(); 
      debug_err(ERR"status error,code:%d \r\n",stats_code);
    }
    
    // 获取JSON中的对象数据
    jsmn_init(&parser);
    int r = jsmn_parse(&parser, http_buf, strlen(http_buf), tokens, MAX_TOKENS);
    if(r>0){
      for (int i = 1; i < r; i++) {
        // 获取 JSON 对象位置信息
        char *p =http_buf + tokens[i + 1].start;
        int len = tokens[i + 1].end - tokens[i + 1].start;
        if (jsoneq(http_buf, &tokens[i], "msg") == 0) {
          if(len < sizeof(res_msg)){
            memcpy(res_msg, p, len);res_msg[len]='\0';
          }
          else debug_err(ERR"buf not space\r\n");
          i++;
        }
        else if (jsoneq(http_buf, &tokens[i], "f_version") == 0) {
          memcpy(temp, p, len);temp[len]='\0';
          i++;
        }  
      }
      
      // 显示接收信息
      debug_ota(HTTP_RECV"msg:%s\r\n",res_msg);
      // 根据信息处理下一步
      if(strstr(res_msg,"succ") && strstr(fw_version,temp)) {
        // 可以发送下一次请求
        err_times=0;
        REQUEST_OK;
        request_index++; 
      }else http_request_retry();
    }else http_request_retry();
  }
  // 处理 上报设备版本号 任务
  else if(strstr(current_request,OTA_TASK6))
  {
    // 检查状态行
    if(stats_code!=200 || Content_Type!=1){
      http_request_retry(); 
      debug_err(ERR"status error,code:%d \r\n",stats_code);
    }
    
    // 获取JSON中的对象数据
    jsmn_init(&parser);
    int r = jsmn_parse(&parser, http_buf, strlen(http_buf), tokens, MAX_TOKENS);
    if(r>0){
      for (int i = 1; i < r; i++) {
        // 获取 JSON 对象位置信息
        char *p =http_buf + tokens[i + 1].start;
        int len = tokens[i + 1].end - tokens[i + 1].start;
        if (jsoneq(http_buf, &tokens[i], "msg") == 0) {
          if(len < sizeof(res_msg)){
            memcpy(res_msg, p, len);res_msg[len]='\0';
          }
          else debug_err(ERR"buf not space\r\n");
          i++;
        } 
      }
      // 显示接收信息
      debug_ota(HTTP_RECV"msg:%s\r\n",res_msg);
      // 根据信息处理下一步
      if(strstr(res_msg,"succ")) {
        // 可以发送下一次请求
        err_times=0;
        REQUEST_OK;
        request_index++; 
      }else http_request_retry();
    }else http_request_retry();
  }
  
  /* 一个请求处理结束 */
  http_request_reset();
}

// http 请求超时
// 通常一个请求响应会比较快，超时则进行重试
// http 请求的情况2
static void http_request_timeout()
{
  debug_war(WARNING"%s -- http request timeout:\r\n",current_request);
  http_request_retry();
}

/***********************************************
*  本文件的外部接口
***********************************************/
// 进行 ota 升级系统的初始化
void OTA_system_init(void)
{
  softTimer_create(HTTP_REQUEST_ID,MODE_ONE_SHOT,http_request_timeout);
  http_system_reset();     // 首次复位
}

// 应用处理循环
void OTA_system_loop(void)
{
  http_request_handle();
  http_respond_handle();
}

// 进行一次 OTA 更新
// flag:0上报版本
// 1：进行OTA升级
void OTA_mission_start(char num)
{
  if(esp_link!=1)return ;
    
  // 判断是否正在进行更新
  if(updating==1){
    debug_war(WARNING"updating in progress!\r\n");
    return ;
  }
  
  if(!(http_flag & BIT_0)){
    http_system_reset();     // 首次复位
    // 选择 请求 序列
    if(num) current_list=ota_update_list;
    else current_list=post_version_list;
    
//    /********************测试**************/
//    strcpy(sys_parameter.app1_fw_version,"V1.0");
//    strcpy(sys_parameter.app2_fw_version,"V1.0");
//    write_sys_parameter();
//    /********************测试**************/

    // 进行分区判断
    if(download_part==1){
      /* 分区1 */
      FlashDestination = APP1_START_ADDR;
      ota_partition_start=APP1_START_ADDR;
      ota_partition_size=APP1_SIZE;
      strcpy(fw_version,sys_parameter.app1_fw_version);
    }
    else if(download_part==2){
      /* 分区2 */
      FlashDestination = APP2_START_ADDR;
      ota_partition_start=APP2_START_ADDR;
      ota_partition_size=APP2_SIZE;
      strcpy(fw_version,sys_parameter.app2_fw_version);
    }else {
      debug_err(ERR"Please set partition to 1 or 2!\r\n");
      return ;
    }

    // 启动更新
    if(num==1)
      debug_info(INFO"download partition is app%d,start:0x%08x,size:%d Bytes\r\n",
        download_part,ota_partition_start,ota_partition_size);
    
    updating =1;
    http_flag |= BIT_0;
  }else{
    debug_info(INFO"HTTP requesting\r\n");
  }
}
