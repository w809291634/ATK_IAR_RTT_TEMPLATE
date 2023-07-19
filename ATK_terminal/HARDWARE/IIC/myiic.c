#include "myiic.h"
#include "config.h"

//��ʼ��IIC
void IIC_Init(void)
{			
  GPIO_InitTypeDef  GPIO_InitStructure={0};
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);//ʹ��GPIOBʱ��

  //GPIOB8,B9��ʼ������
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//��ͨ���ģʽ
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//�������
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//����
  GPIO_Init(GPIOB, &GPIO_InitStructure);//��ʼ��
	IIC_SCL_H;
	IIC_SDA_H;
}
//����IIC��ʼ�ź�
void IIC_Start(void)
{
	SDA_OUT();     //sda�����
	IIC_SDA_H;	  	  
	IIC_SCL_H;
	hw_us_delay(4);
 	IIC_SDA_L;//START:when CLK is high,DATA change form high to low 
	hw_us_delay(4);
	IIC_SCL_L;//ǯסI2C���ߣ�׼�����ͻ�������� 
}	  
//����IICֹͣ�ź�
void IIC_Stop(void)
{
	SDA_OUT();//sda�����
	IIC_SCL_L;
	IIC_SDA_L;//STOP:when CLK is high DATA change form low to high
 	hw_us_delay(4);
	IIC_SCL_H; 
	IIC_SDA_H;//����I2C���߽����ź�
	hw_us_delay(4);							   	
}
//�ȴ�Ӧ���źŵ���
//����ֵ��1������Ӧ��ʧ��
//        0������Ӧ��ɹ�
u8 IIC_Wait_Ack(void)
{
	u8 ucErrTime=0;
	SDA_IN();      //SDA����Ϊ����  
	IIC_SDA_H;hw_us_delay(1);	   
	IIC_SCL_H;hw_us_delay(1);	 
	while(READ_SDA)
	{
		ucErrTime++;
		if(ucErrTime>250)
		{
			IIC_Stop();
			return 1;
		}
	}
	IIC_SCL_L;//ʱ�����0 	   
	return 0;  
} 
//����ACKӦ��
void IIC_Ack(void)
{
	IIC_SCL_L;
	SDA_OUT();
	IIC_SDA_L;
	hw_us_delay(2);
	IIC_SCL_H;
	hw_us_delay(2);
	IIC_SCL_L;
}
//������ACKӦ��		    
void IIC_NAck(void)
{
	IIC_SCL_L;
	SDA_OUT();
	IIC_SDA_H;
	hw_us_delay(2);
	IIC_SCL_H;
	hw_us_delay(2);
	IIC_SCL_L;
}					 				     
//IIC����һ���ֽ�
//���شӻ�����Ӧ��
//1����Ӧ��
//0����Ӧ��			  
void IIC_Send_Byte(u8 txd)
{                        
  u8 t;   
	SDA_OUT(); 	    
  IIC_SCL_L;//����ʱ�ӿ�ʼ���ݴ���
  for(t=0;t<8;t++)
  {              
    if((txd&0x80)>>7) IIC_SDA_H;
    else IIC_SDA_L;
    txd<<=1; 	  
    hw_us_delay(2);   //��TEA5767��������ʱ���Ǳ����
    IIC_SCL_H;
    hw_us_delay(2); 
    IIC_SCL_L;	
    hw_us_delay(2);
  }	 
} 	    
//��1���ֽڣ�ack=1ʱ������ACK��ack=0������nACK   
u8 IIC_Read_Byte(unsigned char ack)
{
	unsigned char i,receive=0;
	SDA_IN();//SDA����Ϊ����
    for(i=0;i<8;i++ )
	{
    IIC_SCL_L; 
    hw_us_delay(2);
		IIC_SCL_H;
    receive<<=1;
    if(READ_SDA)receive++;   
		hw_us_delay(1); 
  }					 
  if (!ack)  IIC_NAck();//����nACK
  else       IIC_Ack(); //����ACK   
  return receive;
}

