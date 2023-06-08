/* YMODEM support for bootldr
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^
 * Copyright (C) 2001  John G Dorsey
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * The author may be contacted via electronic mail at <john+@cs.cmu.edu>,
 * or at the following address:
 *
 *   John Dorsey
 *   Carnegie Mellon University
 *   HbH2201 - ICES
 *   5000 Forbes Avenue
 *   Pittsburgh, PA  15213
 *
 *
 * Notes:
 * ^^^^^^
 * Tested against lsz (`sb') from within Minicom. The YMODEM spec says
 * that the receiver should just keep sending CRC/NAKs to the sender 
 * until the transfer begins, but something breaks if a CRC/YMODEM_NAK is
 * sent out while the user is typing in a filename to Minicom. Best
 * solution currently is just to repeat the transfer.
 *
 * History:
 * ^^^^^^^^
 * 12 March, 2001 - created. (jd)
 *
 */

#include "stm32f10x.h"
#include "flash_if.h"
#include "command.h"
#include "main.h"
#include "flash.h"

extern uint8_t watch_dog_flag;
//#include <stdarg.h>
//#include <stdio.h>

/*********************************************************************************************************
** 函数名称: RunUserProgram
** 功能描述: 运行用户程序。
** 输　入: cp		要发送数据的缓冲区指针
** 输　出: 无
** 全局变量: 无
** 调用模块: 
**
** 作　者: 
** 日　期: 
**-------------------------------------------------------------------------------------------------------
** 修改人: 
** 日　期: 
**------------------------------------------------------------------------------------------------------
********************************************************************************************************/
//__asm static void SystemReset(void) 
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
** 函数名称: RunUserProgram
** 功能描述: 运行用户程序。
** 输　入: cp		要发送数据的缓冲区指针
** 输　出: 无
** 全局变量: 无
** 调用模块: 
**
** 作　者: 
** 日　期: 
**-------------------------------------------------------------------------------------------------------
** 修改人: 
** 日　期: 
**------------------------------------------------------------------------------------------------------
********************************************************************************************************/
//static void Encryption(void){
//		uint32_t dat1,dat2,dat3;
//		extern unsigned char userdata[];
//	  user_data_t *p = (user_data_t*)userdata;
//	
//		dat1 = *(volatile unsigned*)(0x1FFF7A18);
//	  dat2 = *(volatile unsigned*)(0x1FFF7A14);
//	  dat3 = *(volatile unsigned*)(0x1FFF7A10);
//	  
//	  if(dat1 != p->hw_id[2] || dat2 != p->hw_id[1] || dat3 != p->hw_id[0] || p->boot_enable != BOOT_ENABLE)
//			SystemReset();
//}
/*********************************************************************************************************
** 函数名称: RunUserProgram
** 功能描述: 运行用户程序。
** 输　入: cp		要发送数据的缓冲区指针
** 输　出: 无
** 全局变量: 无
** 调用模块: 
**
** 作　者: 
** 日　期: 
**-------------------------------------------------------------------------------------------------------
** 修改人: 
** 日　期: 
**------------------------------------------------------------------------------------------------------
********************************************************************************************************/
void RunUserProgram(uint32_t programe_addr) {
  typedef  void (*pFunction)(void);
	pFunction Jump_To_Application; 
  unsigned JumpAddress;	
//	Encryption();
  extern unsigned char userdata[];
	user_data_t *p = (user_data_t*)userdata;
  
  /*跳转区不存在可运行程序；或跳转到APP时，若APP未准备好：切回IAP模式并复位*/
  if ((((*(__IO uint32_t*)programe_addr) & 0x2FFE0000 ) != 0x20000000) ||\
      (programe_addr == APPLICATION_ADDRESS && p->app_en != APP_OK)) {
    p->boot_mode = BOOT_IAP;
    p->app_en = APP_ERR;
    flash_write(PARAMETER_ADDRESS, (unsigned char *)(userdata), sizeof(user_data_t));
    NVIC_SystemReset();
    while(1);
  }

  LED_OFF();
  //关闭所有中断
  NVIC_DisableIRQ(TERM_UART_IRQn);
  SysTick->CTRL = 0;
  SysTick->VAL  = 0;
  //关闭串口功能
	USART_Cmd(TERM_UART, DISABLE);
	USART_ITConfig(TERM_UART, USART_IT_RXNE, DISABLE);
	USART_ClearITPendingBit(TERM_UART, USART_IT_RXNE);
  USART_DeInit(TERM_UART);
	TERM_UART_CLK_INIT(TERM_UART_CLK, DISABLE);
  //重置串口引脚
  GPIO_DeInit(TERM_GPIO_PORT);
  RCC_APB2PeriphClockCmd(TERM_GPIO_CLK, DISABLE);
  //重置运行灯、按键引脚
  GPIO_DeInit(LED_RUN_PORT);
  RCC_APB2PeriphClockCmd(LED_RUN_CLK, DISABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, DISABLE);
  RCC_DeInit();
  
		/* Jump to user application */
	JumpAddress = *(volatile unsigned*) (programe_addr + 4);
	Jump_To_Application = (pFunction) JumpAddress;
	/* Initialize user application's Stack Pointer */
	__set_PSP(*(volatile unsigned*) programe_addr);
	__set_CONTROL(0);
	__set_MSP(*(volatile unsigned*) programe_addr);
	Jump_To_Application();
}
/*********************************************************************************************************
** Function name:           uint8 Uart0GetByte(void)
** Descriptions:            获取一个字节数据
** input parameters:        Buffer:发送数据存储位置
**                          NByte:发送数据个数
** Output parameters::      无
** Returned value:          无
** Created by:              Zhou Shaogang
** Created Date:            2006.11.18
**--------------------------------------------------------------------------------------------------------
** Modified by:             Ni Likao 倪力考
** Modified date:           2007.10.20 
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
//#include "stm32_eval.h"
void putcc(unsigned char dat){
		while (!(TERM_UART->SR & USART_FLAG_TXE));
		TERM_UART->DR = dat;
}
//#define OVER_TIME 5
char getcc(void){
	extern void LED_Toogle(void);
	extern int ringbuf_read(uint8_t *c);
	uint8_t result=0;
	unsigned counter=0;

  while(ringbuf_read(&result)==0) {
    if(counter++ > 123456) {
      counter = 0;
      LED_Toogle();
    }
    if(watch_dog_flag) IWDG_ReloadCounter();
  }
	return result;
}
/**
  ******************************************************************************
  * @file    STM32L1xx_IAP/src/ymodem.c 
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    24-January-2012
  * @brief   This file provides all the software functions related to the ymodem 
  *          protocol.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * FOR MORE INFORMATION PLEASE READ CAREFULLY THE LICENSE AGREEMENT FILE
  * LOCATED IN THE ROOT DIRECTORY OF THIS FIRMWARE PACKAGE.
  *
  * <h2><center>&copy; COPYRIGHT 2012 STMicroelectronics</center></h2>
  ******************************************************************************
  */

