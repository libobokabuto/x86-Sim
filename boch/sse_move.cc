#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_CPU_LEVEL >= 6

#include "simd_int.h"

void BX_CPU_C::print_state_SSE(void)
{
    //BX_DEBUG(("MXCSR: 0x%08x", BX_MXCSR_REGISTER));
    for (unsigned n = 0; n < BX_XMM_REGISTERS; n++) {
        BxPackedXmmRegister xmm = BX_READ_XMM_REG(n);
        //BX_DEBUG(("XMM%02u: %08x%08x:%08x%08x", n,
            //xmm.xmm32u(3), xmm.xmm32u(2), xmm.xmm32u(1), xmm.xmm32u(0)));
    }
}

#endif

#if BX_SUPPORT_FPU
Bit8u BX_CPU_C::pack_FPU_TW(Bit16u twd)
{
    Bit8u tag_byte = 0;

    if ((twd & 0x0003) != 0x0003) tag_byte |= 0x01;
    if ((twd & 0x000c) != 0x000c) tag_byte |= 0x02;
    if ((twd & 0x0030) != 0x0030) tag_byte |= 0x04;
    if ((twd & 0x00c0) != 0x00c0) tag_byte |= 0x08;
    if ((twd & 0x0300) != 0x0300) tag_byte |= 0x10;
    if ((twd & 0x0c00) != 0x0c00) tag_byte |= 0x20;
    if ((twd & 0x3000) != 0x3000) tag_byte |= 0x40;
    if ((twd & 0xc000) != 0xc000) tag_byte |= 0x80;

    return tag_byte;
}

Bit16u unpack_FPU_TW(const i387_t* i387, Bit16u tag_byte)
{
    Bit32u twd = 0;

    /*                                 FTW
     *
     * Note that the original format for FTW can be recreated from the stored
     * FTW valid bits and the stored 80-bit FP data (assuming the stored data
     * was not the contents of MMX registers) using the following table:

       | Exponent | Exponent | Fraction | J,M bits | FTW valid | x87 FTW |
       |  all 1s  |  all 0s  |  all 0s  |          |           |         |
       -------------------------------------------------------------------
       |    0     |    0     |    0     |    0x    |     1     | S    10 |
       |    0     |    0     |    0     |    1x    |     1     | V    00 |
       -------------------------------------------------------------------
       |    0     |    0     |    1     |    00    |     1     | S    10 |
       |    0     |    0     |    1     |    10    |     1     | V    00 |
       -------------------------------------------------------------------
       |    0     |    1     |    0     |    0x    |     1     | S    10 |
       |    0     |    1     |    0     |    1x    |     1     | S    10 |
       -------------------------------------------------------------------
       |    0     |    1     |    1     |    00    |     1     | Z    01 |
       |    0     |    1     |    1     |    10    |     1     | S    10 |
       -------------------------------------------------------------------
       |    1     |    0     |    0     |    1x    |     1     | S    10 |
       |    1     |    0     |    0     |    1x    |     1     | S    10 |
       -------------------------------------------------------------------
       |    1     |    0     |    1     |    00    |     1     | S    10 |
       |    1     |    0     |    1     |    10    |     1     | S    10 |
       -------------------------------------------------------------------
       |        all combinations above             |     0     | E    11 |

     *
     * The J-bit is defined to be the 1-bit binary integer to the left of
     * the decimal place in the significand.
     *
     * The M-bit is defined to be the most significant bit of the fractional
     * portion of the significand (i.e., the bit immediately to the right of
     * the decimal place). When the M-bit is the most significant bit of the
     * fractional portion  of the significand, it must be  0 if the fraction
     * is all 0's.
     */

    for (int index = 7; index >= 0; index--, twd <<= 2, tag_byte <<= 1)
    {
        if (tag_byte & 0x80) {
            const floatx80& fpu_reg = i387->st_space[index];
            twd |= FPU_tagof(fpu_reg);
        }
        else {
            twd |= FPU_Tag_Empty;
        }
    }

    return (twd >> 2);
}
#endif
