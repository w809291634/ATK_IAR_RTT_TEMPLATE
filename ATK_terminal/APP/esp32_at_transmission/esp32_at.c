#include "esp32_at.h"
#include "string.h"
#include <stdlib.h>
#include "stdio.h"
/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
/* Hardware and starter kit includes. */
#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"
#include "serial.h"
#include "usart.h"
#include "app_data_protocol.h"
#include "config.h"

#define DEBUG           0x80                                    // 0x80-仅打印无线数据 0x01-打印所有 0x00-不打印

#if (DEBUG & 0x80)
#define debug xprintf
#define debugAt(...)
#else
#if (DEBUG & 0x01)
#define debug xprintf
#define debugAt xprintf
#else
#define debug(...)
#define debugAt(...)
#endif
#endif

#define ERR   "error:"
#define WARN   "warning:"
#define ESP32_STM32 "ESP32 --> STM32  "
#define STM32_ESP32 "STM32 --> ESP32  "
#define ARRAY_LEN(list)  (sizeof(list)/sizeof(list[0]))
extern QueueHandle_t uart3_RxQueue;
// 指令初始化相关
static unsigned char ESP32_ready=0;                   //AT初始化正常
static unsigned char cmd_index=0;                     //cmd_list 索引
static const char cmd_list[][40]={                    //所有的初始化指令
"AT\r\n",
"ATE0\r\n",
"AT+DESTADDR=\""CAR_MACADDR"\"\r\n"
};  

// 数据发送相关
static SemaphoreHandle_t DataSendSemaphore;           //发送命令的互斥量
static char sendcmd[40]={0};                          //当前发送命令指令
static char senddata[ESP32_AT_RECV_SIZE]={0};                        //存储数据发送命令
static char recv_buf[ESP32_AT_RECV_SIZE]={0};

static volatile unsigned char datalen=0;              //发送数据长度

#if(ESP32_AT_CMD_SHOW_TIME==1)
static unsigned int st_time;
static unsigned int en_time;
#endif
static void esp32_at_cmd_init()
{
  if(cmd_index<ARRAY_LEN(cmd_list))
  {   
    strcpy(sendcmd,cmd_list[cmd_index]);            //存储当前发送命令
    USART3_putstring((char *)cmd_list[cmd_index]);  //
    debug(STM32_ESP32"%s",cmd_list[cmd_index]);
  }
}

static void dump_debug_infor(void){
  debug("sendcmd: %s",sendcmd);                   // 打印 当前储存的 命令
  debug("datasendbuf: %s\r\n",senddata);           // 打印 当前储存的 数据
  debug("recv_buf: %s\r\n",recv_buf);                 // 打印 当前储存的 命令返回
  debug("\r\n");
}

