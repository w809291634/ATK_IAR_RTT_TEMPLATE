#ifndef __SYS_CONFIG_H__
#define __SYS_CONFIG_H__
#include "stm32f4xx.h"

/** 系统参数初始值 **/
#define WIFI_SSID               "Xiaomi_B596"
#define WIFI_PASSWD             "WH15572388670LOL"
#define SYS_FW_VERSION          "V1.0"
#define DEFAULT_PARTITION       2

/** 系统参数类型 **/
typedef struct{
  char wifi_ssid[48];                 // wifi名称
  char wifi_pwd[48];                  // wifi密码

  // app分区
  uint8_t current_part;               // 当前所在分区 1 2 ;0xff 不启动APP,进入boot
  uint32_t app1_flag;                 // 分区标记
  char app1_fw_version[12];         
  uint32_t app2_flag;                 // 分区标记  
  char app2_fw_version[12]; 
  
  // 参数区域的状态
  uint32_t parameter_flag;            // 参数初始标志
}sys_parameter_t;
extern sys_parameter_t sys_parameter;

typedef  void (*pFunction)(void); 

/** 分区信息 **/
#define FLAG_OK     0xE2F9A1B3
#define FLAG_NOK    0xFFFFFFFF
#define APP_OK      0xA1B3C2D4
#define APP_ERR     0xFFFFFFFF

#define BOOTLOADER_START_ADDR             ADDR_FLASH_SECTOR_0             // BOOT区域地址(48 Kbytes )
#define SYS_PARAMETER_START_ADDR          ADDR_FLASH_SECTOR_3             // 参数区域地址(16 Kbytes )
#define APP1_START_ADDR                   ADDR_FLASH_SECTOR_4             // APP区域1(448 Kbytes ) 可以当做工厂分区，不允许轻易覆盖这个分区
#define APP2_START_ADDR                   ADDR_FLASH_SECTOR_8             // APP区域2(512 Kbytes )

// 推算
#define SYS_PARAMETER_END_ADDR            (APP1_START_ADDR-1)
#define SYS_PARAMETER_SIZE                (APP1_START_ADDR-SYS_PARAMETER_START_ADDR) 

#define APP1_END_ADDR                     (APP2_START_ADDR-1)
#define APP1_SIZE                         (APP2_START_ADDR-APP1_START_ADDR)

#define APP2_END_ADDR                     ADDR_FLASH_SECTOR_11_END
#define APP2_SIZE                         (APP2_END_ADDR-APP2_START_ADDR+1)

// 计算上传镜像 
#define FLASH_IMAGE_SIZE                 (uint32_t) (STM32_FLASH_SIZE - (APP1_START_ADDR - 0x08000000)) // 没有适配

/** 参数区域定义 **/
#define SYS_PARAMETER_WORD_SIZE           (sizeof(sys_parameter)/4+((sizeof(sys_parameter)%4)?1:0))   // 多少个字
#define SYS_PARAMETER_READ                {STMFLASH_Read(SYS_PARAMETER_START_ADDR,\
                                            (uint32_t*)&sys_parameter,SYS_PARAMETER_WORD_SIZE);}
#define SYS_PARAMETER_WRITE               STMFLASH_Write(SYS_PARAMETER_START_ADDR,\
                                            (uint32_t*)&sys_parameter,SYS_PARAMETER_WORD_SIZE)
/** FLASH地址 **/
// 起始地址和大小
#define STM32_FLASH_BASE            0x08000000 	        //STM32 FLASH的起始地址
#define STM32_FLASH_SIZE            (0x100000)          /* 1 MByte */
#define STM32_FLASH_END             (STM32_FLASH_BASE+STM32_FLASH_SIZE-1)

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
    
#define BIT_0       (1<<0)
#define BIT_1       (1<<1)
#define BIT_2       (1<<2)
#define BIT_3       (1<<3)
#define BIT_4       (1<<4)
#define BIT_5       (1<<5)
#define BIT_6       (1<<6)
#define BIT_7       (1<<7)
#define ARRAY_SIZE(p) (sizeof(p)/sizeof(p[0]))
                                            
#endif
