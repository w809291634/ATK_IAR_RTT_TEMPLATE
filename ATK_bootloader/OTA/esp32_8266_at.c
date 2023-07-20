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
* 本文件兼容ESP8266 ESP32  推荐使用8266
********************************************/

// 复位引脚 PF6
#define ESP_RESET_GPIO              GPIOF
#define ESP_RESET_RCC               RCC_AHB1Periph_GPIOF
#define ESP_RESET_PIN               GPIO_Pin_6
// 连接服务器
#define RECV_CMD_BUF_SIZE           256 
#define ESP_RES                     "ESP-->"
#define ESP_SEND                    "ESP<--"
#define CMD_OK                      {esp_flag |= BIT_3;}
#define CMD_SENDED                  {esp_flag &= ~BIT_3;}
#define MAX_ERROR_NUM               3  // 命令重试次数

static char recv_buf[RECV_CMD_BUF_SIZE];
static char send_cmd_count;         // 发送命令的计数
static char esp_flag;               // esp的标志位 
                                    // 位0 进行连接服务器
                                    // 位1 备用
                                    // 位2 备用
                                    // 位3 0：当前命令正在处理 1：处理结束  
                                    // 位4 备用
char esp_link;                      // 已经连接服务器并进入透传
static char err_times;              // 命令发生错误的重试次数
static const char* cmd_list[]={ "ATE0","AT+CWMODE","AT+CWJAP","AT+CIPMODE","AT+CIPSTART","AT+CIPSEND"};
static char current_cmd[100]={0};   // 存储当前错误

// esp发送命令
static void esp_send_cmd(const char * strbuf, unsigned short len)
{
  debug_at(ESP_SEND"%s",strbuf);
  usart3_puts(strbuf,len);
}

// esp 退出透传模式
static void esp_exit_transparent(void)
{
  esp_send_cmd("+++",3);
  debug_at("\r\n");
  esp_link=0;
  LED1=1;
}

// 发送命令处理系统复位
static void esp_cmd_handle_reset()
{
  // 清理控制位
  esp_flag=0;
  send_cmd_count=0;
  // 停止命令响应超时定时器
  softTimer_stop(ESP_TIMEOUT_TIMER_ID);
  memset(recv_buf,0,RECV_CMD_BUF_SIZE);
  
  // 清除错误次数
  err_times=0;
  
  // 可以发送一次命令
  CMD_OK;
}

