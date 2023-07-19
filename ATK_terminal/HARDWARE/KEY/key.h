#ifndef __KEY_H
#define __KEY_H	 
#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"

/*下面的方式是通过直接操作库函数方式读取IO*/
#define KEY0 		GPIO_ReadInputDataBit(GPIOE,GPIO_Pin_4) //PE4
#define KEY1 		GPIO_ReadInputDataBit(GPIOE,GPIO_Pin_3)	//PE3 
#define KEY2 		GPIO_ReadInputDataBit(GPIOE,GPIO_Pin_2) //PE2
#define WK_UP 	GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_0)	//PA0

#define KEY1_NUM                      0x01
#define KEY2_NUM                      0x02
#define KEY3_NUM                      0x04
#define KEY4_NUM                      0x08

/*下面方式是通过位带操作方式读取IO*/
/*
#define KEY0 		PEin(4)   	//PE4
#define KEY1 		PEin(3)		//PE3 
#define KEY2 		PEin(2)		//P32
#define WK_UP 	PAin(0)		//PA0
*/

void KEY_Init(void);	          //IO初始化
u8 KEY_Scan(u8);  		          //按键扫描函数	
unsigned char key_getState(void);

#endif
