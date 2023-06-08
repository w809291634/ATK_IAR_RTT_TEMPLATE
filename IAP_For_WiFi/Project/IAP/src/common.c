/**
  ******************************************************************************
  * @file    IAP/src/common.c 
  * @author  MCD Application Team
  * @version V3.3.0
  * @date    10/15/2010
  * @brief   This file provides all the common functions.
  ******************************************************************************
  * @copy
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2010 STMicroelectronics</center></h2>
  */ 

/** @addtogroup IAP
  * @{
  */

/* Includes ------------------------------------------------------------------*/
#include "common.h"
#include "ymodem.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
pFunction Jump_To_Application;
uint32_t JumpAddress;
uint32_t BlockNbr = 0, UserMemoryMask = 0;
__IO uint32_t FlashProtection = 0;
extern uint32_t FlashDestination;
#define RINGBUF_SIZE  2048

static  uint8_t       ringbuf[RINGBUF_SIZE];
static volatile uint16_t      rbuf_write_index=0;
static volatile uint16_t      rbuf_read_index=0;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/*******************************************************************************
* Function Name  : void ringbuf_write(void)
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void ringbuf_write(uint8_t dat)
{
	ringbuf[rbuf_write_index]=dat;
	rbuf_write_index=(rbuf_write_index+1)%RINGBUF_SIZE;
}
/******************************************************************************
 * FunctionName : ringbuf_read
 * Description  : user interface for init uart
 * Parameters   : UartBautRate uart0_br - uart0 bautrate
 *                UartBautRate uart1_br - uart1 bautrate
 * Returns      : NONE
*******************************************************************************/
int ringbuf_read(uint8_t *dat)
{
	uint16_t count=1;
	uint16_t start = rbuf_read_index;
	uint16_t stop  = rbuf_write_index;
	if(stop > start)
	{
		*dat = ringbuf[start];
		rbuf_read_index = start+1;
	}
	else if(stop < start)
	{
		if(start < RINGBUF_SIZE){
			*dat = ringbuf[start];
		}
		else
			*dat = ringbuf[0];
		rbuf_read_index = (start+1)%RINGBUF_SIZE;
	}
	else
	{
		return 0;
	}
	return count;
}
/*******************************************************************************
* Function Name  : void ringbuf_flush(void)
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void ringbuf_flush(void)
{
	rbuf_read_index = rbuf_write_index;
}
/**
  * @brief  Convert an Integer to a string
  * @param  str: The string
  * @param  intnum: The intger to be converted
  * @retval None
  */
void Int2Str(uint8_t* str, int32_t intnum)
{
  uint32_t i, Div = 1000000000, j = 0, Status = 0;

  for (i = 0; i < 10; i++)
  {
    str[j++] = (intnum / Div) + 48;

    intnum = intnum % Div;
    Div /= 10;
    if ((str[j-1] == '0') & (Status == 0))
    {
      j = 0;
    }
    else
    {
      Status++;
    }
  }
}

/**
  * @brief  Convert a string to an integer
  * @param  inputstr: The string to be converted
  * @param  intnum: The intger value
  * @retval 1: Correct
  *         0: Error
  */
uint32_t Str2Int(uint8_t *inputstr, int32_t *intnum)
{
  uint32_t i = 0, res = 0;
  uint32_t val = 0;

  if (inputstr[0] == '0' && (inputstr[1] == 'x' || inputstr[1] == 'X'))
  {
    if (inputstr[2] == '\0')
    {
      return 0;
    }
    for (i = 2; i < 11; i++)
    {
      if (inputstr[i] == '\0')
      {
        *intnum = val;
        /* return 1; */
        res = 1;
        break;
      }
      if (ISVALIDHEX(inputstr[i]))
      {
        val = (val << 4) + CONVERTHEX(inputstr[i]);
      }
      else
      {
        /* return 0, Invalid input */
        res = 0;
        break;
      }
    }
    /* over 8 digit hex --invalid */
    if (i >= 11)
    {
      res = 0;
    }
  }
  else /* max 10-digit decimal input */
  {
    for (i = 0;i < 11;i++)
    {
      if (inputstr[i] == '\0')
      {
        *intnum = val;
        /* return 1 */
        res = 1;
        break;
      }
      else if ((inputstr[i] == 'k' || inputstr[i] == 'K') && (i > 0))
      {
        val = val << 10;
        *intnum = val;
        res = 1;
        break;
      }
      else if ((inputstr[i] == 'm' || inputstr[i] == 'M') && (i > 0))
      {
        val = val << 20;
        *intnum = val;
        res = 1;
        break;
      }
      else if (ISVALIDDEC(inputstr[i]))
      {
        val = val * 10 + CONVERTDEC(inputstr[i]);
      }
      else
      {
        /* return 0, Invalid input */
        res = 0;
        break;
      }
    }
    /* Over 10 digit decimal --invalid */
    if (i >= 11)
    {
      res = 0;
    }
  }

  return res;
}

/**
  * @brief  Get an integer from the HyperTerminal
  * @param  num: The inetger
  * @retval 1: Correct
  *         0: Error
  */
uint32_t GetIntegerInput(int32_t * num)
{
  uint8_t inputstr[16];

  while (1)
  {
    GetInputString(inputstr);
    if (inputstr[0] == '\0') continue;
    if ((inputstr[0] == 'a' || inputstr[0] == 'A') && inputstr[1] == '\0')
    {
      SerialPutString("User Cancelled \r\n");
      return 0;
    }

    if (Str2Int(inputstr, num) == 0)
    {
      SerialPutString("Error, Input again: \r\n");
    }
    else
    {
      return 1;
    }
  }
}

