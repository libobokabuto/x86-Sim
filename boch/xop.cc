#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_SUPPORT_AVX

#include "softfloat.h"

extern softfloat_status_t mxcsr_to_softfloat_status_word(bx_mxcsr_t mxcsr);

#include "simd_int.h"
#include "simd_compare.h"

typedef void (*simd_compare_method)(BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2);

// comparison predicate for PCOMB
static simd_compare_method xop_compare8[8] = {
  xmm_pcmpltb,
  xmm_pcmpleb,
  xmm_pcmpgtb,
  xmm_pcmpgeb,
  xmm_pcmpeqb,
  xmm_pcmpneb,
  xmm_pcmpfalse,
  xmm_pcmptrue
};

// comparison predicate for PCOMUB
static simd_compare_method xop_compare8u[8] = {
  xmm_pcmpltub,
  xmm_pcmpleub,
  xmm_pcmpgtub,
  xmm_pcmpgeub,
  xmm_pcmpeqb,
  xmm_pcmpneb,
  xmm_pcmpfalse,
  xmm_pcmptrue
};

// comparison predicate for PCOMW
static simd_compare_method xop_compare16[8] = {
  xmm_pcmpltw,
  xmm_pcmplew,
  xmm_pcmpgtw,
  xmm_pcmpgew,
  xmm_pcmpeqw,
  xmm_pcmpnew,
  xmm_pcmpfalse,
  xmm_pcmptrue
};

// comparison predicate for PCOMUW
static simd_compare_method xop_compare16u[8] = {
  xmm_pcmpltuw,
  xmm_pcmpleuw,
  xmm_pcmpgtuw,
  xmm_pcmpgeuw,
  xmm_pcmpeqw,
  xmm_pcmpnew,
  xmm_pcmpfalse,
  xmm_pcmptrue
};

// comparison predicate for PCOMD
static simd_compare_method xop_compare32[8] = {
  xmm_pcmpltd,
  xmm_pcmpled,
  xmm_pcmpgtd,
  xmm_pcmpged,
  xmm_pcmpeqd,
  xmm_pcmpned,
  xmm_pcmpfalse,
  xmm_pcmptrue
};

// comparison predicate for PCOMUD
static simd_compare_method xop_compare32u[8] = {
  xmm_pcmpltud,
  xmm_pcmpleud,
  xmm_pcmpgtud,
  xmm_pcmpgeud,
  xmm_pcmpeqd,
  xmm_pcmpned,
  xmm_pcmpfalse,
  xmm_pcmptrue
};

// comparison predicate for PCOMQ
static simd_compare_method xop_compare64[8] = {
  xmm_pcmpltq,
  xmm_pcmpleq,
  xmm_pcmpgtq,
  xmm_pcmpgeq,
  xmm_pcmpeqq,
  xmm_pcmpneq,
  xmm_pcmpfalse,
  xmm_pcmptrue
};

// comparison predicate for PCOMUQ
static simd_compare_method xop_compare64u[8] = {
  xmm_pcmpltuq,
  xmm_pcmpleuq,
  xmm_pcmpgtuq,
  xmm_pcmpgeuq,
  xmm_pcmpeqq,
  xmm_pcmpneq,
  xmm_pcmpfalse,
  xmm_pcmptrue
};

typedef Bit8u(*vpperm_operation)(Bit8u byte);

BX_CPP_INLINE Bit8u vpperm_bit_reverse(Bit8u v8)
{
    return  (v8 >> 7) |
        ((v8 >> 5) & 0x02) |
        ((v8 >> 3) & 0x04) |
        ((v8 >> 1) & 0x08) |
        ((v8 << 1) & 0x10) |
        ((v8 << 3) & 0x20) |
        ((v8 << 5) & 0x40) |
        (v8 << 7);
}

BX_CPP_INLINE Bit8u vpperm_noop(Bit8u v8) { return v8; }
BX_CPP_INLINE Bit8u vpperm_invert(Bit8u v8) { return ~v8; }
BX_CPP_INLINE Bit8u vpperm_invert_bit_reverse(Bit8u v8) { return vpperm_bit_reverse(~v8); }
BX_CPP_INLINE Bit8u vpperm_zeros(Bit8u v8) { return 0; }
BX_CPP_INLINE Bit8u vpperm_ones(Bit8u v8) { return 0xff; }
BX_CPP_INLINE Bit8u vpperm_replicate_msb(Bit8u v8) { return (((Bit8s)v8) >> 7); }
BX_CPP_INLINE Bit8u vpperm_invert_replicate_msb(Bit8u v8) { return vpperm_replicate_msb(~v8); }

// logical operation for VPPERM
static vpperm_operation vpperm_op[8] = {
  vpperm_noop,
  vpperm_invert,
  vpperm_bit_reverse,
  vpperm_invert_bit_reverse,
  vpperm_zeros,
  vpperm_ones,
  vpperm_replicate_msb,
  vpperm_invert_replicate_msb
};

BX_CPP_INLINE Bit64s add_saturate64(Bit64s a, Bit64s b)
{
    Bit64s r = a + b;
    Bit64u overflow = GET_ADD_OVERFLOW(a, b, r, BX_CONST64(0x8000000000000000));
    if (!overflow) return r;
    // signed overflow detected, saturate
    if (a > 0) overflow--;
    return (Bit64s)overflow;
}

#endif