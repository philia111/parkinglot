#ifndef _MFRC522_H_
#define _MFRC522_H_

#include "stm32f4xx.h"

/*==========================================================================
 * 引脚定义 (模拟SPI)
 *   PB4  -> MISO (输入)
 *   PG6  -> SDA/CS (输出)
 *   PG7 -> RST (输出)
 *   PB3  -> SCK (输出)
 *   PB5  -> MOSI (输出)
 *==========================================================================*/

/* SDA/CS */
#define RC522_CS_HIGH() GPIO_SetBits(GPIOG, GPIO_Pin_6)
#define RC522_CS_LOW()  GPIO_ResetBits(GPIOG, GPIO_Pin_6)

/* RST */
#define RC522_RST_HIGH() GPIO_SetBits(GPIOG, GPIO_Pin_7)
#define RC522_RST_LOW()  GPIO_ResetBits(GPIOG, GPIO_Pin_7)

/* SCK */
#define RC522_SCK_HIGH() GPIO_SetBits(GPIOB, GPIO_Pin_3)
#define RC522_SCK_LOW()  GPIO_ResetBits(GPIOB, GPIO_Pin_3)

/* MOSI */
#define RC522_MOSI_HIGH() GPIO_SetBits(GPIOB, GPIO_Pin_5)
#define RC522_MOSI_LOW()  GPIO_ResetBits(GPIOB, GPIO_Pin_5)

/* MISO */
#define RC522_MISO_READ() GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_4)

/*==========================================================================
 * MFRC522 寄存器地址定义
 *==========================================================================*/
/* Command and Status */
#define MFRC522_REG_COMMAND     0x01
#define MFRC522_REG_COM_I_EN    0x02
#define MFRC522_REG_DIV_I_EN    0x03
#define MFRC522_REG_COM_IRQ     0x04
#define MFRC522_REG_DIV_IRQ     0x05
#define MFRC522_REG_ERROR       0x06
#define MFRC522_REG_STATUS1     0x07
#define MFRC522_REG_STATUS2     0x08
#define MFRC522_REG_FIFO_DATA   0x09
#define MFRC522_REG_FIFO_LEVEL  0x0A
#define MFRC522_REG_WATER_LEVEL 0x0B
#define MFRC522_REG_CONTROL     0x0C
#define MFRC522_REG_BIT_FRAMING 0x0D
#define MFRC522_REG_COLL        0x0E

/* Command */
#define MFRC522_REG_MODE         0x11
#define MFRC522_REG_TX_MODE      0x12
#define MFRC522_REG_RX_MODE      0x13
#define MFRC522_REG_TX_CONTROL   0x14
#define MFRC522_REG_TX_ASK       0x15
#define MFRC522_REG_TX_SEL       0x16
#define MFRC522_REG_RX_SEL       0x17
#define MFRC522_REG_RX_THRESHOLD 0x18
#define MFRC522_REG_DEMOD        0x19
#define MFRC522_REG_MIFARE       0x1C
#define MFRC522_REG_SERIAL_SPEED 0x1F

/* Configuration */
#define MFRC522_REG_CRC_RESULT_H    0x21
#define MFRC522_REG_CRC_RESULT_L    0x22
#define MFRC522_REG_MOD_WIDTH       0x24
#define MFRC522_REG_RF_CFG          0x26
#define MFRC522_REG_GS_N            0x27
#define MFRC522_REG_CW_GS_P         0x28
#define MFRC522_REG_MOD_GS_P        0x29
#define MFRC522_REG_T_MODE          0x2A
#define MFRC522_REG_T_PRESCALER     0x2B
#define MFRC522_REG_T_RELOAD_H      0x2C
#define MFRC522_REG_T_RELOAD_L      0x2D
#define MFRC522_REG_T_COUNTER_VAL_H 0x2E
#define MFRC522_REG_T_COUNTER_VAL_L 0x2F

/* Test */
#define MFRC522_REG_TEST_SEL1      0x31
#define MFRC522_REG_TEST_SEL2      0x32
#define MFRC522_REG_TEST_PIN_EN    0x33
#define MFRC522_REG_TEST_PIN_VALUE 0x34
#define MFRC522_REG_TEST_BUS       0x35
#define MFRC522_REG_AUTO_TEST      0x36
#define MFRC522_REG_VERSION        0x37
#define MFRC522_REG_ANALOG_TEST    0x38
#define MFRC522_REG_TEST_DAC1      0x39
#define MFRC522_REG_TEST_DAC2      0x3A
#define MFRC522_REG_TEST_ADC       0x3B

/*==========================================================================
 * MFRC522 命令字
 *==========================================================================*/
#define PCD_IDLE       0x00 /* 空闲 */
#define PCD_AUTHENT    0x0E /* 验证密钥 */
#define PCD_RECEIVE    0x08 /* 接收数据 */
#define PCD_TRANSMIT   0x04 /* 发送数据 */
#define PCD_TRANSCEIVE 0x0C /* 发送并接收数据 */
#define PCD_RESETPHASE 0x0F /* 复位 */
#define PCD_CALCCRC    0x03 /* CRC计算 */

/*==========================================================================
 * Mifare_One 卡片命令字
 *==========================================================================*/
#define PICC_REQIDL    0x26 /* 寻天线区内未进入休眠状态的卡 */
#define PICC_REQALL    0x52 /* 寻天线区内全部卡 */
#define PICC_ANTICOLL  0x93 /* 防冲撞 */
#define PICC_SELECTTAG 0x93 /* 选卡 */
#define PICC_AUTHENT1A 0x60 /* 验证A密钥 */
#define PICC_AUTHENT1B 0x61 /* 验证B密钥 */
#define PICC_READ      0x30 /* 读块 */
#define PICC_WRITE     0xA0 /* 写块 */
#define PICC_DECREMENT 0xC0 /* 扣款 */
#define PICC_INCREMENT 0xC1 /* 充值 */
#define PICC_RESTORE   0xC2 /* 调块数据到缓冲区 */
#define PICC_TRANSFER  0xB0 /* 保存缓冲区中数据 */
#define PICC_HALT      0x50 /* 休眠 */

/*==========================================================================
 * 返回状态
 *==========================================================================*/
#define MI_OK       0
#define MI_NOTAGERR 1
#define MI_ERR      2

/*==========================================================================
 * 函数声明
 *==========================================================================*/
void MFRC522_GPIO_Init(void);
void MFRC522_Init(void);
void MFRC522_Reset(void);
void MFRC522_AntennaOn(void);
void MFRC522_AntennaOff(void);

void MFRC522_WriteReg(u8 addr, u8 val);
u8 MFRC522_ReadReg(u8 addr);
void MFRC522_SetBitMask(u8 reg, u8 mask);
void MFRC522_ClearBitMask(u8 reg, u8 mask);

u8 MFRC522_Request(u8 reqMode, u8 *TagType);
u8 MFRC522_Anticoll(u8 *serNum);
u8 MFRC522_ToCard(u8 command, u8 *sendData, u8 sendLen, u8 *backData, u16 *backLen);
void MFRC522_CalcCRC(u8 *pIndata, u8 len, u8 *pOutData);
u8 MFRC522_SelectTag(u8 *serNum);
u8 MFRC522_Halt(void);

u8 MFRC522_Auth(u8 authMode, u8 blockAddr, u8 *key, u8 *serNum);
u8 MFRC522_Read(u8 blockAddr, u8 *recvData);
u8 MFRC522_Write(u8 blockAddr, u8 *writeData);

#endif
