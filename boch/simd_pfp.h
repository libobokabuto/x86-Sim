#pragma once
#ifndef BX_SIMD_PFP_FUNCTIONS_H
#define BX_SIMD_PFP_FUNCTIONS_H
BX_CPP_INLINE void xmm_addps(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status)
{
	//29
}

BX_CPP_INLINE void xmm_addpd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status)
{
	//46
}

BX_CPP_INLINE void xmm_subps(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status)
{
	//80
}

BX_CPP_INLINE void xmm_subpd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status)
{
	//97
}

BX_CPP_INLINE void xmm_mulps(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status)
{
	//131
}

BX_CPP_INLINE void xmm_mulpd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status)
{
	//148
}

BX_CPP_INLINE void xmm_divps(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status)
{
	//182
}

BX_CPP_INLINE void xmm_divpd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status)
{
	//199
}

BX_CPP_INLINE void xmm_minps(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status)
{
	//387
}

BX_CPP_INLINE void xmm_minpd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status)
{
	//404
}

BX_CPP_INLINE void xmm_maxps(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status)
{
	//438
}

BX_CPP_INLINE void xmm_maxpd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status)
{
	//455
}

BX_CPP_INLINE void xmm_sqrtps(BxPackedXmmRegister* op, softfloat_status_t& status)
{
	//827
}

BX_CPP_INLINE void xmm_sqrtpd(BxPackedXmmRegister* op, softfloat_status_t& status)
{
	//844
}
#endif