/** @addtogroup STM32L1xx_IAP
  * @{
  */ 
  
/* Includes ------------------------------------------------------------------*/
#include "common.h"
#include "ymodem.h"
#include "string.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint8_t FileName[FILE_NAME_LENGTH];
static  uint8_t Use_CRC=0;
static uint8_t tcp_valid=0;

/* Private function prototypes -----------------------------------------------*/
uint16_t Cal_CRC16(const uint8_t *,uint32_t );
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Receive byte from sender
  * @param  c: Character
  * @param  timeout: Timeout
  * @retval 0: Byte received
  *        -1: Timeout
  */
static  int32_t Receive_Byte (uint8_t *c, uint32_t timeout)
{
  while (timeout-- > 0)
  {
		if(0 == (timeout%10000)) IWDG_ReloadCounter();  
    if (SerialKeyPressed(c) == 1)
    {
      return 0;
    }
  }
  return -1;
}

/**
  * @brief  Send a byte
  * @param  c: Character
  * @retval 0: Byte sent
  */
static uint32_t Send_Byte (uint8_t c)
{
  SerialPutChar(c);
  return 0;
}

void ringbuf_flush(void);
/**
  * @brief  Receive a packet from sender
  * @param  data
  * @param  length
  * @param  timeout
  *     0: end of transmission
  *    -1: abort by sender
  *    >0: packet length
  * @retval 0: normally return
  *        -1: timeout or packet error
  *         1: abort by user
  */
