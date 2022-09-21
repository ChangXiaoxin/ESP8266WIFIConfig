#ifndef __ESPUSART_H
#define __ESPUSART_H 	

#include "stm32f10x.h"
#include "stdio.h "

#define USART1_REC_LEN  			50  	//定义最大接收字节数 50
#define ESP_BUF_SIZE 				1024 


#define DEBUG

typedef struct{
	unsigned char buf[ESP_BUF_SIZE];
	unsigned short int length;
	unsigned short int fornt;
	unsigned short int rear;
}ESP_BufTypeDef;

void EspPrintf(char* fmt, ...);
void EspUsartInit(u32 baud);


#endif
