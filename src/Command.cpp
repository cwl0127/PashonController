#include "Command.h"
#include "usrTaskDefs.h"
#include <vector>

extern String strMAC;
extern STM32RTC &rtc;

typedef int (*Handle_t)(JsonVariant &obj);

struct CmdTable
{
    const char *cmd;
    Handle_t handle;
};

xSemaphoreHandle xSendAllStatusBinarySem = NULL;
volatile bool operatingSwitchs = false;
const char *prefix = "RELAY_";
const char *suffix = "\r\n";

static std::vector<String> split(const String &str, const String &pattern);
static String weekday2Str(uint8_t w);
static int operateDevice(JsonVariant &obj);
static int updateTimer(JsonVariant &obj);
static int deleteTimer(JsonVariant &obj);
static int syncTime(JsonVariant &obj);
static void postTimer(void);

static std::vector<String> split(const String &str, const String &pattern)
{
    //const char* convert to char*
    char strc[strlen(str.c_str()) + 1];
    strcpy(strc, str.c_str());
    std::vector<String> resultVec;
    char *tmpStr = strtok(strc, pattern.c_str());
    while (tmpStr != NULL)
    {
        resultVec.push_back(String(tmpStr));
        tmpStr = strtok(NULL, pattern.c_str());
    }

    return resultVec;
}

static String weekday2Str(uint8_t w)
{
    String ret;

    if (0 == w)
    {
        return ret;
    }
    for (uint8_t i = 0; i < 7; ++i)
    {
        if (bitRead(w, i))
        {
            if (0 == i)
            {
                ret += "7";
            }
            else
            {
                if (ret.length() > 0)
                {
                    ret += ",";
                }
                ret += String(i);
            }
        }
    }

    return ret;
}

