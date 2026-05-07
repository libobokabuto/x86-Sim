#pragma once


#ifndef __MINGW32__ //46


#if !defined(_MSC_VER) //51
#else//57
#define FMT_LL "%I64" 
#endif//63

#if defined(_MSC_VER)
//#define fstat _fstati64
//#define stat  _stati64
//#define read _read
//#define open _open
//#define close _close
#else 
#endif

#else//105
#endif  /* __MINGW32__ defined */ //118

