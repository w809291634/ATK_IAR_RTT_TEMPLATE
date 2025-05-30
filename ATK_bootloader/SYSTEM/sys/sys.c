#include "sys.h"
#include "usart1.h"
#include "flash.h"

uint8_t download_part;
uint8_t updating;

//标准库需要的支持函数                 
struct __FILE 
{ 
	int handle; 
};
FILE __stdout;   

//定义_sys_exit()以避免使用半主机模式    
void _sys_exit(int x) 
{ 
	x = x; 
} 

//重定义fputc函数 
int fputc(int ch, FILE *f)
{      
  usart1_put(ch);
  return ch;
}

void* my_memcpy(void* dest, const void* src, size_t num)
{
	void* ret = dest;
	assert_param(dest != NULL);
	assert_param(src != NULL);
  char* _dest=(char*)dest;
  char* _src=(char*)src;
	while (num--)
	{
		*_dest = *_src;
		_dest++;
    _src++;
	}
	return ret;
}

// 列表中是否包含字符串
// obj 目标字符串  list 字符串列表  len 列表长度
int list_contains_str(char* str,char** list,int len)
{
  for(int i=0;i<len;i++){
    if(strstr(str,list[i]))
      return 1;
  }
  return 0;
}

// 数组元素左移，使用0填充
// arr 数组   size 数组元素个数  shiftAmount 移动元素个数
void leftShiftCharArray(char* arr, int size, int shiftAmount) 
{
  memmove(arr, arr + shiftAmount, (size - shiftAmount) * sizeof(char));
  memset(arr + size - shiftAmount, 0, shiftAmount * sizeof(char));
}

// 系统参数恢复默认值
void sys_parameter_init(void)
{
  if(sys_parameter.parameter_flag!=FLAG_OK){
    strcpy(sys_parameter.wifi_ssid,WIFI_SSID);
    strcpy(sys_parameter.wifi_pwd,WIFI_PASSWD);
    strcpy(sys_parameter.app1_fw_version,SYS_FW_VERSION);
    strcpy(sys_parameter.app2_fw_version,SYS_FW_VERSION);
    write_sys_parameter();
  }
}

int isASCIIString(const char* str)
{
  while(*str){
    if (*str <= 0 || *str > 127) return 0;    // 不是 ASCII 字符串
    str++;
  }
  return 1;  // 是 ASCII 字符串
}

#ifdef  USE_FULL_ASSERT
 /**
   * @brief  Reports the name of the source file and the source line number
   *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
   * @param  line: assert_param error line source number
   * @retval None
   */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
  printf("\r\nWrong parameters value: file %s on line %d\r\n", file, line);*/
  printf("\r\nWrong parameters value: file %s on line %d\r\n", file, line);
  /* Infinite loop */
  while (1)
  {
  }
}
#endif

