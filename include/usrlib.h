#ifndef __USRLIB_H
#define __USRLIB_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void strToHex(uint8_t *pbDest, const char *pbSrc, size_t nLen);
void hexToStr(char *pszDest, const uint8_t *pbSrc, size_t nLen);

#ifdef __cplusplus
}
#endif

#endif