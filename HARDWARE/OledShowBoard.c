#include <stdio.h>
#include "oled.h"
#include "OledShowBoard.h"
#include "FreeRTOS.h"
#include "task.h"
#include "rtc.h"

/**
 * @brief 刷新OLED主界面
 */
void OLED_Update_MainPage(ParkingData_t g_ParkingData)
{
    char buf[32]; // 缓冲区，每行最多16个字符+1个结束符就够了，给大一点防警告

    // 第1行：车位状态 (例如 "1:[X] 2:[ ] ")
    // 用 'X' 代表有车，' ' 代表没车
    sprintf(buf, "1:%c 2:%c 3:%c 4:%c ", 
            g_ParkingData.Spot[0].is_occupied ? 'X' : 'O',
            g_ParkingData.Spot[1].is_occupied ? 'X' : 'O',
            g_ParkingData.Spot[2].is_occupied ? 'X' : 'O',
            g_ParkingData.Spot[3].is_occupied ? 'X' : 'O');
    OLED_ShowString(1, 1, buf);

    // 第2行：空位和温度 (例如 "Free:2   26.5C  ")
    // 为了防止部分单片机C库默认不开启浮点数打印（导致%f不显示），把浮点拆成两个整数显示
    int temp_int = (int)g_ParkingData.Temp;
    int temp_dec = (int)(g_ParkingData.Temp * 10) % 10;
    if (temp_dec < 0) temp_dec = -temp_dec; // 确保小数部分是正数
    sprintf(buf, "Free:%d   %d.%dC  ", g_ParkingData.EmptySpotNum, temp_int, temp_dec);
    OLED_ShowString(2, 1, buf);

    // 第3行：网络状态 (例如 "WiFi:OK MQTT:OK ")
    // 注意留足空格覆盖掉以前旧的字符
    sprintf(buf, "WF:%s  MQ:%s  ", 
            g_ParkingData.WifiState ? "OK" : "NO",
            g_ParkingData.MqttState ? "OK" : "NO");
    OLED_ShowString(3, 1, buf);

    // 第4行：显示实时对时后的 RTC 时钟时间 (提取 HH:MM:SS 部分)
    char time_str[20];
    RTC_Get_Calendar_String(time_str);
    sprintf(buf, "Time: %s", &time_str[11]); 
    OLED_ShowString(4, 1, buf);
}


/**
 * @brief 显示车位已满
 * 
 */
void OLED_Update_SpotFull(void)
{

    // 清屏或者直接用空格覆盖
    OLED_Clear(); 
    
    OLED_ShowString(1, 1, ">> SPOT FULL <<");
    
}

/**
 * @brief 入场提示界面
 * @param uid 读到的RFID卡号（4字节原始二进制数组）
 * @param slot 分配的车位号（0~3，显示为 1~4）
 */
void OLED_Update_EnterPage(const uint8_t *uid, uint8_t slot)
{
    char buf[32];
    
    OLED_Clear(); 
    OLED_ShowString(1, 1, ">> CAR ENTER << ");
    
    // 将4字节卡号格式化为8位16进制大写字符串，避免指针作为字符串读取导致的乱码
    sprintf(buf, "UID: %02X%02X%02X%02X ", uid[0], uid[1], uid[2], uid[3]);
    OLED_ShowString(2, 1, buf);
    
    // 显示 1~4 号车位，对应主界面的车位号
    sprintf(buf, "Slot: #%d        ", slot + 1);
    OLED_ShowString(3, 1, buf);
    
    OLED_ShowString(4, 1, "GATE OPENING... ");
    vTaskDelay(pdMS_TO_TICKS(1000));
    OLED_Clear();
}

/**
 * @brief 出场结算界面
 * @param uid 读到的RFID卡号（4字节原始二进制数组）
 * @param hours 停车时长（小时）
 * @param mins 停车时长（分钟）
 * @param fee 费用（元）
 */
void OLED_Update_ExitPage(const uint8_t *uid, uint8_t hours, uint8_t mins, uint8_t fee)
{
    char buf[32];
    
    OLED_Clear();
    OLED_ShowString(1, 1, ">> CAR EXIT <<  ");
    
    sprintf(buf, "UID: %02X%02X%02X%02X ", uid[0], uid[1], uid[2], uid[3]);
    OLED_ShowString(2, 1, buf);
    
    sprintf(buf, "T:%dh%dm Fee:%dY  ", hours, mins, fee);
    OLED_ShowString(3, 1, buf);
    
    OLED_ShowString(4, 1, "KEY0:Pay BT:Pay ");
    vTaskDelay(pdMS_TO_TICKS(1000));
    OLED_Clear();
}

