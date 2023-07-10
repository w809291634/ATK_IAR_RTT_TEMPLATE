/*
    FreeRTOS V9.0.0 - Copyright (C) 2016 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>>> AND MODIFIED BY <<<< the FreeRTOS exception.

    ***************************************************************************
    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<
    ***************************************************************************

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available on the following
    link: http://www.freertos.org/a00114.html

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that is more than just the market leader, it     *
     *    is the industry's de facto standard.                               *
     *                                                                       *
     *    Help yourself get started quickly while simultaneously helping     *
     *    to support the FreeRTOS project by purchasing a FreeRTOS           *
     *    tutorial book, reference manual, or both:                          *
     *    http://www.FreeRTOS.org/Documentation                              *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org/FAQHelp.html - Having a problem?  Start by reading
    the FAQ page "My application does not run, what could be wrong?".  Have you
    defined configASSERT()?

    http://www.FreeRTOS.org/support - In return for receiving this top quality
    embedded software for free we request you assist our global community by
    participating in the support forum.

    http://www.FreeRTOS.org/training - Investing in training allows your team to
    be as productive as possible as early as possible.  Now you can receive
    FreeRTOS training directly from Richard Barry, CEO of Real Time Engineers
    Ltd, and the world's leading authority on the world's leading RTOS.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
    Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.

    http://www.OpenRTOS.com - Real Time Engineers ltd. license FreeRTOS to High
    Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and commercial middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

/*
 * NOTE:  This file uses a third party USB CDC driver.
 */

/* Standard includes. */
#include "string.h"
#include "stdio.h"
#include  <stdarg.h>
/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* Example includes. */
#include "FreeRTOS_CLI.h"

/* Demo application includes. */
#include "serial.h"
#include "stm32f4xx_it.h"
#include "config.h"

/* Dimensions the buffer into which input characters are placed. */
#define cmdMAX_INPUT_SIZE		50

/* Dimentions a buffer to be used by the UART driver, if the UART driver uses a
buffer at all. */
#define cmdQUEUE_LENGTH			CMDQUEUE_LENGTH

/* DEL acts as a backspace. */
#define cmdASCII_DEL		( 0x7F )

/* The maximum time to wait for the mutex that guards the UART to become
available. */
#define cmdMAX_MUTEX_WAIT		pdMS_TO_TICKS( 300 )

#ifndef configCLI_BAUD_RATE
	#define configCLI_BAUD_RATE	115200
#endif

#ifdef USART1_USE_DMA_TX
extern char _CLI_DMA_SendBuff[USART1_USE_DMA_TX_SEND_BUF_SIZE];
extern unsigned int CLI_DMA_Send_len;
extern void CLI_uart_DMA_send(char *str,unsigned int ndtr);
#endif
/*-----------------------------------------------------------*/

/*
 * The task that implements the command console processing.
 */
static void prvUARTCommandConsoleTask( void *pvParameters );
//void vUARTCommandConsoleStart( uint16_t usStackSize, UBaseType_t uxPriority );

/*-----------------------------------------------------------*/

/* Const messages output by the command console. */
static const char * const pcWelcomeMessage = "FreeRTOS command server.\r\nType Help to view a list of registered commands.\r\n\r\n>";
static const char * const pcEndOfOutputMessage = "\r\n[Press ENTER to execute the previous command again]\r\n>";
static const char * const pcNewLine = "\r\n";

/* Used to guard access to the UART in case messages are sent to the UART from
more than one task. */
static SemaphoreHandle_t xTxMutex = NULL;

/* The handle to the UART port, which is not used by all ports. */
static xComPortHandle xPort = 0;

/*-----------------------------------------------------------*/

