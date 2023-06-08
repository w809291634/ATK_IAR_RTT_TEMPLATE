/****************************************Copyright (c)**************************************************
**                               	丹东东方测控技术有限公司
**                                     		通讯导航部
**                                        
**
**                                		 http://www.dfmc.cc
**
**--------------文件信息--------------------------------------------------------------------------------
**文   件   名: HexParse.c
**创   建   人: 何文超
**最后修改日期: 2010年12月16日
**描        述: 系统主函数
**              
**--------------历史版本信息----------------------------------------------------------------------------
** 创建人: 何文超
** 版  本: v1.6
** 日　期: 2010年12月16日
** 描　述: 第六版本
**
**--------------当前版本修订------------------------------------------------------------------------------
** 修改人: 何文超
** 日　期: 2010年12月16日
** 描　述: 
**
**----------------------------------------------------------------------------------------------------
********************************************************************************************************/

//#include <includes.h>
#include "HexParse.h"
//#include "sst25vf016bdrv.h"
#include "ymodem.h"
#include "flash_if.h"
#include "stdio.h"

/*********************************************************************************************************
** 函数名称: void HexParse(char *buf,int len)
** 函数功能: 解析Hex文件
** 入口参数:  Hex文件包， 包长度
** 出口参数:  无
** 说明：     包为ascill格式  转换为十六进制存入Flash，但未校验
*********************************************************************************************************/
static int PacketLenghth=0;
// /*********************************************************************************************************
//** 函数名称: command_reset
//** 功能描述: 系统软复位。
//** 输　入: cp		要发送数据的缓冲区指针
//** 输　出: 无
//** 全局变量: 无
//** 调用模块: 
//**
//** 作　者: 
//** 日　期: 
//**-------------------------------------------------------------------------------------------------------
//** 修改人: 
//** 日　期: 
//**------------------------------------------------------------------------------------------------------
//********************************************************************************************************/
//__asm void SystemReset(void) 
//{ 
// MOV R0, #1           //;  
// MSR FAULTMASK, R0    //; 清除FAULTMASK 禁止一切中断 
// LDR R0, =0xE000ED0C  //; 
// LDR R1, =0x05FA0004  //; 
// STR R1, [R0]         //; 系统软件复位  

//deadloop 
//    B deadloop        //; 等待复位成功
//}
/*********************************************************************************************************
** 函数名称:  void IAPPrograming(unsigned CodeAddress,unsigned endaddress) 
** 函数功能:  IAP编程
** 入口参数:  filelen  待编程hex文件末地址,	CodeAddress  Hex文件在SPI Flash中的偏移地址
** 出口参数:  无
*********************************************************************************************************/
unsigned char ram[516] __attribute__((at(0x2001C000))); //数据缓冲区;
void IAPPrograming(unsigned CodeAddress){
	extern int Image$$ER_IROM1$$Base; //分散加载段
  unsigned char data[20];
  unsigned char len=0,j=0;
  unsigned base=0,offset=0;
  unsigned i=0;  
  unsigned char *pdat=data;
	unsigned firstseg=0;
  pStrHex pst = (pStrHex)data;
//  EraseSector(FLASHSARTADDRESS);
start:   
  SSTF016B_RD(CodeAddress+i,1,&len);
	if(len!=0){
	  if(len<17){
		  i++;
		  SSTF016B_RD(CodeAddress+i,len+4,pdat);
		  i+=len+4;
		  switch(pst->type){		  
		    case  0x00:			//数据行，执行flash编程
					offset =*pdat;
					offset=(offset<<8)+*(pdat+1);//计算相对偏移地址			    	 
					offset = base + offset;
					if(offset>=APPLICATION_ADDRESS){	  //IAP程序段，不能擦除
						  GPIO_ToggleBits(GPIOA,GPIO_Pin_15);
              if(firstseg==0){
								if(0!=FLASH_If_Erase(offset,offset)){
									DPRINTK("Seg:%d Erase faile\n",FLASH_GetSector(offset));
									SystemReset(); 
								}
								else
									DPRINTK( "\33[1A\33[KSeg:%d erased\n",FLASH_GetSector(offset));
								firstseg=1;
					    }						
							if(firstseg==1 && FLASH_GetSector(offset+len)!=FLASH_GetSector(offset)){									//是否是一个新段，新段的话就要执行擦除
								FLASH_If_Erase(offset+len,offset+len);
								DPRINTK("\33[1A\33[KSeg:%d Erased\n",FLASH_GetSector(offset+len));
							}
							for(j=0;j<len;j++)
								ram[j]=*(pdat+3+j);
							if(0!=FLASH_If_Write(&offset,((unsigned *)(&ram[0])),len/4))
								DPRINTK("Flash Addr:%x Program Fail\n",offset);
					}else{
						printk("编程非法地址,系统复位\r\n");
						SystemReset();
					}
			 break;
			case  0x01:
			 break;
			case  0x02:
			 break;
			case  0x03:
			 break;
			case  0x04:		   //扩展的线性记录
			   base = *(pdat+3);
			   base =(base<<8)+ *(pdat+4);  //计算基址 		   
			   base <<=16;
				break;
			case  0x05:
			default:
				break;
		  };
		  goto start;
		}	   //end of  if(len<17)
		else{
			  printk("编程错误,系统复位\r\n");
				SystemReset();
				//系统复位			    
		}
	} //end of if(len!=0)
	printk("\33[1A\33[K");
	printk("程序更新成功,系统复位\r\n");
	SystemReset();
}