// ESP32-->STM32 数据接收处理
void app_usart3_Task( void *pvParameters )
{
  unsigned char i=0,timeout=0;
  char Char;
  DataSendSemaphore=xSemaphoreCreateBinary();   // 由于是两个线程，这里适合使用二值信号量
  xSemaphoreGive(DataSendSemaphore);					//释放一个信号量
  configASSERT(DataSendSemaphore);
  for( ;; )
  {
    if(ESP32_ready!=1){
      if(cmd_index<ARRAY_LEN(cmd_list)) {
        // 发送 准备命令，测试和设置AT通讯
        esp32_at_cmd_init();                    
				timeout++;
				if(timeout>100){
					debug(ERR"Esp32 connection error");
					break;
				}
      }else ESP32_ready=1;
      // 如果ESP32没有准备就绪的话，这里设置为低速循环状态 
      vTaskDelay(100 / portTICK_RATE_MS);           
    }
    //  STM32处理ESP32的回复，这里的周期越小，响应越快
    while(xQueueReceive( uart3_RxQueue, &Char, STM32_CMD_PROCESSING_CYCLE / portTICK_RATE_MS )==pdTRUE) 
    {
      recv_buf[i++]=Char;
//      debugAt("Char: %d\r\n",Char);
      debugAt("Char: %c\r\n",Char);
      //处理具体字符
      if( Char == '>'){
        taskENTER_CRITICAL();
        USART3_putstring(senddata);                           // 发送数据命令
        taskEXIT_CRITICAL();  
#if(ESP32_AT_CMD_SHOW_TIME==1)
        en_time=xtime();
        xprintf("sendtime: %d\r\n",en_time-st_time);
#endif
        goto CLEAN;
      }
      if(recv_buf[i-1]=='\n' && recv_buf[i-2]=='\r')          // 处理一条回复
      {
        recv_buf[i-1]=0;
        recv_buf[i-2]=0;
        if(strlen(recv_buf)>0)
        {
          // 接收ESP32 AT指令准备就绪
          if(memcmp(recv_buf, "ready", 5) == 0) {
            debug("ESP32 Ready!\r\n");
          }
          
          // 处理ESP32发送过来的数据
          else if(memcmp(recv_buf, "+RECV=", 6) == 0)
          {
            debug(ESP32_STM32"%s\r\n",recv_buf);
            //处理接收的APP数据   
          }
          
          // 处理自己发送数据的回复
          else if(memcmp(recv_buf, "+SEND=", 6) == 0)           // 发送数据的回复
          {
            unsigned char s_datalen = atoi(recv_buf + 6);       // 获取发送时无线数据长度
            if(datalen==s_datalen){
              debug(STM32_ESP32"%s\r\n",senddata);           // 打印具体发送的数据
            }else{
              debug(ERR"The length of send data err!\r\n");
              dump_debug_infor();
            }
            xSemaphoreGive(DataSendSemaphore);					        //释放一个信号量    
#if(ESP32_AT_CMD_SHOW_TIME==1)
            en_time=xtime();
            xprintf("ok time: %d\r\n",en_time-st_time);
#endif
          }
          
          // 处理指令回复的OK
          else if(memcmp(recv_buf, "OK", 2) == 0)               
          {
            if(memcmp(sendcmd, cmd_list[0], 4) == 0){           //AT\r\n
              cmd_index++;
              debug("ESP32 AT Ready!\r\n"); 
            }else if(memcmp(sendcmd, cmd_list[1], 6) == 0){     //ATE0\r\n
              cmd_index++;
              debug("ESP32 ATE0 Success!\r\n"); 
            }
            else if(memcmp(sendcmd, cmd_list[2], 12) == 0){     //AT+DESTADDR
              cmd_index++;
              debug("ESP32 set DESTADDR Success!\r\n"); 
            }
          }
          
          // 命令异常
          else if(memcmp(recv_buf, "ERROR", 5) == 0)  {
            debug(ERR"ESP32 AT CMD ERROR!\r\n"); 
            dump_debug_infor();
            xSemaphoreGive(DataSendSemaphore);					        // 释放一个信号量    
          }
          
          // 接收数据为ESP32繁忙 busy p...
          else if(memcmp(recv_buf, "busy", 4) == 0){
            debug(WARN"ESP32 AT busy\r\n"); 
          } 
        } 
CLEAN:
        i=0;
        memset(recv_buf,0,256); 
      }
      if(i==255){
        debug(ERR"Command receiving error!\r\n");
        goto CLEAN;
      }
    }//while
  } //for( ;; )
	vTaskDelete(NULL);
}

// STM32-->ESP32 数据发送处理   按键线程优先级 10   u3at 20
void app_data_send_fun( char* data)
{
  if(ESP32_ready==1){
    if(xSemaphoreTake(DataSendSemaphore,ESP32_AT_CMD_TIMEOUT / portTICK_RATE_MS)==pdTRUE){  //等待时间，如果没有收到回复，只能够自己复位
      //发送命令
#if(ESP32_AT_CMD_SHOW_TIME==1)
      st_time=xtime();                            // 记录指令运行的起始时间
#endif
      taskENTER_CRITICAL();
      strcpy(senddata,data);                      // 储存 当前发送的 数据
      datalen=strlen(senddata);          
      sprintf(sendcmd,"AT+SEND=%d\r\n",datalen);  // 储存 当前发送的 命令          
      USART3_putstring(sendcmd);                  // 发送 准备发送数据命令
      taskEXIT_CRITICAL();
    }else{
      debug(ERR"STM32-->ESP32 Cmd Timeout!\r\n\r\n");  
//      xSemaphoreGive(DataSendSemaphore);					        // 释放一个信号量  ，不能够释放，ESP32-MESH中间扫描时会存在一段时间的空档
    }
  }
}


