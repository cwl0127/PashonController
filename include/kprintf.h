#ifndef __KPRINTF_H
#define __KPRINTF_H

#include <WSerial.h>
#include <stdarg.h>
#include <stdio.h>
#include <FreeRTOS.h>
#include <task.h>

#define CONSOLEBUF_SIZE 128

void kprintf(const char *fmt, ...);

#endif