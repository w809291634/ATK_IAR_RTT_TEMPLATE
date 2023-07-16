#include "sys.h"
#include "usart3.h"	
#include "esp32_at.h"
#include "ota.h"

/**************************user config*************************/
// 定义控制台调试串口的硬件
// 串口3 引脚配置
#define USARTx              USART3
#define USARTx_IRQn         USART3_IRQn
#define USARTx_GPIO_RCC     RCC_AHB1Periph_GPIOB
#define USARTx_RCC_CMD_FUN  RCC_APB1PeriphClockCmd
#define USARTx_RCC          RCC_APB1Periph_USART3
#define USARTx_AF           GPIO_AF_USART3

#define USARTx_TX_GPIO      GPIOB
#define USARTx_TX_PIN_NUM   GPIO_PinSource10
#define USARTx_TX_PIN       GPIO_Pin_10

#define USARTx_RX_GPIO      GPIOB
#define USARTx_RX_PIN_NUM   GPIO_PinSource11
#define USARTx_RX_PIN       GPIO_Pin_11

// 串口3 DMA_RX配置
#define USARTx_RINGBUF_SIZE                 256

/**************************user config*************************/
static char esp32_at_ringbuf[USARTx_RINGBUF_SIZE]={0};
static volatile unsigned short Read_Index,Write_Index;
extern uint16_t UART_GetRemainDate(char* data, char* ringbuf, uint16_t size, uint16_t w_index, uint16_t r_index) ;

//初始化IO 串口 
//bound:波特率
void esp32_at_hw_init(u32 bound)
{
  //GPIO端口设置
  GPIO_InitTypeDef GPIO_InitStructure={0};
  USART_InitTypeDef USART_InitStructure={0};
  NVIC_InitTypeDef NVIC_InitStructure={0};

  RCC_AHB1PeriphClockCmd(USARTx_GPIO_RCC,ENABLE);     //使能GPIO时钟
  USARTx_RCC_CMD_FUN(USARTx_RCC,ENABLE);              //使能USART时钟

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

  USART_ITConfig(USARTx, USART_IT_RXNE, ENABLE);//开启相关中断
  
	// NVIC 配置
  NVIC_InitStructure.NVIC_IRQChannel = USARTx_IRQn;			  // 串口中断通道
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0; // 抢占优先级
  NVIC_InitStructure.NVIC_IRQChannelSubPriority =0;		    // 子优先级
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			    // IRQ通道使能
  NVIC_Init(&NVIC_InitStructure);	
  
  USART_Cmd(USARTx, ENABLE);  //使能串口 
}

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
    uint16_t data_len=UART_GetRemainDate(temp,esp32_at_ringbuf,USARTx_RINGBUF_SIZE,Write_Index,Read_Index);
    
    /* 接收并处理此处数据 */
    if(esp32_link)http_data_handle(temp,data_len);
    else{
      int ret=esp32_command_handle(temp,data_len);        // 回调应用层的数据处理函数
      if(ret==-1)
        debug_err(ERR"Read_Index:%d usart3_data:%s\r\n",Read_Index,temp);
    }
    
    /* 数据处理结束 */
    Read_Index = (Read_Index+data_len)% USARTx_RINGBUF_SIZE;          // 下次读取数据的起始位置，防止超出缓存区最大索引   
  }
}

//串口3中断服务程序
void USART3_IRQHandler(void)
{
  if(USART_GetITStatus( USARTx, USART_IT_RXNE ) == SET )
  {	
    esp32_at_ringbuf[Write_Index] = (char)USART_ReceiveData(USARTx);
    Write_Index++;
    Write_Index = Write_Index % USARTx_RINGBUF_SIZE;
  }	
} 