static int32_t Receive_Packet (uint8_t *data, int32_t *length, uint32_t timeout)
{
  uint16_t i, packet_size;
  uint8_t c;
  *length = 0;
  if (Receive_Byte(&c, timeout) != 0)
  {
    return -1;
  }
  switch (c)
  {
    case YMODEM_SOH:
      packet_size = PACKET_SIZE;
      break;
    case YMODEM_STX:
      packet_size = PACKET_1K_SIZE;
      break;
    case YMODEM_EOT:
      return 0;
    case YMODEM_CAN:
      if ((Receive_Byte(&c, timeout) == 0) && (c == YMODEM_CAN))
      {
        *length = -1;
        return 0;
      }
      else
      {
        return -1;
      }
    case YMODEM_ABORT1:
    case YMODEM_ABORT2:
      return 1;
		/*****************************清除掉FF*****************************/
		case YMODEM_TCP:
			*data = c;
			for(i=1;i<6;i++){
				if (Receive_Byte(data + i, timeout) != 0)
				{
					return -1;
				}
			}
			if(*data == 0xFF && *(data+1)==0xFD && *(data+2)==0x00 && \
				 *(data+3)==0xFF && *(data+4)==0xFB && *(data+5)==0x00){
					tcp_valid = 1;
			}
			return -1;
		/*****************************************************************/
    default:
      return -1;
  }
  *data = c;
  for (int ff_flag = 0,i = 1; i < (packet_size +(Use_CRC? PACKET_OVERHEAD_CRC : PACKET_OVERHEAD)); i ++)
  {
    if (Receive_Byte(data + i, timeout) != 0)
    {
      return -1;
    }
		/*****************************清除掉FF*****************************/
		if(tcp_valid == 1){
			if(ff_flag == 1 && *(data+i) == 0xFF){
				i-=1;
				ff_flag = 0;
			}else{
				if(*(data+i) == 0xFF)
					ff_flag =1;
				else
					ff_flag = 0;
			}
		}
		/*****************************************************************/
  }
  if (data[PACKET_SEQNO_INDEX] != ((data[PACKET_SEQNO_COMP_INDEX] ^ 0xff) & 0xff))
  {
    return -1;
  }
	
	if(Use_CRC){
    /* It seems odd that the CRC doesn't cover the three preamble bytes. */
    if((Cal_CRC16((unsigned char *)(data + PACKET_HEADER), packet_size + PACKET_TRAILER_CRC)&0xFFFF) != 0)	{
			return -1;	//crc校验出错
		}
  } else {
    unsigned sum;
    for(i = PACKET_HEADER, sum = 0; i < packet_size + PACKET_HEADER; ++i)
      sum += data[i];

    if((sum & 0xff) != (data[i] & 0xff))
      return -1;	//累加和校验出错
  }
  *length = packet_size;
	ringbuf_flush();
  return 0;
}

/**
  * @brief  Receive a file using the ymodem protocol.
  * @param  buf: Address of the first byte.
  * @retval The size of the file.
  */
