#include "rtc.h"
#include "stm32f4xx.h"
#include <stdio.h>

__IO uint32_t uwTimeDisplay = 0;
uint8_t aShowTime[50] = {0};
uint8_t aShowDate[50] = {0};

void RTC_Config(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;
    RTC_InitTypeDef RTC_InitStructure;
    RTC_TimeTypeDef RTC_TimeStructure;
    RTC_DateTypeDef RTC_DateStructure;

    // 1. 开启电源时钟并允许访问后备寄存器
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    PWR_BackupAccessCmd(ENABLE);

    // 2. 配置中断优先级 (NVIC)
    NVIC_InitStructure.NVIC_IRQChannel = RTC_WKUP_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // 3. 配置外部中断线 22 (RTC Wakeup 对应 EXTI 22)
    EXTI_ClearITPendingBit(EXTI_Line22);
    EXTI_InitStructure.EXTI_Line = EXTI_Line22;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    // 4. 检查是否是首次配置时间（读取备份寄存器 0）
    if (RTC_ReadBackupRegister(RTC_BKP_DR0) != 0x5050)
    {
        // 开启外部低速晶振 LSE
        RCC_LSEConfig(RCC_LSE_ON);
        while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET);

        // 选择 LSE 作为 RTC 时钟源并使能 RTC 时钟
        RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
        RCC_RTCCLKCmd(ENABLE);

        // 等待 RTC 寄存器同步
        RTC_WaitForSynchro();

        // 5. 先初始化 RTC 预分频器（LSE为32768Hz，分频出1Hz）
        RTC_InitStructure.RTC_AsynchPrediv = 128 - 1; // 127
        RTC_InitStructure.RTC_SynchPrediv = 256 - 1;  // 255
        RTC_InitStructure.RTC_HourFormat = RTC_HourFormat_24;
        RTC_Init(&RTC_InitStructure);

        // 6. 后设置首次上电的默认时间和日期
        RTC_TimeStructure.RTC_Hours = 0x23;
        RTC_TimeStructure.RTC_Minutes = 0x59;
        RTC_TimeStructure.RTC_Seconds = 0x50;
        RTC_SetTime(RTC_Format_BCD, &RTC_TimeStructure);

        RTC_DateStructure.RTC_Month = 0x07;
        RTC_DateStructure.RTC_Date = 0x06;
        RTC_DateStructure.RTC_Year = 0x24;
        RTC_DateStructure.RTC_WeekDay = RTC_Weekday_Saturday;
        RTC_SetDate(RTC_Format_BCD, &RTC_DateStructure);

        // 写入配置完成标记到备份寄存器 0
        RTC_WriteBackupRegister(RTC_BKP_DR0, 0x5050);
    }
    else
    {
        // 如果已经初始化过，只需等待寄存器同步即可，不需要重新开启LSE和重置时间
        RTC_WaitForSynchro();
    }

    // 7. 配置 RTC 唤醒定时器 (WakeUp)
    RTC_WakeUpCmd(DISABLE); // 必须先关闭才能配置
    
    // 使用 CK_SPRE (1Hz) 时钟源，计数器设为 1 即 1秒 唤醒一次
    RTC_WakeUpClockConfig(RTC_WakeUpClock_CK_SPRE_16bits); 
    RTC_SetWakeUpCounter(1); // 不能写 0，写 1 代表 1 秒

    RTC_ClearITPendingBit(RTC_IT_WUT);
    RTC_ITConfig(RTC_IT_WUT, ENABLE);
    RTC_WakeUpCmd(ENABLE);
}

void RTC_WKUP_IRQHandler(void)
{
    if (RTC_GetITStatus(RTC_IT_WUT) != RESET)
    {
        uwTimeDisplay = 1;
        RTC_ClearITPendingBit(RTC_IT_WUT);
        EXTI_ClearITPendingBit(EXTI_Line22);
    }
}

/**
 * @brief 根据年、月、日计算星期（蔡勒公式简化版）
 * @return 1 (星期一) 到 7 (星期日)
 */
static uint8_t Get_WeekDay(uint16_t year, uint8_t month, uint8_t day)
{
    if (month == 1 || month == 2)
    {
        month += 12;
        year--;
    }
    // 星期计算公式
    int w = (day + 2 * month + 3 * (month + 1) / 5 + year + year / 4 - year / 100 + year / 400 + 1) % 7;
    if (w == 0) return 7; // 星期日
    return w;
}

/**
 * @brief 将网络时间字符串写入 RTC 寄存器
 * @param time_str 格式必须为 "YYYY-MM-DD HH:MM:SS" (例如 "2026-07-24 20:12:43")
 * @return 1成功 0失败
 */
uint8_t RTC_Set_Calendar_String(const char *time_str)
{
    int year, month, day, hour, minute, second;
    RTC_TimeTypeDef RTC_TimeStructure;
    RTC_DateTypeDef RTC_DateStructure;

    // 1. 解析时间字符串
    if (sscanf(time_str, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second) != 6)
    {
        return 0; // 解析失败
    }

    // 2. 允许写入后备寄存器（解除保护）
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    PWR_BackupAccessCmd(ENABLE);

    // 3. 配置时间结构体 (使用 BIN 格式，库函数会自动转换成 BCD 存入寄存器)
    RTC_TimeStructure.RTC_Hours = (uint8_t)hour;
    RTC_TimeStructure.RTC_Minutes = (uint8_t)minute;
    RTC_TimeStructure.RTC_Seconds = (uint8_t)second;
    RTC_TimeStructure.RTC_H12 = RTC_HourFormat_24;
    
    if (RTC_SetTime(RTC_Format_BIN, &RTC_TimeStructure) == ERROR)
    {
        return 0;
    }

    // 4. 配置日期结构体 (年只需要后两位，如 2026 年存 26)
    RTC_DateStructure.RTC_Year = (uint8_t)(year % 100);
    RTC_DateStructure.RTC_Month = (uint8_t)month;
    RTC_DateStructure.RTC_Date = (uint8_t)day;
    RTC_DateStructure.RTC_WeekDay = Get_WeekDay((uint16_t)year, (uint8_t)month, (uint8_t)day);

    if (RTC_SetDate(RTC_Format_BIN, &RTC_DateStructure) == ERROR)
    {
        return 0;
    }

    // 5. 写入配置完成标记到后备寄存器，防止复位被默认值覆盖
    RTC_WriteBackupRegister(RTC_BKP_DR0, 0x5050);

    return 1;
}

/**
 * @brief 从 RTC 寄存器读取时间并格式化为字符串
 * @param time_str 用于存储结果的缓冲区，长度至少为 20 字节
 */
void RTC_Get_Calendar_String(char *time_str)
{
    RTC_TimeTypeDef RTC_TimeStructure;
    RTC_DateTypeDef RTC_DateStructure;

    // 根据 STM32F4 参考手册，必须先读 Time，再读 Date 才能释放影子寄存器锁，保证数据一致性
    RTC_GetTime(RTC_Format_BIN, &RTC_TimeStructure);
    RTC_GetDate(RTC_Format_BIN, &RTC_DateStructure);

    // 格式化输出 "YYYY-MM-DD HH:MM:SS"
    sprintf(time_str, "20%02d-%02d-%02d %02d:%02d:%02d",
            RTC_DateStructure.RTC_Year,
            RTC_DateStructure.RTC_Month,
            RTC_DateStructure.RTC_Date,
            RTC_TimeStructure.RTC_Hours,
            RTC_TimeStructure.RTC_Minutes,
            RTC_TimeStructure.RTC_Seconds);
}
