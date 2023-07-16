#include "sys.h"
#include "usart3.h"	
#include "esp32_at.h"
#include "ota.h"

/**************************user config*************************/
// �������̨���Դ��ڵ�Ӳ��
// ����3 ��������
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

// ����3 DMA_RX����
#define USARTx_RINGBUF_SIZE                 256

/**************************user config*************************/
static char esp32_at_ringbuf[USARTx_RINGBUF_SIZE]={0};
static volatile unsigned short Read_Index,Write_Index;
extern uint16_t UART_GetRemainDate(char* data, char* ringbuf, uint16_t size, uint16_t w_index, uint16_t r_index) ;

//��ʼ��IO ���� 
//bound:������
void esp32_at_hw_init(u32 bound)
{
  //GPIO�˿�����
  GPIO_InitTypeDef GPIO_InitStructure={0};
  USART_InitTypeDef USART_InitStructure={0};
  NVIC_InitTypeDef NVIC_InitStructure={0};

  RCC_AHB1PeriphClockCmd(USARTx_GPIO_RCC,ENABLE);     //ʹ��GPIOʱ��
  USARTx_RCC_CMD_FUN(USARTx_RCC,ENABLE);              //ʹ��USARTʱ��

  //����1��Ӧ���Ÿ���ӳ��
  GPIO_PinAFConfig(USARTx_TX_GPIO,USARTx_TX_PIN_NUM,USARTx_AF); //����ΪUSART
  GPIO_PinAFConfig(USARTx_RX_GPIO,USARTx_RX_PIN_NUM,USARTx_AF); //����ΪUSART

  //USART �˿�����
  GPIO_InitStructure.GPIO_Pin = USARTx_TX_PIN | USARTx_RX_PIN; 
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//���ù���
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//�ٶ�50MHz
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //���츴�����
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //����
  GPIO_Init(USARTx_TX_GPIO,&GPIO_InitStructure); //��ʼ��

  //USART ��ʼ������
  USART_InitStructure.USART_BaudRate = bound;//����������
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;//�ֳ�Ϊ8λ���ݸ�ʽ
  USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
  USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ
  USART_Init(USARTx, &USART_InitStructure); //��ʼ������

  USART_ITConfig(USARTx, USART_IT_RXNE, ENABLE);//��������ж�
  
	// NVIC ����
  NVIC_InitStructure.NVIC_IRQChannel = USARTx_IRQn;			  // �����ж�ͨ��
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0; // ��ռ���ȼ�
  NVIC_InitStructure.NVIC_IRQChannelSubPriority =0;		    // �����ȼ�
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			    // IRQͨ��ʹ��
  NVIC_Init(&NVIC_InitStructure);	
  
  USART_Cmd(USARTx, ENABLE);  //ʹ�ܴ��� 
}

// �����ַ���
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

// esp32 Ӧ�ó����ȡ��������
void esp32_usart_data_handle()
{
  if(Write_Index!=Read_Index){
    /* ȡ���λ�����ʣ������ */
    char temp[USARTx_RINGBUF_SIZE]={0};
    uint16_t data_len=UART_GetRemainDate(temp,esp32_at_ringbuf,USARTx_RINGBUF_SIZE,Write_Index,Read_Index);
    
    /* ���ղ�����˴����� */
    if(esp32_link)http_data_handle(temp,data_len);
    else{
      int ret=esp32_command_handle(temp,data_len);        // �ص�Ӧ�ò�����ݴ�����
      if(ret==-1)
        debug_err(ERR"Read_Index:%d usart3_data:%s\r\n",Read_Index,temp);
    }
    
    /* ���ݴ������ */
    Read_Index = (Read_Index+data_len)% USARTx_RINGBUF_SIZE;          // �´ζ�ȡ���ݵ���ʼλ�ã���ֹ�����������������   
  }
}

//����3�жϷ������
void USART3_IRQHandler(void)
{
  if(USART_GetITStatus( USARTx, USART_IT_RXNE ) == SET )
  {	
    esp32_at_ringbuf[Write_Index] = (char)USART_ReceiveData(USARTx);
    Write_Index++;
    Write_Index = Write_Index % USARTx_RINGBUF_SIZE;
  }	
} 
