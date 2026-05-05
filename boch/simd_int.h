#pragma once
#ifndef BX_SIMD_INT_FUNCTIONS_H
#define BX_SIMD_INT_FUNCTIONS_H


BX_CPP_INLINE void xmm_unpcklps(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//173
}

BX_CPP_INLINE void xmm_unpckhps(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//181
}

BX_CPP_INLINE void xmm_unpcklpd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//189
}

BX_CPP_INLINE void xmm_unpckhpd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//195
}

BX_CPP_INLINE void xmm_punpcklbw(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//201
}

BX_CPP_INLINE void xmm_punpcklwd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//241
}

BX_CPP_INLINE void xmm_packuswb(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//267
}

BX_CPP_INLINE void xmm_packsswb(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//288
}

BX_CPP_INLINE void xmm_andps(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//703
}

BX_CPP_INLINE void xmm_andnps(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//709
}

BX_CPP_INLINE void xmm_orps(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//715
}

BX_CPP_INLINE void xmm_xorps(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//721
}
#endif