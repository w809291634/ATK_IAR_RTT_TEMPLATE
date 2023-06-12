/**
  ******************************************************************************
  * @file           stm32f4xx_serial_func.h
  * @author         古么宁
  * @brief          串口相关函数生成头文件。
  *                 此头文件可以被多次 include 展开，仅供 serial_f4xx.c 使用
  ******************************************************************************
  *
  * COPYRIGHT(c) 2018 GoodMorning
  *
  ******************************************************************************
  */

static char TTYSxRXBUF[RX_BUF_SIZE] = {0};
static char TTYSxTXBUF[TX_BUF_SIZE] = {0};

/* 
  * @brief 生成一个硬件相关结构体常量
*/
static const struct serialhal TTYSxHAL = {
	.dma            = DMAx                 ,
	.uart           = USARTx               ,
	.clockset       = DMA_SET_CLOCK_FN     ,
	.dma_tx_clear   = DMA_TX_CLEAR_FLAG_FN ,
	.dma_rx_clear   = DMA_RX_CLEAR_FLAG_FN ,
	.dma_clock      = DMA_CLOCK_PERIHGS    ,
	.dma_tx_channel = DMA_TX_CHx           ,
	.dma_tx_stream  = DMA_TX_STRMx         ,
	.dma_rx_channel = DMA_RX_CHx           ,
	.dma_rx_stream  = DMA_RX_STRMx         ,
	.dma_tx_irq     = DMA_TX_IRQn          ,
	.dma_rx_irq     = DMA_RX_IRQn          ,
	.uart_irq       = USART_IRQn           ,
	.dma_priority   = DMAxPRIORITY         ,
	.uart_priority  = IRQnPRIORITY         ,
	.dma_rx_enable  = DMARxEnable          ,
	.dma_tx_enable  = DMATxEnable          ,
};

/* 
  * @brief 生成一个对应的 ttySx , 并初始化其常量
*/
struct serial TTYSx = {
	.hal    = &TTYSxHAL       ,
	.rxbuf  = TTYSxRXBUF      ,
	.txbuf  = TTYSxTXBUF      ,
	.txmax  = TX_BUF_SIZE     ,
	.rxmax  = RX_BUF_SIZE / 2 ,
};


#if (DMATxEnable) // 如果启用 dma 发送，则开启 dma 发送中断
/**
  * @brief    DMA 发送一包数据完成中断。
  *           此时 DMA 把所有的数都传送到 USART->DR 寄存器，
  *           但还需要等待最后一个字节传输结束。
*/
void DMA_TX_IRQ_HANDLER(void)
{
	dma_tx_irq(&TTYSx);
}
#endif



#if (DMARxEnable) // 如果启用 dma 接收，则开启 dma 接收中断
/**
	* @brief    串口 DMA 接收满中断
*/
void DMA_RX_IRQ_HANDLER(void)
{
	dma_rx_irq(&TTYSx);
}
#endif


/**
	* @brief    串口中断函数
*/
void USART_IRQ_HANDLER(void)
{
	usart_irq(&TTYSx);
}



#undef USARTn 
#undef DMAn 
#undef DMATxCHn 
#undef DMARxCHn
#undef DMATxSTRMn
#undef DMARxSTRMn
#undef DMA_CLOCK
#undef IRQnPRIORITY
#undef DMAxPRIORITY
#undef DMATxEnable
#undef DMARxEnable
#undef DMA_CLOCK
