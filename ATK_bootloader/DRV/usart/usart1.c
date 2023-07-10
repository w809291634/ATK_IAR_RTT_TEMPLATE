#include "sys.h"
#include "usart1.h"	

/**************************user config*************************/
// 定义控制台调试串口的硬件
// 串口1 引脚配置
#define USARTx              USART1
#define USARTx_IRQn         USART1_IRQn
#define USARTx_GPIO_RCC     RCC_AHB1Periph_GPIOA
#define USARTx_RCC_CMD_FUN  RCC_APB2PeriphClockCmd
#define USARTx_RCC          RCC_APB2Periph_USART1
#define USARTx_TX_GPIO      GPIOA
#define USARTx_TX_PIN_NUM   GPIO_PinSource9
#define USARTx_TX_PIN       GPIO_Pin_9

#define USARTx_RX_GPIO      GPIOA
#define USARTx_RX_PIN_NUM   GPIO_PinSource10
#define USARTx_RX_PIN       GPIO_Pin_10

#define USARTx_RINGBUF_SIZE                 (256)     

/**************************user config*************************/

// shell 终端使用变量
shellinput_t shell_1;
static char shell_ringbuf[USARTx_RINGBUF_SIZE]={0};
static unsigned short Read_Index;
static unsigned short Write_Index;
static volatile unsigned char Receive_flag;      // 串口空闲中断接收标志，shell开始进行数据处理

char usart1_mode=0;             // 串口的数据流 工作指向  0：数据到达shell控制台  1：数据到达IAP更新

//初始化IO 串口1 
//bound:波特率
void shell_hw_init(u32 bound)
{
  //GPIO端口设置
  GPIO_InitTypeDef GPIO_InitStructure={0};
  USART_InitTypeDef USART_InitStructure={0};
  NVIC_InitTypeDef NVIC_InitStructure={0};

  RCC_AHB1PeriphClockCmd(USARTx_GPIO_RCC,ENABLE);    //使能GPIOA时钟
  USARTx_RCC_CMD_FUN(USARTx_RCC,ENABLE);              //使能USART时钟

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

	USART_ITConfig(USARTx, USART_IT_RXNE, ENABLE);//开启相关中断

	//Usart1 NVIC 配置
  NVIC_InitStructure.NVIC_IRQChannel = USARTx_IRQn;			  // 串口1中断通道
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=1; // 抢占优先级
  NVIC_InitStructure.NVIC_IRQChannelSubPriority =0;		    // 子优先级
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			    // IRQ通道使能
  NVIC_Init(&NVIC_InitStructure);	
  
  USART_Cmd(USARTx, ENABLE);  //使能串口1 
}

// 清空数据
void usart1_flush(void)
{
  Read_Index=Write_Index;
}

// 串口1的发送函数
// 发送字符
void usart1_put(const char data)
{
  USART_ClearFlag(USARTx, USART_FLAG_TC);
  USARTx->DR = data;
  while (USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET);
}

// 串口1的发送函数
// 发送字符串
void usart1_puts(const char * strbuf, unsigned short len)
{
  while(len--)
  {
    USART_ClearFlag(USARTx, USART_FLAG_TC);
    USARTx->DR = *strbuf;
    while (USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET);
    strbuf++ ;
  }
}

/* 获取缓冲区中剩余未读取数据 */
// data 读取剩余数据
// ringbuf 当前环形缓存区 size 缓存区大小 
// w_index r_index 当前缓存区的读写索引
// 返回 读取长度
uint16_t UART_GetRemainDate(char* data, char* ringbuf, uint16_t size, uint16_t w_index, uint16_t r_index) 
{
  //获取剩余数据
  if(w_index >= r_index) { 
    int len1 = w_index - r_index;
    memcpy(data,ringbuf + r_index,len1);
    return len1;
  } else {
    int len1=size-r_index;          // 环形末尾读长度
    int len2=w_index;
    memcpy(data,ringbuf + r_index,len1);
    memcpy(data+len1,ringbuf,len2);
    return len1+len2;
  }
}

// 从缓存区读取一个字节
// 注意：数据处理结果和shell_app_cycle只能选择其一
int usart1_getchar(uint8_t* data)
{
  if(Write_Index!=Read_Index){
    *data=shell_ringbuf[Read_Index];
    Read_Index = (Read_Index+1)% USARTx_RINGBUF_SIZE;   // 读取索引加1
    return 1;
  }else return 0;
}

// shell控制台获取输入数据
void shell_app_cycle()
{
#define SHELL_DATA_PROCESS          {shell_input(&shell_1, temp, data_len);\
                                      Read_Index = (Read_Index+data_len)% USARTx_RINGBUF_SIZE;}
  if(Write_Index!=Read_Index){
    /* 取环形缓存区剩余数据 */
    char temp[USARTx_RINGBUF_SIZE]={0};
    char flag=0x1B;             // 方向键的特殊字符
    uint16_t data_len=UART_GetRemainDate(temp,shell_ringbuf,USARTx_RINGBUF_SIZE,Write_Index,Read_Index);

    /* 数据处理 */
    if(strchr(temp,flag)){
      if(strlen(temp)>2)SHELL_DATA_PROCESS;
    }
    else SHELL_DATA_PROCESS
  }
}

//串口1中断服务程序
void USART1_IRQHandler(void)
{
  if(USART_GetITStatus( USARTx, USART_IT_RXNE ) == SET )
  {	
    shell_ringbuf[Write_Index] = (char)USART_ReceiveData(USARTx);
    Write_Index++;
    Write_Index = Write_Index % USARTx_RINGBUF_SIZE;
  }	
} 
