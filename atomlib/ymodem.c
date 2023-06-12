/**
  ******************************************************************************
  * @file           
  * @author         GoodMorning
  * @brief          ymodem 实现
  * @note
  * <pre>
  * 使用步骤：
  * 0> 编写用户文件操作接口 struct ymodem_fs fs; 实现所需的文件操作  
  *    编写硬件发送函数 int yputs(void * from,const void * data, int len);  
  *    硬件接收到的数据无论是否断包通过 ymodem_recv(&ymod,...) 传入;
  * 
  * 1> 新建传输 struct ymodem ymod ;
  * 
  * 2> 初始化 ymod 的接口 ymodem_init(&ymod,&fs,yputs);
  * 
  * 3> 对 ymod 进行每秒 1 次的 ymodem_period(&ymod) 调用 
  * 
  * 4> 调用 ymodem_start_recv(&ymod) 使能开始接收文件；
  * 
  * *> 如果是仅作接收，则只需要实现 open,close,write 接口即可
  * </pre>
  ******************************************************************************
  *
  * COPYRIGHT(c) 2020 GoodMorning
  *
  ******************************************************************************
  */
/* Includes -----------------------------------------------------------------*/
#include <string.h>
#include <stdlib.h>
#include "ymodem.h"

/* Private macro ------------------------------------------------------------*/

#define PACKET_HEADER           (3)
#define PACKET_TRAILER          (2)
#define PACKET_OVERHEAD         (PACKET_HEADER + PACKET_TRAILER)
#define PACKET_SIZE             (128)
#define PACKET_1K_SIZE          (1024)

#define PACKET_TIMEOUT          (32)

#if 0
#define SOH                     (0x01)  /* start of 128-byte data packet */
#define STX                     (0x02)  /* start of 1024-byte data packet */
#define EOT                     (0x04)  /* end of transmission */
#define ACK                     (0x06)  /* acknowledge */
#define NAK                     (0x15)  /* negative acknowledge */
#define CA                      (0x18)  /* two of these in succession aborts transfer */
#define REQ                     (0x43)  /* 'C' == 0x43, request 16-bit CRC */

#define ABORT1                  (0x41)  /* 'A' == 0x41, abort by user */
#define ABORT2                  (0x61)  /* 'a' == 0x61, abort by user */

#endif

/* Private types ------------------------------------------------------------*/



/* Private variables --------------------------------------------------------*/

static const char SOH = 0x01 ; /* start of 128-byte data packet */
static const char STX = 0x02 ; /* start of 1024-byte data packet */
static const char EOT = 0x04 ; /* end of transmission */
static const char ACK = 0x06 ; /* acknowledge */
static const char NAK = 0x15 ; /* negative acknowledge */
static const char REQ = 'C'  ; /* 'C' == 0x43,request packet */

/* CRC16 implementation acording to CCITT standards */
static const unsigned short crc16tab[256]= {
0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,
0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef,
0x1231,0x0210,0x3273,0x2252,0x52b5,0x4294,0x72f7,0x62d6,
0x9339,0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,0xf3ff,0xe3de,
0x2462,0x3443,0x0420,0x1401,0x64e6,0x74c7,0x44a4,0x5485,
0xa56a,0xb54b,0x8528,0x9509,0xe5ee,0xf5cf,0xc5ac,0xd58d,
0x3653,0x2672,0x1611,0x0630,0x76d7,0x66f6,0x5695,0x46b4,
0xb75b,0xa77a,0x9719,0x8738,0xf7df,0xe7fe,0xd79d,0xc7bc,
0x48c4,0x58e5,0x6886,0x78a7,0x0840,0x1861,0x2802,0x3823,
0xc9cc,0xd9ed,0xe98e,0xf9af,0x8948,0x9969,0xa90a,0xb92b,
0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,0x0a50,0x3a33,0x2a12,
0xdbfd,0xcbdc,0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,0xab1a,
0x6ca6,0x7c87,0x4ce4,0x5cc5,0x2c22,0x3c03,0x0c60,0x1c41,
0xedae,0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,0x8d68,0x9d49,
0x7e97,0x6eb6,0x5ed5,0x4ef4,0x3e13,0x2e32,0x1e51,0x0e70,
0xff9f,0xefbe,0xdfdd,0xcffc,0xbf1b,0xaf3a,0x9f59,0x8f78,
0x9188,0x81a9,0xb1ca,0xa1eb,0xd10c,0xc12d,0xf14e,0xe16f,
0x1080,0x00a1,0x30c2,0x20e3,0x5004,0x4025,0x7046,0x6067,
0x83b9,0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,0xe37f,0xf35e,
0x02b1,0x1290,0x22f3,0x32d2,0x4235,0x5214,0x6277,0x7256,
0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,0xe54f,0xd52c,0xc50d,
0x34e2,0x24c3,0x14a0,0x0481,0x7466,0x6447,0x5424,0x4405,
0xa7db,0xb7fa,0x8799,0x97b8,0xe75f,0xf77e,0xc71d,0xd73c,
0x26d3,0x36f2,0x0691,0x16b0,0x6657,0x7676,0x4615,0x5634,
0xd94c,0xc96d,0xf90e,0xe92f,0x99c8,0x89e9,0xb98a,0xa9ab,
0x5844,0x4865,0x7806,0x6827,0x18c0,0x08e1,0x3882,0x28a3,
0xcb7d,0xdb5c,0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,0xbb9a,
0x4a75,0x5a54,0x6a37,0x7a16,0x0af1,0x1ad0,0x2ab3,0x3a92,
0xfd2e,0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,0x9de8,0x8dc9,
0x7c26,0x6c07,0x5c64,0x4c45,0x3ca2,0x2c83,0x1ce0,0x0cc1,
0xef1f,0xff3e,0xcf5d,0xdf7c,0xaf9b,0xbfba,0x8fd9,0x9ff8,
0x6e17,0x7e36,0x4e55,0x5e74,0x2e93,0x3eb2,0x0ed1,0x1ef0
};

