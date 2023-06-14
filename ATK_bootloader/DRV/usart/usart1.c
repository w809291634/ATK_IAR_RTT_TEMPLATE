#include "sys.h"
#include "usart1.h"	

/**************************user config*************************/
// �������̨���Դ��ڵ�Ӳ��
// ����1 ��������
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

// ����1DMA_RX����
#define USARTx_RX_DMA_STREAM     						DMA2_Stream5
#define USARTx_RX_DMA_CHANNEL    						DMA_Channel_4
#define USARTx_RINGBUF_SIZE                 256

/**************************user config*************************/

// shell �ն�ʹ�ñ���
shellinput_t shell_1;
static char shell_ringbuf[USARTx_RINGBUF_SIZE]={0};
static unsigned short Read_Index;
static unsigned short Write_Index;
static volatile char Receive_flag;      // ���ڿ����жϽ��ձ�־��shell��ʼ�������ݴ���

//��ʼ��IO ����1 
//bound:������
static void UART_Init(u32 bound)
{
  //GPIO�˿�����
  GPIO_InitTypeDef GPIO_InitStructure={0};
  USART_InitTypeDef USART_InitStructure={0};

  RCC_AHB1PeriphClockCmd(USARTx_GPIO_RCC,ENABLE);    //ʹ��GPIOAʱ��
  RCC_APB2PeriphClockCmd(USARTx_RCC,ENABLE);   //ʹ��USARTʱ��

  //����1��Ӧ���Ÿ���ӳ��
  GPIO_PinAFConfig(USARTx_TX_GPIO,USARTx_TX_PIN_NUM,GPIO_AF_USART1); //GPIOA9����ΪUSART
  GPIO_PinAFConfig(USARTx_RX_GPIO,USARTx_RX_PIN_NUM,GPIO_AF_USART1); //GPIOA10����ΪUSART

  //USART �˿�����
  GPIO_InitStructure.GPIO_Pin = USARTx_TX_PIN | USARTx_RX_PIN; //GPIOA9��GPIOA10
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//���ù���
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//�ٶ�50MHz
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //���츴�����
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //����
  GPIO_Init(USARTx_TX_GPIO,&GPIO_InitStructure); //��ʼ��PA9��PA10

  //USART ��ʼ������
  USART_InitStructure.USART_BaudRate = bound;//����������
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;//�ֳ�Ϊ8λ���ݸ�ʽ
  USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
  USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ
  USART_Init(USARTx, &USART_InitStructure); //��ʼ������

  USART_Cmd(USARTx, ENABLE);  //ʹ�ܴ���1 
}

// ���� DMA Rx ����
static void UART_DMA_RxConfig(DMA_Stream_TypeDef *DMA_Streamx,u32 channel,u32 addr_Peripherals,u32 addr_Mem,u16 Length)                                       //DMA�ĳ�ʼ��
{
  //	NVIC_InitTypeDef NVIC_InitStructure={0};
  DMA_InitTypeDef UARTDMA_InitStructure={0};

  if((u32)DMA_Streamx>(u32)DMA2)//DMA1?DMA2
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2,ENABLE);//DMA2 	
  else 
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1,ENABLE);//DMA1

  DMA_DeInit(DMA_Streamx);
  while (DMA_GetCmdStatus(DMA_Streamx) != DISABLE){}//�ȴ�DMA������ 

  UARTDMA_InitStructure.DMA_Channel = channel;																	//ͨ��
  UARTDMA_InitStructure.DMA_PeripheralBaseAddr = ( uint32_t)(addr_Peripherals) ;//�����ַ
  UARTDMA_InitStructure.DMA_Memory0BaseAddr =   ( uint32_t)addr_Mem; 						//�����ַ
  UARTDMA_InitStructure.DMA_DIR =  DMA_DIR_PeripheralToMemory;									//����-���赽�ڴ�
  UARTDMA_InitStructure.DMA_BufferSize = Length; 																//���䳤��
  UARTDMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;					//���������ģʽ
  UARTDMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;										//�洢������ģʽ
  UARTDMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte ;	//�������ݳ���:8λ
  UARTDMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte ;					//�洢�����ݳ���:8λ
  UARTDMA_InitStructure.DMA_Mode = DMA_Mode_Circular   ;											  //ʹ��ѭ��ģʽ
  UARTDMA_InitStructure.DMA_Priority = DMA_Priority_Medium;											//�е����ȼ�
  UARTDMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
  UARTDMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
  UARTDMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single ;							//�洢��ͻ�����δ���
  UARTDMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;				//����ͻ�����δ���
  DMA_Init(DMA_Streamx, &UARTDMA_InitStructure);																//��ʼ��DMA Stream

  // ����û��ʹ�õ�DMA�Ľ����ж�
  //  NVIC_InitStructure.NVIC_IRQChannel = DMA_USART_RX_IRQ ;                //����ʹ�ô��ڵĿ����жϣ����������û�п���
  //  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  //  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  //  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  //  NVIC_Init(&NVIC_InitStructure);
  //  DMA_ITConfig(DMA_Streamx, DMA_IT_TC, ENABLE);//������������жϴ���   //DMA�ж�ʹ��

  DMA_Cmd(DMA_Streamx, ENABLE);     																			//DMA����  
}

