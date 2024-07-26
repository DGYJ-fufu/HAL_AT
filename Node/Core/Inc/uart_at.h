//
// Created by 24171 on 2024/7/23.
//

#ifndef UART_AT_H
#define UART_AT_H

#include "main.h"
#include "usart.h"

#define AT_UART huart2
#define AT_UARTRx_DMA hdma_usart2_rx

#define WIFI_SSID "CMCC-1407"
#define WIFI_PASSWORD "CX612345"

#define MQTT_HOST "192.168.10.15"
#define MQTT_PORT 1883

#define MQTT_ClientID "SmokeFlames01"
#define MQTT_USER   "SmokeFlames01"
#define MQTT_PASSWORD   "12345678"

typedef struct {
    char keyname[50];
    char value[30];
} PUB_DATA;

#define SENSOR_NUM 2
extern PUB_DATA mqtt_pub_data[SENSOR_NUM];

#define TOPIC_HEAD "home/"
#define IDENTIFIERS "SmokeFlames01"
#define PUB_TOPIC (TOPIC_HEAD IDENTIFIERS)

extern uint8_t AT_RX_BUFF[1024];
extern uint8_t AT_RX_FLAG;
extern uint8_t AT_RX_LEN;
extern uint8_t AT_TX_FLAG;
extern uint8_t MQTT_LINK_FLAG;

void AT_UART_Init(); // 初始化AT模块串口
char AT_Send_Cmd(char *cmd, char *ack, unsigned int waitms, unsigned char cnt); // 发送AT指令
void AT_MQTT_Init(); // MQTT初始化
void Publish_MQTT_message(); // 消息发布
void Get_MQTT_message(); // 解析订阅消息

#endif //UART_AT_H
