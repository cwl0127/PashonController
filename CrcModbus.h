#ifndef __CRC_MODBUS_H
#define __CRC_MODBUS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

uint16_t crc16Modbus( uint8_t *ptr, size_t Len );

#ifdef __cplusplus
}
#endif

#endif