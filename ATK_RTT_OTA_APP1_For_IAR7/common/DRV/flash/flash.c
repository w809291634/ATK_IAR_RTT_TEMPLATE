#include  "flash.h"
#include  <string.h>
#include "board.h"

//读取指定地址的字(32位数据) 
//faddr:读地址 
//返回值:对应数据.
static uint32_t STMFLASH_ReadWord(uint32_t faddr)
{
	return *(volatile uint32_t*)faddr; 
}  

//获取某个地址所在的flash扇区
//addr: flash地址
//返回值:0~11,即addr所在的扇区
uint8_t STMFLASH_GetFlashSector(uint32_t addr)
{
	if(addr<ADDR_FLASH_SECTOR_1)return FLASH_SECTOR_0;
	else if(addr<ADDR_FLASH_SECTOR_2)return FLASH_SECTOR_1;
	else if(addr<ADDR_FLASH_SECTOR_3)return FLASH_SECTOR_2;
	else if(addr<ADDR_FLASH_SECTOR_4)return FLASH_SECTOR_3;
	else if(addr<ADDR_FLASH_SECTOR_5)return FLASH_SECTOR_4;
	else if(addr<ADDR_FLASH_SECTOR_6)return FLASH_SECTOR_5;
	else if(addr<ADDR_FLASH_SECTOR_7)return FLASH_SECTOR_6;
	else if(addr<ADDR_FLASH_SECTOR_8)return FLASH_SECTOR_7;
	else if(addr<ADDR_FLASH_SECTOR_9)return FLASH_SECTOR_8;
	else if(addr<ADDR_FLASH_SECTOR_10)return FLASH_SECTOR_9;
	else if(addr<ADDR_FLASH_SECTOR_11)return FLASH_SECTOR_10;   
	return FLASH_SECTOR_11;	
}

/************************************************************
* 特别注意:因为STM32F4的扇区实在太大,没办法本地保存扇区数据,所以本函数
           写地址如果非0XFF,那么会先擦除整个扇区且不保存扇区数据.所以
           写非0XFF的地址,将导致整个扇区数据丢失.建议写之前确保扇区里
           没有重要数据,最好是整个扇区先擦除了,然后慢慢往后写. 
* 函数功能：从指定地址开始写入指定长度的数据
* 该函数对OTP区域也有效!可以用来写OTP区! OTP区域地址范围:0X1FFF7800~0X1FFF7A0F
* WriteAddr:  起始地址(此地址必须为4的倍数!!)   0x8000 0000 - 0x080F FFFF 
* pBuffer:    数据指针
* NumToWrite: 字(32位)的数量，多少个字(32位)
* 返回：0表示成功 写入成功字(32位)的数量
*************************************************************/
uint32_t STMFLASH_Write(uint32_t WriteAddr,uint32_t* pBuffer,uint32_t NumToWrite)	
{ 
	FLASH_EraseInitTypeDef FlashEraseInit;
	HAL_StatusTypeDef FlashStatus=HAL_OK;
  uint32_t SectorError=0;
	uint32_t addrx=WriteAddr;                        // 写入的起始地址
	uint32_t end_addr=WriteAddr+NumToWrite*4;        // 写入的结束地址,加1
  uint32_t word_num=0;
  /* 地址检查 */
  if( addrx < STM32_FLASH_BASE ||
      addrx >= ADDR_FLASH_SYSTEM_MEM ||       // 起始地址检查
      end_addr > ADDR_FLASH_SYSTEM_MEM ||    // 终止地址检查
      addrx % 4 
      ){
        rt_kprintf("STMFLASH_Write Address error! addrx:0x%08x\r\n",addrx);
    return 0;
  }
  
  /* 准备写入FLASH */
	HAL_FLASH_Unlock();             //解锁	
  
  /* 先擦除所在的扇区 FLASH */
  while(addrx < end_addr)		                // 扫清一切障碍.(对非FFFFFFFF的地方,先擦除)
  {
    if(STMFLASH_ReadWord(addrx)!=0XFFFFFFFF)// 有非0XFFFFFFFF的地方,要擦除这个扇区
    {   
      uint8_t Sector_num = STMFLASH_GetFlashSector(addrx);
      FlashEraseInit.TypeErase=FLASH_TYPEERASE_SECTORS;       //擦除类型，扇区擦除 
      FlashEraseInit.Sector=Sector_num;                       //要擦除的扇区
      FlashEraseInit.NbSectors=1;                             //一次只擦除一个扇区
      FlashEraseInit.VoltageRange=FLASH_VOLTAGE_RANGE_3;      //电压范围，VCC=2.7~3.6V之间!!
      if(HAL_FLASHEx_Erase(&FlashEraseInit,&SectorError)!=HAL_OK) 
      {
        rt_kprintf("FLASH_EraseSector error! Sector_num:0x%04x\r\n",Sector_num);
        break;//发生错误了	
      }
    }else addrx+=4;
    FLASH_WaitForLastOperation(FLASH_WAITETIME);                //等待上次操作完成
  } 
  FlashStatus=FLASH_WaitForLastOperation(FLASH_WAITETIME);            //等待上次操作完成
  
  /* 开始编程 FLASH */
  if(FlashStatus==HAL_OK){
    while(WriteAddr < addrx)
    {
      if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,WriteAddr,*pBuffer)!=HAL_OK)// 写入数据
      { 
        rt_kprintf("FLASH_ProgramWord error! WriteAddr:0x%08x\r\n",WriteAddr);
        return word_num;
      }
      WriteAddr+=4;
      pBuffer++;
      word_num++;
    } 
  }
  
  /* 写入FLASH结束 */
	HAL_FLASH_Lock();           //上锁
  return word_num;
} 

