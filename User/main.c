#include "app_parking.h"


int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

    // 2. 将栈大小由 128 改为 256 (1KB) 防止 ESP8266 等初始化时栈溢出
    xTaskCreate(InitTask, "InitTask", 256, NULL, 4, &InitTaskHandle);

    // 3. 必须启动 FreeRTOS 调度器！
    vTaskStartScheduler();

    // 如果运行到这里，说明系统堆栈溢出或内存不足导致调度器启动失败
    while (1)
    {
    }
}