int32_t Ymodem_Receive (uint8_t *buf)
{
  uint8_t packet_data[PACKET_1K_SIZE + PACKET_OVERHEAD], file_size[FILE_SIZE_LENGTH], *file_ptr, *buf_ptr;
  int32_t i, packet_length, session_done, file_done, packets_received, errors, session_begin, size = 0;
  uint32_t flashdestination, ramsource;
	int ret;

  /* Initialize flashdestination variable */
  flashdestination = APPLICATION_ADDRESS;

  for (session_done = 0, errors = 0, session_begin = 0; ;)
  {
    for (packets_received = 0, file_done = 0, buf_ptr = buf; ;)
    {
      switch (Receive_Packet(packet_data, &packet_length, NAK_TIMEOUT))
      {
        case 0:
          errors = 0;
          switch (packet_length)
          {
            /* Abort by sender */
            case -1:
              Send_Byte(YMODEM_ACK);
						  tcp_valid = 0;
              return 0;
            /* End of transmission */
            case 0:
              Send_Byte(YMODEM_ACK);
              file_done = 1;
              break;
            /* Normal packet */
            default:
              if ((packet_data[PACKET_SEQNO_INDEX] & 0xff) != (packets_received & 0xff))
              {
                Send_Byte(YMODEM_NAK);
              }
              else
              {
                if (packets_received == 0)
                {
                  /* Filename packet */
                  if (packet_data[PACKET_HEADER] != 0)
                  {
                    /* Filename packet has valid data */
                    for (i = 0, file_ptr = packet_data + PACKET_HEADER; (*file_ptr != 0) && (i < FILE_NAME_LENGTH);)
                    {
                      FileName[i++] = *file_ptr++;
                    }
                    FileName[i++] = '\0';
                    for (i = 0, file_ptr ++; (*file_ptr != ' ') && (i < FILE_SIZE_LENGTH);)
                    {
                      file_size[i++] = *file_ptr++;
                    }
                    file_size[i++] = '\0';
//                    Str2Int(file_size, &size);
										size = stringtoul((const char*)file_size,&ret);

                    /* Test the size of the image to be sent */
                    /* Image size is greater than Flash size */
                    if (size > (USER_FLASH_SIZE + 1))
                    {
                      /* End session */
                      Send_Byte(YMODEM_CAN);
                      Send_Byte(YMODEM_CAN);
											tcp_valid = 0;
                      return -1;
                    }
//                  /* erase user application area */
										/* Erase the needed pages where the user application will be loaded */
                    /* Define the number of page to be erased */
										{
											uint16_t PageSize = PAGE_SIZE;
											uint32_t EraseCounter = 0x0;
											uint32_t NbrOfPage = 0;
											FLASH_Status FLASHStatus = FLASH_COMPLETE;
											size = stringtoul((const char*)file_size,&ret);
											NbrOfPage = FLASH_PagesMask(size);
											/* Erase the FLASH pages */
											for (EraseCounter = 0; (EraseCounter < NbrOfPage) && (FLASHStatus == FLASH_COMPLETE); EraseCounter++)
											{
												FLASHStatus = FLASH_ErasePage(flashdestination + (PageSize * EraseCounter));
											}
										}
                    Send_Byte(YMODEM_ACK);
                    Send_Byte(YMODEM_CRC);
                  }
                  /* Filename packet is empty, end session */
                  else
                  {
                    Send_Byte(YMODEM_ACK);
                    file_done = 1;
                    session_done = 1;
                    break;
                  }
                }
                /* Data packet */
                else
                {
									for(unsigned i=0;i<packet_length;i++){
										*(buf_ptr+i)=*(packet_data+PACKET_HEADER+i);
									}
//                  memcpy(buf_ptr, packet_data + PACKET_HEADER, packet_length);
                  ramsource = (uint32_t)buf;
									{
										uint32_t j;
										size = stringtoul((const char*)file_size,&ret);
										for (j = 0;(j < packet_length) && (flashdestination <  APPLICATION_ADDRESS + size);j += 4)
										{
											/* Program the data received into STM32F10x Flash */
											FLASH_ProgramWord(flashdestination, *(uint32_t*)ramsource);
											if (*(uint32_t*)flashdestination != *(uint32_t*)ramsource)
											{
												/* End session */
												Send_Byte(YMODEM_CAN);
												Send_Byte(YMODEM_CAN);
												tcp_valid = 0;
												return -2;
											}
											flashdestination += 4;
											ramsource += 4;
										}
										Send_Byte(YMODEM_ACK);
									}
                }
                packets_received ++;
                session_begin = 1;
              }
          }
          break;
        case 1:
          Send_Byte(YMODEM_CAN);
          Send_Byte(YMODEM_CAN);
					tcp_valid = 0;
          return -3;
        default:
          if (session_begin > 0)
          {
            errors ++;
          }
          if (errors > MAX_ERRORS)
          {
            Send_Byte(YMODEM_CAN);
            Send_Byte(YMODEM_CAN);
						tcp_valid = 0;
            return 0;
          }
          Send_Byte(YMODEM_CRC);
					Use_CRC=1;
          break;
      }
      if (file_done != 0)
      {
        break;
      }
    }
    if (session_done != 0)
    {
      break;
    }
    if(watch_dog_flag) IWDG_ReloadCounter();
  }
	tcp_valid = 0;
  //延时一段时间
  uint32_t count = 6666666;
  while(count-- > 0) {
    if(watch_dog_flag) IWDG_ReloadCounter();
  }
	printk("\nFile %s (%s bytes) ",FileName,file_size);
	return (int32_t)(size);
}

