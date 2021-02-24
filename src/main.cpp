#include <Arduino.h>
#include <STM32FreeRTOS.h>
#include <STM32RTC.h>
#include <xxtea-lib.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <IWatchdog.h>
#include <STM32SD.h>
#include "boardpindef.h"
#include "usrconfig.h"
#include "PashonController.h"
#include "Command.h"
#include "WebconfDefines.h"
#include "html.h"
#include "CfgParameterDefs.h"
#include "usrlib.h"
#include "usrTaskDefs.h"

static ConfInfo cfgData = {0};
static bool cfgChanged = false;

uint8_t mac[6];
String strMAC;
String strLogin;
String successResHead("RELAY_{\"ret\":\"success\",\"control\":");
String successResTail("}\r\n");
String failureRes;  //R"(RELAY_{"ret":"failure","control":{"mac":"%s"}}\r\n)";
String strCmdJson;

static const char *ver = "1.0";
static const char *defaultIp = "192.168.5.10";
static const char *defaultSub = "255.255.255.0";
static const char *defaultGw = "192.168.5.1";
static const char *defaultDns = "192.168.5.1";
static const char *defaultServer = "192.168.5.103";
static uint16_t defaultPort = 8283;
static uint16_t defaultRelayTotal = 0;

IPAddress ip;
IPAddress gateway;
IPAddress subnet;
IPAddress dns;
char serverName[65] = {0};
uint16_t serverPort;
bool webserverEnable = false;
EthernetWebServer webserver(80);
STM32RTC &rtc = STM32RTC::getInstance();
xTaskHandle xConnHandle, xServerRcvHandle, xClientConnHandle, xClientRcvHandle, xPushAlarmHandle, xHandleTaskStart;
EventGroupHandle_t xCreatedEventGroup = NULL;

void softReset(void);
static uint8_t *MACAddressDefault(void);
static void EthernetInit(void);
static void webserverStartup(void);
static void vboardInit(void);
void loadConfiguration(void);
void updateConfiguration(ConfInfo &info);
void handleRoot(void);
void handleNotFound(void);
static void vConnectionCheckTask(void *pvParameters);
static void vServerReceiveDataTask(void *pvParameters);
static void vClientStatusCheckTask(void *pvParameters);
static void vClientReceiveDataTask(void *pvParametems);
static void vPushAlarm(void *pvParameters);
//static void vPrintTask(void *pvParameters);
static void vTaskStart(void *pvParameters);
static void appObjCreate(void);

void setup()
{
  vboardInit();
  xTaskCreate(vConnectionCheckTask, "ConnCheck", configMINIMAL_STACK_SIZE * 8, NULL, tskIDLE_PRIORITY + 4, &xConnHandle);
  xTaskCreate(vServerReceiveDataTask, "SrvRcveData", configMINIMAL_STACK_SIZE * 8, NULL, tskIDLE_PRIORITY + 2, &xServerRcvHandle);
  xTaskCreate(vClientStatusCheckTask, "CliStaCheck", configMINIMAL_STACK_SIZE * 10, NULL, tskIDLE_PRIORITY + 1, &xClientConnHandle);
  xTaskCreate(vClientReceiveDataTask, "CliRcveData", configMINIMAL_STACK_SIZE * 8, NULL, tskIDLE_PRIORITY + 1, &xClientRcvHandle);
  xTaskCreate(vPushAlarm, "PushAlarm", configMINIMAL_STACK_SIZE * 8, NULL, tskIDLE_PRIORITY + 3, &xPushAlarmHandle);
  //xTaskCreate(vPrintTask, "TaskPrint", configMINIMAL_STACK_SIZE * 2, NULL, tskIDLE_PRIORITY + 1, NULL);
  xTaskCreate(vTaskStart, "vTaskStart", configMINIMAL_STACK_SIZE * 2, NULL, tskIDLE_PRIORITY + 4, &xHandleTaskStart);
  appObjCreate();
  DebugPrintln("Start scheduler");
  vTaskStartScheduler();
  while (1)
    ;
}

void loop()
{
}

/**
 * @brief  Reset the mcu by software
 * @param  none
 * @note   use the 3.5 version of the firmware library. 
 */
void softReset(void)
{
  __set_FAULTMASK(1); // 关闭所有中断
  NVIC_SystemReset(); // 复位
}

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

