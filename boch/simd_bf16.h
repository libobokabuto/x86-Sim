#pragma once
#ifndef BX_SIMD_BF16_FUNCTIONS_H
#define BX_SIMD_BF16_FUNCTIONS_H

BX_CPP_INLINE void xmm_addbf16(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//31
}

BX_CPP_INLINE void xmm_subbf16(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//48
}

BX_CPP_INLINE void xmm_mulbf16(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//65
}

BX_CPP_INLINE void xmm_divbf16(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//82
}

BX_CPP_INLINE void xmm_minbf16(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//101
}

BX_CPP_INLINE void xmm_maxbf16(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//118
}

BX_CPP_INLINE void xmm_fmaddbf16(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3)
{
	//137
}

BX_CPP_INLINE void xmm_fmsubbf16(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3)
{
	//154
}

BX_CPP_INLINE void xmm_fnmaddbf16(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3)
{
	//171
}

BX_CPP_INLINE void xmm_fnmsubbf16(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3)
{
	//188
}

BX_CPP_INLINE void xmm_sqrtbf16(BxPackedXmmRegister* op)
{
	//207
}

BX_CPP_INLINE void xmm_getexpbf16(BxPackedXmmRegister* op)
{
	//226
}

BX_CPP_INLINE void xmm_scalefbf16(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//245
}

#endif
