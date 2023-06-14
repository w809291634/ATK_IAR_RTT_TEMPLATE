#include "stm32f4xx.h"
#include "systick.h"
#include "usart1.h"
#include "led.h"
#include "user_cmd.h"

static void hardware_init()
{
  NVIC_PriorityGroupConfig( NVIC_PriorityGroup_4 );		// �жϷ���
  led_init();                                         // ��ʼ�� LED
  systick_init();                                     // ʱ�ӳ�ʼ��
  /* ��ʼ�� shell ����̨ */
  shell_hw_init(115200);                              // ��ʼ�� ����̨����Ӳ��
  shell_init("shell >" ,usart_puts);                  // ��ʼ�� ����̨���
  shell_input_init(&shell_1,usart_puts);              // ��ʼ�� ����
}

static void app_init()
{
  register_user_cmd();
}

int main(void)
{
  hardware_init();
  app_init();
  while(1){
    led_app();                // led ָʾ��
    shell_hw_input();         // shell ����̨Ӧ��ѭ��          
  }
}
