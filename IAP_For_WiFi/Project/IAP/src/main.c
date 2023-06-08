/**
  ******************************************************************************
  * @file    IAP/src/main.c 
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

/** @addtogroup IAP
  * @{
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "common.h"
#include "flash_if.h"
#include "command.h"
#include "flash.h"
#include "term.h"
//#include "command.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
#define USER_APP_BEGIN  (unsigned)(0x08000000)
/* Private variables ---------------------------------------------------------*/
//tUSER UserConfig;
extern pFunction Jump_To_Application;
extern uint32_t JumpAddress;

#ifdef __GNUC__
  /* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */
	

/* Private function prototypes -----------------------------------------------*/
static void Terminal_Init (void) ;

/* Private functions ---------------------------------------------------------*/
/*******************************************************************************
* Function Name  : static void LED_Init(void)
* Description    : LED 初始化
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
static void LED_Init(void){
  GPIO_InitTypeDef GPIO_InitStructure;

  RCC_APB2PeriphClockCmd(LED_RUN_CLK, ENABLE);
  /* Configure Button pin as input */
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Pin = LED_RUN_PIN;
  GPIO_Init(LED_RUN_PORT, &GPIO_InitStructure);
  LED_ON();
}

/*******************************************************************************
* Function Name  : void LED_Ctl(uint8_t en)
* Description    : LED 控制
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void LED_Ctl(uint8_t en){
	if(en) {
		LED_ON();
	} else {
		LED_OFF();
  }
}
/*******************************************************************************
* Function Name  : void LED_Toogle(void)
* Description    : LED 控制
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void LED_Toogle(void){
	static uint8_t state = 0;
	if(state) {
		LED_ON(); 
  } else {
		LED_OFF();
  }
	state = (state+1)&0x01;
}

uint8_t watch_dog_flag = 0;
int main(void)
{
    extern unsigned char userdata[];
    user_data_t *p = (user_data_t*)userdata;
    
    NVIC_SetVectorTable(NVIC_VectTab_FLASH,(USER_APP_BEGIN - NVIC_VectTab_FLASH));//重映射中断向量表
    if(IWDG->RLR != 0x0FFF) {
      watch_dog_flag = 1;
      IWDG_ReloadCounter();
    }
    
    LED_Init();
    flash_read(PARAMETER_ADDRESS, (unsigned char *)(userdata), sizeof(user_data_t));//读取配置信息
	  if(p->boot_mode == BOOT_APP && p->app_en == APP_OK) { //APP模式：跳转到APP运行
			RunUserProgram(APPLICATION_ADDRESS);
		}
    
    
    Terminal_Init();
    init_builtin_cmds();
    printk("\tEnter IAP mode, input \"IAP update\" to update firmware!\n");
    while (1)
    {
      serial_terminal();
      if(watch_dog_flag) IWDG_ReloadCounter();
    }
}
/**
  * @brief  Configures the nested vectored interrupt controller.
  * @param  None
  * @retval None
  */
static void NVIC_Configuration(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;

  /* Configure the NVIC Preemption Priority Bits */  
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
  
  /* Enable the USARTy Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TERM_UART_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}
/*******************************************************************************
* Function Name  : Terminal_Init
* Description    : Terminal 初始化
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
static void Terminal_Init (void)  
{
	extern unsigned char userdata[];
	GPIO_InitTypeDef  GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
  
	RCC_APB2PeriphClockCmd(TERM_GPIO_CLK,ENABLE);
	TERM_UART_CLK_INIT(TERM_UART_CLK, ENABLE);

  /* Configure USARTy Rx as input floating */
  GPIO_InitStructure.GPIO_Pin = TERM_GPIO_RX_PIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_Init(TERM_GPIO_PORT, &GPIO_InitStructure);
        
  /* Configure USARTx_Tx as alternate function push-pull */
  GPIO_InitStructure.GPIO_Pin = TERM_GPIO_TX_PIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(TERM_GPIO_PORT, &GPIO_InitStructure);

  USART_DeInit(TERM_UART);
	USART_InitStructure.USART_BaudRate            = TERM_UART_BAUD;
	USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits            = USART_StopBits_1;
	USART_InitStructure.USART_Parity              = USART_Parity_No ;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(TERM_UART, &USART_InitStructure);
	USART_Cmd(TERM_UART, ENABLE);
	NVIC_Configuration();
	USART_ITConfig(TERM_UART,USART_IT_RXNE,ENABLE);
}

#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

PUTCHAR_PROTOTYPE
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the USART */
  USART_SendData(TERM_UART, (uint8_t) ch);

  /* Loop until the end of transmission */
  while (USART_GetFlagStatus(TERM_UART, USART_FLAG_TC) == RESET)
  {}

  return ch;
}

/**
  * @}
  */

/******************* (C) COPYRIGHT 2010 STMicroelectronics *****END OF FILE****/
