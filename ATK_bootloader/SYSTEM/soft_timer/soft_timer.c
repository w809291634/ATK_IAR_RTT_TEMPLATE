#include "soft_timer.h"

/*****************************************************
* 用户配置
* tickCnt_Update 放置在systick中断中
* softTimer_Update 放置在大循环中
* softTimer_Init 放置在硬件初始化
******************************************************/
/* 软件定时器数量 */
#define TIMER_NUM           10

static volatile uint32_t tickCnt = 0;   //软件定时器时钟节拍
static softTimer timer[TIMER_NUM];      //软件定时器数组
 
/*****************************************************
* function: 更新时钟节拍
* param:
* return:
* note:     需在定时器中断内执行
******************************************************/
void tickCnt_Update(void)
{
    tickCnt++;
}
 
/*****************************************************
* function: 获取时钟节拍
* param:
* return:   时钟节拍
* note:
******************************************************/
uint32_t tickCnt_Get(void)
{
  return tickCnt;
}
 
/*****************************************************
* function: 软件定时器初始化
* param:
* return:
* note:
******************************************************/
void softTimer_Init(void)
{
    uint16_t i;
 
    for(i=0; i<TIMER_NUM; i++)
    {
        timer[i].state = SOFT_TIMER_STOPPED;
        timer[i].mode = MODE_ONE_SHOT;
        timer[i].match = 0;
        timer[i].period = 0;
        timer[i].cb = NULL;
        timer[i].argv = NULL;
        timer[i].argc = 0;
    }
}
 
/*****************************************************
* function: 获取软件定时器状态
* param:    软件定时器ID
* return:   定时器状态
* note:
******************************************************/
uint8_t softTimer_GetState(uint16_t id)
{
    return timer[id].state;
}
 
void softTimer_create(uint16_t id, tmrMode mode, TIMER_CALLBACK *cb)
{
    assert_param(id < TIMER_NUM);
    assert_param(mode == MODE_ONE_SHOT || mode == MODE_PERIODIC);
    timer[id].state = SOFT_TIMER_STOPPED;
    timer[id].mode = mode;
    timer[id].cb = cb;
}
 
/*****************************************************
* function: 启动软件定时器
* param1:   软件定时器ID
* param2:   定时器模式
* param3:   延时时间(对周期定时器，亦是周期时间)，单位为多少个tick
* param4:   回调函数指针
* param5:   回调函数参数：参数指针
* param6:   回调函数参数：参数长度
* return:   
* note:     
******************************************************/
void softTimer_start(uint16_t id, uint32_t delay)
{
    assert_param(id < TIMER_NUM);
 
    timer[id].match = tickCnt_Get() + delay;
    timer[id].period = delay;
    timer[id].state = SOFT_TIMER_RUNNING;
}
 
/*****************************************************
* function: 停止软件定时器
* param:    软件定时器ID
* return:   
* note:     
******************************************************/
void softTimer_stop(uint16_t id)
{
    assert_param(id < TIMER_NUM);
    timer[id].state = SOFT_TIMER_STOPPED;
}
 
/*****************************************************
* function: 更新软件定时器状态
* param:    
* return:   
* note:     
******************************************************/
void softTimer_Update(void)
{
    uint16_t i;
 
    for(i=0; i<TIMER_NUM; i++)
    {
        switch (timer[i].state)
        {
        case SOFT_TIMER_STOPPED:
            break;
 
        case SOFT_TIMER_RUNNING:
            if(timer[i].match <= tickCnt_Get())
            {
                timer[i].state = SOFT_TIMER_TIMEOUT;
                timer[i].cb();       //执行回调函数
            }
            break;
 
        case SOFT_TIMER_TIMEOUT:
            if(timer[i].mode == MODE_ONE_SHOT)
            {
                timer[i].state = SOFT_TIMER_STOPPED;
            }
            else
            {
                timer[i].match = tickCnt_Get() + timer[i].period;
                timer[i].state = SOFT_TIMER_RUNNING;
            }
            break;
 
        default:
            //printf("timer[%d] state error!\r\n", i);
            break;
        }
    }
}
