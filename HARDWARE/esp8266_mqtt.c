#include "esp8266_mqtt.h"
#include "ESP8266.h"
#include "USART3.h"
#include "delay.h"
#include <stdio.h>
#include <string.h>

/*
 * ===================================================================
 *  本文件实现 MQTT v3.1.1 协议的报文手动封装
 *
 *  MQTT报文结构统一格式：
 *  ┌──────────┬──────────────┬──────────┬──────────┐
 *  │ 固定头    │ 剩余长度      │ 可变头   │ 载荷     │
 *  │ (1字节)  │ (1~4字节)     │ (可选)   │ (可选)   │
 *  └──────────┴──────────────┴──────────┴──────────┘
 *
 *  固定头第一字节: 高4位=报文类型，低4位=标志位
 *  报文类型: CONNECT=1, CONNACK=2, PUBLISH=3, SUBSCRIBE=8,
 *           PINGREQ=12, PINGRESP=13, DISCONNECT=14
 * ===================================================================
 */

/* ==================== 内部变量 ==================== */
static uint8_t  mqtt_txbuf[256];    /* MQTT发送缓冲区 */
static uint16_t mqtt_packet_id = 1; /* 报文标识符(自增)，SUBSCRIBE用 */


/* ==================== 内部辅助函数 ==================== */

/**
 * @brief 编码MQTT"剩余长度"字段（变长编码）
 *
 * MQTT协议规定：
 *   每个字节的低7位存数据，最高位(bit7)为1表示后面还有字节
 *   所以: 0~127 用1字节，128~16383 用2字节，以此类推
 *
 * 举例：
 *   长度 = 30   → 编码为 [0x1E]           (1字节)
 *   长度 = 200  → 编码为 [0xC8, 0x01]     (2字节)
 *
 * @param buf    输出缓冲区
 * @param length 要编码的长度值
 * @return 编码用了几个字节
 */
static uint8_t encode_remaining_length(uint8_t *buf, uint32_t length)
{
    uint8_t i = 0;
    do {
        uint8_t byte = length % 128;
        length /= 128;
        if (length > 0)
            byte |= 0x80;  /* 最高位置1，告诉接收方"后面还有字节" */
        buf[i++] = byte;
    } while (length > 0);
    return i;
}

/**
 * @brief 发送MQTT报文并等待Broker响应
 *
 * @param data          要发送的报文数据
 * @param len           报文长度
 * @param expected_type 期望的响应报文类型（第一个字节）
 * @param timeout_ms    等待超时时间
 * @return 1=收到期望响应  0=超时或响应不对
 */
static uint8_t mqtt_send_and_wait(const uint8_t *data, uint16_t len,
                                   uint8_t expected_type, uint32_t timeout_ms)
{
    /* 1. 清空接收缓冲区 */
    u3_recvcnt = 0;
    memset(u3_recvbuf, 0, sizeof(u3_recvbuf));

    /* 2. 通过USART3发送MQTT报文（透传模式下直接到TCP） */
    USART3_SendBytes(data, len);

    /* 3. 等待Broker响应 */
    delay_ms(timeout_ms);

    /* 4. 检查响应 */
    if (u3_recvcnt >= 2 && u3_recvbuf[0] == expected_type)
    {
        return 1;
    }

    printf("[MQTT] Wait response: expected 0x%02X, got 0x%02X (cnt=%d)\r\n",
           expected_type, u3_recvbuf[0], (int)u3_recvcnt);
    return 0;
}


/* ==================== MQTT 核心函数 ==================== */

/**
 * @brief 发送 MQTT CONNECT 报文
 *
 * 报文结构详解：
 *
 *  字节序号     内容              说明
 *  ─────────────────────────────────────────────
 *  [0]        0x10              固定头: CONNECT类型(0001) + 标志(0000)
 *  [1]        剩余长度           后面所有字节的总数
 *  ─── 可变头 (固定10字节) ─────
 *  [2-3]      0x00, 0x04        协议名长度 = 4
 *  [4-7]      'M','Q','T','T'   协议名
 *  [8]        0x04              协议级别 = MQTT v3.1.1
 *  [9]        0x02              连接标志: Clean Session=1, 其余=0
 *  [10-11]    Keep Alive        心跳间隔(秒), 高字节在前
 *  ─── 载荷 ─────
 *  [12-13]    ClientID长度       高字节在前
 *  [14-...]   ClientID字符串     你的设备名称
 */
