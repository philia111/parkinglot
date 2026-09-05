#ifndef __ESP8266_MQTT_H__
#define __ESP8266_MQTT_H__

#include "stm32f4xx.h"

/* ==================== MQTT Broker 配置 ==================== */
/* 使用公共测试Broker，无需自建服务器，注册都不用 */
#define MQTT_BROKER_IP      "mqtt.bemfa.com"
#define MQTT_BROKER_PORT    9501
#define MQTT_CLIENT_ID      "00aa60925bad47830643ab464b859c24"
#define MQTT_KEEP_ALIVE     60   /* 心跳间隔(秒)，超过这个时间不发消息Broker会断开 */

/* ==================== 停车场 Topic 定义 ==================== */
#define TOPIC_PARKING_ENTER     "ParkingEnter"     /* 车辆入场通知 */
#define TOPIC_PARKING_EXIT      "ParkingExit"      /* 车辆出场通知 */
#define TOPIC_PARKING_STATUS    "ParkingStatus"    /* 车位状态(定时上报) */
#define TOPIC_PARKING_CMD       "ParkingCmd"       /* 远程控制指令(订阅) */

/* ==================== 函数声明 ==================== */

/**
 * @brief 一键启动MQTT连接（WiFi需要先连好）
 *        内部流程：连TCP → 进透传 → 发CONNECT报文
 * @return 1成功 0失败
 */
uint8_t MQTT_Start(void);

/**
 * @brief 发送MQTT CONNECT报文，与Broker建立MQTT会话
 * @param clientId  客户端标识，Broker用这个区分不同设备
 * @param keepAlive 心跳间隔(秒)
 * @return 1成功(收到CONNACK) 0失败
 */
uint8_t MQTT_Connect(const char *clientId, uint16_t keepAlive);

/**
 * @brief 订阅一个主题，Broker会把该主题的消息推送过来
 * @param topic 主题字符串，如 "parking/cmd"
 * @param qos   服务质量等级 0/1/2
 * @return 1成功(收到SUBACK) 0失败
 */
uint8_t MQTT_Subscribe(const char *topic, uint8_t qos);

/**
 * @brief 发布消息到指定主题 (QoS 0, 发完就走不等回复)
 * @param topic   主题字符串，如 "parking/enter"
 * @param message 消息内容，如 JSON 字符串
 * @return 1成功 0失败(消息太长)
 */
uint8_t MQTT_Publish(const char *topic, const char *message);

uint8_t MQTT_CheckMessage(char *topic_out, uint8_t topic_size, char *payload_out, uint8_t payload_size);


/**
 * @brief 发送心跳包，告诉Broker"我还活着"
 * @return 1成功(收到PINGRESP) 0失败
 */
uint8_t MQTT_PingReq(void);

/**
 * @brief 发送DISCONNECT报文，优雅断开MQTT连接
 */
void MQTT_Disconnect(void);

#endif
