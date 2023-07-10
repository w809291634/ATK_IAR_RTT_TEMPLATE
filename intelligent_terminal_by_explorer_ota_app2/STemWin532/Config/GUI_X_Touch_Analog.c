#include "GUI.h"
#include "GUIDRV_FlexColor.h"
#include "LCD/lcd.h"
#include "TOUCH/touch.h" 

int  GUI_TOUCH_X_MeasureX (void)
{
	int32_t xvalue;
	//电容屏的触摸值获取
  tp_dev.scan(1);         //获取的电阻屏的AD值，物理坐标
  xvalue= tp_dev.x[0];
  return xvalue;
}

int  GUI_TOUCH_X_MeasureY(void) 
{	
  int32_t yvalue;
  //电容屏的触摸值获取
  yvalue = tp_dev.y[0];
  return yvalue;
}

void GUI_TOUCH_X_ActivateX(void){

}

void GUI_TOUCH_X_ActivateY(void){

}

