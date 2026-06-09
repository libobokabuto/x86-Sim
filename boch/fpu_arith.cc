#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_SUPPORT_FPU

#include "softfloat.h"

softfloat_status_t i387cw_to_softfloat_status_word(Bit16u control_word)
{
    softfloat_status_t status;

    int precision = control_word & FPU_CW_PC;

    switch (precision)
    {
    case FPU_PR_32_BITS:
        status.extF80_roundingPrecision = 32;
        break;
    case FPU_PR_64_BITS:
        status.extF80_roundingPrecision = 64;
        break;
    case FPU_PR_80_BITS:
        status.extF80_roundingPrecision = 80;
        break;
    default:
        /* With the precision control bits set to 01 "(reserved)", a
           real CPU behaves as if the precision control bits were
           set to 11 "80 bits" */
        status.extF80_roundingPrecision = 80;
    }

    status.softfloat_exceptionFlags = 0; // clear exceptions before execution
    status.softfloat_roundingMode = (control_word & FPU_CW_RC) >> 10;
    status.softfloat_flush_underflow_to_zero = 0;
    status.softfloat_suppressException = 0;
    status.softfloat_exceptionMasks = control_word & FPU_CW_Exceptions_Mask;
    status.softfloat_denormals_are_zeros = 0;

    return status;
}

#include "fpu_trans.h"

floatx80 FPU_handle_NaN(floatx80 a, int aIsNaN, float32 b32, int bIsNaN, softfloat_status_t& status)
{
    int aIsSignalingNaN = extF80_isSignalingNaN(a);
    int bIsSignalingNaN = f32_isSignalingNaN(b32);

    if (aIsSignalingNaN | bIsSignalingNaN)
        softfloat_raiseFlags(&status, softfloat_flag_invalid);

    // propagate QNaN to SNaN
    a = softfloat_propagateNaNExtF80UI(a.signExp, a.signif, 0, 0, &status);

    if (aIsNaN & !bIsNaN) return a;

    // float32 is NaN so conversion will propagate SNaN to QNaN and raise
    // appropriate exception flags
    floatx80 b = f32_to_extF80(b32, &status);

    if (aIsSignalingNaN) {
        if (bIsSignalingNaN) goto returnLargerSignificand;
        return bIsNaN ? b : a;
    }
    else if (aIsNaN) {
        if (bIsSignalingNaN) return a;
    returnLargerSignificand:
        if (a.signif < b.signif) return b;
        if (b.signif < a.signif) return a;
        return (a.signExp < b.signExp) ? a : b;
    }
    else {
        return b;
    }
}

bool FPU_handle_NaN(floatx80 a, float32 b, floatx80& r, softfloat_status_t& status)
{
    if (extF80_isUnsupported(a)) {
        softfloat_raiseFlags(&status, softfloat_flag_invalid);
        r = floatx80_default_nan;
        return true;
    }

    int aIsNaN = extF80_isNaN(a), bIsNaN = f32_isNaN(b);
    if (aIsNaN | bIsNaN) {
        r = FPU_handle_NaN(a, aIsNaN, b, bIsNaN, status);
        return true;
    }
    return false;
}

floatx80 FPU_handle_NaN(floatx80 a, int aIsNaN, float64 b64, int bIsNaN, softfloat_status_t& status)
{
    int aIsSignalingNaN = extF80_isSignalingNaN(a);
    int bIsSignalingNaN = f64_isSignalingNaN(b64);

    if (aIsSignalingNaN | bIsSignalingNaN)
        softfloat_raiseFlags(&status, softfloat_flag_invalid);

    // propagate QNaN to SNaN
    a = softfloat_propagateNaNExtF80UI(a.signExp, a.signif, 0, 0, &status);

    if (aIsNaN & !bIsNaN) return a;

    // float64 is NaN so conversion will propagate SNaN to QNaN and raise
    // appropriate exception flags
    floatx80 b = f64_to_extF80(b64, &status);

    if (aIsSignalingNaN) {
        if (bIsSignalingNaN) goto returnLargerSignificand;
        return bIsNaN ? b : a;
    }
    else if (aIsNaN) {
        if (bIsSignalingNaN) return a;
    returnLargerSignificand:
        if (a.signif < b.signif) return b;
        if (b.signif < a.signif) return a;
        return (a.signExp < b.signExp) ? a : b;
    }
    else {
        return b;
    }
}

bool FPU_handle_NaN(floatx80 a, float64 b, floatx80& r, softfloat_status_t& status)
{
    if (extF80_isUnsupported(a)) {
        softfloat_raiseFlags(&status, softfloat_flag_invalid);
        r = floatx80_default_nan;
        return true;
    }

    int aIsNaN = extF80_isNaN(a), bIsNaN = f64_isNaN(b);
    if (aIsNaN | bIsNaN) {
        r = FPU_handle_NaN(a, aIsNaN, b, bIsNaN, status);
        return true;
    }
    return false;
}

#endif