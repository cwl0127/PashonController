#include "PashonController.h"
#include "CfgParameterDefs.h"
#include <EEPROM.h>

const int usrInfoEepAddr = 0;
uint16_t relayTotal = 0;

#define GetPrgVerEvent (1 << 0)
#define GetAutoEnEvent (1 << 1)
#define GetMainOpStaEvent (1 << 2)
#define GetCurFaultInfoEvent (1 << 3)
#define GetNextFaultInfoEvent (1 << 4)
#define GetNextFaultValEvent (1 << 5)
#define GetCurrentEvent (1 << 6)
#define GetLeakageCurrentEvent  (1 << 7)
#define GetVoltageEvent (1 << 8)
#define GetActPowerEvent (1 << 9)
#define GetPowerFactorEvent (1 << 10)
#define GetPowerConsumptionEvent (1 << 11)
#define GetTempEvent (1 << 12)
#define GetSmokeEvent (1 << 13)
#define GetRateCurrentEvent (1 << 14)
#define GetLeakageLimitValEvent (1 << 15)
#define GetUnderVolLimitEvent (1 << 16)
#define GetOverVolLimitEvent (1 << 17)
#define GetTimeEvent (1 << 18)
#define GetTiming1Event (1 << 19)
#define GetSubTimingEvent (1 << 20)
#define GetMainStaEvent (1 << 21)
#define GetSubInfoEvent (1 << 22)

EventGroupHandle_t xDataReceivedEventGroup = NULL;
xQueueHandle xAlarmInfoQueue = NULL;
PashonControllerClass PashonController(&conn);

uint8_t PashonControllerClass::usrNum[6] = {0};     //{0x53, 0x45, 0x4E, 0x30, 0x30, 0x31};
uint8_t PashonControllerClass::factoryNum[4] = {0}; //{0x14, 0x71, 0xC0, 0x03};

void PashonControllerClass::begin()
{
    ConfInfo confInfo;
    EEPROM.get(usrInfoEepAddr, confInfo);
    if (confInfo.isConfigured == 0xAA)
    {
        memcpy(PashonControllerClass::usrNum, confInfo.usrId.data, 6);
        DebugPrintln("usrId:");
        for (size_t i = 0; i < 6; ++i)
        {
            DebugPrintf("%02X ", PashonControllerClass::usrNum[i]);
        }
        DebugPrintln("");
        memcpy(PashonControllerClass::factoryNum, confInfo.factoryId.data, 4);
        DebugPrintln("factoryId:");
        for (size_t i = 0; i < 4; ++i)
        {
            DebugPrintf("%02X ", PashonControllerClass::factoryNum[i]);
        }
        DebugPrintln("");
    }
    else
    {
    }
    this->_pConn->begin();
}

void PashonControllerClass::loopReceiveData()
{
    vTaskSuspendAll();
    int len = this->_pConn->receive(this->rcvBuffer, RCV_BUF_SIZE);
    xTaskResumeAll();
    if (len >= 18) //一帧数据至少18Byte
    {
        // Serial.println("Received data:");
        // for (int i = 0; i < len; ++i)
        // {
        //     Serial.print(this->rcvBuffer[i], HEX);
        //     Serial.print(' ');
        // }
        // Serial.println("");
        this->analysisReceivedData(len);
    }
}

void PashonControllerClass::loopCheckConnection()
{
    this->_pConn->doLoop();
}

bool PashonControllerClass::getProgramVersion(uint16_t *val)
{
    this->readCmd(PRG_VER_ADDR, 1);
    /* 等待应答数据，最大等待1200ms */
    EventBits_t uxBit = xEventGroupWaitBits(xDataReceivedEventGroup, GetPrgVerEvent, pdTRUE, pdTRUE, pdMS_TO_TICKS(1200));   //GetPrgVerEvent
    if ((uxBit & GetPrgVerEvent) != GetPrgVerEvent)
        return false;

    if (rcvDataInfo.regAddress != PRG_VER_ADDR)
        return false;

    *val = BUILD_UINT16(rcvDataInfo.buf[1], rcvDataInfo.buf[0]);
    return true;
}

bool PashonControllerClass::setAutoSwitchOn(bool en)
{
    uint8_t data[2] = {0};
    if (en)
    {
        data[1] = 0x01; //使能自动合闸
    }
    else
    {
        data[1] = 0; //失能自动合闸
    }
    return this->writeCmd(AUTO_EN_ADDR, data, 2);
}

bool PashonControllerClass::getAutoSwitchStatus(uint16_t *val)
{
    this->readCmd(AUTO_EN_ADDR, 1);

    /* 等待应答数据，最大等待1200ms */
    EventBits_t uxBit = xEventGroupWaitBits(xDataReceivedEventGroup, GetAutoEnEvent, pdTRUE, pdTRUE, pdMS_TO_TICKS(1200));   //GetAutoEnEvent
    if ((uxBit & GetAutoEnEvent) != GetAutoEnEvent)
        return false;

    if (rcvDataInfo.regAddress != AUTO_EN_ADDR)
        return false;

    *val = BUILD_UINT16(rcvDataInfo.buf[1], rcvDataInfo.buf[0]);
    return true;
}

bool PashonControllerClass::getSwitchStatus(uint16_t *val)
{
    this->readCmd(MAIN_OP_ADDR, 1);

    /* 等待应答数据，最大等待1200ms */
    EventBits_t uxBit = xEventGroupWaitBits(xDataReceivedEventGroup, GetMainOpStaEvent, pdTRUE, pdTRUE, pdMS_TO_TICKS(1200));   //GetMainOpStaEvent
    if ((uxBit & GetMainOpStaEvent) != GetMainOpStaEvent)
        return false;

    if (rcvDataInfo.regAddress != MAIN_OP_ADDR)
        return false;

    *val = BUILD_UINT16(rcvDataInfo.buf[1], rcvDataInfo.buf[0]);
    return true;
}

