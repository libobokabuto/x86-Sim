#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

/* ********************************************** */
/* SSE Integer Operations (128bit MMX extensions) */
/* ********************************************** */

#if BX_CPU_LEVEL >= 6

#include "simd_int.h"

#endif 

#if BX_CPU_LEVEL >= 6
BX_CPP_INLINE Bit64u xmm_extrq(Bit64u src, unsigned shift, unsigned len)
{
    len &= 0x3f;
    shift &= 0x3f;

    src >>= shift;
    if (len) {
        Bit64u mask = (BX_CONST64(1) << len) - 1;
        return src & mask;
    }

    return src;
}

BX_CPP_INLINE Bit64u xmm_insertq(Bit64u dest, Bit64u src, unsigned shift, unsigned len)
{
    Bit64u mask;

    len &= 0x3f;
    shift &= 0x3f;

    if (len == 0) {
        mask = BX_CONST64(0xffffffffffffffff);
    }
    else {
        mask = (BX_CONST64(1) << len) - 1;
    }

    return (dest & ~(mask << shift)) | ((src & mask) << shift);
}
#endif