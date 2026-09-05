#include "app_parking.h"

// 任务句柄定义
TaskHandle_t InitTaskHandle = NULL;
TaskHandle_t RFIDTaskHandle = NULL;
TaskHandle_t UITaskHandle = NULL;
TaskHandle_t MQTTTaskHandle = NULL;

// IPC对象定义
QueueHandle_t xQueue;
SemaphoreHandle_t xMutex;
EventGroupHandle_t xEventGroup;

// 全局共享数据定义
ParkingData_t g_ParkingData;

void DHT11Task(void *pvParameters)
{
    static TickType_t last_dht_tick = 0;
    while (1)
    {
        last_dht_tick = xTaskGetTickCount();
        xSemaphoreTake(xMutex, portMAX_DELAY);
        DHT11_Read_Float(&g_ParkingData.Temp, &g_ParkingData.Humi);
        xSemaphoreGive(xMutex);

        vTaskDelay(pdMS_TO_TICKS(2000));
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
                // ★ 拿到卡号了！接下来你要做：

                // 3. 判断这张卡是"入场"还是"出场"
                //    思路：遍历 g_ParkingData.spot[]，看有没有车位记录了这个UID
                //    如果没有 → 入场；如果有 → 出场
                int matched_spot = -1;
                MessageQueue.IsEntre = 1; // 默认是入场
                for (int i = 0; i < 4; i++)
                {
                    if (memcmp(g_ParkingData.Spot[i].card_uid, uid, 4) == 0)
                    {
                        matched_spot = i;
                        MessageQueue.IsEntre = 0;
                    }
                }
                // 4. 如果是入场：
                //    - 找一个空车位
                //    - 开闸 SG90_SetAngle(90)
                //    - 蜂鸣器响一声
                //    - 更新 g_ParkingData（要加锁！）
                //    - 把事件塞进队列给 MQTT
                if (MessageQueue.IsEntre)
                {
                    for (int i = 0; i < 4; i++)
                    {
                        if (!g_ParkingData.Spot[i].is_occupied)
                        {
                            SG90_SetAngle(180);
                            Beep(100);
                            xSemaphoreTake(xMutex, portMAX_DELAY);
                            memcpy(g_ParkingData.Spot[i].card_uid, uid, 4);
                            g_ParkingData.Spot[i].is_occupied = 1;
                            g_ParkingData.SpotNum = i;
                            memcpy(g_ParkingData.last_uid, uid, 4);
                            g_ParkingData.EmptySpotNum--;
                            xSemaphoreGive(xMutex);
                            memcpy(MessageQueue.ID, uid, 4);
                            MessageQueue.SpotNum = i;
                            xQueueSend(xQueue, &MessageQueue, 0);
                            break;
                        }
                    }
                }
                else
                {
                    // 5. 如果是出场：
                    //    - 开闸
                    //    - 蜂鸣器
                    //    - 释放车位，更新 g_ParkingData
                    //    - 队列通知 MQTT
                    SG90_SetAngle(180);
                    Beep(100);
                    xSemaphoreTake(xMutex, portMAX_DELAY);
                    g_ParkingData.Spot[matched_spot].is_occupied = 0;
                    memset(g_ParkingData.Spot[matched_spot].card_uid, 0, 4);
                    g_ParkingData.SpotNum = matched_spot;
                    memcpy(g_ParkingData.last_uid, uid, 4);
                    g_ParkingData.EmptySpotNum++;
                    xSemaphoreGive(xMutex);
                    memcpy(MessageQueue.ID, uid, 4);
                    MessageQueue.SpotNum = matched_spot;
                    xQueueSend(xQueue, &MessageQueue, 0);
                }

                // 6. 设置事件组标志位，通知 UI 刷新
                if (MessageQueue.IsEntre)
                {
                    xEventGroupSetBits(xEventGroup, EVT_CAR_ENTER);
                }
                else
                {
                    xEventGroupSetBits(xEventGroup, EVT_CAR_EXIT);
                }
                // 7. 延时等闸门关闭
                // 写pdMS_TO_TICKS(3000)是为了提高移植性，保证延时3秒
                // 如果参数写3000实际延时时间要根据configTICK_RATE_HZ宏来计算
                // 比如100就是100hz一个tick10ms，3000个是30秒
                vTaskDelay(pdMS_TO_TICKS(3000));
                SG90_SetAngle(90); // 关闸
            }
        }
        vTaskDelay(pdMS_TO_TICKS(200)); // 防止重复读卡
    }
}

void UITask(void *pvParameters)
{

    while (1)
    {
        // 检测到刷卡事件
        
        EventBits_t bits = xEventGroupWaitBits(xEventGroup, EVT_CAR_ENTER | EVT_CAR_EXIT, pdTRUE, pdFALSE, 0);
        if (bits & EVT_CAR_ENTER)
        {
            // 判断车位是否已满
            if (g_ParkingData.EmptySpotNum == 0)
            {
                OLED_Update_SpotFull();
                vTaskDelay(pdMS_TO_TICKS(1000)); //阻塞一下看清信息
            }
            else
            {
                // 没满刷新状态
                OLED_Update_EnterPage(g_ParkingData.last_uid, g_ParkingData.SpotNum);
            }
        }
        else if (bits & EVT_CAR_EXIT)
        {
            OLED_Update_ExitPage(g_ParkingData.last_uid, 1, 1, 1); // 计费任务没写先替代
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
    DHT11Init();
    HC05_Init();
    Key_Init();
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

    // 创建 DHT11 任务
    xTaskCreate(DHT11Task, "DHT11Task", 128, NULL, 2, NULL);

    // 4. 必须先退出临界区！
    taskEXIT_CRITICAL();

    // 5. 最后删除启动任务自身，释放堆栈
    vTaskDelete(NULL);
}
