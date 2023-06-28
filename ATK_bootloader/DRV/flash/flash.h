#ifndef _FLASH_H_
#define _FLASH_H_
#include "sys.h"

//FLASH起始地址
#define STM32_FLASH_BASE 0x08000000 	//STM32 FLASH的起始地址
 

//FLASH 扇区的起始地址
#define ADDR_FLASH_SECTOR_0     ((u32)0x08000000) 	//扇区0起始地址, 16 Kbytes  
#define ADDR_FLASH_SECTOR_1     ((u32)0x08004000) 	//扇区1起始地址, 16 Kbytes  
#define ADDR_FLASH_SECTOR_2     ((u32)0x08008000) 	//扇区2起始地址, 16 Kbytes  
#define ADDR_FLASH_SECTOR_3     ((u32)0x0800C000) 	//扇区3起始地址, 16 Kbytes  
#define ADDR_FLASH_SECTOR_4     ((u32)0x08010000) 	//扇区4起始地址, 64 Kbytes  
#define ADDR_FLASH_SECTOR_5     ((u32)0x08020000) 	//扇区5起始地址, 128 Kbytes  
#define ADDR_FLASH_SECTOR_6     ((u32)0x08040000) 	//扇区6起始地址, 128 Kbytes  
#define ADDR_FLASH_SECTOR_7     ((u32)0x08060000) 	//扇区7起始地址, 128 Kbytes  
#define ADDR_FLASH_SECTOR_8     ((u32)0x08080000) 	//扇区8起始地址, 128 Kbytes  
#define ADDR_FLASH_SECTOR_9     ((u32)0x080A0000) 	//扇区9起始地址, 128 Kbytes  
#define ADDR_FLASH_SECTOR_10    ((u32)0x080C0000) 	//扇区10起始地址,128 Kbytes  
#define ADDR_FLASH_SECTOR_11    ((u32)0x080E0000) 	//扇区11起始地址,128 Kbytes  

#define FLASH_PAGE_SIZE             ((uint16_t)0x400)        //页大小   - 1K
#define FLASH_TYPE_LENGTH           ((uint16_t)0x002)        //类型大小 - 2字节
#define FLASH_PAGE_LENGTH           (FLASH_PAGE_SIZE/FLASH_TYPE_LENGTH)
#define FLAG_OK                     0x00
#define FLAG_NOOK                   0x01

/******************************************************************************
** 函数名称: flash_write
** 功能描述: 将数据写入flash
** 入口参数: 要写入的起始地址；要写入的数据头指针；要写入的数据长度(数据单位：16bit)
** 返 回 值: 写入结果：0--失败；1--成功
******************************************************************************/
uint8_t flash_write(uint32_t address, uint16_t *pdata, uint16_t length);

/******************************************************************************
** 函数名称: flash_read
** 功能描述: 读取flash中的数据
** 入口参数: 要读取的起始地址；保存读取出的数据的头指针；要读取的数据长度(数据单位：16bit)
** 返 回 值: 读取结果：0--失败；1--成功
******************************************************************************/
uint8_t flash_read(uint32_t address, uint16_t *pdata, uint16_t length);

#endif //_FLASH_H_
