#ifndef _MAIN_H_
#define _MAIN_H_

#include "stm32f10x.h"

/*红灯：PB13，蓝灯：PB14*/
#define LED_RUN_CLK   RCC_APB2Periph_GPIOB
#define LED_RUN_PORT  GPIOB
#define LED_RUN_PIN   GPIO_Pin_13
#define LED_ON()      GPIO_ResetBits(LED_RUN_PORT, LED_RUN_PIN)
#define LED_OFF()     GPIO_SetBits(LED_RUN_PORT, LED_RUN_PIN)

/*串口2通过17/18脚（RX-PA3/TX-PA2）连接RJ45口，串口3通过7/8脚（RX-PB11/TX-PB10）连接USB口*/
/*需要使用透传节点的设备一般只连接了17/18脚*/
#define TERM_UART_BAUD        38400
#define USB_TEST  0
#if USB_TEST==1
  #define TERM_UART           USART3
  #define TERM_UART_CLK_INIT  RCC_APB1PeriphClockCmd
  #define TERM_UART_CLK       RCC_APB1Periph_USART3
  #define TERM_UART_IRQn      USART3_IRQn
  #define TERM_UART_ISR       USART3_IRQHandler

  #define TERM_GPIO_CLK       RCC_APB2Periph_GPIOB
  #define TERM_GPIO_PORT      GPIOB
  #define TERM_GPIO_TX_PIN    GPIO_Pin_10
  #define TERM_GPIO_RX_PIN    GPIO_Pin_11
#else
  #define TERM_UART           USART2
  #define TERM_UART_CLK_INIT  RCC_APB1PeriphClockCmd
  #define TERM_UART_CLK       RCC_APB1Periph_USART2
  #define TERM_UART_IRQn      USART2_IRQn
  #define TERM_UART_ISR       USART2_IRQHandler

  #define TERM_GPIO_CLK       RCC_APB2Periph_GPIOA
  #define TERM_GPIO_PORT      GPIOA
  #define TERM_GPIO_TX_PIN    GPIO_Pin_2
  #define TERM_GPIO_RX_PIN    GPIO_Pin_3
#endif

#endif //_MAIN_H_
