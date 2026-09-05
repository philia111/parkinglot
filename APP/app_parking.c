#include "app_parking.h"
#include "AT24C02.h"
#include "ESP8266.h"
#include "HC05.h"
#include "MFRC522.h"
#include "USART1.h"
#include "USART3.h"
#include "beep.h"
#include "dht11.h"
#include "esp8266_mqtt.h"
#include "key.h"
#include "led.h"
#include "oled.h"
#include "rtc.h"
#include "sg90.h"
#include <stdio.h>
#include <string.h>

// 任务句柄定义
TaskHandle_t InitTaskHandle = NULL;
TaskHandle_t RFIDTaskHandle = NULL;
TaskHandle_t UITaskHandle = NULL;
TaskHandle_t MQTTTaskHandle = NULL;
TaskHandle_t DHT11TaskHandle = NULL;

// IPC对象定义
QueueHandle_t xQueue;
SemaphoreHandle_t xMutex;
EventGroupHandle_t xEventGroup;

// 全局共享数据定义
ParkingData_t g_ParkingData;

void DHT11Task(void *pvParameters)
{
    float temp = 0.0f;
    float humi = 0.0f;
    while (1)
    {
        // 1. 先进行单总线数据采集（不霸占互斥锁）
        if (DHT11_Read_Float(&temp, &humi))
        {
            // 2. 仅在更新全局结构体时极短加锁（微秒级）
            xSemaphoreTake(xMutex, portMAX_DELAY);
            g_ParkingData.Temp = temp;
            g_ParkingData.Humi = humi;
            xSemaphoreGive(xMutex);
        }

        // 3. 每 3 秒采集一次温湿度（DHT11 要求间隔 >= 1~2 秒）
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

void RFIDTask(void *pvParameters)
{
    uint8_t status;
    uint8_t uid[4];
    MessageQueue_t MessageQueue;
    g_ParkingData.EmptySpotNum = 4; // 初始4个空车位
    while (1)
    {
        // 1. 寻卡
        status = MFRC522_Request(PICC_REQIDL, uid);
        if (status == MI_OK)
        {
            printf(" get card !\n");
            // 2. 防冲撞，获取完整 UID
            status = MFRC522_Anticoll(uid);
            if (status == MI_OK)
            {
                // 3. 判断这张卡是"入场"还是"出场"
                int matched_spot = -1;
                for (int i = 0; i < 4; i++)
                {
                    if (g_ParkingData.Spot[i].is_occupied &&
                        memcmp(g_ParkingData.Spot[i].card_uid, uid, 4) == 0)
                    {
                        matched_spot = i;
                        break;
                    }
                }

                if (matched_spot != -1)
                {
                    // 4. 出场流程：

                    // 车费计算
                    TickType_t parked_ticks = xTaskGetTickCount() - g_ParkingData.Spot[matched_spot].enter_tick;
                    uint32_t parked_seconds = parked_ticks / configTICK_RATE_HZ;
                    uint8_t fee = (parked_seconds + 2) / 3; // +2 是为了向上取整
                    if (fee == 0)
                        fee = 1; // 最低1元
                    // 为了快速演示，一小时设定为30秒，一分钟为5秒
                    uint8_t hours = parked_seconds / 30;
                    uint8_t mins = (parked_seconds % 30) / 5;

                    SG90_SetAngle(180);
                    Beep(100);
                    xSemaphoreTake(xMutex, portMAX_DELAY);
                    g_ParkingData.fee = fee;
                    g_ParkingData.hours = hours;
                    g_ParkingData.mins = mins;

                    g_ParkingData.Spot[matched_spot].is_occupied = 0;
                    memset(g_ParkingData.Spot[matched_spot].card_uid, 0, 4);
                    g_ParkingData.SpotNum = matched_spot;
                    memcpy(g_ParkingData.last_uid, uid, 4);
                    g_ParkingData.EmptySpotNum++;
                    xSemaphoreGive(xMutex);

                    MessageQueue.IsEntre = 0;
                    memcpy(MessageQueue.ID, uid, 4);
                    MessageQueue.SpotNum = matched_spot;
                    xQueueSend(xQueue, &MessageQueue, 0);

                    // 通知 UI 刷新出场界面
                    xEventGroupSetBits(xEventGroup, EVT_CAR_EXIT);

                    // 延时等闸门关闭
                    vTaskDelay(pdMS_TO_TICKS(3000));
                    SG90_SetAngle(90); // 关闸
                }
                else
                {
                    // 5. 入场流程：
                    // 首先判断车位是否已满
                    if (g_ParkingData.EmptySpotNum == 0)
                    {
                        // 车位已满：不开闸，长鸣警示，通知 UI 显示车位已满
                        Beep(300);
                        xEventGroupSetBits(xEventGroup, EVT_SPOT_FULL);
                    }
                    else
                    {
                        // 寻找空车位
                        int allocated_spot = -1;
                        for (int i = 0; i < 4; i++)
                        {
                            if (!g_ParkingData.Spot[i].is_occupied)
                            {
                                allocated_spot = i;
                                break;
                            }
                        }

                        if (allocated_spot != -1)
                        {
                            SG90_SetAngle(180);
                            Beep(100);
                            // 修改全局变量加锁保证数据安全
                            xSemaphoreTake(xMutex, portMAX_DELAY);
                            // 将当前UID写入数据结构体里
                            memcpy(g_ParkingData.Spot[allocated_spot].card_uid, uid, 4);
                            // 更新占用情况车位信息
                            g_ParkingData.Spot[allocated_spot].is_occupied = 1;
                            g_ParkingData.SpotNum = allocated_spot;

                            memcpy(g_ParkingData.last_uid, uid, 4);
                            // 记录入场时间方便计费
                            g_ParkingData.Spot[allocated_spot].enter_tick = xTaskGetTickCount();
                            // 减少车位
                            g_ParkingData.EmptySpotNum--;
                            // 解锁
                            xSemaphoreGive(xMutex);

                            MessageQueue.IsEntre = 1;
                            memcpy(MessageQueue.ID, uid, 4);
                            MessageQueue.SpotNum = allocated_spot;
                            xQueueSend(xQueue, &MessageQueue, 0);

                            // 通知 UI 刷新入场界面
                            xEventGroupSetBits(xEventGroup, EVT_CAR_ENTER);

                            // 延时等闸门关闭
                            vTaskDelay(pdMS_TO_TICKS(3000));
                            SG90_SetAngle(90); // 关闸
                        }
                    }
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(200)); // 防止重复读卡
    }
}

void UITask(void *pvParameters)
{
    while (1)
    {
        // 检测到刷卡事件或车位已满事件
        EventBits_t bits = xEventGroupWaitBits(xEventGroup,
                                               EVT_CAR_ENTER | EVT_CAR_EXIT | EVT_SPOT_FULL,
                                               pdTRUE, pdFALSE, 0);
        if (bits & EVT_SPOT_FULL)
        {
            OLED_Update_SpotFull();
            vTaskDelay(pdMS_TO_TICKS(1500)); // 阻塞一下看清车满提示
        }
        else if (bits & EVT_CAR_ENTER)
        {
            // 只要收到入场事件，说明成功分配了车位（哪怕是最后一个车位），正常显示入场欢迎页
            OLED_Update_EnterPage(g_ParkingData.last_uid, g_ParkingData.SpotNum);
        }
        else if (bits & EVT_CAR_EXIT)
        {
            OLED_Update_ExitPage(g_ParkingData.last_uid, g_ParkingData.hours, g_ParkingData.mins, g_ParkingData.fee); // 计费任务没写先替代
        }

        // 默认刷新主界面
        OLED_Update_MainPage(g_ParkingData);

        // 刷新led指示车位
        g_ParkingData.Spot[0].is_occupied ? Led1_On() : Led1_Off();
        g_ParkingData.Spot[1].is_occupied ? Led2_On() : Led2_Off();
        g_ParkingData.Spot[2].is_occupied ? Led3_On() : Led3_Off();
        g_ParkingData.Spot[3].is_occupied ? Led4_On() : Led4_Off();

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void MQTTTask(void *pvParameters)
{
    MessageQueue_t evt;
    char json_buf[128];
    char timebuf[20];
    TickType_t last_ping = xTaskGetTickCount();
    TickType_t last_report = xTaskGetTickCount();

    g_ParkingData.WifiState = 0;
    g_ParkingData.MqttState = 0;
    uint8_t Subscribe = 0;
    // 网络初始化在后台运行，不阻塞系统前台启动
    ESP8266_Init();

    while (1)
    {
        if (!g_ParkingData.WifiState || !g_ParkingData.MqttState)
        {
            g_ParkingData.WifiState = ESP8266_Connect_WIFI();
            if (g_ParkingData.WifiState)
            {
                // 1. WiFi连接成功且处于AT模式时授时（仅需同步一次）
                static uint8_t s_time_synced = 0;
                if (!s_time_synced)
                {
                    printf("[RTC] Syncing network time...\r\n");
                    if (ESP8266_Get_Network_Time(timebuf))
                    {
                        RTC_Set_Calendar_String(timebuf);
                        s_time_synced = 1;
                        printf("[RTC] Time sync success: %s\r\n", timebuf);
                    }
                    else
                    {
                        printf("[RTC] Time sync failed!\r\n");
                    }
                }

                // 2. 授时完成后，建立MQTT连接并进入透传
                g_ParkingData.MqttState = MQTT_Start();
            }
            else
            {
                g_ParkingData.MqttState = 0;
                vTaskDelay(pdMS_TO_TICKS(5000)); // 连接失败等5秒再重试
                continue;                        // 跳过后面的业务逻辑
            }
        }

        if (g_ParkingData.MqttState)
        {
            if (!Subscribe)
            {
                MQTT_Subscribe(TOPIC_PARKING_ENTER, 1);
                MQTT_Subscribe(TOPIC_PARKING_EXIT, 1);
                MQTT_Subscribe(TOPIC_PARKING_STATUS, 1);
                MQTT_Subscribe(TOPIC_PARKING_CMD, 0);
                Subscribe = 1;

                // 订阅完成后，清掉 SUBACK 报文的残留数据
                u3_recvcnt = 0;
                memset(u3_recvbuf, 0, sizeof(u3_recvbuf));
            }

            // 1. 检查队列有没有刷卡事件（等 100ms）
            if (xQueueReceive(xQueue, &evt, pdMS_TO_TICKS(100)) == pdTRUE)
            {
                // 有事件！组装 JSON 上报
                // 提示：用 sprintf 拼一个简单 JSON 字符串
                // 例如: {"uid":"AABBCCDD","enter":1,"spot":2}
                // 然后 MQTT_Publish(TOPIC_PARKING_ENTER 或 EXIT, json_buf);
                if (evt.IsEntre)
                {
                    sprintf(json_buf, " \"uid\":\"%02X%02X%02X%02X\" , \"enter\":%d , \"spot\":%d ", evt.ID[0], evt.ID[1], evt.ID[2], evt.ID[3], evt.IsEntre, evt.SpotNum);
                    MQTT_Publish(TOPIC_PARKING_ENTER, json_buf);
                    memset(json_buf, 0, sizeof(json_buf));
                }
                else
                {
                    // 出场要拼接计费信息
                    sprintf(json_buf, " \"uid\":\"%02X%02X%02X%02X\" , \"enter\":%d , \"spot\":%d , \"fee\":%d ", evt.ID[0], evt.ID[1], evt.ID[2], evt.ID[3], evt.IsEntre, evt.SpotNum, g_ParkingData.fee);
                    MQTT_Publish(TOPIC_PARKING_EXIT, json_buf);
                    memset(json_buf, 0, sizeof(json_buf));
                }
            }
            // 2. 心跳（每 50 秒）
            if ((xTaskGetTickCount() - last_ping) >= pdMS_TO_TICKS(50000))
            {
                last_ping = xTaskGetTickCount();
                MQTT_PingReq();
            }
            // 3. 定时上报车位状态（每 30 秒）
            if ((xTaskGetTickCount() - last_report) >= pdMS_TO_TICKS(30000))
            {
                last_report = xTaskGetTickCount();
                // 读 g_ParkingData，组装状态 JSON，Publish
                sprintf(json_buf, "\"temp\":%d.%d,\"humi\":%d.%d,\"empty\":%d",
                        (int)g_ParkingData.Temp, ((int)(g_ParkingData.Temp * 10)) % 10,
                        (int)g_ParkingData.Humi, ((int)(g_ParkingData.Humi * 10)) % 10,
                        g_ParkingData.EmptySpotNum);

                MQTT_Publish(TOPIC_PARKING_STATUS, json_buf);
                memset(json_buf, 0, sizeof(json_buf));
            }
        }

        // 4. 检查是否收到云端指令
        char topic[32], payload[64];
        if (MQTT_CheckMessage(topic, sizeof(topic), payload, sizeof(payload)))
        {
            printf("[MQTT] Recv: topic=%s payload=%s\r\n", topic, payload);

            // 根据 payload 内容执行动作
            if (strcmp(topic, TOPIC_PARKING_CMD) == 0)
            {
                if (strstr(payload, "open"))
                {
                    // 远程开闸
                    SG90_SetAngle(180);
                    vTaskDelay(pdMS_TO_TICKS(3000));
                    SG90_SetAngle(90);
                }
                // 可以继续加其他指令...
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/**
 * @brief 全局初始化任务
 *
 * @param pvParameters
 */
void InitTask(void *pvParameters)
{
    // 1. 初始化基础外设（硬件配置，此时已允许中断正常接收）
    SG90_Init();
    OLED_Init();
    Beep_Init();
    DHT11_Init();
    // HC05_Init();
    // Key_Init();
    LED_Init();
    MFRC522_Init();
    RTC_Config();
    USART1_Config(115200);
    USART3_Config(115200);
    AT24C02_Config();

    // 2. 创建IPC对象（不再在此阻塞等待网络初始化，让系统瞬时启动）
    xQueue = xQueueCreate(5, sizeof(MessageQueue_t));

    xMutex = xSemaphoreCreateMutex();

    xEventGroup = xEventGroupCreate();
    // 3. 创建各个系统工作任务（进入临界区快速创建）
    taskENTER_CRITICAL();

    // 创建 UI 任务
    xTaskCreate(UITask, "UITask", 256, NULL, 4, &UITaskHandle);

    // 创建 RFID 刷卡与道闸任务
    xTaskCreate(RFIDTask, "RFIDTask", 256, NULL, 1, &RFIDTaskHandle);

    // 创建 MQTT 网络通信任务
    xTaskCreate(MQTTTask, "MQTTTask", 512, NULL, 3, &MQTTTaskHandle);

    // 创建 DHT11 温湿度独立任务（栈大小 256 字 = 1024 字节，充足安全）
    xTaskCreate(DHT11Task, "DHT11Task", 256, NULL, 2, &DHT11TaskHandle);

    // 按键任务

    // 4. 必须先退出临界区！
    taskEXIT_CRITICAL();

    // 5. 最后删除启动任务自身，释放堆栈
    vTaskDelete(NULL);
}
