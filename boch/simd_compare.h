#pragma once
#ifndef BX_SIMD_INT_COMPARE_FUNCTIONS_H
#define BX_SIMD_INT_COMPARE_FUNCTIONS_H

BX_CPP_INLINE void xmm_pcmpgtb(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//293
}

BX_CPP_INLINE void xmm_pcmpgtw(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//309
}

BX_CPP_INLINE void xmm_pcmpgtd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//325
}

BX_CPP_INLINE void xmm_pcmpgtq(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//341
}

BX_CPP_INLINE void xmm_pcmpeqb(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//557
}

BX_CPP_INLINE void xmm_pcmpeqw(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//573
}

BX_CPP_INLINE void xmm_pcmpeqd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//589
}

BX_CPP_INLINE void xmm_pcmpeqq(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//605
}

#endif
