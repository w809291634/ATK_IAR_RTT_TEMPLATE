#include "download.h"
#include "config.h"
#include "ymodem.h"

extern uint8_t file_name[FILE_NAME_LENGTH];
uint8_t tab_1024[1024] = {0};

// ͨ������ IAP �� flash ����һ���ļ�
void IAP_download_cycle(void)
{
  uint8_t Number[10] = "          ";
  int32_t Size = 0;

  printk("Waiting for the file to be sent ... (press 'a' key to abort)\n\r");
  Size = Ymodem_Receive(&tab_1024[0]);            // ��������
  for(int i =1000000;i>0;i--){};    // Ҫ�ȴ�һ��
  if (Size > 0)
  {
    printk("\n\n\r Programming Completed Successfully!\n\r--------------------------------\r\n Name: %s",(char*)file_name);
    Int2Str(Number, Size);
    printk("\n\r Size: %s Bytes\r\n",(char*)Number);
    printk("-------------------\r\n");
  }
  else if (Size == 0) debug_err(ERR"\n\rTermination by sender!\n\r");
  else if (Size == -1) debug_err(ERR"\n\n\rProgramming address error!\n\r");
  else if (Size == -2) debug_err(ERR"\n\n\rErase flash error!\n\r");
  else if (Size == -3) debug_err(ERR"\n\n\rProgramming flash error!\n\r");
  else if (Size == -10) debug_err(ERR"\r\n\nError count exceeded.\n\r");
  else if (Size == -30) debug_war(WARNING"\r\n\nAborted by user.\n\r");
  else debug_err(ERR"\n\rFailed to receive the file!\n\r");

  // �������̨ģʽ
  usart1_mode=0;
}
