#ifndef __CONFIG_H__
#define __CONFIG_H__
#include "sys.h"
#include "flash.h"

/** ϵͳ�������� **/
typedef struct{
  char wifi_ssid[50];               // wifi����
  char wifi_pwd[50];                // wifi����
  uint8_t flag;                     // �洢��־
  
  // app����
  uint8_t current_part;             // ��ǰ���ڷ��� 1 2
}sys_parameter_t;
extern sys_parameter_t sys_parameter;

typedef  void (*pFunction)(void); 

/** ������Ϣ **/
#define BOOTLOADER_START_ADDR             ADDR_FLASH_SECTOR_0             // BOOT�����ַ(64 Kbytes )
#define SYS_PARAMETER_START_ADDR          ADDR_FLASH_SECTOR_4             // ���������ַ(64 Kbytes )
#define APP1_START_ADDR                   ADDR_FLASH_SECTOR_5             // APP����1(384 Kbytes ) ���Ե����������������������׸����������
#define APP1_END_ADDR                     (ADDR_FLASH_SECTOR_8-1)
#define APP1_SIZE                         (ADDR_FLASH_SECTOR_8-ADDR_FLASH_SECTOR_5)

#define APP2_START_ADDR                   ADDR_FLASH_SECTOR_8             // APP����2(512 Kbytes )
#define APP2_END_ADDR                     (ADDR_FLASH_SECTOR_11-1)
#define APP2_SIZE                         (ADDR_FLASH_SECTOR_11-ADDR_FLASH_SECTOR_8)

// �����ϴ����� 
#define FLASH_IMAGE_SIZE                 (uint32_t) (STM32_FLASH_SIZE - (APP1_START_ADDR - 0x08000000)) // û������

/** ���������� **/
#define SYS_PARAMETER_END_ADDR            (ADDR_FLASH_SECTOR_5-1)
#define SYS_PARAMETER_OK                  0xDA
#define SYS_PARAMETER_NOK                 0xBC
#define SYS_PARAMETER_SIZE                (sizeof(sys_parameter)/4+((sizeof(sys_parameter)%4)?1:0))   // ���ٸ���
#define SYS_PARAMETER_READ                {STMFLASH_Read(SYS_PARAMETER_START_ADDR,\
                                            (u32*)&sys_parameter,SYS_PARAMETER_SIZE);}
#define SYS_PARAMETER_WRITE               STMFLASH_Write(SYS_PARAMETER_START_ADDR,\
                                            (u32*)&sys_parameter,SYS_PARAMETER_SIZE)

/** FLASH��ַ **/
// ��ʼ��ַ�ʹ�С
#define STM32_FLASH_BASE        0x08000000 	        //STM32 FLASH����ʼ��ַ
#define STM32_FLASH_SIZE        (0x100000)          /* 1 MByte */
#define STM32_FLASH_END         (STM32_FLASH_BASE+STM32_FLASH_SIZE-1)

// FLASH ��������ʼ��ַ
#define ADDR_FLASH_SECTOR_0     ((u32)0x08000000) 	//����0��ʼ��ַ, 16 Kbytes  
#define ADDR_FLASH_SECTOR_1     ((u32)0x08004000) 	//����1��ʼ��ַ, 16 Kbytes  
#define ADDR_FLASH_SECTOR_2     ((u32)0x08008000) 	//����2��ʼ��ַ, 16 Kbytes  
#define ADDR_FLASH_SECTOR_3     ((u32)0x0800C000) 	//����3��ʼ��ַ, 16 Kbytes  
#define ADDR_FLASH_SECTOR_4     ((u32)0x08010000) 	//����4��ʼ��ַ, 64 Kbytes  
#define ADDR_FLASH_SECTOR_5     ((u32)0x08020000) 	//����5��ʼ��ַ, 128 Kbytes  
#define ADDR_FLASH_SECTOR_6     ((u32)0x08040000) 	//����6��ʼ��ַ, 128 Kbytes  
#define ADDR_FLASH_SECTOR_7     ((u32)0x08060000) 	//����7��ʼ��ַ, 128 Kbytes  
#define ADDR_FLASH_SECTOR_8     ((u32)0x08080000) 	//����8��ʼ��ַ, 128 Kbytes  
#define ADDR_FLASH_SECTOR_9     ((u32)0x080A0000) 	//����9��ʼ��ַ, 128 Kbytes  
#define ADDR_FLASH_SECTOR_10    ((u32)0x080C0000) 	//����10��ʼ��ַ,128 Kbytes  
#define ADDR_FLASH_SECTOR_11    ((u32)0x080E0000) 	//����11��ʼ��ַ,128 Kbytes  
#define ADDR_FLASH_SYSTEM_MEM   ((u32)0X1FFF0000) 	//STM32F4 ϵͳ�洢������ʼ��ַ 30 Kbytes
                                            
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
