#include "HC05.h"
#include "USART2.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/**
 * @brief HC05蓝牙初始化
 *        内部调用USART2的初始化，波特率通常为9600
 */
void HC05_Init(void)
{
    USART2_Config(HC05_BAUD_RATE);
}

/**
 * @brief 通过蓝牙发送字符串到手机
 * @param str 要发送的字符串
 */
void HC05_SendString(const char *str)
{
    USART2_SendString(str);
}

/**
 * @brief 格式化发送数据到手机 (类似 printf)
 * @param fmt 格式化字符串，如 "Temp: %.1f\r\n"
 */
void HC05_SendFormat(const char *fmt, ...)
{
    char buf[128];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    USART2_SendString(buf);
}

/**
 * @brief 获取手机发来的蓝牙指令
 * @param out_cmd 用于存放获取到的指令字符串（需要确保外部数组足够大）
 * @return 1=成功获取到新指令，0=没有新指令
 */
uint8_t HC05_GetCommand(char *out_cmd)
{
    // 利用 USART2 空闲中断，判断是否接收到了完整的一帧数据
    if (u2_recvcnt > 0)
    {   
        // 拷贝数据到输出缓冲区
        strcpy(out_cmd, (char *)u2_recvbuf);
        
        // 清空接收缓冲区，准备接收下一次指令
        u2_recvcnt = 0;
        memset(u2_recvbuf, 0, sizeof(u2_recvbuf));
        
        return 1;
    }
    return 0;
}
