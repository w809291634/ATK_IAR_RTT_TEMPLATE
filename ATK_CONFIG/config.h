#ifndef __CONFIG_H__
#define __CONFIG_H__
#include "flash.h"
#include "stm32f4xx.h"

#define BIT_0       (1<<0)
#define BIT_1       (1<<1)
#define BIT_2       (1<<2)
#define BIT_3       (1<<3)
#define BIT_4       (1<<4)
#define BIT_5       (1<<5)
#define BIT_6       (1<<6)
#define BIT_7       (1<<7)
#define ARRAY_SIZE(p) (sizeof(p)/sizeof(p[0]))

/** 系统参数类型 **/
typedef struct{
  char wifi_ssid[50];               // wifi名称
  char wifi_pwd[50];                // wifi密码
  uint8_t wifi_flag;                // wifi标志
  
  // app分区
  uint8_t current_part;             // 当前所在分区 1 2 ;0xff 不启动APP,进入boot
  uint8_t app1_flag;                // 分区标记
  uint8_t app2_flag;                // 分区标记  
}sys_parameter_t;
extern sys_parameter_t sys_parameter;

typedef  void (*pFunction)(void); 

/** 分区信息 **/
#define BOOT_IAP    0xA2
#define BOOT_APP    0x3C
#define FLAG_OK     0xDA
#define FLAG_NOK    0xFF
#define APP_OK      0x4D
#define APP_ERR     0xFF

#define BOOTLOADER_START_ADDR             ADDR_FLASH_SECTOR_0             // BOOT区域地址(64 Kbytes )
#define SYS_PARAMETER_START_ADDR          ADDR_FLASH_SECTOR_4             // 参数区域地址(64 Kbytes )
#define APP1_START_ADDR                   ADDR_FLASH_SECTOR_5             // APP区域1(384 Kbytes ) 可以当做工厂分区，不允许轻易覆盖这个分区
#define APP1_END_ADDR                     (ADDR_FLASH_SECTOR_8-1)
#define APP1_SIZE                         (ADDR_FLASH_SECTOR_8-ADDR_FLASH_SECTOR_5)

#define APP2_START_ADDR                   ADDR_FLASH_SECTOR_8             // APP区域2(512 Kbytes )
#define APP2_END_ADDR                     ADDR_FLASH_SECTOR_11_END
#define APP2_SIZE                         (ADDR_FLASH_SECTOR_11_END+1 -ADDR_FLASH_SECTOR_8)

// 计算上传镜像 
#define FLASH_IMAGE_SIZE                 (uint32_t) (STM32_FLASH_SIZE - (APP1_START_ADDR - 0x08000000)) // 没有适配

/** 参数区域定义 **/
#define SYS_PARAMETER_END_ADDR            (ADDR_FLASH_SECTOR_5-1)
#define SYS_PARAMETER_PART_SIZE           ((APP1_START_ADDR-SYS_PARAMETER_START_ADDR)/4)              // 参数分区容量 单位：字
#define SYS_PARAMETER_SIZE                (sizeof(sys_parameter)/4+((sizeof(sys_parameter)%4)?1:0))   // 多少个字
#define SYS_PARAMETER_READ                {STMFLASH_Read(SYS_PARAMETER_START_ADDR,\
                                            (uint32_t*)&sys_parameter,SYS_PARAMETER_SIZE);}
#define SYS_PARAMETER_WRITE               STMFLASH_Write(SYS_PARAMETER_START_ADDR,\
                                            (uint32_t*)&sys_parameter,SYS_PARAMETER_SIZE)

/** FLASH地址 **/
// 起始地址和大小
#define STM32_FLASH_BASE        0x08000000 	        //STM32 FLASH的起始地址
#define STM32_FLASH_SIZE        (0x100000)          /* 1 MByte */
#define STM32_FLASH_END         (STM32_FLASH_BASE+STM32_FLASH_SIZE-1)

// FLASH 扇区的起始地址
#define ADDR_FLASH_SECTOR_0       ((uint32_t)0x08000000) 	//扇区0起始地址, 16 Kbytes  
#define ADDR_FLASH_SECTOR_1       ((uint32_t)0x08004000) 	//扇区1起始地址, 16 Kbytes  
#define ADDR_FLASH_SECTOR_2       ((uint32_t)0x08008000) 	//扇区2起始地址, 16 Kbytes  
#define ADDR_FLASH_SECTOR_3       ((uint32_t)0x0800C000) 	//扇区3起始地址, 16 Kbytes  
#define ADDR_FLASH_SECTOR_4       ((uint32_t)0x08010000) 	//扇区4起始地址, 64 Kbytes  
#define ADDR_FLASH_SECTOR_5       ((uint32_t)0x08020000) 	//扇区5起始地址, 128 Kbytes  
#define ADDR_FLASH_SECTOR_6       ((uint32_t)0x08040000) 	//扇区6起始地址, 128 Kbytes  
#define ADDR_FLASH_SECTOR_7       ((uint32_t)0x08060000) 	//扇区7起始地址, 128 Kbytes  
#define ADDR_FLASH_SECTOR_8       ((uint32_t)0x08080000) 	//扇区8起始地址, 128 Kbytes  
#define ADDR_FLASH_SECTOR_9       ((uint32_t)0x080A0000) 	//扇区9起始地址, 128 Kbytes  
#define ADDR_FLASH_SECTOR_10      ((uint32_t)0x080C0000) 	//扇区10起始地址,128 Kbytes  
#define ADDR_FLASH_SECTOR_11      ((uint32_t)0x080E0000) 	//扇区11起始地址,128 Kbytes  
#define ADDR_FLASH_SECTOR_11_END  ((uint32_t)0x080FFFFF) 	//扇区11终止地址 
#define ADDR_FLASH_SYSTEM_MEM     ((uint32_t)0X1FFF0000) 	//STM32F4 系统存储器的起始地址 30 Kbytes
         
/** debug 层控制 **/
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
                                            
#endif
