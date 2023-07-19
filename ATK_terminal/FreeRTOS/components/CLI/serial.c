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
	BASIC INTERRUPT DRIVEN SERIAL PORT DRIVER FOR UART0.
*/

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
/* Library includes. */
/* Hardware and starter kit includes. */
#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"

/* Demo application includes. */
#include "serial.h"
#include "stdio.h"
#include "string.h"
#include "config.h"
/*-----------------------------------------------------------*/

/* Misc defines. */
#define serINVALID_QUEUE				( ( QueueHandle_t ) 0 )
#define serNO_BLOCK						( ( TickType_t ) 0 )
#define serTX_BLOCK_TIME				( 40 / portTICK_PERIOD_MS )

/*-----------------------------------------------------------*/

/* The queue used to hold received characters. */
static QueueHandle_t xRxedChars;
#ifdef USART1_USE_DMA_RX
static char DMA_ReceiveBuff[USART1_USE_DMA_RX_RECEIVE_BUF_SIZE]={0};
#endif
#ifdef USART1_USE_DMA_TX
static char CLI_DMA_SendBuff[USART1_USE_DMA_TX_SEND_BUF_SIZE]={0};       //用于DMA传输的MEM
char _CLI_DMA_SendBuff[USART1_USE_DMA_TX_SEND_BUF_SIZE]={0};							//用户缓存发送的字符串
unsigned int CLI_DMA_Send_len=0;
SemaphoreHandle_t CLI_uartDMATCSemaphore;
#else
#ifdef USART1_USE_IT_TXE
static QueueHandle_t xCharsForTx;
#endif
#endif

#ifdef USART1_USE_DMA_RX
/*-------------------------串口DMA接收配置----------------------------------*/
static void UART_DMA_RxConfig(DMA_Stream_TypeDef *DMA_Streamx,u32 channel,u32 addr_Peripherals,u32 addr_Mem,u16 Length)                                       //DMA的初始化
{
//	NVIC_InitTypeDef NVIC_InitStructure;
	DMA_InitTypeDef UARTDMA_InitStructure;

	if((u32)DMA_Streamx>(u32)DMA2)//DMA1?DNA2
	{
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2,ENABLE);//DMA2 	
	}
	else 
	{
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1,ENABLE);//DMA1
	}                 

  DMA_DeInit(DMA_Streamx);
	while (DMA_GetCmdStatus(DMA_Streamx) != DISABLE){}//等待DMA可配置 
		

  UARTDMA_InitStructure.DMA_Channel = channel;																	//通道
  UARTDMA_InitStructure.DMA_PeripheralBaseAddr = ( uint32_t)(addr_Peripherals) ;//外设地址
  UARTDMA_InitStructure.DMA_Memory0BaseAddr =   ( uint32_t)addr_Mem; 						//缓存地址
  UARTDMA_InitStructure.DMA_DIR =  DMA_DIR_PeripheralToMemory;									//方向-外设到内存
  UARTDMA_InitStructure.DMA_BufferSize = Length; 																//传输长度
  UARTDMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;					//外设非增量模式
  UARTDMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;										//存储器增量模式
  UARTDMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte ;	//外设数据长度:8位
  UARTDMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte ;					//存储器数据长度:8位
  UARTDMA_InitStructure.DMA_Mode = DMA_Mode_Normal   ;													//使用普通模式 
  UARTDMA_InitStructure.DMA_Priority = DMA_Priority_Medium;											//中等优先级
  UARTDMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
  UARTDMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
  UARTDMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single ;							//存储器突发单次传输
  UARTDMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;				//外设突发单次传输
  DMA_Init(DMA_Streamx, &UARTDMA_InitStructure);																//初始化DMA Stream

		// 这里没有使用到DMA的接收中断
//  NVIC_InitStructure.NVIC_IRQChannel = DMA_USART_RX_IRQ ;                //这里使用串口的空闲中断，所以这里就没有开启
//  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
//  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//  NVIC_Init(&NVIC_InitStructure);
//  DMA_ITConfig(DMA_Streamx, DMA_IT_TC, ENABLE);//开启传输完成中断触发   //DMA中断使能

  /* DMA2 Stream3  or Stream6 enable */

  DMA_Cmd(DMA_Streamx, ENABLE);     																			//DMA开启  
}
#endif

