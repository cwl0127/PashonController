#ifndef __CONNECTION_H
#define __CONNECTION_H

#include <SPI.h>
#include <Ethernet.h>

//#define MAX_CLIENT_NUM  7

class ServerConnectionClass : public EthernetServer
{
public:
    ServerConnectionClass(uint16_t port) : EthernetServer(port), _timeout(1000) {}
//    int availableData();
    bool send(uint8_t b);
    bool send(uint8_t *buf, size_t size);
    int receive(uint8_t *buf, size_t len);
    bool connected();
    void doLoop();
private:
    uint16_t _timeout;
    EthernetClient remoteClient;
};

class ClientConnectionClass : public EthernetClient
{

};

extern ServerConnectionClass conn;
extern EthernetClient client;

#endif