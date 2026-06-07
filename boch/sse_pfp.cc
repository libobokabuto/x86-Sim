#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_CPU_LEVEL >= 6

#include "softfloat-compare.h"

#include "simd_pfp.h"
#include "simd_int.h"

void BX_CPU_C::check_exceptionsSSE(int exceptions_flags)
{
    exceptions_flags &= MXCSR_EXCEPTIONS;
    int unmasked = ~(MXCSR.get_exceptions_masks()) & exceptions_flags;
    // unmasked pre-computational exception detected (#IA, #DE or #DZ)
    if (unmasked & 0x7) exceptions_flags &= 0x7;
    MXCSR.set_exceptions(exceptions_flags);

    if (unmasked)
    {
        if (BX_CPU_THIS_PTR cr4.get_OSXMMEXCPT())
            exception(BX_XM_EXCEPTION, 0);
        else
            exception(BX_UD_EXCEPTION, 0);
    }
}

softfloat_status_t mxcsr_to_softfloat_status_word(bx_mxcsr_t mxcsr)
{
    softfloat_status_t status;

    status.softfloat_exceptionFlags = 0; // clear exceptions before execution
    status.softfloat_roundingMode = mxcsr.get_rounding_mode();
    // if underflow is masked and FUZ is 1, set it to 1, else to 0
    status.softfloat_flush_underflow_to_zero =
        (mxcsr.get_flush_masked_underflow() && mxcsr.get_UM()) ? 1 : 0;
    status.softfloat_exceptionMasks = mxcsr.get_exceptions_masks();
    status.softfloat_suppressException = 0;
    status.softfloat_denormals_are_zeros = mxcsr.get_DAZ();

    return status;
}

void mxcsr_to_softfloat_status_word_imm_override(softfloat_status_t& status, Bit8u control)
{
    // override MXCSR rounding mode with control coming from imm8
    if ((control & 0x4) == 0)
        status.softfloat_roundingMode = control & 0x3;
    // ignore precision exception result
    if (control & 0x8)
        status.softfloat_suppressException |= softfloat_flag_inexact;
}

#if BX_SUPPORT_EVEX
// implement SAE and EVEX-encoded (embedded) rounding control
void softfloat_status_word_rc_override(softfloat_status_t& status, bxInstruction_c* i)
{
    /* must be VL512 otherwise EVEX.LL encodes vector length */
    if (i->modC0() && i->getEvexb() && ((i->getVL() == BX_VL512) || (!i->getEvexU() && (i->getVL() == BX_VL256)))) {
        status.softfloat_roundingMode = i->getRC();
        status.softfloat_suppressException = softfloat_all_exceptions_mask;
        status.softfloat_exceptionMasks = softfloat_all_exceptions_mask;
    }
}
#endif

/* Comparison predicate for CMPSS/CMPPS instructions */
static float32_compare_method compare32[8] = {
  f32_eq_ordered_quiet,
  f32_lt_ordered_signalling,
  f32_le_ordered_signalling,
  f32_unordered_quiet,
  f32_neq_unordered_quiet,
  f32_nlt_unordered_signalling,
  f32_nle_unordered_signalling,
  f32_ordered_quiet
};

/* Comparison predicate for CMPSD/CMPPD instructions */
static float64_compare_method compare64[8] = {
  f64_eq_ordered_quiet,
  f64_lt_ordered_signalling,
  f64_le_ordered_signalling,
  f64_unordered_quiet,
  f64_neq_unordered_quiet,
  f64_nlt_unordered_signalling,
  f64_nle_unordered_signalling,
  f64_ordered_quiet
};

#endif

#if BX_CPU_LEVEL >= 6

