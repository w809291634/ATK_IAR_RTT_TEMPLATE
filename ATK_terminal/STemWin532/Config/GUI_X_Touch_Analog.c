#include "GUI.h"
#include "GUIDRV_FlexColor.h"
#include "LCD/lcd.h"
#include "TOUCH/touch.h" 

int  GUI_TOUCH_X_MeasureX (void)
{
	int32_t xvalue;
	//�������Ĵ���ֵ��ȡ
  tp_dev.scan(1);         //��ȡ�ĵ�������ADֵ����������
  xvalue= tp_dev.x[0];
  return xvalue;
}

int  GUI_TOUCH_X_MeasureY(void) 
{	
  int32_t yvalue;
  //�������Ĵ���ֵ��ȡ
  yvalue = tp_dev.y[0];
  return yvalue;
}

void GUI_TOUCH_X_ActivateX(void){

}

void GUI_TOUCH_X_ActivateY(void){

}