static void EthernetInit(void)
{
  //reset W5500
  digitalWrite(W5500_RESET_PIN, LOW);
  delay(5);
  digitalWrite(W5500_RESET_PIN, HIGH);
  //start the Ethernet connection:
  Ethernet.init(USE_THIS_SS_PIN);
  if (digitalRead(USER_SW) == LOW)
  {
    DebugPrintln("USER_PIN IS LOW");
    ip.fromString(defaultIp);
    dns.fromString(defaultDns);
    gateway.fromString(defaultGw);
    subnet.fromString(defaultSub);
    webserverEnable = true;
  }
  else
  {
    loadConfiguration();
  }

  if (ip == INADDR_NONE)
  {
    Ethernet.begin(mac);
  }
  else
  {
    Ethernet.begin(mac, ip, dns, gateway, subnet);
  }

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware)
  {
    DebugPrintf("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    vTaskSuspendAll();
  }
  
  // if (Ethernet.linkStatus() == LinkOFF)
  // {
  //   DebugPrintln("Ethernet cable is not connected.");
  // }
}

static void webserverStartup(void)
{
  if (webserverEnable)
  {
    webserver.on("/", handleRoot);
    webserver.onNotFound(handleNotFound);
    webserver.begin();

    DebugPrintln(F("HTTP EthernetWebServer is @ IP : "));
    DebugPrintln(Ethernet.localIP());

    for (;;)
    {
      webserver.handleClient();
      if (cfgChanged)
      {
        cfgChanged = false;
        updateConfiguration(cfgData);
        delay(1000);
        softReset();
      }
    }
  }
}

