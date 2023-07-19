/*********************************************************************************************
* 文件: gui_thread.h
* 作者：Zhouchj 2020.10.12
* 说明：GUI例程头文件
* 修改：
* 注释：
*********************************************************************************************/
#ifndef __GUI_THREAD_H_
#define __GUI_THREAD_H_
#include "GUI.h"
#include "WM.h"

// 窗口消息定义
#define MSG_SWITCH            (WM_USER + 1)
#define MSG_MAIN_PAGE_SWITCH  (WM_USER + 2)
#define MSG_WIN_INVAILD       (WM_USER + 3)
#define MSG_LOCK              (WM_USER + 4)

// 通知父窗口消息定义
#define NOTIFICATION_UNLOCK (WM_NOTIFICATION_USER+1)

// 窗口ID
#define LOCK_WIN_ID         1
#define MIAN_PAGE_WIN_ID    2
#define CONTROL_WIN_ID         3


#define WIN_MAX_NUM       5    //页面支持的最大窗口数量

// 外部引用声明
extern WM_HWIN _hLastFrame[WIN_MAX_NUM];										//当前所在窗口的句柄，全局
extern unsigned char Last_win_id;								//当前所在窗口的ID，全局
extern unsigned short LCD_xsize;
extern unsigned short LCD_ysize;

// 函数声明
void gui_thread_Task(void *parameter);
void lock(void);              //创建解锁画面
void switch_win(WM_HWIN* win,unsigned char id);    //切换窗口需要引用
void detete_last_win(void);
#endif //__GUI_THREAD_H_
