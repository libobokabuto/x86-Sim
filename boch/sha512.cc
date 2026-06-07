#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_SUPPORT_AVX

#include "scalar_arith.h"

BX_CPP_INLINE Bit64u s0(Bit64u val64)
{
	return ror64(val64, 1) ^ ror64(val64, 8) ^ (val64 >> 7);
}

BX_CPP_INLINE Bit64u s1(Bit64u val64)
{
	return ror64(val64, 19) ^ ror64(val64, 61) ^ (val64 >> 6);
}

BX_CPP_INLINE Bit64u cap_sigma0(Bit64u val64)
{
	return ror64(val64, 28) ^ ror64(val64, 34) ^ ror64(val64, 39);
}

BX_CPP_INLINE Bit64u cap_sigma1(Bit64u val64)
{
	return ror64(val64, 14) ^ ror64(val64, 18) ^ ror64(val64, 41);
}

BX_CPP_INLINE Bit64u sha_maj(Bit64u a, Bit64u b, Bit64u c)
{
	return (a & b) ^ (a & c) ^ (b & c);
}

BX_CPP_INLINE Bit64u sha_ch(Bit64u e, Bit64u f, Bit64u g)
{
	return (e & f) ^ (g & ~e);
}

#endif