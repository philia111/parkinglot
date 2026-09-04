#include "delay.h"
#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"

// 初始化 DWT 外设以用于微秒延时
static void DWT_Init(void)
{
    // 使能 TRC 模块
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    // 使能 DWT 中的 CYCCNT 计数器
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void delay_us(u32 us)
{
    // 确保 DWT 使能了
    if ((DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) == 0)
    {
        DWT_Init();
    }
    
    uint32_t startTicks = DWT->CYCCNT;
    // 计算需要的 CPU 周期数 (SystemCoreClock / 1000000 即 168 周期/微秒)
    uint32_t targetTicks = us * (SystemCoreClock / 1000000);
    
    while ((DWT->CYCCNT - startTicks) < targetTicks)
    {
        // 阻塞等待
    }
}

void delay_ms(u32 ms)
{
    // 如果 FreeRTOS 调度器已经跑起来了，我们用非阻塞的 vTaskDelay
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    {
        vTaskDelay(pdMS_TO_TICKS(ms));
    }
    else
    {
        // 调度器没跑起来时，使用微秒延时进行阻塞等待
        delay_us(ms * 1000);
    }
}