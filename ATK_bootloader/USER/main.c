#include "stm32f4xx.h"
#include "systick.h"
#include "user_cmd.h"
#include "usart1.h"
#include "usart3.h"
#include "led.h"
#include "esp32_at.h"
#include "soft_timer.h"
#include "config.h"
#include "flash.h"
#include "download.h"
#include "app_start.h"

// ϵͳ������������
sys_parameter_t sys_parameter;

// ϵͳ��ʼ��
static void system_init()
{
  /* ��ʼ������Ӳ�� */
  NVIC_SetVectorTable(NVIC_VectTab_FLASH,0);          // ��ӳ���ж�������
  NVIC_PriorityGroupConfig( NVIC_PriorityGroup_4 );		// �жϷ���
  shell_hw_init(115200);                              // ��ʼ�� ����̨����Ӳ��
  /* ���� app */
  SYS_PARAMETER_READ;
  start_app_partition(sys_parameter.current_part);
  
  /* ��ʼ�� shell ����̨ */
  shell_init("shell >" ,usart1_puts);     // ��ʼ�� ����̨���
  shell_input_init(&shell_1,usart1_puts); // ��ʼ�� ����
  welcome_gets(&shell_1,0,0);             // ������ʾ welcome'
  printf(INFO"Entered the bootloader program!\r\n");
  cmdline_gets(&shell_1,"\r",1);          // һ�λ���
}

// Ӳ����ʼ��
static void hardware_init()
{
  led_init();                             // ��ʼ�� LED
  systick_init();                         // ʱ�ӳ�ʼ��
  softTimer_Init();                       // �����ʱ����ʼ��
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
  system_init();
  hardware_init();
  app_init();
  while(1){
    softTimer_Update();         // �����ʱ��ɨ��
    esp32_at_app_cycle();       // esp32 ��Ӧ��ѭ��
    if(usart1_mode==0)
      shell_app_cycle();          // shell ����̨Ӧ��ѭ�� 
    else{
      IAP_download();
    }
  }
}
