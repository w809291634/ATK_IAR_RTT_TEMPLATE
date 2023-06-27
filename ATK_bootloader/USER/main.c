#include "stm32f4xx.h"
#include "systick.h"
#include "user_cmd.h"
#include "usart1.h"
#include "usart3.h"
#include "led.h"
#include "esp32_at.h"
#include "soft_timer.h"

// Ӳ����ʼ��
static void hardware_init()
{
  NVIC_PriorityGroupConfig( NVIC_PriorityGroup_4 );		// �жϷ���
  led_init();                             // ��ʼ�� LED
  systick_init();                         // ʱ�ӳ�ʼ��
  softTimer_Init();                       // �����ʱ����ʼ��
  
  /* ��ʼ�� shell ����̨ */
  shell_hw_init(115200);                  // ��ʼ�� ����̨����Ӳ��
  shell_init("shell >" ,usart1_puts);     // ��ʼ�� ����̨���
  shell_input_init(&shell_1,usart1_puts); // ��ʼ�� ����
  
  /* ��ʼ�� ESP8266 WiFi at���� */
  esp32_at_hw_init(115200);               // ��ʼ�� ESP8266 WiFi at���� 
}

// Ӧ�ó�ʼ��
static void app_init()
{
  register_user_cmd();
  esp32_at_app_init();
  led_app_init();  
}

// ������
int main(void)
{
  hardware_init();
  app_init();
  while(1){
    softTimer_Update();         // �����ʱ��ɨ��
    shell_app_cycle();          // shell ����̨Ӧ��ѭ�� 
    esp32_at_app_cycle();       // esp32 ��Ӧ��ѭ��
  }
}
