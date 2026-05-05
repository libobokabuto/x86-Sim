#pragma once
#ifndef BX_SIMD_PFP_FUNCTIONS_H
#define BX_SIMD_PFP_FUNCTIONS_H

BX_CPP_INLINE void xmm_addps(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status)
{
	//29
}

BX_CPP_INLINE void xmm_addps_mask(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status, Bit32u mask)
{
	//36
}

BX_CPP_INLINE void xmm_addpd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status)
{
	//46
}

BX_CPP_INLINE void xmm_addpd_mask(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status, Bit32u mask)
{
	//53
}

BX_CPP_INLINE void xmm_addph(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status)
{
	//63
}

BX_CPP_INLINE void xmm_addph_mask(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status, Bit32u mask)
{
	//70
}

BX_CPP_INLINE void xmm_subps(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status)
{
	//80
}

BX_CPP_INLINE void xmm_subps_mask(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status, Bit32u mask)
{
	//87
}

BX_CPP_INLINE void xmm_subpd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status)
{
	//97
}

BX_CPP_INLINE void xmm_subpd_mask(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status, Bit32u mask)
{
	//104
}

BX_CPP_INLINE void xmm_subph(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status)
{
	//114
}

BX_CPP_INLINE void xmm_subph_mask(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status, Bit32u mask)
{
	//121
}

BX_CPP_INLINE void xmm_mulps(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status)
{
	//131
}

BX_CPP_INLINE void xmm_mulps_mask(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status, Bit32u mask)
{
	//138
}

BX_CPP_INLINE void xmm_mulpd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status)
{
	//148
}

BX_CPP_INLINE void xmm_mulpd_mask(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status, Bit32u mask)
{
	//155
}

BX_CPP_INLINE void xmm_mulph(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status)
{
	//165
}

BX_CPP_INLINE void xmm_mulph_mask(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status, Bit32u mask)
{
	//172
}

BX_CPP_INLINE void xmm_divps(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status)
{
	//182
}

BX_CPP_INLINE void xmm_divps_mask(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status, Bit32u mask)
{
	//189
}

BX_CPP_INLINE void xmm_divpd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status)
{
	//199
}

BX_CPP_INLINE void xmm_divpd_mask(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status, Bit32u mask)
{
	//206
}

BX_CPP_INLINE void xmm_divph(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status)
{
	//216
}

BX_CPP_INLINE void xmm_divph_mask(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status, Bit32u mask)
{
	//223
}

BX_CPP_INLINE void xmm_addsubps(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status)
{
	//233
}

BX_CPP_INLINE void xmm_addsubpd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status)
{
	//264
}

BX_CPP_INLINE void xmm_haddps(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status)
{
	//285
}

BX_CPP_INLINE void xmm_haddpd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status)
{
	//316
}

BX_CPP_INLINE void xmm_hsubps(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status)
{
	//335
}

BX_CPP_INLINE void xmm_hsubpd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status)
{
	//366
}

BX_CPP_INLINE void xmm_minps(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status)
{
	//387
}

BX_CPP_INLINE void xmm_minps_mask(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status, Bit32u mask)
{
	//394
}

BX_CPP_INLINE void xmm_minpd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status)
{
	//404
}

BX_CPP_INLINE void xmm_minpd_mask(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status, Bit32u mask)
{
	//411
}

BX_CPP_INLINE void xmm_minph(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status)
{
	//421
}

BX_CPP_INLINE void xmm_minph_mask(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status, Bit32u mask)
{
	//428
}

BX_CPP_INLINE void xmm_maxps(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status)
{
	//438
}

BX_CPP_INLINE void xmm_maxps_mask(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status, Bit32u mask)
{
	//445
}

BX_CPP_INLINE void xmm_maxpd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status)
{
	//455
}

BX_CPP_INLINE void xmm_maxpd_mask(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status, Bit32u mask)
{
	//462
}

BX_CPP_INLINE void xmm_maxph(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status)
{
	//472
}

BX_CPP_INLINE void xmm_maxph_mask(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status, Bit32u mask)
{
	//479
}

BX_CPP_INLINE void xmm_fmaddps(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3, softfloat_status_t& status)
{
	//491
}

BX_CPP_INLINE void xmm_fmaddps_mask(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3, softfloat_status_t& status, Bit32u mask)
{
	//498
}

BX_CPP_INLINE void xmm_fmaddpd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3, softfloat_status_t& status)
{
	//508
}

BX_CPP_INLINE void xmm_fmaddpd_mask(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3, softfloat_status_t& status, Bit32u mask)
{
	//515
}

BX_CPP_INLINE void xmm_fmaddph(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3, softfloat_status_t& status)
{
	//525
}

BX_CPP_INLINE void xmm_fmaddph_mask(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3, softfloat_status_t& status, Bit32u mask)
{
	//532
}

BX_CPP_INLINE void xmm_fmsubps(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3, softfloat_status_t& status)
{
	//542
}

BX_CPP_INLINE void xmm_fmsubps_mask(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3, softfloat_status_t& status, Bit32u mask)
{
	//549
}

BX_CPP_INLINE void xmm_fmsubpd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3, softfloat_status_t& status)
{
	//559
}

BX_CPP_INLINE void xmm_fmsubpd_mask(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3, softfloat_status_t& status, Bit32u mask)
{
	//566
}

BX_CPP_INLINE void xmm_fmsubph(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3, softfloat_status_t& status)
{
	//576
}

BX_CPP_INLINE void xmm_fmsubph_mask(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3, softfloat_status_t& status, Bit32u mask)
{
	//583
}

