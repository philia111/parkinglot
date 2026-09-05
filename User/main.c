#include "app_parking.h"


int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

    // 创建系统初始化任务（栈大小 256 字 = 1024 字节）
    xTaskCreate(InitTask, "InitTask", 256, NULL, 4, &InitTaskHandle);

    // 启动 FreeRTOS 调度器
    vTaskStartScheduler();

    // 如果运行到这里，说明系统堆栈溢出或内存不足导致调度器启动失败
    while (1)
    {
    }
}
