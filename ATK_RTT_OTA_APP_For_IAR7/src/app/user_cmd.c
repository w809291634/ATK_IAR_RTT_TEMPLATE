#include "user_cmd.h"
#include "config.h"
#include "flash.h"
#include <rthw.h>
#include <rtdevice.h>

static void enter_iap(uint8_t argc, char **argv)
{
  sys_parameter.current_part=0xff;  // Ҫ����������
  write_sys_parameter();
  rt_hw_cpu_reset();                // ϵͳ������λ
}
FINSH_FUNCTION_EXPORT_ALIAS(enter_iap, __cmd_enter_iap, Enter iap Bootloader);

