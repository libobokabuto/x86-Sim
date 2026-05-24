#include "bochs.h"
#include "bxthread.h"

#if BX_HAVE_REALTIME_USEC
#if defined(WIN32)
static Bit64u last_realtime64_top = 0;
static Bit64u last_realtime64_bottom = 0;

Bit64u bx_get_realtime64_usec(void)
{
    Bit64u new_bottom = ((Bit64u)GetTickCount()) & BX_CONST64(0x0FFFFFFFF);
    if (new_bottom < last_realtime64_bottom) {
        last_realtime64_top += BX_CONST64(0x0000000100000000);
    }
    last_realtime64_bottom = new_bottom;
    Bit64u interim_realtime64 =
        (last_realtime64_top & BX_CONST64(0xFFFFFFFF00000000)) |
        (new_bottom & BX_CONST64(0x00000000FFFFFFFF));
    return interim_realtime64 * (BX_CONST64(1000));
}
#elif BX_HAVE_GETTIMEOFDAY
Bit64u bx_get_realtime64_usec(void)
{
    timeval thetime;
    gettimeofday(&thetime, 0);
    Bit64u mytime;
    mytime = (Bit64u)thetime.tv_sec * (Bit64u)1000000 + (Bit64u)thetime.tv_usec;
    return mytime;
}
#endif
#endif
