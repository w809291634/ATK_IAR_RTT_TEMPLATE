/**
  ******************************************************************************
  * @file           stm32f1xx_serial.c
  * @author         古么宁
  * @brief          基于 LL 库的串口驱动具体实现。
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
#include <string.h>
#include <stdint.h>

#include "stm32f1xx_serial.h"

/** 
  * @brief 连接两个宏定义。会进行多次宏展开替换，比如
  *        已有 #define y 99 ，则 MACRO_CAT2(x,y) 会得到 x99 ,依次类推
*/
#define MACRO_CAT2_(x,y)        x##y
#define MACRO_CAT2(x,y)         MACRO_CAT2_(x,y)

///< @brief 连接三个宏定义
#define MACRO_CAT3_(x,y,z)      x##y##z
#define MACRO_CAT3(x,y,z)       MACRO_CAT3_(x,y,z)

///< 连接5个宏
#define MACRO_CAT5_(a,b,c,d,e)  a##b##c##d##e
#define MACRO_CAT5(a,b,c,d,e)   MACRO_CAT5_(a,b,c,d,e)


#define DMAx                    MACRO_CAT2(DMA,DMAn)                                ///< 串口所在 dma
#define USARTx                  MACRO_CAT2(USART,USARTn)                            ///< 引用串口
#define USART_IRQn              MACRO_CAT3(USART,USARTn,_IRQn)                      ///< 生成中断号 USARTx_IRQn
#define USART_IRQ_HANDLER       MACRO_CAT3(USART,USARTn,_IRQHandler)                ///< 中断函数名 USARTx_IRQHandler

#define DMA_TX_CHx              MACRO_CAT2(LL_DMA_CHANNEL_,DMATxCHn)                ///< 串口发送 dma 通道   LL_DMA_CHANNEL_x
#define DMA_TX_IRQn             MACRO_CAT5(DMA,DMAn,_Channel,DMATxCHn,_IRQn)       ///< 生成 DMA 中断函数号 DMAx_Channely_IRQn
#define DMA_TX_IRQ_HANDLER      MACRO_CAT5(DMA,DMAn,_Channel,DMATxCHn,_IRQHandler) ///< 生成 DMA 中断函数名 DMAx_Channely_IRQHandler
#define DMA_TX_CLEAR_FLAG_FN    MACRO_CAT2(LL_DMA_ClearFlag_TC,DMATxCHn)          ///< 清空 DMA 发送结束 flag

#define DMA_RX_CHx              MACRO_CAT2(LL_DMA_CHANNEL_,DMARxCHn)                ///< 串口发送 dma 通道   LL_DMA_CHANNEL_x
#define DMA_RX_IRQn             MACRO_CAT5(DMA,DMAn,_Channel,DMARxCHn,_IRQn)       ///< 生成 DMA 中断函数号 DMAx_Channely_IRQn
#define DMA_RX_IRQ_HANDLER      MACRO_CAT5(DMA,DMAn,_Channel,DMARxCHn,_IRQHandler) ///< 生成 DMA 中断函数名 DMAx_Channely_IRQHandler
#define DMA_RX_CLEAR_FLAG_FN    MACRO_CAT2(LL_DMA_ClearFlag_TC,DMARxCHn)          ///< 清空 DMA 发送结束 flag

#define DMA_SET_CLOCK_FN        MACRO_CAT3(LL_,DMA_CLOCK,_EnableClock)              ///< dma 开启时钟函数
#define DMA_CLOCK_PERIHGS       MACRO_CAT5(LL_,DMA_CLOCK,_PERIPH,_DMA,DMAn)         ///< dma 所在时钟源

#define TTYSxTXBUF              MACRO_CAT2(txbuf,USARTn)                            ///< 串口发送缓存命名
#define TTYSxRXBUF              MACRO_CAT2(rxbuf,USARTn)                            ///< 串口接收缓存命名
#define TTYSxHAL                MACRO_CAT2(halS,USARTn)                             ///< 串口硬件相关结构体命名
#define TTYSx                   MACRO_CAT2(ttyS,USARTn)                             ///< 串口对外可用结构体命名


///< 用循环直接对内存进行拷贝会比调用 memcpy 效率要高。
#define MEMCPY(_to,_from,size)\
do{\
	char *to=(char*)_to,*from=(char*)_from; \
	for (int i = 0 ; i <(size); ++i)        \
		*to++ = *from++ ;                   \
}while(0)

