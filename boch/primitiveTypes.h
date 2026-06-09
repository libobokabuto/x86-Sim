#pragma once
#ifndef primitiveTypes_h
#define primitiveTypes_h 1

#include "config.h"

/*----------------------------------------------------------------------------
| These macros are used to isolate the differences in word order between big-
| endian and little-endian platforms.
*----------------------------------------------------------------------------*/
#ifdef BX_LITTLE_ENDIAN
#define wordIncr 1
#define indexWord(total, n) (n)
#define indexWordHi(total) ((total) - 1)
#define indexWordLo(total) 0
#define indexMultiword(total, m, n) (n)
#define indexMultiwordHi(total, n) ((total) - (n))
#define indexMultiwordLo(total, n) 0
#define indexMultiwordHiBut(total, n) (n)
#define indexMultiwordLoBut(total, n) 0
#define INIT_UINTM4(v3, v2, v1, v0) { v0, v1, v2, v3 }
#else
#define wordIncr -1
#define indexWord(total, n) ((total) - 1 - (n))
#define indexWordHi(total) 0
#define indexWordLo(total) ((total) - 1)
#define indexMultiword(total, m, n) ((total) - 1 - (m))
#define indexMultiwordHi(total, n) 0
#define indexMultiwordLo(total, n) ((total) - (n))
#define indexMultiwordHiBut(total, n) 0
#define indexMultiwordLoBut(total, n) (n)
#define INIT_UINTM4(v3, v2, v1, v0) { v3, v2, v1, v0 }
#endif

#endif