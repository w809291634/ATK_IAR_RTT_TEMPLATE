#ifndef __SOFT_TIMER_H__
#define __SOFT_TIMER_H__
#include "sys.h"
#include "stm32f4xx_conf.h"
#include "stdio.h"

/*****************************************************
* 已使用定时器ID
******************************************************/
#define ESP32_TIMEOUT_TIMER_ID    0
#define LED_APP_TIMER_ID          1




#define TEST_TIMER_ID             9

typedef void TIMER_CALLBACK(void);
 
typedef struct softTimer
{
    uint8_t state;          //状态
    uint8_t mode;           //模式
    uint32_t match;         //到期时间
    uint32_t period;        //定时周期
    TIMER_CALLBACK *cb;     //回调函数指针
    void *argv;             //参数指针
    uint16_t argc;          //参数个数
}softTimer;
 
typedef enum tmrState
{
    SOFT_TIMER_STOPPED = 0,  //停止
    SOFT_TIMER_RUNNING,      //运行
    SOFT_TIMER_TIMEOUT       //超时
}tmrState;
 
typedef enum tmrMode
{
    MODE_ONE_SHOT = 0,       //单次模式
    MODE_PERIODIC,           //周期模式
}tmrMode;
 
void tickCnt_Update(void);
uint32_t tickCnt_Get(void);
void softTimer_Init(void);
void soft_timers_init(void);
uint8_t softTimer_GetState(uint16_t id);
void softTimer_create(uint16_t id, tmrMode mode, TIMER_CALLBACK *cb);
void softTimer_start(uint16_t id, uint32_t delay);
void softTimer_stop(uint16_t id);
void softTimer_Update(void);
#endif
