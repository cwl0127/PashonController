/*
  ESP8266 Blink by Simon Peter
  Blink the blue LED on the ESP-01 module
  This example code is in the public domain

  The blue LED on the ESP-01 module is connected to GPIO1
  (which is also the TXD pin; so we cannot use DebugPrint() at the same time)

  Note that this sketch uses RED_LED to find the pin with the internal LED
*/
#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include "crcmodbus.h"
#include "PashonCmd.h"
#include "usrconfig.h"

#define RED_LED_Pin PA13
#define GREEN_LED_Pin PA14
#define YELLOW_LED_Pin PA15
#define BLUE_LED_Pin PB4
#define USER_SW  USER_BTN 

#define ARR_NUM(arr)  (uint32_t)(sizeof(arr) / sizeof(arr[0]))

uint8_t read[] = {0x50, 0x53, 0x44, 0x5A, 0x4B, 0x4A, 0x00, 0x08, 0x14, 0x87, 0x00, 0x01, 0x0a, 0x00, 0x00, 0x0a};

uint8_t mac[6];
IPAddress ip(192, 168, 5, 120);
// Enter the IP address of the server you're connecting to:
IPAddress serverAddr(192, 168, 5, 105);
IPAddress dnsserver(192, 168, 5, 1);
IPAddress gateway(192, 168, 5, 1);
IPAddress subnet(255, 255, 255, 0);

EthernetClient client, newClients[7];
EthernetServer server(8080);

uint8_t *MACAddressDefault(void)
{
    uint32_t baseUID = *(uint32_t *)UID_BASE;
    mac[0] = 0x00;
    mac[1] = 0x80;
    mac[2] = 0xE1;
    mac[3] = (baseUID & 0x00FF0000) >> 16;
    mac[4] = (baseUID & 0x0000FF00) >> 8;
    mac[5] = (baseUID & 0x000000FF);
  
  return mac;
}

uint8_t res[sizeof(read) + 2];

void setup()
{
  MACAddressDefault();
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  DebugPrintf("MAC: %X%X%X%X%X%X\n", mac[0], mac[1], mac[2],mac[3], mac[4], mac[5]);
  DebugPrintln("");
  
  //start the Ethernet connection:
  Ethernet.init(4);
  Ethernet.begin(mac, ip, dnsserver, gateway, subnet);
  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware)
  {
    DebugPrintln("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true)
    {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF)
  {
    DebugPrintln("Ethernet cable is not connected.");
  }
  // give the Ethernet shield a second to initialize:
  delay(1000);
  DebugPrintln("connecting...");

  // if you get a connection, report back via serial:
  if (client.connect(serverAddr, 8283))
  {
    DebugPrintln("connected");
  }
  else
  {
    // if you didn't get a connection to the server:
    DebugPrintln("connection failed");
  }
  server.begin();
  DebugPrint("Server address:");
  DebugPrintln(Ethernet.localIP());
}

// the loop function runs over and over again forever
void loop()
{
  // // if there are incoming bytes available
  // // from the server, read them and print them:
  if (client.available())
  {
    char c = client.read();
    DebugPrint(c);
  }

  // // as long as there are bytes in the serial queue,
  // // read them and send them out the socket if it's open:
  while (Serial.available() > 0)
  {
    char inChar = Serial.read();
    if (client.connected())
    {
      client.print(inChar);
    }
  }

  // // if the server's disconnected, stop the client:
  if (!client.connected())
  {
    DebugPrintln();
    DebugPrintln("disconnecting.");
    if (Ethernet.linkStatus() == LinkON)
    {
      // if you get a connection, report back via serial:
      if (client.connect(serverAddr, 8283))
      {
        DebugPrintln("connected");
      }
      else
      {
        // if you didn't get a connection to the server:
        DebugPrintln("connection failed");
      }
    }
  }

  EthernetClient newClient = server.accept();
  if (newClient)
  {
    for (byte i = 0; i < ARR_NUM(newClients); i++)
    {
      if (!newClients[i])
      {
        DebugPrint("We have a new client #");
        DebugPrintln(i);
        newClient.print("Hello, client number: ");
        newClient.println(i);
        // Once we "accept", the client is no longer tracked by EthernetServer
        // so we must store it into our list of clients
        newClients[i] = newClient;
        break;
      }
    }
  }

  // check for incoming data from all clients
  for (byte i = 0; i < ARR_NUM(newClients); i++)
  {
    if (newClients[i] && newClients[i].available() > 0)
    {
      // read bytes from a client
      byte buffer[80];
      int count = newClients[i].read(buffer, 80);
      // write the bytes to Serial
      DebugPrintf("Receive data from client %d\n", i);
      Serial.write(buffer, count);
      DebugPrintln("");
      newClients[i].println("ok");
    }
  }

  // stop any clients which disconnect
  for (byte i = 0; i < 8; i++)
  {
    if (newClients[i] && !newClients[i].connected())
    {
      DebugPrint("disconnect client #");
      DebugPrintln(i);
      newClients[i].stop();
    }
  }
}
