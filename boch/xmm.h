#ifndef BX_SSE_EXTENSIONS_H
#define BX_SSE_EXTENSIONS_H

typedef
#if defined(_MSC_VER) && (_MSC_VER>=1300)
__declspec(align(16))
#endif
union bx_xmm_reg_t {
	Bit8s   xmm_sbyte[16];
	Bit16s  xmm_s16[8];
	Bit32s  xmm_s32[4];
	Bit64s  xmm_s64[2];
	Bit8u   xmm_ubyte[16];
	Bit16u  xmm_u16[8];
	Bit32u  xmm_u32[4];
	Bit64u  xmm_u64[2];

	void clear() { xmm_u64[0] = xmm_u64[1] = 0; }
}BxPackedXmmRegister;

struct softfloat_status_t;

#endif