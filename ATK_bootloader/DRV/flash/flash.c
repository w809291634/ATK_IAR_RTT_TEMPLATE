#include  "flash.h"
#include  <string.h>

//��ȡָ����ַ����(32λ����) 
//faddr:����ַ 
//����ֵ:��Ӧ����.
static u32 STMFLASH_ReadWord(u32 faddr)
{
	return *(vu32*)faddr; 
}  

//��ȡĳ����ַ���ڵ�flash����
//addr: flash��ַ
//����ֵ:0~11,��addr���ڵ�����
uint16_t STMFLASH_GetFlashSector(u32 addr)
{
	if(addr<ADDR_FLASH_SECTOR_1)return FLASH_Sector_0;
	else if(addr<ADDR_FLASH_SECTOR_2)return FLASH_Sector_1;
	else if(addr<ADDR_FLASH_SECTOR_3)return FLASH_Sector_2;
	else if(addr<ADDR_FLASH_SECTOR_4)return FLASH_Sector_3;
	else if(addr<ADDR_FLASH_SECTOR_5)return FLASH_Sector_4;
	else if(addr<ADDR_FLASH_SECTOR_6)return FLASH_Sector_5;
	else if(addr<ADDR_FLASH_SECTOR_7)return FLASH_Sector_6;
	else if(addr<ADDR_FLASH_SECTOR_8)return FLASH_Sector_7;
	else if(addr<ADDR_FLASH_SECTOR_9)return FLASH_Sector_8;
	else if(addr<ADDR_FLASH_SECTOR_10)return FLASH_Sector_9;
	else if(addr<ADDR_FLASH_SECTOR_11)return FLASH_Sector_10; 
	return FLASH_Sector_11;	
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
u32 STMFLASH_Write(u32 WriteAddr,u32* pBuffer,u32 NumToWrite)	
{ 
  FLASH_Status status = FLASH_COMPLETE;
	u32 addrx=WriteAddr;                        // д�����ʼ��ַ
	u32 end_addr=WriteAddr+NumToWrite*4;        // д��Ľ�����ַ,��1
  u32 word_num=0;
  /* ��ַ��� */
  if( addrx < STM32_FLASH_BASE ||
      addrx > ADDR_FLASH_SYSTEM_MEM ||       // ��ʼ��ַ���
      end_addr > ADDR_FLASH_SYSTEM_MEM ||    // ��ֹ��ַ���
      addrx % 4 
      ){
        debug_err(ERR"STMFLASH_Write Address error! addrx:0x%08x\r\n",addrx);
    return 0;
  }
  
  /* ׼��д��FLASH */
	FLASH_Unlock();									          // ���� 
  FLASH_DataCacheCmd(DISABLE);              // FLASH�����ڼ�,�����ֹ���ݻ��� 
  
  /* �Ȳ������ڵ����� FLASH */
  while(addrx < end_addr)		                // ɨ��һ���ϰ�.(�Է�FFFFFFFF�ĵط�,�Ȳ���)
  {
    if(STMFLASH_ReadWord(addrx)!=0XFFFFFFFF)// �з�0XFFFFFFFF�ĵط�,Ҫ�����������
    {   
      uint16_t Sector_num = STMFLASH_GetFlashSector(addrx);
      status=FLASH_EraseSector(Sector_num,VoltageRange_3);  //VCC=2.7~3.6V֮��!!
      if(status!=FLASH_COMPLETE){
        debug_err(ERR"FLASH_EraseSector error! Sector_num:0x%04x\r\n",Sector_num);
        return 0;
      }
    }else addrx+=4;                         // ��ǰ��д��ֹ��ַ
  } 

  /* ��ʼ��� FLASH */
  while(WriteAddr < addrx)
  {
    if(FLASH_ProgramWord(WriteAddr,*pBuffer)!=FLASH_COMPLETE)// д������
    { 
      debug_err(ERR"FLASH_ProgramWord error! WriteAddr:0x%08x\r\n",WriteAddr);
      return word_num;
    }
    WriteAddr+=4;
    pBuffer++;
    word_num++;
  } 

  /* д��FLASH���� */
  FLASH_DataCacheCmd(ENABLE);	// FLASH��������,�������ݻ���
	FLASH_Lock();               // ����
  return word_num;
} 

//��ָ����ַ��ʼ����ָ�����ȵ�����
//ReadAddr:��ʼ��ַ
//pBuffer:����ָ��
//NumToRead:��(4λ)��
void STMFLASH_Read(u32 ReadAddr,u32* pBuffer,u32 NumToRead)   	
{
	u32 i;
	for(i=0;i<NumToRead;i++)
	{
		pBuffer[i]=STMFLASH_ReadWord(ReadAddr);     // ��ȡ4���ֽ�.
		ReadAddr+=4;                                // ƫ��4���ֽ�.	
	}
}

// Ҫд�뵽STM32 FLASH���ַ�������
const u8 TEXT_Buffer[]={"This a FLASH TEST Program"};
#define TEXT_LENTH  sizeof(TEXT_Buffer)	 		  	          // ���鳤��	
#define SIZE        (TEXT_LENTH/4+((TEXT_LENTH%4)?1:0))   // ���ٸ���
// flash ���Գ���
// start_add: ��ʼ��ַ(�˵�ַ����Ϊ4�ı���!!) 
// (����)NumToWrite ����д����ٸ�TEXT_Buffer
// end_add��������ַ
// STMFLASH_Read_Write_test(ADDR_FLASH_SECTOR_3-96,ADDR_FLASH_SECTOR_3+1000);
void STMFLASH_Read_Write_test(u32 start_add,u32 end_add)
{
  static char count=0;
  char datatemp[TEXT_LENTH+1]={0};
//  u32 end_add=start_add+NumToWrite*SIZE*4;
  while(start_add < end_add && count==0){
    int write_len=STMFLASH_Write(start_add,(u32*)TEXT_Buffer,SIZE);
    STMFLASH_Read(start_add,(u32*)datatemp,SIZE);
    if(memcmp(datatemp,TEXT_Buffer,TEXT_LENTH)==0){
      debug_info(INFO"STMFLASH_Read Success,start_add:0x%08x writelen:%d str:%s\r\n",start_add,write_len,datatemp);
    }
    else{
      debug_err(ERR"STMFLASH_Read_Write_test error!\r\n");
    }
    start_add+=SIZE*4;
  }
  count=1;
}

// дϵͳ����
void write_sys_parameter()
{
  if(SYS_PARAMETER_SIZE<=SYS_PARAMETER_PART_SIZE && SYS_PARAMETER_SIZE==SYS_PARAMETER_WRITE){
    debug_info(INFO"System Parameter Write Success!\r\n");
  }else{
    debug_err(ERR"System Parameter Write Failed!");
  }
}