/**
  * @brief  Test to see if a key has been pressed on the HyperTerminal
  * @param  key: The key pressed
  * @retval 1: Correct
  *         0: Error
  */
uint32_t SerialKeyPressed(uint8_t *key)
{

//  if ( USART_GetFlagStatus(EVAL_COM1, USART_FLAG_RXNE) != RESET)
//  {
//    *key = (uint8_t)EVAL_COM1->DR;
//    return 1;
//  }
//  else
//  {
//    return 0;
//  }
	return ringbuf_read(key);
}

/**
  * @brief  Get a key from the HyperTerminal
  * @param  None
  * @retval The Key Pressed
  */
uint8_t GetKey(void)
{
  uint8_t key = 0;

  /* Waiting for user input */
  while (1)
  {
    if (SerialKeyPressed((uint8_t*)&key)) break;
  }
  return key;

}

/**
  * @brief  Print a character on the HyperTerminal
  * @param  c: The character to be printed
  * @retval None
  */
#include "main.h"
void SerialPutChar(uint8_t c)
{
  USART_SendData(TERM_UART, c);
  while (USART_GetFlagStatus(TERM_UART, USART_FLAG_TXE) == RESET)
  {
  }
}

/**
  * @brief  Print a string on the HyperTerminal
  * @param  s: The string to be printed
  * @retval None
  */
void Serial_PutString(uint8_t *s)
{
  while (*s != '\0')
  {
    SerialPutChar(*s);
    s++;
  }
}

/**
  * @brief  Get Input string from the HyperTerminal
  * @param  buffP: The input string
  * @retval None
  */
void GetInputString (uint8_t * buffP)
{
  uint32_t bytes_read = 0;
  uint8_t c = 0;
  do
  {
    c = GetKey();
    if (c == '\r')
      break;
    if (c == '\b') /* Backspace */
    {
      if (bytes_read > 0)
      {
        SerialPutString("\b \b");
        bytes_read --;
      }
      continue;
    }
    if (bytes_read >= CMD_STRING_SIZE )
    {
      SerialPutString("Command string size overflow\r\n");
      bytes_read = 0;
      continue;
    }
    if (c >= 0x20 && c <= 0x7E)
    {
      buffP[bytes_read++] = c;
      SerialPutChar(c);
    }
  }
  while (1);
  SerialPutString(("\n\r"));
  buffP[bytes_read] = '\0';
}

/**
  * @brief  Calculate the number of pages
  * @param  Size: The image size
  * @retval The number of pages
  */
uint32_t FLASH_PagesMask(__IO uint32_t Size)
{
  uint32_t pagenumber = 0x0;
  uint32_t size = Size;

  if ((size % PAGE_SIZE) != 0)
  {
    pagenumber = (size / PAGE_SIZE) + 1;
  }
  else
  {
    pagenumber = size / PAGE_SIZE;
  }
  return pagenumber;

}

///**
//  * @brief  Disable the write protection of desired pages
//  * @param  None
//  * @retval None
//  */
//void FLASH_DisableWriteProtectionPages(void)
//{
//  uint32_t useroptionbyte = 0, WRPR = 0;
//  uint16_t var1 = OB_IWDG_SW, var2 = OB_STOP_NoRST, var3 = OB_STDBY_NoRST;
//  FLASH_Status status = FLASH_BUSY;

//  WRPR = FLASH_GetWriteProtectionOptionByte();

//  /* Test if user memory is write protected */
//  if ((WRPR & UserMemoryMask) != UserMemoryMask)
//  {
//    useroptionbyte = FLASH_GetUserOptionByte();

//    UserMemoryMask |= WRPR;

//    status = FLASH_EraseOptionBytes();

//    if (UserMemoryMask != 0xFFFFFFFF)
//    {
//      status = FLASH_EnableWriteProtection((uint32_t)~UserMemoryMask);
//    }

//    /* Test if user Option Bytes are programmed */
//    if ((useroptionbyte & 0x07) != 0x07)
//    { 
//      /* Restore user Option Bytes */
//      if ((useroptionbyte & 0x01) == 0x0)
//      {
//        var1 = OB_IWDG_HW;
//      }
//      if ((useroptionbyte & 0x02) == 0x0)
//      {
//        var2 = OB_STOP_RST;
//      }
//      if ((useroptionbyte & 0x04) == 0x0)
//      {
//        var3 = OB_STDBY_RST;
//      }

//      FLASH_UserOptionByteConfig(var1, var2, var3);
//    }

//    if (status == FLASH_COMPLETE)
//    {
//      SerialPutString("Write Protection disabled...\r\n");

//      SerialPutString("...and a System Reset will be generated to re-load the new option bytes\r\n");

//      /* Generate System Reset to load the new option byte values */
//      NVIC_SystemReset();
//    }
//    else
//    {
//      SerialPutString("Error: Flash write unprotection failed...\r\n");
//    }
//  }
//  else
//  {
//    SerialPutString("Flash memory not write protected\r\n");
//  }
//}

/**
  * @}
  */

/*******************(C)COPYRIGHT 2010 STMicroelectronics *****END OF FILE******/
