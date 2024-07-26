//
// Created by 24171 on 2024/7/23.
//

#include "uart_at.h"
#include "stdarg.h"
#include <stdio.h>
#include <string.h>

uint8_t AT_RX_BUFF[1024] = {0}; // AT模块数据接收缓冲区
uint8_t AT_RX_FLAG = 0; // AT模块响应接收标志 0:未接收到 1:接收到
uint8_t AT_TX_FLAG = 1; // AT模块指令发送标志 0:未发送完成 1:发送完成
uint8_t MQTT_LINK_FLAG = 0; // AT模块MQTT连接状态 0:未连接 1:已连接

/**
 * 初始化AT模块串口
 */
void AT_UART_Init() {
    HAL_UARTEx_ReceiveToIdle_DMA(&AT_UART, AT_RX_BUFF, 1024);
    __HAL_DMA_DISABLE_IT(&AT_UARTRx_DMA, DMA_IT_HT);
}

int AT_UART_Send_String(const char *str, ...) {
    va_list arg;
    static char AT_TX_BUFF[1024] = {0};
    int len = 0;
    while (!AT_TX_FLAG);

    va_start(arg, str);
    len = vsnprintf(AT_TX_BUFF, sizeof(AT_TX_BUFF) + 1, (char *) str, arg);
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
char AT_Send_Cmd(char *cmd, char *ack, unsigned int waitms, unsigned char cnt) {
    AT_UART_Send_String(cmd); // 1.发送AT指令
    while (cnt--) {
        // 时间间隔
        HAL_Delay(waitms);
        // 串口中断接收蓝牙应答
        if (AT_RX_FLAG == 1) {
            AT_RX_FLAG = 0;
            // 查找是否有想要的数据
            if (strstr(AT_RX_BUFF, ack) != NULL) {
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
void AT_MQTT_Init() {
    char CMD_BUFF[1024] = {0};

RST:
    // AT模块测试
    AT_Send_Cmd("AT\r\n", "OK", 100, 2);
    // 配置WIFI工作模式
    AT_Send_Cmd("AT+WMODE=1,1\r\n", "OK", 1000, 2);
    // 连接WIFI
    sprintf(CMD_BUFF, "AT+WJAP=%s,%s\r\n",WIFI_SSID,WIFI_PASSWORD);
    AT_Send_Cmd(CMD_BUFF, "OK", 8000, 2);
    memset(CMD_BUFF, '\0', strlen(CMD_BUFF));
    // 检查网络是否联通
    sprintf(CMD_BUFF, "AT+PING=%s\r\n",MQTT_HOST);
    if (!AT_Send_Cmd(CMD_BUFF, "OK", 8000, 2)) {
        // 网络连接疑似异常
        AT_Send_Cmd("AT+RESTORE\r\n", "OK", 1000, 2);
        goto RST;
    }
    memset(CMD_BUFF, '\0', strlen(CMD_BUFF));

    sprintf(CMD_BUFF, "AT+MQTT=1,%s\r\n", MQTT_HOST);
    AT_Send_Cmd(CMD_BUFF, "OK", 500, 2); // 配置MQTT Host
    memset(CMD_BUFF, '\0', strlen(CMD_BUFF));

    sprintf(CMD_BUFF, "AT+MQTT=2,%d\r\n", MQTT_PORT); // 配置MQTT Port
    AT_Send_Cmd(CMD_BUFF, "OK", 500, 2);
    memset(CMD_BUFF, '\0', strlen(CMD_BUFF));

    AT_Send_Cmd("AT+MQTT=3,1\r\n", "OK", 500, 2); // 配置MQTT连接方式

    sprintf(CMD_BUFF, "AT+MQTT=4,%s\r\n", MQTT_ClientID);
    AT_Send_Cmd(CMD_BUFF, "OK", 500, 2); // 配置MQTT ClientID
    memset(CMD_BUFF, '\0', strlen(CMD_BUFF));


    sprintf(CMD_BUFF, "AT+MQTT=5,%s\r\n", MQTT_USER); // 配置MQTT用户名
    AT_Send_Cmd(CMD_BUFF, "OK", 500, 2);
    memset(CMD_BUFF, '\0', strlen(CMD_BUFF));

    sprintf(CMD_BUFF, "AT+MQTT=6,%s\r\n", MQTT_PASSWORD); // 配置MQTT密码
    AT_Send_Cmd(CMD_BUFF, "OK", 500, 2);

    if (!AT_Send_Cmd("AT+MQTT\r\n", "OK", 500, 3)) // 连接MQTT
    {
        AT_Send_Cmd("AT+RESTORE\r\n", "OK", 1000, 2);
    }

    HAL_Delay(2000);

    AT_Send_Cmd("AT+MQTTPUB=home/switch1/status,0,0,online\r\n", "OK", 1000, 2); // HA设备上线

    AT_Send_Cmd("AT+MQTTSUB=home/switch1/set,0\r\n", "OK", 1000, 2); // 订阅Switch

    MQTT_LINK_FLAG = 1;
}


/**
 * 传感器结构
 */
PUB_DATA mqtt_pub_data[SENSOR_NUM] = {
    {"Smoke", 0},
    {"Flames", "0"}
};

/**
 * 消息发布
 */
void Publish_MQTT_message() {
    char CMD[400] = {0};
    char params[300] = {0}, i, *sp;

    sp = params;

    for (i = 0; i < SENSOR_NUM; i++) {
        if (i < SENSOR_NUM - 1) {
            sprintf(sp, "\\\"%s\\\":%s\\,", mqtt_pub_data[i].keyname, mqtt_pub_data[i].value);
            while (*sp != 0) sp++;
        } else {
            sprintf(sp, "\\\"%s\\\":%s", mqtt_pub_data[i].keyname, mqtt_pub_data[i].value);
        }
    }

    sprintf(CMD, "AT+MQTTPUB=%s,0,0,\"%s\"\r\n",PUB_TOPIC, params);
    AT_UART_Send_String(CMD);
}

/**
 * 解析订阅消息
 */
void Get_MQTT_message() {
    if (strstr(AT_RX_BUFF, ":MQTT_SUB,home/switch1/set") != NULL) {
        if (strstr(AT_RX_BUFF, "ON") != NULL) {
            HAL_GPIO_WritePin(LED_GPIO_Port,LED_Pin, GPIO_PIN_SET);
        } else if (strstr(AT_RX_BUFF, "OFF") != NULL) {
            HAL_GPIO_WritePin(LED_GPIO_Port,LED_Pin, GPIO_PIN_RESET);
        }
        AT_RX_FLAG = 0;
    }
}
