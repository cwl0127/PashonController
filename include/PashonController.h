#ifndef __PASHON_CONTROLLER
#define __PASHON_CONTROLLER

#include "Connection.h"
#include "defs.h"
#include "CrcModbus.h"
#include "usrconfig.h"
#include "TimeLib.h"
#include <stdint.h>
#include <string.h>
#include <WSerial.h>
#include <FreeRTOS.h>
#include <event_groups.h>
#include <semphr.h>
#include <queue.h>

#define RCV_BUF_SIZE 512
#define DATA_MAX_LEN 128

enum OperateMode
{
    SWITCH_ON = (uint16_t)0x0111,
    SWITCH_OFF = (uint16_t)0x0122,
    REBOOT = (uint16_t)0x01AF,
    CLEAR_FAULT_DATA = (uint16_t)0x01EE,
    UNKNOWN_MODE = (uint16_t)0xFFFF
};

enum FaultType
{
    OVER_CUR,
    OVER_LEAK,
    LOW_VIN,
    HIGH_VIN,
    SYS_OFF,
    OPEN,
    CLOSE,
    INPUT_OFF,
    UNKNOWN_FAULE = 0xFF
};


enum RegAddress
{
    PRG_VER_ADDR = (uint8_t)0,
    AUTO_EN_ADDR = (uint8_t)2,
    MAIN_OP_ADDR = (uint8_t)3,
    CUR_FAULT_INFO_ADDR = (uint8_t)4,
    NEXT_FAULT_INFO_ADDR = (uint8_t)5,
    NEXT_FAULT_VAL_ADDR = (uint8_t)6,
    CURRENT_ADDR = (uint8_t)7,
    LEAKAGE_CURRENT_ADDR = (uint8_t)8,
    VOLTAGE_ADDR = (uint8_t)9,
    ACT_POWER_ADDR = (uint8_t)10,
    POWER_FACTOR_ADDR = (uint8_t)11,
    POWER_CONSUMPTION_ADDR = (uint8_t)12,
    TEMP_ADDR = (uint8_t)13,
    SMOKE_ADDR = (uint8_t)14,
    LIGHTING_STRICK_CNT_ADDR = (uint8_t)15,
    OVERCURRENT_CNT_ADDR = (uint8_t)16,
    LEAKEGE_CNT_ADDR = (uint8_t)17,
    UNDER_VOL_ADDR = (uint8_t)18,
    OVER_VOL_ADDR = (uint8_t)19,
    OUTAGES_CNT_ADDR = (uint8_t)20, //断电次数
    RATE_CURRENT_ADDR = (uint8_t)21,
    LEAKAGE_LIMIT_VAL_ADDR = (uint8_t)22,
    UNDER_VOL_LIMIT_ADDR = (uint8_t)23,
    OVER_VOL_LIMIT_ADDR = (uint8_t)24,
    TIME_ADDR = (uint8_t)25,
    CARD_ID_ADDR = (uint8_t)26,
    TIMING1_ADDR = (uint8_t)27,
    TIMING2_ADDR = (uint8_t)28,
    TIMING3_ADDR = (uint8_t)29,
    TIMING4_ADDR = (uint8_t)30,
    TIMING5_ADDR = (uint8_t)31,
    TIMING6_ADDR = (uint8_t)32,
    TIMING7_ADDR = (uint8_t)33,
    TIMING8_ADDR = (uint8_t)34,
    CLEAR_ALL_TIMING_ADDR = (uint8_t)35,
    LOCAL_OP_ALARM_ADDR = (uint8_t)36,
    LOST_INPUT_ALARM_ADDR = (uint8_t)37,
    OVER_VOL_ALARM_ADDR = (uint8_t)38,
    OVER_CURRENT_ALARM_ADDR = (uint8_t)39,
    LEAKAGE_ALARM_ADDR = (uint8_t)40,
    UNDER_VOL_ALARM_ADDR = (uint8_t)41,
    HEARTBEAT_ADDR = (uint8_t)42,

    SUB_DEV1_OP_ADDR = (uint8_t)45,
    SUB_DEV1_1_TIMING1_ADDR = (uint8_t)54,
    SUB_DEV1_1_TIMING2_ADDR = (uint8_t)55,
    SUB_DEV1_2_TIMING1_ADDR = (uint8_t)56,
    SUB_DEV1_2_TIMING2_ADDR = (uint8_t)57,
    SUB_DEV1_3_TIMING1_ADDR = (uint8_t)58,
    SUB_DEV1_3_TIMING2_ADDR = (uint8_t)59,
    SUB_DEV1_4_TIMING1_ADDR = (uint8_t)60,
    SUB_DEV1_4_TIMING2_ADDR = (uint8_t)61,
    SUB_DEV1_ALL_TIMING_ADDR = (uint8_t)62,
    CLEAR_SUB_DEV1_TIMING_ADDR = (uint8_t)63,
    
    SUB_DEV2_OP_ADDR = (uint8_t)64,
    SUB_DEV2_1_TIMING1_ADDR = (uint8_t)73,
    SUB_DEV2_1_TIMING2_ADDR = (uint8_t)74,
    SUB_DEV2_2_TIMING1_ADDR = (uint8_t)75,
    SUB_DEV2_2_TIMING2_ADDR = (uint8_t)76,
    SUB_DEV2_3_TIMING1_ADDR = (uint8_t)77,
    SUB_DEV2_3_TIMING2_ADDR = (uint8_t)78,
    SUB_DEV2_4_TIMING1_ADDR = (uint8_t)79,
    SUB_DEV2_4_TIMING2_ADDR = (uint8_t)80,
    SUB_DEV2_ALL_TIMING_ADDR = (uint8_t)81,
    CLEAR_SUB_DEV2_TIMING_ADDR = (uint8_t)82,

