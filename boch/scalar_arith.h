#pragma once

BX_CPP_INLINE unsigned parity_byte(Bit8u val_8)
{
    return (0x9669 >> ((val_8 ^ (val_8 >> 4)) & 0xF)) & 1;
}

BX_CPP_INLINE unsigned tzcntw(Bit16u val_16)
{
    Bit16u mask = 0x1;
    unsigned count = 0;

    while ((val_16 & mask) == 0 && mask) {
        mask <<= 1;
        count++;
    }

    return count;
}

BX_CPP_INLINE unsigned tzcntd(Bit32u val_32)
{
    Bit32u mask = 0x1;
    unsigned count = 0;

    while ((val_32 & mask) == 0 && mask) {
        mask <<= 1;
        count++;
    }

    return count;
}

BX_CPP_INLINE unsigned tzcntq(Bit64u val_64)
{
    Bit64u mask = 0x1;
    unsigned count = 0;

    while ((val_64 & mask) == 0 && mask) {
        mask <<= 1;
        count++;
    }

    return count;
}

BX_CPP_INLINE unsigned lzcntw(Bit16u val_16)
{
    Bit16u mask = 0x8000;
    unsigned count = 0;

    while ((val_16 & mask) == 0 && mask) {
        mask >>= 1;
        count++;
    }

    return count;
}

BX_CPP_INLINE unsigned most_significant_bitw(Bit16u val_16)
{
    return 15 - lzcntw(val_16);
}

BX_CPP_INLINE unsigned lzcntd(Bit32u val_32)
{
    Bit32u mask = 0x80000000;
    unsigned count = 0;

    while ((val_32 & mask) == 0 && mask) {
        mask >>= 1;
        count++;
    }

    return count;
}

BX_CPP_INLINE unsigned most_significant_bitd(Bit32u val_32)
{
    return 31 - lzcntd(val_32);
}

BX_CPP_INLINE unsigned lzcntq(Bit64u val_64)
{
    Bit64u mask = BX_CONST64(0x8000000000000000);
    unsigned count = 0;

    while ((val_64 & mask) == 0 && mask) {
        mask >>= 1;
        count++;
    }

    return count;
}

BX_CPP_INLINE unsigned most_significant_bitq(Bit64u val_64)
{
    return 63 - lzcntq(val_64);
}

BX_CPP_INLINE unsigned popcntb(Bit8u val_8)
{
    val_8 = ((val_8 >> 1) & 0x55) + (val_8 & 0x55);
    val_8 = ((val_8 >> 2) & 0x33) + (val_8 & 0x33);
    val_8 = ((val_8 >> 4) & 0x0F) + (val_8 & 0x0F);

    return (unsigned) val_8;
}

BX_CPP_INLINE unsigned popcntw(Bit16u val_16)
{
    val_16 = ((val_16 >> 1) & 0x5555) + (val_16 & 0x5555);
    val_16 = ((val_16 >> 2) & 0x3333) + (val_16 & 0x3333);
    val_16 = ((val_16 >> 4) & 0x0F0F) + (val_16 & 0x0F0F);
    val_16 = ((val_16 >> 8) & 0x00FF) + (val_16 & 0x00FF);

    return (unsigned) val_16;
}

BX_CPP_INLINE unsigned popcntd(Bit32u val_32)
{
    val_32 = ((val_32 >> 1) & 0x55555555) + (val_32 & 0x55555555);
    val_32 = ((val_32 >> 2) & 0x33333333) + (val_32 & 0x33333333);
    val_32 = ((val_32 >> 4) & 0x0F0F0F0F) + (val_32 & 0x0F0F0F0F);
    val_32 = ((val_32 >> 8) & 0x00FF00FF) + (val_32 & 0x00FF00FF);
    val_32 = ((val_32 >> 16) & 0x0000FFFF) + (val_32 & 0x0000FFFF);

    return (unsigned) val_32;
}

BX_CPP_INLINE unsigned popcntq(Bit64u val_64)
{
    val_64 = ((val_64 >> 1) & BX_CONST64(0x5555555555555555)) + (val_64 & BX_CONST64(0x5555555555555555));
    val_64 = ((val_64 >> 2) & BX_CONST64(0x3333333333333333)) + (val_64 & BX_CONST64(0x3333333333333333));
    val_64 = ((val_64 >> 4) & BX_CONST64(0x0F0F0F0F0F0F0F0F)) + (val_64 & BX_CONST64(0x0F0F0F0F0F0F0F0F));
    val_64 = ((val_64 >> 8) & BX_CONST64(0x00FF00FF00FF00FF)) + (val_64 & BX_CONST64(0x00FF00FF00FF00FF));
    val_64 = ((val_64 >> 16) & BX_CONST64(0x0000FFFF0000FFFF)) + (val_64 & BX_CONST64(0x0000FFFF0000FFFF));
    val_64 = ((val_64 >> 32) & BX_CONST64(0x00000000FFFFFFFF)) + (val_64 & BX_CONST64(0x00000000FFFFFFFF));

    return (unsigned) val_64;
}

BX_CPP_INLINE Bit32u bextrd(Bit32u val_32, unsigned start, unsigned len)
{
    Bit32u result = 0;

    if (start < 32 && len > 0) {
        result = val_32 >> start;

        if (len < 32) {
            Bit32u extract_mask = (1 << len) - 1;
            result &= extract_mask;
        }
    }

    return result;
}

BX_CPP_INLINE Bit64u bextrq(Bit64u val_64, unsigned start, unsigned len)
{
    Bit64u result = 0;

    if (start < 64 && len > 0) {
        result = val_64 >> start;

        if (len < 64) {
            Bit64u extract_mask = (BX_CONST64(1) << len) - 1;
            result &= extract_mask;
        }
    }

    return result;
}

BX_CPP_INLINE Bit8u rol8(Bit8u v8, unsigned count)
{
    return (v8 << count) | (v8 >> (8 - count));
}

BX_CPP_INLINE Bit8u ror8(Bit8u v8, unsigned count)
{
    return (v8 >> count) | (v8 << (8 - count));
}

BX_CPP_INLINE Bit16u rol16(Bit16u v16, unsigned count)
{
    return (v16 << count) | (v16 >> (16 - count));
}

BX_CPP_INLINE Bit16u ror16(Bit16u v16, unsigned count)
{
    return (v16 >> count) | (v16 << (16 - count));
}

BX_CPP_INLINE Bit32u rol32(Bit32u v32, unsigned count)
{
    return (v32 << count) | (v32 >> (32 - count));
}

BX_CPP_INLINE Bit32u ror32(Bit32u v32, unsigned count)
{
    return (v32 >> count) | (v32 << (32 - count));
}

BX_CPP_INLINE Bit64u rol64(Bit64u v64, unsigned count)
{
    return (v64 << count) | (v64 >> (64 - count));
}

BX_CPP_INLINE Bit64u ror64(Bit64u v64, unsigned count)
{
    return (v64 >> count) | (v64 << (64 - count));
}
