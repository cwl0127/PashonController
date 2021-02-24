#include "usrlib.h"
#include <ctype.h>

void strToHex(uint8_t *pbDest, const char *pbSrc, size_t nLen)
{
    char h1, h2;
    uint8_t s1, s2;

    for (size_t i = 0; i < nLen / 2; i++)
    {
        h1 = pbSrc[2 * i];
        h2 = pbSrc[2 * i + 1];

        s1 = toupper(h1) - 0x30; //toupper 转换为大写字母
        if (s1 > 9)
            s1 -= 7;
        s2 = toupper(h2) - 0x30;
        if (s2 > 9)
            s2 -= 7;

        pbDest[i] = s1 * 16 + s2;
    }
}

void hexToStr(char *pszDest, const uint8_t *pbSrc, size_t nLen)
{
    char ddl, ddh;

    for (size_t i = 0; i < nLen; i++)
    {
        ddh = 48 + pbSrc[i] / 16;
        ddl = 48 + pbSrc[i] % 16;
        if (ddh > 57)
            ddh = ddh + 7;
        if (ddl > 57)
            ddl = ddl + 7;
        pszDest[i * 2] = ddh;
        pszDest[i * 2 + 1] = ddl;
    }
    pszDest[nLen * 2] = '\0';
}