/**
  * @brief  check response using the ymodem protocol
  * @param  buf: Address of the first byte
  * @retval The size of the file
  */
int32_t Ymodem_CheckResponse(uint8_t c)
{
  return 0;
}

/**
  * @brief  Prepare the first block
  * @param  timeout
  *     0: end of transmission
  * @retval None
  */
void Ymodem_PrepareIntialPacket(uint8_t *data, const uint8_t* fileName, uint32_t *length)
{
  uint16_t i, j;
  uint8_t file_ptr[10];
  
  /* Make first three packet */
  data[0] = YMODEM_SOH;
  data[1] = 0x00;
  data[2] = 0xff;
  
  /* Filename packet has valid data */
  for (i = 0; (fileName[i] != '\0') && (i < FILE_NAME_LENGTH);i++)
  {
     data[i + PACKET_HEADER] = fileName[i];
  }

  data[i + PACKET_HEADER] = 0x00;
  
  Int2Str (file_ptr, *length);
  for (j =0, i = i + PACKET_HEADER + 1; file_ptr[j] != '\0' ; )
  {
     data[i++] = file_ptr[j++];
  }
  
  for (j = i; j < PACKET_SIZE + PACKET_HEADER; j++)
  {
    data[j] = 0;
  }
}

/**
  * @brief  Prepare the data packet
  * @param  timeout
  *     0: end of transmission
  * @retval None
  */
void Ymodem_PreparePacket(uint8_t *SourceBuf, uint8_t *data, uint8_t pktNo, uint32_t sizeBlk)
{
  uint16_t i, size, packetSize;
  uint8_t* file_ptr;
  
  /* Make first three packet */
  packetSize = sizeBlk >= PACKET_1K_SIZE ? PACKET_1K_SIZE : PACKET_SIZE;
  size = sizeBlk < packetSize ? sizeBlk :packetSize;
  if (packetSize == PACKET_1K_SIZE)
  {
     data[0] = YMODEM_STX;
  }
  else
  {
     data[0] = YMODEM_SOH;
  }
  data[1] = pktNo;
  data[2] = (~pktNo);
  file_ptr = SourceBuf;
  
  /* Filename packet has valid data */
  for (i = PACKET_HEADER; i < size + PACKET_HEADER;i++)
  {
     data[i] = *file_ptr++;
  }
  if ( size  <= packetSize)
  {
    for (i = size + PACKET_HEADER; i < packetSize + PACKET_HEADER; i++)
    {
      data[i] = 0x1A; /* EOF (0x1A) or 0x00 */
    }
  }
}

/**
  * @brief  Update YMODEM_CRC for input byte
  * @param  CRC input value 
  * @param  input byte
  * @retval None
  */
uint16_t UpdateCRC16(uint16_t crcIn, uint8_t byte)
{
  uint32_t crc = crcIn;
  uint32_t in = byte | 0x100;

  do
  {
    crc <<= 1;
    in <<= 1;
    if(in & 0x100)
      ++crc;
    if(crc & 0x10000)
      crc ^= 0x1021;
  }
  
  while(!(in & 0x10000));

  return crc & 0xffffu;
}


