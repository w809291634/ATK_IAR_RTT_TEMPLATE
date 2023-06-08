/****************************************Copyright (c)**************************************************
**                               	丹东东方测控技术有限公司
**                                     		通讯导航部
**                                        
**
**                                		 http://www.dfmc.cc
**
**--------------文件信息--------------------------------------------------------------------------------
**文   件   名: HexParse.h
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
#ifndef _HEXPARSE_H_
#define _HEXPARSE_H_


#define TYPE_DAT   0
#define TYPE_EOF   1
#define TYPE_ESAR  2
#define TYPE_SSAR  3
#define TYPE_ELAR  4
#define TYPE_SLAR  5

#define USER_CODE_START_SEGMENT   16  //测试发现大于64K的块  64K之后的不能一次擦除 , 分多次进行擦除	 16
#define USER_CODE_STOP_SEGMENT    176
#define BACKUP_CODE_START_SEGMENT 320 //备份程序区  这区间的SPI Flash不允许使用						 48
#define BACKUP_CODE_STOP_SEGMENT  352

#define OFFSET     (USER_CODE_START_SEGMENT<<12)
#define STOPADDR   (USER_CODE_STOP_SEGMENT<<12)
#define BACKUP     (BACKUP_CODE_START_SEGMENT<<12)
#define BACKUPEND   (BACKUP_CODE_STOP_SEGMENT<<12)

#define TRUE    1
#define FALSE   0
  

#ifndef  GLOBAL 
#define  GLOBAL   extern
#endif

typedef struct{	
 unsigned short offset;  //地址
 unsigned char type;     //数据类型
 unsigned char data;	 //数据
}strHex,*pStrHex;
/*********************************************************************************************************
** 函数名称: __inline void HexPacketReset(void)
** 函数功能: 复位包计数器
** 入口参数:  无
** 出口参数:  无
** 说明：     
*********************************************************************************************************/
GLOBAL  void HexPacketReset(void); 
/*********************************************************************************************************
** 函数名称: void HexParse(char *buf,int len)
** 函数功能: 解析Hex文件
** 入口参数:  Hex文件包， 包长度
** 出口参数:  无
** 说明：     包为ascill格式  转换为十六进制存入Flash，但未校验
*********************************************************************************************************/
GLOBAL void HexParse(unsigned char *buf,int len);
/*********************************************************************************************************
** 函数名称: unsigned char  HexFileVerify(unsigned *ret)
** 函数功能: Hex文件校验
** 入口参数: 用户代码行最后一行地址
** 出口参数: 校验正确 TRUE   校验错误FALSE
** 说明：    转化后的十六进制文件校验 
*********************************************************************************************************/
GLOBAL unsigned   HexFileVerify(unsigned *ret);
/*********************************************************************************************************
** 函数名称:  void IAPPrograming(unsigned CodeAddress,unsigned endaddress) 
** 函数功能:  IAP编程
** 入口参数:  filelen  待编程hex文件末地址,	CodeAddress  Hex文件在SPI Flash中的偏移地址
** 出口参数:  无
*********************************************************************************************************/
GLOBAL void IAPPrograming(unsigned CodeAddress);

#endif
