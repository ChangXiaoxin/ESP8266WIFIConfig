# ESP8266WIFI模块说明-V1.0

## 1. 运行环境
- stm32f1xx系列 @72Mhz
- AT固件版本  ESP8266_AT_Bin_V1.6.2
- 硬件模块  ESP-WROOM-02D

*(若要在别的主频下或者stm32f4xx系列中使用，请根据时钟速度，自行更改TIM4计时器中的参数达到时钟匹配)*
## 2. 文件结构说明（code目录下）
- esp8266.c : ESP8266模块功能状态机实现
- esp8266.h
- espusart.c ：配置与ESP8266模块通信的串口
- espusart.h
- key.c ：调试使用的按键(中断)
- key.h
- sys.c ：delay.c依赖文件
- sys.h
- delay.c ：提供延时函数
- delay.h

## 3. 模块功能说明
模块正常工作时使用AT固件的透传模式进行数据传输，在设备端可以将其当作是与服务器连接的串口直接对数据进行处理即可

第一次使用时要先进入配网模式对需要连入的WIFI热点信息（SSID & PWD）以及TCP服务器信息（IP & PORT）进行配置
- 若成功连接热点以及TCP服务器，自动进入透传模式进行正常工作，下次启动自动连接WIFI和TCP服务器并进入透传模式
- 若连接失败则自动重新进入配网模式
- 在配网模式时可手动退出配网模式

默认使用按键操作触发配网模式和手动推出配网模式，触发方式可以根据项目自定义
### 3.1 串口模块（espusart.c & espusart.h）
#### 3.1.1 ESP通信串口
通过串口使用AT指令对ESP8266模块进行配置，可以通过修改代码对不同串口进行配置
#### 3.1.2 调试串口
可使用串口输出调试信息
- 正常运行：无输出
- 一键配网："ESP: Start config done!"
- 接收到数据："ESP: Received Data2:%s %s %s %s"
- WIFI连接成功: "ESP: Wifi connected!"
- Wifi连接失败："ESP: Wifi connected error!"
- TCP连接成功： "ESP: TCP connected!"
- TCP连接失败： "ESP: TCP connected error!"
- 手动退出配网： "ESP: Wifi forced to stop!"

### 3.2 配网功能（key.c & key.h）
可以通过按键或者其他方式进行配网
#### 3.2.1 使用实体按键配网
- 当模块处于正常运行模式时，按下按键可以进入配网模式，配网成功后自动切换回正常运行模式，配网失败时重新进入配网模式
- 当模块处于配网模式时，可按下按键强制退出配网模式，进入正常运行模式

#### 3.2.2 其他方式触发配网
可以自行修改配网模式的触发方式，具体操作见`config.md`中`3.2.2`

### 3.3 用户数据处理（esp8266.c & esp8266.h & espusart.c）
#### 3.3.1 串口接收配置
可通过串口中断中解析代码的替换来解析不同协议的数据
#### 3.3.2 数据解析配置
可通过对主任务中数据解析函数的修改来对不同协议数据进行解析并作出相应动作

## 4. 主任务状态机说明
### 4.1 状态说明
- NORMAL：正常工作状态，透传模式传输数据
- START_CFG: 配网模式AT指令发送
- WAIT_CFG: 等待用户发送配网信息
- CHECK_CONT: 检测WIFI连接
- CHECK_TCP: 检测TCP连接
- RESTART_CFG: 重启WIFI模块
- FORCE_STOP: 手动退出配网模式
- PRE_NORMAL: 准备进入正常工作模式

### 4.2 状态转换表
|当前状态|下一状态|转换条件|备注|
| --- | --- | --- | --- |
|NORMAL|START_CFG|按键或者其他方式触发一键配网|初始状态|
|START_CFG|WAIT_CFG|命令发送完成自动切换||
|WAIT_CFG|START_CONT|接收到特定格式数据||
|START_CONT|CHECK_CONT|命令发送完成自动切换||
|CHECK_CONT|CHECK_TCP|WIFI连接成功||
|CHECK_CONT|START_CFG|WIFI连接失败||
|CHECK_TCP|NORMAL|TCP连接成功||
|CHECK_TCP|START_CFG|TCP连接失败||
|FORCE_STOP|NORMAL|按键或者其他方式触发退出配网||

### 4.3 状态转换图
![state](state.jpg)
