#ifndef __PASHON_CMD_H
#define __PASHON_CMD_H


#include <stdint.h>

// struct ReadCmdFrame
// {
//     uint8_t usrNum[6];
//     uint16_t frameLength;
//     uint8_t factoryNum[4];
    
// };

// struct WriteCmdFrame
// {

// };
// struct PashonContorller
// {
//     uint16_t frameLength;
//     uint8_t regAddress;
//     uint16_t regCount;
//     void read(uint8_t addr, uint16_t cnt);
//     void write(uint8_t addr, uint16_t cnt);
// };
#ifdef __cplusplus
extern "C" {
#endif

void pashonReadCmd(uint8_t addr, uint16_t cnt);
void pashonWriteCmd(uint8_t addr, uint8_t *buffer, uint16_t cnt);

#ifdef __cplusplus
}
#endif

#endif