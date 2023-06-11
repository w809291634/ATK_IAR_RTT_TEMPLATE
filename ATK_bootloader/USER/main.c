#include "stm32f4xx.h"
#include "usart.h"
#include "delay.h"


static int hardware_init()
{
	uart_init(115200);
	delay_init(84);
}


int main(void)
{
	u32 t=0;
	
	hardware_init();
  while(1){
    printf("t:%d\r\n",t);
		delay_ms(500);
		t++;
		
		
		
	}
}
