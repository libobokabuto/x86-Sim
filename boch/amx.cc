#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_SUPPORT_AMX

#include "amx.h"

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

#include "ia_opcodes.h"

#endif // BX_SUPPORT_AMX