bool PashonControllerClass::operateController(OperateMode mode)
{
    uint8_t cmd[2] = {0x01};
    if (mode == SWITCH_ON)
    {
        cmd[1] = 0x11;
        return this->writeCmd(MAIN_OP_ADDR, cmd, 2);
    }
    else if (mode == SWITCH_OFF)
    {
        cmd[1] = 0x22;
        return this->writeCmd(MAIN_OP_ADDR, cmd, 2);
    }
    else if (mode == REBOOT)
    {
        cmd[1] = 0xAF;
        return this->writeCmd(MAIN_OP_ADDR, cmd, 2);
    }
    else if (mode == CLEAR_FAULT_DATA)
    {
        cmd[1] = 0xEE;
        return this->writeCmd(MAIN_OP_ADDR, cmd, 2);
    }
    else
    {
        return false;
    }
    
}

bool PashonControllerClass::getCurFaultInfo(uint16_t *val)
{
    this->readCmd(CUR_FAULT_INFO_ADDR, 1);

    /* 等待应答数据，最大等待1200ms */
    EventBits_t uxBit = xEventGroupWaitBits(xDataReceivedEventGroup, GetCurFaultInfoEvent, pdTRUE, pdTRUE, pdMS_TO_TICKS(1200));   //GetCurFaultInfoEvent
    if ((uxBit & GetCurFaultInfoEvent) != GetCurFaultInfoEvent)
        return false;

    if (rcvDataInfo.regAddress != MAIN_OP_ADDR)
        return false;

    *val = BUILD_UINT16(rcvDataInfo.buf[1], rcvDataInfo.buf[0]);
    return true;
}

bool PashonControllerClass::getNextFaultInfo(uint16_t *val)
{
    this->readCmd(NEXT_FAULT_INFO_ADDR, 1);

    /* 等待应答数据，最大等待1200ms */
    EventBits_t uxBit = xEventGroupWaitBits(xDataReceivedEventGroup, GetNextFaultInfoEvent, pdTRUE, pdTRUE, pdMS_TO_TICKS(1200));   //GetNextFaultInfoEvent
    if ((uxBit & GetNextFaultInfoEvent) != GetNextFaultInfoEvent)
        return false;

    if (rcvDataInfo.regAddress != NEXT_FAULT_INFO_ADDR)
        return false;

    *val = BUILD_UINT16(rcvDataInfo.buf[1], rcvDataInfo.buf[0]);
    return true;
}

bool PashonControllerClass::getFaultInfoValue(uint16_t *val)
{
    this->readCmd(NEXT_FAULT_VAL_ADDR, 1);

    /* 等待应答数据，最大等待1200ms */
    EventBits_t uxBit = xEventGroupWaitBits(xDataReceivedEventGroup, GetNextFaultValEvent, pdTRUE, pdTRUE, pdMS_TO_TICKS(1200));   //GetNextFaultValEvent
    if ((uxBit & GetNextFaultValEvent) != GetNextFaultValEvent)
        return false;

    if (rcvDataInfo.regAddress != NEXT_FAULT_VAL_ADDR)
        return false;

    *val = BUILD_UINT16(rcvDataInfo.buf[1], rcvDataInfo.buf[0]);
    return true;
}

bool PashonControllerClass::getCurrent(float *val)
{
    this->readCmd(CURRENT_ADDR, 1);

    /* 等待应答数据，最大等待1200ms */
    EventBits_t uxBit = xEventGroupWaitBits(xDataReceivedEventGroup, GetCurrentEvent, pdTRUE, pdTRUE, pdMS_TO_TICKS(1200));   //GetCurrentEvent
    if ((uxBit & GetCurrentEvent) != GetCurrentEvent)
        return false;

    if (rcvDataInfo.regAddress != CURRENT_ADDR)
        return false;

    *val = BUILD_UINT16(rcvDataInfo.buf[1], rcvDataInfo.buf[0]) / 10.0;
    return true;
}

bool PashonControllerClass::getLeakageCurrent(uint16_t *val)
{
    this->readCmd(LEAKAGE_CURRENT_ADDR, 1);

    /* 等待应答数据，最大等待1200ms */
    EventBits_t uxBit = xEventGroupWaitBits(xDataReceivedEventGroup, GetLeakageCurrentEvent, pdTRUE, pdTRUE, pdMS_TO_TICKS(1200));   //GetLeakageCurrentEvent
    if ((uxBit & GetLeakageCurrentEvent) != GetLeakageCurrentEvent)
        return false;

    if (rcvDataInfo.regAddress != LEAKAGE_CURRENT_ADDR)
        return false;

    *val = BUILD_UINT16(rcvDataInfo.buf[1], rcvDataInfo.buf[0]);
    return true;
}

bool PashonControllerClass::getVoltage(float *val)
{
    this->readCmd(VOLTAGE_ADDR, 1);

    /* 等待应答数据，最大等待1200ms */
    EventBits_t uxBit = xEventGroupWaitBits(xDataReceivedEventGroup, GetVoltageEvent, pdTRUE, pdTRUE, pdMS_TO_TICKS(1200));   //GetVoltageEvent
    if ((uxBit & GetVoltageEvent) != GetVoltageEvent)
        return false;

    if (rcvDataInfo.regAddress != VOLTAGE_ADDR)
        return false;

    *val = BUILD_UINT16(rcvDataInfo.buf[1], rcvDataInfo.buf[0]) / 100.0;
    return true;
}

