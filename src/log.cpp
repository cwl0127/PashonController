#include "log.h"
#include "usrconfig.h"
#include "TimeLib.h"
#include <STM32RTC.h>
#include <STM32SD.h>
#include <stdio.h>
#include <stdarg.h>

extern STM32RTC &rtc;

// bool writeLog(const char *str)
// {
//     File logFile = SD.open("log.txt", FILE_WRITE);
//     if (!logFile)
//         return false;
//     DebugPrintln("Writing log.txt");
//     uint32_t size = logFile.size();
//     logFile.seek(size);
//     String realTime = timestamp2Str(rtc.getEpoch());
//     logFile.print(realTime.c_str());
//     logFile.print(" ");
//     logFile.println(str);
//     logFile.println("=================================");
//     logFile.close();
//     return true;
// }

bool writeLog(const char *fmt, ...)
{
    va_list args;
    size_t length;
    char logBuf[CONSOLEBUF_SIZE];

    File logFile = SD.open("log.txt", FILE_WRITE);
    if (!logFile)
        return false;
    DebugPrintln("Writing log.txt");
    uint32_t size = logFile.size();
    logFile.seek(size);
    String realTime = timestamp2Str(rtc.getEpoch());
    logFile.print(realTime.c_str());
    logFile.print(" ");
    va_start(args, fmt);
    length = vsnprintf(logBuf, sizeof(logBuf) - 1, fmt, args);
    if (length > CONSOLEBUF_SIZE - 1)
        length = CONSOLEBUF_SIZE - 1;
    logFile.write(logBuf, length);
    logFile.println("");
    logFile.println("=================================");
    logFile.close();
    return true;
}