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