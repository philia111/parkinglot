#include "MFRC522.h"
extern void delay_us(u32 us);
extern void delay_ms(u32 ms);
/*==========================================================================
 * GPIO 初始化 (模拟SPI引脚)
 *==========================================================================*/
void MFRC522_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    /* 开启时钟: GPIOB, GPIOG */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOG, ENABLE);
    /* PB4 - MISO (输入，上拉) */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    /* PG6 - SDA/CS (推挽输出) */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOG, &GPIO_InitStructure);
    /* PB3 - SCK, PB5 - MOSI, PG7 - RST (推挽输出) */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
    GPIO_Init(GPIOG, &GPIO_InitStructure);
    /* 默认状态：CS高，SCK低 */
    RC522_CS_HIGH();
    RC522_SCK_LOW();
}
/*==========================================================================
 * 模拟SPI: 发送一个字节 (MSB first, CPOL=0, CPHA=0)
 *==========================================================================*/
static void SPI_SendByte(u8 byte)
{
    u8 i;
    for (i = 0; i < 8; i++)
    {
        if (byte & 0x80)
            RC522_MOSI_HIGH();
        else
            RC522_MOSI_LOW();
        RC522_SCK_HIGH();
        delay_us(1);
        RC522_SCK_LOW();
        delay_us(1);
        byte <<= 1;
    }
}
/*==========================================================================
 * 模拟SPI: 读取一个字节 (MSB first, CPOL=0, CPHA=0)
 *==========================================================================*/
static u8 SPI_ReadByte(void)
{
    u8 i, data = 0;
    for (i = 0; i < 8; i++)
    {
        data <<= 1;
        RC522_SCK_HIGH();
        delay_us(1);
        if (RC522_MISO_READ())
            data |= 0x01;
        RC522_SCK_LOW();
        delay_us(1);
    }
    return data;
}
/*==========================================================================
 * 写MFRC522寄存器
 * addr: 寄存器地址  val: 写入值
 * 地址格式: 0XXXXXX0  (bit7=0表示写, bit0=0)
 *==========================================================================*/
void MFRC522_WriteReg(u8 addr, u8 val)
{
    RC522_CS_LOW();
    SPI_SendByte((addr << 1) & 0x7E); /* 地址字节：bit7=0(写), [6:1]=addr, bit0=0 */
    SPI_SendByte(val);
    RC522_CS_HIGH();
}
/*==========================================================================
 * 读MFRC522寄存器
 * addr: 寄存器地址
 * 地址格式: 1XXXXXX0  (bit7=1表示读, bit0=0)
 *==========================================================================*/
u8 MFRC522_ReadReg(u8 addr)
{
    u8 val;
    RC522_CS_LOW();
    SPI_SendByte(((addr << 1) & 0x7E) | 0x80); /* 地址字节：bit7=1(读), [6:1]=addr, bit0=0 */
    val = SPI_ReadByte();
    RC522_CS_HIGH();
    return val;
}
/*==========================================================================
 * 置位寄存器指定位
 *==========================================================================*/
void MFRC522_SetBitMask(u8 reg, u8 mask)
{
    u8 tmp = MFRC522_ReadReg(reg);
    MFRC522_WriteReg(reg, tmp | mask);
}
/*==========================================================================
 * 清除寄存器指定位
 *==========================================================================*/
void MFRC522_ClearBitMask(u8 reg, u8 mask)
{
    u8 tmp = MFRC522_ReadReg(reg);
    MFRC522_WriteReg(reg, tmp & (~mask));
}
/*==========================================================================
 * 开启天线（每次开启需至少1ms的间隔）
 *==========================================================================*/
void MFRC522_AntennaOn(void)
{
    u8 temp = MFRC522_ReadReg(MFRC522_REG_TX_CONTROL);
    if (!(temp & 0x03))
    {
        MFRC522_SetBitMask(MFRC522_REG_TX_CONTROL, 0x03);
    }
}
/*==========================================================================
 * 关闭天线
 *==========================================================================*/
void MFRC522_AntennaOff(void)
{
    MFRC522_ClearBitMask(MFRC522_REG_TX_CONTROL, 0x03);
}
/*==========================================================================
 * 复位MFRC522
 *==========================================================================*/
void MFRC522_Reset(void)
{
    RC522_RST_HIGH();
    delay_us(10);
    RC522_RST_LOW();
    delay_us(10);
    RC522_RST_HIGH();
    delay_us(100);
    MFRC522_WriteReg(MFRC522_REG_COMMAND, PCD_RESETPHASE); /* 软复位 */
    delay_ms(50);
}
/*==========================================================================
 * 初始化MFRC522
 *==========================================================================*/
