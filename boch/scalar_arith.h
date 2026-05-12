#pragma once


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
