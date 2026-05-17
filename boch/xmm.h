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

typedef
#if defined(_MSC_VER) && (_MSC_VER>=1300)
__declspec(align(32))
#endif
union bx_ymm_reg_t {
	Bit8s   ymm_sbyte[32];
	Bit16s  ymm_s16[16];
	Bit32s  ymm_s32[8];
	Bit64s  ymm_s64[4];
	Bit8u   ymm_ubyte[32];
	Bit16u  ymm_u16[16];
	Bit32u  ymm_u32[8];
	Bit64u  ymm_u64[4];
	BxPackedXmmRegister ymm_v128[2];

	void clear() {
		ymm_v128[0].clear();
		ymm_v128[1].clear();
	}
} BxPackedYmmRegister;

#ifdef BX_BIG_ENDIAN
#define ymm64s(i)   ymm_s64[3 - (i)]
#define ymm32s(i)   ymm_s32[7 - (i)]
#define ymm16s(i)   ymm_s16[15 - (i)]
#define ymmsbyte(i) ymm_sbyte[31 - (i)]
#define ymmubyte(i) ymm_ubyte[31 - (i)]
#define ymm16u(i)   ymm_u16[15 - (i)]
#define ymm32u(i)   ymm_u32[7 - (i)]
#define ymm64u(i)   ymm_u64[3 - (i)]
#define ymm128(i)   ymm_v128[1 - (i)]
#else
#define ymm64s(i)   ymm_s64[(i)]
#define ymm32s(i)   ymm_s32[(i)]
#define ymm16s(i)   ymm_s16[(i)]
#define ymmsbyte(i) ymm_sbyte[(i)]
#define ymmubyte(i) ymm_ubyte[(i)]
#define ymm16u(i)   ymm_u16[(i)]
#define ymm32u(i)   ymm_u32[(i)]
#define ymm64u(i)   ymm_u64[(i)]
#define ymm128(i)   ymm_v128[(i)]
#endif

struct softfloat_status_t;

#endif