bool PashonControllerClass::getActivePower(uint16_t *val)
{
    this->readCmd(ACT_POWER_ADDR, 1);

    /* 等待应答数据，最大等待1200ms */
    EventBits_t uxBit = xEventGroupWaitBits(xDataReceivedEventGroup, GetActPowerEvent, pdTRUE, pdTRUE, pdMS_TO_TICKS(1200));   //GetActPowerEvent
    if ((uxBit & GetActPowerEvent) != GetActPowerEvent)
        return false;

    if (rcvDataInfo.regAddress != ACT_POWER_ADDR)
        return false;

    *val = BUILD_UINT16(rcvDataInfo.buf[1], rcvDataInfo.buf[0]);
    return true;
}

bool PashonControllerClass::getPowerFactor(uint16_t *val)
{
    this->readCmd(POWER_FACTOR_ADDR, 1);

    /* 等待应答数据，最大等待1200ms */
    EventBits_t uxBit = xEventGroupWaitBits(xDataReceivedEventGroup, GetPowerFactorEvent, pdTRUE, pdTRUE, pdMS_TO_TICKS(1200));   //GetPowerFactorEvent
    if ((uxBit & GetPowerFactorEvent) != GetPowerFactorEvent)
        return false;

    if (rcvDataInfo.regAddress != POWER_FACTOR_ADDR)
        return false;

    *val = BUILD_UINT16(rcvDataInfo.buf[1], rcvDataInfo.buf[0]) / 100;
    return true;
}

bool PashonControllerClass::getPowerconsumption(float *val) //月用电量
{
    this->readCmd(POWER_CONSUMPTION_ADDR, 1);

    /* 等待应答数据，最大等待1200ms */
    EventBits_t uxBit = xEventGroupWaitBits(xDataReceivedEventGroup, GetPowerConsumptionEvent, pdTRUE, pdTRUE, pdMS_TO_TICKS(1200));   //GetPowerConsumptionEvent
    if ((uxBit & GetPowerConsumptionEvent) != GetPowerConsumptionEvent)
        return false;

    if (rcvDataInfo.regAddress != POWER_CONSUMPTION_ADDR)
        return false;

    *val = BUILD_UINT16(rcvDataInfo.buf[1], rcvDataInfo.buf[0]) / 10.0;
    return true;
}

bool PashonControllerClass::getTemperature(float *val)
{
    this->readCmd(TEMP_ADDR, 1);

    /* 等待应答数据，最大等待1200ms */
    EventBits_t uxBit = xEventGroupWaitBits(xDataReceivedEventGroup, GetTempEvent, pdTRUE, pdTRUE, pdMS_TO_TICKS(1200));   //GetTempEvent
    if ((uxBit & GetTempEvent) != GetTempEvent)
        return false;

    if (rcvDataInfo.regAddress != TEMP_ADDR)
        return false;

    *val = BUILD_UINT16(rcvDataInfo.buf[1], rcvDataInfo.buf[0]) / 10.0;
    return true;
}

bool PashonControllerClass::hasSmoke()
{
    this->readCmd(SMOKE_ADDR, 1);

    /* 等待应答数据，最大等待1200ms */
    EventBits_t uxBit = xEventGroupWaitBits(xDataReceivedEventGroup, GetSmokeEvent, pdTRUE, pdTRUE, pdMS_TO_TICKS(1200));   //GetSmokeEvent
    if ((uxBit & GetSmokeEvent) != GetSmokeEvent)
        return false;

    if (rcvDataInfo.regAddress != SMOKE_ADDR)
        return false;

    uint16_t val = BUILD_UINT16(rcvDataInfo.buf[1], rcvDataInfo.buf[0]);
    return 0x0011 == val;
}

bool PashonControllerClass::getRatedCurrent(float *val)
{
    this->readCmd(RATE_CURRENT_ADDR, 1);

    /* 等待应答数据，最大等待1200ms */
    EventBits_t uxBit = xEventGroupWaitBits(xDataReceivedEventGroup, GetRateCurrentEvent, pdTRUE, pdTRUE, pdMS_TO_TICKS(1200));   //GetRateCurrentEvent
    if ((uxBit & GetRateCurrentEvent) != GetRateCurrentEvent)
        return false;

    if (rcvDataInfo.regAddress != RATE_CURRENT_ADDR)
        return false;

    *val = BUILD_UINT16(rcvDataInfo.buf[1], rcvDataInfo.buf[0]) / 1000.0;
    return true;
}

bool PashonControllerClass::setRatedCurrent(uint16_t cur)
{
    uint8_t current[2];

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    current[0] = HI_UINT16(cur);
    current[1] = LO_UINT16(cur);
#else
    current[1] = HI_UINT16(cur);
    current[0] = LO_UINT16(cur);
#endif
    return this->writeCmd(RATE_CURRENT_ADDR, current, 2);
}

bool PashonControllerClass::getLeakageCurrentProtectionVal(uint16_t *val)
{
    this->readCmd(LEAKAGE_LIMIT_VAL_ADDR, 1);

    /* 等待应答数据，最大等待1200ms */
    EventBits_t uxBit = xEventGroupWaitBits(xDataReceivedEventGroup, GetLeakageLimitValEvent, pdTRUE, pdTRUE, pdMS_TO_TICKS(1200));   //GetLeakageLimitValEvent
    if ((uxBit & GetLeakageLimitValEvent) != GetLeakageLimitValEvent)
        return false;

    if (rcvDataInfo.regAddress != LEAKAGE_LIMIT_VAL_ADDR)
        return false;

    *val = BUILD_UINT16(rcvDataInfo.buf[1], rcvDataInfo.buf[0]);
    return true;
}

