#include "download.h"
#include "config.h"
#include "ymodem.h"

extern uint8_t file_name[FILE_NAME_LENGTH];
uint8_t packet_1024[1024] = {0};
uint8_t download_part;

// 通过串口 IAP 向 flash 下载一个文件
void IAP_download(void)
{
  uint8_t Number[10] = "          ";
  int32_t Size = 0;
  uint32_t partition_start ,partition_size;
  uint32_t timeout=30;
  
  if(download_part==1){
    /* 分区1 */
    partition_start=APP1_START_ADDR;
    partition_size=APP1_SIZE;
  }
  else if(download_part==2){
    /* 分区2 */
    partition_start=APP2_START_ADDR;
    partition_size=APP2_SIZE;
  }else {
    debug_err(ERR"\n\rPlease set partition to 1 or 2!\n\r");
    goto RESET;
  }
  
  /* 进入下载模式 */
  printk("Waiting for the file to be sent ... (press 'a' key to exit IAP)\n\r");
  
  // 阻塞运行
  Size = Ymodem_Receive(&packet_1024[0],partition_start,partition_size,timeout);
  hw_ms_delay(500);
  if (Size > 0)
  {
    printk("\n\n\r Programming Completed Successfully!\n\r--------------------------------\r\n Name: %s",(char*)file_name);
    Int2Str(Number, Size);
    printk("\n\r Size: %s Bytes\r\n",(char*)Number);
    printk("-------------------\r\n");
  }
  else if (Size == 0) debug_err(ERR"\n\rTermination by sender!\n\r");
  else if (Size == -1) debug_err(ERR"\n\n\rProgramming address error or File too large!\n\r");
  else if (Size == -2) debug_err(ERR"\n\n\rErase flash error!\n\r");
  else if (Size == -3) debug_err(ERR"\n\n\rProgramming flash error!\n\r");
  else if (Size == -10) debug_err(ERR"\r\n\nError count exceeded.\n\r");
  else if (Size == -30) debug_war(WARNING"\r\n\nAborted by user.\n\r");
  else debug_err(ERR"\n\rFailed to receive the file!\n\r");

RESET:
  /* 进入控制台模式 */
  usart1_mode=0;
}

