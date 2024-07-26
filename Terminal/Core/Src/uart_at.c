//
// Created by 24171 on 2024/7/23.
//

#include "uart_at.h"
#include "stdarg.h"
#include <stdio.h>
#include <string.h>
#include "tim.h"

uint8_t AT_RX_BUFF[1024] = {0}; // AT模块数据接收缓冲区
uint8_t AT_RX_FLAG = 0;         // AT模块响应接收标志 0:未接收到 1:接收到
uint8_t AT_TX_FLAG = 1;         // AT模块指令发送标志 0:未发送完成 1:发送完成
uint8_t MQTT_LINK_FLAG = 0;     // AT模块MQTT连接状态 0:未连接 1:已连接

/**
 * 初始化AT模块串口
 */
void AT_UART_Init()
{
    HAL_UARTEx_ReceiveToIdle_DMA(&AT_UART, AT_RX_BUFF, 1024);
    __HAL_DMA_DISABLE_IT(&AT_UARTRx_DMA, DMA_IT_HT);
}

int AT_UART_Send_String(const char *str, ...)
{
    va_list arg;
    static char AT_TX_BUFF[1024] = {0};
    int len = 0;
    while (!AT_TX_FLAG)
        ;

    va_start(arg, str);
    len = vsnprintf(AT_TX_BUFF, sizeof(AT_TX_BUFF) + 1, (char *)str, arg);
    va_end(arg);

    HAL_UART_Transmit_DMA(&AT_UART, AT_TX_BUFF, len);
    AT_TX_FLAG = 0;

    return len;
}

/**
 * 发送AT指令
 * @param cmd 要发送的AT指令
 * @param ack AT指令的响应
 * @param waitms 响应等待时间
 * @param cnt 响应次数
 * @return 响应结果 0:未响应 1:收到响应
 */
char AT_Send_Cmd(char *cmd, char *ack, unsigned int waitms, unsigned char cnt)
{
    AT_UART_Send_String(cmd); // 1.发送AT指令
    while (cnt--)
    {
        // 时间间隔
        HAL_Delay(waitms);
        // 串口中断接收蓝牙应答
        if (AT_RX_FLAG == 1)
        {
            AT_RX_FLAG = 0;
            // 查找是否有想要的数据
            if (strstr(AT_RX_BUFF, ack) != NULL)
            {
                return 1;
            }
            // 清除接收的数据
            memset(AT_RX_BUFF, '\0', 1024);
        }
    }
    AT_RX_FLAG = 0;
    return 0;
}

/**
 * MQTT初始化
 */
void AT_MQTT_Init()
{
    char CMD_BUFF[1024] = {0};

RST:
    // AT模块测试
    AT_Send_Cmd("AT\r\n", "OK", 100, 2);
    // 配置WIFI工作模式
    AT_Send_Cmd("AT+CWMODE=3\r\n", "OK", 1000, 2);
    // 连接WIFI
    sprintf(CMD_BUFF, "AT+CWJAP=\"%s\",\"%s\"\r\n", WIFI_SSID, WIFI_PASSWORD);
    AT_Send_Cmd(CMD_BUFF, "OK", 8000, 2);
    memset(CMD_BUFF, '\0', strlen(CMD_BUFF));

    sprintf(CMD_BUFF, "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"\r\n", MQTT_ClientID, MQTT_USER, MQTT_PASSWORD);
    AT_Send_Cmd(CMD_BUFF, "OK", 500, 2); // 配置MQTT Host
    memset(CMD_BUFF, '\0', strlen(CMD_BUFF));

    sprintf(CMD_BUFF, "AT+MQTTCONN=0,\"%s\",%d,1\r\n", MQTT_HOST, MQTT_PORT); // 连接MQTT
    AT_Send_Cmd(CMD_BUFF, "OK", 1000, 2);
    memset(CMD_BUFF, '\0', strlen(CMD_BUFF));

    HAL_Delay(2000);

    AT_Send_Cmd("AT+MQTTSUB=0,\"home/SmokeFlames01\",1\r\n", "OK", 1000, 2); // 订阅烟雾火焰

    MQTT_LINK_FLAG = 1;
}

/**
 * 传感器结构
 */
PUB_DATA mqtt_pub_data[SENSOR_NUM] = {
    {"Smoke", 0.0},
    {"Flames", 0.0}
};

/**
 * 解析订阅消息
 */
void Get_MQTT_message()
{
    char *ptr;
    char *token;
    char tempBuff[256]; // 临时缓冲区，假设与 AT_RX_BUFF 大小相同
    
    // 将 AT_RX_BUFF 复制到临时缓冲区 tempBuff
    strncpy(tempBuff, AT_RX_BUFF, sizeof(tempBuff));
    
    // 检查 MQTT 消息是否匹配预期格式
    if (strstr(tempBuff, "MQTTSUBRECV:0,\"home/SmokeFlames01\"") != NULL)
    {
        // 使用 strtok() 分割消息
        ptr = strtok(tempBuff, ":,");
        while (ptr != NULL)
        {
            // 检查是否找到 "Smoke" 键
            if (strcmp(ptr, "\"Smoke\"") == 0)
            {
                token = strtok(NULL, ":,"); // 获取下一个标记
                if (token != NULL)
                {
									if((int)(atof(token))>8){
										Smoke_FLAG = 1;
									}
									else{
										Smoke_FLAG = 0;
									}
									if((int)atof(token)/10==0)
                    sprintf(mqtt_pub_data[0].value,"%.2f ",atof(token));
									else
										sprintf(mqtt_pub_data[0].value,"%.2f",atof(token));
                }
            }
            // 检查是否找到 "Flames" 键
            else if (strcmp(ptr, "\"Flames\"") == 0)
            {
                token = strtok(NULL, ":,"); // 获取下一个标记
								if(atoi(token)){
									Fire_FLAG = 1;
								}
								else{
									Fire_FLAG = 0;
								}
                if (token != NULL)
                {
                    sprintf(mqtt_pub_data[1].value,"%d",atoi(token));
                }
            }
            ptr = strtok(NULL, ":,"); // 移动到下一个标记
        }
        
        AT_RX_FLAG = 0; // 处理完 MQTT 消息后清除标志
    }
}
