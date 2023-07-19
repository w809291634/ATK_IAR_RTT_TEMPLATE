#ifndef _FLASH_H_
#define _FLASH_H_
#include "stm32f4xx.h"
#include <rtthread.h>

#define FLASH_WAITETIME  50000          //FLASH等待超时时间

uint8_t STMFLASH_GetFlashSector(uint32_t addr);
uint32_t STMFLASH_Write(uint32_t WriteAddr,uint32_t *pBuffer,uint32_t NumToWrite);
void STMFLASH_Read(uint32_t ReadAddr,uint32_t *pBuffer,uint32_t NumToRead);
void STMFLASH_Read_Write_test(uint32_t start_add,uint32_t end_add);
void write_sys_parameter();
#endif //_FLASH_H_