///< 修改全局变量时，上锁串口接收。主要是屏蔽中断
#define USART_RX_LOCK(H)   __disable_irq()
#define USART_RX_UNLOCK(H) __enable_irq()
#define USART_TX_LOCK(H)   __disable_irq()
#define USART_TX_UNLOCK(H) __enable_irq()

// DMA 设置接收下一包的地址 
#define DMA_NEXT_RECV(h,maddr,max)\
do{\
	LL_DMA_DisableChannel((h)->dma,(h)->dma_rx_channel);          \
	while(LL_DMA_IsEnabledChannel((h)->dma,(h)->dma_rx_channel)); \
	(h)->dma_rx_clear((h)->dma);                                  \
	LL_DMA_SetDataLength((h)->dma,(h)->dma_rx_channel,(max));     \
	LL_DMA_SetMemoryAddress((h)->dma,(h)->dma_rx_channel,(maddr));\
	LL_DMA_EnableChannel((h)->dma,(h)->dma_rx_channel);           \
}while(0)

static inline void dma_send_pkt(serial_t * ttySx);
static void usart_irq(serial_t *ttySx);
static void dma_tx_irq(serial_t *ttySx);
static void dma_rx_irq(serial_t *ttySx);

//---------------------HAL层相关--------------------------
// 如果要对硬件进行移植修改，修改下列宏

#if USE_USART1  // 如果启用串口 USARTn
	#define USARTn        1           ///< 引用串口号
	#define DMAn          1           ///< 对应 DMA 
	#define DMATxCHn      4           ///< 发送对应 DMA 通道号
	#define DMARxCHn      5           ///< 接收对应 DMA 通道号
	#define DMA_CLOCK     AHB1_GRP1   ///< DMAn 所在的总线

	#define DMARxEnable   1           ///< 为 1 时启用 dma 接收
	#define DMATxEnable   1           ///< 为 1 时启用 dma 发送
	#define IRQnPRIORITY  7           ///< 串口相关中断的优先级
	#define DMAxPRIORITY  6           ///< dma 相关中断的优先级
	#define RX_BUF_SIZE   256         ///< 硬件接收缓冲区
	#define TX_BUF_SIZE   512         ///< 硬件发送缓冲区

	///<  这个头文件仅供此 .c 文件内部使用，此头文件展开会生成串口对应的中断函数和硬件初始化函数
	#include "stm32f1xx_serial_func.h"
#endif


#if USE_USART2  // 如果启用串口 USARTn
	#define USARTn        2           ///< 引用串口号
	#define DMAn          1           ///< 对应 DMA 
	#define DMATxCHn      4           ///< 发送对应 DMA 通道号
	#define DMARxCHn      4           ///< 接收对应 DMA 通道号
	#define DMA_CLOCK     AHB1_GRP1   ///< DMAn 所在的总线

	#define DMARxEnable   1           ///< 为 1 时启用 dma 接收
	#define DMATxEnable   1           ///< 为 1 时启用 dma 发送
	#define IRQnPRIORITY  7           ///< 串口相关中断的优先级
	#define DMAxPRIORITY  6           ///< dma 相关中断的优先级
	#define RX_BUF_SIZE   256         ///< 硬件接收缓冲区
	#define TX_BUF_SIZE   512         ///< 硬件发送缓冲区

	///<  这个头文件仅供此 .c 文件内部使用，此头文件展开会生成串口对应的中断函数和硬件初始化函数
	#include "stm32f1xx_serial_func.h"
#endif


#if USE_USART3  // 如果启用串口 USARTn
	#define USARTn        3           ///< 引用串口号
	#define DMAn          1           ///< 对应 DMA 
	#define DMATxCHn      2           ///< 发送对应 DMA 通道号
	#define DMARxCHn      3           ///< 接收对应 DMA 通道号
	#define DMA_CLOCK     AHB1_GRP1   ///< DMAn 所在的总线

	#define DMARxEnable   1           ///< 为 1 时启用 dma 接收
	#define DMATxEnable   1           ///< 为 1 时启用 dma 发送
	#define IRQnPRIORITY  7           ///< 串口相关中断的优先级
	#define DMAxPRIORITY  6           ///< dma 相关中断的优先级
	#define RX_BUF_SIZE   256         ///< 硬件接收缓冲区
	#define TX_BUF_SIZE   512         ///< 硬件发送缓冲区

	///<  这个头文件仅供此 .c 文件内部使用，此头文件展开会生成串口对应的中断函数和硬件初始化函数
	#include "stm32f1xx_serial_func.h"