bool PashonControllerClass::setLeakageCurrentProtectionVal(uint16_t val)
{
    uint8_t leakageCurrent[2];

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    leakageCurrent[0] = HI_UINT16(val);
    leakageCurrent[1] = LO_UINT16(val);
#else
    leakageCurrent[1] = HI_UINT16(val);
    leakageCurrent[0] = LO_UINT16(val);
#endif
    return this->writeCmd(LEAKAGE_LIMIT_VAL_ADDR, leakageCurrent, 2);
}

bool PashonControllerClass::getUnderVoltageLimit(float *val)
{
    this->readCmd(UNDER_VOL_LIMIT_ADDR, 1);

    /* 等待应答数据，最大等待1200ms */
    EventBits_t uxBit = xEventGroupWaitBits(xDataReceivedEventGroup, GetUnderVolLimitEvent, pdTRUE, pdTRUE, pdMS_TO_TICKS(1200));   //GetUnderVolLimitEvent
    if ((uxBit & GetUnderVolLimitEvent) != GetUnderVolLimitEvent)
        return false;

    if (rcvDataInfo.regAddress != UNDER_VOL_LIMIT_ADDR)
        return false;

    *val = BUILD_UINT16(rcvDataInfo.buf[1], rcvDataInfo.buf[0]) / 100.0;
    return true;
}

bool PashonControllerClass::setUnderVoltageLimit(uint16_t val)
{
    uint8_t UnderVoltage[2];

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    UnderVoltage[0] = HI_UINT16(val);
    UnderVoltage[1] = LO_UINT16(val);
#else
    UnderVoltage[1] = HI_UINT16(val);
    UnderVoltage[0] = LO_UINT16(val);
#endif
    return this->writeCmd(LEAKAGE_LIMIT_VAL_ADDR, UnderVoltage, 2);
}

bool PashonControllerClass::getOverVoltageLimit(float *val)
{
    this->readCmd(OVER_VOL_LIMIT_ADDR, 1);

    /* 等待应答数据，最大等待1200ms */
    EventBits_t uxBit = xEventGroupWaitBits(xDataReceivedEventGroup, GetOverVolLimitEvent, pdTRUE, pdTRUE, pdMS_TO_TICKS(1200));   //GetOverVolLimitEvent
    if ((uxBit & GetOverVolLimitEvent) != GetOverVolLimitEvent)
        return false;

    if (rcvDataInfo.regAddress != OVER_VOL_LIMIT_ADDR)
        return false;

    *val = BUILD_UINT16(rcvDataInfo.buf[1], rcvDataInfo.buf[0]) / 100.0;
    return true;
}

bool PashonControllerClass::setOverVoltageLimit(uint16_t val)
{
    uint8_t OverVoltage[2];

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    OverVoltage[0] = HI_UINT16(val);
    OverVoltage[1] = LO_UINT16(val);
#else
    OverVoltage[1] = HI_UINT16(val);
    OverVoltage[0] = LO_UINT16(val);
#endif
    return this->writeCmd(LEAKAGE_LIMIT_VAL_ADDR, OverVoltage, 2);
}

bool PashonControllerClass::setDeviceTime(uint32_t t)
{
    uint8_t timeBuf[6];
    tmElements_t xTime;

    breakTime(t, xTime);                    //Year是自1970年以来的时间
    timeBuf[5] = (xTime.Year + 1970) % 100; //只取个位和10位
    timeBuf[4] = xTime.Month;
    timeBuf[3] = xTime.Day;
    timeBuf[2] = xTime.Hour;
    timeBuf[1] = xTime.Minute;
    timeBuf[0] = xTime.Second;
    return this->writeCmd(TIME_ADDR, timeBuf, 6);
}

bool PashonControllerClass::getCurrentTime(uint32_t *t)
{
    this->readCmd(TIME_ADDR, 1);

    /* 等待应答数据，最大等待1200ms */
    EventBits_t uxBit = xEventGroupWaitBits(xDataReceivedEventGroup, GetTimeEvent, pdTRUE, pdTRUE, pdMS_TO_TICKS(1200));   //GetTimeEvent
    if ((uxBit & GetTimeEvent) != GetTimeEvent)
        return false;

    if (rcvDataInfo.regAddress != TIME_ADDR)
        return false;

    tmElements_t xTime;
    xTime.Second = rcvDataInfo.buf[0];
    xTime.Minute = rcvDataInfo.buf[1];
    xTime.Hour = rcvDataInfo.buf[2];
    xTime.Day = rcvDataInfo.buf[3];
    xTime.Month = rcvDataInfo.buf[4];
    xTime.Year = rcvDataInfo.buf[5] + 30; //原始时间为相对于2000年以来的时间，转换为相对于1970年以来的时间
    *t = makeTime(xTime);

    return true;
}