static void vboardInit(void)
{
  pinMode(USER_SW, INPUT_PULLUP);
  pinMode(RED_LED_Pin, OUTPUT);
  pinMode(W5500_RESET_PIN, OUTPUT);
  Serial.begin(115200);
  MACAddressDefault();
  char buf[16];
  snprintf(buf, sizeof(buf), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  strMAC.concat(buf);
  DebugPrintf("MAC: %s\r\n", strMAC.c_str());
  strLogin = "RELAY_{\"ret\":\"success\",\"control\":{\"cmd\":\"ONLINE\",\"mac\":\"" + strMAC + "\"}}\r\n";
  failureRes = "RELAY_{\"ret\":\"failure\",\"control\":{\"mac\":\"" + strMAC + "\"}}\r\n";
  xxtea.setKey("^sensertek^");
  // Serial.print("Initializing SD card...");
  // while (!SD.begin(SD_DETECT_PIN))
  // {
  //   delay(10);
  // }
  // DebugPrintln("initialization done.");
  // if (!SD.exists("log.txt")) 
  // {
  //   DebugPrintln("Log.txt does not exist, create it");
  //   File logFile = SD.open("log.txt", FILE_READ);
  //   logFile.close();
  // }
  // initialize RTC 24H format
  rtc.begin();
  delay(500);
  EthernetInit();
  // Ethernet初始化完成后指示灯闪烁3次
  DebugPrintln("led start flash");
  for (int i = 0; i < 5; ++i)
  {
    digitalToggle(RED_LED_Pin);
    delay(500);
  }
  digitalWrite(RED_LED_Pin, LOW);
  webserverStartup();
  DebugPrint("Destination Server: ");
  DebugPrintln(serverName);
  DebugPrint("Destination port: ");
  DebugPrintln(serverPort);
  PashonController.begin();
  DebugPrintf("server address: ");
  DebugPrintln(Ethernet.localIP());
  DebugPrint("Shunt relay total: ");
  DebugPrintln(relayTotal);
  client.setTimeout(10);
}

void loadConfiguration(void)
{
  ConfInfo info;

  EEPROM.get(usrInfoEepAddr, info);
  // 如果没有设置IP就使用默认IP, 否则使用用户自定义的IP
  if (info.isConfigured != 0xAA)
  {
    ip.fromString(defaultIp);
    gateway.fromString(defaultGw);
    subnet.fromString(defaultSub);
    dns.fromString(defaultDns);
    memcpy(serverName, defaultServer, strlen(defaultServer));
    serverPort = defaultPort;
    relayTotal = defaultRelayTotal;
  }
  else
  {
    ip = IPAddress(info.localIP.data[0], info.localIP.data[1], info.localIP.data[2], info.localIP.data[3]);
    gateway = IPAddress(info.gw.data[0], info.gw.data[1], info.gw.data[2], info.gw.data[3]);
    subnet = IPAddress(info.sub.data[0], info.sub.data[1], info.sub.data[2], info.sub.data[3]);
    dns = IPAddress(info.dns.data[0], info.dns.data[1], info.dns.data[2], info.dns.data[3]);
    memset(serverName, 0, sizeof(serverName));
    memcpy(serverName, info.server.data, info.server.len);
    serverPort = info.port;
    relayTotal = info.relayTotal;
  }
}

void updateConfiguration(ConfInfo &info)
{
  EEPROM.put(usrInfoEepAddr, info);
}

void handleRoot(void)
{
  int argNum = webserver.args();
  Serial.print("URI: ");
  Serial.println(webserver.uri());
  if (0 == argNum)
  {
    const size_t capacity = JSON_OBJECT_SIZE(10) + 170;
    DynamicJsonDocument doc(capacity);
    doc["factoryid"] = "";
    doc["usrid"] = "";
    doc["ver"] = ver;
    char buf[20];
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    doc["mac"] = buf;
    doc["lip"] = defaultIp;
    doc["sub"] = defaultSub;
    doc["gw"] = defaultGw;
    doc["dns"] = defaultDns;
    doc["rip"] = defaultServer;
    doc["port"] = String(defaultPort).c_str();
    String s;
    serializeJson(doc, s);
    char tmpBuf[10240];
    snprintf(tmpBuf, 10240, indexPage, s.c_str());
    webserver.send(200, "text/html", tmpBuf);
  }
  else if (1 == argNum)
  {
    String message = webserver.argName(0) + ": " + webserver.arg(0) + "\n";
    Serial.println(message);
    webserver.send(200, "text/plain", "Configuration changes abandoned");
    softReset();
  }
  else
  {
    String message;
    bool success = true;

    memset(&cfgData, 0, sizeof(cfgData));
    if (webserver.hasArg("factoryid"))
    {
      strToHex(cfgData.factoryId.data, webserver.arg("factoryid").c_str(), webserver.arg("factoryid").length());
    }
    if (webserver.hasArg("usrid"))
    {
      strToHex(cfgData.usrId.data, webserver.arg("usrid").c_str(), webserver.arg("usrid").length());
    }
    if (webserver.hasArg("lip") && webserver.arg("lip").length() > 0)
    {
      IPAddress ip;
      if (!ip.fromString(webserver.arg("lip")))
      {
        message = "IP地址无效"; //"Invalid IP address";
        success = false;
        goto END;
      }
      cfgData.localIP.data[0] = ip[0];
      cfgData.localIP.data[1] = ip[1];
      cfgData.localIP.data[2] = ip[2];
      cfgData.localIP.data[3] = ip[3];

      if ((!webserver.hasArg("sub")) || (!ip.fromString(webserver.arg("sub"))))
      {
        message = "无效的子网掩码"; //"Invalid subnet mask";
        success = false;
        goto END;
      }
      cfgData.sub.data[0] = ip[0];
      cfgData.sub.data[1] = ip[1];
      cfgData.sub.data[2] = ip[2];
      cfgData.sub.data[3] = ip[3];

      if ((!webserver.hasArg("gw")) || (!ip.fromString(webserver.arg("gw"))))
      {
        message = "无效的网关"; //"Invalid gateway";
        success = false;
        goto END;
      }
      cfgData.gw.data[0] = ip[0];
      cfgData.gw.data[1] = ip[1];
      cfgData.gw.data[2] = ip[2];
      cfgData.gw.data[3] = ip[3];

      if (webserver.hasArg("dns") && ip.fromString(webserver.arg("dns")))
      {
        cfgData.dns.data[0] = ip[0];
        cfgData.dns.data[1] = ip[1];
        cfgData.dns.data[2] = ip[2];
        cfgData.dns.data[3] = ip[3];
      }
    }
    if (webserver.hasArg("rip"))
    {
      cfgData.server.len = webserver.arg("rip").length();
      memcpy(cfgData.server.data, webserver.arg("rip").c_str(), cfgData.server.len);
    }
    if (webserver.hasArg("port"))
    {
      cfgData.port = static_cast<uint16_t>(webserver.arg("port").toInt());
    }
    if (!webserver.hasArg("relaytotal"))
    {
      message = "继电器输出回路数量有误"; //"Invalid relay total";
      success = false;
      goto END;
    }
    cfgData.relayTotal = static_cast<uint16_t>(webserver.arg("relaytotal").toInt());
    cfgData.isConfigured = 0xAA;
    webserver.send(200, "text/plain", "Parameter configuration is successful");
    cfgChanged = true;
  END:
    if (!success)
    {
      webserver.send(200, "text/plain", message);
    }
  }
}

void handleNotFound(void)
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += webserver.uri();
  message += "\nMethod: ";
  message += (webserver.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += webserver.args();
  message += "\n";

  for (uint8_t i = 0; i < webserver.args(); i++)
  {
    message += " " + webserver.argName(i) + ": " + webserver.arg(i) + "\n";
  }

  webserver.send(404, "text/plain", message);
}

static void vConnectionCheckTask(void *pvParameters)
{
  for (;;)
  {
    PashonController.loopCheckConnection();
    xEventGroupSetBits(xCreatedEventGroup, TASK_BIT_0);
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

static void vServerReceiveDataTask(void *pvParameters)
{
  for (;;)
  {
    PashonController.loopReceiveData();
    xEventGroupSetBits(xCreatedEventGroup, TASK_BIT_1);
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

static void vClientStatusCheckTask(void *pvParameters)
{

  for (;;)
  {
    if (Ethernet.linkStatus() == LinkON)
    {
      if (!client.connected())
      {
        if (client.connect(serverName, serverPort))
        {
          taskENTER_CRITICAL();
          DebugPrintf("Connect to server %s successfully.\r\n", serverName);
//          writeLog("Connect to server %s successfully.", serverName);
          client.print(strLogin);
          taskEXIT_CRITICAL();
        }
        else
        {
          DebugPrintf("Failed to Connect server %s.\r\n", serverName);
//          writeLog("Failed to Connect server %s.", serverName);
        }
        
      }
      else
      {
        static uint32_t ulLastHeartbeatTime = millis();
        xTaskGetTickCount();
        if (millis() - ulLastHeartbeatTime > 15000)
        {
          taskENTER_CRITICAL();
          client.print(strLogin);
          taskEXIT_CRITICAL();
          xSemaphoreTake(xSendAllStatusBinarySem, 0);
          if (!operatingSwitchs)
          {
            postrt();
          }
          ulLastHeartbeatTime = millis();
          // 状态指示灯闪烁一次
          DebugPrintln("Send heartbeat");
          digitalWrite(RED_LED_Pin, HIGH);
          vTaskDelay(pdMS_TO_TICKS(200));
          digitalWrite(RED_LED_Pin, LOW);
        }
        else
        {
          if (xSemaphoreTake(xSendAllStatusBinarySem, 0) == pdTRUE)
          {
            postrt();
            ulLastHeartbeatTime = millis();
          }
        }
      }
    }
    else
    {
      static uint32_t lastMillis = 0;
      if (millis() - lastMillis >= 1000)
      {
        DebugPrintln("Ethernet cable is not connected.");
        lastMillis = millis();
      }
    }
    
    xEventGroupSetBits(xCreatedEventGroup, TASK_BIT_2);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

static void vClientReceiveDataTask(void *pvParametems)
{
  for (;;)
  {
    if (client.available())
    {
      vTaskSuspendAll();
      String rcvStr = client.readString();
      xTaskResumeAll();
      //String decodedStr = xxtea.decrypt(rcvStr);
      //cmdPreParse(decodedStr);
      //cmdPreParse(rcvStr);
      if (rcvStr.startsWith("RELAY_") && rcvStr.endsWith("\r\n"))
      {
        //DebugPrintf("Receive cmd: %s\r\n", rcvStr.c_str());
        strCmdJson = rcvStr.substring(6, rcvStr.length() - 2);
        if (cmdPreParse(strCmdJson) != 0) ////命令执行失败
        {
            portENTER_CRITICAL();
            client.print(failureRes);
            DebugPrint(failureRes);
            portEXIT_CRITICAL();
        }
        else  //命令发送成功
        {
          String successRes = successResHead + strCmdJson + successResTail;
          portENTER_CRITICAL();
          client.print(successRes);
          //DebugPrint(successRes);
          portEXIT_CRITICAL();
        }
      }
    }
    xEventGroupSetBits(xCreatedEventGroup, TASK_BIT_3);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

static void vPushAlarm(void *pvParameters)
{
  for (;;)
  {
    pushAlarmInfo();
    xEventGroupSetBits(xCreatedEventGroup, TASK_BIT_4);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// static void vPrintTask(void *pvParameters)
// {
//   for (;;)
//   {
//     // Sleep for one second.
//     vTaskDelay(pdMS_TO_TICKS(3000));
// #if (DEBUG == 1)
//     // Print count for previous second.
//     portENTER_CRITICAL();
//     // Print unused stack for threads.
//     Serial.println(F("Unused Stack: "));
//     Serial.print(pcTaskGetName(NULL));
//     Serial.print(": ");
//     Serial.println(uxTaskGetStackHighWaterMark(NULL));
//     Serial.print(pcTaskGetName(xConnHandle));
//     Serial.print(": ");
//     Serial.println(uxTaskGetStackHighWaterMark(xConnHandle));
//     Serial.print(pcTaskGetName(xServerRcvHandle));
//     Serial.print(": ");
//     Serial.println(uxTaskGetStackHighWaterMark(xServerRcvHandle));
//     Serial.print(pcTaskGetName(xClientConnHandle));
//     Serial.print(": ");
//     Serial.println(uxTaskGetStackHighWaterMark(xClientConnHandle));
//     Serial.print(pcTaskGetName(xClientRcvHandle));
//     Serial.print(": ");
//     Serial.println(uxTaskGetStackHighWaterMark(xClientRcvHandle));
//     Serial.print(pcTaskGetName(xPushAlarmHandle));
//     Serial.print(": ");
//     Serial.println(uxTaskGetStackHighWaterMark(xPushAlarmHandle));
//     portEXIT_CRITICAL();
// #endif
//   }
// }

static void vTaskStart(void *pvParameters)
{
  EventBits_t uxBits;

  if (IWatchdog.isReset(true))
  { // LED blinks to indicate reset
    for (uint8_t idx = 0; idx < 5; idx++)
    {
      digitalWrite(LED_BUILTIN, HIGH);
      vTaskDelay(pdMS_TO_TICKS(100));
      digitalWrite(LED_BUILTIN, LOW);
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }
  IWatchdog.begin(25000000); // Init the watchdog timer with 25 seconds timeout
  if (!IWatchdog.isEnabled())
  { // LED blinks indefinitely
    while (1)
    {
      digitalWrite(LED_BUILTIN, HIGH);
      vTaskDelay(pdMS_TO_TICKS(500));
      digitalWrite(LED_BUILTIN, LOW);
      vTaskDelay(pdMS_TO_TICKS(500));
    }
  }
  /* 打印系统开机状态，方便查看系统是否复位 */
  kprintf("=====================================================\r\n");
  kprintf("=System boot\r\n");
  kprintf("=====================================================\r\n");

  while (1)
  { 
    /* 等待所有任务发来事件标志 */
    uxBits = xEventGroupWaitBits(xCreatedEventGroup,  /* 事件标志组句柄 */
                                 TASK_BIT_ALL,        /* 等待TASK_BIT_ALL被设置 */
                                 pdTRUE,              /* 退出前TASK_BIT_ALL被清除，这里是TASK_BIT_ALL都被设置才表示“退出”*/
                                 pdTRUE,              /* 设置为pdTRUE表示等待TASK_BIT_ALL都被设置*/
                                 pdMS_TO_TICKS(5000)); /* 等待延迟时间 */

    if ((uxBits & TASK_BIT_ALL) == TASK_BIT_ALL)
    {
      IWatchdog.reload();
      //      kprintf("All user tasks are running normally\r\n");
    }
    else
    {
      /* 基本是每5000ms进来一次 */
      /* 通过变量uxBits简单的可以在此处检测那个任务长期没有发来运行标志 */
      kprintf("uxBits: %d\r\n", uxBits);
    }
  }
}

static void appObjCreate(void)
{
  xAlarmInfoQueue = xQueueCreate(5, sizeof(AlarmInfo));
  if (NULL == xAlarmInfoQueue)
  {
    kprintf("Failed to create xAlarmInfoQueue\r\n");
  }

  // 创建事件标志组
  xCreatedEventGroup = xEventGroupCreate();
  if (NULL == xCreatedEventGroup)
  {
    kprintf("Failed to create xCreatedEventGroup\r\n");
  }

  xDataReceivedEventGroup = xEventGroupCreate();
  if (NULL == xCreatedEventGroup)
  {
    kprintf("Failed to create xDataReceivedEventGroup\r\n");
  }

  xSendAllStatusBinarySem = xSemaphoreCreateBinary();
  if (NULL == xSendAllStatusBinarySem)
  {
    kprintf("Failed to create xSendAllStatusSem\r\n");
  }
}