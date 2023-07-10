#include "apl_key.h"
/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
#include "key.h"
#include "serial.h"
#include "app_data_protocol.h"
#include "esp32_at.h"
#include "stdlib.h"

#define KEY_SCAN_CYCLE   100
#define PACKAGING_CYALE  200       //数据封包发送周期，单位ms
#define STM32_ESP32_AT_TEST     0     // 是否启动STM32 和 ESP32 之间的AT命令测试程序，测试AT命令之间的通讯稳定性

extern volatile signed int car_rocker_offset_x;      // 摇杆X方向的移动量
extern volatile signed int car_rocker_offset_y;      // 摇杆Y方向的移动量
extern unsigned int car_rocker_offset_max;  // 摇杆XY方向的最大移动量

static void packaging_value(signed int x, signed int y ,unsigned char keyVal)
{
  char buf[50]={0};
  sprintf(buf,"%d/%d/%d",x,y,keyVal);
  zxbeeBegin();
  zxbeeAdd("V1",buf);
  char *p = zxbeeEnd();
  app_data_send_fun(p);
}

static void packaging_fun()
{
  static unsigned char keyVal = 0;  
  static unsigned char last_keyVal = 0;  
  static unsigned char cnt=0;
  
  static signed int last_car_rocker_offset_x = 0;  
  static signed int last_car_rocker_offset_y = 0; 
  static signed int rocker_offset_x_percent = 0;
  static signed int rocker_offset_y_percent = 0;
  
  // 处理按键的消息
  keyVal = key_getState();            // 检测是否有按键按下 
  vTaskDelay(10 / portTICK_RATE_MS);  // 消抖
  if(key_getState()!=keyVal)keyVal=0;
  
  // 用来测试stm32和ESP32 指令通讯的代码
#if(STM32_ESP32_AT_TEST==1)
  static unsigned char test_bit=0;
  if(test_bit){
    car_rocker_offset_x++;
    car_rocker_offset_y++;
  }else{
    car_rocker_offset_x--;
    car_rocker_offset_y--;
  }
  if(car_rocker_offset_x>(signed int)car_rocker_offset_max/2){
    test_bit=0;
  }
    
  if(car_rocker_offset_x<(signed int)-car_rocker_offset_max/2){
    test_bit=1;
  }
    
#endif
  
  // 获取摇杆的数据
  rocker_offset_x_percent=(signed int)((float)car_rocker_offset_x/car_rocker_offset_max*100);
  rocker_offset_y_percent=-(signed int)((float)car_rocker_offset_y/car_rocker_offset_max*100);
  
  if(last_car_rocker_offset_x!=car_rocker_offset_x || last_car_rocker_offset_y!=car_rocker_offset_y || last_keyVal!=keyVal)
  { 
    // 打包具体的数数值s
    packaging_value(rocker_offset_x_percent,rocker_offset_y_percent,keyVal);
    // 保存此次的有效值
    last_keyVal=keyVal;
    last_car_rocker_offset_x=car_rocker_offset_x;
    last_car_rocker_offset_y=car_rocker_offset_y;
  }else{                        // 键值没有变化
    if(last_car_rocker_offset_x!=0 || last_car_rocker_offset_y!=0 || keyVal!=0){              // 表示此时按键一直被触发
      cnt++;
      if(cnt%(PACKAGING_CYALE/KEY_SCAN_CYCLE)==0){
        packaging_value(rocker_offset_x_percent,rocker_offset_y_percent,keyVal);
      } 
    }else{
      cnt=0;
    }
  }
}

void app_key_Task(void *parameter)
{
  (void)parameter;
  while(1)
  {
    packaging_fun();
    vTaskDelay(KEY_SCAN_CYCLE / portTICK_RATE_MS);                         // 按键扫描周期
  }
}
