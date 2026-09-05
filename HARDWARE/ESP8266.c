#include "ESP8266.h"
#include "USART3.h"
#include "delay.h"
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"

/**
 * @brief ESP8266串口发送指令函数（轮询等待响应版本）
 *
 * 改进点：不再用单次 delay_ms 死等，而是每隔 100ms 检查一次响应，
 *         一旦匹配到期望字符串立即返回，大幅提高响应速度和可靠性。
 *
 * @param cmd       需要发送的指令
 * @param rsp       期待的回复关键词
 * @param timeoutms 最大等待时长（毫秒）
 * @return uint8_t  成功返回1，失败返回0
 */
uint8_t ESP8266_Send_Cmd(char *cmd, const char *rsp, u32 timeoutms)
{
    uint32_t elapsed = 0;

    // 1. 清空串口接收缓冲区
    u3_recvcnt = 0;
    memset(u3_recvbuf, 0, sizeof(u3_recvbuf));

    // 2. 发送指令（加上 \r\n）
    USART3_SendString(cmd);
    USART3_SendString("\r\n");

    // 3. 轮询等待响应（每100ms检查一次）
    while (elapsed < timeoutms)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
        elapsed += 100;

        // 确保缓冲区以'\0'结尾再搜索
        u3_recvbuf[u3_recvcnt] = '\0';

        if (strstr((char *)u3_recvbuf, rsp) != NULL)
        {
            printf("[ESP8266] CMD \"%s\" => OK (%dms)\r\n", cmd, (int)elapsed);
            u3_recvcnt = 0;
            return 1;
        }

        // 如果收到ERROR，提前退出不用等超时
        if (strstr((char *)u3_recvbuf, "ERROR") != NULL)
        {
            printf("[ESP8266] CMD \"%s\" => ERROR (rsp: %s)\r\n", cmd, u3_recvbuf);
            u3_recvcnt = 0;
            return 0;
        }
    }

    // 超时
    printf("[ESP8266] CMD \"%s\" => TIMEOUT %dms (rsp: %s)\r\n",
           cmd, (int)timeoutms, u3_recvbuf);
    u3_recvcnt = 0;
    return 0;
}

/**
 * @brief ESP8266初始化函数
 *
 * 改进点：
 *   1. 先退出可能的透传模式
 *   2. 发送AT+RST硬复位，让模块回到干净状态
 *   3. AT指令握手，最多重试5次
 *   4. 兼容新旧AT固件设置STA模式
 */
