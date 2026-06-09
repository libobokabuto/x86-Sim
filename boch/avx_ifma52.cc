#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_SUPPORT_AVX || BX_SUPPORT_EVEX

#include "wide_int.h"

// 52-bit integer FMA

BX_CPP_INLINE Bit64u pmadd52luq_scalar(Bit64u dst, Bit64u op1, Bit64u op2)
{
	op1 &= BX_CONST64(0x000fffffffffffff);
	op2 &= BX_CONST64(0x000fffffffffffff);

	return dst + ((op1 * op2) & BX_CONST64(0x000fffffffffffff));
}

BX_CPP_INLINE Bit64u pmadd52huq_scalar(Bit64u dst, Bit64u op1, Bit64u op2)
{
	op1 &= BX_CONST64(0x000fffffffffffff);
	op2 &= BX_CONST64(0x000fffffffffffff);

	Bit128u product_128;
	long_mul(&product_128, op1, op2);

	Bit64u temp = (product_128.lo >> 52) | ((product_128.hi & BX_CONST64(0x000000ffffffffff)) << 12);

	return dst + temp;
}
#endif