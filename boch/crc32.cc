#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_CPU_LEVEL >= 6

// 3-byte opcodes

const Bit64u CRC32_POLYNOMIAL = BX_CONST64(0x11edc6f41);

// primitives for CRC32 usage
BX_CPP_INLINE Bit8u BitReflect8(Bit8u val8)
{
    return ((val8 & 0x80) >> 7) |
        ((val8 & 0x40) >> 5) |
        ((val8 & 0x20) >> 3) |
        ((val8 & 0x10) >> 1) |
        ((val8 & 0x08) << 1) |
        ((val8 & 0x04) << 3) |
        ((val8 & 0x02) << 5) |
        ((val8 & 0x01) << 7);
}

BX_CPP_INLINE Bit16u BitReflect16(Bit16u val16)
{
    return ((Bit16u)(BitReflect8(val16 & 0xff)) << 8) | BitReflect8(val16 >> 8);
}

BX_CPP_INLINE Bit32u BitReflect32(Bit32u val32)
{
    return ((Bit32u)(BitReflect16(val32 & 0xffff)) << 16) | BitReflect16(val32 >> 16);
}

static Bit32u mod2_64bit(Bit64u divisor, Bit64u dividend)
{
    Bit64u remainder = dividend >> 32;

    for (int bitpos = 31; bitpos >= 0; bitpos--) {
        // copy one more bit from the dividend
        remainder = (remainder << 1) | ((dividend >> bitpos) & 1);

        // if MSB is set, then XOR divisor and get new remainder
        if (((remainder >> 32) & 1) == 1) {
            remainder ^= divisor;
        }
    }

    return (Bit32u)remainder;
}
#endif // BX_CPU_LEVEL >= 6
