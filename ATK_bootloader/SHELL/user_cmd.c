#include "user_cmd.h"
#include "shell.h"
#include "esp32_at.h"
#include "sys.h"
#include "usart1.h"
#include "download.h"
#include "stdlib.h"
#include "app_start.h"
#include "flash.h"
#include "ota.h"

void SoftReset(void* arg)
{ 
  (void)arg;
  __set_FAULTMASK(1); // �ر������ж�
  NVIC_SystemReset(); // ��λ
}

// esp ��������AP
void connect_server(void* arg)
{ 
  esp32_connect_start();
}

// ���� ESP �� wifi���� �� ����
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

  write_sys_parameter();
}

// ��ȡ flash �д洢�� ssid
void esp_get_ssid_pass(void * arg)
{
  SYS_PARAMETER_READ;
  debug_info(INFO"wifi ssid:%s\r\n",sys_parameter.wifi_ssid);
  debug_info(INFO"wifi passwd:%s\r\n",sys_parameter.wifi_pwd);
}

// ���� IAP ģʽ
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

// ���� APP
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

// ��������汾
void __OTA_update_start(void * arg)
{
  char * argv[2];
  int argc =cmdline_strtok((char*)arg,argv,2);
  if(argc<2){
    debug_info(INFO"please input %s [<partition>] \r\n",OTA_UPDATE);
    return;
  }
  download_part=atoi(argv[1]);
  OTA_mission_start(1);
}

// ��������汾
void __OTA_post_version(void * arg)
{
  char * argv[2];
  int argc =cmdline_strtok((char*)arg,argv,2);
  if(argc<2){
    debug_info(INFO"please input %s [<partition>] \r\n",OTA_POST_VERSION);
    return;
  }
  download_part=atoi(argv[1]);
  OTA_mission_start(0);
}

// �û�����ע��
void register_user_cmd()
{
  shell_register_command("reboot",SoftReset);
  shell_register_command("esp_connect",connect_server);
  shell_register_command(ESP_SET_SSID_PASS_CMD,esp_set_ssid_pass);
  shell_register_command(ESP_GET_SSID_PASS_CMD,esp_get_ssid_pass);
  shell_register_command(IAP_CMD,start_IAP_mode);
  shell_register_command(APP_START,Start_APP);
  shell_register_command(OTA_UPDATE,__OTA_update_start);
  shell_register_command(OTA_POST_VERSION,__OTA_post_version);
}
