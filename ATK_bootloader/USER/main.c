#include "stm32f4xx.h"
#include "usart.h"
#include "delay.h"



static void hardware_init()
{
	uart_init(115200);
	delay_init(84);
}


int main(void)
{
  hardware_init();

  u32 t=0;

  while(1){
    printf("t:%d\r\n",t);
		delay_ms(500);
		t++;

	}
}
