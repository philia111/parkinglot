#ifndef __DHT11_H
#define __DHT11_H

#include <stdbool.h>
#include "stm32f4xx.h"

// 引脚宏定义：PG9 连接 DHT11 单总线
#define DHT11_RCC      RCC_AHB1Periph_GPIOG
#define DHT11_PORT     GPIOG
#define DHT11_PIN      GPIO_Pin_9

/**
 * @brief 初始化 DHT11 引脚并置为空闲高电平状态
 * @return 0 成功
 */
uint8_t DHT11_Init(void);

/**
 * @brief 读取 5 字节原始温湿度及校验和数据
 * @param buf 存储 5 字节数据的数组 (buf[0]:湿度整, buf[1]:湿度小, buf[2]:温度整, buf[3]:温度小, buf[4]:校验和)
 * @return true 读取并校验成功
 * @return false 读取超时或校验失败
 */
bool DHT11_Read_Raw(uint8_t buf[5]);

/**
 * @brief 直接读取整数温湿度
 * @param temp 输出温度指针 (摄氏度)
 * @param humi 输出相对湿度指针 (%)
 * @return true 读取成功
 * @return false 读取失败
 */
bool DHT11_Read_Data(uint8_t *temp, uint8_t *humi);

/**
 * @brief 直接读取浮点型温湿度 (可直接赋值给系统状态结构体)
 * @param temp 输出温度指针 (摄氏度)
 * @param humi 输出相对湿度指针 (%)
 * @return true 读取成功
 * @return false 读取失败
 */
bool DHT11_Read_Float(float *temp, float *humi);

// 兼容旧接口别名
#define Read_DHT11Data(buf)  DHT11_Read_Raw(buf)

#endif /* __DHT11_H */