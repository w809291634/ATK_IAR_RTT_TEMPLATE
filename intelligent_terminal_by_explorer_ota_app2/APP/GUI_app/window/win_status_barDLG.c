/*********************************************************************
*                                                                    *
*                SEGGER Microcontroller GmbH & Co. KG                *
*        Solutions for real time microcontroller applications        *
*                                                                    *
**********************************************************************
*                                                                    *
* C-file generated by:                                               *
*                                                                    *
*        GUI_Builder for emWin version 5.32                          *
*        Compiled Oct  8 2015, 11:59:02                              *
*        (c) 2015 Segger Microcontroller GmbH & Co. KG               *
*                                                                    *
**********************************************************************
*                                                                    *
*        Internet: www.segger.com  Support: support@segger.com       *
*                                                                    *
**********************************************************************
*/

// USER START (Optionally insert additional includes)
// USER END

#include "DIALOG.h"
#include "GUI_app/gui_thread.h"
#include "config.h"
/*********************************************************************
*
*       user config
*
**********************************************************************
*/
#define STATUS_SCREEN_TEXT      GUI_FONT_6X8_ASCII
#define STATUS_SCREEN_X         0
#define STATUS_SCREEN_Y         0
#define STATUS_SCREEN_XSIZE     (LCD_xsize-STATUS_SCREEN_X)
#define STATUS_SCREEN_YSIZE     STATUS_BAR_HEIGHT
#define STATUS_REFRESH_RATE     1000

//区域划分    
//时间
#define STATUS_TIME_XSIZE       (80)
#define STATUS_TIME_X           (LCD_xsize-STATUS_TIME_XSIZE)
//CPU使用率
#define STATUS_CPU_USAGE_XSIZE  (60)
#define STATUS_CPU_USAGE_X      ((LCD_xsize-STATUS_CPU_USAGE_XSIZE)/2)

#define TIMER_0           0
#define TIMER_1           1


/*********************************************************************
*
*       variables
*
**********************************************************************
*/
extern RTC_TimeTypeDef RTC_TimeStruct;
extern RTC_DateTypeDef RTC_DateStruct;
extern volatile unsigned char app_cpu_usage;
/*********************************************************************
*
*       Defines
*
**********************************************************************
*/
#define ID_WINDOW_0        (GUI_ID_USER + 0x00)
#define ID_TEXT_0        (GUI_ID_USER + 0x01)
#define ID_TEXT_1        (GUI_ID_USER + 0x02)

/*********************************************************************
*
*       _cbDialog
*/
static void _cbDialog(WM_MESSAGE * pMsg) {
  WM_HWIN hItem;
  // USER START (Optionally insert additional variables)
  // USER END
  char tbuf[40];
  switch (pMsg->MsgId) {
  case WM_CREATE:
    //
    //ID_TEXT_0
    //
    sprintf((char*)tbuf,"%d:%02d ",RTC_TimeStruct.RTC_Hours,RTC_TimeStruct.RTC_Minutes); 
    hItem=TEXT_CreateEx(STATUS_TIME_X, STATUS_SCREEN_Y,STATUS_TIME_XSIZE, STATUS_SCREEN_YSIZE,
      pMsg->hWin,WM_CF_SHOW,TEXT_CF_VCENTER|TEXT_CF_RIGHT, ID_TEXT_0,tbuf);
  
    TEXT_SetFont(hItem,STATUS_SCREEN_TEXT);
    TEXT_SetBkColor(hItem, GUI_INVALID_COLOR);            //使用背景颜色，透明字体
    TEXT_SetTextColor(hItem,GUI_WHITE);
    //
    //ID_TEXT_1
    //
    sprintf((char*)tbuf,"CPU %d%%",app_cpu_usage); 
    hItem=TEXT_CreateEx(STATUS_CPU_USAGE_X, STATUS_SCREEN_Y,STATUS_CPU_USAGE_XSIZE, STATUS_SCREEN_YSIZE,
      pMsg->hWin,WM_CF_SHOW,TEXT_CF_VCENTER|TEXT_CF_HCENTER, ID_TEXT_1,tbuf);
  
    TEXT_SetFont(hItem,STATUS_SCREEN_TEXT);
    TEXT_SetBkColor(hItem, GUI_INVALID_COLOR);            //使用背景颜色，透明字体
    TEXT_SetTextColor(hItem,GUI_WHITE);
    break;
  
  case WM_PAINT:
    GUI_SetBkColor(GUI_BLACK);         
    GUI_Clear();
    break;
  
  case WM_TIMER:
    WM_RestartTimer(pMsg->Data.v, STATUS_REFRESH_RATE);
    WM_InvalidateWindow(pMsg->hWin);
 
    //右上角显示时间
    sprintf((char*)tbuf,"%02d:%02d ",RTC_TimeStruct.RTC_Hours,RTC_TimeStruct.RTC_Minutes); 
    hItem=WM_GetDialogItem(pMsg->hWin, ID_TEXT_0);
    TEXT_SetText(hItem,tbuf);
  //中间显示CPU使用率
    sprintf((char*)tbuf,"CPU %d%%",app_cpu_usage); 
    hItem=WM_GetDialogItem(pMsg->hWin, ID_TEXT_1);
    TEXT_SetText(hItem,tbuf);
    break;
  
  default:
    WM_DefaultProc(pMsg);
    break;
  }
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/
/*********************************************************************
*
*       Createstatus_bar
*/
WM_HWIN Createstatus_bar(void);
WM_HWIN Createstatus_bar(void) {
  WM_HWIN hWin;
  hWin=WM_CreateWindowAsChild(STATUS_SCREEN_X, STATUS_SCREEN_Y, STATUS_SCREEN_XSIZE, STATUS_SCREEN_YSIZE,
  WM_HBKWIN, WM_CF_SHOW, _cbDialog, 0);
  
  WM_CreateTimer(hWin, TIMER_0, STATUS_REFRESH_RATE, 0);
  return hWin;
}

// USER START (Optionally insert additional public code)
// USER END

/*************************** End of file ****************************/