    SUB_DEV3_OP_ADDR = (uint8_t)83,
    SUB_DEV3_1_TIMING1_ADDR = (uint8_t)92,
    SUB_DEV3_1_TIMING2_ADDR = (uint8_t)93,
    SUB_DEV3_2_TIMING1_ADDR = (uint8_t)94,
    SUB_DEV3_2_TIMING2_ADDR = (uint8_t)95,
    SUB_DEV3_3_TIMING1_ADDR = (uint8_t)96,
    SUB_DEV3_3_TIMING2_ADDR = (uint8_t)97,
    SUB_DEV3_4_TIMING1_ADDR = (uint8_t)98,
    SUB_DEV3_4_TIMING2_ADDR = (uint8_t)99,
    SUB_DEV3_ALL_TIMING_ADDR = (uint8_t)100,
    CLEAR_SUB_DEV3_TIMING_ADDR = (uint8_t)101,

    SUB_DEV4_OP_ADDR = (uint8_t)102,
    SUB_DEV4_1_TIMING1_ADDR = (uint8_t)111,
    SUB_DEV4_1_TIMING2_ADDR = (uint8_t)112,
    SUB_DEV4_2_TIMING1_ADDR = (uint8_t)113,
    SUB_DEV4_2_TIMING2_ADDR = (uint8_t)114,
    SUB_DEV4_3_TIMING1_ADDR = (uint8_t)115,
    SUB_DEV4_3_TIMING2_ADDR = (uint8_t)116,
    SUB_DEV4_4_TIMING1_ADDR = (uint8_t)117,
    SUB_DEV4_4_TIMING2_ADDR = (uint8_t)118,
    SUB_DEV4_ALL_TIMING_ADDR = (uint8_t)119,
    CLEAR_SUB_DEV4_TIMING_ADDR = (uint8_t)120,

    CLEAR_ALL_SUB_DEV_TIMING_ADDR = (uint8_t)121,
    ALL_SUB_DEV_INFO_ADDR = (uint8_t)122,
};
struct ReceiveDataInfo
{
    uint8_t regAddress;
    int dataLen;
    uint8_t buf[DATA_MAX_LEN];
};

struct TimerInfo
{
    uint8_t id;
    uint8_t type;
    uint8_t loopType;
    uint8_t used;
    uint8_t weekday;
    uint8_t time[6];
    uint8_t addr;
    uint8_t status;
};

struct ControllerStatusInfo
{
    uint8_t status;
    float current;
    float voltage;
    float power;
    float energyused;
    bool netCtrl;
    uint16_t alarm;
    uint8_t realTime[6];
};

struct SubDeviceStatusInfo
{
    uint8_t status;
    float current;
    uint16_t rateCurrent;
    bool netCtrl;
};

struct AlarmInfo
{
    int alarmType;
    float threshold;
    float trigger;
};

class PashonControllerClass
{
public:
    PashonControllerClass(ServerConnectionClass *pConn) : mainSwStatus(0), _pConn(pConn){}
    void begin();
    void loopReceiveData();
    void loopCheckConnection();
    bool connected() {return this->_pConn->connected();}
    bool getProgramVersion(uint16_t *val);
    bool setAutoSwitchOn(bool en);
    bool getAutoSwitchStatus(uint16_t *val);
    bool getSwitchStatus(uint16_t *val);
    bool operateController(OperateMode mode);
    bool getCurFaultInfo(uint16_t *val);
    bool getNextFaultInfo(uint16_t *val);
    bool getFaultInfoValue(uint16_t *val);
    bool getCurrent(float *val);
    bool getLeakageCurrent(uint16_t *val);
    bool getVoltage(float *val);
    bool getActivePower(uint16_t *val);
    bool getPowerFactor(uint16_t *val);
    bool getPowerconsumption(float *val); //月用电量
    bool getTemperature(float *val);
    bool hasSmoke();
    bool getRatedCurrent(float *val);
    bool setRatedCurrent(uint16_t cur);
    bool getLeakageCurrentProtectionVal(uint16_t *val);
    bool setLeakageCurrentProtectionVal(uint16_t val);
    bool getUnderVoltageLimit(float *val);
    bool setUnderVoltageLimit(uint16_t val);
    bool getOverVoltageLimit(float *val);
    bool setOverVoltageLimit(uint16_t val);
    bool setDeviceTime(uint32_t t);
    bool getCurrentTime(uint32_t *t);
    bool setControllerTimer(TimerInfo &t);
    bool getControllerTimer(TimerInfo *t, size_t *cnt);
    bool clearControllerTiming();
    bool getControllerStatus(ControllerStatusInfo &devStatus);
    bool operateSubDevice(int num, bool sw);
    bool getSubDevSwitchStatus(int num, bool *sw);
    bool getAllSubDeviceStatus(SubDeviceStatusInfo *devStatus);
    void setMainSwStatus(uint8_t sta) {mainSwStatus = sta;}
    uint8_t getMainSwStatus() {return mainSwStatus;}

private:
    bool readCmd(uint8_t addr, uint16_t cnt);
    bool writeCmd(uint8_t addr, uint8_t *buffer, uint16_t cnt);
    bool analysisReceivedData(int len);
    static uint8_t usrNum[6];
    static uint8_t factoryNum[4];
    uint8_t rcvBuffer[RCV_BUF_SIZE];
    volatile uint8_t mainSwStatus;
    ReceiveDataInfo rcvDataInfo;
    ServerConnectionClass *_pConn;
};

extern PashonControllerClass PashonController;
extern xQueueHandle xAlarmInfoQueue;
extern volatile uint8_t subDevRecord;
extern const int usrInfoEepAddr;
extern EventGroupHandle_t xDataReceivedEventGroup;
extern uint16_t relayTotal;

#endif