static int operateDevice(JsonVariant &obj)
{
    operatingSwitchs = true;
    const char *status = obj["status"];
    int addr = obj["addr"];
    bool sw;
    OperateMode mode = UNKNOWN_MODE;

    DebugPrintf("operateDevice status:%s addr: %d\r\n", status, addr);
    if (strcmp(status, "open") == 0)
    {
        sw = true;
        mode = SWITCH_ON;
    }
    else if (strcmp(status, "close") == 0)
    {
        sw = false;
        mode = SWITCH_OFF;
    }
    else
    {
        return -1;
    }
    if (addr < 0 || addr > 16)
    {
        return -2;
    }
    int ret = -3;
    switch (addr)
    {
    case 0:
    {
        // 如果是全部合闸，先判断主路设备的开关状态
        if (SWITCH_ON == mode) //全部合闸
        {
            if (PashonController.getMainSwStatus() != 0)
            {
                DebugPrintf("relayTotal=%d, start operate shunting switchs\r\n", relayTotal);
                for (int n = 1; n <= relayTotal; ++n)
                {
                    DebugPrintf("Turn on switch %d\r\n", n);
                    PashonController.operateSubDevice(n, sw);
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    xEventGroupSetBits(xCreatedEventGroup, TASK_BIT_3);
                }
                DebugPrintln("All switches turn on");
                ret = 0;
            }
            else
            {
                PashonController.operateController(mode);
                DebugPrintln("The terminal is just switch on, it needs to wait 3 seconds to switch on the shunting switchs");
                vTaskDelay(pdMS_TO_TICKS(3000)); //终端刚合闸时要等3秒后才能合闸分路，因为分路设备上电初始化需要时间
                xEventGroupSetBits(xCreatedEventGroup, TASK_BIT_3);
                uint16_t sta = 0;
                int cnt = 2;
                do
                {
                    if (PashonController.getSwitchStatus(&sta))
                    {
                        break;
                    }
                    --cnt;
                    vTaskDelay(pdMS_TO_TICKS(300));
                    xEventGroupSetBits(xCreatedEventGroup, TASK_BIT_3);
                } while (cnt > 0);
                if (0 == cnt)
                {
                    DebugPrintln("Failed to get terminal status");
                }
                else
                {
                    if (SWITCH_ON == sta)
                    {
                        portENTER_CRITICAL();
                        PashonController.setMainSwStatus(1);
                        portEXIT_CRITICAL();
                        vTaskDelay(pdMS_TO_TICKS(300));
                        DebugPrintf("relayTotal=%d, start operate shunting switchs\r\n", relayTotal);
                        for (int n = 1; n <= relayTotal; ++n)
                        {
                            DebugPrintf("Turn on switch %d\r\n", n);
                            PashonController.operateSubDevice(n, sw);
                            vTaskDelay(pdMS_TO_TICKS(1200));
                            xEventGroupSetBits(xCreatedEventGroup, TASK_BIT_3);
                        }
                        DebugPrintln("All switches turn on");
                        ret = 0;
                    }
                    else
                    {
                        DebugPrintln("Failed to turn on all switches");
                    }
                }
            }
        }
        else //全部分闸
        {
            if (PashonController.getMainSwStatus() != 0)
            {
                PashonController.operateController(mode);
                DebugPrintln("The terminal is just turn off, it needs to wait 3 seconds before reading the terminal status");
                vTaskDelay(pdMS_TO_TICKS(3000)); //终端刚合闸时要等1秒后再回读状态
                xEventGroupSetBits(xCreatedEventGroup, TASK_BIT_3);
                uint16_t sta = 0;
                int cnt = 2;
                do
                {
                    if (PashonController.getSwitchStatus(&sta))
                    {
                        break;
                    }
                    --cnt;
                    vTaskDelay(pdMS_TO_TICKS(300));
                    xEventGroupSetBits(xCreatedEventGroup, TASK_BIT_3);
                } while (cnt > 0);
                if (0 == cnt)
                {
                    DebugPrintln("Failed to get terminal status");
                }
                else
                {
                    if (SWITCH_OFF == sta)
                    {
                        portENTER_CRITICAL();
                        PashonController.setMainSwStatus(0);
                        portEXIT_CRITICAL();
                        DebugPrintln("All switches turn off");
                        ret = 0;
                    }
                    else
                    {
                        DebugPrintln("Failed to turn off all switches");
                    }
                }
            }
            else
            {
                DebugPrintln("All switches turn off");
                ret = 0;
            }
        }
    }
    break;
    default:
    {
        //只有终端设备合闸时才能操作分路设备
        if (PashonController.getMainSwStatus() != 0)
        {
            PashonController.operateSubDevice(addr, sw);
            vTaskDelay(pdMS_TO_TICKS(600));
            xEventGroupSetBits(xCreatedEventGroup, TASK_BIT_3);
            bool sta;
            if (PashonController.getSubDevSwitchStatus(addr, &sta))
            {
                if (sw == sta)
                {
                    DebugPrintf("Switch %d operate successfully\r\n", addr);
                    ret = 0;
                }
            }
        }
        else
        {
            PashonController.operateController(SWITCH_ON);
            DebugPrintln("The terminal is just switch on, it needs to wait 3 seconds to switch on the shunting equipment");
            vTaskDelay(pdMS_TO_TICKS(3000)); //终端刚合闸时要等3秒后才能合闸分路，因为分路设备上电初始化需要时间,继电器吸合时有干扰会造成TCP重连
            xEventGroupSetBits(xCreatedEventGroup, TASK_BIT_3);
            uint16_t sta = 0;
            int cnt = 2;
            do
            {
                if (PashonController.getSwitchStatus(&sta))
                {
                    break;
                }
                --cnt;
                vTaskDelay(pdMS_TO_TICKS(300));
                xEventGroupSetBits(xCreatedEventGroup, TASK_BIT_3);
            } while (cnt > 0);
            if (0 == cnt)
            {
                DebugPrintln("Failed to get master switch status");
            }
            else
            {
                if (SWITCH_ON == sta)
                {
                    portENTER_CRITICAL();
                    PashonController.setMainSwStatus(1);
                    portEXIT_CRITICAL();
                }
            }
            PashonController.operateSubDevice(addr, sw);
            vTaskDelay(pdMS_TO_TICKS(600));
            xEventGroupSetBits(xCreatedEventGroup, TASK_BIT_3);
            bool s;
            if (PashonController.getSubDevSwitchStatus(addr, &s))
            {
                if (sw == s)
                {
                    DebugPrintf("Switche %d operate successfully\r\n", addr);
                    ret = 0;
                }
            }
        }
    }
    break;
    }
    xSemaphoreGive(xSendAllStatusBinarySem);
    operatingSwitchs = false;
    return ret;
}

