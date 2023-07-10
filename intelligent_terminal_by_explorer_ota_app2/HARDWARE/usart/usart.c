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
#include "usart.h"
#include "stdio.h"
#include "string.h"
#include "serial.h"
#include "config.h"

#define UART3_TX_CONNFIG IO_PIN(B,10)
#define UART3_RX_CONNFIG IO_PIN(B,11)

uart_config uart3_config[]={UART3_TX_CONNFIG,UART3_RX_CONNFIG};
QueueHandle_t uart3_RxQueue;

void uart3_init(u32 bound)
{
   //GPIO端口设置
  GPIO_InitTypeDef GPIO_InitStructure={0};
	USART_InitTypeDef USART_InitStructure={0};
	NVIC_InitTypeDef NVIC_InitStructure={0};
  
  uart3_RxQueue = xQueueCreate( CMDQUEUE_LENGTH_RX, ( unsigned portBASE_TYPE ) sizeof( signed char ) );
  configASSERT(uart3_RxQueue);
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB,ENABLE); //使能GPIOB时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3,ENABLE);//使能USART3时钟
 
	GPIO_PinAFConfig(uart3_config[0].gpio,uart3_config[0].PinSource,GPIO_AF_USART3); 
	GPIO_PinAFConfig(uart3_config[1].gpio,uart3_config[1].PinSource,GPIO_AF_USART3); 
	
  GPIO_InitStructure.GPIO_Pin = uart3_config[0].pin | uart3_config[1].pin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;          //复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	    //速度50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;      //推挽复用输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;          //上拉
	GPIO_Init(uart3_config[0].gpio,&GPIO_InitStructure); 

	USART_InitStructure.USART_BaudRate = bound;//波特率设置
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
  USART_Init(USART3, &USART_InitStructure); 
	
  USART_Cmd(USART3, ENABLE); 
	
//	USART_ClearFlag(USART3, USART_FLAG_TC);
	
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);//开启相关中断

	//Usart3 NVIC 配置
  NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=USART3_INT_PRIORITY;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =0;		  
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	                      //根据指定的参数初始化VIC寄存器
}

void USART3_IRQHandler(void)                	
{
  // 进入关键区
  taskENTER_CRITICAL_FROM_ISR();
	char cChar;	
  portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	if( USART_GetITStatus( USART3, USART_IT_RXNE ) == SET )
	{
		cChar = USART_ReceiveData( USART3 );
		xQueueSendFromISR( uart3_RxQueue, &cChar, &xHigherPriorityTaskWoken );  // 使用这个API可能导致一些阻塞任务就绪
	}	
  portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );      // 主动进行任务切换
  taskEXIT_CRITICAL_FROM_ISR(0);
  // 退出关键区
  
  // 测试使用的代码
//  unsigned int st_time=0;
//  st_time=xtime();
//  xprintf("usart3 use time:%d\r\n",xtime()-st_time);  //3-5us
} 

// 此函数只能在任务中使用
void USART3_putstring(char *data)                	
{
	while(*data)
	{
    USART_ClearFlag(USART3, USART_FLAG_TC);
    USART3->DR = *data;
    while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET); 
		data++;
	}
} 