bool PashonControllerClass::setControllerTimer(TimerInfo &t)
{
    uint8_t cmdBuf[12];
    cmdBuf[0] = t.id;
    cmdBuf[1] = t.type;
    cmdBuf[2] = t.loopType;
    cmdBuf[3] = t.used;
    cmdBuf[4] = t.weekday;
    memcpy(&cmdBuf[5], t.time, 6);
    cmdBuf[11] = t.status;

    uint8_t regAddr = 0xFF;
    if (0 == t.addr)
    {
        regAddr = TIMING1_ADDR + t.id - 1;
    }
    else if (t.addr > 0 && t.addr < 5)
    {
        regAddr = SUB_DEV1_1_TIMING1_ADDR + 2 * t.addr + t.id - 3;
    }
    else if (t.addr > 4 && t.addr < 9)
    {
        uint8_t tmp = t.addr - 4;
        regAddr = SUB_DEV2_1_TIMING1_ADDR + 2 * tmp + t.id - 3;
    }
    else if (t.addr > 8 && t.addr < 13)
    {
        uint8_t tmp = t.addr - 8;
        regAddr = SUB_DEV3_1_TIMING1_ADDR + 2 * tmp + t.id - 3;
    }
    else if (t.addr > 12 && t.addr < 17)
    {
        uint8_t tmp = t.addr - 12;
        regAddr = SUB_DEV4_1_TIMING1_ADDR + 2 * tmp + t.id - 3;
    }
    return this->writeCmd(regAddr, cmdBuf, 12);
}
/*
bool PashonControllerClass::getControllerTimer(TimerInfo &t, uint8_t addr, uint8_t id)
{
    uint8_t regAddr = 0xFF;

    if ((addr > 16) || 
        (addr == 0 && id > 8 ) || 
        (addr > 0 && id > 2))
    {
        return false;
    }

    if (0 == addr)
    {
        regAddr = TIMING1_ADDR;
        this->readCmd(regAddr, 8);
    }
    else if (addr > 0 && addr < 5)
    {
        regAddr = SUB_DEV1_ALL_TIMING_ADDR;
        this->readCmd(regAddr, 0x0011);
    }
    else if (addr > 4 && addr < 9)
    {
        regAddr = SUB_DEV2_ALL_TIMING_ADDR;
        this->readCmd(regAddr, 0x0011);
    }
    else if (addr > 8 && addr < 13)
    {
        regAddr = SUB_DEV3_ALL_TIMING_ADDR;
        this->readCmd(regAddr, 0x0011);
    }
    else if (addr > 12 && addr < 17)
    {
        regAddr = SUB_DEV4_ALL_TIMING_ADDR;
        this->readCmd(regAddr, 0x0011);
    }
    else
    {
        return false;
    }
    
    // 等待应答数据，最大等待1200ms
    if (xSemaphoreTake(xRcvBinarySem, pdMS_TO_TICKS(300)) != pdTRUE)
        return false;
    if (rcvDataInfo.regAddress != regAddr)
        return false;
    t.addr = addr;
    t.id = id;
    uint8_t *pData = nullptr;
    if (0 == addr)
    {
        pData = &rcvDataInfo.buf[12 * (id -1)];
    }
    else
    {
        if (addr % 4 == 0)
        {
            pData = &rcvDataInfo.buf[12 * (5 + id)];
        }
        else
        {
            pData = &rcvDataInfo.buf[12 * (2 * (addr % 4 -1) + id - 1)];
        }
        
    }
    t.type = pData[1];
    t.loopType = pData[2];
    t.used = pData[3];
    t.weekday = pData[4];
    memcpy(t.time, &pData[5], 6);
    t.status = pData[11];
    return true;
}
*/

