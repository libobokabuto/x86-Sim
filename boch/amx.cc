#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_SUPPORT_AMX

#include "amx.h"

void BX_CPU_C::check_tiles(bxInstruction_c* i, unsigned tile_dst, unsigned tile_src1, unsigned tile_src2)
{
	// #UD if srcdest == src1 OR src1 == src2 OR srcdest == src2
	if (tile_dst == tile_src1 || tile_dst == tile_src2 || tile_src1 == tile_src2) {
		//BX_ERROR(("%s: must use different tiles", i->getIaOpcodeNameShort()));
		exception(BX_UD_EXCEPTION, 0);
	}

	// #UD if TILES_CONFIGURED == 0
	// #UD if srcdest/src1/src2 are not valid tiles
	// #UD if srcdest/src1/src2 are >= palette_table[tilecfg.palette_id].max_names
	check_tile(i, tile_dst);
	check_tile(i, tile_src1);
	check_tile(i, tile_src2);

	unsigned rows[3];
	unsigned dword_elements_per_row[3];

	rows[0] = BX_CPU_THIS_PTR amx->tile_num_rows(tile_dst);
	dword_elements_per_row[0] = BX_CPU_THIS_PTR amx->tile_dword_elements_per_row(tile_dst);
	rows[1] = BX_CPU_THIS_PTR amx->tile_num_rows(tile_src1);
	dword_elements_per_row[1] = BX_CPU_THIS_PTR amx->tile_dword_elements_per_row(tile_src1);
	rows[2] = BX_CPU_THIS_PTR amx->tile_num_rows(tile_src2);
	dword_elements_per_row[2] = BX_CPU_THIS_PTR amx->tile_dword_elements_per_row(tile_src2);

	//     R   C
	// A = m x k (tsrc1)
	// B = k x n (tsrc2)
	// C = m x n (tsrcdest)
	unsigned n = dword_elements_per_row[0];
	unsigned m = rows[1];
	unsigned k = rows[2];

	// #UD if srcdest.colbytes != src2.colbytes (n)
	// #UD if srcdest.rows != src1.rows (m)
	// #UD if src1.colbytes / 4 != src2.rows (k)
	if (n != dword_elements_per_row[2] || m != rows[0] || k != dword_elements_per_row[1]) {
		//BX_ERROR(("%s: invalid matmul tile dimenstions", i->getIaOpcodeNameShort()));
		exception(BX_UD_EXCEPTION, 0);
	}

	// #UD if srcdest.colbytes > tmul_maxn
	// #UD if src2.colbytes > tmul_maxn
	// #UD if src1.colbytes/4 > tmul_maxk
	// #UD if src2.rows > tmul_maxk
	if (n > 16 || k > 16) {
		BX_ERROR(("%s: unsupported matmul tile dimenstions", i->getIaOpcodeNameShort()));
		exception(BX_UD_EXCEPTION, 0);
	}
}

BX_CPP_INLINE Bit32u DPBDSS(Bit32u x, Bit32u y)
{
	const Bit8u xbyte[4] = { Bit8u(x & 0xff), Bit8u((x >> 8) & 0xff), Bit8u((x >> 16) & 0xff), Bit8u(x >> 24) };
	const Bit8u ybyte[4] = { Bit8u(y & 0xff), Bit8u((y >> 8) & 0xff), Bit8u((y >> 16) & 0xff), Bit8u(y >> 24) };

	Bit32s p0dword = Bit32s(xbyte[0]) * Bit32s(ybyte[0]);
	Bit32s p1dword = Bit32s(xbyte[1]) * Bit32s(ybyte[1]);
	Bit32s p2dword = Bit32s(xbyte[2]) * Bit32s(ybyte[2]);
	Bit32s p3dword = Bit32s(xbyte[3]) * Bit32s(ybyte[3]);

	return p0dword + p1dword + p2dword + p3dword;
}

BX_CPP_INLINE Bit32u DPBDSU(Bit32u x, Bit32u y)
{
	const Bit8u xbyte[4] = { Bit8u(x & 0xff), Bit8u((x >> 8) & 0xff), Bit8u((x >> 16) & 0xff), Bit8u(x >> 24) };
	const Bit8u ybyte[4] = { Bit8u(y & 0xff), Bit8u((y >> 8) & 0xff), Bit8u((y >> 16) & 0xff), Bit8u(y >> 24) };

	Bit32s p0dword = Bit32s(xbyte[0]) * Bit32u(ybyte[0]);
	Bit32s p1dword = Bit32s(xbyte[1]) * Bit32u(ybyte[1]);
	Bit32s p2dword = Bit32s(xbyte[2]) * Bit32u(ybyte[2]);
	Bit32s p3dword = Bit32s(xbyte[3]) * Bit32u(ybyte[3]);

	return p0dword + p1dword + p2dword + p3dword;
}

BX_CPP_INLINE Bit32u DPBDUS(Bit32u x, Bit32u y)
{
	const Bit8u xbyte[4] = { Bit8u(x & 0xff), Bit8u((x >> 8) & 0xff), Bit8u((x >> 16) & 0xff), Bit8u(x >> 24) };
	const Bit8u ybyte[4] = { Bit8u(y & 0xff), Bit8u((y >> 8) & 0xff), Bit8u((y >> 16) & 0xff), Bit8u(y >> 24) };

	Bit32s p0dword = Bit32u(xbyte[0]) * Bit32s(ybyte[0]);
	Bit32s p1dword = Bit32u(xbyte[1]) * Bit32s(ybyte[1]);
	Bit32s p2dword = Bit32u(xbyte[2]) * Bit32s(ybyte[2]);
	Bit32s p3dword = Bit32u(xbyte[3]) * Bit32s(ybyte[3]);

	return p0dword + p1dword + p2dword + p3dword;
}

BX_CPP_INLINE Bit32u DPBDUU(Bit32u x, Bit32u y)
{
	const Bit8u xbyte[4] = { Bit8u(x & 0xff), Bit8u((x >> 8) & 0xff), Bit8u((x >> 16) & 0xff), Bit8u(x >> 24) };
	const Bit8u ybyte[4] = { Bit8u(y & 0xff), Bit8u((y >> 8) & 0xff), Bit8u((y >> 16) & 0xff), Bit8u(y >> 24) };

	Bit32u p0dword = Bit32u(xbyte[0]) * Bit32u(ybyte[0]);
	Bit32u p1dword = Bit32u(xbyte[1]) * Bit32u(ybyte[1]);
	Bit32u p2dword = Bit32u(xbyte[2]) * Bit32u(ybyte[2]);
	Bit32u p3dword = Bit32u(xbyte[3]) * Bit32u(ybyte[3]);

	return p0dword + p1dword + p2dword + p3dword;
}

#include "softfloat.h"
#include "bf16.h"

extern softfloat_status_t prepare_ne_softfloat_status_helper(bool denormals_are_zeros);

extern float32 convert_ne_fp16_to_fp32(float16 op);

BX_CPP_INLINE float32 f32_silence_snan(float32 a)
{
	if (f32_isNaN(a))
		a = convert_to_QNaN(a);
	return a;
}


#include "ia_opcodes.h"

#endif // BX_SUPPORT_AMX