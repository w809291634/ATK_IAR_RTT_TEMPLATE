#include "download.h"
#include "config.h"
#include "ymodem.h"

extern uint8_t file_name[FILE_NAME_LENGTH];
uint8_t download_part;

// 通过串口 IAP 向 flash 下载一个文件
void IAP_download(void)
{
  uint8_t Number[10] = "          ";
  int32_t Size = 0;
  uint32_t partition_start ,partition_size;
  uint32_t timeout=30*1000;
  int32_t errarr[MAX_ERRORS]={0};
  
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
  printf("The current download partition is app%d,start_address:0x%08x,size:%d Bytes\n\r",
     download_part,partition_start,partition_size);
  printf("Waiting for the file to be sent ... (press 'a' key to exit IAP mode)\n\r");
  
  // 阻塞运行
  Size = Ymodem_Receive(partition_start,partition_size,timeout,errarr);
  hw_ms_delay(500);
  if (Size > 0)
  {
    printf("\n\n\r Programming Completed Successfully!\n\r--------------------------------\r\n Name: %s",(char*)file_name);
    Int2Str(Number, Size);
    printf("\n\r Size: %s Bytes\r\n",(char*)Number);
    printf("-------------------\r\n");
    if(download_part==1) sys_parameter.app1_flag=APP_OK;
    else if(download_part==2) sys_parameter.app2_flag=APP_OK;
    write_sys_parameter();
  }
  else if (Size == 0) debug_err("\n\r"ERR"Termination by sender!\n\r");
  else if (Size == -1) debug_err("\n\r"ERR"Programming address error or File too large!\n\r");
  else if (Size == -2) debug_err("\n\r"ERR"Erase flash error!\n\r");
  else if (Size == -3) debug_err("\n\r"ERR"Programming flash error!\r\n");
  else if (Size == -10) {
    debug_err("\n\r"ERR"Error count exceeded.\r\n");
    for(int i=0;i<ARRAY_SIZE(errarr);i++){
      debug_err(ERR"Error%d=%d.\r\n",i,errarr[i]);
    }
  }
  else if (Size == -11) debug_err("\n\r"ERR"Waiting timeout\n\r");
  else if (Size == -30) debug_war("\n\r"WARNING"Aborted by user.\n\r");
  else debug_err("\n\r"ERR"Failed to receive the file!\n\r");

RESET:
  /* 进入控制台模式 */
  usart1_mode=0;
}

