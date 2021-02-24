#include "crcmodbus.h"

/**
  *@brief 计算CRC16检验值 crcValue-16 (Modbus)
  *@param ptr 为待校验数据首地址，u16Len 为待校验数据长度
  *@retval 返回值为校验结果
  */
uint16_t crc16Modbus( uint8_t *ptr, size_t Len )
{
    uint16_t crcValue = 0xFFFF;
    size_t i;
    uint8_t temp;
    uint16_t const CRCTalbeAbs[] = {
        0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401,
        0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400
    };

    for (i = 0; i < Len; i++)
    {
        temp = *ptr++;
        crcValue = CRCTalbeAbs[(temp ^ crcValue) & 15] ^ (crcValue >> 4);
        crcValue = CRCTalbeAbs[((temp >> 4) ^ crcValue) & 15] ^ (crcValue >> 4);
    }
    return crcValue;
}