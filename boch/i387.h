#pragma once
typedef union bx_packed_reg_t {
	Bit8s   _sbyte[8];
	Bit16s  _s16[4];
	Bit32s  _s32[2];
	Bit64s  _s64;
	Bit8u   _ubyte[8];
	Bit16u  _u16[4];
	Bit32u  _u32[2];
	Bit64u  _u64;
public:
	bx_packed_reg_t() {}
	bx_packed_reg_t(Bit64u val) : _u64(val) {}
	bx_packed_reg_t(Bit64s val) : _s64(val) {}
} BxPackedRegister;


#ifdef BX_BIG_ENDIAN
#define s64      _s64
#define s32(i)   _s32[1 - (i)]
#define s16(i)   _s16[3 - (i)]
#define sbyte(i) _sbyte[7 - (i)]
#define ubyte(i) _ubyte[7 - (i)]
#define u16(i)   _u16[3 - (i)]
#define u32(i)   _u32[1 - (i)]
#define u64      _u64
#else
#define s64      _s64
#define s32(i)   _s32[(i)]
#define s16(i)   _s16[(i)]
#define sbyte(i) _sbyte[(i)]
#define ubyte(i) _ubyte[(i)]
#define u16(i)   _u16[(i)]
#define u32(i)   _u32[(i)]
#define u64      _u64
#endif