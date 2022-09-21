#include "esp8266.h"
#include "stm32f10x.h"
#include "delay.h"
#include "espusart.h"
#include "string.h"
#include "key.h"

char* espIP = "192.168.1.1";	//esp配置模式热点IP
char* espSSID = "ESP8266";		//esp配置模式热点SSID
char* espPWD = "12345678";		//esp配置模式热点PWD
char* espPORT = "1234";			//esp配置模式热点PORT

u8 espState = NORMAL;					//ESP工作状态，初始化为NORMAL（透传传输数据）
u8 done = 0;							
u16 delayCounter = DELAY_TIME;			
u16 heartCounter = 50000;
u32 heartIndex = 0;
u8 ledonoff = 0;
u8 rx_buff_temp[150]={0};     //临时存放接收到的数据

extern u8 usart1Buffer[USART1_REC_LEN];	//Esp串口接收缓存
extern u8 usart1Rx;						//接收计数
extern u8 usart1S;						//帧起始位
extern u8 usart1Len;					//数据帧长度
extern u16 usart1Sta;					//串口状态/计数
extern u8 espRxState;
extern ESP_BufTypeDef ESP_RX_BUF;

ESPTypeDef ESP;
/**************************************************
*函数名： 	EspReadQueneData
*函数功能：	ESP接收缓存读取数据
*输入参数：	缓存地址
*返回值： 	返回读取的结果
****************************************************/
u8 EspReadQueneData(ESP_BufTypeDef* Rx_buf)
{
    u8 read_data_temp;
	if(Rx_buf->length == 0)    //没有数据
	{
	    read_data_temp = 0;
	}
	else
	{
	    read_data_temp = Rx_buf->buf[Rx_buf->fornt];  //读出头指针所指的数据
		Rx_buf->fornt = (Rx_buf->fornt + 1) % ESP_BUF_SIZE;//头指针增加
		Rx_buf->length --;  //数据长度减少
	}
	return read_data_temp;  //返回读取的结果
}

/**************************************************
*函数名： 	FlashLed
*函数功能：	Led改变显示状态(调试使用)
*输入参数：	无
*返回值： 	无
****************************************************/
void FlashLed(void){
	ledonoff = ~ledonoff;
	if(ledonoff){
		//LedOn();		//开灯
	}else{
		//LedOff();		//关灯
	}
}

/**************************************************
*函数名： 	WaitCfg
*函数功能：	等待用户配置，解析用户发送的SSID，PWD, IP, PORT数据
*输入参数：	无
*返回值： 	无
****************************************************/
u8 WaitCfg(void){
	u8 data_temp;				//临时存放数据
	u8 pt_temp = 0;             //rx_buff_temp的下标rx_buff_temp
	u8 length_temp = 0;   		//临时数组的当前长度
	u8 i;						//清除数组数据索引
	char* SSIDPtr	=	NULL;	//SSID字符串首地址指针
	char* PWDPtr	=	NULL;	//PWD字符串首地址指针
	char* IPPtr		=	NULL;	//IP字符串首地址指针
	char* PORTPtr	=	NULL;	//PORT字符串首地址指针
	char* ENDPtr	=	NULL;	//END字符串首地址指针
	for(i = 0; i < 150; i++){	//初始化接收缓存
		rx_buff_temp[i] = 0;
	}
	if(espRxState > 0){		//接收一次数据
		do{
			if(length_temp < 150){
				data_temp = EspReadQueneData(&ESP_RX_BUF);//从缓冲区读取一个值到数组
				rx_buff_temp[pt_temp] = data_temp;
				pt_temp ++;  //下标向后移动一位
				length_temp ++;
			}else{ 
				break;  
			}
		}while(data_temp != '\0');	
		espRxState--; //读取完一个字符串，接收区计数减一								    
	}	
	SSIDPtr = strstr((char*)rx_buff_temp, "SSID:");		//提取SSID首地址
	PWDPtr = strstr((char*)rx_buff_temp, "PWD:");		//提取PWD首地址
	IPPtr = strstr((char*)rx_buff_temp, "IP:");			//提取IP首地址
	PORTPtr = strstr((char*)rx_buff_temp, "PORT:");		//提取PORT首地址
	ENDPtr = strstr((char*)rx_buff_temp, "END");		//提取END首地址
	if((SSIDPtr!=NULL) && (PWDPtr!=NULL) && (IPPtr!=NULL) && (PORTPtr!=NULL) && (ENDPtr!=NULL)){	//如果传入数据符合预定格式
		*(ENDPtr - 1) = 0;		//为PORT添加字符串结束符
		*(PORTPtr - 1) = 0;		//为IP添加字符串结束符
		*(IPPtr - 1) = 0;		//为PWD添加字符串结束符
		*(PWDPtr - 1) = 0;		//为SSID添加字符串结束符
		ESP.ssid = SSIDPtr+5;	//将SSID指针指向用户传入的SSID内容
		ESP.pwd = PWDPtr+4;		//将PWD指针指向用户传入的PWD内容
		ESP.ip = IPPtr+3;		//将IP指针指向用户传入的IP内容
		ESP.port = PORTPtr+5;	//将PORT指针指向用户传入的PORT内容
		#ifdef DEBUG
			printf("\tESP: Received Data2:%s %s %s %s\r\n", ESP.ssid, ESP.pwd, ESP.ip, ESP.port);	//串口输出接收到数据
		#endif
		return 1;
	}else{
		return 0;
	}
}
	
