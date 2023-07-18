#ifndef __SYS_CONFIG_H__
#define __SYS_CONFIG_H__
#include "stm32f4xx.h"

/** ϵͳ������ʼֵ **/
#define WIFI_SSID               "Xiaomi_B596"
#define WIFI_PASSWD             "WH15572388670LOL"
#define SYS_FW_VERSION          "V1.0"
#define DEFAULT_PARTITION       2

/** ϵͳ�������� **/
typedef struct{
  char wifi_ssid[48];                 // wifi����
  char wifi_pwd[48];                  // wifi����

  // app����
  uint8_t current_part;               // ��ǰ���ڷ��� 1 2 ;0xff ������APP,����boot
  uint32_t app1_flag;                 // �������
  char app1_fw_version[12];         
  uint32_t app2_flag;                 // �������  
  char app2_fw_version[12]; 
  
  // ���������״̬
  uint32_t parameter_flag;            // ������ʼ��־
}sys_parameter_t;
extern sys_parameter_t sys_parameter;

typedef  void (*pFunction)(void); 

/** ������Ϣ **/
#define FLAG_OK     0xE2F9A1B3
#define FLAG_NOK    0xFFFFFFFF
#define APP_OK      0xA1B3C2D4
#define APP_ERR     0xFFFFFFFF

#define BOOTLOADER_START_ADDR             ADDR_FLASH_SECTOR_0             // BOOT�����ַ(48 Kbytes )
#define SYS_PARAMETER_START_ADDR          ADDR_FLASH_SECTOR_3             // ���������ַ(16 Kbytes )
#define APP1_START_ADDR                   ADDR_FLASH_SECTOR_4             // APP����1(448 Kbytes ) ���Ե����������������������׸����������
#define APP2_START_ADDR                   ADDR_FLASH_SECTOR_8             // APP����2(512 Kbytes )

// ����
#define SYS_PARAMETER_END_ADDR            (APP1_START_ADDR-1)
#define SYS_PARAMETER_SIZE                (APP1_START_ADDR-SYS_PARAMETER_START_ADDR) 

#define APP1_END_ADDR                     (APP2_START_ADDR-1)
#define APP1_SIZE                         (APP2_START_ADDR-APP1_START_ADDR)

#define APP2_END_ADDR                     ADDR_FLASH_SECTOR_11_END
#define APP2_SIZE                         (APP2_END_ADDR-APP2_START_ADDR+1)

// �����ϴ����� 
#define FLASH_IMAGE_SIZE                 (uint32_t) (STM32_FLASH_SIZE - (APP1_START_ADDR - 0x08000000)) // û������

/** ���������� **/
#define SYS_PARAMETER_WORD_SIZE           (sizeof(sys_parameter)/4+((sizeof(sys_parameter)%4)?1:0))   // ���ٸ���
#define SYS_PARAMETER_READ                {STMFLASH_Read(SYS_PARAMETER_START_ADDR,\
                                            (uint32_t*)&sys_parameter,SYS_PARAMETER_WORD_SIZE);}
#define SYS_PARAMETER_WRITE               STMFLASH_Write(SYS_PARAMETER_START_ADDR,\
                                            (uint32_t*)&sys_parameter,SYS_PARAMETER_WORD_SIZE)
/** FLASH��ַ **/
// ��ʼ��ַ�ʹ�С
#define STM32_FLASH_BASE            0x08000000 	        //STM32 FLASH����ʼ��ַ
#define STM32_FLASH_SIZE            (0x100000)          /* 1 MByte */
#define STM32_FLASH_END             (STM32_FLASH_BASE+STM32_FLASH_SIZE-1)

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
