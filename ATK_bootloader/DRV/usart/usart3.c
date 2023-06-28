#include "sys.h"
#include "usart3.h"	
#include "esp32_at.h"

/**************************user config*************************/
// 定义控制台调试串口的硬件
// 串口3 引脚配置
#define USARTx              USART3
#define USARTx_IRQn         USART3_IRQn
#define USARTx_GPIO_RCC     RCC_AHB1Periph_GPIOB
#define USARTx_RCC          RCC_APB1Periph_USART3
#define USARTx_AF           GPIO_AF_USART3

#define USARTx_TX_GPIO      GPIOB
#define USARTx_TX_PIN_NUM   GPIO_PinSource10
#define USARTx_TX_PIN       GPIO_Pin_10

#define USARTx_RX_GPIO      GPIOB
#define USARTx_RX_PIN_NUM   GPIO_PinSource11
#define USARTx_RX_PIN       GPIO_Pin_11

// 串口3 DMA_RX配置
#define USARTx_RX_DMA_STREAM     						DMA1_Stream1
#define USARTx_RX_DMA_CHANNEL    						DMA_Channel_4
#define USARTx_RINGBUF_SIZE                 256

/**************************user config*************************/
static char esp32_at_ringbuf[USARTx_RINGBUF_SIZE]={0};
static unsigned short Read_Index;
static unsigned short Write_Index;

//初始化IO 串口1 
//bound:波特率
static void UART_Init(u32 bound)
{
  //GPIO端口设置
  GPIO_InitTypeDef GPIO_InitStructure={0};
  USART_InitTypeDef USART_InitStructure={0};

  RCC_AHB1PeriphClockCmd(USARTx_GPIO_RCC,ENABLE);     //使能GPIO时钟
  RCC_APB1PeriphClockCmd(USARTx_RCC,ENABLE);          //使能USART时钟
//  RCC_APB2PeriphClockCmd(USARTx_RCC,ENABLE);          //使能USART时钟

  //串口1对应引脚复用映射
  GPIO_PinAFConfig(USARTx_TX_GPIO,USARTx_TX_PIN_NUM,USARTx_AF); //复用为USART
  GPIO_PinAFConfig(USARTx_RX_GPIO,USARTx_RX_PIN_NUM,USARTx_AF); //复用为USART

  //USART 端口配置
  GPIO_InitStructure.GPIO_Pin = USARTx_TX_PIN | USARTx_RX_PIN; 
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//速度50MHz
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //推挽复用输出
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //上拉
  GPIO_Init(USARTx_TX_GPIO,&GPIO_InitStructure); //初始化

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
                    (u32)esp32_at_ringbuf,								  // DMA接收区域
                    USARTx_RINGBUF_SIZE);
  USART_DMACmd(USARTx, USART_DMAReq_Rx , ENABLE);      	// 使能串口1的DMA接收
  // 配置中断
  USART_ITConfig(USARTx, USART_IT_IDLE, ENABLE);				// 开启空闲中断
  
  NVIC_InitStructure.NVIC_IRQChannel = USARTx_IRQn;			  // 串口中断通道
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

// ESP32/8266 AT组件 硬件初始化
void esp32_at_hw_init(u32 bound)
{
  UART_Init(bound);
  UART_DMA_Init();
}

// 串口1的发送函数
// 发送字符串
void usart3_puts(const char * strbuf, unsigned short len)
{
  while(len--)
  {
    USART_ClearFlag(USARTx, USART_FLAG_TC);
    USARTx->DR = *strbuf;
    while (USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET);
    strbuf++ ;
  }
}

// esp32 应用程序获取输入数据
void esp32_usart_data_handle()
{
  if(Write_Index!=Read_Index){
    /* 取环形缓存区剩余数据 */
    char temp[USARTx_RINGBUF_SIZE]={0};
    unsigned short data_len= UART_GetRemain();          // 获取当前数据长度
    if(Read_Index+data_len>USARTx_RINGBUF_SIZE){
      // 索引处读取长度超出缓存
      int len1=USARTx_RINGBUF_SIZE-Read_Index;          // 环形末尾读长度
      int len2=data_len-len1;
      memcpy(temp,esp32_at_ringbuf + Read_Index,len1);
      memcpy(temp+len1,esp32_at_ringbuf,len2);
    }else{
      memcpy(temp,esp32_at_ringbuf + Read_Index,data_len);
    }
      
    /* 接收并处理此处数据 */
    int ret=esp32_command_handle(temp,data_len);        // 回调应用层的数据处理函数
    if(ret==-1){
      debug_err(ERR"Read_Index:%d usart3_data:%s\r\n",Read_Index,temp);
    }
    
    /* 数据处理结束 */
    Read_Index = (Read_Index+data_len)% USARTx_RINGBUF_SIZE;          // 下次读取数据的起始位置，防止超出缓存区最大索引
  }
}

//串口3中断服务程序
void USART3_IRQHandler(void)
{
  if(USART_GetITStatus( USARTx, USART_IT_IDLE ) == SET )
  {	
    USART_ReceiveData(USARTx);//清除IDLE中断标志位 
    // 更新当前串口接收的缓存末端
    Write_Index = (USARTx_RINGBUF_SIZE-DMA_GetCurrDataCounter(USARTx_RX_DMA_STREAM)) % USARTx_RINGBUF_SIZE;
  }	
} 
