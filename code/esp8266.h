#ifndef __ESP8266_H__
#define __ESP8266_H__
#include "stm32f10x.h"

#define DELAY_TIME         10000

#define  NORMAL	           0	//正常工作状态，透传模式传输数据
#define  START_CFG   	   1	//配网模式AT指令发送
#define  WAIT_CFG	   	   2	//等待用户发送配网信息
#define  START_CONT        3    //重启WIFI模块
#define  FORCE_STOP	       4	//手动退出配网模式
#define  PRE_NORMAL		   5	//准备进入正常工作模式
#define  CHECK_CONT        6    //检查WIFI连接
#define  CHECK_TCP		   7 	//检查TCP连接
typedef struct{
	char* ssid;
	char* pwd;
	char* ip;
	char* port;
}ESPTypeDef;

void EspInit(void);
void ChangeState(u8 newstate);
void EspTask(void);
void Send2EspStr(char* str);				    
#endif
