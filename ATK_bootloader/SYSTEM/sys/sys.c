#include "sys.h"
#include "usart1.h"

//��׼����Ҫ��֧�ֺ���                 
struct __FILE 
{ 
	int handle; 
};
FILE __stdout;   

//����_sys_exit()�Ա���ʹ�ð�����ģʽ    
void _sys_exit(int x) 
{ 
	x = x; 
} 

//�ض���fputc���� 
int fputc(int ch, FILE *f)
{      
  usart1_put(ch);
  return 0;
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

// �б����Ƿ�����ַ���
// obj Ŀ���ַ���  list �ַ����б�  len �б���
int list_contains_str(char* str,char** list,int len)
{
  for(int i=0;i<len;i++){
    if(strstr(str,list[i]))
      return 1;
  }
  return 0;
}

// ����Ԫ�����ƣ�ʹ��0���
// arr ����   size ����Ԫ�ظ���  shiftAmount �ƶ�Ԫ�ظ���
void leftShiftCharArray(char* arr, int size, int shiftAmount) 
{
  memmove(arr, arr + shiftAmount, (size - shiftAmount) * sizeof(char));
  memset(arr + size - shiftAmount, 0, shiftAmount * sizeof(char));
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

