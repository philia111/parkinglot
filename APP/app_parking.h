#ifndef _APP_PARKING_H_
#define _APP_PARKING_H_


#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"
#include "event_groups.h"

#include "OledShowBoard.h"


//事件标志位
#define EVT_CAR_ENTER   (1 << 0)  
#define EVT_CAR_EXIT    (1 << 1)
#define EVT_SPOT_FULL   (1 << 2)



//消息队列传递数据结构体
typedef struct{
    uint8_t ID[4];
    bool IsEntre;
    uint8_t SpotNum;
    uint8_t IsFull;

}MessageQueue_t ;

//任务句柄
extern TaskHandle_t InitTaskHandle;
extern TaskHandle_t RFIDTaskHandle;
extern TaskHandle_t UITaskHandle;
extern TaskHandle_t MQTTTaskHandle;
extern TaskHandle_t DHT11TaskHandle;

//IPC对象声明
extern QueueHandle_t xQueue;
extern SemaphoreHandle_t xMutex;
extern EventGroupHandle_t xEventGroup;

//全局共享数据声明
extern ParkingData_t g_ParkingData;   

//任务声明
void InitTask(void *pvParameters);
void RFIDTask(void *pvParameters);
void UITask(void *pvParameters);
void MQTTTask(void *pvParameters);
void DHT11Task(void *pvParameters);


#endif