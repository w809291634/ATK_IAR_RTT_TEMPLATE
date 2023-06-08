/**
  ******************************************************************************
  * @file    IAP/src/wifi_download.c 
  * @author  MCD Application Team
  * @version V3.3.0
  * @date    10/15/2010
  * @brief   Main program body
  ******************************************************************************
  * @copy
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2010 STMicroelectronics</center></h2>
  */ 
#include "stm32f10x.h"
	
static void delay_ms(unsigned ms){

	unsigned i,j;
	for(i=0;i<ms;i++)
		for(j=0;j<72000;j++);
}

static void wifi_hw_init(void){
		// GPIO2 -- PA11  GPIO16(RESET) -- PA13 GPIO0(MODE F=work/L=Download) -- PA8  CH_PD -- PA12  
		GPIO_InitTypeDef GPIO_InitStructure;
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
		GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable,ENABLE);
		/* Configure Button pin as input */
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_8;
		GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	//GPIOA初始化，USART1 Tx (PA9) 复用开漏输出
	GPIO_InitStructure.GPIO_Pin 	= GPIO_Pin_9; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
	GPIO_InitStructure.GPIO_Mode 	= GPIO_Mode_IPU; 
	GPIO_Init(GPIOA, &GPIO_InitStructure);		  
	//Configure USART1 Rx (PA10) as input floating 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
  USART_DeInit(USART1);
  GPIO_ResetBits(GPIOA,GPIO_Pin_13);
  delay_ms(50);
  GPIO_ResetBits(GPIOA,GPIO_Pin_12);
  delay_ms(50);
  GPIO_ResetBits(GPIOA,GPIO_Pin_8); 
  delay_ms(50);
  GPIO_SetBits(GPIOA,GPIO_Pin_12);
  delay_ms(50);
  GPIO_SetBits(GPIOA,GPIO_Pin_13);
}

static void wifi_nvic_init(void){
	
		NVIC_InitTypeDef NVIC_InitStructure;

		/* Configure the NVIC Preemption Priority Bits */  
		NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
  
		/* Enable the USARTy Interrupt */
		NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);

		/* Enable the USARTz Interrupt */
		NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);
}

static void wifi_uart_init(void){
	USART_InitTypeDef USART_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA |RCC_APB2Periph_AFIO,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);
	
	//GPIOA初始化，USART1 Tx (PA9) 复用开漏输出
	GPIO_InitStructure.GPIO_Pin 	= GPIO_Pin_9; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
	GPIO_InitStructure.GPIO_Mode 	= GPIO_Mode_AF_PP; 
	GPIO_Init(GPIOA, &GPIO_InitStructure);	
	  
	//Configure USART1 Rx (PA10) as input floating 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

  USART_DeInit(USART1);

  USART_InitStructure.USART_BaudRate = 115200;//460800;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx; 
  USART_Init(USART1, &USART_InitStructure);
	
	USART_Cmd(USART1, ENABLE);		 //使能UART
	USART_ITConfig(USART1,USART_IT_RXNE,ENABLE);
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB |RCC_APB2Periph_AFIO,ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3,ENABLE);
	
	//GPIOB初始化，USART3 Tx (PB10) 复用开漏输出
	GPIO_InitStructure.GPIO_Pin 	= GPIO_Pin_10; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
	GPIO_InitStructure.GPIO_Mode 	= GPIO_Mode_AF_PP; 
	GPIO_Init(GPIOB, &GPIO_InitStructure);	
	  
	//Configure USART3 Rx (PB11) as input floating 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	USART_DeInit(USART3);
	
	USART_InitStructure.USART_BaudRate = 115200;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx; 
  USART_Init(USART3, &USART_InitStructure);
  USART_Cmd(USART3, ENABLE);		 //使能UART
	USART_ITConfig(USART3,USART_IT_RXNE,ENABLE);
	
}

void wifi_download_Init(void){
	wifi_hw_init();
//	wifi_uart_init();
//	wifi_nvic_init();
	while(1);
}
