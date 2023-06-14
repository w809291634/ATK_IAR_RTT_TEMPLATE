#include "stm32f4xx.h"
#include "delay.h"

#include "usart1.h"

static void hardware_init()
{
  NVIC_PriorityGroupConfig( NVIC_PriorityGroup_4 );		// �жϷ���
  delay_init(84);

  // ��ʼ�� shell ����̨
  shell_hw_init(115200);
  shell_init("shell >" ,usart_puts);          // ��ʼ�� ����̨���
  shell_input_init(&shell_1,usart_puts);      // ��ʼ�� ����
}

int main(void)
{
  hardware_init();
  while(1){
    shell_hw_input();         // shell ����̨Ӧ��ѭ��          
  }
}
