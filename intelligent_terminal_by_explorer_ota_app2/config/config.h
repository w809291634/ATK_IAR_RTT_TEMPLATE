#ifndef __CONFIG_H__
#define __CONFIG_H__
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
#include "stdio.h"
#include "string.h"
/* Hardware and starter kit includes. */
#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"
/*************全局config文件*************/
  
/*************任务优先级配置*************/
#define ESP32_AT_APP_TASKS_PRIORITY       20
#define KEY_APP_TASKS_PRIORITY            10
#define GUI_APP_TASKS_PRIORITY            30
#define RTC_APP_TASKS_PRIORITY            5
#define CPU_APP_TASKS_PRIORITY            4
/*************ESP32配置*************/
#define CAR_MACADDR     "40:22:d8:eb:49:bc"        	//设置小车的目标地址
//#define CAR_MACADDR     "40:22:d8:eb:4d:d0"       
//#define CAR_MACADDR  ""

//#define CAR_MACADDR     ""        	//设置小车的目标地址
#define ESP32_AT_RECV_SIZE    256
#define ESP32_AT_CMD_TIMEOUT  1000        // AT指令的超时时间  单位 ms
#define STM32_CMD_PROCESSING_CYCLE  10    // STM32处理回复的周期时间 单位ms
#define ESP32_AT_CMD_SHOW_TIME   0
/*************STemWin532*************/
#define PKG_STEMWIN_MEM_SIZE    120									//STemWin532使用的动态内存大小,内存设备需要较大的RAM
#define GUI_NUMBYTES  (PKG_STEMWIN_MEM_SIZE * 1024)
/* Define the average block size */
#define GUI_BLOCKSIZE 0x80                          //块大小

/* 电阻触摸屏的AD定义 校准使用 */
#define TOUCH_AD_LEFT       170
#define TOUCH_AD_RIGHT      3850
#define TOUCH_AD_TOP        265
#define TOUCH_AD_BOTTOM     3950

/* GUI线程 */
#define GUI_TASK_DELAY      5      //GUI主线程延时，单位ms
#define ENABLE_LOCK_SCREEN   0  //是否启动锁屏功能
#define LOCK_DELAY          120       //屏幕没有触发后时间锁定，单位s
#define USE_GUI_EXAMPLE     0       //GUI开启示例

#define STATUS_BAR_HEIGHT   15      //单位像素 

/*************微秒级函数*************/
void hw_us_delay(unsigned int us);    //微秒级延时
unsigned int xtime(void);             //获取系统的运行时间us

/*************中断优先级配置*************/
#define USART1_INT_PRIORITY       6
#define USART3_INT_PRIORITY       6
#define RTC_ALARM_INT_PRIORITY    10
#define CPU_USAGE_TIMER_PRIORITY  6

/*************CLI示例命令*************/
#define CLI_USE_EXAMPLE           0

/*************CPU使用率统计*************/
#define CPU_USAGE_USE_TIM         0
/*************定时器使用*************/
#if(CPU_USAGE_USE_TIM==1)
#define CPU_USAGE_TIMER           TIM6
#define CPU_USAGE_TIMER_RCC       RCC_APB1Periph_TIM6
#define CPU_USAGE_TIMER_IRQ       TIM6_DAC_IRQn
#endif
#endif