#endif



#if USE_USART4  // 如果启用串口 USARTn
	#define USARTn        4           ///< 引用串口号
	#define DMAn          1           ///< 对应 DMA 
	#define DMATxCHn      4           ///< 发送对应 DMA 通道号
	#define DMARxCHn      4           ///< 接收对应 DMA 通道号
	#define DMA_CLOCK     AHB1_GRP1   ///< DMAn 所在的总线

	#define DMARxEnable   1           ///< 为 1 时启用 dma 接收
	#define DMATxEnable   1           ///< 为 1 时启用 dma 发送
	#define IRQnPRIORITY  7           ///< 串口相关中断的优先级
	#define DMAxPRIORITY  6           ///< dma 相关中断的优先级
	#define RX_BUF_SIZE   256         ///< 硬件接收缓冲区
	#define TX_BUF_SIZE   512         ///< 硬件发送缓冲区

	///<  这个头文件仅供此 .c 文件内部使用，此头文件展开会生成串口对应的中断函数和硬件初始化函数
	#define USART UART
	#include "stm32f1xx_serial_func.h"
	#undef  USART
#endif


#if USE_USART5  // 如果启用串口 USARTn
	#define USARTn        5           ///< 引用串口号
	#define DMAn          1           ///< 对应 DMA 
	#define DMATxCHn      4           ///< 发送对应 DMA 通道号
	#define DMARxCHn      4           ///< 接收对应 DMA 通道号
	#define DMA_CLOCK     AHB1_GRP1   ///< DMAn 所在的总线

	#define DMARxEnable   1           ///< 为 1 时启用 dma 接收
	#define DMATxEnable   1           ///< 为 1 时启用 dma 发送
	#define IRQnPRIORITY  7           ///< 串口相关中断的优先级
	#define DMAxPRIORITY  6           ///< dma 相关中断的优先级
	#define RX_BUF_SIZE   256         ///< 硬件接收缓冲区
	#define TX_BUF_SIZE   512         ///< 硬件发送缓冲区

	///<  这个头文件仅供此 .c 文件内部使用，此头文件展开会生成串口对应的中断函数和硬件初始化函数
	#define USART UART
	#include "stm32f1xx_serial_func.h"
	#undef  USART
#endif


#if USE_USART6  ///<  如果启用串口
	#define USARTn        6           ///< 引用串口号
	#define DMAn          2           ///< 对应 DMA
	#define DMATxCHn      5           ///< 发送对应 DMA 通道号
	#define DMARxCHn      5           ///< 接收对应 DMA 通道号
	#define DMA_CLOCK     AHB1_GRP1   ///< DMAn 所在的总线

	#define DMARxEnable   1           ///< 为 1 时启用 dma 接收
	#define DMATxEnable   1           ///< 为 1 时启用 dma 发送
	#define IRQnPRIORITY  7           ///< 串口相关中断的优先级
	#define DMAxPRIORITY  6           ///< dma 相关中断的优先级
	#define RX_BUF_SIZE   256         ///< 硬件接收缓冲区
	#define TX_BUF_SIZE   512         ///< 硬件发送缓冲区

	///<  这个头文件仅供此 .c 文件内部使用，此头文件展开会生成串口对应的中断函数和硬件初始化函数
	#include "stm32f1xx_serial_func.h"
#endif

//------------------------------华丽的分割线------------------------------

