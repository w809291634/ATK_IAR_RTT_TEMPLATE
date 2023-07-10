/*********************************************************************************************
* 文件: gui_thread.c
* 作者：Zhouchj 2020.10.12
* 说明：GUI例程源文件
* 修改：
* 注释：
*********************************************************************************************/
#include "GUI_app/gui_thread.h"
#include "stm32f4xx.h"
#include "GUI.h"
#include "WM.h"
#include "serial.h"
#include "LCD/lcd.h"

/*********************************************************************************************
* GUI全局配置变量
*********************************************************************************************/

WM_HWIN _hLastFrame[WIN_MAX_NUM]={0};											//当前所在窗口的句柄，全局
unsigned char Last_win_id;								//当前所在窗口的ID，全局
GUI_MEMDEV_Handle    hMempic;
GUI_PID_STATE pid_state;
unsigned short LCD_xsize;
unsigned short LCD_ysize;

extern GUI_CONST_STORAGE GUI_BITMAP bmIOS15_1;
extern struct graphic_device g_dev;
extern WM_HWIN Createlockwin(void);
extern WM_HWIN Createwin_mainpage(void);
extern WM_HWIN CreateFramewin(void);
extern WM_HWIN Createstatus_bar(void);
extern GUI_CONST_STORAGE GUI_BITMAP bmicon_smile;

/*********************************************************************************************
* 声明
*********************************************************************************************/
#if(USE_GUI_EXAMPLE==2)
static void touch_app_1(void);
static void gui_app_1(void);
static void gui_app_2(void);
#endif
#if(USE_GUI_EXAMPLE==1)
static void gui_alloc_GetNumFreeBytes(void);
#endif

void switch_win(WM_HWIN* win,unsigned char id)
{
  memcpy(_hLastFrame,win,sizeof(WM_HWIN)*WIN_MAX_NUM);
  Last_win_id=id;
}

void detete_last_win(void)
{
  char i;
  WM_HWIN temp;
  for(i=0;i<WIN_MAX_NUM;i++){
    if(_hLastFrame[i]!=0){
      temp=_hLastFrame[i];
      _hLastFrame[i]=0;           //在父窗口删除前，先清理掉标志位。防止硬件错误。
      WM_DeleteWindow(temp);
    } 
  }
}      
/*********************************************************************************************
* 名称：emwin_init()
* 功能：emWin初始化
* 参数：无
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
static void emwin_init(void)
{                                 
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_CRC, ENABLE);           // 开启CRC时钟
  GUI_Init();                                                   // emWin初始化
  WM_SetCreateFlags(WM_CF_MEMDEV);                              // 窗口启用内存设备
  GUI_UC_SetEncodeUTF8();                                       // 使用UTF-8编码
  GUI_EnableAlpha(1);                                           // 开启透明效果
//  WM_MULTIBUF_Enable(1);                                        // 开启多缓存
  WM_MOTION_Enable(1);                                          // 窗口移动支持
}

static void emwin_init_parameter(void)
{
  LCD_xsize=LCD_GetXSize();
  LCD_ysize=LCD_GetYSize(); 
}
/*********************************************************************************************
* 函 数 名: _cbBkWindow
* 功能说明: 桌面窗口回调函数
* 形    参：pMsg  参数指针
* 返 回 值: 无
*********************************************************************************************/
static void _cbBkWindow(WM_MESSAGE* pMsg)
{

  switch (pMsg->MsgId)
  {
    case WM_PAINT:
      GUI_SetBkColor(GUI_WHITE);         //自定义调色
      GUI_Clear();
    break;
    
    case MSG_LOCK:
      detete_last_win();
      Createlockwin();
      break;
    
    default:
      WM_DefaultProc(pMsg);
  }
}