//从指定地址开始读出指定长度的数据
//ReadAddr:起始地址
//pBuffer:数据指针
//NumToRead:字(4位)数
void STMFLASH_Read(uint32_t ReadAddr,uint32_t* pBuffer,uint32_t NumToRead)   	
{
	uint32_t i;
	for(i=0;i<NumToRead;i++)
	{
		pBuffer[i]=STMFLASH_ReadWord(ReadAddr);     // 读取4个字节.
		ReadAddr+=4;                                // 偏移4个字节.	
	}
}

// 要写入到STM32 FLASH的字符串数组
const char TEXT_Buffer[]={"This a FLASH TEST Program"};
#define TEXT_LENTH  sizeof(TEXT_Buffer)	 		  	          // 数组长度	
#define SIZE        (TEXT_LENTH/4+((TEXT_LENTH%4)?1:0))   // 多少个字
// flash 测试程序
// start_add: 起始地址(此地址必须为4的倍数!!) 
// (废弃)NumToWrite 测试写入多少个TEXT_Buffer
// end_add：结束地址
// STMFLASH_Read_Write_test(ADDR_FLASH_SECTOR_3-96,ADDR_FLASH_SECTOR_3+1000);
void STMFLASH_Read_Write_test(uint32_t start_add,uint32_t end_add)
{
  static char count=0;
  char datatemp[TEXT_LENTH+1]={0};
//  u32 end_add=start_add+NumToWrite*SIZE*4;
  while(start_add < end_add && count==0){
    int write_len=STMFLASH_Write(start_add,(uint32_t*)TEXT_Buffer,SIZE);
    STMFLASH_Read(start_add,(uint32_t*)datatemp,SIZE);
    if(memcmp(datatemp,TEXT_Buffer,TEXT_LENTH)==0){
      rt_kprintf("STMFLASH_Read Success,start_add:0x%08x writelen:%d str:%s\r\n",start_add,write_len,datatemp);
    }
    else{
      rt_kprintf("STMFLASH_Read_Write_test error!\r\n");
      break;
    }
    start_add+=SIZE*4;
  }
  count=1;
}

// 写系统参数
void write_sys_parameter()
{
  sys_parameter.parameter_flag=FLAG_OK;
  if(SYS_PARAMETER_WORD_SIZE<=SYS_PARAMETER_SIZE/4 && SYS_PARAMETER_WORD_SIZE==SYS_PARAMETER_WRITE){
    rt_kprintf("System Parameter Write Success!\r\n");
  }else{
    sys_parameter.parameter_flag=FLAG_NOK;
    rt_kprintf("System Parameter Write Failed!");
  }
}
