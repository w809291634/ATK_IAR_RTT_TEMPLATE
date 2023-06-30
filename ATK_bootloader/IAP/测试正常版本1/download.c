#include "download.h"
#include "config.h"
#include "ymodem.h"

extern uint8_t file_name[FILE_NAME_LENGTH];
uint8_t tab_1024[1024] = {0};

// 通过串口 IAP 向 flash 下载一个文件
void IAP_download_cycle(void)
{
  uint8_t Number[10] = "          ";
  int32_t Size = 0;

  printk("Waiting for the file to be sent ... (press 'a' to abort)\n\r");
  Size = Ymodem_Receive(&tab_1024[0]);
  if (Size > 0)
  {
    printk("\n\n\r Programming Completed Successfully!\n\r--------------------------------\r\n Name: ");
    printk((char*)file_name);
    Int2Str(Number, Size);
    printk("\n\r Size: ");
    printk((char*)Number);
    printk(" Bytes\r\n");
    printk("-------------------\n");
  }
  else if (Size == -1)
  {
    debug_err(ERR"\n\n\rProgramming address error!\n\r");
  }
  else if (Size == -2)
  {
    debug_err(ERR"\n\n\rErase flash error!\n\r");
  }
  else if (Size == -3)
  {
    debug_err(ERR"\n\n\rProgramming flash error!\n\r");
  }
  else if (Size == -10)
  {
    debug_war(WARNING"\r\n\nAborted by user.\n\r");
  }
  else
  {
    printk("\n\rFailed to receive the file!\n\r");
  }
}