#ifdef USART1_USE_DMA_TX
/*-------------------------串口DMA发送配置----------------------------------*/
static void UART_DMA_TxConfig(DMA_Stream_TypeDef *DMA_Streamx,u32 channel,u32 addr_Peripherals,u32 addr_Mem,u16 Length)                                       //DMA的初始化
{
//	NVIC_InitTypeDef NVIC_InitStructure;
	DMA_InitTypeDef UARTDMA_InitStructure={0};
	NVIC_InitTypeDef NVIC_InitStructure={0};

	if((u32)DMA_Streamx>(u32)DMA2)//DMA1?DNA2
	{
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2,ENABLE);//DMA2 	
	}
	else 
	{
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1,ENABLE);//DMA1
	}                 

  DMA_DeInit(DMA_Streamx);
	while (DMA_GetCmdStatus(DMA_Streamx) != DISABLE){}//等待DMA可配置 

  UARTDMA_InitStructure.DMA_Channel = channel;																	//通道
  UARTDMA_InitStructure.DMA_PeripheralBaseAddr = ( uint32_t)(addr_Peripherals) ;//外设地址
  UARTDMA_InitStructure.DMA_Memory0BaseAddr =   ( uint32_t)addr_Mem; 						//缓存地址
  UARTDMA_InitStructure.DMA_DIR =  DMA_DIR_MemoryToPeripheral;									//存储器到外设模式
  UARTDMA_InitStructure.DMA_BufferSize = Length; 																//传输长度
  UARTDMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;					//外设非增量模式
  UARTDMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;										//存储器增量模式
  UARTDMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte ;	//外设数据长度:8位
  UARTDMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte ;					//存储器数据长度:8位
  UARTDMA_InitStructure.DMA_Mode = DMA_Mode_Normal   ;													//使用普通模式 
  UARTDMA_InitStructure.DMA_Priority = DMA_Priority_Medium;											//中等优先级
  UARTDMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
  UARTDMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
  UARTDMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single ;							//存储器突发单次传输
  UARTDMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;				//外设突发单次传输
  DMA_Init(DMA_Streamx, &UARTDMA_InitStructure);																//初始化DMA Stream

	//中断配置
	DMA_ITConfig(DMA_Streamx,DMA_IT_TC,ENABLE);  							//配置DMA发送完成后产生中断
	NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream7_IRQn;		//
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=7;		//抢占优先级7
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =0;       	//子优先级0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;         	//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure); 													//根据指定的参数初始化VIC寄存器
	
	USART_DMACmd(USART1,USART_DMAReq_Tx,ENABLE);  						//使能串口1的DMA发送
	DMA_Cmd (DMA_Streamx,DISABLE);														//先不要使能DMA！ 
}
#endif

/*
 * See the serial1.h header file.  串口1
 */
xComPortHandle xSerialPortInitMinimal( unsigned long ulWantedBaud, unsigned portBASE_TYPE TxQueueLength ,unsigned portBASE_TYPE RxQueueLength)
{
	xComPortHandle xReturn;
	USART_InitTypeDef USART_InitStructure={0};
	NVIC_InitTypeDef NVIC_InitStructure={0};
	GPIO_InitTypeDef GPIO_InitStructure={0};

	/* Create the queues used to hold Rx/Tx characters. */
	xRxedChars = xQueueCreate( RxQueueLength, ( unsigned portBASE_TYPE ) sizeof( signed char ) );
	configASSERT(xRxedChars);
#ifdef USART1_USE_DMA_TX	
	CLI_uartDMATCSemaphore=xSemaphoreCreateBinary();
	configASSERT(CLI_uartDMATCSemaphore);
	xSemaphoreGive(CLI_uartDMATCSemaphore);					//释放一个信号量
#else 
#ifdef USART1_USE_IT_TXE
	xCharsForTx = xQueueCreate( TxQueueLength + 1, ( unsigned portBASE_TYPE ) sizeof( signed char ) );
	configASSERT(xCharsForTx);
#endif
#endif	
		/* Enable USART1 clock */
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE); //使能GPIOA时钟
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);//使能USART1时钟

		//串口1对应引脚复用映射
		GPIO_PinAFConfig(GPIOA,GPIO_PinSource9,GPIO_AF_USART1); //GPIOA9复用为USART1
		GPIO_PinAFConfig(GPIOA,GPIO_PinSource10,GPIO_AF_USART1); //GPIOA10复用为USART1
			
		//USART1端口配置
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10; //GPIOA9与GPIOA10
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//速度50MHz
		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //推挽复用输出
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //上拉
		GPIO_Init(GPIOA,&GPIO_InitStructure); //初始化PA9，PA10
			
		//USART1 初始化设置
		USART_InitStructure.USART_BaudRate = ulWantedBaud;//波特率设置
		USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
		USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
		USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
		USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
		USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
		
		USART_Init(USART1, &USART_InitStructure); //初始化串口1	

