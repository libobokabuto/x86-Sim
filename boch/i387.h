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

#include "tag_w.h"
#include "status_w.h"
#include "control_w.h"

struct BOCHSAPI_MSVCONLY i387_t
{
	i387_t() {}

public:
    void    init();
    void    reset();

    int     is_IA_masked() const { return (cwd & FPU_CW_Invalid); }

    Bit16u    get_control_word() const { return cwd; }
    Bit16u    get_tag_word() const { return twd; }
    Bit16u    get_status_word() const { return (swd & ~FPU_SW_Top & 0xFFFF) | ((tos << 11) & FPU_SW_Top); }
    Bit16u    get_partial_status() const { return swd; }

    void      FPU_pop();
    void      FPU_push();

    void      FPU_settagi(int tag, int stnr);
    void      FPU_settagi_valid(int stnr);
    int       FPU_gettagi(int stnr);

    floatx80  FPU_read_regi(int stnr) { return st_space[(tos + stnr) & 7]; }
    void      FPU_save_regi(floatx80 reg, int stnr);
    void      FPU_save_regi(floatx80 reg, int tag, int stnr);

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

extern int FPU_tagof(const floatx80& reg);
extern Bit16u unpack_FPU_TW(const i387_t* i387, Bit16u tag_byte);\

#define IS_TAG_EMPTY(i)                                                 \
  ((BX_CPU_THIS_PTR the_i387.FPU_gettagi(i)) == FPU_Tag_Empty)

#define BX_READ_FPU_REG(i)                                              \
  (BX_CPU_THIS_PTR the_i387.FPU_read_regi(i))

#define BX_WRITE_FPU_REG(value, i)                                      \
    BX_CPU_THIS_PTR the_i387.FPU_save_regi((value), (i));

#define BX_WRITE_FPU_REGISTER_AND_TAG(value, tag, i)                    \
    BX_CPU_THIS_PTR the_i387.FPU_save_regi((value), (tag), (i));

BX_CPP_INLINE int i387_t::FPU_gettagi(int stnr)
{
    return (twd >> (((stnr + tos) & 7) * 2)) & 3;
}

BX_CPP_INLINE void i387_t::FPU_settagi_valid(int stnr)
{
    int regnr = (stnr + tos) & 7;
    twd &= ~(3 << (regnr * 2));     // FPU_Tag_Valid == '00
}

BX_CPP_INLINE void i387_t::FPU_settagi(int tag, int stnr)
{
    int regnr = (stnr + tos) & 7;
    twd &= ~(3 << (regnr * 2));
    twd |= (tag & 3) << (regnr * 2);
}

BX_CPP_INLINE void i387_t::FPU_push(void)
{
    tos = (tos - 1) & 7;
}

BX_CPP_INLINE void i387_t::FPU_pop(void)
{
    twd |= 3 << (tos * 2);
    tos = (tos + 1) & 7;
}

// it is only possisble to read FPU tag word through certain
// instructions like FNSAVE, and they update tag word to its
// real value anyway
BX_CPP_INLINE void i387_t::FPU_save_regi(floatx80 reg, int stnr)
{
    st_space[(stnr + tos) & 7] = reg;
    FPU_settagi_valid(stnr);
}

BX_CPP_INLINE void i387_t::FPU_save_regi(floatx80 reg, int tag, int stnr)
{
    st_space[(stnr + tos) & 7] = reg;
    FPU_settagi(tag, stnr);
}

#include <string.h>

BX_CPP_INLINE void i387_t::init()
{
    cwd = 0x037F;
    swd = 0;
    tos = 0;
    twd = 0xFFFF;
    foo = 0;
    fip = 0;
    fcs = 0;
    fds = 0;
    fdp = 0;
}


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