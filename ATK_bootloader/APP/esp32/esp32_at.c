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
#define MAX_ERROR_NUM               2  // 命令重试次数

static char recv_buf[RECV_CMD_BUF_SIZE];
static char send_cmd_count;        // 发送命令的计数
static char esp32_flag;            // esp32的标志位 
                                   // 位0 进行连接服务器
                                   // 位1 备用
                                   // 位2 备用
                                   // 位3 0：当前命令正在处理 1：处理结束  
                                   // 位4 备用
static char esp32_link;            // 已经连接服务器并进入透传
static char err_times;

static const char* cmd_list[]={ "ATE0","AT+CWMODE","AT+CWJAP",        
                                "AT+CIPMODE","AT+CIPSTART","AT+CIPSEND"};
char current_cmd[100]={0};

// esp32发送命令
static void esp32_send_cmd(const char * strbuf, unsigned short len)
{
  debug_at(ESP32_SEND"%s",strbuf);
  usart3_puts(strbuf,len);
}

// esp32 退出透传模式
static void esp32_exit_transparent(void)
{
  esp32_send_cmd("+++",3);
  debug_at("\r\n");
  hw_ms_delay(1000);
  esp32_link=0;
}

// 发送命令处理系统复位
static void esp32_cmd_handle_reset()
{
  // 清理控制位
  esp32_flag=0;
  send_cmd_count=0;
  // 停止命令响应超时定时器
  softTimer_stop(ESP32_TIMEOUT_TIMER_ID);
  memset(recv_buf,0,RECV_CMD_BUF_SIZE);
  
  // 清除错误次数
  err_times=0;
  
  // 可以发送一次命令
  CMD_OK;
}

// esp32 连接服务器并进入透传
static int esp32_connect_server_handle(void)
{
  // 控制位 
  if(!(esp32_flag & BIT_0))return 0;
  
  // 命令能够发送标志位
  if(!(esp32_flag & BIT_3))return 0;
  
  // 开始发送命令
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
      softTimer_start(ESP32_TIMEOUT_TIMER_ID,17000);    // 默认连接超时为15s
    }break;
    case 3:{
      /* AT+CIPMODE=1 */
      send_cmd_count=send_cmd_count%ARRAY_SIZE(cmd_list);
      sprintf(current_cmd,"%s=1\r\n",(char*)cmd_list[send_cmd_count]);
      esp32_send_cmd(current_cmd,strlen(current_cmd));
      softTimer_start(ESP32_TIMEOUT_TIMER_ID,1000);
    }break;  
    case 4:{
      /* AT+CIPSTART 进行tcp连接 */
      send_cmd_count=send_cmd_count%ARRAY_SIZE(cmd_list);
      sprintf(current_cmd,"%s=\"TCP\",\"%s\",%d\r\n",(char*)cmd_list[send_cmd_count]
        ,IOT_SERVER_IP,IOT_SERVER_PORT);
      esp32_send_cmd(current_cmd,strlen(current_cmd));
      softTimer_start(ESP32_TIMEOUT_TIMER_ID,2000);
    }break;
    case 5:{
      /* AT+CIPSEND 进入透传模式 */
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

// 处理 esp32 的回复
static void esp32_reply_analysis(char* valid_reply)
{
#define CMD_ERR_RETRY           { err_times++; \
                                send_cmd_count--; \
                                if(err_times>=MAX_ERROR_NUM)esp32_cmd_handle_reset(); \
                                else CMD_OK;}
  
  // 处理"OK\r\n" 
  if(strstr(valid_reply, "OK")){   
    // 显示
    debug_at(ESP32_RES"%s\r\n",valid_reply);
    // 处理内容 
    if(strstr(current_cmd,"AT+CIPSEND")==NULL)
      CMD_OK;
  }
  // 显示错误
  else if(strstr(valid_reply, "ERROR")){   
    debug_at(ESP32_RES"%s\r\n",valid_reply);
    CMD_ERR_RETRY;
  }
  // 处理 AT+CWJAP 连接AP的错误情况
  else if(strstr(valid_reply, "+CWJAP:")){   
    // 显示
    debug_at(ESP32_RES"%s\r\n",valid_reply);
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
    debug_at(ESP32_RES"%s\r\n",valid_reply);
  }
  else if(strstr(valid_reply, "WIFI GOT IP")){  
    // 显示
    debug_at(ESP32_RES"%s\r\n",valid_reply);
  }
  else if(strstr(valid_reply, "WIFI DISCONNECT")){  
    // 显示
    debug_at(ESP32_RES"%s\r\n",valid_reply);
  }
  // 处理 ATE0
  else if(strstr(valid_reply, "ATE0")){  
    // 显示
    debug_at(ESP32_RES"%s\r\n",valid_reply);
  }
  // 处理 ready
  else if(strstr(valid_reply, "ready")){  
    // 显示
    debug_at(ESP32_RES"%s\r\n",valid_reply);
  }
  // 处理 AT+CIPSTART
  else if(strstr(valid_reply, "CONNECT")){  
    // 显示
    debug_at(ESP32_RES"%s\r\n",valid_reply);
  }
  // 处理 CLOSED显示
  else if(strstr(valid_reply, "CLOSED")){  
    // 显示
    debug_at(ESP32_RES"%s\r\n",valid_reply);
  }
  // 当前回复没有处理
  else{
    debug_err("valid_reply:%s not processed\r\n",valid_reply);
  }
}