void ESP8266_Init(void)
{
    uint8_t i;
    uint8_t ready = 0;

    printf("[ESP8266] Quick AT handshake...\r\n");

    // 1. 快速尝试 AT 握手（避免冷启动无脑复位浪费 3~4 秒）
    for (i = 0; i < 3; i++)
    {
        if (ESP8266_Send_Cmd("AT", "OK", 300))
        {
            ready = 1;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // 2. 如果未响应，尝试退出透传模式并重新测试
    if (!ready)
    {
        USART3_SendString("+++");
        vTaskDelay(pdMS_TO_TICKS(500));
        if (ESP8266_Send_Cmd("AT", "OK", 500))
        {
            ready = 1;
        }
    }

    // 3. 仍未响应则进行软复位
    if (!ready)
    {
        printf("[ESP8266] Module not responding, resetting...\r\n");
        USART3_SendString("AT+RST\r\n");
        vTaskDelay(pdMS_TO_TICKS(2000));
        u3_recvcnt = 0;
        memset(u3_recvbuf, 0, sizeof(u3_recvbuf));
        for (i = 0; i < 5; i++)
        {
            if (ESP8266_Send_Cmd("AT", "OK", 500))
            {
                ready = 1;
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(300));
        }
    }

    // 4. 关闭回显（减少接收数据量）
    ESP8266_Send_Cmd("ATE0", "OK", 500);

    // 5. 设置 STA 模式
    if (!ESP8266_Send_Cmd("AT+CWMODE=1", "OK", 1000))
    {
        ESP8266_Send_Cmd("AT+CWMODE_CUR=1", "OK", 1000);
    }

    printf("[ESP8266] Init done.\r\n");
}

/**
 * @brief ESP8266连接WIFI
 *
 * 改进点：
 *   1. 主动查询 AT+CWJAP?，若已自动连接直接返回（仅需数十毫秒）
 *   2. 检测到 No AP 立即连接，不再死等 3000ms 超时
 *
 * @return uint8_t 成功返回1，失败返回0
 */
uint8_t ESP8266_Connect_WIFI(void)
{
    char cmd_buf[128];
    uint16_t elapsed = 0;

    // 1. 快速检查是否已经连接了WiFi（ESP8266上电会自动连上次保存的AP）
    u3_recvcnt = 0;
    memset(u3_recvbuf, 0, sizeof(u3_recvbuf));
    USART3_SendString("AT+CWJAP?\r\n");

    while (elapsed < 1500)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
        elapsed += 100;
        u3_recvbuf[u3_recvcnt] = '\0';

        if (strstr((char *)u3_recvbuf, "+CWJAP:") != NULL)
        {
            printf("[ESP8266] Already connected to WiFi!\r\n");
            return 1;
        }
        if (strstr((char *)u3_recvbuf, "No AP") != NULL)
        {
            // 明确没有连接，跳出查询直接开始连接
            break;
        }
    }

    // 2. 尝试发起连接
    printf("[ESP8266] Connecting to WiFi: %s ...\r\n", WIFI_SSID);
    snprintf(cmd_buf, sizeof(cmd_buf), "AT+CWJAP=\"%s\",\"%s\"", WIFI_SSID, WIFI_PSWD);

    if (ESP8266_Send_Cmd(cmd_buf, "OK", 15000))
    {
        return 1;
    }

    // 备用指令
    snprintf(cmd_buf, sizeof(cmd_buf), "AT+CWJAP_DEF=\"%s\",\"%s\"", WIFI_SSID, WIFI_PSWD);
    if (ESP8266_Send_Cmd(cmd_buf, "OK", 15000))
    {
        return 1;
    }

    return 0;
}

/**
 * @brief ESP8266重置函数
 *
 */
void ESP8266_Reset(void)
{
    ESP8266_Send_Cmd("AT+RST", "ready", 3000);
}

/**
 * @brief ESP8266退出透传模式
 *
 * @return uint8_t
 */
void ESP8266_Exit_Transparent_Transmission(void)
{
     vTaskDelay(pdMS_TO_TICKS(1000));
    USART3_SendString("+++");
     vTaskDelay(pdMS_TO_TICKS(1000));
    // 退出透传模式，发送下一条AT指令要间隔1秒
     vTaskDelay(pdMS_TO_TICKS(1000));
}

/**
 * @brief ESP8266进入透传模式并进入发送状态
 *
 * @return uint8_t 是否成功。成功返回1失败返回0
 */
uint8_t ESP8266_Entry_Transparent_Transmission(void)
{
    // 进入透传模式
    if (ESP8266_Send_Cmd("AT+CIPMODE=1", "OK", 5000) != 1)
    {
        return 0;
    }

    // 开启发送状态
    if (ESP8266_Send_Cmd("AT+CIPSEND", ">", 5000) != 1)
    {
        return 0;
    }
    return 1;
}

/**
 * @brief TCP服务器连接
 *
 * @param ip 服务器IP地址
 * @param port 服务器端口
 * @return int8_t 是否连接成功。成功返回1失败返回0
 */
int8_t ESP8266_Connect_Server(char *ip, uint16_t port)
{
    char buf[128] = {0};

    // 组装 AT+CIPSTART="TCP","IP",PORT 指令
    sprintf(buf, "AT+CIPSTART=\"TCP\",\"%s\",%d", ip, port);

    if (ESP8266_Send_Cmd(buf, "OK", 10000))
    {
        return 1;
    }
    return 0;
}

/**
 * @brief TCP服务器断开连接
 *
 * @return int8_t
 */
int8_t ESP8266_Disconnect_Server(void)
{
    if (ESP8266_Send_Cmd("AT+CIPCLOSE", "OK", 6000))
    {
        return 1;
    }
    return 0;
}

/**
 * @brief 设置连接模式 (0:单连接, 1:多连接)
 */
uint8_t ESP8266_Set_CIPMUX(uint8_t mode)
{
    char cmd[32];
    sprintf(cmd, "AT+CIPMUX=%d", mode);
    return ESP8266_Send_Cmd(cmd, "OK", 1000);
}

/**
 * @brief 从网络获取当前时间（通过 api.k780.com HTTP 接口）
 *
 * @param datetime_buf 用于存储提取的时间字符串（格式：YYYY-MM-DD HH:MM:SS，长度至少20字节）
 * @return uint8_t 成功返回 1，失败返回 0
 */
uint8_t ESP8266_Get_Network_Time(char *datetime_buf)
{
    uint8_t res = 0;
    
    // 1. 取消回显
    ESP8266_Send_Cmd("ATE0", "OK", 500);
    
    // 2. 设置为单路连接模式
    if (!ESP8266_Set_CIPMUX(0))
    {
        return 0;
    }
    
    // 3. 连接到时间 API 服务器（标准 HTTP 80 端口）
    if (!ESP8266_Connect_Server("api.k780.com", 80))
    {
        return 0;
    }
    
    // 4. 进入透传模式并准备发送
    if (!ESP8266_Entry_Transparent_Transmission())
    {
        ESP8266_Send_Cmd("AT+CIPCLOSE", "OK", 500);
        return 0;
    }
    
    // 5. 清空串口接收缓冲区，准备接收 API 响应
    u3_recvcnt = 0;
    memset(u3_recvbuf, 0, sizeof(u3_recvbuf));
    
    // 6. 发送 HTTP GET 请求获取时间
    USART3_SendString("GET http://api.k780.com/?app=life.time&appkey=10003&sign=b59bc3ef6191eb9f747dd4e83c99f2a4&format=json HTTP/1.1\r\nHost: api.k780.com\r\nConnection: close\r\n\r\n");
    
    // 7. 等待接收响应（最多等待 3 秒）
    uint16_t delay_count = 0;
    char *time_ptr = NULL;
    while (delay_count < 3000)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
        delay_count += 100;
        
        // 确保缓冲区以'\0'结尾
        u3_recvbuf[u3_recvcnt] = '\0';
        
        // 查找返回 JSON 中的 datetime_1 字段
        time_ptr = strstr((char *)u3_recvbuf, "\"datetime_1\":\"");
        if (time_ptr != NULL)
        {
            break;
        }
    }
    
    // 8. 解析提取时间字符串
    if (time_ptr != NULL)
    {
        time_ptr += 14; // 跳过 "\"datetime_1\":\"" 的长度
        strncpy(datetime_buf, time_ptr, 19);
        datetime_buf[19] = '\0';
        res = 1;
    }
    else
    {
        printf("[ESP8266] Get network time failed or timeout!\r\n");
    }
    
    // 9. 快速退出透传并关闭 TCP 连接（无需等待3秒）
    USART3_SendString("+++");
    vTaskDelay(pdMS_TO_TICKS(500));
    ESP8266_Send_Cmd("AT+CIPMODE=0", "OK", 500);
    ESP8266_Send_Cmd("AT+CIPCLOSE", "OK", 500);
    
    return res;
}
