#include "delay.h"
#include "dht11.h"
#include "FreeRTOS.h"
#include "task.h"


// PG9 - DHT11 单总线引脚
void DHT11_GPIO_In(void); // 前向声明

/**
 * @brief 配置PG9为输出模式，并且发送开始信号
 */
void DHT11_GPIO_Out(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    // 1. 开启GPIOG的时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_High_Speed;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;

    GPIO_Init(GPIOG, &GPIO_InitStructure);

    // 2. 主机拉低总线持续至少18ms
    GPIO_ResetBits(GPIOG, GPIO_Pin_9);
    delay_ms(20);

    // 3. 主机拉高电平(释放总线)持续20~40us，等待DHT11响应
    GPIO_SetBits(GPIOG, GPIO_Pin_9);
    delay_us(30);

    // 4. 立即切换为输入模式，释放总线让DHT11能拉低产生ACK
    DHT11_GPIO_In();
}

/**
 * @brief 配置PG9为输入模式
 */
void DHT11_GPIO_In(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_High_Speed;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;

    GPIO_Init(GPIOG, &GPIO_InitStructure);
}

/**
 * @brief 判断DHT11是否正确响应
 * @return true 响应成功
 * @return false 响应失败
 */
bool DHT11_Is_ACK(void)
{
    uint32_t cnt = 0;

    // DHT11_GPIO_Out() 末尾已切换为输入模式，无需重复调用 DHT11_GPIO_In()

    // 等待总线被DHT11拉低(ACK起始)，超时100us
    while (GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_9) == SET && cnt < 100)
    {
        delay_us(1);
        cnt++;
    }
    if (cnt >= 100)
        return false;

    cnt = 0;

    // 等待DHT11释放总线(拉高)，超时100us
    while (GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_9) == RESET && cnt < 100)
    {
        delay_us(1);
        cnt++;
    }

    // 低电平持续约80us，cnt应远小于100
    if (cnt >= 100)
        return false;
    else
        return true;
}

uint8_t Read_bit(void)
{
    uint32_t timeout = 0;

    // 等待低电平出现(数据位起始信号)，超时100us
    while (GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_9) == SET)
    {
        timeout++;
        delay_us(1);
        if (timeout > 100)
            return 0;
    }

    // 等待高电平出现(数据位开始)，超时100us
    timeout = 0;
    while (GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_9) == RESET)
    {
        timeout++;
        delay_us(1);
        if (timeout > 100)
            return 0;
    }

    // 此时为高电平起始，一次性精准延时 40us
    delay_us(40);

    // 40us 后，采样判定引脚电平
    if (GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_9) == SET)
    {
        // 40us 后仍为高电平，代表数据 1 (高电平持续 70us)
        // 等待其高电平结束变回低电平，防止影响下一比特的检测
        timeout = 0;
        while (GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_9) == SET)
        {
            timeout++;
            delay_us(1);
            if (timeout > 100)
                break;
        }
        return 1;
    }
    else
    {
        // 40us 后已变回低电平，代表数据 0 (高电平持续 26us)
        return 0;
    }
}

uint8_t Read_Byte(void)
{
    uint8_t data = 0;
    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        data |= Read_bit() << (7 - i);
    }
    return data;
}

bool Read_Data(uint8_t buf[5])
{
    int i;
    for (i = 0; i < 5; i++)
    {
        buf[i] = Read_Byte();
    }

    // 校验和 = 前4字节低8位之和
    if (buf[4] == (uint8_t)(buf[0] + buf[1] + buf[2] + buf[3]))
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool Read_DHT11Data(uint8_t buf[5])
{
    bool res = false;
    
    // 1. 发送开始信号 (这一步包含 20ms 延时，不可在临界区中运行)
    DHT11_GPIO_Out();

    // 2. 进入 FreeRTOS 临界区，仅锁定微秒级的数据传输过程
    taskENTER_CRITICAL();

    if (DHT11_Is_ACK() == true)
    {
        if (Read_Data(buf) == true)
        {
            res = true;
        }
    }

    // 3. 退出临界区，恢复任务调度
    taskEXIT_CRITICAL();
    return res;
}

void DHT11Init(void)
{
    uint8_t buf[5] = {0};

    // DHT11上电后需等待1s稳定
    delay_ms(1000);
        if (Read_DHT11Data(buf))
        {
           
        }
        else
        {
           
        }
        delay_ms(1500); // 采样间隔加长到 1.5s，确保 DHT11 有足够时间准备好响应
    
}