void vUARTCommandConsoleStart( uint16_t usStackSize, UBaseType_t uxPriority )
{
	/* Create the semaphore used to access the UART Tx. */
	xTxMutex = xSemaphoreCreateMutex();
	configASSERT( xTxMutex );
	/* 初始化对应的UART. */
	xPort = xSerialPortInitMinimal( configCLI_BAUD_RATE, CMDQUEUE_LENGTH_TX ,CMDQUEUE_LENGTH_RX);

	/* Create that task that handles the console itself. */
	xTaskCreate( 	prvUARTCommandConsoleTask,	/* The task that implements the command console. */
					"CLI",						/* Text name assigned to the task.  This is just to assist debugging.  The kernel does not use this name itself. */
					usStackSize,				/* The size of the stack allocated to the task. */
					NULL,						/* The parameter is not used, so NULL is passed. */
					uxPriority,					/* The priority allocated to the task. */
					NULL );						/* A handle is not required, so just pass NULL. */
}
/*-----------------------------------------------------------*/

static void prvUARTCommandConsoleTask( void *pvParameters )
{
signed char cRxedChar;
uint8_t ucInputIndex = 0;
char *pcOutputString;
static char cInputString[ cmdMAX_INPUT_SIZE ], cLastInputString[ cmdMAX_INPUT_SIZE ];
BaseType_t xReturned;
xComPortHandle xPort;

	( void ) pvParameters;

	/* Obtain the address of the output buffer.  Note there is no mutual
	exclusion on this buffer as it is assumed only one command console interface
	will be used at any one time. */
	pcOutputString = FreeRTOS_CLIGetOutputBuffer();

	/* Send the welcome message. */
	vSerialPutString( xPort, ( signed char * ) pcWelcomeMessage, ( unsigned short ) strlen( pcWelcomeMessage ) );

	for( ;; )
	{
#ifdef USART1_USE_DMA_TX
			/* 每次处理完命令后固定调用一次 */
			/* 有需要发送的字符，启动一次UART_DMA传输，注意一定不能长度为0，否则卡死 */
			if(CLI_DMA_Send_len!=0){
				CLI_uart_DMA_send(_CLI_DMA_SendBuff,CLI_DMA_Send_len);					//将临时缓存区里面的字符发送。
				memset(_CLI_DMA_SendBuff,'\0',USART1_USE_DMA_TX_SEND_BUF_SIZE);	//清零缓存区数据
				CLI_DMA_Send_len=0;
			}
#endif
		/* 等待下一个字符。如果一定时间没有获取到字符，进行DMA发送 */
		while( xSerialGetChar( xPort, &cRxedChar, 2 ) != pdPASS ){
#ifdef USART1_USE_DMA_TX
			/* 命令处理完成，可以发送字符 */
			/* 有需要发送的字符，启动一次UART_DMA传输，注意一定不能长度为0，否则卡死 */
			if(CLI_DMA_Send_len!=0){
				CLI_uart_DMA_send(_CLI_DMA_SendBuff,CLI_DMA_Send_len);					//将临时缓存区里面的字符发送。
				memset(_CLI_DMA_SendBuff,'\0',USART1_USE_DMA_TX_SEND_BUF_SIZE);	//清零缓存区数据
				CLI_DMA_Send_len=0;
			}
#endif
		};

		/* Ensure exclusive access to the UART Tx. */
		if( xSemaphoreTake( xTxMutex, cmdMAX_MUTEX_WAIT ) == pdPASS )
		{
			/* 输出输入的字符，将其显示 */
			xSerialPutChar( xPort, cRxedChar, portMAX_DELAY );

			/* 是不是一行的终端，表示开始处理对应的命令 */
			if( cRxedChar == '\n' || cRxedChar == '\r' )
			{
				/* 只是为了将输出与输入隔开。换行 */
				vSerialPutString( xPort, ( signed char * ) pcNewLine, ( unsigned short ) strlen( pcNewLine ) );

				/*查看命令是否为空，表示将再次执行最后一个命令。 */
				if( ucInputIndex == 0 )
				{
					/* 将最后一个命令复制回输入字符串。 */
					strcpy( cInputString, cLastInputString );
				}

				/* 将接收到的命令传递给命令解释器。反复调用命令解释器，直到它返回pdFALSE（表示没有更多输出），因为它可能会生成多个字符串。 */
				do
				{
					/* 从命令解释器获取下一个输出字符串。 */
					xReturned = FreeRTOS_CLIProcessCommand( cInputString, pcOutputString, configCOMMAND_INT_MAX_OUTPUT_SIZE );

					/* 将生成的字符串写入UART。 */
					vSerialPutString( xPort, ( signed char * ) pcOutputString, ( unsigned short ) strlen( pcOutputString ) );

				} while( xReturned != pdFALSE );

				/*输入命令生成的所有字符串都已发送。清除输入字符串，准备接收下一个命令。记住刚处理的命令，以防再次处理。*/
				strcpy( cLastInputString, cInputString );
				ucInputIndex = 0;
				memset( cInputString, 0x00, cmdMAX_INPUT_SIZE );
				
				/* 发送命令处理完成命令 */
				vSerialPutString( xPort, ( signed char * ) pcEndOfOutputMessage, ( unsigned short ) strlen( pcEndOfOutputMessage ) );
			}
			else   //删除按键，具体的字符按键储存
			{
				if( cRxedChar == '\r' )
				{
					/* Ignore the character. */
				}
				else if( ( cRxedChar == '\b' ) || ( cRxedChar == cmdASCII_DEL ) )
				{
					/* 已按下Backspace键。擦除字符串中的最后一个字符（如果有）. */
					if( ucInputIndex > 0 )
					{
						ucInputIndex--;
						cInputString[ ucInputIndex ] = '\0';
					}
				}
				else
				{
					/* 输入了一个字符。将其添加到目前输入的字符串中。当输入\n时，整个字符串将传递给命令解释器. */
					if( ( cRxedChar >= ' ' ) && ( cRxedChar <= '~' ) )
					{
						if( ucInputIndex < cmdMAX_INPUT_SIZE )
						{
							cInputString[ ucInputIndex ] = cRxedChar;
							ucInputIndex++;
						}
					}
				}
			}

			/* Must ensure to give the mutex back. */
			xSemaphoreGive( xTxMutex );
		}
	}
}
/*-----------------------------------------------------------*/
void vOutputString( const char * const pcMessage ,int length)
{
	configASSERT( xTxMutex );
	if(bool_is_TRQ()){
		vSerialPutString( xPort, ( signed char * ) pcMessage, ( unsigned short ) length );		//如果在中断中直接打印
	}
	else{
    // 这里的互斥量是为了保证一个任务中的信息能够正常输出完毕，不会被其他任务中断。
		if( xSemaphoreTake( xTxMutex, cmdMAX_MUTEX_WAIT ) == pdPASS )			
		{
			vSerialPutString( xPort, ( signed char * ) pcMessage, ( unsigned short ) length );
			xSemaphoreGive( xTxMutex );
		}
	}
}

//void vOutputString( const char * const pcMessage ,int length)
//{
//  vSerialPutString( xPort, ( signed char * ) pcMessage, ( unsigned short ) length );
//}

int xprintf(const char *fmt, ...)
{
    va_list args;
    unsigned int length;
    static char rt_log_buf[RT_CONSOLEBUF_SIZE];

		va_start(args, fmt);/* 初始化变量ap，让ap指向可变参数表里面的第一个参数 */
    length = vsnprintf(rt_log_buf, sizeof(rt_log_buf) - 1, fmt, args);
    if (length > RT_CONSOLEBUF_SIZE - 1)
        length = RT_CONSOLEBUF_SIZE - 1;

		va_end(args); /* 释放指针，将输入的参数 ap 置为 NULL */
		
    vOutputString(rt_log_buf,length);
    return length;
}
/*-----------------------------------------------------------*/

