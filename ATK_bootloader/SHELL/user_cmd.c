#include "user_cmd.h"
#include "shell.h"
#include "esp32_8266_at.h"
#include "sys.h"
#include "usart1.h"
#include "download.h"
#include "stdlib.h"
#include "app_start.h"
#include "flash.h"
#include "ota.h"

static void SoftReset(void* arg)
{ 
  (void)arg;
  __set_FAULTMASK(1); // 关闭所有中端
  NVIC_SystemReset(); // 复位
}

// esp 触发连接AP
static void connect_server(void* arg)
{ 
  esp_connect_start();
}

// 设置 ESP 的 wifi名称 和 密码
static void esp_set_ssid_pass(void * arg)
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

  write_sys_parameter();
}

// 读取 系统 参数
static void sys_information_get(void * arg)
{
  SYS_PARAMETER_READ;
  
  if(sys_parameter.app1_flag == APP_OK) debug_info("APP1_OK\r\n");
  else debug_info("APP1_ERR\r\n");
  debug_info("app1_fw_version:%s\r\n",sys_parameter.app1_fw_version);
  
  if(sys_parameter.app2_flag == APP_OK) debug_info("APP2_OK\r\n");
  else debug_info("APP2_ERR\r\n");
  debug_info("app2_fw_version:%s\r\n",sys_parameter.app2_fw_version);
  
  debug_info("wifi ssid:%s\r\n",sys_parameter.wifi_ssid);
  debug_info("wifi passwd:%s\r\n",sys_parameter.wifi_pwd);
}

// 启动 IAP 模式
// 默认升级 2 号分区
static void start_IAP_mode(void * arg)
{
  char * argv[2];
  int argc =cmdline_strtok((char*)arg,argv,2);
  if(argc<2)download_part = DEFAULT_PARTITION;
  else download_part=atoi(argv[1]);
  usart1_mode=1;
}

// 启动 APP
// 默认启动 2 号分区
static void Start_APP(void * arg)
{
  char * argv[2];
  int partition;
  
  int argc =cmdline_strtok((char*)arg,argv,2);
  if(argc<2) partition = DEFAULT_PARTITION;
  else partition=atoi(argv[1]);
  sys_parameter.current_part=partition;
  write_sys_parameter();
  start_app_partition(partition);
}

// 开始进行OTA升级。命令指定分区
// 默认升级 2 号分区
static void __OTA_update_start(void * arg)
{
  char * argv[2];
  int argc =cmdline_strtok((char*)arg,argv,2);
  if(argc<2)download_part = DEFAULT_PARTITION;
  else download_part=atoi(argv[1]);
  OTA_mission_start(1);
}

// 发送升级版本
// 默认发布 2 号分区
static void __OTA_post_version(void * arg)
{
  char * argv[2];
  int argc =cmdline_strtok((char*)arg,argv,2);
  if(argc<2)download_part = DEFAULT_PARTITION;
  else download_part=atoi(argv[1]);
  OTA_mission_start(0);
}

// 用户命令注册
void register_user_cmd()
{
  shell_register_command("reboot",SoftReset);
  shell_register_command(SYS_INFO_GET,sys_information_get);
  shell_register_command("esp_connect",connect_server);
  shell_register_command(ESP_SET_SSID_PASS_CMD,esp_set_ssid_pass);
  shell_register_command(IAP_CMD,start_IAP_mode);
  shell_register_command(APP_START,Start_APP);
  shell_register_command(OTA_UPDATE,__OTA_update_start);
  shell_register_command(OTA_POST_VERSION,__OTA_post_version);
}
