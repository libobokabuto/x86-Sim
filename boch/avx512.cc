#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_SUPPORT_EVEX

#include "simd_int.h"
#include "simd_compare.h"
#include "scalar_arith.h"

// compare

typedef Bit32u(*avx512_compare_method)(const BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2);

static avx512_compare_method avx512_compare8[8] = {
  xmm_pcmpeqb_mask,     // equal
  xmm_pcmpltb_mask,     // less than
  xmm_pcmpleb_mask,     // less or equal than
  xmm_pcmpfalse_mask,   // false
  xmm_pcmpneb_mask,     // not equal
  xmm_pcmpgeb_mask,     // not less than => greater or equal than
  xmm_pcmpgtb_mask,     // not less or equal than => greater than
  xmm_pcmptrueb_mask    // true
};

static avx512_compare_method avx512_compare16[8] = {
  xmm_pcmpeqw_mask,     // equal
  xmm_pcmpltw_mask,     // less than
  xmm_pcmplew_mask,     // less or equal than
  xmm_pcmpfalse_mask,   // false
  xmm_pcmpnew_mask,     // not equal
  xmm_pcmpgew_mask,     // not less than => greater or equal than
  xmm_pcmpgtw_mask,     // not less or equal than => greater than
  xmm_pcmptruew_mask    // true
};

static avx512_compare_method avx512_compare32[8] = {
  xmm_pcmpeqd_mask,     // equal
  xmm_pcmpltd_mask,     // less than
  xmm_pcmpled_mask,     // less or equal than
  xmm_pcmpfalse_mask,   // false
  xmm_pcmpned_mask,     // not equal
  xmm_pcmpged_mask,     // not less than => greater or equal than
  xmm_pcmpgtd_mask,     // not less or equal than => greater than
  xmm_pcmptrued_mask    // true
};

static avx512_compare_method avx512_compare64[8] = {
  xmm_pcmpeqq_mask,     // equal
  xmm_pcmpltq_mask,     // less than
  xmm_pcmpleq_mask,     // less or equal than
  xmm_pcmpfalse_mask,   // false
  xmm_pcmpneq_mask,     // not equal
  xmm_pcmpgeq_mask,     // not less than => greater or equal than
  xmm_pcmpgtq_mask,     // not less or equal than => greater than
  xmm_pcmptrueq_mask    // true
};

static avx512_compare_method avx512_compare8u[8] = {
  xmm_pcmpeqb_mask,     // equal
  xmm_pcmpltub_mask,    // less than
  xmm_pcmpleub_mask,    // less or equal than
  xmm_pcmpfalse_mask,   // false
  xmm_pcmpneb_mask,     // not equal
  xmm_pcmpgeub_mask,    // not less than => greater or equal than
  xmm_pcmpgtub_mask,    // not less or equal than => greater than
  xmm_pcmptrueb_mask    // true
};

static avx512_compare_method avx512_compare16u[8] = {
  xmm_pcmpeqw_mask,     // equal
  xmm_pcmpltuw_mask,    // less than
  xmm_pcmpleuw_mask,    // less or equal than
  xmm_pcmpfalse_mask,   // false
  xmm_pcmpnew_mask,     // not equal
  xmm_pcmpgeuw_mask,    // not less than => greater or equal than
  xmm_pcmpgtuw_mask,    // not less or equal than => greater than
  xmm_pcmptruew_mask    // true
};

static avx512_compare_method avx512_compare32u[8] = {
  xmm_pcmpeqd_mask,     // equal
  xmm_pcmpltud_mask,    // less than
  xmm_pcmpleud_mask,    // less or equal than
  xmm_pcmpfalse_mask,   // false
  xmm_pcmpned_mask,     // not equal
  xmm_pcmpgeud_mask,    // not less than => greater or equal than
  xmm_pcmpgtud_mask,    // not less or equal than => greater than
  xmm_pcmptrued_mask    // true
};

static avx512_compare_method avx512_compare64u[8] = {
  xmm_pcmpeqq_mask,     // equal
  xmm_pcmpltuq_mask,    // less than
  xmm_pcmpleuq_mask,    // less or equal than
  xmm_pcmpfalse_mask,   // false
  xmm_pcmpneq_mask,     // not equal
  xmm_pcmpgeuq_mask,    // not less than => greater or equal than
  xmm_pcmpgtuq_mask,    // not less or equal than => greater than
  xmm_pcmptrueq_mask    // true
};

BX_CPP_INLINE Bit32u ternlogd_scalar(Bit32u op1, Bit32u op2, Bit32u op3, unsigned imm8)
{
    Bit32u result = 0;

    for (unsigned bit = 0; bit < 32; bit++) {
        unsigned tmp = (op1 >> bit) & 0x1;
        tmp <<= 1;
        tmp |= (op2 >> bit) & 0x1;
        tmp <<= 1;
        tmp |= (op3 >> bit) & 0x1;

        result |= ((Bit32u)((imm8 >> tmp) & 0x1)) << bit;
    }

    return result;
}

BX_CPP_INLINE Bit64u ternlogq_scalar(Bit64u op1, Bit64u op2, Bit64u op3, unsigned imm8)
{
    Bit64u result = 0;

    for (unsigned bit = 0; bit < 64; bit++) {
        unsigned tmp = (op1 >> bit) & 0x1;
        tmp <<= 1;
        tmp |= (op2 >> bit) & 0x1;
        tmp <<= 1;
        tmp |= (op3 >> bit) & 0x1;

        result |= ((Bit64u)((imm8 >> tmp) & 0x1)) << bit;
    }

    return result;
}

BX_CPP_INLINE Bit64u pmultishiftqb_scalar(Bit64u val_64, Bit64u control)
{
    // use packed register as 64-bit value with convinient accessors
    BxPackedRegister result;

    for (unsigned n = 0; n < 8; n++, control >>= 8) {
        unsigned ctrl = (control & 0x3f);
        Bit64u tmp = val_64;
        if (ctrl != 0)
            tmp = (val_64 << (64 - ctrl)) | (val_64 >> ctrl);
        result.ubyte(n) = tmp & 0xff;
    }

    return MMXUQ(result);
}


#endif