#ifdef USART1_USE_DMA_TX																	//串口控制台使用DMA发送
		// DMA发送配置
		UART_DMA_TxConfig(USART1_USE_DMA_TX_STREAM,						// DMA串口接收流号码
											USART1_USE_DMA_TX_CHANNEL,					// DMA流对应的通道
											(u32)&USART1->DR,										// 寄存器
											(u32)CLI_DMA_SendBuff,									//  DMA接收区域
											USART1_USE_DMA_TX_SEND_BUF_SIZE);

#endif	
#ifdef USART1_USE_DMA_RX																	//串口控制台使用DMA接收
		// DMA接收配置
		UART_DMA_RxConfig(USART1_USE_DMA_RX_STREAM,						// DMA串口接收流号码
											USART1_USE_DMA_RX_CHANNEL,					// DMA流对应的通道
											(u32)&USART1->DR,										// 寄存器
											(u32)DMA_ReceiveBuff,								//  DMA接收区域
											USART1_USE_DMA_RX_RECEIVE_BUF_SIZE);
		USART_DMACmd(USART1, USART_DMAReq_Rx , ENABLE);      										//使能串口1的DMA接收
		// 配置中断
		USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);				//开启空闲中断

		// Usart1 NVIC 配置
		NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;			//串口1中断通道
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=USART1_INT_PRIORITY;//**抢占优先级6  ，这里的优先级必须小于系统时钟的优先级
		NVIC_InitStructure.NVIC_IRQChannelSubPriority =0;		//子优先级
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
		NVIC_Init(&NVIC_InitStructure);											//根据指定的参数初始化VIC寄存器	

#else
		// 配置中断
		USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);				//开启相关中断

		// Usart1 NVIC 配置
		NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;			//串口1中断通道
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=6;//**抢占优先级6  ，这里的优先级必须小于系统时钟的优先级
		NVIC_InitStructure.NVIC_IRQChannelSubPriority =0;			//子优先级
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//IRQ通道使能
		NVIC_Init(&NVIC_InitStructure);												//根据指定的参数初始化VIC寄存器	
		
#endif		
	USART_Cmd(USART1, ENABLE);  //使能串口1 	

	/* This demo file only supports a single port but we have to return
	something to comply with the standard demo header file. */
	return xReturn;
}

#ifdef USART1_USE_DMA_TX
/*----------------------开启一次DMA发送-------------------------*/
void CLI_uart_DMA_send(char *str,unsigned int len)
{
    while(xSemaphoreTake(CLI_uartDMATCSemaphore,2)!=pdTRUE);				//获取信号量，等待DMA发送可用
	
		// 复位并启动DMA发送
		DMA_ClearFlag(USART1_USE_DMA_TX_STREAM,DMA_FLAG_TCIF7 | DMA_FLAG_FEIF7 | DMA_FLAG_DMEIF7 | DMA_FLAG_TEIF7 | DMA_FLAG_HTIF7);
		DMA_Cmd(USART1_USE_DMA_TX_STREAM, DISABLE);                     //关闭DMA传输 
    DMA_SetCurrDataCounter(USART1_USE_DMA_TX_STREAM,len);          	//设置数据传输量 
		memcpy(CLI_DMA_SendBuff,str,len);																//准备传输数据
    DMA_Cmd(USART1_USE_DMA_TX_STREAM, ENABLE);                      //开启DMA传输 
}
#endif

signed portBASE_TYPE xSerialGetChar( xComPortHandle pxPort, signed char *pcRxedChar, TickType_t xBlockTime )
{
	/* The port handle is not required as this driver only supports one port. */
	( void ) pxPort;

	/* Get the next character from the buffer.  Return false if no characters
	are available, or arrive before xBlockTime expires. */
	if( xQueueReceive( xRxedChars, pcRxedChar, xBlockTime ) )
	{
		return pdTRUE;
	}
	else
	{
		return pdFALSE;
	}
}
/*-----------------------------------------------------------*/

void vSerialPutString( xComPortHandle pxPort, const signed char * const pcString, unsigned short usStringLength )
{
signed char *pxNext;

	/* A couple of parameters that this port does not use. */
	( void ) usStringLength;
	( void ) pxPort;

	/* NOTE: This implementation does not handle the queue being full as no
	block time is used! */

	/* The port handle is not required as this driver only supports UART1. */
	( void ) pxPort;

	/* Send each character in the string, one at a time. */
	pxNext = ( signed char * ) pcString;
//	while (usStringLength		//rt-thread
//  {
//		xSerialPutChar( pxPort, *pxNext, serNO_BLOCK );
//		++ pxNext;
//		-- usStringLength;
//	}
	while( *pxNext )
	{
		xSerialPutChar( pxPort, *pxNext, serNO_BLOCK );
		pxNext++;
	}
}