/*********************************************************************************************************
** 函数名称: __inline void HexPacketReset(void)
** 函数功能: 复位包计数器
** 入口参数:  无
** 出口参数:  无
** 说明：     
*********************************************************************************************************/

 void HexPacketReset(void){	

   PacketLenghth=0;
}
void HexParse(unsigned char buf[],int len){

	 int i;
	 static char dat=0,flag=0;
	 static unsigned char tmptarr[22];
	 static unsigned char index=0;
	 static unsigned int  addroffset=OFFSET;

	 for(i=0;i<len;i++){
	 
	    switch(buf[i]){
		
		  case 0x3A: 		      
			  break;
     	  case 0x0D:
		      break;
		  case 0x0A:
		      SSTF016B_WR(addroffset,tmptarr,tmptarr[0]+5);  //写Flash	 			  
					addroffset=OFFSET+PacketLenghth;
					index=0;
		      break;
		  case 0x30:   //0
		  case 0x31:   //1
		  case 0x32:   //2
		  case 0x33:   //3
		  case 0x34:   //4
		  case 0x35:   //5
		  case 0x36:   //6
		  case 0x37:   //7
		  case 0x38:   //8
		  case 0x39:   //9
		   if(25>index){
		     if(flag==FALSE){
			   dat=buf[i]-'0';
			   flag=TRUE;
			 }
			 else{
			  dat= (dat<<4)+(buf[i]-'0');
			  tmptarr[index++]=dat;
			  flag=FALSE;
			  PacketLenghth++;
			 }
		   }
		   break;
		  case 0x41:   //A
		  case 0x42:   //B
		  case 0x43:   //C
		  case 0x44:   //D
		  case 0x45:   //E
		  case 0x46:   //F
		     if(25>index){
			   if(flag==FALSE){
			     dat=buf[i]-'7';
				 flag=TRUE;
			   }
			   else{
			     dat= (dat<<4)+(buf[i]-'7');
			     tmptarr[index++]=dat;
			     flag=FALSE;
				 PacketLenghth++;
			   }
			 }
		    break;
		  default:
		    break;
		}
	 }   
}
/*********************************************************************************************************
** 函数名称: unsigned char  HexFileVerify(void)
** 函数功能: Hex文件校验
** 入口参数: 无
** 出口参数: 校验正确 TRUE   校验错误FALSE
** 说明：    转化后的十六进制文件校验 
*********************************************************************************************************/
unsigned   HexFileVerify(unsigned *ret){

  unsigned index=0;
	unsigned dat=0,dat1=0,dat2=0;
	unsigned char i=0,j=0,len=0;
	unsigned char temarr[20];
	unsigned filelen=0;

start:
	SSTF016B_RD(index+OFFSET,1,&len);  //读取行长度
	if(len==0){				           //判断是否到达文件末尾
	 SSTF016B_RD(index+OFFSET,5,temarr);
	 filelen=index;
	 if(temarr[1]==0x00&&temarr[2]==0x00&&temarr[3]==0x01&&temarr[4]==0xFF){	//文件以 00000001FF	结尾 
	   i=0xAA;
	   SSTF016B_WR(STOPADDR-8,&i,1);
	   return filelen;
	 }
	 else
	   return FALSE ;
	 }
	 else if(len>16){
	   return FALSE;
	 }
	 index++;
	 SSTF016B_RD(index+OFFSET,len+4,temarr);
     index+=len+4;
	 dat=len;
	 if(j%2)
	   dat1=(temarr[0]<<8)+temarr[1];
	 else
	   dat2=(temarr[0]<<8)+temarr[1];
	 
	 if(temarr[2]==0x05){   
	   if(j%2)
		*ret=dat2;		
	   else 
	    *ret=dat1;
	 }
	 j++;

	 for(i=0;i<(len+4);i++){ 	 
	   dat+=temarr[i];
	 }
	 if((dat%0x100)==0){		  
	   
	    goto start;		   
	  }
	 else
	  return FALSE;	
	  	 
}
