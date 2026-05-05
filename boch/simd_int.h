#pragma once
#ifndef BX_SIMD_INT_FUNCTIONS_H
#define BX_SIMD_INT_FUNCTIONS_H

BX_CPP_INLINE void xmm_pabsb(BxPackedXmmRegister* op)
{
	//29
}

BX_CPP_INLINE void xmm_pabsw(BxPackedXmmRegister* op)
{
	//36
}

BX_CPP_INLINE void xmm_pabsd(BxPackedXmmRegister* op)
{
	//43
}

BX_CPP_INLINE void xmm_pabsq(BxPackedXmmRegister* op)
{
	//50
}

BX_CPP_INLINE void xmm_pminsb(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//59
}

BX_CPP_INLINE void xmm_pminub(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//66
}

BX_CPP_INLINE void xmm_pminsw(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//73
}

BX_CPP_INLINE void xmm_pminuw(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//80
}

BX_CPP_INLINE void xmm_pminsd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//87
}

BX_CPP_INLINE void xmm_pminud(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//94
}

BX_CPP_INLINE void xmm_pminsq(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//101
}

BX_CPP_INLINE void xmm_pminuq(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//108
}

BX_CPP_INLINE void xmm_pmaxsb(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//115
}

BX_CPP_INLINE void xmm_pmaxub(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//122
}

BX_CPP_INLINE void xmm_pmaxsw(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//129
}

BX_CPP_INLINE void xmm_pmaxuw(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//136
}

BX_CPP_INLINE void xmm_pmaxsd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//143
}

BX_CPP_INLINE void xmm_pmaxud(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//150
}

BX_CPP_INLINE void xmm_pmaxsq(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//157
}

BX_CPP_INLINE void xmm_pmaxuq(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//164
}

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

BX_CPP_INLINE void xmm_punpckhbw(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//221
}

BX_CPP_INLINE void xmm_punpcklwd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//241
}

BX_CPP_INLINE void xmm_punpckhwd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//253
}

BX_CPP_INLINE void xmm_packuswb(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//267
}

BX_CPP_INLINE void xmm_packsswb(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//288
}

BX_CPP_INLINE void xmm_packusdw(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//309
}

BX_CPP_INLINE void xmm_packssdw(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//322
}

BX_CPP_INLINE void xmm_pshufb(BxPackedXmmRegister* r, const BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//337
}

BX_CPP_INLINE void xmm_permilps(BxPackedXmmRegister* r, const BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//381
}

BX_CPP_INLINE void xmm_permilpd(BxPackedXmmRegister* r, const BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//389
}

BX_CPP_INLINE void xmm_psignb(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//429
}

BX_CPP_INLINE void xmm_psignw(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//437
}

BX_CPP_INLINE void xmm_psignd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//445
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

BX_CPP_INLINE void xmm_paddb(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//729
}

BX_CPP_INLINE void xmm_paddw(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//736
}

BX_CPP_INLINE void xmm_paddd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//743
}

BX_CPP_INLINE void xmm_paddq(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//750
}

BX_CPP_INLINE void xmm_psubb(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//757
}

BX_CPP_INLINE void xmm_psubw(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//764
}

BX_CPP_INLINE void xmm_psubd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//771
}

BX_CPP_INLINE void xmm_psubq(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//778
}

BX_CPP_INLINE void xmm_paddsb(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//787
}

BX_CPP_INLINE void xmm_paddsw(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//794
}

BX_CPP_INLINE void xmm_paddusb(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//801
}

BX_CPP_INLINE void xmm_paddusw(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//808
}

BX_CPP_INLINE void xmm_psubsb(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//815
}

BX_CPP_INLINE void xmm_psubsw(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//822
}

BX_CPP_INLINE void xmm_psubusb(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//829
}

BX_CPP_INLINE void xmm_psubusw(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//840
}

BX_CPP_INLINE void xmm_phaddw(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//853
}

BX_CPP_INLINE void xmm_phaddd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//866
}

BX_CPP_INLINE void xmm_phaddsw(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//874
}

BX_CPP_INLINE void xmm_phsubw(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//887
}