static int updateTimer(JsonVariant &obj)
{
    const char *status = obj["status"];
    const char *time = obj["time"];
    String weekday = obj["weekday"];
    String addr = obj["addr"];
    uint8_t id = obj["id"];
    TimerInfo t;

    t.status = (strcmp(status, "open") == 0 ? 0x22 : 0x33);
    if (weekday == "") //单次定时
    {
        t.type = 1;     //闹钟类别01，年月日定时
        t.loopType = 0; //闹钟循环类别00,单次
        t.used = 1;     //启用
        t.weekday = 0;  //单次定时，本字节置0
        int year, mon, day, hour, min, sec;
        if (sscanf(time, "%d-%d-%d %d:%d:%d", &year, &mon, &day, &hour, &min, &sec) != 6)
        {
            return -1;
        }
        t.time[0] = static_cast<uint8_t>(year - 2000); //转换为相对于2000年以来的时间
        t.time[1] = static_cast<uint8_t>(mon);
        t.time[2] = static_cast<uint8_t>(day);
        t.time[3] = static_cast<uint8_t>(hour);
        t.time[4] = static_cast<uint8_t>(min);
        t.time[5] = static_cast<uint8_t>(sec);
    }
    else //循环定时
    {
        t.type = 0;     //闹钟类别为00，星期定时
        t.loopType = 1; //闹钟循环类别01，星期类型的循环
        t.used = 1;     //启用
        std::vector<String> vec = split(weekday, ",");
        size_t len = vec.size();
        if (len == 0 || len > 7)
        {
            return -1;
        }
        uint8_t tmp = 0;
        for (size_t i = 0; i < len; ++i)
        {
            long w = vec[i].toInt();
            if (w > 0L && w < 8L)
            {
                if (7L == w)
                {
                    bitSet(tmp, 0);
                }
                else
                {
                    bitSet(tmp, w);
                }
            }
        }
        t.weekday = tmp;
        t.time[0] = t.time[1] = t.time[2] = 0;
        int hour, min, sec;
        if (sscanf(time, "%d:%d:%d", &hour, &min, &sec) != 3)
        {
            return -1;
        }
        t.time[3] = static_cast<uint8_t>(hour);
        t.time[4] = static_cast<uint8_t>(min);
        t.time[5] = static_cast<uint8_t>(sec);
    }
    if (id > 8)
    {
        return -1;
    }
    std::vector<String> addrVec = split(addr, ",");
    size_t len = addrVec.size();
    if (len > 17)
    {
        return -1;
    }
    //检查是否有分路设备和主设备的定时
    bool hasSubTimer = false;
    bool hasMianTimer = false;
    for (auto item : addrVec)
    {
        if (item != "0")
        {
            hasSubTimer = true;
        }
        else
        {
            hasMianTimer = true;
        }
        if (hasSubTimer && hasMianTimer)
            break;
    }
    
    if (t.status == 0x22) //开定时
    {
        t.id = id;
        t.addr = 0;
        DebugPrintf("Setting circuit %u open timer %u on %02u:%02u:%02u\r\n", t.addr, t.id, t.time[3], t.time[4], t.time[5]);
        PashonController.setControllerTimer(t);
        vTaskDelay(pdMS_TO_TICKS(200));
        if (hasSubTimer)
        {
            if (PashonController.getMainSwStatus() == 0)
            {
                DebugPrintln("Main switch not switched on");
                return -1;
            }
            else
            {
                //分路的定时开时间要比主路开时间晚4秒，主路开启后要至少等3秒分路设备才会启动完成
                uint32_t tmp = t.time[3] * 3600U + t.time[4] * 60U + t.time[5];
                tmp += 10;
                t.time[3] = tmp / 3600U;
                t.time[4] = (tmp % 3600U) / 60U;
                t.time[5] = tmp % 60;
                for (size_t i = 0; i < len; ++i)
                {
                    long a = addrVec[i].toInt();
                    if (a > 0L && a < 17L && id > 0 && id < 3)
                    {
                        t.id = id;
                        t.addr = static_cast<uint8_t>(a);
                        DebugPrintf("Setting circuit %u open timer %u on %02u:%02u:%02u\r\n", t.addr, t.id, t.time[3], t.time[4], t.time[5]);
                        if (!PashonController.setControllerTimer(t))
                        {
                            DebugPrintf("Setting circuit %u open timer %u failed\r\n", t.addr, t.id);
                            return -1;
                        }
                        vTaskDelay(pdMS_TO_TICKS(200));
                    }
                }
            }
        }
    }
    else //关定时
    {
        if (hasMianTimer)
        {
            t.id = id;
            t.addr = 0;
            DebugPrintf("Setting circuit %u close timer %u on %02u:%02u:%02u\r\n", t.addr, t.id, t.time[3], t.time[4], t.time[5]);
            if (!PashonController.setControllerTimer(t))
            {
                DebugPrintf("Setting circuit %u close timer %u failed\r\n", t.addr, t.id);
                return -1;
            }
        }
        else
        {
            if (PashonController.getMainSwStatus() == 0)
            {
                DebugPrintln("Main switch not switched on");
                return -1;
            }
            for (size_t i = 0; i < len; ++i)
            {
                long a = addrVec[i].toInt();
                if (a > 0L && a < 17L && id > 0 && id < 3)
                {
                    t.id = id;
                    t.addr = static_cast<uint8_t>(a);
                    DebugPrintf("Setting circuit %u close timer %u on %02u:%02u:%02u\r\n", t.addr, t.id, t.time[3], t.time[4], t.time[5]);
                    if (!PashonController.setControllerTimer(t))
                    {
                        DebugPrintf("Setting circuit %u close timer %u failed\r\n", t.addr, t.id);
                        return -1;
                    }
                    vTaskDelay(pdMS_TO_TICKS(200));
                }
            }
        }
    }
#if (DEBUG == 1)
//    postTimer();
#endif
    return 0;
}

