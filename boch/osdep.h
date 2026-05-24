#pragma once


#ifndef __MINGW32__ //46


#if !defined(_MSC_VER) //51
#else//57
#define FMT_LL "%I64" 
#endif//63

#if defined(_MSC_VER)
#define fseeko64 _fseeki64
//#define fstat _fstati64
//#define stat  _stati64
//#define read _read
//#define open _open
//#define close _close
#else 
#endif

#else//105
#endif  /* __MINGW32__ defined */ //118

#if BX_HAVE_TMPFILE64 == 0
#define tmpfile64 tmpfile /* use regular tmpfile() function */
#endif

#if !BX_HAVE_SSIZE_T
// needed on Windows
typedef Bit64s ssize_t;
#endif

#if BX_HAVE_REALTIME_USEC
// 64-bit time in useconds.
BOCHSAPI_MSVCONLY extern Bit64u bx_get_realtime64_usec(void);
#endif