BX_CPP_INLINE void xmm_fmaddsubps(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3, softfloat_status_t& status)
{
	//593
}

BX_CPP_INLINE void xmm_fmaddsubps_mask(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3, softfloat_status_t& status, Bit32u mask)
{
	//601
}

BX_CPP_INLINE void xmm_fmaddsubpd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3, softfloat_status_t& status)
{
	//616
}

BX_CPP_INLINE void xmm_fmaddsubpd_mask(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3, softfloat_status_t& status, Bit32u mask)
{
	//622
}

BX_CPP_INLINE void xmm_fmaddsubph(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3, softfloat_status_t& status)
{
	//635
}

BX_CPP_INLINE void xmm_fmaddsubph_mask(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3, softfloat_status_t& status, Bit32u mask)
{
	//643
}

BX_CPP_INLINE void xmm_fmsubaddps(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3, softfloat_status_t& status)
{
	//658
}

BX_CPP_INLINE void xmm_fmsubaddps_mask(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3, softfloat_status_t& status, Bit32u mask)
{
	//666
}

BX_CPP_INLINE void xmm_fmsubaddpd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3, softfloat_status_t& status)
{
	//681
}

BX_CPP_INLINE void xmm_fmsubaddpd_mask(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3, softfloat_status_t& status, Bit32u mask)
{
	//687
}

BX_CPP_INLINE void xmm_fmsubaddph(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3, softfloat_status_t& status)
{
	//700
}

BX_CPP_INLINE void xmm_fmsubaddph_mask(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3, softfloat_status_t& status, Bit32u mask)
{
	//708
}

BX_CPP_INLINE void xmm_fnmaddps(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3, softfloat_status_t& status)
{
	//723
}

BX_CPP_INLINE void xmm_fnmaddps_mask(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3, softfloat_status_t& status, Bit32u mask)
{
	//730
}

BX_CPP_INLINE void xmm_fnmaddpd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3, softfloat_status_t& status)
{
	//740
}

BX_CPP_INLINE void xmm_fnmaddpd_mask(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3, softfloat_status_t& status, Bit32u mask)
{
	//747
}

BX_CPP_INLINE void xmm_fnmaddph(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3, softfloat_status_t& status)
{
	//757
}

BX_CPP_INLINE void xmm_fnmaddph_mask(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3, softfloat_status_t& status, Bit32u mask)
{
	//764
}

BX_CPP_INLINE void xmm_fnmsubps(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3, softfloat_status_t& status)
{
	//774
}

BX_CPP_INLINE void xmm_fnmsubps_mask(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3, softfloat_status_t& status, Bit32u mask)
{
	//781
}

BX_CPP_INLINE void xmm_fnmsubpd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3, softfloat_status_t& status)
{
	//791
}

BX_CPP_INLINE void xmm_fnmsubpd_mask(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3, softfloat_status_t& status, Bit32u mask)
{
	//798
}

BX_CPP_INLINE void xmm_fnmsubph(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3, softfloat_status_t& status)
{
	//808
}

BX_CPP_INLINE void xmm_fnmsubph_mask(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, const BxPackedXmmRegister* op3, softfloat_status_t& status, Bit32u mask)
{
	//815
}

BX_CPP_INLINE void xmm_sqrtps(BxPackedXmmRegister* op, softfloat_status_t& status)
{
	//827
}

BX_CPP_INLINE void xmm_sqrtps_mask(BxPackedXmmRegister* op, softfloat_status_t& status, Bit32u mask)
{
	//834
}

BX_CPP_INLINE void xmm_sqrtpd(BxPackedXmmRegister* op, softfloat_status_t& status)
{
	//844
}

BX_CPP_INLINE void xmm_sqrtpd_mask(BxPackedXmmRegister* op, softfloat_status_t& status, Bit32u mask)
{
	//851
}

BX_CPP_INLINE void xmm_sqrtph(BxPackedXmmRegister* op, softfloat_status_t& status)
{
	//861
}

BX_CPP_INLINE void xmm_sqrtph_mask(BxPackedXmmRegister* op, softfloat_status_t& status, Bit32u mask)
{
	//868
}

BX_CPP_INLINE void xmm_getexpps(BxPackedXmmRegister* op, softfloat_status_t& status)
{
	//880
}

BX_CPP_INLINE void xmm_getexpps_mask(BxPackedXmmRegister* op, softfloat_status_t& status, Bit32u mask)
{
	//887
}

BX_CPP_INLINE void xmm_getexppd(BxPackedXmmRegister* op, softfloat_status_t& status)
{
	//897
}

BX_CPP_INLINE void xmm_getexppd_mask(BxPackedXmmRegister* op, softfloat_status_t& status, Bit32u mask)
{
	//904
}

BX_CPP_INLINE void xmm_getexpph(BxPackedXmmRegister* op, softfloat_status_t& status)
{
	//914
}

BX_CPP_INLINE void xmm_getexpph_mask(BxPackedXmmRegister* op, softfloat_status_t& status, Bit32u mask)
{
	//921
}

BX_CPP_INLINE void xmm_scalefps(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status)
{
	//933
}

BX_CPP_INLINE void xmm_scalefps_mask(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status, Bit32u mask)
{
	//940
}

BX_CPP_INLINE void xmm_scalefpd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status)
{
	//950
}

BX_CPP_INLINE void xmm_scalefpd_mask(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status, Bit32u mask)
{
	//957
}

BX_CPP_INLINE void xmm_scalefph(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status)
{
	//975
}

BX_CPP_INLINE void xmm_scalefph_mask(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status, Bit32u mask)
{
	//982
}

#endif