uint8_t MQTT_Connect(const char *clientId, uint16_t keepAlive)
{
    uint16_t clientId_len = strlen(clientId);
    uint16_t idx = 0;

    /* 计算剩余长度：可变头(10字节) + ClientID长度前缀(2字节) + ClientID */
    uint32_t remaining_len = 10 + 2 + clientId_len;

    /* ===== 固定头 ===== */
    mqtt_txbuf[idx++] = 0x10;   /* CONNECT报文类型 = 0001 0000 */
    idx += encode_remaining_length(&mqtt_txbuf[idx], remaining_len);

    /* ===== 可变头 ===== */
    /* 协议名 "MQTT" */
    mqtt_txbuf[idx++] = 0x00;   /* 协议名长度高字节 */
    mqtt_txbuf[idx++] = 0x04;   /* 协议名长度低字节 = 4 */
    mqtt_txbuf[idx++] = 'M';
    mqtt_txbuf[idx++] = 'Q';
    mqtt_txbuf[idx++] = 'T';
    mqtt_txbuf[idx++] = 'T';

    /* 协议级别 */
    mqtt_txbuf[idx++] = 0x04;   /* 0x04 = MQTT v3.1.1 */

    /* 连接标志 */
    /*
     * bit7: Username Flag  = 0 (不用用户名)
     * bit6: Password Flag  = 0 (不用密码)
     * bit5: Will Retain    = 0
     * bit4-3: Will QoS     = 00
     * bit2: Will Flag      = 0
     * bit1: Clean Session  = 1 (每次连接清除旧会话)
     * bit0: Reserved       = 0
     * 合起来 = 0000 0010 = 0x02
     */
    mqtt_txbuf[idx++] = 0x02;

    /* 心跳间隔 (Keep Alive), 大端序 */
    mqtt_txbuf[idx++] = (keepAlive >> 8) & 0xFF;   /* 高字节 */
    mqtt_txbuf[idx++] = keepAlive & 0xFF;           /* 低字节 */

    /* ===== 载荷 ===== */
    /* Client ID (MQTT要求第一个载荷就是ClientID) */
    mqtt_txbuf[idx++] = (clientId_len >> 8) & 0xFF; /* 长度高字节 */
    mqtt_txbuf[idx++] = clientId_len & 0xFF;         /* 长度低字节 */
    memcpy(&mqtt_txbuf[idx], clientId, clientId_len);
    idx += clientId_len;

    /* ===== 发送并等待CONNACK ===== */
    /*
     * CONNACK响应格式：
     *   [0] = 0x20  (CONNACK类型)
     *   [1] = 0x02  (剩余长度=2)
     *   [2] = 0x00  (Session Present标志)
     *   [3] = 返回码  0x00=接受, 其他=拒绝
     */
    printf("[MQTT] CONNECT (clientId: %s, keepAlive: %ds)...\r\n",
           clientId, keepAlive);

    if (mqtt_send_and_wait(mqtt_txbuf, idx, 0x20, 3000))
    {
        if (u3_recvbuf[3] == 0x00)
        {
            printf("[MQTT] CONNECT OK! Broker accepted.\r\n");
            return 1;
        }
        else
        {
            printf("[MQTT] CONNECT rejected! code=%d\r\n", u3_recvbuf[3]);
            return 0;
        }
    }

    printf("[MQTT] CONNECT failed! No CONNACK.\r\n");
    return 0;
}


/**
 * @brief 发送 MQTT SUBSCRIBE 报文
 *
 * 报文结构：
 *  [0]        0x82              固定头: SUBSCRIBE类型 + 固定标志0x02
 *  [1]        剩余长度
 *  [2-3]      报文标识符         Broker回复SUBACK时会带同样的ID
 *  [4-5]      Topic长度
 *  [6-...]    Topic字符串
 *  [最后1字节] QoS               请求的服务质量等级
 */
uint8_t MQTT_Subscribe(const char *topic, uint8_t qos)
{
    uint16_t topic_len = strlen(topic);
    uint16_t idx = 0;

    /* 剩余长度 = 报文ID(2) + Topic长度前缀(2) + Topic + QoS(1) */
    uint32_t remaining_len = 2 + 2 + topic_len + 1;

    /* ===== 固定头 ===== */
    mqtt_txbuf[idx++] = 0x82;   /* SUBSCRIBE = 1000 0010 */
    idx += encode_remaining_length(&mqtt_txbuf[idx], remaining_len);

    /* ===== 可变头 ===== */
    /* 报文标识符 (每次递增，Broker用它匹配请求和响应) */
    mqtt_txbuf[idx++] = (mqtt_packet_id >> 8) & 0xFF;
    mqtt_txbuf[idx++] = mqtt_packet_id & 0xFF;
    mqtt_packet_id++;

    /* ===== 载荷 ===== */
    mqtt_txbuf[idx++] = (topic_len >> 8) & 0xFF;
    mqtt_txbuf[idx++] = topic_len & 0xFF;
    memcpy(&mqtt_txbuf[idx], topic, topic_len);
    idx += topic_len;
    mqtt_txbuf[idx++] = qos;

    /* ===== 发送并等待SUBACK (0x90) ===== */
    printf("[MQTT] SUBSCRIBE \"%s\" (QoS %d)...\r\n", topic, qos);

    if (mqtt_send_and_wait(mqtt_txbuf, idx, 0x90, 2000))
    {
        printf("[MQTT] SUBSCRIBE OK!\r\n");
        return 1;
    }

    printf("[MQTT] SUBSCRIBE failed!\r\n");
    return 0;
}