#define SSE_SCALAR_SINGLE_FP_CPU_LEVEL6(HANDLER, func)                                          \
  /* SSE packed shift instruction */                                                            \
  void BX_CPP_AttrRegparmN(1) BX_CPU_C:: HANDLER (bxInstruction_c *i)                           \
  {                                                                                             \
    float32 op1 = BX_READ_XMM_REG_LO_DWORD(i->dst()), op2 = BX_READ_XMM_REG_LO_DWORD(i->src()); \
                                                                                                \
    softfloat_status_t status = mxcsr_to_softfloat_status_word(MXCSR);                          \
    op1 = (func)(op1, op2, &status);                                                            \
    check_exceptionsSSE(softfloat_getExceptionFlags(&status));                                  \
    BX_WRITE_XMM_REG_LO_DWORD(i->dst(), op1);                                                   \
    BX_NEXT_INSTR(i);                                                                           \
  }                                                                                             \

#else

#define SSE_SCALAR_SINGLE_FP_CPU_LEVEL6(HANDLER, func)                                          \
  /* SSE instruction with two src operands */                                                   \
  void BX_CPP_AttrRegparmN(1) BX_CPU_C :: HANDLER (bxInstruction_c *i)                          \
  {                                                                                             \
    BX_NEXT_INSTR(i);                                                                           \
  }

#endif

SSE_SCALAR_SINGLE_FP_CPU_LEVEL6(ADDSS_VssWssR, f32_add);
SSE_SCALAR_SINGLE_FP_CPU_LEVEL6(SUBSS_VssWssR, f32_sub);
SSE_SCALAR_SINGLE_FP_CPU_LEVEL6(MULSS_VssWssR, f32_mul);
SSE_SCALAR_SINGLE_FP_CPU_LEVEL6(DIVSS_VssWssR, f32_div);
SSE_SCALAR_SINGLE_FP_CPU_LEVEL6(MINSS_VssWssR, f32_min);
SSE_SCALAR_SINGLE_FP_CPU_LEVEL6(MAXSS_VssWssR, f32_max);

#if BX_CPU_LEVEL >= 6

#define SSE_SCALAR_DOUBLE_FP_CPU_LEVEL6(HANDLER, func)                                          \
  /* SSE packed shift instruction */                                                            \
  void BX_CPP_AttrRegparmN(1) BX_CPU_C:: HANDLER (bxInstruction_c *i)                           \
  {                                                                                             \
    float64 op1 = BX_READ_XMM_REG_LO_QWORD(i->dst()), op2 = BX_READ_XMM_REG_LO_QWORD(i->src()); \
                                                                                                \
    softfloat_status_t status = mxcsr_to_softfloat_status_word(MXCSR);                          \
    op1 = (func)(op1, op2, &status);                                                            \
    check_exceptionsSSE(softfloat_getExceptionFlags(&status));                                  \
    BX_WRITE_XMM_REG_LO_QWORD(i->dst(), op1);                                                   \
    BX_NEXT_INSTR(i);                                                                           \
  }                                                                                             \

#else

#define SSE_SCALAR_DOUBLE_FP_CPU_LEVEL6(HANDLER, func)                                          \
  /* SSE instruction with two src operands */                                                   \
  void BX_CPP_AttrRegparmN(1) BX_CPU_C :: HANDLER (bxInstruction_c *i)                          \
  {                                                                                             \
    BX_NEXT_INSTR(i);                                                                           \
  }

#endif

SSE_SCALAR_DOUBLE_FP_CPU_LEVEL6(ADDSD_VsdWsdR, f64_add);
SSE_SCALAR_DOUBLE_FP_CPU_LEVEL6(SUBSD_VsdWsdR, f64_sub);
SSE_SCALAR_DOUBLE_FP_CPU_LEVEL6(MULSD_VsdWsdR, f64_mul);
SSE_SCALAR_DOUBLE_FP_CPU_LEVEL6(DIVSD_VsdWsdR, f64_div);
SSE_SCALAR_DOUBLE_FP_CPU_LEVEL6(MINSD_VsdWsdR, f64_min);
SSE_SCALAR_DOUBLE_FP_CPU_LEVEL6(MAXSD_VsdWsdR, f64_max);