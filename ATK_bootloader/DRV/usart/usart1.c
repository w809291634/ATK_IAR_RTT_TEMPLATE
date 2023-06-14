#include "sys.h"
#include "usart1.h"	

/**************************user config*************************/
// 定义控制台调试串口的硬件
// 串口1 引脚配置
#define USARTx              USART1
#define USARTx_IRQn         USART1_IRQn
#define USARTx_GPIO_RCC     RCC_AHB1Periph_GPIOA
#define USARTx_RCC          RCC_APB2Periph_USART1
#define USARTx_TX_GPIO      GPIOA
#define USARTx_TX_PIN_NUM   GPIO_PinSource9
#define USARTx_TX_PIN       GPIO_Pin_9

#define USARTx_RX_GPIO      GPIOA
#define USARTx_RX_PIN_NUM   GPIO_PinSource10
#define USARTx_RX_PIN       GPIO_Pin_10

// 串口1DMA_RX配置
#define USARTx_RX_DMA_STREAM     						DMA2_Stream5
#define USARTx_RX_DMA_CHANNEL    						DMA_Channel_4
#define USARTx_RINGBUF_SIZE                 256

/**************************user config*************************/

// shell 终端使用变量
shellinput_t shell_1;
static char shell_ringbuf[USARTx_RINGBUF_SIZE]={0};
static unsigned short Read_Index;
static unsigned short Write_Index;
static volatile char Receive_flag;      // 串口空闲中断接收标志，shell开始进行数据处理

//初始化IO 串口1 
//bound:波特率
static void UART_Init(u32 bound)
{
  //GPIO端口设置
  GPIO_InitTypeDef GPIO_InitStructure={0};
  USART_InitTypeDef USART_InitStructure={0};

  RCC_AHB1PeriphClockCmd(USARTx_GPIO_RCC,ENABLE);    //使能GPIOA时钟
  RCC_APB2PeriphClockCmd(USARTx_RCC,ENABLE);   //使能USART时钟

  //串口1对应引脚复用映射
  GPIO_PinAFConfig(USARTx_TX_GPIO,USARTx_TX_PIN_NUM,GPIO_AF_USART1); //GPIOA9复用为USART
  GPIO_PinAFConfig(USARTx_RX_GPIO,USARTx_RX_PIN_NUM,GPIO_AF_USART1); //GPIOA10复用为USART

  //USART 端口配置
  GPIO_InitStructure.GPIO_Pin = USARTx_TX_PIN | USARTx_RX_PIN; //GPIOA9与GPIOA10
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//速度50MHz
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //推挽复用输出
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //上拉
  GPIO_Init(USARTx_TX_GPIO,&GPIO_InitStructure); //初始化PA9，PA10

  //USART 初始化设置
  USART_InitStructure.USART_BaudRate = bound;//波特率设置
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
  USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
  USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
  USART_Init(USARTx, &USART_InitStructure); //初始化串口

  USART_Cmd(USARTx, ENABLE);  //使能串口1 
}

// 串口 DMA Rx 配置
static void UART_DMA_RxConfig(DMA_Stream_TypeDef *DMA_Streamx,u32 channel,u32 addr_Peripherals,u32 addr_Mem,u16 Length)                                       //DMA的初始化
{
  //	NVIC_InitTypeDef NVIC_InitStructure={0};
  DMA_InitTypeDef UARTDMA_InitStructure={0};

  if((u32)DMA_Streamx>(u32)DMA2)//DMA1?DMA2
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2,ENABLE);//DMA2 	
  else 
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1,ENABLE);//DMA1

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
  UARTDMA_InitStructure.DMA_Mode = DMA_Mode_Circular   ;											  //使用循环模式
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

  DMA_Cmd(DMA_Streamx, ENABLE);     																			//DMA开启  
}

// 串口1 DMA 初始化
static void UART_DMA_Init()
{
  NVIC_InitTypeDef NVIC_InitStructure={0};
  // DMA接收配置
  UART_DMA_RxConfig(USARTx_RX_DMA_STREAM,						    // DMA串口接收流号码
                    USARTx_RX_DMA_CHANNEL,					    // DMA流对应的通道
                    (u32)&USARTx->DR,										// 寄存器
                    (u32)shell_ringbuf,								  // DMA接收区域
                    USARTx_RINGBUF_SIZE);
  USART_DMACmd(USARTx, USART_DMAReq_Rx , ENABLE);      	// 使能串口1的DMA接收
  // 配置中断
  USART_ITConfig(USARTx, USART_IT_IDLE, ENABLE);				// 开启空闲中断
  
  NVIC_InitStructure.NVIC_IRQChannel = USARTx_IRQn;			  // 串口1中断通道
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=1; // 抢占优先级
  NVIC_InitStructure.NVIC_IRQChannelSubPriority =0;		    // 子优先级
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			    // IRQ通道使能
  NVIC_Init(&NVIC_InitStructure);		
}

/* 获取缓冲区中剩余未读取数据长度 */
static uint16_t UART_GetRemain(void) {
  uint16_t remain_length;             // 剩余数据长度
  uint16_t write_index=Write_Index;   // 拷贝当前写索引位置
  //获取剩余数据长度
  if(write_index >= Read_Index) { 
    remain_length = write_index - Read_Index;
  } else {
    remain_length = USARTx_RINGBUF_SIZE - Read_Index + write_index;   // 此时说明串口缓存区数据从头开始缓存
  }
  return remain_length;
}

// shell 硬件初始化
void shell_hw_init(u32 bound)
{
  UART_Init(bound);
  UART_DMA_Init();
}

// 串口1的发送函数
// 发送字符串
void usart_puts(const char * strbuf, unsigned short len)
{
  while(len--)
  {
    USART_ClearFlag(USARTx, USART_FLAG_TC);
    USARTx->DR = *strbuf;
    while (USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET);
    strbuf++ ;
  }
}

// shell控制台获取输入数据
void shell_hw_input()
{
  if(Receive_flag){
    unsigned short data_len= UART_GetRemain();                        // 获取当前数据长度
    shell_input(&shell_1, shell_ringbuf + Read_Index, data_len);
    Read_Index = (Read_Index+data_len)% USARTx_RINGBUF_SIZE;          // 下次读取数据的起始位置，防止超出缓存区最大索引
    
    USARTx->CR1 &= ~USART_CR1_IDLEIE;             // 临界段保护
    Receive_flag=0;
    USARTx->CR1 |= USART_CR1_IDLEIE;
  }
}

//串口1中断服务程序
void USART1_IRQHandler(void)
{
  if(USART_GetITStatus( USARTx, USART_IT_IDLE ) == SET )
  {	
    USART_ReceiveData(USARTx);//清除IDLE中断标志位 
    // 更新当前串口接收的缓存末端
    Write_Index = (USARTx_RINGBUF_SIZE-DMA_GetCurrDataCounter(USARTx_RX_DMA_STREAM)) % USARTx_RINGBUF_SIZE;
    Receive_flag=1; 
  }	
} 