BX_CPP_INLINE void xmm_phsubd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//900
}

BX_CPP_INLINE void xmm_phsubsw(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//908
}

BX_CPP_INLINE void xmm_pavgb(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//923
}

BX_CPP_INLINE void xmm_pavgw(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//930
}

BX_CPP_INLINE void xmm_pmullw(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//939
}

BX_CPP_INLINE void xmm_pmulhw(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//946
}

BX_CPP_INLINE void xmm_pmulhuw(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//954
}

BX_CPP_INLINE void xmm_pmulld(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//962
}

BX_CPP_INLINE void xmm_pmullq(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//969
}

BX_CPP_INLINE void xmm_pmuldq(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//976
}

BX_CPP_INLINE void xmm_pmuludq(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//982
}

BX_CPP_INLINE void xmm_pmulhrsw(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//988
}

BX_CPP_INLINE void xmm_pmaddubsw(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//997
}

BX_CPP_INLINE void xmm_pmaddwd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//1008
}

BX_CPP_INLINE void xmm_psadbw(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//1079
}

BX_CPP_INLINE void xmm_psravw(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//1170
}

BX_CPP_INLINE void xmm_psravd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//1181
}

BX_CPP_INLINE void xmm_psravq(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//1192
}

BX_CPP_INLINE void xmm_psllvw(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//1203
}

BX_CPP_INLINE void xmm_psllvd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//1214
}

BX_CPP_INLINE void xmm_psllvq(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//1225
}

BX_CPP_INLINE void xmm_psrlvw(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//1236
}

BX_CPP_INLINE void xmm_psrlvd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//1247
}

BX_CPP_INLINE void xmm_psrlvq(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//1258
}

BX_CPP_INLINE void xmm_psraw(BxPackedXmmRegister* op, Bit64u shift_64)
{
	//1269
}

BX_CPP_INLINE void xmm_psrad(BxPackedXmmRegister* op, Bit64u shift_64)
{
	//1284
}

BX_CPP_INLINE void xmm_psraq(BxPackedXmmRegister* op, Bit64u shift_64)
{
	//1299
}

BX_CPP_INLINE void xmm_psrlw(BxPackedXmmRegister* op, Bit64u shift_64)
{
	//1314
}

BX_CPP_INLINE void xmm_psrld(BxPackedXmmRegister* op, Bit64u shift_64)
{
	//1326
}

BX_CPP_INLINE void xmm_psrlq(BxPackedXmmRegister* op, Bit64u shift_64)
{
	//1338
}

BX_CPP_INLINE void xmm_psllw(BxPackedXmmRegister* op, Bit64u shift_64)
{
	//1350
}

BX_CPP_INLINE void xmm_pslld(BxPackedXmmRegister* op, Bit64u shift_64)
{
	//1362
}

BX_CPP_INLINE void xmm_psllq(BxPackedXmmRegister* op, Bit64u shift_64)
{
	//1374
}

BX_CPP_INLINE void xmm_psrldq(BxPackedXmmRegister* op, Bit64u shift)
{
	//1386
}

BX_CPP_INLINE void xmm_pslldq(BxPackedXmmRegister* op, Bit64u shift)
{
	//1405
}

BX_CPP_INLINE void xmm_prord(BxPackedXmmRegister* op, Bit64u shift_ctrl)
{
	//1471
}

BX_CPP_INLINE void xmm_prorq(BxPackedXmmRegister* op, Bit64u shift_ctrl)
{
	//1480
}

BX_CPP_INLINE void xmm_prorvd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//1489
}

BX_CPP_INLINE void xmm_prorvq(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//1497
}

BX_CPP_INLINE void xmm_prold(BxPackedXmmRegister* op, Bit64u shift_ctrl)
{
	//1525
}

BX_CPP_INLINE void xmm_prolq(BxPackedXmmRegister* op, Bit64u shift_ctrl)
{
	//1534
}

BX_CPP_INLINE void xmm_prolvd(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//1543
}

BX_CPP_INLINE void xmm_prolvq(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2)
{
	//1551
}

#endif