static int deleteTimer(JsonVariant &obj)
{
    String addr = obj["addr"];
    uint8_t id = obj["id"];
    TimerInfo t = {0};

    std::vector<String> addrVec = split(addr, ",");
    size_t len = addrVec.size();
    if (len > 17)
    {
        return -1;
    }
    //检查是否有分路设备的定时
    bool hasSubTimer = false;
    for (auto item : addrVec)
    {
        if (item != "0")
        {
            hasSubTimer = true;
            break;
        }
    }
    uint8_t onFlag = 0;
    if (hasSubTimer)
    {
        if (PashonController.getMainSwStatus() == 0)
        {
            PashonController.operateController(SWITCH_ON);
            vTaskDelay(pdMS_TO_TICKS(3500));
            onFlag = 1;
        }
    }
    for (size_t i = 0; i < len; ++i)
    {
        long a = addrVec[i].toInt();
        if ((0L == a && id > 0 && id < 9) || (a > 0L && a < 17L && id > 0 && id < 3))
        {
            t.id = id;
            t.addr = static_cast<uint8_t>(a);
            PashonController.setControllerTimer(t);
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
    if (onFlag)
    {
        PashonController.operateController(SWITCH_OFF);
    }
#if (DEBUG == 1)
//    postTimer();
#endif
    return 0;
}

static int syncTime(JsonVariant &obj)
{
    uint32_t time = obj["time"];
    uint32_t localTime = time + 3600 * 8;
    rtc.setEpoch(localTime);
    PashonController.setDeviceTime(localTime);
    return 0;
}

static void postTimer(void)
{
    size_t cnt = 0;
    TimerInfo allTimerInfo[40];
    bool ret = PashonController.getControllerTimer(allTimerInfo, &cnt);
    DebugPrintf("cnt=%u\r\n", cnt);
    if (!ret)
        return;
    const size_t capacity = JSON_ARRAY_SIZE(40) + JSON_OBJECT_SIZE(4) + 40 * JSON_OBJECT_SIZE(5);
    DynamicJsonDocument doc(capacity);

    doc["cmd"] = "POSTTIMER";
    doc["mac"] = strMAC.c_str();
    String t = timestamp2Str(rtc.getEpoch());
    doc["time"] = t.c_str();
    JsonArray value = doc.createNestedArray("value");
    char valueBuffer[100];
    for (size_t i = 0; i < cnt; ++i)
    {
        String time = rawTime2Str(allTimerInfo[i].time);
        String week = weekday2Str(allTimerInfo[i].weekday);
        const size_t cap = JSON_OBJECT_SIZE(5);
        DynamicJsonDocument subDoc(cap);
        snprintf(valueBuffer,
                 sizeof(valueBuffer),
                 "{\"addr\":%u,\"id\":%u,\"status\":\"%s\",\"time\":\"%s\",\"weekday\":\"%s\"}",
                 allTimerInfo[i].addr,
                 allTimerInfo[i].id,
                 allTimerInfo[i].status == 0x33 ? "close" : "open",
                 time.c_str(),
                 week.c_str());
        deserializeJson(subDoc, valueBuffer);
        value.add(subDoc);
    }
    portENTER_CRITICAL();
    deserializeJson(doc, Serial);
    portEXIT_CRITICAL();
//     size_t len = measureJson(doc);
//     DebugPrintf("postTimer len: %u\r\n", len);
//     char *tmpBuffer = (char *)pvPortMalloc(len + 10);
//     if (NULL == tmpBuffer)
//     {
//         DebugPrintln("postTimer failed to allocate memory");
//         return;
//     }
//     memset(tmpBuffer, 0, len + 10);
//     strcat(tmpBuffer, prefix);
//     serializeJson(doc, &tmpBuffer[0] + (sizeof(prefix) - 1), len + 1);
//     strcat(tmpBuffer, suffix);
//     portENTER_CRITICAL();
//     client.print(tmpBuffer);
// //    vPortFree(tmpBuffer);
//     // // 写日志到SD卡
//     // writeLog("Switch status uploaded");
//     portEXIT_CRITICAL();
}

void postrt(void)
{
    DynamicJsonDocument doc(6142);
    bool mainRes = false, subRes = false; //readTimeRes = false;
    ControllerStatusInfo mainDevInfo = {0};
    SubDeviceStatusInfo subDevInfo[16] = {0};

    mainRes = PashonController.getControllerStatus(mainDevInfo);
    if (!mainRes)
        return;
    // 只有当终端设备合闸时才读分路设备的状态
    if (1 == mainDevInfo.status)
    {
        portENTER_CRITICAL();
        PashonController.setMainSwStatus(1);
        portEXIT_CRITICAL();
        vTaskDelay(pdMS_TO_TICKS(500));
        subRes = PashonController.getAllSubDeviceStatus(subDevInfo);
    }
    else
    {
        portENTER_CRITICAL();
        PashonController.setMainSwStatus(0);
        portEXIT_CRITICAL();
    }
    doc["ret"] = "success";
    JsonObject control = doc.createNestedObject("control");
    control["cmd"] = "POSTRT";
    control["mac"] = strMAC.c_str();
    String controllerNowTime = rawTime2Str(mainDevInfo.realTime);
    control["time"] = controllerNowTime.c_str();
    JsonArray value = control.createNestedArray("value");
    JsonObject mainValue = value.createNestedObject();
    mainValue["addr"] = 0;
    mainValue["status"] = mainDevInfo.status == 1 ? "open" : "close";
    char tmp[16];
    snprintf(tmp, sizeof(tmp), "%.2f", mainDevInfo.current);
    mainValue["A"] = tmp;
    snprintf(tmp, sizeof(tmp), "%.2f", mainDevInfo.voltage);
    mainValue["V"] = tmp;
    snprintf(tmp, sizeof(tmp), "%.2f", mainDevInfo.power);
    mainValue["P"] = tmp;
    snprintf(tmp, sizeof(tmp), "%.2f", mainDevInfo.energyused);
    mainValue["E"] = tmp;
    mainValue["NetCtrl"] = mainDevInfo.netCtrl ? "true" : "false";
    String alarmInfo;
    if (NUM_GET_BIT(mainDevInfo.alarm, 0))
    {
        alarmInfo += "OVER_CUR";
    }
    if (NUM_GET_BIT(mainDevInfo.alarm, 1))
    {
        if (alarmInfo.length() == 0)
        {
            alarmInfo += "OVER_LEAK";
        }
        else
        {
            alarmInfo += "&OVER_LEAK";
        }
    }
    if (NUM_GET_BIT(mainDevInfo.alarm, 2))
    {
        if (alarmInfo.length() == 0)
        {
            alarmInfo += "LOW_VIN";
        }
        else
        {
            alarmInfo += "&LOW_VIN";
        }
    }
    if (NUM_GET_BIT(mainDevInfo.alarm, 3))
    {
        if (alarmInfo.length() == 0)
        {
            alarmInfo += "HIGH_VIN";
        }
        else
        {
            alarmInfo += "&HIGH_VIN";
        }
    }
    if (NUM_GET_BIT(mainDevInfo.alarm, 7))
    {
        if (alarmInfo.length() == 0)
        {
            alarmInfo += "SYS_OFF";
        }
        else
        {
            alarmInfo += "&SYS_OFF";
        }
    }
    mainValue["alarm"] = alarmInfo.c_str();

    if (subRes)
    {
        for (int i = 0; i < relayTotal; ++i)
        {
            if (subDevInfo[i].rateCurrent != 0)
            {
                JsonObject subSubValue = value.createNestedObject();
                subSubValue["addr"] = i + 1;
                subSubValue["status"] = subDevInfo[i].status == 1 ? "open" : "close";
                char tmp[16];
                snprintf(tmp, sizeof(tmp), "%.2f", subDevInfo[i].current);
                subSubValue["A"] = tmp;
                subSubValue["V"] = "";
                subSubValue["P"] = "";
                subSubValue["E"] = "";
                subSubValue["NetCtrl"] = subDevInfo[i].netCtrl ? "true" : "false";
            }
        }
    }
    else
    {
        for (int i = 1; i <= relayTotal; ++i)
        {
            JsonObject subSubValue = value.createNestedObject();
            subSubValue["addr"] = i;
            subSubValue["status"] = "close";
            subSubValue["A"] = "";
            subSubValue["V"] = "";
            subSubValue["P"] = "";
            subSubValue["E"] = "";
            subSubValue["NetCtrl"] = "true";
        }
    }

    size_t len = measureJson(doc);
    //DebugPrintf("postrt len: %u\r\n", len);
    char *tmpBuffer = (char *)pvPortMalloc(len + 10);
    if (NULL == tmpBuffer)
    {
        DebugPrintln("postrt failed to allocate memory");
        return;
    }
    memset(tmpBuffer, 0, len + 10);
    strcat(tmpBuffer, prefix);
    serializeJson(doc, &tmpBuffer[0] + strlen(prefix), len + 1);
    strcat(tmpBuffer, suffix);
    portENTER_CRITICAL();
    client.print(tmpBuffer);
    vPortFree(tmpBuffer);
    // 写日志到SD卡
    //writeLog("Switch status uploaded");
    portEXIT_CRITICAL();
}

CmdTable CMDS[] = {
    {"SWITCH", operateDevice},
    {"UPDATETIMER", updateTimer},
    {"DELTIMER", deleteTimer},
    {"SYNCTIME", syncTime}};

int cmdPreParse(const String &str)
{
    StaticJsonDocument<300> doc;

    DeserializationError err = deserializeJson(doc, str);
    if (err)
    {
        DebugPrintln(err.c_str());
        return -1;
    }

    const char *mac = doc["mac"];
    if (strcmp(mac, strMAC.c_str()) != 0)
    {
        DebugPrintln("MAC address error");
        return -2;
    }

    if (!doc.containsKey("cmd"))
    {
        DebugPrintln("cmd error");
        return -3;
    }

    const char *cmd = doc["cmd"];
    if (!doc.containsKey("value"))
    {
        DebugPrintln("value error");
        return -4;
    }
    JsonVariant value = doc["value"];
    for (uint32_t i = 0; i < ARRAYNUM(CMDS); ++i)
    {
        if (strcmp(cmd, CMDS[i].cmd) == 0)
        {
            if (CMDS[i].handle(value) < 0)
            {
                return -5;
            }
            else
            {
                return 0;
            }
        }
    }
    return -6;
}

void pushAlarmInfo(void)
{
    AlarmInfo info;

    if (xQueueReceive(xAlarmInfoQueue, &info, 0) == pdTRUE)
    {
        DynamicJsonDocument doc(300);

        doc["ret"] = "success";
        JsonObject control = doc.createNestedObject("control");
        control["cmd"] = "POSTALARM";
        control["mac"] = strMAC.c_str();
        String t = timestamp2Str(rtc.getEpoch());
        control["time"] = t.c_str();

        JsonObject value = control.createNestedObject("value");
        if (info.alarmType == OPEN)
        {
            value["type"] = "open";
        }
        else if (info.alarmType == CLOSE)
        {
            value["type"] = "close";
        }
        else if (info.alarmType == INPUT_OFF)
        {
            value["type"] = "inputoff";
        }
        else if (info.alarmType == OVER_CUR)
        {
            value["type"] = "overcurrent";
        }
        else if (info.alarmType == OVER_LEAK)
        {
            value["type"] = "leakage";
        }
        else if (info.alarmType == LOW_VIN)
        {
            value["type"] = "undervoltage";
        }
        else
        {
            value["type"] = "overvoltage";
        }
        // value["threshold"] = info.threshold;
        // value["trigger"] = info.trigger;

        size_t len = measureJson(doc);
        DebugPrintf("pushAlarmInfo len: %u\r\n", len);
        char *tmpBuffer = (char *)pvPortMalloc(len + 10);
        if (NULL == tmpBuffer)
        {
            DebugPrintln("pushAlarmInfo failed to allocate memory");
            return;
        }
        memset(tmpBuffer, 0, len + 10);
        strcat(tmpBuffer, prefix);
        serializeJson(doc, &tmpBuffer[0] + (sizeof(prefix) - 1), len + 1);
        strcat(tmpBuffer, suffix);

        portENTER_CRITICAL();
        client.print(tmpBuffer);
        vPortFree(tmpBuffer);
        portEXIT_CRITICAL();
    }
}