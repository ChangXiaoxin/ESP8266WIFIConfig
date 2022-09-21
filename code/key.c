#include "key.h"
#include "stm32f10x.h"
#include "esp8266.h"
//#include "led.h"

	
//extern void ChangeState(u8 newstate);
extern u8 espState;
/************************************************************************
PAx ~ PGx 端口的中断事件都连接到了EXTIx，即同一时刻EXTIx只能响应一个端口
多个 GPIO 口的时间无法同一时间响应，但是可以分时复用
EXTI最普通的应用就是接上一个按键，设置为下降沿触发，用中断来检测按键
************************************************************************/

void KeyExtiInit(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;

    //打开GPIOE时钟和AFIO时钟
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE );
    
    /*****************************************************************************
    NVIC_IRQChannel指需要配置的中断向量。因使用PE4口的按键，所以配置在4通道
    如果使用GPIO_PIN_5~GPIO_PIN9的任意一个，    则配置通道为EXTI9_5_IRQn
    如果使用GPIO_PIN_10~GPIO_PIN15的任意一个，则配置通道为EXTI15_10_IRQn
    ******************************************************************************/
    NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    
    NVIC_Init( &NVIC_InitStructure );

    
    // PE4外部中断配置
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;//上拉输入
    GPIO_Init( GPIOA, &GPIO_InitStructure );
    
    GPIO_EXTILineConfig( GPIO_PortSourceGPIOA, GPIO_PinSource6 );
    EXTI_InitStructure.EXTI_Line = EXTI_Line6;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;//下降沿中断
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    
    EXTI_Init( &EXTI_InitStructure );
    
}

/********************************************************
中断服务函数写在stm32f10x_it_c中，新定义一个函数即可。
它的名字必须要与启动文件startup中的中断向量表定义一致。
中断函数入口的函数名只有下面两种写法：
（1）
EXTI0_IRQHandler    ;EXTI Line 0
EXTI1_IRQHandler    ;EXTI Line 1
EXTI2_IRQHandler    ;EXTI Line 2
EXTI3_IRQHandler    ;EXTI Line 3
EXTI0_IRQHandler    ;EXTI Line 4
（2）
EXTI9_5_IRQHandler    ;EXTI Line 5~9
EXTI15_10_IRQHandler    ;EXTI Line 10~15
********************************************************/
// void LedOn(void){
	// GPIO_ResetBits(GPIOA,GPIO_Pin_12);	
// }
// void LedOff(void){
	// GPIO_SetBits(GPIOA,GPIO_Pin_12);	
// }

void EXTI9_5_IRQHandler( void )
{
    //确保产生了 EXTI Line 中断
    if( EXTI_GetITStatus( EXTI_Line6 ) != RESET )
    {
        //LED2_REV;
		switch(espState){		//切换WIFI模块工作模式
			case WAIT_CFG:
			case CHECK_CONT:
			case CHECK_TCP:
				ChangeState(FORCE_STOP);		//手动退出配网
				break;
			case NORMAL:
				ChangeState(START_CFG);			//进入配网模式
				break;
			default:
				break;
		}
		//LED1=!LED1;
        //清除中断标志位
        EXTI_ClearITPendingBit( EXTI_Line6 );
    }
}
