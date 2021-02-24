#ifndef __COMMAND_H
#define __COMMAND_H

#include <ArduinoJson.h>
#include <STM32RTC.h>
#include <STM32SD.h>
#include "usrconfig.h"
#include "defs.h"
#include "TimeLib.h"
//#include "log.h"
#include "PashonController.h"

int cmdPreParse(const String &str);
void postrt(void);
void pushAlarmInfo(void);

extern const char *prefix;
extern const char *suffix;
extern xSemaphoreHandle xSendAllStatusBinarySem;
extern volatile bool operatingSwitchs;

#endif