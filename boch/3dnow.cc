#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_SUPPORT_3DNOW

#if BX_CPU_LEVEL >= 5

#include "softfloat.h"

softfloat_status_t prepare_softfloat_status_word_3dnow(int rounding_mode = softfloat_round_near_even)
{
    softfloat_status_t status;

    status.softfloat_exceptionFlags = 0; // clear exceptions before execution
    status.softfloat_roundingMode = rounding_mode;
    status.softfloat_flush_underflow_to_zero = true;
    status.softfloat_exceptionMasks = softfloat_all_exceptions_mask;
    status.softfloat_suppressException = softfloat_all_exceptions_mask;
    status.softfloat_denormals_are_zeros = true;

    return status;
}

//////////////////////////////////////////////////////////////////////////////////////////
// helper functions
//////////////////////////////////////////////////////////////////////////////////////////

BX_CPP_INLINE static float32 f32_add_3dnow(float32 a, float32 b)
{
    int a_is_zero = (a << 1) == 0;
    int b_is_zero = (b << 1) == 0;

    if (a_is_zero) {
        if (b_is_zero) {
            // result +0/-0 with sign of logical AND of signs of both operands
            return a & b & 0x80000000;
        }
        return b; // otherwise take src2
    }

    if (b_is_zero) {
        return a; // take src1
    }

    static softfloat_status_t status = prepare_softfloat_status_word_3dnow(softfloat_round_near_even); // Note, actual rounding mode is not specified by 3dNow! manual

    // Note that Inf/NaN handling is not documented in 3Dnow! manuals
    // The manual doesn't specify what result going to be if one or both arguments are Inf/NaN (undefined behavior)
    // This implementation choose IEEE-754 behavior which might not necessary match actual AMD's hardware
    return f32_add(a, b, &status);
}

BX_CPP_INLINE static float32 f32_sub_3dnow(float32 a, float32 b)
{
    int a_is_zero = (a << 1) == 0;
    int b_is_zero = (b << 1) == 0;

    if (a_is_zero) {
        if (b_is_zero) {
            // result +0/-0 with sign of logical AND of the sign of src1 and inverse of the sign of src2
            return a & ~b & 0x80000000;
        }
        return b ^ 0x80000000; // otherwise take -src2
    }

    if (b_is_zero) {
        return a; // take src1
    }

    static softfloat_status_t status = prepare_softfloat_status_word_3dnow(softfloat_round_near_even); // Note, actual rounding mode is not specified by 3dNow! manual

    // Note that Inf/NaN handling is not documented in 3Dnow! manuals
    // The manual doesn't specify what result going to be if one or both arguments are Inf/NaN (undefined behavior)
    // This implementation choose IEEE-754 behavior which might not necessary match actual AMD's hardware
    return f32_sub(a, b, &status);
}

BX_CPP_INLINE static float32 f32_mul_3dnow(float32 a, float32 b)
{
    // if either a or b is zero
    if (((a | b) << 1) == 0) {
        // result is zero with sign of logical XOR of signs of both operands
        return (a ^ b) & 0x80000000;
    }

    static softfloat_status_t status = prepare_softfloat_status_word_3dnow(softfloat_round_near_even); // Note, actual rounding mode is not specified by 3dNow! manual

    // Note that Inf/NaN handling is not documented in 3Dnow! manuals
    // The manual doesn't specify what result going to be if one or both arguments are Inf/NaN (undefined behavior)
    // This implementation choose IEEE-754 behavior which might not necessary match actual AMD's hardware
    return f32_mul(a, b, &status);
}

BX_CPP_INLINE static float32 f32_min_3dnow(float32 a, float32 b)
{
    if (a == 0x80000000) a = 0; // remove sign of zero
    if (b == 0x80000000) b = 0; // remove sign of zero

    // both arguments zero: return +0
    // negative value and zero: return +0

    static softfloat_status_t status = prepare_softfloat_status_word_3dnow();

    // Note that Inf/NaN handling is not documented in 3Dnow! manuals
    // The manual doesn't specify what result going to be if one or both arguments are Inf/NaN (undefined behavior)
    // This implementation choose IEEE-754 behavior which might not necessary match actual AMD's hardware
    return f32_min(a, b, &status);
}

BX_CPP_INLINE static float32 f32_max_3dnow(float32 a, float32 b)
{
    if (a == 0x80000000) a = 0; // remove sign of zero
    if (b == 0x80000000) b = 0; // remove sign of zero

    // both arguments zero: return +0
    // negative value and zero: return +0

    static softfloat_status_t status = prepare_softfloat_status_word_3dnow();

    // Note that Inf/NaN handling is not documented in 3Dnow! manuals
    // The manual doesn't specify what result going to be if one or both arguments are Inf/NaN (undefined behavior)
    // This implementation choose IEEE-754 behavior which might not necessary match actual AMD's hardware
    return f32_max(a, b, &status);
}

// 3dnow! handling of PFCMPEQ/PFCMPGT/PFCMPGE
BX_CPP_INLINE static int f32_compare_3dnow(float32 a, float32 b)
{
    a = f32_denormal_to_zero(a);
    b = f32_denormal_to_zero(b);

    if ((a == b) || ((uint32_t)((a | b) << 1) == 0)) return softfloat_relation_equal;

    int signA = f32_sign(a);
    int signB = f32_sign(b);
    if (signA != signB)
        return (signA) ? softfloat_relation_less : softfloat_relation_greater;

    if (signA ^ (a < b)) return softfloat_relation_less;
    return softfloat_relation_greater;
}

#endif

#endif