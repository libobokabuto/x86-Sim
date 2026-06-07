#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_SUPPORT_AVX

#include "scalar_arith.h"

BX_CPP_INLINE Bit32u SM3_P1(Bit32u v32)
{
    return v32 ^ rol32(v32, 15) ^ rol32(v32, 23);
}

BX_CPP_INLINE Bit32u SM3_P0(Bit32u v32)
{
    return v32 ^ rol32(v32, 9) ^ rol32(v32, 17);
}

BX_CPP_INLINE Bit32u SM3_FF(Bit32u x, Bit32u y, Bit32u z, unsigned round)
{
    if (round < 16)
        return (x ^ y ^ z);
    else
        return (x & y) | (x & z) | (y & z);
}

BX_CPP_INLINE Bit32u SM3_GG(Bit32u x, Bit32u y, Bit32u z, unsigned round)
{
    if (round < 16)
        return (x ^ y ^ z);
    else
        return (x & y) | (~x & z);
}

#endif