void MFRC522_Init(void)
{
    MFRC522_GPIO_Init();
    MFRC522_Reset();
    /* 定时器配置：TModeReg, TPrescalerReg */
    MFRC522_WriteReg(MFRC522_REG_T_MODE, 0x8D);      /* TAuto=1, f(Timer)=6.78MHz/TPreScaler */
    MFRC522_WriteReg(MFRC522_REG_T_PRESCALER, 0x3E); /* TPreScaler = TModeReg[3:0]:TPrescalerReg */
    MFRC522_WriteReg(MFRC522_REG_T_RELOAD_L, 30);    /* 定时器重载值低字节 */
    MFRC522_WriteReg(MFRC522_REG_T_RELOAD_H, 0);     /* 定时器重载值高字节 */
    MFRC522_WriteReg(MFRC522_REG_TX_ASK, 0x40); /* 100%ASK调制 */
    MFRC522_WriteReg(MFRC522_REG_MODE, 0x3D);   /* CRC初始值0x6363 */
    MFRC522_AntennaOn(); /* 开启天线 */
}
/*==========================================================================
 * MFRC522和ISO14443卡通信
 *  command:   MFRC522命令字
 *  sendData:  通过MFRC522发送到卡片的数据
 *  sendLen:   发送数据的字节长度
 *  backData:  接收到的卡片返回数据
 *  backLen:   返回数据的位长度
 * 返回值:  MI_OK / MI_ERR
 *==========================================================================*/
u8 MFRC522_ToCard(u8 command, u8 *sendData, u8 sendLen, u8 *backData, u16 *backLen)
{
    u8 status = MI_ERR;
    u8 irqEn = 0x00;
    u8 waitIRq = 0x00;
    u8 lastBits;
    u8 n;
    u16 i;
    switch (command)
    {
    case PCD_AUTHENT:
        irqEn = 0x12;
        waitIRq = 0x10;
        break;
    case PCD_TRANSCEIVE:
        irqEn = 0x77;
        waitIRq = 0x30;
        break;
    default:
        break;
    }
    MFRC522_WriteReg(MFRC522_REG_COM_I_EN, irqEn | 0x80); /* 允许中断请求 */
    MFRC522_ClearBitMask(MFRC522_REG_COM_IRQ, 0x80);      /* 清除所有中断请求标志位 */
    MFRC522_SetBitMask(MFRC522_REG_FIFO_LEVEL, 0x80);     /* FlushBuffer=1, FIFO初始化 */
    MFRC522_WriteReg(MFRC522_REG_COMMAND, PCD_IDLE); /* 空闲命令，取消当前命令 */
    /* 向FIFO中写入数据 */
    for (i = 0; i < sendLen; i++)
    {
        MFRC522_WriteReg(MFRC522_REG_FIFO_DATA, sendData[i]);
    }
    /* 执行命令 */
    MFRC522_WriteReg(MFRC522_REG_COMMAND, command);
    if (command == PCD_TRANSCEIVE)
    {
        MFRC522_SetBitMask(MFRC522_REG_BIT_FRAMING, 0x80); /* StartSend=1, 开始传输数据 */
    }
    /* 等待接收数据完成，超时约25ms */
    i = 2000;
    do
    {
        n = MFRC522_ReadReg(MFRC522_REG_COM_IRQ);
        i--;
    } while ((i != 0) && !(n & 0x01) && !(n & waitIRq));
    MFRC522_ClearBitMask(MFRC522_REG_BIT_FRAMING, 0x80); /* StartSend=0 */
    if (i != 0)
    {
        if (!(MFRC522_ReadReg(MFRC522_REG_ERROR) & 0x1B)) /* 无错误 */
        {
            status = MI_OK;
            if (n & irqEn & 0x01)
            {
                status = MI_NOTAGERR;
            }
            if (command == PCD_TRANSCEIVE)
            {
                n = MFRC522_ReadReg(MFRC522_REG_FIFO_LEVEL);
                lastBits = MFRC522_ReadReg(MFRC522_REG_CONTROL) & 0x07;
                if (lastBits)
                    *backLen = (u16)(n - 1) * 8 + lastBits;
                else
                    *backLen = (u16)n * 8;
                if (n == 0)
                    n = 1;
                if (n > 16)
                    n = 16;
                /* 读FIFO中接收到的数据 */
                for (i = 0; i < n; i++)
                {
                    backData[i] = MFRC522_ReadReg(MFRC522_REG_FIFO_DATA);
                }
            }
        }
        else
        {
            status = MI_ERR;
        }
    }
    return status;
}
/*==========================================================================
 * 寻卡
 *  reqMode:  寻卡模式
 *            PICC_REQIDL = 寻天线区内未进入休眠的卡
 *            PICC_REQALL = 寻天线区内全部卡
 *  TagType:  卡片类型代码 (2字节)
 *            0x4400 = Mifare_UltraLight
 *            0x0400 = Mifare_One(S50)
 *            0x0200 = Mifare_One(S70)
 *            0x0800 = Mifare_Pro(X)
 *            0x4403 = Mifare_DESFire
 * 返回值:  MI_OK / MI_ERR
 *==========================================================================*/