// at 命令 处理结果1
int esp32_command_handle(char* buf,unsigned short len)
{
  /** 单独处理透传命令 **/
  if(strchr(buf,'>')){
    leftShiftCharArray(buf,len,1);
    debug_at(ESP32_RES"%c\r\n",'>');
    CMD_OK;
  }

  /** 条件判断 **/
  // 当前 指令处理缓存 空间足够
  if(strlen(recv_buf)+len>RECV_CMD_BUF_SIZE){
    debug_err(ERR"recv_cmd_buf not enough space\r\n");
    esp32_cmd_handle_reset();
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
      debug_err(ERR"recv_cmd_buf processing cycles exceeded,recv_cmd_buf:%s,substring:%s\r\n",
                recv_buf,substring);
      esp32_cmd_handle_reset();
      return -1;
    }
    
    /** 去除多余字符 **/
    substring[0]=0;                       // 获取一个指令
    char *valid_reply=recv_buf;           // 得到一条有效回复
    int offset=strlen(valid_reply)+2;      
    
    /** 处理有效回复 **/
    if(strlen(valid_reply)>0){
      debug("valid_reply:%s\r\n",valid_reply);
      esp32_reply_analysis(valid_reply);
    }

    /** 删除此条有效回复 **/
    leftShiftCharArray(recv_buf,vaild_len,offset);
  }
  return 0;
}

// at 命令 处理结果2 发送命令超时
static void esp32_send_cmd_timeout()
{
  esp32_cmd_handle_reset();
  debug_war(WARNING"cmd timeout:%s\r\n",current_cmd);
}

// esp32_at 应用层初始化
void esp32_at_app_init(void)
{
  softTimer_create(ESP32_TIMEOUT_TIMER_ID,MODE_ONE_SHOT,esp32_send_cmd_timeout);
//  softTimer_create(TEST_TIMER_ID,MODE_PERIODIC,esp32_connect_ap_start);
//  softTimer_start(TEST_TIMER_ID,1000);
  esp32_cmd_handle_reset();     // 首次复位
}

// esp32_at 应用周期循环函数
void esp32_at_app_cycle(void)
{
  esp32_connect_server_handle();    // 控制连接esp32连接AP
  esp32_usart_data_handle();        // 周期处理串口数据
}

// esp32 开始连接 ap
void esp32_connect_start(void)
{
  // 首次使用请设置 wifi 连接参数
  if(sys_parameter.wifi_flag!=FLAG_OK){
    debug_info(INFO"Please use %s cmd set wifi parameter!\r\n",ESP_SET_SSID_PASS_CMD);
    return;
  }
  
  // 进行ap连接
  if(!(esp32_flag & BIT_0)){
    esp32_cmd_handle_reset();
    esp32_flag|=BIT_0;
  }
  else{
    debug_info(INFO"ESP32 connecting ap\r\n");
  }
  
  // 退出透传模式
  if(esp32_link) esp32_exit_transparent();
}