/**************************************************
*函数名： 	CheckEspCmd
*函数功能：	Esp指令返回值验证
*输入参数：	需要匹配的字符串
*返回值： 	匹配到字符首次出现的地址或NULL
****************************************************/
u8 CheckEspCmd(char* str,char*str2){
	char *str_temp;               //被检测的字符串的临时变量
	char* str_temp2;
	u8 data_temp;
	u8 rx_buff_temp1[150]={0};     //临时存放接收到的数据
	u8 pt_temp = 0;               //rx_buff_temp的下标rx_buff_temp
	u8 length_temp = 0;   //临时数组的当前长度
	str_temp = str;
	str_temp2 = str2;
	if(espRxState > 0){
		do{
			if(length_temp < 150){
				data_temp = EspReadQueneData(&ESP_RX_BUF);//从缓冲区读取一个值到数组
				rx_buff_temp1[pt_temp] = data_temp;
				pt_temp ++;  //下标向后移动一位
				length_temp ++;
			}else{ 
				break;  
			}
		}while(data_temp != '\0');	
		espRxState--; //读取完一个字符串，接收区计数减一								    
	}	
	if(strstr((const char *)rx_buff_temp1, (const char *)str_temp ) != NULL){  //检测到有想要的回复，就能得到不为空的地址
		return 1;   //
	}else if(strstr((const char*)rx_buff_temp1, (const char*)str_temp2) != NULL){
		return 2;
	}else{
		return 0;    //如果没有，就是失败·??ò￡?·μ??ê§°ü	
	}
}

/**************************************************
*函数名： 	ChangeState
*函数功能：	切换Esp工作状态,清零公用变量
*输入参数：	要切换的目标状态
*返回值： 	无
****************************************************/
void ChangeState(u8 newstate){
	if(espState != newstate){
		espState = newstate;
		done = 0;
		usart1Sta = 0;
	}
}

/**************************************************
*函数名： 	SendHeart
*函数功能：	发送心跳包数据
*输入参数： 无
*返回值： 	无
****************************************************/
void SendHeart(void){
	if(heartCounter>=50000){		//5s发送一次心跳包数据
		heartCounter = 0;
		heartIndex++;
		EspPrintf("WiFi_Heart: %d\r\n", heartIndex);
	}
}
/**************************************************
*函数名： 	EspInit
*函数功能：	硬件初始化
*输入参数： 无
*返回值： 	无
****************************************************/
void EspInit(void){
	EspUsartInit(115200);
	
};

