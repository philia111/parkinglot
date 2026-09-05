#ifndef OLEDSHOWBOARD_H
#define OLEDSHOWBOARD_H
#include "stm32f4xx.h"
#include <stdbool.h>

// 定义一个结构体来保存我们需要显示的数据
//车位状态
typedef struct {
    uint8_t  is_occupied;      // 0:空闲, 1:占用
    uint8_t  card_uid[4];      // 占用卡UID
} Spot_t;

//全局状态
typedef struct{
    Spot_t Spot[4];
    uint8_t  SpotNum;
    uint8_t EmptySpotNum;
    uint8_t last_uid[4];   // 最近一次刷卡的UID（给UI显示用）
    float Temp;
    float Humi;
    bool WifiState;
    bool MqttState;
    
} ParkingData_t;

void OLED_Update_MainPage(ParkingData_t g_ParkingData);

void OLED_Update_SpotFull(void);

// 入场提示界面
void OLED_Update_EnterPage(const uint8_t *uid, uint8_t slot);

// 出场结算界面
void OLED_Update_ExitPage(const uint8_t *uid, uint8_t hours, uint8_t mins, uint8_t fee);

#endif
