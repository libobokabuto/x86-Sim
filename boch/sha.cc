#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_CPU_LEVEL >= 6

#include "scalar_arith.h"

BX_CPP_INLINE Bit32u sha_f0(Bit32u B, Bit32u C, Bit32u D)
{
	return (B & C) ^ (~B & D);
}

BX_CPP_INLINE Bit32u sha_f1(Bit32u B, Bit32u C, Bit32u D)
{
	return (B ^ C ^ D);
}

BX_CPP_INLINE Bit32u sha_f2(Bit32u B, Bit32u C, Bit32u D)
{
	return (B & C) ^ (B & D) ^ (C & D);
}

BX_CPP_INLINE Bit32u sha_f(Bit32u B, Bit32u C, Bit32u D, unsigned index)
{
	if (index == 0)
		return sha_f0(B, C, D);
	if (index == 2)
		return sha_f2(B, C, D);

	// sha_f3() and sha_f1() are the same
	return sha_f1(B, C, D);
}

#define sha_ch(E,F,G) sha_f0((E), (F), (G))

#define sha_maj(A,B,C) sha_f2((A), (B), (C))

BX_CPP_INLINE Bit32u sha256_transformation_rrr(Bit32u val_32, unsigned rotate1, unsigned rotate2, unsigned rotate3)
{
	return ror32(val_32, rotate1) ^ ror32(val_32, rotate2) ^ ror32(val_32, rotate3);
}

BX_CPP_INLINE Bit32u sha256_transformation_rrs(Bit32u val_32, unsigned rotate1, unsigned rotate2, unsigned shr)
{
	return ror32(val_32, rotate1) ^ ror32(val_32, rotate2) ^ (val_32 >> shr);
}











#endif