/**************************************************
*函数名： 	EspTest
*函数功能：	注册样机演示交互
*输入参数： 无
*返回值： 	无
****************************************************/
void EspTest(void){
	if(usart1Sta){	
		switch(usart1Buffer[usart1S + 1]){	
			case 0x11:					//接收到5A 11 A5
				Send2EspStr("WiFi_Get: 11\r\n");
				break;
			case 0x22:					//接收到5A 22 A5
				Send2EspStr("WiFi_Get: 22\r\n");
				break;
			default:
 
				break;
		}	
		usart1Sta = 0;
	}
}

/**************************************************
*函数名： 	Send2EspStr
*函数功能：	发送字符串给ESP模块
*输入参数：	要发送的字符串
*返回值： 	无
****************************************************/
void Send2EspStr(char* str){
	USART1->SR;
	while(*str){
		USART1->DR = *str++;
		while((USART1->SR&0x40) == 0);
	}
}


/**************************************************
*函数名： 	ProcessData
*函数功能：	传输数据处理
*输入参数：	无
*返回值： 	无
****************************************************/
void ProcessData(void){	
	SendHeart();	//发送心跳包
	EspTest();		//注册样机测试
}

/**************************************************
*函数名： 	EspStartSmartCfg
*函数功能：	发送SmartConfig的AT指令
*输入参数：	无
*返回值： 	无
****************************************************/
void EspStartCfg(void){
	Send2EspStr("+++");		//退出透传模式
	delay_ms(100);
	EspPrintf("\r\n");		//防止未在透传模式时误发送命令
	EspPrintf("AT+CWMODE=2\r\n");	//将ESP8266配置成AP模式
	delay_ms(100);
	EspPrintf("AT+CIPAP=\"%s\"\r\n", espIP);//设置本机IP
	delay_ms(100);
	EspPrintf("AT+CWSAP=\"%s\",\"%s\",1,3\r\n", espSSID, espPWD);	//设置AP信息
	delay_ms(1000);
	delay_ms(1000);
	delay_ms(1000);
	EspPrintf("AT+CIPMODE=0\r\n");		//设置非透传模式
	delay_ms(100);
	EspPrintf("AT+CIPMUX=1\r\n");		//设置多连接模式
	delay_ms(100);
	EspPrintf("AT+CIPSERVER=1,%s\r\n", espPORT);	//开启TCP服务器，监听1234端口
	delay_ms(100);
	#ifdef DEBUG
		printf("\tESP: Start config done!\r\n");	//输出调试信息
	#endif
};

/**************************************************
*函数名： 	EspStopSmartCfg
*函数功能：	发送配置TCP服务器及软重启的AT指令
*输入参数：	无
*返回值： 	无
****************************************************/
void EspStartCont(void){
	EspPrintf("AT+CWMODE=1\r\n");		//将ESP8266配置成STA模式
	delay_ms(100);
	EspPrintf("AT+CWJAP=\"%s\",\"%s\"\r\n", ESP.ssid, ESP.pwd);		//接入Wifi
	delay_ms(100);	
};

/**************************************************
*函数名： 	EspForceStop
*函数功能：	用户手动退出配置模式
*输入参数：	无
*返回值： 	无
****************************************************/
void EspForceStop(void){
	EspPrintf("AT+CWMODE=1\r\n");	//将ESP8266配置成STA模式
	delay_ms(100);
	EspPrintf("AT+CIPSERVER=0\r\n");	//关闭服务器模式
	delay_ms(100);
	EspPrintf("AT+CIPMUX=0\r\n");		//切换单连接模式
	delay_ms(100);
	#ifdef DEBUG
		printf("\tESP: Wifi forced to stop!\r\n");	//输出调试信息
	#endif
	EspPrintf("AT+RST\r\n");	//重启模块
	delay_ms(1000);
}

