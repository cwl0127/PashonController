#ifndef __CFG_PARAMETER_DEFS_H
#define __CFG_PARAMETER_DEFS_H

#include <stdint.h>

struct ConfInfo
{
    struct
    {
        uint8_t data[4];
    } factoryId;
    struct
    {
        uint8_t data[6];
    } usrId;

    struct
    {
        uint8_t data[4];
    } localIP;

    struct
    {
        uint8_t data[4];
    } sub;

    struct
    {
        uint8_t data[4];
    } gw;

    struct
    {
        uint8_t data[4];
    } dns;

    struct
    {
        char data[64];
        uint8_t len;
    } server;

    uint16_t port;
    uint16_t relayTotal;
    uint8_t isConfigured;
};

#endif