// esp 连接服务器并进入透传
static int esp_connect_server_handle(void)
{
  // 控制位 
  if(!(esp_flag & BIT_0))return 0;
  
  // 命令能够发送标志位
  if(!(esp_flag & BIT_3))return 0;
  
  // 开始发送命令
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
      softTimer_start(ESP_TIMEOUT_TIMER_ID,17000);    // 默认连接超时为15s
    }break;
    /* AT+CIPMODE=1 */
    case 3:{
      send_cmd_count=send_cmd_count%ARRAY_SIZE(cmd_list);
      sprintf(current_cmd,"%s=1\r\n",(char*)cmd_list[send_cmd_count]);
      esp_send_cmd(current_cmd,strlen(current_cmd));
      softTimer_start(ESP_TIMEOUT_TIMER_ID,1000);
    }break;  
    /* AT+CIPSTART 进行tcp连接 */
    case 4:{
      send_cmd_count=send_cmd_count%ARRAY_SIZE(cmd_list);
      sprintf(current_cmd,"%s=\"TCP\",\"%s\",%d\r\n",(char*)cmd_list[send_cmd_count]
        ,IOT_SERVER_IP,IOT_SERVER_PORT);
      esp_send_cmd(current_cmd,strlen(current_cmd));
      softTimer_start(ESP_TIMEOUT_TIMER_ID,17000);
    }break;
    /* AT+CIPSEND 进入透传模式 */
    case 5:{
      send_cmd_count=send_cmd_count%ARRAY_SIZE(cmd_list);
      sprintf(current_cmd,"%s\r\n",(char*)cmd_list[send_cmd_count]);
      esp_send_cmd(current_cmd,strlen(current_cmd));
      softTimer_start(ESP_TIMEOUT_TIMER_ID,1000);
    }break;
    /* 系统复位 */
    case 6:{
      esp_cmd_handle_reset();
      esp_link=1;
      LED1=0;
      debug_info(INFO"Connected to the server successfully\r\n");
      
      // 可以开始HTTP请求 默认上报二号分区（应用分区）
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

// 处理 esp 的回复
static void esp_reply_analysis(char* valid_reply)
{
#define CMD_ERR_RETRY           { err_times++; \
                                send_cmd_count--; \
                                if(err_times>=MAX_ERROR_NUM){ \
                                  debug_err(ERR"Attempts Exceeded\r\n"); \
                                  esp_cmd_handle_reset(); \
                                } \
                                else CMD_OK;}
  
  // 处理"OK\r\n" 
  if(strstr(valid_reply, "OK")){   
    // 显示
    debug_at(ESP_RES"%s\r\n",valid_reply);
    // 处理内容 
    if(strstr(current_cmd,"AT+CIPSEND")==NULL){
      CMD_OK;
      err_times=0;
    } 
  }
  // 显示错误
  else if(strstr(valid_reply, "ERROR")){   
    debug_at(ESP_RES"%s\r\n",valid_reply);
    CMD_ERR_RETRY;
  }
  // 处理 AT+CWJAP 连接AP的错误情况
  else if(strstr(valid_reply, "+CWJAP:")){   
    // 显示
    debug_at(ESP_RES"%s\r\n",valid_reply);
    // 处理信息 类似 +CWJAP:4
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
  
  /** 仅仅处理显示 **/
  // 处理 AT+CWJAP 连接AP的正确情况
  else if(strstr(valid_reply, "WIFI CONNECTED")){  
    // 显示
    // 正点原子固件这里回复多了\n,进行去除
    while(1){
      char* p=strchr(valid_reply,'\n');
      if(p==NULL)break;
      p[0]=' '; 
    }
    debug_at(ESP_RES"%s\r\n",valid_reply);
  }
  else if(strstr(valid_reply, "WIFI GOT IP")){  
    // 显示
    debug_at(ESP_RES"%s\r\n",valid_reply);
  }
  else if(strstr(valid_reply, "WIFI DISCONNECT")){  
    // 显示
    debug_at(ESP_RES"%s\r\n",valid_reply);
  }
  // 处理 ATE0
  else if(strstr(valid_reply, "ATE0")){  
    // 显示
    debug_at(ESP_RES"%s\r\n",valid_reply);
  }
  // 处理 ready
  else if(strstr(valid_reply, "ready")){  
    // 显示
    debug_at(ESP_RES"%s\r\n",valid_reply);
    esp_connect_start();            // 主动进行连接
  }
  // 处理 AT+CIPSTART
  else if(strstr(valid_reply, "CONNECT")){  
    // 显示
    debug_at(ESP_RES"%s\r\n",valid_reply);
  }
  // 处理 CLOSED显示
  else if(strstr(valid_reply, "CLOSED")){  
    // 显示
    debug_at(ESP_RES"%s\r\n",valid_reply);
  }
  // 当前回复没有处理
  else{
//    debug_err("reply:%s not process\r\n",valid_reply);
  }
}

// at 命令 处理结果1
int esp_command_handle(char* buf,unsigned short len)
{
  /** 单独处理透传命令 **/
  if(strchr(buf,'>') && strstr(current_cmd,"AT+CIPSEND")){
    leftShiftCharArray(buf,len,1);
    debug_at(ESP_RES">\r\n");
    CMD_OK;
  }
  
  /** 进行过滤 **/
  if(!isASCIIString(buf))
    return -2;
  
  /** 条件判断 **/
  // 当前 指令处理缓存 空间足够
  if(strlen(recv_buf)+len>RECV_CMD_BUF_SIZE){
    debug_err(ERR"recv_cmd_buf not enough space\r\n");
    esp_cmd_handle_reset();
    return -1;
  }
  strncat(recv_buf,buf,len);
  
  /** 处理接收缓存 **/
  char cycle_counts=0;
  while(1){
    char* substring=strstr(recv_buf,"\r\n");
    int vaild_len=strlen(recv_buf);
    if(substring==NULL)break;
    
    // 循环检测
    cycle_counts++;
    if(cycle_counts > 6){
      debug_err(ERR"cycles exceeded,buf:%s,sub:%s\r\n",
                recv_buf,substring);
      esp_cmd_handle_reset();
      return -1;
    }
    
    /** 去除多余字符 **/
    substring[0]=0;                       // 获取一个指令
    char *valid_reply=recv_buf;           // 得到一条有效回复
    int offset=strlen(valid_reply)+2;      
    
    /** 处理有效回复 **/
    if(strlen(valid_reply)>0)
      esp_reply_analysis(valid_reply);

    /** 删除此条有效回复 **/
    leftShiftCharArray(recv_buf,vaild_len,offset);
  }
  return 0;
}

// at 命令 处理结果2 发送命令超时.
// 一般一个AT指令都会有回复，如果没有回复，可能等待时间不够，或者通讯错误。
// 这里不再进行重试，只有回复了才能够重试，重试在 esp_reply_analysis 函数中处理
static void esp_send_cmd_timeout()
{
  esp_cmd_handle_reset();
  debug_war(WARNING"cmd timeout:%s\r\n",current_cmd);
}

// 进行连接
static void _esp_connect_start(void){
  // 首次使用请设置 wifi 连接参数
  if(sys_parameter.parameter_flag!=FLAG_OK){
    debug_err(ERR"sys_parameter error!\r\n");
    return;
  }
  
  // 进行ap连接
  if(!(esp_flag & BIT_0)){
    esp_cmd_handle_reset();
    esp_flag|=BIT_0;
  }
  else{
    debug_info(INFO"ESP connecting\r\n");
  }
}

// 进行一次复位
void esp_reset(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure={0};
  RCC_AHB1PeriphClockCmd(ESP_RESET_RCC, ENABLE);  //使能GPIOF时钟

  //GPIOF9,F10初始化设置
  GPIO_InitStructure.GPIO_Pin = ESP_RESET_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;     //普通输出模式
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;    //推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;      //上拉
  GPIO_Init(ESP_RESET_GPIO, &GPIO_InitStructure); //初始化GPIO
	
	GPIO_ResetBits(ESP_RESET_GPIO,ESP_RESET_PIN);   
  hw_ms_delay(100);
  GPIO_SetBits(ESP_RESET_GPIO,ESP_RESET_PIN); 
}

// esp_at 应用层初始化
void esp_at_app_init(void)
{
  softTimer_create(ESP_TIMEOUT_TIMER_ID,MODE_ONE_SHOT,esp_send_cmd_timeout);
//  softTimer_create(TEST_TIMER_ID,MODE_PERIODIC,esp_connect_ap_start);
//  softTimer_start(TEST_TIMER_ID,1000);
  esp_cmd_handle_reset();     // 首次复位
}

// esp_at 应用周期循环函数
void esp_at_app_cycle(void)
{
  esp_connect_server_handle();    // 控制连接esp连接服务器
  esp_usart_data_handle();        // 周期处理串口数据
}

// esp 开始连接 ap
void esp_connect_start(void)
{
  softTimer_create(ESP_CONNECT_ID,MODE_ONE_SHOT,_esp_connect_start);
  
  if(esp_link){
    // 退出透传模式,再次连接
    esp_exit_transparent();
    softTimer_start(ESP_CONNECT_ID,1000);
  }else _esp_connect_start();
}