u8 MFRC522_Request(u8 reqMode, u8 *TagType)
{
    u8 status;
    u16 backBits;
    MFRC522_WriteReg(MFRC522_REG_BIT_FRAMING, 0x07); /* TxLastBits=7, 发送7bit */
    TagType[0] = reqMode;
    status = MFRC522_ToCard(PCD_TRANSCEIVE, TagType, 1, TagType, &backBits);
    if ((status != MI_OK) || (backBits != 0x10)) /* 期望返回16bit(2字节) */
    {
        status = MI_ERR;
    }
    return status;
}
/*==========================================================================
 * 防冲撞：获取卡片序列号 (4字节UID + 1字节校验)
 *  serNum: 返回5字节 [0..3]=UID, [4]=BCC校验
 * 返回值:  MI_OK / MI_ERR
 *==========================================================================*/
u8 MFRC522_Anticoll(u8 *serNum)
{
    u8 status;
    u8 i;
    u8 serNumCheck = 0;
    u16 backBits;
    MFRC522_WriteReg(MFRC522_REG_BIT_FRAMING, 0x00); /* TxLastBits=0, 发送完整字节 */
    serNum[0] = PICC_ANTICOLL;
    serNum[1] = 0x20;
    status = MFRC522_ToCard(PCD_TRANSCEIVE, serNum, 2, serNum, &backBits);
    if (status == MI_OK)
    {
        /* 校验序列号 */
        for (i = 0; i < 4; i++)
        {
            serNumCheck ^= serNum[i];
        }
        if (serNumCheck != serNum[4])
        {
            status = MI_ERR;
        }
    }
    return status;
}
/*==========================================================================
 * CRC计算
 *==========================================================================*/
void MFRC522_CalcCRC(u8 *pIndata, u8 len, u8 *pOutData)
{
    u8 i, n;
    MFRC522_ClearBitMask(MFRC522_REG_DIV_IRQ, 0x04);  /* CRCIRq = 0 */
    MFRC522_SetBitMask(MFRC522_REG_FIFO_LEVEL, 0x80); /* 清FIFO */
    /* 向FIFO写入数据 */
    for (i = 0; i < len; i++)
    {
        MFRC522_WriteReg(MFRC522_REG_FIFO_DATA, *(pIndata + i));
    }
    MFRC522_WriteReg(MFRC522_REG_COMMAND, PCD_CALCCRC);
    /* 等CRC计算完成 */
    i = 0xFF;
    do
    {
        n = MFRC522_ReadReg(MFRC522_REG_DIV_IRQ);
        i--;
    } while ((i != 0) && !(n & 0x04));
    /* 读取CRC结果 */
    pOutData[0] = MFRC522_ReadReg(MFRC522_REG_CRC_RESULT_L);
    pOutData[1] = MFRC522_ReadReg(MFRC522_REG_CRC_RESULT_H);
}
/*==========================================================================
 * 选卡
 *  serNum: 卡片序列号 (4字节)
 * 返回值: 卡片容量（SAK）
 *==========================================================================*/
u8 MFRC522_SelectTag(u8 *serNum)
{
    u8 i;
    u8 status;
    u8 size;
    u16 recvBits;
    u8 buffer[9];
    buffer[0] = PICC_SELECTTAG;
    buffer[1] = 0x70;
    for (i = 0; i < 5; i++)
    {
        buffer[i + 2] = *(serNum + i);
    }
    MFRC522_CalcCRC(buffer, 7, &buffer[7]);
    status = MFRC522_ToCard(PCD_TRANSCEIVE, buffer, 9, buffer, &recvBits);
    if ((status == MI_OK) && (recvBits == 0x18)) /* 返回24bit = 3字节 */
    {
        size = buffer[0];
    }
    else
    {
        size = 0;
    }
    return size;
}
/*==========================================================================
 * 让卡片进入休眠模式
 *==========================================================================*/
