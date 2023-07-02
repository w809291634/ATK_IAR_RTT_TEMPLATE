#include "user_cmd.h"
#include "shell.h"
#include "esp32_at.h"
#include "config.h"
#include "usart1.h"
#include "download.h"
#include "stdlib.h"
#include "app_start.h"

void SoftReset(void* arg)
{ 
  (void)arg;
  __set_FAULTMASK(1); // 关闭所有中端
  NVIC_SystemReset(); // 复位，大多数外设模块的寄存器状态会在复位时回到其默认值
}

// esp 触发连接AP
void connect_ap(void* arg)
{ 
  esp32_connect_ap_start();
}

// 设置 ESP 的 wifi名称 和 密码
void esp_set_ssid_pass(void * arg)
{
  char * argv[3];
  int argc =cmdline_strtok((char*)arg,argv,3);
  if(argc<3){
    debug_info(INFO"please input %s [<ssid>] [<pwd>]\r\n",ESP_SET_SSID_PASS_CMD);
    return;
  }
  if(strlen(argv[1])>50 || strlen(argv[2])>50 ){
    debug_err(ERR"%s ssid and password failed!\r\n",ESP_SET_SSID_PASS_CMD);
    return;
  }
  strcpy(sys_parameter.wifi_ssid,argv[1]);
  strcpy(sys_parameter.wifi_pwd,argv[2]);
  sys_parameter.wifi_flag=FLAG_OK;
  
  write_sys_parameter();
}

// 读取 flash 中存储的 ssid
void esp_get_ssid_pass(void * arg)
{
  SYS_PARAMETER_READ;
  if(sys_parameter.wifi_flag!=APP_OK){
    debug_info(INFO"Please use %s cmd set wifi parameter!\r\n",ESP_SET_SSID_PASS_CMD);
  }
  debug_info(INFO"wifi ssid:%s\r\n",sys_parameter.wifi_ssid);
  debug_info(INFO"wifi passwd:%s\r\n",sys_parameter.wifi_pwd);
}

// 启动 IAP 模式
void start_IAP_mode(void * arg)
{
  char * argv[2];
  int argc =cmdline_strtok((char*)arg,argv,2);
  if(argc<2){
    debug_info(INFO"please input %s [<partition>] \r\n",IAP_CMD);
    return;
  }
  download_part=atoi(argv[1]);
  usart1_mode=1;
}

// 启动 APP
void Start_APP(void * arg)
{
  char * argv[2];
  int argc =cmdline_strtok((char*)arg,argv,2);
  if(argc<2){
    debug_info(INFO"please input %s [<partition>] \r\n",APP_START);
    return;
  }
  int partition=atoi(argv[1]);
  sys_parameter.current_part=partition;
  write_sys_parameter();
  start_app_partition(partition);
}

// 用户命令注册
void register_user_cmd()
{
  shell_register_command("reboot",SoftReset);
  shell_register_command("esp_connect_ap",connect_ap);
  shell_register_command(ESP_SET_SSID_PASS_CMD,esp_set_ssid_pass);
  shell_register_command(ESP_GET_SSID_PASS_CMD,esp_get_ssid_pass);
  shell_register_command(IAP_CMD,start_IAP_mode);
  shell_register_command(APP_START,Start_APP);
}
