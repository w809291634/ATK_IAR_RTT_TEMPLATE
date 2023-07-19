#include "user_cmd.h"
#include "board.h"
#include "flash.h"
#include <rthw.h>
#include <rtdevice.h>

static void enter_iap(uint8_t argc, char **argv)
{
  sys_parameter.current_part=0xff;  // 要求不启动分区
  write_sys_parameter();
  SYS_PARAMETER_READ;
  rt_kprintf("current_part:0x%02x\r\n",sys_parameter.current_part);
  rt_hw_cpu_reset();                // 系统重启复位
}
FINSH_FUNCTION_EXPORT_ALIAS(enter_iap, __cmd_enter_iap, Enter iap Bootloader);