u8 MFRC522_Halt(void)
{
    u16 backLen;
    u8 buff[4];
    buff[0] = PICC_HALT;
    buff[1] = 0;
    MFRC522_CalcCRC(buff, 2, &buff[2]);
    MFRC522_ToCard(PCD_TRANSCEIVE, buff, 4, buff, &backLen);
    return MI_OK;
}
/*==========================================================================
 * 验证卡片密钥
 *  authMode:  PICC_AUTHENT1A (0x60) 或 PICC_AUTHENT1B (0x61)
 *  blockAddr: 块地址
 *  key:       密钥 (6字节)
 *  serNum:    卡片序列号 (4字节)
 * 返回值: MI_OK / MI_ERR
 *==========================================================================*/
u8 MFRC522_Auth(u8 authMode, u8 blockAddr, u8 *key, u8 *serNum)
{
    u8 status;
    u16 backBits;
    u8 i;
    u8 buff[12];
    /* {验证命令, 块地址, 6字节密钥, 4字节UID} = 12字节 */
    buff[0] = authMode;
    buff[1] = blockAddr;
    for (i = 0; i < 6; i++)
    {
        buff[i + 2] = *(key + i);
    }
    for (i = 0; i < 4; i++)
    {
        buff[i + 8] = *(serNum + i);
    }
    status = MFRC522_ToCard(PCD_AUTHENT, buff, 12, buff, &backBits);
    /* 检查Status2Reg的MFCrypto1On位 (bit3) */
    if ((status != MI_OK) || (!(MFRC522_ReadReg(MFRC522_REG_STATUS2) & 0x08)))
    {
        status = MI_ERR;
    }
    return status;
}
/*==========================================================================
 * 读块数据 (16字节)
 *  blockAddr: 块地址
 *  recvData:  接收缓冲区 (至少18字节: 16数据 + 2CRC)
 * 返回值: MI_OK / MI_ERR
 *==========================================================================*/
u8 MFRC522_Read(u8 blockAddr, u8 *recvData)
{
    u8 status;
    u16 backLen;
    recvData[0] = PICC_READ;
    recvData[1] = blockAddr;
    MFRC522_CalcCRC(recvData, 2, &recvData[2]);
    status = MFRC522_ToCard(PCD_TRANSCEIVE, recvData, 4, recvData, &backLen);
    if ((status != MI_OK) || (backLen != 0x90)) /* 期望返回 144 bit = 18字节 */
    {
        status = MI_ERR;
    }
    return status;
}
/*==========================================================================
 * 写块数据 (16字节)
 *  blockAddr: 块地址
 *  writeData: 要写入的数据 (16字节)
 * 返回值: MI_OK / MI_ERR
 *
 * Mifare写块流程：
 *   1. 发送写命令 + 块地址 + CRC → 卡回ACK (4bit)
 *   2. 发送16字节数据 + CRC → 卡回ACK (4bit)
 *==========================================================================*/
u8 MFRC522_Write(u8 blockAddr, u8 *writeData)
{
    u8 status;
    u16 backLen;
    u8 i;
    u8 buff[18];
    /* 第一步: 发送写命令 */
    buff[0] = PICC_WRITE;
    buff[1] = blockAddr;
    MFRC522_CalcCRC(buff, 2, &buff[2]);
    status = MFRC522_ToCard(PCD_TRANSCEIVE, buff, 4, buff, &backLen);
    /* 卡片应答ACK: 4bit, 值为0x0A */
    if ((status != MI_OK) || (backLen != 4) || ((buff[0] & 0x0F) != 0x0A))
    {
        status = MI_ERR;
    }
    if (status == MI_OK)
    {
        /* 第二步: 发送16字节数据 + CRC */
        for (i = 0; i < 16; i++)
        {
            buff[i] = *(writeData + i);
        }
        MFRC522_CalcCRC(buff, 16, &buff[16]);
        status = MFRC522_ToCard(PCD_TRANSCEIVE, buff, 18, buff, &backLen);
        if ((status != MI_OK) || (backLen != 4) || ((buff[0] & 0x0F) != 0x0A))
        {
            status = MI_ERR;
        }
    }
    return status;
}