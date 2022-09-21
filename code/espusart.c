#include "espusart.h"
#include "stdio.h"	
#include "esp8266.h"
#include "string.h"
#include "stdarg.h"



//加入以下代码,支持printf函数,而不需要选择use MicroLIB	  
#ifdef DEBUG		//调试串口使用USART3
#pragma import(__use_no_semihosting)             
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

//__use_no_semihosting was requested, but _ttywrch was 
void _ttywrch(int ch)
{
ch = ch;
}
//重定义fputc函数 
int fputc(int ch, FILE *f)
{      
	while((USART3->SR&0X40)==0){};//循环发送,直到发送完毕   
    USART3->DR = (u8) ch;      
	return ch;
}
#endif 

u8 usart1Buffer[USART1_REC_LEN];
u8 usart1TBuffer[USART1_REC_LEN];
u8 usart1Rx = 0;
u8 usart1S = 0;
u8 usart1Len = 0;
u16 usart1Sta = 0;
extern u8 espState; 
u8 espRxState = 0;
ESP_BufTypeDef ESP_RX_BUF;


#ifdef DEBUG
void DebugUsartInit(u32 baud)
{
  //GPIO端口设置
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);	//使能USART3时钟
  
  //USART3_TX   GPIOB.10
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//推挽输出
	GPIO_Init(GPIOB, &GPIO_InitStructure);//初始化GPIO
   
  //USART3_RX	  GPIOB.11
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
	GPIO_Init(GPIOB, &GPIO_InitStructure);//初始化GPIO
	
  //USART 初始化设置
	USART_InitStructure.USART_BaudRate = baud;//串口波特率
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
	USART_Init(USART3, &USART_InitStructure); //初始化串口3


  //Usart3 NVIC 中断初始化
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=4 ;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
  
	
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);//开启串口接受中断
	USART_Cmd(USART3, ENABLE);                    //使能串口3

}


//调试串口中断服务程序
void USART3_IRQHandler(void)                	
{
	u8 Res;
	if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)  //接收中断
	{
		Res =USART_ReceiveData(USART3);	//读取接收到的数据
		USART_SendData(USART1,Res);		//发送数据		 
	} 

} 
#endif

void TIM4_Init(u16 arr,u16 psc);


void EspUsartInit(u32 baud)
{
      //GPIO端口设置
    GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1|RCC_APB2Periph_GPIOA, ENABLE);	//使能USART1，GPIOA时钟
	//USART1_TX   GPIOA.9
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; //PA.9
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
    GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化GPIOA.9
   
    //USART1_RX	  GPIOA.10初始化
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;//PA10
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
    GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化GPIOA.10  

    //Usart1 NVIC 配置
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
  
    //USART 初始化设置

	USART_InitStructure.USART_BaudRate = baud;//串口波特率
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式

    USART_Init(USART1, &USART_InitStructure); //初始化串口1
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//开启串口接受中断
    USART_Cmd(USART1, ENABLE);                    //使能串口1 
	USART_ClearITPendingBit(USART1, USART_IT_RXNE);
	TIM4_Init(1000-1, 720-1);
	#ifdef DEBUG
		DebugUsartInit(115200);
	#endif
}



void EspRxBufInit(void)
{
//	  int i ;
	  memset(ESP_RX_BUF.buf,0,sizeof(ESP_RX_BUF.buf));   //使用memset（）函数需要包含头文件<string.h>
//	for(i=0;i< ESP_BUF_SIZE; i++)
//	  {
//	      ESP_RX_BUF.buf[i] = 0;
//	  }
      ESP_RX_BUF.fornt = 0;
	  ESP_RX_BUF.length = 0;
	  ESP_RX_BUF.rear = 0;
	
	  espRxState = 0;          //?êDí?óê?êy?Y
}


//ESP串口中断服务程序
void USART1_IRQHandler(void)                	
{
	u8 Res;
	
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)  //接收中断
	{
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
		Res =USART_ReceiveData(USART1);	//读取接收到的数据
		switch(espState){
			case START_CFG:
			case CHECK_CONT:
			case CHECK_TCP:
			case WAIT_CFG:
				#ifdef DEBUG
					//USART_SendData(USART3,Res);
				#endif
				if(ESP_RX_BUF.length <= ESP_BUF_SIZE) //如果数据没有溢出è?1?êy?Y??óDò?3?
				{									 
					ESP_RX_BUF.buf[ESP_RX_BUF.rear] = Res;			//将数据存入缓冲区尾部??êy?Y′?è?BUFF?o3????22?
					ESP_RX_BUF.length ++;                           //缓冲区长度增加?o3???3¤?è???ó
					ESP_RX_BUF.rear = (ESP_RX_BUF.rear + 1) % ESP_BUF_SIZE;  //尾指针+1，防止溢出，实现循环队列·à?1ò?3?
				}		
				//这里的TIM3其实可以用宏定义，这样可以方便更改使用的定时器
				TIM_SetCounter(TIM4,0);                         //定时器清空??êy?÷????
				TIM_Cmd(TIM4,ENABLE);                           //使能定时器ê1?ü?¨ê±?÷	
				break;
			case NORMAL:
				#ifdef DEBUG
					//USART_SendData(USART3,Res);
				#endif
				usart1Buffer[usart1Rx] = Res;
				usart1Rx++;
				usart1Rx &= 0xFF;
				if(usart1Buffer[usart1Rx-1] == 0x5A){
					usart1S = usart1Rx - 1;
				}
				if((usart1Buffer[usart1S] == 0x5A) && (usart1Buffer[usart1Rx - 1] == 0xA5)){
					usart1Len = usart1Rx-1 -usart1S;
					usart1Sta = 1;
				}
				if(USART_GetFlagStatus(USART1, USART_FLAG_ORE) == SET){
					USART_ClearFlag(USART1, USART_FLAG_ORE);
					USART_ReceiveData(USART1);
				}
				break;
			default:
				#ifdef DEBUG
					//USART_SendData(USART3,Res);
				#endif
				break;
		}
	}
} 

void TIM4_IRQHandler(void)
{ 	
	if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)//TIM4中断
	{	 			   
		espRxState++; 			//接收超时计数
	}	 
	TIM_ClearITPendingBit(TIM4, TIM_FLAG_Update  );  //清除标志位
	TIM_Cmd(TIM4, DISABLE);		//关闭中断
}


void TIM4_Init(u16 arr,u16 psc)
{	
	NVIC_InitTypeDef NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE); //打开外设时钟
	
	//配置定时器计数周期
	TIM_TimeBaseStructure.TIM_Period = arr; //计数值	
	TIM_TimeBaseStructure.TIM_Prescaler =psc; //分频系数
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure); //传入配置参数
 
	TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE ); //

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1); 	  
	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority= 0;//设置优先级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;		//设置响应优先级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//
	NVIC_Init(&NVIC_InitStructure);	//传入配置参数
	
}

void EspPrintf(char* fmt, ...){
	u16 i, j;
	va_list ap;
	va_start(ap, fmt);
	vsprintf((char*)usart1TBuffer, fmt, ap);
	va_end(ap);
	i = strlen((const char*)usart1TBuffer)+1;
	for(j = 0; j < i; j++){
		while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
		USART_SendData(USART1, usart1TBuffer[j]);
	}
}

