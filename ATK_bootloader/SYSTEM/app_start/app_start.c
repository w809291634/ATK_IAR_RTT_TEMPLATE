#include "app_start.h"

// ��λ����Ĵ���
static void peripheral_reset(void)
{
  // �رյδ�ʱ������λ��Ĭ��ֵ
  SysTick->CTRL = 0;
  SysTick->LOAD = 0;
  SysTick->VAL = 0;
  
  // ʹ�ܸ�λ
  RCC->APB2RSTR = 0xFFFFFFFF;
  RCC->AHB1RSTR = 0xFFFFFFFF;
  RCC->AHB2RSTR = 0xFFFFFFFF;
  RCC->AHB3RSTR = 0xFFFFFFFF;

  // �رո�λ
  RCC->APB2RSTR = 0x00000000;
  RCC->AHB1RSTR = 0x00000000;
  RCC->AHB2RSTR = 0x00000000;
  RCC->AHB3RSTR = 0x00000000;
}

// ��λ�жϿ�����
void nvic_reset(void) 
{
  // �رյδ�ʱ������λ��Ĭ��ֵ
  SysTick->CTRL = 0;
  SysTick->LOAD = 0;
  SysTick->VAL = 0;
  
  // ���������ж�
  NVIC_DisableIRQ(USART1_IRQn);
  NVIC_DisableIRQ(USART3_IRQn);
  // ���� ������Ҫ���������ж�

  // ��������жϹ����־λ
  NVIC_ClearPendingIRQ(USART1_IRQn);
  NVIC_ClearPendingIRQ(USART3_IRQn);
  // ���� ������Ҫ��������жϹ����־λ
}

// ��λ�жϿ�����
static void NVIC_Reset(void) 
{
  int i;
  __set_FAULTMASK(1); // �ر������ж�
  
  // �ر������ж�,��������жϹ����־
  for (i = 0; i < 8; i++) {
    NVIC->ICER[i] = 0xFFFFFFFF; 
    NVIC->ICPR[i] = 0xFFFFFFFF; 
  }

  // ���������ж����ȼ�ΪĬ��ֵ
  for (i = 0; i < 240; i++) {
      NVIC->IP[i] = 0;
  }
  
  __set_FAULTMASK(0); // ʹ�������ж�
}

// ��ת��ָ����ַ����
void JumpToApp(uint32_t app_addr)
{
  pFunction Jump_To_Application; 
  uint32_t JumpAddress;	
  
  /* peripheral_reset */
  NVIC_Reset();
  peripheral_reset();
  
  /* Jump to user application */
  JumpAddress = *(volatile uint32_t*)(app_addr + 4);
  Jump_To_Application = (pFunction) JumpAddress;
  
  /* Initialize user application's Stack Pointer */
  __set_PSP(*(volatile uint32_t*)app_addr);
  __set_CONTROL(0);
  __set_MSP(*(volatile uint32_t*)app_addr);
  Jump_To_Application();

  /* ��ת�ɹ��Ļ�������ִ�е�����û�������������Ӵ��� */
  while (1)
  {
  }
}

// ����appϵͳ����
void start_app_partition(uint8_t partition)
{
  switch(partition){
    case 1:{
      if(sys_parameter.app1_flag==APP_OK){
        printk(INFO"Starting partition 1!\r\n");
        printk(INFO"Next automatic start partition 1!\r\n");
        JumpToApp(APP1_START_ADDR);
      }else{
        printk(INFO"Starting partition 1 fail!\r\n");
      }
    }break;
    case 2:{
      if(sys_parameter.app2_flag==APP_OK){
        printk(INFO"Starting partition 2!\r\n");
        printk(INFO"Next automatic start partition 2!\r\n");
        JumpToApp(APP2_START_ADDR);
      }else{
        printk(INFO"Starting partition 2 fail!\r\n");
      }
    }break;
    case 0xff:break;
    default:{
      printk(INFO"partition %d not assigned!\r\n",partition);
    }break;
  }
}