/**
  * @brief  Cal YMODEM_CRC for YModem Packet
  * @param  data
  * @param  length
  * @retval None
  */
uint16_t Cal_CRC16(const uint8_t* data, uint32_t size)
{
  uint32_t crc = 0;
  const uint8_t* dataEnd = data+size;

  while(data < dataEnd)
    crc = UpdateCRC16(crc, *data++);
 
  crc = UpdateCRC16(crc, 0);
  crc = UpdateCRC16(crc, 0);

  return crc&0xffffu;
}

/**
  * @brief  Cal Check sum for YModem Packet
  * @param  data
  * @param  length
  * @retval None
  */
uint8_t CalChecksum(const uint8_t* data, uint32_t size)
{
  uint32_t sum = 0;
  const uint8_t* dataEnd = data+size;

  while(data < dataEnd )
    sum += *data++;

  return (sum & 0xffu);
}

/**
  * @brief  Transmit a data packet using the ymodem protocol
  * @param  data
  * @param  length
  * @retval None
  */
void Ymodem_SendPacket(uint8_t *data, uint16_t length)
{
  uint16_t i;
  i = 0;
  while (i < length)
  {
    Send_Byte(data[i]);
    i++;
  }
}

/**
  * @brief  Transmit a file using the ymodem protocol
  * @param  buf: Address of the first byte
  * @retval The size of the file
  */