/* Global  variables --------------------------------------------------------*/

/* Private function prototypes ----------------------------------------------*/

/* Gorgeous Split-line ------------------------------------------------------*/


/**
  * @author   Unknown
  * @brief    initialize a ymodem device
  * @param    ymodem   : ymodem module
  * @param    fs       : user file operation interface
  * @param    puts     : 
  * @return   On seccess , 0 is returned;On error, -1 is returned.
*/
static unsigned short crc16_ccitt(const unsigned char *buf, int len)
{
	register int counter;
	register unsigned short crc = 0;
	for( counter = 0; counter < len; counter++){
		crc = (crc<<8) ^ crc16tab[((crc>>8) ^ *(char *)buf++)&0xFF];
	}
	return crc;
}



int ymodem_send(struct ymodem* ymod,const char * file,int filesize)
{
	int len ;
	if (!ymod->fs.open || !ymod->fs.close || 
	    !ymod->fs.read || !ymod->puts) {
		return -1;
	}

	len = strlen(file);//sprintf(ymod->buf[3])

	return 0;
}

/**
  * @author   GoodMorning
  * @brief    ymodem device start to receive
  * @param    ymodem   : ymodem module
  * @return   On seccess , 0 is returned;On error, -1 is returned.
*/
int ymodem_start_recv(struct ymodem *ymod)
{
	if (!ymod->fs.open || !ymod->fs.close || 
	    !ymod->fs.write || !ymod->puts) {
		return -1;
	}

	ymod->fsize = 0   ;
	ymod->recv  = 0   ;
	ymod->tail  = 0   ;
	ymod->nums  = 0   ;
	ymod->stat  = REQ ; 
	return 0;
}


/**
  * @author   GoodMorning
  * @brief    ymodem device period hander
  * @param    ymodem    : ymodem module
  * @param    timestamp : user timestamp
  * @return   don't care
*/
void ymodem_period(struct ymodem * ymodem)
{
	if (!ymodem->stat) {
		return ;
	} 

	if (ymodem->time++ > PACKET_TIMEOUT) {
		ymodem->time = 0 ;
		ymodem->nums = 0 ;
		if (ymodem->stat && ymodem->stat != REQ) {
			ymodem->puts(ymodem,"A",1);
			ymodem->fs.close(&ymodem->fd);
		}
		ymodem->stat = 0 ;
	}

	if (REQ == ymodem->stat && !ymodem->tail){
		ymodem->puts(ymodem,&REQ,1);
	}
}


