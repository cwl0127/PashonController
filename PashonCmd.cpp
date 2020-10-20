#include "PashonCmd.h"
#include "CrcModbus.h"
#include <string.h>
#include <WSerial.h>

static const uint8_t usrNum[6] = {0x53, 0x45, 0x4E, 0x30, 0x30, 0x31};
static const uint8_t factoryNum[4] = {0x14, 0x91, 0x00, 0x01};

void pashonReadCmd(uint8_t addr, uint16_t cnt)
{
    uint8_t cmdBuffer[18];
    //拷贝用户序列号
    memcpy(cmdBuffer, usrNum, 6);
    //拷贝帧长度
    cmdBuffer[6] = 0x00;
    cmdBuffer[7] = 0x08;
    //拷贝出厂序列号
    memcpy(&cmdBuffer[8], factoryNum, 4);
    cmdBuffer[12] = 0x0A; //帧命令
    cmdBuffer[13] = addr;//寄存器首地址
    //拷贝寄存器数量
#if __BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__
    cmdBuffer[14] = (cnt >> 8) & 0xFF;
    cmdBuffer[15] = cnt & 0xFF;
#else
    cmdBuffer[14] = cnt & 0xFF;
    cmdBuffer[15] = (cnt >> 8) & 0xFF;
#endif
    //计算CRC16校验值
    uint16_t crcValue = crc16Modbus(cmdBuffer, 16);
    cmdBuffer[16] = crcValue & 0xFF;
    cmdBuffer[17] = (crcValue >> 8) & 0xFF;
    //发送
    for (size_t i = 0; i < sizeof(cmdBuffer); ++i)
    {
        //Serial.printf(cmdBuffer[i], HEX);
        Serial.printf("%02X ", cmdBuffer[i]);
    }
}

void pashonWriteCmd(uint8_t addr, uint8_t *buffer, uint16_t cnt)
{
    if (cnt < 2 && cnt > 12 && (cnt % 2 != 0))
    {
        Serial.println("data length error");
        return;
    }
    uint8_t cmdBuffer[16 + cnt];
    //拷贝用户序列号
    memcpy(cmdBuffer, usrNum, 6);
    //拷贝帧长度
    uint16_t frameLen = 6 + cnt;
#if __BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__
    cmdBuffer[6] = (frameLen >> 8) & 0xFF;
    cmdBuffer[7] = frameLen & 0xFF;
#else
    cmdBuffer[6] = frameLen & 0xFF;
    cmdBuffer[7] = (frameLen >> 8) & 0xFF;
#endif
    //拷贝出厂序列号
    memcpy(&cmdBuffer[8], factoryNum, 4);
    cmdBuffer[12] = 0x0B; //帧命令
    cmdBuffer[13] = addr;//寄存器地址
    memcpy(&cmdBuffer[14], buffer, cnt);
    //计算CRC16校验值
    uint16_t crcValue = crc16Modbus(cmdBuffer, 14 + cnt);

    cmdBuffer[14 + cnt] = crcValue & 0xFF;
    cmdBuffer[15 + cnt] = (crcValue >> 8) & 0xFF;
    //发送
    for (size_t i = 0; i < sizeof(cmdBuffer); ++i)
    {
        Serial.printf("%02X ", cmdBuffer[i]);
    }
}