/*********************************************************************************************
* 名称：gui_thread_entry()
* 功能：GUI线程入口函数
* 参数：*parameter -> 入口参数(暂无)
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
void gui_thread_Task(void *parameter)
{
  (void)parameter;
  emwin_init();
  emwin_init_parameter();
  WM_SetCallback(WM_HBKWIN, &_cbBkWindow);    //设置桌面窗口的回调函数 
  
  Createstatus_bar();
#if(ENABLE_LOCK_SCREEN==1)
  unsigned int time=0;
  Createlockwin();
#else
  Createwin_mainpage();
#endif
  while(1)
  {
#if(USE_GUI_EXAMPLE==1)
    gui_alloc_GetNumFreeBytes();      // 获取GUI VM管理器的剩余内存        
//    gui_app_1();                            // 测试DEMO
//    touch_app_1();                          // 测试DEMO
#endif
    GUI_TOUCH_Exec();                         // 触摸刷新
    GUI_Delay(GUI_TASK_DELAY);                            // GUI刷新并延时

#if(ENABLE_LOCK_SCREEN==1)
    // 触发锁屏
    if(Last_win_id!=LOCK_WIN_ID){
      time++;
      if(time*GUI_TASK_DELAY>=(unsigned int)LOCK_DELAY*1000){
        WM_SendMessageNoPara(WM_HBKWIN,MSG_LOCK);
        time=0;
      }
      // 重置计时
      if(time%20==0){       //降低消耗
        GUI_TOUCH_GetState(&pid_state); 
      }
      if(pid_state.Pressed){              
        time=0;
        pid_state.Pressed=0;
      }  
    }
#endif
  }  
//  vTaskDelete(NULL);
}

#if(USE_GUI_EXAMPLE==1)
// 获取GUI当前的剩余内存
static void gui_alloc_GetNumFreeBytes(void)
{
    uint32_t ram;
    static uint32_t times;
    times++;
    if(times%1000==0){
      ram=GUI_ALLOC_GetNumFreeBytes();      //获取GUI剩余内存空间
      xprintf("ram is %d\r\n",ram);
    }
}
#endif
#if(USE_GUI_EXAMPLE==2)
//测试验证读点函数
static void gui_app_2(void)
{
  GUI_SetColor(GUI_YELLOW);
  GUI_SetDefaultFont(&GUI_Font8x16);
  GUI_SetFont(&GUI_Font8x16);
  GUI_SetBkColor(GUI_BLUE);
  GUI_Clear();
  GUI_SetPenSize(10);
  GUI_SetColor(GUI_RED);
  GUI_DrawLine(20, 10, 180, 90);        //某个指定起点到某个指定终点之间的线,前面是点1的XY
  GUI_DrawLine(20, 90, 180, 10);
  GUI_SetBkColor(GUI_BLACK);
  GUI_SetColor(GUI_WHITE);
  GUI_SetTextMode(GUI_TM_NORMAL);       //设置为显示正常文本
  GUI_DispStringHCenterAt("GUI_TM_NORMAL", 100, 10);
  GUI_SetTextMode(GUI_TM_REV);          //设置为显示反转文本，就是字体和背景颜色反转颜色
  GUI_DispStringHCenterAt("GUI_TM_REV", 100, 26);
  GUI_SetTextMode(GUI_TM_TRANS);        //设置为显示透明文本
  GUI_DispStringHCenterAt("GUI_TM_TRANS", 100, 42);
  GUI_SetTextMode(GUI_TM_XOR);          //设置为反相显示的文本，会存在读点颜色
  GUI_DispStringHCenterAt("GUI_TM_XOR", 100, 58);
  GUI_SetTextMode(GUI_TM_TRANS | GUI_TM_REV);   //反转文本和透明背景
  GUI_DispStringHCenterAt("GUI_TM_TRANS | GUI_TM_REV", 100, 74);
}

//验证
static void gui_app_1(void)
{
  static const GUI_COLOR color[3][2] = {
  {0x00FFFF, 0x0000FF},                                         // 黄色 -> 红色
  {0xFFFF00, 0x00FF00},                                         // 青色 -> 绿色
  {0xFF00FF, 0xFF0000},                                         // 紫色 -> 蓝色
  };
  static short x = 0, y = 0;
  static unsigned char num = 0;
  if(x >= g_dev.lcd_dev->width)                                                // 制造位移变色效果
  {
    x = -60;            //X复位
    y += 20;            //Y方向下移
    if(y >= g_dev.lcd_dev->height)
    {
      num++;            //变化颜色
      if(num >= 3)
        num = 0;
      y = 0;            //Y复位
    }
  }
  // 绘制用水平颜色梯度填充的矩形
  GUI_DrawGradientH(x, y, x+60, y+20, color[num][0], color[num][1]);
  x++;
}

//验证emwin是否正常回调触摸刷新
static void touch_app_1(void)
{
  int x_value=GUI_TOUCH_GetxPhys();
  int y_value=GUI_TOUCH_GetyPhys();
  xprintf("x:%d,y:%d\r\n",x_value,y_value);
}
#endif