/**
  * @author   GoodMorning
  * @brief    ymodem device parse a packet
  * @param    ymod      : ymodem module
  * @param    timestamp : user timestamp
  * @return   don't care
*/
static int ymodem_parse(
	struct ymodem * ymod   ,
	unsigned char * data   ,
	int             pktlen )
{
	unsigned short crc16recv ;
	unsigned short crc16cal  ;
	char *         filename  ;
	int            filesize  ;
	int            frest     ;
	
	/* check packet serial number */
	if (data[1] != ymod->nums || data[1] + data[2] != 0xff){
		/* 序号错误，回复 NAK 让主机重发 */
		ymod->time++ ;
		ymod->puts(ymod,&NAK,1);
		return -1;
	}
	else {
		ymod->nums += 1 ;
	}

	/* check crc16 ccitt */
	crc16recv = (data[pktlen-2] << 8) | data[pktlen-1];
	crc16cal  = crc16_ccitt(&data[PACKET_HEADER],pktlen - PACKET_OVERHEAD);
	if (crc16cal != crc16recv) {
		ymod->time++ ;
		ymod->puts(ymod,&NAK,1);
		return -1;
	}

	if (REQ == ymod->stat) {  /* start to recv file */
		/* SOH 00 FF  filename  filezise  NUL  CRCH CRCL */
		filename = (char*)&data[3] ; 
		filesize = atoi(&filename[strlen(filename)+1]);
		if (0 == ymod->fs.open(&ymod->fd,filename)) {
			ymod->puts(ymod,&ACK,1);
			ymod->puts(ymod,&REQ,1);
			ymod->fsize = filesize ;
			ymod->stat  = data[0]  ;
		}
		else {
			/* cannot open file,abort */
			ymod->puts(ymod,"A",1);
			ymod->stat = 0 ;
			ymod->nums = 0 ;
		}
	}
	else /* recving file */
	if (SOH == ymod->stat || STX == ymod->stat) {
		/* file data */
		frest   = ymod->fsize - ymod->recv ;
		pktlen -= PACKET_OVERHEAD          ;
		if (pktlen > frest) {
			pktlen = frest ;
		}

		ymod->fs.write(&ymod->fd,&data[3],pktlen);
		ymod->puts(ymod,&ACK,1);
		ymod->recv += pktlen ;
		if (ymod->recv == ymod->fsize) {
			/* 接收完数据帧 */
			ymod->fs.close(&ymod->fd);
			ymod->stat = EOT ;
			ymod->nums = 0   ;
		}
		else {
			ymod->stat = data[0] ;
		}
	}
	else 
	/*if (EOT == ymod->stat) ; end of receive */{ 
		ymod->stat = 0;
		ymod->puts(ymod,&ACK,1);
	}

	return 0;
}


/**
  * @author   GoodMorning
  * @brief    ymodem device receive & cache data
  * @param    ymodem   : ymodem module
  * @param    data     : 接收到的数据
  * @param    len      : 数据大小
  * @return   On seccess , 0 is returned;On error, -1 is returned.
*/
int ymodem_recv(struct ymodem * ymod , void * data , int len)
{
	unsigned char * buf = data;
	int             pktlen ;
	int             pktype ;

	if (ymod->stat < 1 || len < 1) {
		return -1;
	}

	/* get packet length */
	pktype = ymod->tail ? ymod->buf[0] : buf[0] ;
	if (SOH == pktype) {
		pktlen = PACKET_SIZE + PACKET_OVERHEAD;
	}
	else
	if (STX == pktype) { /* ymodem 1K */
		pktlen = PACKET_1K_SIZE + PACKET_OVERHEAD;
	}
	else 
	if (EOT == pktype) {
		/* end of receive */
		if (EOT != ymod->stat) {
			ymod->fs.close(&ymod->fd);
			ymod->stat = EOT ;
		}
		ymod->puts(ymod,&ACK,1);
		ymod->puts(ymod,&REQ,1);
		return 0;
	}
	else{
		ymod->stat = 0 ;
		ymod->tail = 0 ;
		return -1;
	}

	/* Cache a packet of data */
	if (ymod->tail + len < pktlen) {
		memcpy(&ymod->buf[ymod->tail],data,len);
		ymod->tail += len ;
		return 0;
	}
	else 
	if (ymod->tail){
		memcpy(&ymod->buf[ymod->tail],data,pktlen-ymod->tail);
		ymod->tail = 0 ;
		buf = ymod->buf ;
	}

	ymodem_parse(ymod,buf,pktlen);
	ymod->time = 0   ;
	return 0;
}


/**
  * @author   GoodMorning
  * @brief    initialize a ymodem device
  * @param    ymodem   : ymodem module
  * @param    fs       : user file operation interface
  * @param    puts     : 
  * @return   On seccess , 0 is returned;On error, -1 is returned.
*/
int ymodem_init(
	struct ymodem *    ymodem ,
	struct ymodem_fs * fs     ,
	yputs_t            puts   )
{
	if (!fs || !puts || !ymodem) {
		return -1;
	}

	memset(ymodem      ,0  ,sizeof(struct ymodem)   );
	memcpy(&ymodem->fs ,fs ,sizeof(struct ymodem_fs));
	ymodem->puts = puts;
	return 0;
}

