#ifndef _FLASH_H_
#define _FLASH_H_
#include "sys.h"

uint16_t STMFLASH_GetFlashSector(u32 addr);
u32 STMFLASH_Write(u32 WriteAddr,u32 *pBuffer,u32 NumToWrite);
void STMFLASH_Read(u32 ReadAddr,u32 *pBuffer,u32 NumToRead);
void STMFLASH_Read_Write_test(u32 start_add,u32 end_add);
#endif //_FLASH_H_
