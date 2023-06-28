#ifndef _FLASH_H_
#define _FLASH_H_
#include "sys.h"

//FLASH��ʼ��ַ
#define STM32_FLASH_BASE 0x08000000 	//STM32 FLASH����ʼ��ַ
 

//FLASH ��������ʼ��ַ
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

#define FLASH_PAGE_SIZE             ((uint16_t)0x400)        //ҳ��С   - 1K
#define FLASH_TYPE_LENGTH           ((uint16_t)0x002)        //���ʹ�С - 2�ֽ�
#define FLASH_PAGE_LENGTH           (FLASH_PAGE_SIZE/FLASH_TYPE_LENGTH)
#define FLAG_OK                     0x00
#define FLAG_NOOK                   0x01

/******************************************************************************
** ��������: flash_write
** ��������: ������д��flash
** ��ڲ���: Ҫд�����ʼ��ַ��Ҫд�������ͷָ�룻Ҫд������ݳ���(���ݵ�λ��16bit)
** �� �� ֵ: д������0--ʧ�ܣ�1--�ɹ�
******************************************************************************/
uint8_t flash_write(uint32_t address, uint16_t *pdata, uint16_t length);

/******************************************************************************
** ��������: flash_read
** ��������: ��ȡflash�е�����
** ��ڲ���: Ҫ��ȡ����ʼ��ַ�������ȡ�������ݵ�ͷָ�룻Ҫ��ȡ�����ݳ���(���ݵ�λ��16bit)
** �� �� ֵ: ��ȡ�����0--ʧ�ܣ�1--�ɹ�
******************************************************************************/
uint8_t flash_read(uint32_t address, uint16_t *pdata, uint16_t length);

#endif //_FLASH_H_
