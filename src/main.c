#include <arch/antares.h>
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"
#include <stm32f10x.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_tim.h>
#include <stm32f10x_usart.h>
#include <stm32f10x_i2c.h>

#include <stdio.h>

/* I2C addr with RW BIT. RTFM! */
#define MPU6050      	    0xd1

static USART_InitTypeDef usart1 = {
	.USART_BaudRate = 115200,
	.USART_WordLength = USART_WordLength_8b,
	.USART_StopBits = USART_StopBits_1,
	.USART_Parity = USART_Parity_No,
	.USART_Mode = USART_Mode_Rx | USART_Mode_Tx,
	.USART_HardwareFlowControl = USART_HardwareFlowControl_None,	
};

static GPIO_InitTypeDef usart_tx = {
	.GPIO_Pin = GPIO_Pin_9,
	.GPIO_Speed = GPIO_Speed_50MHz,
	.GPIO_Mode = GPIO_Mode_AF_PP
};

static GPIO_InitTypeDef usart_rx = {
	.GPIO_Pin = GPIO_Pin_10,
	.GPIO_Speed = GPIO_Speed_50MHz,
	.GPIO_Mode = GPIO_Mode_IN_FLOATING
};

static USART_ClockInitTypeDef USART_ClockInitStructure;

void usart1_init()
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
	GPIO_Init(GPIOA, (GPIO_InitTypeDef*) &usart_tx);
	GPIO_Init(GPIOA, (GPIO_InitTypeDef*) &usart_rx);
	USART_ClockStructInit(&USART_ClockInitStructure);
	USART_ClockInit(USART1, &USART_ClockInitStructure);
	USART_Init(USART1, &usart1);
	USART_Cmd(USART1, ENABLE);
}


static GPIO_InitTypeDef gpc = {
	.GPIO_Pin = GPIO_Pin_9|GPIO_Pin_8,
	.GPIO_Speed = GPIO_Speed_50MHz,
	.GPIO_Mode = GPIO_Mode_Out_PP
};

ANTARES_INIT_LOW(gpio_init)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOC, ENABLE);
//	RCC->APB2ENR |= 0x10 | 0x04;					
	GPIO_Init(GPIOC, &gpc);
}

void greeter()
{
	int a;
	printf("Hello, world\n\r");
	scanf("%d", &a);
	printf("===> %d\n",a);
}

void init_i2c()
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB|RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
	
	/* I2C Power control hack */
	GPIO_InitTypeDef PORTB_init_struct;

	PORTB_init_struct.GPIO_Pin = GPIO_Pin_6;
	PORTB_init_struct.GPIO_Speed = GPIO_Speed_50MHz;
	PORTB_init_struct.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &PORTB_init_struct);
	/* That should provide a logic 3.3v for MPU6050 */
	GPIO_SetBits(GPIOC,GPIO_Pin_6);
	

	PORTB_init_struct.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_7;
	PORTB_init_struct.GPIO_Speed = GPIO_Speed_50MHz;
	PORTB_init_struct.GPIO_Mode = GPIO_Mode_AF_OD;
	GPIO_Init(GPIOB, &PORTB_init_struct);

	I2C_DeInit(I2C1);
	I2C_InitTypeDef I2C_init_struct;
	I2C_init_struct.I2C_Mode = I2C_Mode_I2C;
	I2C_init_struct.I2C_DutyCycle = I2C_DutyCycle_16_9;
	I2C_init_struct.I2C_OwnAddress1 = 1;
	I2C_init_struct.I2C_Ack = I2C_Ack_Enable;
	I2C_init_struct.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	I2C_init_struct.I2C_ClockSpeed = 400000;
	I2C_Cmd(I2C1, ENABLE);
	I2C_Init(I2C1, &I2C_init_struct);
	I2C_AcknowledgeConfig(I2C1, ENABLE);

}


void i2c_detect() 
{
	printf("Trying to detect any i2c slaves...\n\r");
	uint8_t addr = 0;
	int i;
	while (1) {
		init_i2c();
		i=40000;
		while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY)); 
		I2C_GenerateSTART(I2C1, ENABLE);
		while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));
		I2C_Send7bitAddress(I2C1, addr++, I2C_Direction_Transmitter);
		while (1) {
			if (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
			{
				printf("Found something @ %hhx\n\r",addr);
				break;
			}
			i--;
			if (!i)
			{
				printf("NACK @ %hhx\n\r",addr);
				break;	
			}
		}
	}
}

uint8_t i2c_read(uint8_t DeviceAddr, uint8_t IntAddr)
{
	printf("r\n\r");
	while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY)); 
	I2C_GenerateSTART(I2C1, ENABLE);
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));
	printf("r\n\r");
	I2C_Send7bitAddress(I2C1, MPU6050, I2C_Direction_Transmitter);
        while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
	printf("ack\n\r");
	I2C_SendData(I2C1,IntAddr);							  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
	printf("r\n\r");
	I2C_GenerateSTART(I2C1, ENABLE);						  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));			    I2C_Send7bitAddress(I2C1, MPU6050, I2C_Direction_Receiver);
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED));
	I2C_NACKPositionConfig(I2C1, I2C_NACKPosition_Current);			
	I2C_GenerateSTOP(I2C1, ENABLE);				    
	return I2C_ReceiveData(I2C1);

}

int main()
{
	int i=500000;
	while(i--);;
	gpio_init();
	usart1_init();
	//init_i2c();
	printf("I2C init done\n\r");
	i2c_detect();
	char b = i2c_read(MPU6050,0x0);
	printf("b == %x \n", b);
}
