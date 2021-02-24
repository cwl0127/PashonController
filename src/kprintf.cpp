#include "kprintf.h"

void kprintf(const char *fmt, ...)
{
    va_list args;
    size_t length;
    char logBuf[CONSOLEBUF_SIZE];

    va_start(args, fmt);
    /* the return value of vsnprintf is the number of bytes that would be
     * written to buffer had if the size of the buffer been sufficiently
     * large excluding the terminating null byte. If the output string
     * would be larger than the logBuf, we have to adjust the output
     * length. */
    length = vsnprintf(logBuf, sizeof(logBuf) - 1, fmt, args);
    if (length > CONSOLEBUF_SIZE - 1)
        length = CONSOLEBUF_SIZE - 1;
    taskENTER_CRITICAL();
    Serial.print(logBuf);
    taskEXIT_CRITICAL();
    va_end(args);
}

