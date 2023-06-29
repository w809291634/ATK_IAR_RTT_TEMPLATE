#ifndef __CONFIG_H__
#define __CONFIG_H__
#include "sys.h"
#include "flash.h"

// 0xff 显示所有层的信息
// 0x00 所有层的信息都不显示
#define DEBUG           0x1E        // 0x01-显示 debug 信息 
                                    // 0x02-显示 error 信息 
                                    // 0x04-显示 warning 信息 
                                    // 0x08-显示 info 信息 
                                    // 0x10-显示 at 信息
#define ERR "ERROR:"
#define WARNING "WARNING:"
#define INFO "INFORMATION:"

#if (DEBUG & 0x01)
#define debug printk
#else
#define debug(...)
#endif

#if (DEBUG & 0x02)
#define debug_err printk
#else
#define debug_err(...)
#endif

#if (DEBUG & 0x04)
#define debug_war printk
#else
#define debug_war(...)
#endif

#if (DEBUG & 0x08)
#define debug_info printk
#else
#define debug_info(...)
#endif

#if (DEBUG & 0x10)
#define debug_at printk
#else
#define debug_at(...)
#endif

typedef struct{
  char wifi_ssid[50];               // wifi名称
  char wifi_pwd[50];                // wifi密码
  uint8_t flag;                     // 有参数标志
}sys_parameter_t;
extern sys_parameter_t sys_parameter;

#define SYS_PARAMETER_START_ADDR          ADDR_FLASH_SECTOR_4
#define SYS_PARAMETER_END_ADDR            (ADDR_FLASH_SECTOR_5-1)
#define SYS_PARAMETER_OK                  0xDA
#define SYS_PARAMETER_NOK                 0xBC
#define SYS_PARAMETER_SIZE                (sizeof(sys_parameter)/4+((sizeof(sys_parameter)%4)?1:0))   // 多少个字
#define SYS_PARAMETER_READ                {STMFLASH_Read(SYS_PARAMETER_START_ADDR,\
                                            (u32*)&sys_parameter,SYS_PARAMETER_SIZE);}
#define SYS_PARAMETER_WRITE               STMFLASH_Write(SYS_PARAMETER_START_ADDR,\
                                            (u32*)&sys_parameter,SYS_PARAMETER_SIZE)
                                        
#endif