/**************************************************
*函数名： 	CheckWifi
*函数功能：	检查Wifi连接情况
*输入参数：	无
*返回值： 	1---连接成功   2---连接失败    0---检测中
****************************************************/
u8 CheckWifi(void){
	u8 wifiSta=0;
	wifiSta = CheckEspCmd("OK","FAIL");		//检测返回信息
	if(wifiSta == 1){						//返回“OK”
		#ifdef DEBUG
			printf("\tESP: Wifi connected!\r\n");	//输出调试信息
		#endif
		EspPrintf("AT+CIPSERVER=0\r\n");		//关闭服务器模式
		delay_ms(100);
		EspPrintf("AT+CIPMUX=0\r\n");			//切换单连接模式
		delay_ms(100);
		EspPrintf("AT+CIPSTART=\"TCP\",\"%s\",%s\r\n",ESP.ip,ESP.port);	//连接TCP服务器
		return 1;
	}else if(wifiSta == 2){					//返回“FAIL”
		#ifdef DEBUG
			printf("\tESP: Wifi connected error!\r\n");		//输出调试信息
		#endif
		delay_ms(100);
		return 2;
	}else{									//还未返回
		return 0;
	}
}

/**************************************************
*函数名： 	CheckTcp
*函数功能：	检查TCP服务器连接情况
*输入参数：	无
*返回值： 	1---连接成功   2---连接失败    0---检测中
****************************************************/
u8 CheckTcp(void){
	u8 tcpSta;
	tcpSta = CheckEspCmd("CONNECT","ERROR");	//检测返回信息
	if(tcpSta == 1){							//返回“CONNECT”
		#ifdef DEBUG
			printf("\tESP: TCP connected!\r\n");
		#endif
		EspPrintf("AT+SAVETRANSLINK=1,\"%s\",%s\r\n",ESP.ip,ESP.port);	//设置开机TCP透传
		EspPrintf("AT+RST\r\n");				//重启模块
		return 1;
	}else if(tcpSta == 2){						//返回“ERROR”
		#ifdef DEBUG
			printf("\tESP: TCP connected error!\r\n");	//输出调试信息
		#endif
		delay_ms(100);
		EspPrintf("AT+RST\r\n");				//重启模块
		delay_ms(1000);
		return 2;
	}else{
		return 0;
	}
}

/**************************************************
*函数名： 	EspTask
*函数功能：	ESP模块状态机
*输入参数：	无
*返回值： 	无
****************************************************/
void EspTask(void){
	switch(espState){
		case NORMAL:			//正常工作模式：透传收发数据
			ProcessData();		//数据处理
			break;
		case START_CFG:			//配网模式
			if(0 == done){	
				EspStartCfg();		//ESP8266切换AP模式开始配网
				done = 1;
			}				
			ChangeState(WAIT_CFG); 
			break;
		case WAIT_CFG:			//等待用户发送密码数据
			if(WaitCfg()){		//接收到给定格式的数据
				ChangeState(START_CONT);
			}
			break;
		case START_CONT:				//开始连接WIFI
			if(0 == done){
				EspStartCont();			//开始连接WIFI
				done = 1;
			}
			ChangeState(CHECK_CONT);		
			break;
		case CHECK_CONT:				//检测WIFI连接
			switch(CheckWifi()){
				case 1:	
					ChangeState(CHECK_TCP);		//WIFI连接成功，开始连接TCP服务器
					break;
				case 2:
					ChangeState(START_CFG);		//WIFI连接失败，重新开始配网
					break;
				default:
					break;
			}
			break;
		case CHECK_TCP:					//检测TCP连接
			switch(CheckTcp()){	
				case 1:		
					ChangeState(NORMAL);	//TCP连接成功，开始进入正常工作模式
					#ifdef DEBUG
						printf("\tESP: Config successful!\r\n");	//输出调试信息
					#endif
					break;
				case 2:
					ChangeState(START_CFG);		//TCP连接失败，重新开始配网
					break;
				default:
					break;
			}
			break;	
		case FORCE_STOP:				//退出配网模式
			EspForceStop();				//取消配网，进入STA模式
			ChangeState(NORMAL);
			break;
		default:
			break;
	}	
}
