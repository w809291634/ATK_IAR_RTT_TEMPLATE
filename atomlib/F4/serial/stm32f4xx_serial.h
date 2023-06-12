/**
  ******************************************************************************
  * @file           stm32f1xx_serial.h
  * @author         古么宁
  * @brief          串口驱动对外文件。
  * @note 
  * <pre>
  * 使用步骤：
  * 1> 此驱动不包含引脚驱动，因为引脚有肯能被各种重定向。所以第一步是在 stm32cubemx 中
  *    把串口对应的引脚使能为串口的输入输出，使能 usart 。注意不需要使能 usart 的 dma
  *    和中断，仅选择引脚初始化和串口初始化即可。然后选择库为 LL 库。生成工程。
  * 2> 把 stm32fxxx_serial.c 和头文件加入工程，其中 stm32fxxx_serial_func.h 仅为
  *     stm32fxxx_serial.c 内部使用。stm32fxxx_serial.h 为所有对外接口定义。
  * 3> include "stm32fxxx_serial.h" ,在此头文件下，打开所需要用的串口宏 USE_USARTx 
  *    后，调用 serial_open(&ttySx,...) ;打开串口后，便可调用 
  *    serial_write/serial_gets/serial_read 等函数进行操作。
  * #> 有需要则在 stm32fxxx_serial.c  修改串口配置参数，比如缓存大小，是否使能 DMA 等。
  * </pre>
  ******************************************************************************
  *
  * COPYRIGHT(c) 2018 GoodMorning
  *
  ******************************************************************************
  */
/* Includes ---------------------------------------------------*/
#ifndef _F4SERIAL_H_
#define _F4SERIAL_H_

#include "stm32f4xx_ll_bus.h"  ///< LL 库必须依赖项
#include "stm32f4xx_ll_usart.h"///< LL 库必须依赖项
#include "stm32f4xx_ll_dma.h"  ///< LL 库必须依赖项

#define SERIAL_OS    0 ///< 有无操作系统

#define USE_USART1   1 ///< 启用串口 1
#define USE_USART2   0 ///< 启用串口 2
#define USE_USART3   0 ///< 启用串口 3
#define USE_USART4   0 ///< 启用串口 4
#define USE_USART5   0 ///< 启用串口 5
#define USE_USART6   0 ///< 启用串口 6


#if SERIAL_OS  ///< 如果带操作系统，需要提供操作系统的信号量

	#include "cmsis_os.h"
	typedef osSemaphoreId serial_sem_t ;

	#define OS_SEM_POST(x)   osSemaphoreRelease((x))
	#define OS_SEM_WAIT(x)   osSemaphoreWait((x),osWaitForever)
	#define OS_SEM_DEINIT(x) vSemaphoreDelete((x))
	#define OS_SEM_INIT(x) \
	do {\
		osSemaphoreDef(Rx);\
		(x) = osSemaphoreCreate(osSemaphore(Rx), 1);\
	}while(0)

#else          ///< 无操作系统下，信号量相关被如下定义
	typedef char serial_sem_t ;
	#define OS_SEM_INIT(x)   do{(x) = 0;}while(0)
	#define OS_SEM_POST(x)   do{(x) = 1;}while(0)
	#define OS_SEM_WAIT(x)   do{for(int i=0;!(x);i++);(x)=0;}while(0)
	#define OS_SEM_DEINIT(x) OS_SEM_INIT(x)
#endif 


#define O_NOBLOCK   0
#define O_BLOCK     1
#define O_BLOCKING  0x40

#define tcdrain(x)     for (int i = 0 ; (x)->txtail ; i++)
#define serial_busy(x) ((x)->txtail != 0)

#define FLAG_INIT     (1)
#define FLAG_TX_BLOCK (1<<2)
#define FLAG_RX_BLOCK (1<<3)


#define FLAG_RXSEM_POST (FLAG_RX_BLOCK|FLAG_INIT)
#define FLAG_TXSEM_POST (FLAG_TX_BLOCK|FLAG_INIT)



