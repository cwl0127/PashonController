#ifndef __USR_TASK_DEFS_H
#define __USR_TASK_DEFS_H

#include <FreeRTOS.h>
#include <event_groups.h>

#define TASK_BIT_0 (1 << 0)
#define TASK_BIT_1 (1 << 1)
#define TASK_BIT_2 (1 << 2)
#define TASK_BIT_3 (1 << 3)
#define TASK_BIT_4 (1 << 4)
//#define TASK_BIT_5 (1 << 5)
#define TASK_BIT_ALL (TASK_BIT_0 | TASK_BIT_1 | TASK_BIT_2 | TASK_BIT_3 | TASK_BIT_4)

extern EventGroupHandle_t xCreatedEventGroup;
#endif