/*-----------------------------------------------------------*/
signed portBASE_TYPE xSerialPutChar( xComPortHandle pxPort, signed char cOutChar, TickType_t xBlockTime )
{
	signed portBASE_TYPE xReturn;

#ifdef USART1_USE_DMA_TX
	if(CLI_DMA_Send_len<=USART1_USE_DMA_TX_SEND_BUF_SIZE){
		_CLI_DMA_SendBuff[CLI_DMA_Send_len++]=cOutChar;
	}
	else{
		memset(_CLI_DMA_SendBuff,'\0',USART1_USE_DMA_TX_SEND_BUF_SIZE);	//清零缓存区数据
		CLI_DMA_Send_len=0;
		xprintf("error,send_buff size to small \r\n");
	} 
	xReturn = pdPASS;
#else
  
#ifdef USART1_USE_IT_TXE
	if( xQueueSend( xCharsForTx, &cOutChar, xBlockTime ) == pdPASS )
	{
		xReturn = pdPASS;
		USART_ITConfig( USART1, USART_IT_TXE, ENABLE );	
	}
	else
	{
		xReturn = pdFAIL;
	}
#else
    USART_ClearFlag(USART1, USART_FLAG_TC);
    USART1->DR = cOutChar;
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
	
#endif  //USART1_USE_IT_TXE
#endif  //USART1_USE_DMA_TX
	return xReturn;
}
/*-----------------------------------------------------------*/

void vSerialClose( xComPortHandle xPort )
{
	/* Not supported as not required by the demo application. */
}
/*-----------------------------------------------------------*/

#ifdef USART1_USE_DMA_TX
void DMA2_Stream7_IRQHandler(void) 
{
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	if(DMA_GetITStatus(USART1_USE_DMA_TX_STREAM,DMA_IT_TCIF7)!= RESET) 					//检查DMA传输完成中断 DMA_IT_TCIF7
	{
			DMA_ClearITPendingBit(USART1_USE_DMA_TX_STREAM,DMA_IT_TCIF7); 
			if(CLI_uartDMATCSemaphore!=NULL)
			{
					xSemaphoreGiveFromISR(CLI_uartDMATCSemaphore,&xHigherPriorityTaskWoken);    //释放DMA传输完成二值信号量
			}
	}
	portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );											//如果需要的话进行一次任务切换
}
#endif

void USART1_IRQHandler(void) 
{
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	uint16_t Count=0;
	int i = 0;
#ifndef USART1_USE_DMA_TX
#ifdef USART1_USE_IT_TXE
	
	if( USART_GetITStatus( USART1, USART_IT_TXE ) == SET )
	{
		/* The interrupt was caused by the THR becoming empty.  Are there any
		more characters to transmit? */
		if( xQueueReceiveFromISR( xCharsForTx, &cChar, &xHigherPriorityTaskWoken ) == pdTRUE )
		{
			/* A character was retrieved from the queue so can be sent to the
			THR now. */
			USART_SendData( USART1, cChar );
		}
		else
		{
			USART_ITConfig( USART1, USART_IT_TXE, DISABLE );		
		}		
	}
#endif
#endif
#ifdef USART1_USE_DMA_RX
	if( USART_GetITStatus( USART1, USART_IT_IDLE ) == SET )
	{	
		//清楚标志位
		USART1->SR;
    USART1->DR;    
		//获取接收缓存区已使用的长度
		Count=USART1_USE_DMA_RX_RECEIVE_BUF_SIZE-DMA_GetCurrDataCounter(USART1_USE_DMA_RX_STREAM);
		for(i = 0; i<Count;i++)
		{
		  xQueueSendFromISR(xRxedChars, &DMA_ReceiveBuff[i], &xHigherPriorityTaskWoken);		
		}

		// DMA接收复位
		DMA_Cmd(USART1_USE_DMA_RX_STREAM, DISABLE);     												//DMA关闭 
		// 清除所有状态位
		DMA_ClearFlag(USART1_USE_DMA_RX_STREAM,DMA_FLAG_TCIF5 | DMA_FLAG_FEIF5 | DMA_FLAG_DMEIF5 | DMA_FLAG_TEIF5 | DMA_FLAG_HTIF5);
		DMA_SetCurrDataCounter(USART1_USE_DMA_RX_STREAM,USART1_USE_DMA_RX_RECEIVE_BUF_SIZE);
		DMA_Cmd(USART1_USE_DMA_RX_STREAM, ENABLE);     													//DMA开始
	}	
#else
	char cChar;
	if( USART_GetITStatus( USART1, USART_IT_RXNE ) == SET )
	{
		cChar = USART_ReceiveData( USART1 );
		xQueueSendFromISR( xRxedChars, &cChar, &xHigherPriorityTaskWoken );
	}	
#endif
	portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}

