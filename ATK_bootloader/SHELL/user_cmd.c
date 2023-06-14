#include "user_cmd.h"
#include "shell.h"

void SoftReset(void* arg)
{ 
  (void)arg;
  __set_FAULTMASK(1); // 关闭所有中端
  NVIC_SystemReset(); // 复位
}

void register_user_cmd()
{
  shell_register_command("reboot",SoftReset);
}
