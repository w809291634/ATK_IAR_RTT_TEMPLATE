#include "user_cmd.h"
#include "shell.h"
#include "esp32_at.h"
#include "config.h"
#include "usart1.h"

void SoftReset(void* arg)
{ 
  (void)arg;
  __set_FAULTMASK(1); // �ر������ж�
  NVIC_SystemReset(); // ��λ�����������ģ��ļĴ���״̬���ڸ�λʱ�ص���Ĭ��ֵ
}

// esp ��������AP
void connect_ap(void* arg)
{ 
  esp32_connect_ap_start();
}

// ���� ESP �� wifi���� �� ����
void esp_set_ssid_pass(void * arg)
{
  char * argv[4];
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
  sys_parameter.flag=SYS_PARAMETER_OK;
  
  uint32_t num=SYS_PARAMETER_WRITE;
  if(SYS_PARAMETER_SIZE==num){
    debug_info(INFO"System Parameter Write Success!,num:%d word\r\n",num);
  }else{
    debug_err(ERR"System Parameter Write Failed!size:%d num:%d\r\n",SYS_PARAMETER_SIZE,num);
  }
}

// ��ȡ flash �д洢�� ssid
void esp_get_ssid_pass(void * arg)
{
  SYS_PARAMETER_READ;
  if(sys_parameter.flag!=SYS_PARAMETER_OK){
    debug_info(INFO"Please use %s cmd set wifi parameter!\r\n",ESP_SET_SSID_PASS_CMD);
  }
  debug_info(INFO"wifi ssid:%s\r\n",sys_parameter.wifi_ssid);
  debug_info(INFO"wifi passwd:%s\r\n",sys_parameter.wifi_pwd);
}

// ���ô���ģʽ
void set_usart_mode(void * arg)
{
  usart1_mode=1;
}

// �û�����ע��
void register_user_cmd()
{
  shell_register_command("reboot",SoftReset);
  shell_register_command("esp_connect_ap",connect_ap);
  shell_register_command(ESP_SET_SSID_PASS_CMD,esp_set_ssid_pass);
  shell_register_command(ESP_GET_SSID_PASS_CMD,esp_get_ssid_pass);
  shell_register_command("AT+IAP",set_usart_mode);
}
