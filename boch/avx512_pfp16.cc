#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_SUPPORT_EVEX

extern softfloat_status_t mxcsr_to_softfloat_status_word(bx_mxcsr_t mxcsr);
extern void mxcsr_to_softfloat_status_word_imm_override(softfloat_status_t& status, Bit8u immb);

#include "softfloat-compare.h"
#include "simd_int.h"
#include "simd_pfp.h"

extern float16_compare_method avx_compare16[32];

#include "ia_opcodes.h"

#include "softfloat-specialize.h"

static BX_CPP_INLINE int f16_fpclass(float16 op, int selector, int daz)
{
    extern int fpclass(softfloat_class_t op_class, int sign, int selector);

    if (daz)
        op = f16_denormal_to_zero(op);

    return fpclass(f16_class(op), f16_sign(op), selector);
}

static BX_CPP_INLINE float16 float16_reduce(float16 a, Bit8u scale, softfloat_status_t& status)
{
    if (a == float16_negative_inf || a == float16_positive_inf)
        return 0;

    float16 tmp = f16_roundToInt(a, scale, &status);
    return f16_sub(a, tmp, &status);
}


#endif