uint8_t Ymodem_Transmit (uint8_t *buf, const uint8_t* sendFileName, uint32_t sizeFile)
{
  
  uint8_t packet_data[PACKET_1K_SIZE + PACKET_OVERHEAD];
  uint8_t filename[FILE_NAME_LENGTH];
  uint8_t *buf_ptr, tempCheckSum;
  uint16_t tempCRC;
  uint16_t blkNumber;
  uint8_t receivedC[2], CRC16_F = 0, i;
  uint32_t errors, ackReceived, size = 0, pktSize;

  errors = 0;
  ackReceived = 0;
  for (i = 0; i < (FILE_NAME_LENGTH - 1); i++)
  {
    filename[i] = sendFileName[i];
  }
  CRC16_F = 1;
    
  /* Prepare first block */
  Ymodem_PrepareIntialPacket(&packet_data[0], filename, &sizeFile);
  
  do 
  {
    /* Send Packet */
    Ymodem_SendPacket(packet_data, PACKET_SIZE + PACKET_HEADER);

    /* Send CRC or Check Sum based on CRC16_F */
    if (CRC16_F)
    {
       tempCRC = Cal_CRC16(&packet_data[3], PACKET_SIZE);
       Send_Byte(tempCRC >> 8);
       Send_Byte(tempCRC & 0xFF);
    }
    else
    {
       tempCheckSum = CalChecksum (&packet_data[3], PACKET_SIZE);
       Send_Byte(tempCheckSum);
    }
  
    /* Wait for Ack and 'C' */
    if (Receive_Byte(&receivedC[0], 10000) == 0)  
    {
      if (receivedC[0] == YMODEM_ACK)
      { 
        /* Packet transferred correctly */
        ackReceived = 1;
      }
    }
    else
    {
        errors++;
    }
  }while (!ackReceived && (errors < 0x0A));
  
  if (errors >=  0x0A)
  {
    return errors;
  }
  buf_ptr = buf;
  size = sizeFile;
  blkNumber = 0x01;
  /* Here 1024 bytes package is used to send the packets */
 
  /* Resend packet if YMODEM_NAK  for a count of 10 else end of communication */
  while (size)
  {
    /* Prepare next packet */
    Ymodem_PreparePacket(buf_ptr, &packet_data[0], blkNumber, size);
    ackReceived = 0;
    receivedC[0]= 0;
    errors = 0;
    do
    {
      /* Send next packet */
      if (size >= PACKET_1K_SIZE)
      {
        pktSize = PACKET_1K_SIZE;
       
      }
      else
      {
        pktSize = PACKET_SIZE;
      }
      Ymodem_SendPacket(packet_data, pktSize + PACKET_HEADER);
      /* Send CRC or Check Sum based on CRC16_F */
      /* Send CRC or Check Sum based on CRC16_F */
      if (CRC16_F)
      {
         tempCRC = Cal_CRC16(&packet_data[3], pktSize);
         Send_Byte(tempCRC >> 8);
         Send_Byte(tempCRC & 0xFF);
      }
      else
      {
        tempCheckSum = CalChecksum (&packet_data[3], pktSize);
        Send_Byte(tempCheckSum);
      }
      
      /* Wait for Ack */
      if ((Receive_Byte(&receivedC[0], 100000) == 0)  && (receivedC[0] == YMODEM_ACK))
      {
        ackReceived = 1;  
        if (size > pktSize)
        {
           buf_ptr += pktSize;  
           size -= pktSize;
           if (blkNumber == (USER_FLASH_SIZE/1024))
           {
             return 0xFF; /*  error */
           }
           else
           {
              blkNumber++;
           }
        }
        else
        {
          buf_ptr += pktSize;
          size = 0;
        }
      }
      else
      {
        errors++;
      }
    }while(!ackReceived && (errors < 0x0A));
    /* Resend packet if YMODEM_NAK  for a count of 10 else end of communication */
    
    if (errors >=  0x0A)
    {
      return errors;
    }
    
  }
  ackReceived = 0;
  receivedC[0] = 0x00;
  errors = 0;
  do 
  {
    Send_Byte(YMODEM_EOT);
    /* Send (YMODEM_EOT); */
    /* Wait for Ack */
    if ((Receive_Byte(&receivedC[0], 10000) == 0)  && receivedC[0] == YMODEM_ACK)
    {
      ackReceived = 1;  
    }
    else
    {
      errors++;
    }
  }while (!ackReceived && (errors < 0x0A));
    
  if (errors >=  0x0A)
  {
    return errors;
  }
  
  /* Last packet preparation */
  ackReceived = 0;
  receivedC[0] = 0x00;
  errors = 0;

  packet_data[0] = YMODEM_SOH;
  packet_data[1] = 0;
  packet_data [2] = 0xFF;

  for (i = PACKET_HEADER; i < (PACKET_SIZE + PACKET_HEADER); i++)
  {
     packet_data [i] = 0x00;
  }
  
  do 
  {
    /* Send Packet */
    Ymodem_SendPacket(packet_data, PACKET_SIZE + PACKET_HEADER);

    /* Send CRC or Check Sum based on CRC16_F */
    tempCRC = Cal_CRC16(&packet_data[3], PACKET_SIZE);
    Send_Byte(tempCRC >> 8);
    Send_Byte(tempCRC & 0xFF);
  
    /* Wait for Ack and 'C' */
    if (Receive_Byte(&receivedC[0], 10000) == 0)  
    {
      if (receivedC[0] == YMODEM_ACK)
      { 
        /* Packet transferred correctly */
        ackReceived = 1;
      }
    }
    else
    {
        errors++;
    }
  }while (!ackReceived && (errors < 0x0A));

  /* Resend packet if YMODEM_NAK  for a count of 10  else end of communication */
  if (errors >=  0x0A)
  {
    return errors;
  }  
  
  do 
  {
    Send_Byte(YMODEM_EOT);
    /* Send (YMODEM_EOT); */
    /* Wait for Ack */
    if ((Receive_Byte(&receivedC[0], 10000) == 0)  && receivedC[0] == YMODEM_ACK)
    {
      ackReceived = 1;  
    }
    else
    {
      errors++;
    }
  }while (!ackReceived && (errors < 0x0A));

  if (errors >=  0x0A)
  {
    return errors;
  }
  return 0; /* file transmitted successfully */
}

/**
  * @}
  */

/*******************(C)COPYRIGHT 2012 STMicroelectronics *****END OF FILE****/
