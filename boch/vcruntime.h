#pragma once
//56
#include <sal.h>
#include <vadefs.h>

//148
#ifdef _M_CEE_PURE
#define __CLRCALL_PURE_OR_CDECL __clrcall
#else
#define __CLRCALL_PURE_OR_CDECL __cdecl
#endif

#define __CRTDECL __CLRCALL_PURE_OR_CDECL
