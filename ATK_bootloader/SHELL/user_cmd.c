#include "user_cmd.h"
#include "shell.h"
#include "esp32_at.h"

void SoftReset(void* arg)
{ 
  (void)arg;
  __set_FAULTMASK(1); // �ر������ж�
  NVIC_SystemReset(); // ��λ
}

void connect_ap(void* arg)
{ 
  esp32_connect_ap_start();
}

void register_user_cmd()
{
  shell_register_command("reboot",SoftReset);
  shell_register_command("connect_ap",connect_ap);
}
