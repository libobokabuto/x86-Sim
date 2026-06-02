#pragma once

#if BX_SUPPORT_FPU

#include "softfloat_types.h"

#define BX_FPU_REG(index) \
    (BX_CPU_THIS_PTR the_i387.st_space[index & 0x7])

#if defined(NEED_CPU_REG_SHORTCUTS)
#define FPU_PARTIAL_STATUS     (BX_CPU_THIS_PTR the_i387.swd)
#define FPU_CONTROL_WORD       (BX_CPU_THIS_PTR the_i387.cwd)
#define FPU_TAG_WORD           (BX_CPU_THIS_PTR the_i387.twd)
#define FPU_TOS                (BX_CPU_THIS_PTR the_i387.tos)
#endif

struct BOCHSAPI_MSVCONLY i387_t
{
	i387_t() {}

public:
    void    reset();
public:
    Bit16u cwd;     // control word
    Bit16u swd;     // status word
    Bit16u twd;     // tag word
    Bit16u foo;     // last instruction opcode

    bx_address fip;
    bx_address fdp;
    Bit16u fcs;
    Bit16u fds;

    floatx80 st_space[8];

    unsigned char tos;
    unsigned char align1;
    unsigned char align2;
    unsigned char align3;
};

BX_CPP_INLINE void i387_t::reset()
{
    cwd = 0x0040;
    swd = 0;
    tos = 0;
    twd = 0x5555;
    foo = 0;
    fip = 0;
    fcs = 0;
    fds = 0;
    fdp = 0;

    memset(st_space, 0, sizeof(floatx80) * 8);
}

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

#endif    