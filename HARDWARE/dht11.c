#include "dht11.h"
#include "FreeRTOS.h"
#include "delay.h"
#include "task.h"

/* ---------------- 引脚模式与电平底层操作 ---------------- */

/**
 * @brief 将 DHT11 引脚设置为通用推挽输出
 */
static void DHT11_Mode_Out(void)
{
    DHT11_PORT->MODER &= ~(3U << (9 * 2));
    DHT11_PORT->MODER |= (1U << (9 * 2)); // 01: 通用输出模式
}

/**
 * @brief 将 DHT11 引脚设置为上拉输入
 */
static void DHT11_Mode_In(void)
{
    DHT11_PORT->MODER &= ~(3U << (9 * 2)); // 00: 输入模式
}

static inline void DHT11_PIN_HIGH(void)
{
    GPIO_SetBits(DHT11_PORT, DHT11_PIN);
}

static inline void DHT11_PIN_LOW(void)
{
    GPIO_ResetBits(DHT11_PORT, DHT11_PIN);
}

static inline uint8_t DHT11_PIN_READ(void)
{
    return GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN);
}

/* ---------------- 驱动初始化 ---------------- */

/**
 * @brief 初始化 DHT11 引脚，并释放总线使其进入空闲高电平状态
 */
uint8_t DHT11_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    // 1. 使能 GPIO 时钟
    RCC_AHB1PeriphClockCmd(DHT11_RCC, ENABLE);

    // 2. 配置引脚为推挽输出、上拉、高翻转速度
    GPIO_InitStructure.GPIO_Pin = DHT11_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_High_Speed;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(DHT11_PORT, &GPIO_InitStructure);

    // 3. 空闲状态：总线拉高释放
    DHT11_PIN_HIGH();

    return 0;
}

/* ---------------- 单总线时序协议实现 ---------------- */

/**
 * @brief 读取单位 (bit) 数据
 * @return 0 或 1; 超时返回 -1
 */
static int8_t DHT11_Read_Bit(void)
{
    uint32_t retry = 0;

    // 1. 等待前导 50us 低电平结束 (等待引脚变高)
    while (DHT11_PIN_READ() == Bit_RESET && retry < 100)
    {
        delay_us(1);
        retry++;
    }
    if (retry >= 100)
        return -1; // 超时

    // 2. 引脚变为高电平，延时 40us 后采样
    // 0 码高电平持续 26~28us，40us 时已被拉低
    // 1 码高电平持续 70us，40us 时仍为高电平
    delay_us(40);

    int8_t bit_val = 0;
    if (DHT11_PIN_READ() == Bit_SET)
    {
        bit_val = 1;

        // 3. 若为 1 码，等待剩余高电平结束 (等待引脚变低，对齐下一位起始边界)
        retry = 0;
        while (DHT11_PIN_READ() == Bit_SET && retry < 100)
        {
            delay_us(1);
            retry++;
        }
        if (retry >= 100)
            return -1;
    }
    else
    {
        // 若为 0 码，40us 后引脚已恢复为低电平 (已进入下一位的前导低电平)
        bit_val = 0;
    }

    return bit_val;
}

/**
 * @brief 读取单个字节 (8 bits, MSB 先行)
 * @return 读取的字节数据 (0~255); 超时返回 -1
 */
static int16_t DHT11_Read_Byte(void)
{
    uint8_t data = 0;
    for (uint8_t i = 0; i < 8; i++)
    {
        int8_t bit = DHT11_Read_Bit();
        if (bit < 0)
            return -1; // 超时错误直接退出
        data = (data << 1) | (uint8_t)bit;
    }
    return (int16_t)data;
}

/* ---------------- 对外 API 实现 ---------------- */

/**
 * @brief 读取 5 字节完整温湿度原始数据
 * @param buf 存放 5 字节数据的数组 (湿度整, 湿度小, 温度整, 温度小, 校验和)
 * @return true 成功; false 失败或校验错误
 */
bool DHT11_Read_Raw(uint8_t buf[5])
{
    uint32_t retry = 0;

    // 1. 主机发送起始信号：拉低总线持续 20ms
    DHT11_Mode_Out();
    DHT11_PIN_LOW();
    delay_ms(20);

    // 2. 在释放总线前进入 FreeRTOS 临界区，锁定全程微秒级交互
    taskENTER_CRITICAL();

    // 3. 主机拉高 30us 释放总线
    DHT11_PIN_HIGH();
    delay_us(30);

    // 4. 切换为输入模式，准备接收从机响应
    DHT11_Mode_In();

    // 5. 等待 DHT11 响应信号 (ACK)
    // 5.1 等待 DHT11 拉低总线 (响应起始，20~40us 内出现)
    retry = 0;
    while (DHT11_PIN_READ() == Bit_SET && retry < 100)
    {
        delay_us(1);
        retry++;
    }
    if (retry >= 100)
    {
        taskEXIT_CRITICAL();
        DHT11_Mode_Out();
        DHT11_PIN_HIGH();
        return false;
    }

    // 5.2 等待 DHT11 拉高总线 (80us 低电平响应结束)
    retry = 0;
    while (DHT11_PIN_READ() == Bit_RESET && retry < 100)
    {
        delay_us(1);
        retry++;
    }
    if (retry >= 100)
    {
        taskEXIT_CRITICAL();
        DHT11_Mode_Out();
        DHT11_PIN_HIGH();
        return false;
    }

    // 5.3 等待 DHT11 再次拉低总线 (80us 高电平准备结束，进入数据首位前导低电平)
    retry = 0;
    while (DHT11_PIN_READ() == Bit_SET && retry < 100)
    {
        delay_us(1);
        retry++;
    }
    if (retry >= 100)
    {
        taskEXIT_CRITICAL();
        DHT11_Mode_Out();
        DHT11_PIN_HIGH();
        return false;
    }

    // 6. 依次读取 5 字节数据
    for (uint8_t i = 0; i < 5; i++)
    {
        int16_t byte = DHT11_Read_Byte();
        if (byte < 0)
        {
            taskEXIT_CRITICAL();
            DHT11_Mode_Out();
            DHT11_PIN_HIGH();
            return false;
        }
        buf[i] = (uint8_t)byte;
    }

    // 7. 读取完毕，退出临界区，恢复任务调度
    taskEXIT_CRITICAL();

    // 8. 恢复为输出空闲高电平
    DHT11_Mode_Out();
    DHT11_PIN_HIGH();

    // 9. 校验和校验：前4字节低8位之和 == 校验和
    if (buf[4] == (uint8_t)(buf[0] + buf[1] + buf[2] + buf[3]))
    {
        return true;
    }

    return false;
}

/**
 * @brief 直接获取整型温湿度数据
 */
bool DHT11_Read_Data(uint8_t *temp, uint8_t *humi)
{
    uint8_t buf[5];
    if (DHT11_Read_Raw(buf))
    {
        if (humi)
            *humi = buf[0];
        if (temp)
            *temp = buf[2];
        return true;
    }
    return false;
}

/**
 * @brief 直接获取浮点型温湿度数据 (方便与 ParkingData_t 对接)
 */
bool DHT11_Read_Float(float *temp, float *humi)
{
    uint8_t buf[5];
    if (DHT11_Read_Raw(buf))
    {
        if (humi)
            *humi = (float)buf[0] + (float)buf[1] / 10.0f;
        if (temp)
            *temp = (float)buf[2] + (float)buf[3] / 10.0f;
        return true;
    }
    return false;
}