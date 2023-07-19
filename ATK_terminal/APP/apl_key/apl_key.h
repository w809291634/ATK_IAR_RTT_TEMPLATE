#ifndef __APL_KEY_H_
#define __APL_KEY_H_

#define K1_PRESSED            0x01                              //宏定义K1数字编号
#define K2_PRESSED            0x02                              //宏定义K2数字编号
#define K3_PRESSED            0x04                              //宏定义K3数字编号
#define K4_PRESSED            0x08                              //宏定义K4数字编号

void app_key_Task(void *parameter);

#endif // __APL_KEY_H_
