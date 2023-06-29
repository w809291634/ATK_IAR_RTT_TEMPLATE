#ifndef __CONFIG_H__
#define __CONFIG_H__
#include "sys.h"
#include "flash.h"

// 0xff ��ʾ���в����Ϣ
// 0x00 ���в����Ϣ������ʾ
#define DEBUG           0x1E        // 0x01-��ʾ debug ��Ϣ 
                                    // 0x02-��ʾ error ��Ϣ 
                                    // 0x04-��ʾ warning ��Ϣ 
                                    // 0x08-��ʾ info ��Ϣ 
                                    // 0x10-��ʾ at ��Ϣ
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
  char wifi_ssid[50];               // wifi����
  char wifi_pwd[50];                // wifi����
  uint8_t flag;                     // �в�����־
}sys_parameter_t;
extern sys_parameter_t sys_parameter;

#define SYS_PARAMETER_START_ADDR          ADDR_FLASH_SECTOR_4
#define SYS_PARAMETER_END_ADDR            (ADDR_FLASH_SECTOR_5-1)
#define SYS_PARAMETER_OK                  0xDA
#define SYS_PARAMETER_NOK                 0xBC
#define SYS_PARAMETER_SIZE                (sizeof(sys_parameter)/4+((sizeof(sys_parameter)%4)?1:0))   // ���ٸ���
#define SYS_PARAMETER_READ                {STMFLASH_Read(SYS_PARAMETER_START_ADDR,\
                                            (u32*)&sys_parameter,SYS_PARAMETER_SIZE);}
#define SYS_PARAMETER_WRITE               STMFLASH_Write(SYS_PARAMETER_START_ADDR,\
                                            (u32*)&sys_parameter,SYS_PARAMETER_SIZE)
                                        
#endif