bool PashonControllerClass::getControllerTimer(TimerInfo *t, size_t *cnt)
{
    //读取总路的8个定时信息
    this->readCmd(TIMING1_ADDR, 8);
    // 等待应答数据，最大等待1200ms
    EventBits_t uxBit = xEventGroupWaitBits(xDataReceivedEventGroup, GetTiming1Event, pdTRUE, pdTRUE, pdMS_TO_TICKS(1200));   //GetTiming1Event
    if ((uxBit & GetTiming1Event) != GetTiming1Event)
    {
        DebugPrintf("Main no timing data return\r\n");
        return false;
    }
 
    if (rcvDataInfo.regAddress != TIMING1_ADDR || rcvDataInfo.dataLen != 96)
    {
        DebugPrintf("Main return timing data error\r\n");
        return false;
    }

    uint8_t *pData = nullptr;
    size_t j = 0;
    for (size_t i = 0; i < 8; ++i)
    {
        pData = &rcvDataInfo.buf[12 * i];
        if (pData[3] == 1) //pData[3] = 1,启用该路定时，= 0，停用该路定时
        {
            t[j].addr = 0;
            t[j].id = pData[0];
            t[j].type = pData[1];
            t[j].loopType = pData[2];
            t[j].used = pData[3];
            t[j].weekday = pData[4];
            memcpy(&t[j].time, &pData[5], 6);
            t[j].status = pData[11];
            j++;
        }
    }
    vTaskDelay(pdMS_TO_TICKS(500));
    //依次读取分路的所有定时信息
    for (int n = 0; n < 4; ++n)
    {
        DebugPrintf("Read regAddr: %u\r\n", SUB_DEV1_ALL_TIMING_ADDR + 19 * n);
        this->readCmd(SUB_DEV1_ALL_TIMING_ADDR + 19 * n, 0x0011);
        // 等待应答数据，最大等待1200ms
        EventBits_t uxBit = xEventGroupWaitBits(xDataReceivedEventGroup, GetSubTimingEvent, pdTRUE, pdTRUE, pdMS_TO_TICKS(2000));   //GetSubTimingEvent
        if ((uxBit & GetSubTimingEvent) != GetSubTimingEvent)
        {
            DebugPrintf("Sub no timing data return\r\n");
            continue;
        }

        if (rcvDataInfo.regAddress != (SUB_DEV1_ALL_TIMING_ADDR + 19 * n) || rcvDataInfo.dataLen != 96)
        {
            DebugPrintf("Sub return timing data error\r\n");
            continue;
        }
        for (int i = 0; i < 8; ++i)
        {
            pData = &rcvDataInfo.buf[12 * i];
            if (pData[3] == 1) //pData[3] = 1,启用该路定时，= 0，停用该路定时
            {
                uint8_t m = 0;
                if (i < 2)
                    m = 1;
                else if (i >= 2 && i < 4)
                    m = 2;
                else if (i >= 4 && i < 6)
                    m = 3;
                else if (i >= 6)
                    m = 4;
                t[j].addr = 4 * n + m;
                t[j].id = (pData[0] % 2 == 0 ? 2 : 1);
                t[j].type = pData[1];
                t[j].loopType = pData[2];
                t[j].used = pData[3];
                t[j].weekday = pData[4];
                memcpy(&t[j].time, &pData[5], 6);
                t[j].status = pData[11];
                j++;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    *cnt = j;
    return true;
}

bool PashonControllerClass::clearControllerTiming()
{
    uint8_t cmd[2] = {0x00, 0x22};

    return this->writeCmd(CLEAR_ALL_TIMING_ADDR, cmd, 2);
}

bool PashonControllerClass::getControllerStatus(ControllerStatusInfo &devStatus)
{
//    DebugPrintln("Send command to get main status");
    this->readCmd(MAIN_OP_ADDR, 23);

    /* 等待应答数据，最大等待1200ms */
    EventBits_t uxBit = xEventGroupWaitBits(xDataReceivedEventGroup, GetMainStaEvent, pdTRUE, pdTRUE, pdMS_TO_TICKS(1200));   //GetMainStaEvent
    if ((uxBit & GetMainStaEvent) != GetMainStaEvent)
    {
        DebugPrintln("main timeout");
        return false;
    }

//    DebugPrintln("main semaphore obtained");
    if (rcvDataInfo.regAddress != MAIN_OP_ADDR || rcvDataInfo.dataLen != 50)
    {
        DebugPrintln("Data error");
        DebugPrintln("regAddress: " + String(rcvDataInfo.regAddress));
        DebugPrintln("dataLen: " + String(rcvDataInfo.dataLen));
        return false;
    }

    uint16_t sta = BUILD_UINT16(rcvDataInfo.buf[1], rcvDataInfo.buf[0]);
    devStatus.status = sta == 0x0111 ? 1 : 0;
    devStatus.current = BUILD_UINT16(rcvDataInfo.buf[9], rcvDataInfo.buf[8]) * 0.001;
    devStatus.voltage = BUILD_UINT16(rcvDataInfo.buf[13], rcvDataInfo.buf[12]) * 0.01;
    devStatus.power = BUILD_UINT16(rcvDataInfo.buf[15], rcvDataInfo.buf[14]);
    devStatus.energyused = BUILD_UINT16(rcvDataInfo.buf[19], rcvDataInfo.buf[18]) * 0.1;
    devStatus.netCtrl = true;
    devStatus.alarm = BUILD_UINT16(rcvDataInfo.buf[3], rcvDataInfo.buf[2]);
    for (size_t i = 0; i < 6; ++i)
    {
        devStatus.realTime[i] = rcvDataInfo.buf[49 - i];
    }

    return true;
}

bool PashonControllerClass::operateSubDevice(int num, bool sw)
{
    uint8_t cmd[2];
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uint8_t n = static_cast<uint8_t>(num % 4);
    if (0 == n)
    {
        cmd[0] = 0x04;
    }
    else
    {
        cmd[0] = n;
    }
    cmd[1] = sw ? 0x11 : 0x22;
#else
    cmd[0] = sw ? 0x11 : 0x22;
    uint8_t n = static_cast<uint8_t>(num % 4);
    if (0 == n)
    {
        cmd[1] = 0x04;
    }
    else
    {
        cmd[1] = n;
    }
#endif
    uint8_t regAddr;
    if (num > 0 && num < 5)
    {
        regAddr = SUB_DEV1_OP_ADDR;
    }
    else if (num > 4 && num < 9)
    {
        regAddr = SUB_DEV2_OP_ADDR;
    }
    else if (num > 8 && num < 13)
    {
        regAddr = SUB_DEV3_OP_ADDR;
    }
    else if (num > 12 && num < 17)
    {
        regAddr = SUB_DEV4_OP_ADDR;
    }
    else
    {
        return false;
    }
    return this->writeCmd(regAddr, cmd, 2);
}

bool PashonControllerClass::getSubDevSwitchStatus(int num, bool *sw)
{
    SubDeviceStatusInfo devStatusInfo[16];

    if (!this->getAllSubDeviceStatus(devStatusInfo))
    {
        return false;
    }
    uint8_t sta = devStatusInfo[num - 1].status;
    *sw = (sta == 1 ? true : false);
    return true;
}

bool PashonControllerClass::getAllSubDeviceStatus(SubDeviceStatusInfo *devStatus)
{
//    DebugPrintln("Send command to get slave status");
    this->readCmd(ALL_SUB_DEV_INFO_ADDR, 17);

    /* 等待应答数据，最大等待2000ms */
    EventBits_t uxBit = xEventGroupWaitBits(xDataReceivedEventGroup, GetSubInfoEvent, pdTRUE, pdTRUE, pdMS_TO_TICKS(2000));   //GetSubInfoEvent
    if ((uxBit & GetSubInfoEvent) != GetSubInfoEvent)
    {
        DebugPrintln("sub timeout");
        return false;
    }

//    DebugPrintln("sub semaphore obtained");
    if (rcvDataInfo.regAddress != ALL_SUB_DEV_INFO_ADDR || rcvDataInfo.dataLen != 80)
    {
        DebugPrintln("Data error");
        DebugPrintln("regAddress: " + String(rcvDataInfo.regAddress));
        DebugPrintln("dataLen: " + String(rcvDataInfo.dataLen));

        return false;
    }
    for (int i = 0; i < 16; ++i)
    {
        devStatus[i].status = rcvDataInfo.buf[5 * i];                                                      //开关状态
        devStatus[i].current = BUILD_UINT16(rcvDataInfo.buf[5 * i + 2], rcvDataInfo.buf[5 * i + 1]) * 0.1; //实时电流
        devStatus[i].rateCurrent = BUILD_UINT16(rcvDataInfo.buf[5 * i + 4], rcvDataInfo.buf[5 * i + 3]);
        devStatus[i].netCtrl = true;
    }

    return true;
}

bool PashonControllerClass::readCmd(uint8_t addr, uint16_t cnt)
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
    cmdBuffer[13] = addr; //寄存器首地址
    //拷贝寄存器数量
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    cmdBuffer[14] = HI_UINT16(cnt);
    cmdBuffer[15] = LO_UINT16(cnt);
#else
    cmdBuffer[14] = LO_UINT16(cnt);
    cmdBuffer[15] = HI_UINT16(cnt);
#endif
    //计算CRC16校验值
    uint16_t crcValue = crc16Modbus(cmdBuffer, 16);
    cmdBuffer[16] = LO_UINT16(crcValue);
    cmdBuffer[17] = HI_UINT16(crcValue);
    //发送
    bool ret;
    taskENTER_CRITICAL();
    ret = this->_pConn->send(cmdBuffer, 18);
    taskEXIT_CRITICAL();
    return ret;
}

bool PashonControllerClass::writeCmd(uint8_t addr, uint8_t *buffer, uint16_t cnt)
{
    if (cnt < 2 && cnt > 12 && (cnt % 2 != 0))
    {
        DebugPrintln("data length error");
        return false;
    }
    uint8_t cmdBuffer[16 + cnt];
    //拷贝用户序列号
    memcpy(cmdBuffer, usrNum, 6);
    //拷贝帧长度
    uint16_t frameLen = 6 + cnt;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    cmdBuffer[6] = HI_UINT16(frameLen);
    cmdBuffer[7] = LO_UINT16(frameLen);
#else
    cmdBuffer[6] = LO_UINT16(frameLen);
    cmdBuffer[7] = HI_UINT16(frameLen);
#endif
    //拷贝出厂序列号
    memcpy(&cmdBuffer[8], factoryNum, 4);
    cmdBuffer[12] = 0x0B; //帧命令
    cmdBuffer[13] = addr; //寄存器地址
    memcpy(&cmdBuffer[14], buffer, cnt);
    //计算CRC16校验值
    uint16_t crcValue = crc16Modbus(cmdBuffer, 14 + cnt);

    cmdBuffer[14 + cnt] = LO_UINT16(crcValue);
    cmdBuffer[15 + cnt] = HI_UINT16(crcValue);
    //发送
    bool ret;
    portENTER_CRITICAL();
    ret = this->_pConn->send(cmdBuffer, 16 + cnt);
    portEXIT_CRITICAL();
    // DebugWrite(cmdBuffer, 16 + cnt);
    return ret;
}

bool PashonControllerClass::analysisReceivedData(int len)
{
    bool ret = false;
    uint8_t *pBuffer = this->rcvBuffer;
    int remainingLen = len;
    int ParsedDataLen = 0;

    do
    {
        //判断帧头是否时用户序列号
        if (memcmp(pBuffer, PashonControllerClass::usrNum, 6) != 0)
        {
            DebugPrintln("usrNum error");
            break;
        }
        //获取帧长度
        uint16_t frameLen = BUILD_UINT16(pBuffer[7], pBuffer[6]);
        if (remainingLen < frameLen + 10)
        {
            DebugPrintln("frameLen error");
            break;
        };

        //计算帧CRC16校验值
        uint16_t crcVal = crc16Modbus(pBuffer, frameLen + 8);
        uint16_t val = BUILD_UINT16(pBuffer[frameLen + 8], pBuffer[frameLen + 9]);

        if ((crcVal != val) ||                                                  //CRC检验
            (memcmp(&pBuffer[8], PashonControllerClass::factoryNum, 4) != 0) || //出厂序列号校验
            (pBuffer[12] != 0x0A) ||                                            //读写字节校验
            (pBuffer[13] > 122))                                                //控制器最大寄存器地址是122
        {
            this->rcvDataInfo.regAddress = 0xFF;
            this->rcvDataInfo.dataLen = 0;
            DebugPrintln("Data:");
            for (uint16_t i = 0; i < frameLen + 10; ++i)
            {
                DebugPrintf("%02X ", pBuffer[i]);
            }
            DebugPrintln("");
            DebugPrintln("crc error");
            DebugPrintf("CRC calculated value: %X, val: %X\r\n", crcVal, val);
            break;
        }
        switch (pBuffer[13])
        {
        case PRG_VER_ADDR:
        case CARD_ID_ADDR:
            break;
        case HEARTBEAT_ADDR:
        {
            portENTER_CRITICAL();
            if (pBuffer[38] == 0x11)
            {
                PashonController.setMainSwStatus(1);
            }
            else if (pBuffer[38] == 0X22)
            {
                PashonController.setMainSwStatus(0);
            }
            portEXIT_CRITICAL();
        }
        break;
        case LOCAL_OP_ALARM_ADDR:
        {
            AlarmInfo info;
            if (pBuffer[16] == 0x01 && pBuffer[17] == 0x11)
            {
                info.alarmType = CLOSE;
                info.threshold = 0.0f;
                info.trigger = 0.0f;
            }
            else
            {
                info.alarmType = OPEN;
                info.threshold = 0.0f;
                info.trigger = 0.0f;
            }
            xQueueSend(xAlarmInfoQueue, (void *)&info, (TickType_t)0);
        }
        break;
        case LOST_INPUT_ALARM_ADDR:
        {
            AlarmInfo info;
            info.alarmType = INPUT_OFF;
            info.threshold = 0.0f,
            info.trigger = 0.0f;
            xQueueSend(xAlarmInfoQueue, (void *)&info, (TickType_t)0);
        }
        break;
        case OVER_VOL_ALARM_ADDR:
        {
            AlarmInfo info;
            info.alarmType = HIGH_VIN;
            info.threshold = BUILD_UINT16(pBuffer[15], pBuffer[14]) / 100.0f;
            info.trigger = BUILD_UINT16(pBuffer[17], pBuffer[16]) / 100.0f;
            xQueueSend(xAlarmInfoQueue, (void *)&info, (TickType_t)0);
        }
        break;
        case OVER_CURRENT_ALARM_ADDR:
        {
            AlarmInfo info;
            info.alarmType = OVER_CUR;
            info.threshold = BUILD_UINT16(pBuffer[15], pBuffer[14]) * 0.001f;
            info.trigger = BUILD_UINT16(pBuffer[17], pBuffer[16]) * 0.001f;
            xQueueSend(xAlarmInfoQueue, (void *)&info, (TickType_t)0);
        }
        break;
        case LEAKAGE_ALARM_ADDR:
        {
            AlarmInfo info;
            info.alarmType = OVER_LEAK;
            info.threshold = (float)BUILD_UINT16(pBuffer[15], pBuffer[14]);
            info.trigger = (float)BUILD_UINT16(pBuffer[17], pBuffer[16]);
            xQueueSend(xAlarmInfoQueue, (void *)&info, (TickType_t)0);
        }
        break;
        case UNDER_VOL_ALARM_ADDR:
        {
            AlarmInfo info;
            info.alarmType = LOW_VIN;
            info.threshold = BUILD_UINT16(pBuffer[15], pBuffer[14]) / 100.0f;
            info.trigger = (float)BUILD_UINT16(pBuffer[17], pBuffer[16]) / 100.0f;
            xQueueSend(xAlarmInfoQueue, (void *)&info, (TickType_t)0);
        }
        break;
        default:
        {
            this->rcvDataInfo.regAddress = pBuffer[13];
            this->rcvDataInfo.dataLen = frameLen - 6; //len - 16;
            memcpy(this->rcvDataInfo.buf, &pBuffer[14], this->rcvDataInfo.dataLen);
            switch (this->rcvDataInfo.regAddress)
            {
            case PRG_VER_ADDR:
                xEventGroupSetBits(xDataReceivedEventGroup, GetPrgVerEvent);
                break;
            case AUTO_EN_ADDR:
                xEventGroupSetBits(xDataReceivedEventGroup, GetAutoEnEvent);
                break;
            case MAIN_OP_ADDR:
                if (this->rcvDataInfo.dataLen == 2)
                    xEventGroupSetBits(xDataReceivedEventGroup, GetMainOpStaEvent);
                else
                    xEventGroupSetBits(xDataReceivedEventGroup, GetMainStaEvent);
                break;
            case CUR_FAULT_INFO_ADDR:
                xEventGroupSetBits(xDataReceivedEventGroup, GetCurFaultInfoEvent);
                break;
            case NEXT_FAULT_INFO_ADDR:
                xEventGroupSetBits(xDataReceivedEventGroup, GetNextFaultInfoEvent);
                break;
            case NEXT_FAULT_VAL_ADDR:
                xEventGroupSetBits(xDataReceivedEventGroup, GetNextFaultValEvent);
                break;
            case CURRENT_ADDR:
                xEventGroupSetBits(xDataReceivedEventGroup, GetCurrentEvent);
                break;
            case LEAKAGE_LIMIT_VAL_ADDR:
                xEventGroupSetBits(xDataReceivedEventGroup, GetLeakageLimitValEvent);
                break;
            case UNDER_VOL_LIMIT_ADDR:
                xEventGroupSetBits(xDataReceivedEventGroup, GetUnderVolLimitEvent);
                break;
            case OVER_VOL_LIMIT_ADDR:
                xEventGroupSetBits(xDataReceivedEventGroup, GetOverVolLimitEvent);
                break;
            case TIME_ADDR:
                xEventGroupSetBits(xDataReceivedEventGroup, GetTimeEvent);
                break;
            case TIMING1_ADDR:
                xEventGroupSetBits(xDataReceivedEventGroup, GetTiming1Event);
                break;
            case SUB_DEV1_ALL_TIMING_ADDR:
            case SUB_DEV2_ALL_TIMING_ADDR:
            case SUB_DEV3_ALL_TIMING_ADDR:
            case SUB_DEV4_ALL_TIMING_ADDR:
                xEventGroupSetBits(xDataReceivedEventGroup, GetSubTimingEvent);
                break;
            case ALL_SUB_DEV_INFO_ADDR:
                xEventGroupSetBits(xDataReceivedEventGroup, GetSubInfoEvent);
                break;
            default:
                break;
            }
            ret = true;
        }
        break;
        }
        ParsedDataLen += (frameLen + 10);
        remainingLen = len - ParsedDataLen;
        if (remainingLen > 0)
            pBuffer = &(this->rcvBuffer[ParsedDataLen]);
    } while (remainingLen > 0);

    return ret;
}