// ����1 DMA ��ʼ��
static void UART_DMA_Init()
{
  NVIC_InitTypeDef NVIC_InitStructure={0};
  // DMA��������
  UART_DMA_RxConfig(USARTx_RX_DMA_STREAM,						    // DMA���ڽ���������
                    USARTx_RX_DMA_CHANNEL,					    // DMA����Ӧ��ͨ��
                    (u32)&USARTx->DR,										// �Ĵ���
                    (u32)shell_ringbuf,								  // DMA��������
                    USARTx_RINGBUF_SIZE);
  USART_DMACmd(USARTx, USART_DMAReq_Rx , ENABLE);      	// ʹ�ܴ���1��DMA����
  // �����ж�
  USART_ITConfig(USARTx, USART_IT_IDLE, ENABLE);				// ���������ж�
  
  NVIC_InitStructure.NVIC_IRQChannel = USARTx_IRQn;			  // ����1�ж�ͨ��
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=1; // ��ռ���ȼ�
  NVIC_InitStructure.NVIC_IRQChannelSubPriority =0;		    // �����ȼ�
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			    // IRQͨ��ʹ��
  NVIC_Init(&NVIC_InitStructure);		
}

/* ��ȡ��������ʣ��δ��ȡ���ݳ��� */
static uint16_t UART_GetRemain(void) {
  uint16_t remain_length;             // ʣ�����ݳ���
  uint16_t write_index=Write_Index;   // ������ǰд����λ��
  //��ȡʣ�����ݳ���
  if(write_index >= Read_Index) { 
    remain_length = write_index - Read_Index;
  } else {
    remain_length = USARTx_RINGBUF_SIZE - Read_Index + write_index;   // ��ʱ˵�����ڻ��������ݴ�ͷ��ʼ����
  }
  return remain_length;
}

// shell Ӳ����ʼ��
void shell_hw_init(u32 bound)
{
  UART_Init(bound);
  UART_DMA_Init();
}

// ����1�ķ��ͺ���
// �����ַ���
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

// shell����̨��ȡ��������
void shell_hw_input()
{
  if(Receive_flag){
    unsigned short data_len= UART_GetRemain();                        // ��ȡ��ǰ���ݳ���
    shell_input(&shell_1, shell_ringbuf + Read_Index, data_len);
    Read_Index = (Read_Index+data_len)% USARTx_RINGBUF_SIZE;          // �´ζ�ȡ���ݵ���ʼλ�ã���ֹ�����������������
    
    USARTx->CR1 &= ~USART_CR1_IDLEIE;             // �ٽ�α���
    Receive_flag=0;
    USARTx->CR1 |= USART_CR1_IDLEIE;
  }
}

//����1�жϷ������
void USART1_IRQHandler(void)
{
  if(USART_GetITStatus( USARTx, USART_IT_IDLE ) == SET )
  {	
    USART_ReceiveData(USARTx);//���IDLE�жϱ�־λ 
    // ���µ�ǰ���ڽ��յĻ���ĩ��
    Write_Index = (USARTx_RINGBUF_SIZE-DMA_GetCurrDataCounter(USARTx_RX_DMA_STREAM)) % USARTx_RINGBUF_SIZE;
    Receive_flag=1; 
  }	
} 