/**
  * @author   古么宁
  * @brief    串口硬件配置
  * @param    ttySx    : 串口设备
  * @param    baud     : 波特率
  * @param    nbits    : 数据宽度
  * @param    parity   : 校验位
  * @param    nstop    : 停止位
  * @return   don't care
*/
static void serial_init(serial_t *ttySx, uint32_t baud, uint32_t nbits, uint32_t parity, uint32_t nstop)
{
	const struct serialhal * hal = ttySx->hal ;

	// ---------------- 串口参数配置 ----------------
	LL_USART_InitTypeDef USART_InitStruct;

	USART_InitStruct.BaudRate     = baud;
	USART_InitStruct.DataWidth    = nbits;
	USART_InitStruct.Parity       = parity;
	USART_InitStruct.StopBits     = nstop;
	USART_InitStruct.TransferDirection   = LL_USART_DIRECTION_TX_RX;
	USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
	LL_USART_Init(hal->uart, &USART_InitStruct);
	LL_USART_ConfigAsyncMode(hal->uart);

	NVIC_SetPriority(hal->uart_irq, 
		NVIC_EncodePriority(NVIC_GetPriorityGrouping(),hal->uart_priority, 0));
	NVIC_EnableIRQ(hal->uart_irq);

	LL_USART_DisableIT_PE(hal->uart);
	LL_USART_DisableIT_TC(hal->uart);
	LL_USART_EnableIT_IDLE(hal->uart);
	LL_USART_Enable(hal->uart);

	if (hal->dma_tx_enable) {
		hal->clockset(hal->dma_clock);
		hal->dma_tx_clear(hal->dma);
		LL_USART_EnableDMAReq_TX(hal->uart);
		LL_DMA_DisableChannel(hal->dma,hal->dma_tx_channel);//发送暂不使能
		while(LL_DMA_IsEnabledChannel(hal->dma,hal->dma_tx_channel));
		
		/* USART_TX Init */ 
		LL_DMA_SetDataTransferDirection(hal->dma,hal->dma_tx_channel, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
		LL_DMA_SetChannelPriorityLevel(hal->dma,hal->dma_tx_channel, LL_DMA_PRIORITY_MEDIUM);
		LL_DMA_SetMode(hal->dma,hal->dma_tx_channel, LL_DMA_MODE_NORMAL);
		LL_DMA_SetPeriphIncMode(hal->dma,hal->dma_tx_channel, LL_DMA_PERIPH_NOINCREMENT);
		LL_DMA_SetMemoryIncMode(hal->dma,hal->dma_tx_channel, LL_DMA_MEMORY_INCREMENT);
		LL_DMA_SetPeriphSize(hal->dma,hal->dma_tx_channel, LL_DMA_PDATAALIGN_BYTE);
		LL_DMA_SetMemorySize(hal->dma,hal->dma_tx_channel, LL_DMA_MDATAALIGN_BYTE);
		LL_DMA_SetPeriphAddress(hal->dma,hal->dma_tx_channel,LL_USART_DMA_GetRegAddr(hal->uart));

		/* DMA interrupt init 中断 */
		NVIC_SetPriority(hal->dma_tx_irq, 
			NVIC_EncodePriority(NVIC_GetPriorityGrouping(),hal->dma_priority, 0));
		NVIC_EnableIRQ(hal->dma_tx_irq);
		LL_DMA_EnableIT_TC(hal->dma,hal->dma_tx_channel);
	}

	if (hal->dma_rx_enable) {
		hal->clockset(hal->dma_clock);
		hal->dma_rx_clear(hal->dma);
		LL_USART_EnableDMAReq_RX(hal->uart);
		LL_DMA_DisableChannel(hal->dma,hal->dma_rx_channel);//发送暂不使能
		while(LL_DMA_IsEnabledChannel(hal->dma,hal->dma_rx_channel));

		/* USART_RX Init */
		LL_DMA_SetDataTransferDirection(hal->dma,hal->dma_rx_channel, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
		LL_DMA_SetChannelPriorityLevel(hal->dma,hal->dma_rx_channel, LL_DMA_PRIORITY_MEDIUM);
		LL_DMA_SetMode(hal->dma,hal->dma_rx_channel, LL_DMA_MODE_NORMAL);
		LL_DMA_SetPeriphIncMode(hal->dma,hal->dma_rx_channel, LL_DMA_PERIPH_NOINCREMENT);
		LL_DMA_SetMemoryIncMode(hal->dma,hal->dma_rx_channel, LL_DMA_MEMORY_INCREMENT);
		LL_DMA_SetPeriphSize(hal->dma,hal->dma_rx_channel, LL_DMA_PDATAALIGN_BYTE);
		LL_DMA_SetMemorySize(hal->dma,hal->dma_rx_channel, LL_DMA_MDATAALIGN_BYTE);
		LL_DMA_SetPeriphAddress(hal->dma,hal->dma_rx_channel,LL_USART_DMA_GetRegAddr(hal->uart));

		NVIC_SetPriority(hal->dma_rx_irq, 
			NVIC_EncodePriority(NVIC_GetPriorityGrouping(),hal->dma_priority, 0));
		NVIC_EnableIRQ(hal->dma_rx_irq);
		LL_DMA_EnableIT_TC(hal->dma,hal->dma_rx_channel);
		DMA_NEXT_RECV(hal,(uint32_t)ttySx->rxbuf,ttySx->rxmax);
	}
	else {
		LL_USART_EnableIT_RXNE(hal->uart);
	}
}



/**
  * @author   古么宁
  * @brief    串口硬件配置取消。关闭中断
  * @param    ttySx   : 串口设备
  * @return   don't care
*/
static void serial_deinit(serial_t *ttySx)
{
	const struct serialhal * hal = ttySx->hal ;

	NVIC_DisableIRQ(hal->uart_irq);
	LL_USART_Disable(hal->uart);
	LL_USART_DisableIT_IDLE(hal->uart);
	LL_USART_DisableIT_RXNE(hal->uart);
	LL_USART_DisableIT_TC(hal->uart);

	if (hal->dma_rx_enable) {
		LL_USART_DisableDMAReq_RX(hal->uart);
		LL_DMA_DisableIT_TC(hal->dma,hal->dma_rx_channel);
		NVIC_DisableIRQ(hal->dma_rx_irq);
	}

	if (hal->dma_tx_enable) {
		LL_USART_DisableDMAReq_TX(hal->uart);
		LL_DMA_DisableIT_TC(hal->dma,hal->dma_tx_channel);
		NVIC_DisableIRQ(hal->dma_tx_irq);
	}
}



/**
  * @brief    DMA 发送一包数据完成中断
  * @note     此时 DMA 把所有的数都传送到 USART->DR 寄存器，
  *           但还需要等待最后一个字节传输结束。
  * @param    ttySx   : 串口设备
  * @return   空
*/
static void dma_tx_irq(serial_t *ttySx)
{
	const struct serialhal * hal = ttySx->hal ;
	hal->dma_tx_clear(hal->dma);
	LL_USART_EnableIT_TC(hal->uart);
}

/**
  * @brief    串口 DMA 缓冲区接收满中断
  * @param    ttySx   : 串口设备
  * @return   空
*/
static void dma_rx_irq(serial_t *ttySx)
{
	const struct serialhal * hal = ttySx->hal ;
	ttySx->rxtail += ttySx->rxmax; //更新缓冲地址

	// 如果剩余空间不足以缓存最大包长度，从 0 开始
	// rxmax 为缓存区大小的一半，超过一半的时候即回归 0
	if (ttySx->rxtail > ttySx->rxmax) {
		ttySx->rxend  = ttySx->rxtail;
		ttySx->rxtail = 0;
	}

//	hal->dma_rx_clear(hal->dma); // 在 DMA_NEXT_RECV 清除标志
	DMA_NEXT_RECV(hal,(uint32_t)(&ttySx->rxbuf[ttySx->rxtail]),ttySx->rxmax);//设置缓冲地址和最大包长度

	if ((ttySx->flag & FLAG_RXSEM_POST) == FLAG_RXSEM_POST)// 如果是阻塞的，释放信号量
		OS_SEM_POST(ttySx->rxsem);
}

/**
  * @brief    串口中断处理函数
  * @param    ttySx   : 串口设备
  * @return   
*/
static void usart_irq(serial_t *ttySx)
{
	const struct serialhal * hal = ttySx->hal ;

	// 如果不启用 dma 接收，则开启接收中断 
	if (LL_USART_IsActiveFlag_RXNE(hal->uart)) {//接收单个字节中断
		LL_USART_ClearFlag_RXNE(hal->uart); //清除中断

		ttySx->rxbuf[ttySx->recving++] = LL_USART_ReceiveData8(hal->uart);//数据存入内存
		if (ttySx->recving - ttySx->rxtail >= ttySx->rxmax) {//接收到最大包长度
			ttySx->rxtail += ttySx->rxmax ; //更新缓冲尾部

			//如果剩余空间不足以缓存最大包长度，从 0 开始
			if (ttySx->rxtail > ttySx->rxmax) {
				ttySx->rxtail  = 0;
				ttySx->recving = 0;
			}

			if ((ttySx->flag & FLAG_RXSEM_POST) == FLAG_RXSEM_POST)
				OS_SEM_POST(ttySx->rxsem);// 如果是阻塞的，释放信号量
		}
	}

	if (LL_USART_IsActiveFlag_IDLE(hal->uart)) { //空闲中断
		LL_USART_ClearFlag_IDLE(hal->uart);      //清除空闲中断
		if (hal->dma_rx_enable){
			unsigned short pktlen = ttySx->rxmax - LL_DMA_GetDataLength(hal->dma,hal->dma_rx_channel);//得到当前包的长度
			if (pktlen) {
				ttySx->rxtail += pktlen ;	      // 更新缓冲地址
				if (ttySx->rxtail > ttySx->rxmax){// 如果剩余空间不足以缓存最大包长度，从 0 开始
					ttySx->rxend = ttySx->rxtail;
					ttySx->rxtail = 0;
				}
				
				DMA_NEXT_RECV(hal,(uint32_t)&(ttySx->rxbuf[ttySx->rxtail]),ttySx->rxmax);// 设置缓冲地址和最大包长度
				if ((ttySx->flag & FLAG_RXSEM_POST) == FLAG_RXSEM_POST)                  // 如果是阻塞的，释放信号量
					OS_SEM_POST(ttySx->rxsem);
			}
		}
		else {	
			unsigned short pktlen = ttySx->recving - ttySx->rxtail;//空闲中断长度
			if (pktlen) {
				ttySx->rxtail += pktlen ;	 //更新缓冲地址
				if (ttySx->rxtail > ttySx->rxmax){//如果剩余空间不足以缓存最大包长度，从 0 开始
					ttySx->rxend   = ttySx->rxtail;
					ttySx->rxtail  = 0;
					ttySx->recving = 0;
				}

				if ((ttySx->flag & FLAG_RXSEM_POST) == FLAG_RXSEM_POST)
					OS_SEM_POST(ttySx->rxsem);// 如果是阻塞的，释放信号量
			}
		}
	}

	if (LL_USART_IsActiveFlag_TC(hal->uart)) { // 发送中断
		LL_USART_ClearFlag_TC(hal->uart);      // 清除中断
		if (hal->dma_tx_enable) {              // 如果是非 dma 方式发送
			LL_DMA_DisableChannel(hal->dma,hal->dma_tx_channel); //停止 DMA
			LL_USART_DisableIT_TC(hal->uart);
			if (!ttySx->txsize)                    //发送完此包后无数据，复位缓冲区
				ttySx->txtail = 0;
			else                                   //还有数据则继续发送
				dma_send_pkt(ttySx); 
		}
		else { 
			if (0 == ttySx->txhead)
				ttySx->txhead = ttySx->txtail-ttySx->txsize + 1;
			else 
				ttySx->txhead++;

			if (ttySx->txhead < ttySx->txtail) { //如果未发送玩当前数据，发送下一个字节
				LL_USART_TransmitData8(hal->uart,(uint8_t)(ttySx->txbuf[ttySx->txhead]));
			} 
			else {
				ttySx->txsize = 0 ;
				ttySx->txtail = 0 ;
				ttySx->txhead = 0 ;
				LL_USART_DisableIT_TC(hal->uart);
			} 
		}
	}
}





/**
  * @brief    串口启动发送当前包
  * @param    ttySx : 串口设备
  * @retval   空
*/
static inline void dma_send_pkt(serial_t * ttySx)
{
	const struct serialhal * hal = ttySx->hal;
	uint32_t pkt_size = ttySx->txsize ;
	uint32_t pkt_head = ttySx->txtail - pkt_size ;
	ttySx->txsize = 0;
	while(LL_DMA_IsEnabledChannel(hal->dma,hal->dma_tx_channel));
	LL_DMA_SetDataLength(hal->dma,hal->dma_tx_channel,pkt_size);
	LL_DMA_SetMemoryAddress(hal->dma,hal->dma_tx_channel,
		(uint32_t)(&ttySx->txbuf[pkt_head]));
	LL_DMA_EnableChannel(hal->dma,hal->dma_tx_channel); 
}

/**
  * @brief    串口设备发送一包数据
  * @param    ttySx   : 串口设备
  * @param    data    : 需要发送的数据
  * @param    datalen : 需要发送的数据长度
  * @param    block   : O_BLOCK/O_NOBLOCK 是否阻塞
  * @return   实际从硬件发送出去的数据长度
*/
int serial_write(serial_t *ttySx , const void * data , int datalen , int block ) 
{
	const struct serialhal * hal = ttySx->hal;
	if (!(ttySx->flag & FLAG_INIT))    // 未初始化的设备 ，返回 -1
		return -1;

	if (O_NOBLOCK != block) {                  // 如果是阻塞发送，用最简单的方式发送
		uint8_t* databuf = (uint8_t *)data; 
		while(ttySx->txtail) ;                 // 等待缓冲区全部输出
		for (int i = 0 ; i < datalen ; ++i) {
			LL_USART_TransmitData8(hal->uart,databuf[i]);
			while(!LL_USART_IsActiveFlag_TXE(hal->uart));
		}
		return datalen;
	}

	int pkttail = ttySx->txtail;   //先获取当前尾部地址
	int remain  = ttySx->txmax - pkttail;
	int pktsize = (remain > datalen) ? datalen : remain;

	if (remain && datalen) {//当前发送缓存有空间
		MEMCPY(&ttySx->txbuf[pkttail] , data , pktsize);//把数据包拷到缓存区中
		if (hal->dma_tx_enable) {                       // dma 模式下发送
			USART_TX_LOCK(hal);                         // 修改全局变量，禁用中断。互斥
			ttySx->txtail  = pkttail + pktsize;         // 更新尾部
			ttySx->txsize += pktsize;                   // 设置当前包大小
			if (!LL_DMA_IsEnabledChannel(hal->dma,hal->dma_tx_channel))//开始发送
				dma_send_pkt(ttySx);
			USART_TX_UNLOCK(hal);
		}
		else  {
			if (!ttySx->txtail){
				LL_USART_TransmitData8(hal->uart,(uint8_t)(ttySx->txbuf[pkttail]));
				LL_USART_EnableIT_TC(hal->uart);
			}
			ttySx->txtail  = pkttail + pktsize;  // 更新尾部
			ttySx->txsize += pktsize;            // 设置当前包大小
		}
	}

	return pktsize;
}

/**
  * @brief    直接读取串口设备的接收缓存区数据
  * @param    ttySx   : 串口设备
  * @param    databuf : 返回串口设备接收缓存数据包首地址
  * @param    block   : O_BLOCK/O_NOBLOCK 是否阻塞
  * @return   接收缓存数据包长度
*/
int serial_gets(serial_t *ttySx ,char ** databuf , int block  )
{
	const struct serialhal * hal = ttySx->hal;
	int datalen ;
	
	if (!(ttySx->flag & FLAG_INIT))    // 未初始化的设备 ，返回 -1
		return -1;

	if ( ttySx->rxread == ttySx->rxtail ) {
		if ( O_NOBLOCK != block ) {
			ttySx->flag |= FLAG_RX_BLOCK ;    // 标记当前设备为正在阻塞读
			OS_SEM_WAIT(ttySx->rxsem) ;       // 如果是阻塞读，等待信号量
			ttySx->flag &= ~FLAG_RX_BLOCK ;   // 清楚阻塞读标志
			if (!(ttySx->flag & FLAG_INIT)) {                // 判断是否为退出信号
				OS_SEM_DEINIT(ttySx->rxsem);  // 退出时去初始化信号量
				return -1;                    // 返回错误
			}
		}
		else {
			return 0 ; // 非阻塞下无数据直接 return 0;
		}
	}
	
	USART_RX_LOCK(hal);
	
	// 接收到数据
	if (ttySx->rxtail < ttySx->rxread) {        // 接收到数据，且数据包是在缓冲区最后一段
		datalen  = ttySx->rxend - ttySx->rxread ; 
		*databuf = &ttySx->rxbuf[ttySx->rxread];   // 数据缓冲区
		ttySx->rxread = 0 ;
		ttySx->rxend = 0 ;
	}
	else { 
		datalen = ttySx->rxtail - ttySx->rxread;// 当前包数据大小
		*databuf = &ttySx->rxbuf[ttySx->rxread];// 数据缓冲区
		ttySx->rxread = ttySx->rxtail ;         // 数据读更新
	}
	USART_RX_UNLOCK(hal);
	
	return datalen ;
}

/**
  * @brief    读取串口设备接收，存于 databuf 中
  * @param    ttySx   : 串口设备
  * @param    databuf : 读取数据内存
  * @param    bufsize : 读取数据内存总大小
  * @param    block   : O_BLOCK/O_NOBLOCK 是否阻塞
  * @return   接收数据包长度
*/
int  serial_read(serial_t * ttySx ,void * databuf , int bufsize , int block )
{
	const struct serialhal * hal = ttySx->hal;
	int datalen ;
	char * data = NULL;
	if (!(ttySx->flag & FLAG_INIT))    // 未初始化的设备 ，返回 -1
		return -1;

	if (ttySx->rxread == ttySx->rxtail) { // 接收缓冲区中无接收数据
		if ( O_NOBLOCK != block ) {
			ttySx->flag |= FLAG_RX_BLOCK ;    // 标记当前设备为正在阻塞读
			OS_SEM_WAIT(ttySx->rxsem) ;       // 如果是阻塞读，等待信号量
			ttySx->flag &= ~FLAG_RX_BLOCK ;   // 清楚阻塞读标志
			if (!(ttySx->flag & FLAG_INIT)) {                // 判断是否为退出信号
				OS_SEM_DEINIT(ttySx->rxsem);  // 退出时去初始化信号量
				return -1;                    // 返回错误
			}
		}
		else {
			return 0 ; // 非阻塞下无数据直接 return 0;
		}
	}

	USART_RX_LOCK(hal);
	if (ttySx->rxread > ttySx->rxtail) {        // 接收到数据
		data = &ttySx->rxbuf[ttySx->rxread];   // 数据缓冲区
		if (ttySx->rxend > bufsize) {
			datalen = bufsize ; 
			ttySx->rxread += bufsize ;         // 数据读更新
			ttySx->rxend  -= bufsize;
		}
		else {
			datalen  = ttySx->rxend ; 
			ttySx->rxread = 0 ;
			ttySx->rxend = 0 ;
		}
	}
	else { // 接收到数据，且数据包是在缓冲区最后一段
		data = &ttySx->rxbuf[ttySx->rxread];   // 数据缓冲区
		datalen = ttySx->rxtail - ttySx->rxread;// 当前包数据大小
		if (datalen > bufsize)
			datalen = bufsize;
		ttySx->rxread += datalen ;         // 数据读更新
	}
	USART_RX_UNLOCK(hal);
	
	MEMCPY(databuf,data,datalen);
	return datalen ;
}


/**
  * @brief    打开串口设备，并进行初始化
  * @param    ttySx   : 串口设备
  * @param    speed   : 波特率
  * @param    bits    : 数据宽度
  * @param    event   : 校验位
  * @param    stop    : 停止位
  * @return   0
*/
int serial_open(serial_t *ttySx , int speed, int bits, char event, float stop) 
{
	uint32_t parity , nbits  , nstop; 
	ttySx->flag   = 0;
	ttySx->txhead = 0;
	ttySx->txtail = 0;
	ttySx->txsize = 0;
	ttySx->rxtail = 0;
	ttySx->rxend  = 0;
	ttySx->rxread = 0;
	ttySx->recving = 0 ;

	OS_SEM_INIT(ttySx->rxsem); 

	// 数据宽度，一般为 8
	nbits = (bits == 9) ? LL_USART_DATAWIDTH_9B : LL_USART_DATAWIDTH_8B;

	// 奇偶校验
	if (event == 'E')
		parity = LL_USART_PARITY_EVEN ;
	else 
	if (event == 'O')
		parity = LL_USART_PARITY_ODD ; 
	else 
		parity = LL_USART_PARITY_NONE ;

	// 停止位
	if (stop == 2.0f)
		nstop = LL_USART_STOPBITS_2 ;
	else 
	if (stop == 1.5f)
		nstop = LL_USART_STOPBITS_1_5 ;
	else 
	if (stop == 0.5f)
		nstop = LL_USART_STOPBITS_0_5 ;
	else 
		nstop = LL_USART_STOPBITS_1 ;

	serial_init(ttySx,speed,nbits,parity,nstop);
	ttySx->flag |= FLAG_INIT ;
	return 0;
}


/**
  * @brief    打开串口设备，并进行初始化
  * @param    ttySx       : 串口设备
  * @param    rxblock     : 调用 read/gets 读取数据时是否阻塞至有数据接收
  * @param    txblock     : 调用 write/puts 发送数据时是否阻塞至发送结束
  * @return   接收数据包长度
*/
int serial_close(serial_t *ttySx) 
{
	serial_deinit(ttySx);

	ttySx->flag &= ~FLAG_INIT;       // 去初始化
	if (ttySx->flag & FLAG_RX_BLOCK) // 如果有线程在进行阻塞读
		OS_SEM_POST(ttySx->rxsem);   // post 一个信号量让正在阻塞的任务退出
	else 
		OS_SEM_DEINIT(ttySx->rxsem); // 没有线程阻塞时直接删除当前信号
	
	ttySx->txhead = 0;
	ttySx->txtail = 0;
	ttySx->txsize = 0;
	ttySx->rxtail = 0;
	ttySx->rxend  = 0;
	ttySx->rxread = 0;
	ttySx->recving = 0 ;
	return 0;
}



