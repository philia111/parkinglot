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
                            SG90_SetAngle(90);
                            Beep(100);
                            xSemaphoreTake(xMutex, portMAX_DELAY);
                            memcpy(g_ParkingData.Spot[i].card_uid, uid, 4);
                            g_ParkingData.Spot[i].is_occupied = 1;
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
                    SG90_SetAngle(90);
                    Beep(100);
                    xSemaphoreTake(xMutex, portMAX_DELAY);
                    g_ParkingData.Spot[matched_spot].is_occupied = 0;
                    memset(g_ParkingData.Spot[matched_spot].card_uid, 0, 4);
                    g_ParkingData.EmptySpotNum++;
                    xSemaphoreGive(xMutex);
                    memcpy(MessageQueue.ID, uid, 4);
                    MessageQueue.SpotNum = matched_spot;
                    xQueueSend(xQueue, &MessageQueue, 0);
                }

                // 6. 设置事件组标志位，通知 UI 刷新
                xEventGroupSetBits(xEventGroup, EVT_RFID_CARD_DONE);
                // 7. 延时等闸门关闭
                // 写pdMS_TO_TICKS(3000)是为了提高移植性，保证延时3秒
                // 如果参数写3000实际延时时间要根据configTICK_RATE_HZ宏来计算
                // 比如100就是100hz一个tick10ms，3000个是30秒
                vTaskDelay(pdMS_TO_TICKS(3000));
                SG90_SetAngle(0); // 关闸
            }
        }
        vTaskDelay(pdMS_TO_TICKS(200)); // 防止重复读卡
    }
}

void UITask(void *pvParameters)
{
    MessageQueue_t evt;
    while (1)
    {
        EventBits_t bits = xEventGroupWaitBits(xEventGroup, EVT_RFID_CARD_DONE, pdTRUE, pdFALSE, 0);
        if (bits & EVT_RFID_CARD_DONE)
        {
            // 判断车位是否已满
            if (g_ParkingData.EmptySpotNum == 0)
            {
                OLED_Update_SpotFull();
            }
            else
            {
                // 没满刷新状态
                if (xQueueReceive(xQueue, &evt, pdMS_TO_TICKS(100)) == pdTRUE)
                {
                    if (evt.IsEntre) {
                        OLED_Update_EnterPage(evt.ID , evt.SpotNum);
                    }else {
                        OLED_Update_ExitPage(evt.ID,1,1,1);
                    } 
                }
               
            }
        }

        //读取dht11温湿度
        



        //刷新led指示车位
        g_ParkingData.Spot[0].is_occupied ? Led1_On() : Led1_Off();
        g_ParkingData.Spot[1].is_occupied ? Led2_On() : Led2_Off();
        g_ParkingData.Spot[2].is_occupied ? Led3_On() : Led3_Off();
        g_ParkingData.Spot[3].is_occupied ? Led4_On() : Led4_Off();


         vTaskDelay(pdMS_TO_TICKS(200)); 
    }
}

void MQTTTask(void *pvParameters)
{
    while (1)
    {
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
    MFRC522_GPIO_Init();
    RTC_Config();
    USART1_Config(115200);
    USART3_Config(115200);
    AT24C02_Config();

    // 2. 初始化网络（耗时较长，依赖串口中断）
    ESP8266_Init();

    // 创建IPC对象
    xQueue = xQueueCreate(5, sizeof(MessageQueue_t));

    xMutex = xSemaphoreCreateMutex();

    xEventGroup = xEventGroupCreate();
    // 3. 创建各个系统工作任务（进入临界区快速创建）
    taskENTER_CRITICAL();

    // 创建 UI 任务
    xTaskCreate(UITask, "UITask", 256, NULL, 1, &UITaskHandle);

    // 创建 RFID 刷卡与道闸任务
    xTaskCreate(RFIDTask, "RFIDTask", 256, NULL, 3, &RFIDTaskHandle);

    // 创建 MQTT 网络通信任务
    xTaskCreate(MQTTTask, "MQTTTask", 512, NULL, 2, &MQTTTaskHandle);

    // 4. 必须先退出临界区！
    taskEXIT_CRITICAL();

    // 5. 最后删除启动任务自身，释放堆栈
    vTaskDelete(NULL);
}
