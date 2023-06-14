#include "user_cmd.h"
#include "shell.h"

void SoftReset(void* arg)
{ 
  (void)arg;
  __set_FAULTMASK(1); // �ر������ж�
  NVIC_SystemReset(); // ��λ
}

void register_user_cmd()
{
  shell_register_command("reboot",SoftReset);
}
