#include "Connection.h"
#include "usrconfig.h"
#include "log.h"

ServerConnectionClass conn(9991);
EthernetClient client;


bool ServerConnectionClass::send(uint8_t b)
{
    if (!this->remoteClient || !this->remoteClient.connected())
        return false;
    if (this->remoteClient.write(b) > 0)
        return true;
    else
        return false;
}

bool ServerConnectionClass::send(uint8_t *buf, size_t size)
{
    if (!this->remoteClient || !this->remoteClient.connected())
    {
        DebugPrintln("send faild,disconnected");
        return false;
    }
    // Serial.println("Send:");
    // for (size_t i = 0; i < size; ++i)
    // {
    //     Serial.print(buf[i], HEX);
    //     Serial.print(' ');
    // }
    // Serial.println("");
    if (this->remoteClient.write(buf, size) == size)
    {
//        DebugPrintln("send command ok");
        return true;
    }
    else
    {
//        DebugPrintln("send command failed");
        return false;
    }
}

int ServerConnectionClass::receive(uint8_t *buf, size_t len)
{
    if (this->remoteClient && this->remoteClient.available() > 0)
    {
        return this->remoteClient.read(buf, len);
    }
    else
    {
        return 0;
    }
}

bool ServerConnectionClass::connected()
{
    return this->remoteClient && this->remoteClient.connected();
}

void ServerConnectionClass::doLoop()
{
    if (!this->remoteClient)
    {
        //建立新的连接
        vTaskSuspendAll();
        EthernetClient newClient = this->accept();
        xTaskResumeAll();
        if (newClient)
        {
            IPAddress ip = newClient.remoteIP();
            DebugPrintf("New client, ip: %u.%u.%u.%u\r\n", ip[0], ip[1], ip[2], ip[3]);
            taskENTER_CRITICAL();
//            writeLog("We have a new client, ip: %u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
            taskEXIT_CRITICAL();
            newClient.setTimeout(100); //设置接收等待超时时间为100ms
            this->remoteClient = newClient;
        }
    }
    else
    { //停止失效的连接
        if (!(this->remoteClient.connected()))
        {
            IPAddress ip = this->remoteClient.remoteIP();
            DebugPrintf("Client disconnected, ip:  %u.%u.%u.%u\r\n", ip[0], ip[1], ip[2], ip[3]);
            taskENTER_CRITICAL();
//            writeLog("Client disconnected, ip:  %u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
            taskEXIT_CRITICAL();
            this->remoteClient.stop();
        }
    }
}