/**
 * @brief 发送 MQTT PUBLISH 报文 (QoS 0)
 *
 * QoS 0 = "最多发一次"，发完不等回复，最简单最常用
 *
 * 报文结构：
 *  [0]        0x30              固定头: PUBLISH类型 + QoS0 + 不保留
 *  [1]        剩余长度
 *  [2-3]      Topic长度
 *  [4-...]    Topic字符串
 *  [...]      消息内容(JSON等)   没有长度前缀，到报文末尾就是消息
 */
uint8_t MQTT_Publish(const char *topic, const char *message)
{
    uint16_t topic_len = strlen(topic);
    uint16_t msg_len = strlen(message);
    uint16_t idx = 0;

    /* 剩余长度 = Topic长度前缀(2) + Topic + 消息 */
    uint32_t remaining_len = 2 + topic_len + msg_len;

    /* 检查缓冲区是否够用 */
    if (remaining_len + 5 > sizeof(mqtt_txbuf))
    {
        printf("[MQTT] PUBLISH error: message too long! (%d bytes)\r\n",
               (int)remaining_len);
        return 0;
    }

    /* ===== 固定头 ===== */
    mqtt_txbuf[idx++] = 0x30;   /* PUBLISH = 0011 0000 (QoS0, 不保留) */
    idx += encode_remaining_length(&mqtt_txbuf[idx], remaining_len);

    /* ===== 可变头 ===== */
    mqtt_txbuf[idx++] = (topic_len >> 8) & 0xFF;
    mqtt_txbuf[idx++] = topic_len & 0xFF;
    memcpy(&mqtt_txbuf[idx], topic, topic_len);
    idx += topic_len;
    /* 注意：QoS 0 没有报文标识符 */

    /* ===== 载荷 ===== */
    memcpy(&mqtt_txbuf[idx], message, msg_len);
    idx += msg_len;

    /* ===== 发送 (QoS 0 不等回复) ===== */
    USART3_SendBytes(mqtt_txbuf, idx);

    printf("[MQTT] PUBLISH \"%s\" => %s\r\n", topic, message);
    return 1;
}


/**
 * @brief 发送 MQTT PINGREQ 心跳包
 *
 * 最简单的MQTT报文，只有2个字节！
 * 作用：在 Keep Alive 时间内告诉 Broker "我还活着"
 * Broker 会回复 PINGRESP (0xD0, 0x00)
 */
uint8_t MQTT_PingReq(void)
{
    uint8_t ping[2] = {0xC0, 0x00};

    if (mqtt_send_and_wait(ping, 2, 0xD0, 2000))
    {
        printf("[MQTT] PING OK\r\n");
        return 1;
    }

    printf("[MQTT] PING failed! Broker may be disconnected.\r\n");
    return 0;
}


/**
 * @brief 发送 MQTT DISCONNECT 报文
 *
 * 优雅断开，也是2字节
 */
void MQTT_Disconnect(void)
{
    uint8_t disc[2] = {0xE0, 0x00};
    USART3_SendBytes(disc, 2);
    printf("[MQTT] DISCONNECT sent.\r\n");
}


/* ==================== 高层封装 ==================== */

/**
 * @brief 一键启动MQTT连接
 *
 * 调用前确保WiFi已连接。
 * 内部流程：
 *   1. TCP连接到MQTT Broker的1883端口
 *   2. 进入ESP8266透传模式（之后发什么数据就直接到TCP）
 *   3. 发送MQTT CONNECT报文，建立MQTT会话
 *
 * @return 1成功 0失败
 */
uint8_t MQTT_Start(void)
{
    /* 第1步：TCP连接到Broker */
    printf("[MQTT] TCP => %s:%d ...\r\n", MQTT_BROKER_IP, MQTT_BROKER_PORT);

    if (!ESP8266_Connect_Server((char *)MQTT_BROKER_IP, MQTT_BROKER_PORT))
    {
        printf("[MQTT] TCP connect failed!\r\n");
        return 0;
    }
    printf("[MQTT] TCP connected.\r\n");

    /* 第2步：进入透传模式 */
    /*
     * 透传模式 = 透明传输模式
     * 进入后，USART3发出的任何数据都直接通过TCP发给服务器
     * 服务器发回的数据也直接通过USART3收到
     * 不再需要AT指令，就像STM32直接连着Broker一样
     */
    if (!ESP8266_Entry_Transparent_Transmission())
    {
        printf("[MQTT] Enter transparent mode failed!\r\n");
        return 0;
    }
    printf("[MQTT] Transparent mode OK.\r\n");

    /* 短暂等待，确保透传模式稳定 */
    delay_ms(500);

    /* 第3步：发送MQTT CONNECT报文 */
    if (!MQTT_Connect(MQTT_CLIENT_ID, MQTT_KEEP_ALIVE))
    {
        printf("[MQTT] MQTT CONNECT failed!\r\n");
        return 0;
    }

    return 1;  /* 全部成功！ */
}
