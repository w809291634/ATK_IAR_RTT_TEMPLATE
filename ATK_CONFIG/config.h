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

/** ϵͳ�������� **/
typedef struct{
  char wifi_ssid[50];               // wifi����
  char wifi_pwd[50];                // wifi����
  uint8_t wifi_flag;                // wifi��־
  
  // app����
  uint8_t current_part;             // ��ǰ���ڷ��� 1 2 ;0xff ������APP,����boot
  uint8_t app1_flag;                // �������
  uint8_t app2_flag;                // �������  
}sys_parameter_t;
extern sys_parameter_t sys_parameter;

typedef  void (*pFunction)(void); 

/** ������Ϣ **/
#define BOOT_IAP    0xA2
#define BOOT_APP    0x3C
#define FLAG_OK     0xDA
#define FLAG_NOK    0xFF
#define APP_OK      0x4D
#define APP_ERR     0xFF

#define BOOTLOADER_START_ADDR             ADDR_FLASH_SECTOR_0             // BOOT�����ַ(64 Kbytes )
#define SYS_PARAMETER_START_ADDR          ADDR_FLASH_SECTOR_4             // ���������ַ(64 Kbytes )
#define APP1_START_ADDR                   ADDR_FLASH_SECTOR_5             // APP����1(384 Kbytes ) ���Ե����������������������׸����������
#define APP1_END_ADDR                     (ADDR_FLASH_SECTOR_8-1)
#define APP1_SIZE                         (ADDR_FLASH_SECTOR_8-ADDR_FLASH_SECTOR_5)

#define APP2_START_ADDR                   ADDR_FLASH_SECTOR_8             // APP����2(512 Kbytes )
#define APP2_END_ADDR                     ADDR_FLASH_SECTOR_11_END
#define APP2_SIZE                         (ADDR_FLASH_SECTOR_11_END+1 -ADDR_FLASH_SECTOR_8)

// �����ϴ����� 
#define FLASH_IMAGE_SIZE                 (uint32_t) (STM32_FLASH_SIZE - (APP1_START_ADDR - 0x08000000)) // û������

/** ���������� **/
#define SYS_PARAMETER_END_ADDR            (ADDR_FLASH_SECTOR_5-1)
#define SYS_PARAMETER_PART_SIZE           ((APP1_START_ADDR-SYS_PARAMETER_START_ADDR)/4)              // ������������ ��λ����
#define SYS_PARAMETER_SIZE                (sizeof(sys_parameter)/4+((sizeof(sys_parameter)%4)?1:0))   // ���ٸ���
#define SYS_PARAMETER_READ                {STMFLASH_Read(SYS_PARAMETER_START_ADDR,\
                                            (uint32_t*)&sys_parameter,SYS_PARAMETER_SIZE);}
#define SYS_PARAMETER_WRITE               STMFLASH_Write(SYS_PARAMETER_START_ADDR,\
                                            (uint32_t*)&sys_parameter,SYS_PARAMETER_SIZE)

/** FLASH��ַ **/
// ��ʼ��ַ�ʹ�С
#define STM32_FLASH_BASE        0x08000000 	        //STM32 FLASH����ʼ��ַ
#define STM32_FLASH_SIZE        (0x100000)          /* 1 MByte */
#define STM32_FLASH_END         (STM32_FLASH_BASE+STM32_FLASH_SIZE-1)

// FLASH ��������ʼ��ַ
#define ADDR_FLASH_SECTOR_0       ((uint32_t)0x08000000) 	//����0��ʼ��ַ, 16 Kbytes  
#define ADDR_FLASH_SECTOR_1       ((uint32_t)0x08004000) 	//����1��ʼ��ַ, 16 Kbytes  
#define ADDR_FLASH_SECTOR_2       ((uint32_t)0x08008000) 	//����2��ʼ��ַ, 16 Kbytes  
#define ADDR_FLASH_SECTOR_3       ((uint32_t)0x0800C000) 	//����3��ʼ��ַ, 16 Kbytes  
#define ADDR_FLASH_SECTOR_4       ((uint32_t)0x08010000) 	//����4��ʼ��ַ, 64 Kbytes  
#define ADDR_FLASH_SECTOR_5       ((uint32_t)0x08020000) 	//����5��ʼ��ַ, 128 Kbytes  
#define ADDR_FLASH_SECTOR_6       ((uint32_t)0x08040000) 	//����6��ʼ��ַ, 128 Kbytes  
#define ADDR_FLASH_SECTOR_7       ((uint32_t)0x08060000) 	//����7��ʼ��ַ, 128 Kbytes  
#define ADDR_FLASH_SECTOR_8       ((uint32_t)0x08080000) 	//����8��ʼ��ַ, 128 Kbytes  
#define ADDR_FLASH_SECTOR_9       ((uint32_t)0x080A0000) 	//����9��ʼ��ַ, 128 Kbytes  
#define ADDR_FLASH_SECTOR_10      ((uint32_t)0x080C0000) 	//����10��ʼ��ַ,128 Kbytes  
#define ADDR_FLASH_SECTOR_11      ((uint32_t)0x080E0000) 	//����11��ʼ��ַ,128 Kbytes  
#define ADDR_FLASH_SECTOR_11_END  ((uint32_t)0x080FFFFF) 	//����11��ֹ��ַ 
#define ADDR_FLASH_SYSTEM_MEM     ((uint32_t)0X1FFF0000) 	//STM32F4 ϵͳ�洢������ʼ��ַ 30 Kbytes
         
/** debug ����� **/
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
                                            
#endif
