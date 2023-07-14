#include  "flash.h"
#include  <string.h>
#include "board.h"

//��ȡָ����ַ����(32λ����) 
//faddr:����ַ 
//����ֵ:��Ӧ����.
static uint32_t STMFLASH_ReadWord(uint32_t faddr)
{
	return *(volatile uint32_t*)faddr; 
}  

//��ȡĳ����ַ���ڵ�flash����
//addr: flash��ַ
//����ֵ:0~11,��addr���ڵ�����
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
* �ر�ע��:��ΪSTM32F4������ʵ��̫��,û�취���ر�����������,���Ա�����
           д��ַ�����0XFF,��ô���Ȳ������������Ҳ�������������.����
           д��0XFF�ĵ�ַ,�����������������ݶ�ʧ.����д֮ǰȷ��������
           û����Ҫ����,��������������Ȳ�����,Ȼ����������д. 
* �������ܣ���ָ����ַ��ʼд��ָ�����ȵ�����
* �ú�����OTP����Ҳ��Ч!��������дOTP��! OTP�����ַ��Χ:0X1FFF7800~0X1FFF7A0F
* WriteAddr:  ��ʼ��ַ(�˵�ַ����Ϊ4�ı���!!)   0x8000 0000 - 0x080F FFFF 
* pBuffer:    ����ָ��
* NumToWrite: ��(32λ)�����������ٸ���(32λ)
* ���أ�0��ʾ�ɹ� д��ɹ���(32λ)������
*************************************************************/
uint32_t STMFLASH_Write(uint32_t WriteAddr,uint32_t* pBuffer,uint32_t NumToWrite)	
{ 
	FLASH_EraseInitTypeDef FlashEraseInit;
	HAL_StatusTypeDef FlashStatus=HAL_OK;
  uint32_t SectorError=0;
	uint32_t addrx=WriteAddr;                        // д�����ʼ��ַ
	uint32_t end_addr=WriteAddr+NumToWrite*4;        // д��Ľ�����ַ,��1
  uint32_t word_num=0;
  /* ��ַ��� */
  if( addrx < STM32_FLASH_BASE ||
      addrx >= ADDR_FLASH_SYSTEM_MEM ||       // ��ʼ��ַ���
      end_addr > ADDR_FLASH_SYSTEM_MEM ||    // ��ֹ��ַ���
      addrx % 4 
      ){
        rt_kprintf("STMFLASH_Write Address error! addrx:0x%08x\r\n",addrx);
    return 0;
  }
  
  /* ׼��д��FLASH */
	HAL_FLASH_Unlock();             //����	
  
  /* �Ȳ������ڵ����� FLASH */
  while(addrx < end_addr)		                // ɨ��һ���ϰ�.(�Է�FFFFFFFF�ĵط�,�Ȳ���)
  {
    if(STMFLASH_ReadWord(addrx)!=0XFFFFFFFF)// �з�0XFFFFFFFF�ĵط�,Ҫ�����������
    {   
      uint8_t Sector_num = STMFLASH_GetFlashSector(addrx);
      FlashEraseInit.TypeErase=FLASH_TYPEERASE_SECTORS;       //�������ͣ��������� 
      FlashEraseInit.Sector=Sector_num;                       //Ҫ����������
      FlashEraseInit.NbSectors=1;                             //һ��ֻ����һ������
      FlashEraseInit.VoltageRange=FLASH_VOLTAGE_RANGE_3;      //��ѹ��Χ��VCC=2.7~3.6V֮��!!
      if(HAL_FLASHEx_Erase(&FlashEraseInit,&SectorError)!=HAL_OK) 
      {
        rt_kprintf("FLASH_EraseSector error! Sector_num:0x%04x\r\n",Sector_num);
        break;//����������	
      }
    }else addrx+=4;
    FLASH_WaitForLastOperation(FLASH_WAITETIME);                //�ȴ��ϴβ������
  } 
  FlashStatus=FLASH_WaitForLastOperation(FLASH_WAITETIME);            //�ȴ��ϴβ������
  
  /* ��ʼ��� FLASH */
  if(FlashStatus==HAL_OK){
    while(WriteAddr < addrx)
    {
      if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,WriteAddr,*pBuffer)!=HAL_OK)// д������
      { 
        rt_kprintf("FLASH_ProgramWord error! WriteAddr:0x%08x\r\n",WriteAddr);
        return word_num;
      }
      WriteAddr+=4;
      pBuffer++;
      word_num++;
    } 
  }
  
  /* д��FLASH���� */
	HAL_FLASH_Lock();           //����
  return word_num;
} 

//��ָ����ַ��ʼ����ָ�����ȵ�����
//ReadAddr:��ʼ��ַ
//pBuffer:����ָ��
//NumToRead:��(4λ)��
void STMFLASH_Read(uint32_t ReadAddr,uint32_t* pBuffer,uint32_t NumToRead)   	
{
	uint32_t i;
	for(i=0;i<NumToRead;i++)
	{
		pBuffer[i]=STMFLASH_ReadWord(ReadAddr);     // ��ȡ4���ֽ�.
		ReadAddr+=4;                                // ƫ��4���ֽ�.	
	}
}

// Ҫд�뵽STM32 FLASH���ַ�������
const char TEXT_Buffer[]={"This a FLASH TEST Program"};
#define TEXT_LENTH  sizeof(TEXT_Buffer)	 		  	          // ���鳤��	
#define SIZE        (TEXT_LENTH/4+((TEXT_LENTH%4)?1:0))   // ���ٸ���
// flash ���Գ���
// start_add: ��ʼ��ַ(�˵�ַ����Ϊ4�ı���!!) 
// (����)NumToWrite ����д����ٸ�TEXT_Buffer
// end_add��������ַ
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

// дϵͳ����
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