typedef struct serialhal {
	DMA_TypeDef   * dma   ;   ///< 串口所在 DMA
	USART_TypeDef * uart  ;   ///< 串口

	void (*clockset)(uint32_t Periphs) ;

	void (*dma_tx_clear)(DMA_TypeDef *DMAx) ; ///< dma 发送结束清除标位置，一般为 LL_DMA_ClearFlag_TCx
	void (*dma_rx_clear)(DMA_TypeDef *DMAx) ; ///< dma 接收结束清除标位置，一般为 LL_DMA_ClearFlag_TCx

	uint32_t dma_clock;       ///< 
	uint32_t dma_tx_channel;  ///< dma 发送所在流
	uint32_t dma_tx_stream ;  ///< dma 发送所在通道

	uint32_t dma_rx_channel;  ///< dma 接收所在流
	uint32_t dma_rx_stream ;  ///< dma 接收所在通道

	IRQn_Type dma_tx_irq ;     ///< dma 发送通道中断号
	IRQn_Type dma_rx_irq ;     ///< dma 接收通道中断号
	IRQn_Type uart_irq   ;     ///< 串口中断号

	uint32_t dma_priority;    ///< dma 中断优先级
	uint32_t uart_priority;   ///< 串口中断优先级
	char     dma_rx_enable;   ///< 是否使能 DMA 接收
	char     dma_tx_enable;   ///< 是否使能 DMA 发送
}serialhal_t;



typedef struct serial {
	const struct serialhal * hal ;///< 硬件相关

	char * const   rxbuf ;     ///< 串口接收缓冲区指针
	char * const   txbuf ;     ///< 串口发送缓冲区首地址
	const unsigned short txmax;///< 串口一包最大发送，一般为缓冲区大小
	const unsigned short rxmax;///< 串口一包最大接收，一般为缓冲区大小的一半

	volatile unsigned short txhead ;    ///< 串口当前发送头
	volatile unsigned short txsize ;    ///< 串口当前发送包大小
	volatile unsigned short txtail ;    ///< 串口当前发送缓冲区尾部

	volatile unsigned short rxtail ;    ///< 串口当前接收缓冲区尾部
	volatile unsigned short rxread ;    ///< 串口未读数据头部
	volatile unsigned short recving;    ///< 正在进行接收
	
	// 串口在缓冲区最后一包数据包尾部。当 &rxbuf[rxtail] 后面内存不足以存放
	// 一包数据 rxmax 大小时，rxtail 会清零，此时需要记下来当前包的大小
	volatile unsigned short rxend  ;

	char         flag   ;    ///< 一些状态值

	serial_sem_t rxsem   ;   ///< 信号量，表示缓冲区中有数据可读取 
}
serial_t;


/**
  * @brief    打开串口设备，并进行初始化
  * @param    ttySx   : 串口设备
  * @param    speed   : 波特率
  * @param    bits    : 数据宽度
  * @param    event   : 校验位
  * @param    stop    : 停止位
  * @return   0
*/
int serial_open(serial_t *ttySx , int speed, int bits, char event, float stop);

/**
  * @brief    串口设备发送一包数据
  * @param    ttySx   : 串口设备
  * @param    data    : 需要发送的数据
  * @param    datalen : 需要发送的数据长度
  * @param    block   : O_BLOCK/O_NOBLOCK 是否阻塞
  * @return   实际从硬件发送出去的数据长度
*/
int serial_write(serial_t *ttySx , const void * data , int wrtlen, int block );
#define serial_puts(t,d,l,b) serial_write(t,d,l,b)


/**
  * @brief    读取串口设备接收，存于 databuf 中
  * @param    ttySx   : 串口设备
  * @param    databuf : 读取数据内存
  * @param    bufsize : 读取数据内存总大小
  * @param    block   : O_BLOCK/O_NOBLOCK 是否阻塞
  * @return   接收数据包长度
*/
int serial_read(serial_t *ttySx , void * databuf , int bufsize, int block  );

/**
  * @brief    直接读取串口设备的接收缓存区数据
  * @param    ttySx   : 串口设备
  * @param    databuf : 返回串口设备接收缓存数据包首地址
  * @param    block   : O_BLOCK/O_NOBLOCK 是否阻塞
  * @return   接收缓存数据包长度
*/
int serial_gets(serial_t *ttySx , char ** databfuf ,int block );

// 关闭设备，去初始化
int serial_close(serial_t *ttySx);


#if (USE_USART1)
	extern serial_t ttyS1;
#endif 

#if (USE_USART2)
	extern serial_t ttyS2;
#endif 

#if (USE_USART3)
	extern serial_t ttyS3;
#endif 

#if (USE_USART4)
	extern serial_t ttyS4;
#endif 

#if (USE_USART5)
	extern serial_t ttyS5;
#endif 

#if (USE_USART6)
	extern serial_t ttyS6;
#endif 

#endif
