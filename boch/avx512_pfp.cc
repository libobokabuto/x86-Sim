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

extern float32_compare_method avx_compare32[32];
extern float64_compare_method avx_compare64[32];

enum {
	BX_FIXUPIMM_QNAN_TOKEN = 0,
	BX_FIXUPIMM_SNAN_TOKEN = 1,
	BX_FIXUPIMM_ZERO_VALUE_TOKEN = 2,
	BX_FIXUPIMM_POS_ONE_VALUE_TOKEN = 3,
	BX_FIXUPIMM_NEG_INF_TOKEN = 4,
	BX_FIXUPIMM_POS_INF_TOKEN = 5,
	BX_FIXUPIMM_NEG_VALUE_TOKEN = 6,
	BX_FIXUPIMM_POS_VALUE_TOKEN = 7
};

#include "softfloat-specialize.h"

const float32 float32_value_90 = 0x42b40000;
const float32 float32_pi_half = 0x3fc90fdb;
const float32 float32_positive_half = 0x3f000000;

const float64 float64_value_90 = BX_CONST64(0x4056800000000000);
const float64 float64_pi_half = BX_CONST64(0x3ff921fb54442d18);
const float64 float64_positive_half = BX_CONST64(0x3fe0000000000000);

float32 float32_fixupimm(float32 dst, float32 op1, Bit32u op2, unsigned imm8, softfloat_status_t& status)
{
    float32 tmp_op1 = op1;
    if (softfloat_denormalsAreZeros(&status))
        tmp_op1 = f32_denormal_to_zero(op1);

    softfloat_class_t op1_class = f32_class(tmp_op1);
    int sign = f32_sign(tmp_op1);
    unsigned token = 0, ie_fault_mask = 0, divz_fault_mask = 0;

    switch (op1_class)
    {
    case softfloat_zero:
        token = BX_FIXUPIMM_ZERO_VALUE_TOKEN;
        divz_fault_mask = 0x01;
        ie_fault_mask = 0x02;
        break;

    case softfloat_negative_inf:
        token = BX_FIXUPIMM_NEG_INF_TOKEN;
        ie_fault_mask = 0x20;
        break;

    case softfloat_positive_inf:
        token = BX_FIXUPIMM_POS_INF_TOKEN;
        ie_fault_mask = 0x80;
        break;

    case softfloat_SNaN:
        token = BX_FIXUPIMM_SNAN_TOKEN;
        ie_fault_mask = 0x10;
        break;

    case softfloat_QNaN:
        token = BX_FIXUPIMM_QNAN_TOKEN;
        break;

    case softfloat_denormal:
    case softfloat_normalized:
        if (tmp_op1 == float32_positive_one) {
            token = BX_FIXUPIMM_POS_ONE_VALUE_TOKEN;
            divz_fault_mask = 0x04;
            ie_fault_mask = 0x08;
        }
        else {
            if (sign) {
                token = BX_FIXUPIMM_NEG_VALUE_TOKEN;
                ie_fault_mask = 0x40;
            }
            else {
                token = BX_FIXUPIMM_POS_VALUE_TOKEN;
            }
        }
        break;

    default:
        break;
    }

    if (imm8 & ie_fault_mask)
        softfloat_raiseFlags(&status, softfloat_flag_invalid);

    if (imm8 & divz_fault_mask)
        softfloat_raiseFlags(&status, softfloat_flag_divbyzero);

    // access response table, each response is encoded with 4-bit value in the op2
    unsigned token_response = (op2 >> (token * 4)) & 0xf;

    switch (token_response) {
    case 0x1: // apply DAZ to the op1 value
        op1 = tmp_op1;
        break;
    case 0x2: op1 = convert_to_QNaN(tmp_op1); break;
    case 0x3: op1 = float32_default_nan; break;
    case 0x4: op1 = float32_negative_inf; break;
    case 0x5: op1 = float32_positive_inf; break;
    case 0x6:
        op1 = sign ? float32_negative_inf : float32_positive_inf;
        break;
    case 0x7: op1 = float32_negative_zero; break;
    case 0x8: op1 = float32_positive_zero; break;
    case 0x9: op1 = float32_negative_one; break;
    case 0xA: op1 = float32_positive_one; break;
    case 0xB: op1 = float32_positive_half; break;
    case 0xC: op1 = float32_value_90; break;
    case 0xD: op1 = float32_pi_half; break;
    case 0xE: op1 = float32_max_float; break;
    case 0xF: op1 = float32_min_float; break;
    default: // preserve the op1 value
        op1 = dst; break;
    }

    return op1;
}

float64 float64_fixupimm(float64 dst, float64 op1, Bit32u op2, unsigned imm8, softfloat_status_t& status)
{
    float64 tmp_op1 = op1;
    if (softfloat_denormalsAreZeros(&status))
        tmp_op1 = f64_denormal_to_zero(op1);

    softfloat_class_t op1_class = f64_class(tmp_op1);
    int sign = f64_sign(tmp_op1);
    unsigned token = 0, ie_fault_mask = 0, divz_fault_mask = 0;

    switch (op1_class)
    {
    case softfloat_zero:
        token = BX_FIXUPIMM_ZERO_VALUE_TOKEN;
        divz_fault_mask = 0x01;
        ie_fault_mask = 0x02;
        break;

    case softfloat_negative_inf:
        token = BX_FIXUPIMM_NEG_INF_TOKEN;
        ie_fault_mask = 0x20;
        break;

    case softfloat_positive_inf:
        token = BX_FIXUPIMM_POS_INF_TOKEN;
        ie_fault_mask = 0x80;
        break;

    case softfloat_SNaN:
        token = BX_FIXUPIMM_SNAN_TOKEN;
        ie_fault_mask = 0x10;
        break;

    case softfloat_QNaN:
        token = BX_FIXUPIMM_QNAN_TOKEN;
        break;

    case softfloat_denormal:
    case softfloat_normalized:
        if (tmp_op1 == float64_positive_one) {
            token = BX_FIXUPIMM_POS_ONE_VALUE_TOKEN;
            divz_fault_mask = 0x04;
            ie_fault_mask = 0x08;
        }
        else {
            if (sign) {
                token = BX_FIXUPIMM_NEG_VALUE_TOKEN;
                ie_fault_mask = 0x40;
            }
            else {
                token = BX_FIXUPIMM_POS_VALUE_TOKEN;
            }
        }
        break;

    default:
        break;
    }

    if (imm8 & ie_fault_mask)
        softfloat_raiseFlags(&status, softfloat_flag_invalid);

    if (imm8 & divz_fault_mask)
        softfloat_raiseFlags(&status, softfloat_flag_divbyzero);

    // access response table, each response is encoded with 4-bit value in the op2
    unsigned token_response = (op2 >> (token * 4)) & 0xf;

    switch (token_response) {
    case 0x1: // apply DAZ to the op1 value
        op1 = tmp_op1;
        break;
    case 0x2: op1 = convert_to_QNaN(tmp_op1); break;
    case 0x3: op1 = float64_default_nan; break;
    case 0x4: op1 = float64_negative_inf; break;
    case 0x5: op1 = float64_positive_inf; break;
    case 0x6:
        op1 = sign ? float64_negative_inf : float64_positive_inf;
        break;
    case 0x7: op1 = float64_negative_zero; break;
    case 0x8: op1 = float64_positive_zero; break;
    case 0x9: op1 = float64_negative_one; break;
    case 0xA: op1 = float64_positive_one; break;
    case 0xB: op1 = float64_positive_half; break;
    case 0xC: op1 = float64_value_90; break;
    case 0xD: op1 = float64_pi_half; break;
    case 0xE: op1 = float64_max_float; break;
    case 0xF: op1 = float64_min_float; break;
    default: // preserve the op1 value
        op1 = dst; break;
    }

    return op1;
}

int fpclass(softfloat_class_t op_class, int sign, int selector)
{
    return ((op_class == softfloat_QNaN) && (selector & 0x01) != 0) || // QNaN
        ((op_class == softfloat_zero) && !sign && (selector & 0x02) != 0) || // positive zero
        ((op_class == softfloat_zero) && sign && (selector & 0x04) != 0) || // negative zero
        ((op_class == softfloat_positive_inf) && (selector & 0x08) != 0) || // positive inf
        ((op_class == softfloat_negative_inf) && (selector & 0x10) != 0) || // negative inf
        ((op_class == softfloat_denormal) && (selector & 0x20) != 0) || // negative inf
        ((op_class == softfloat_denormal || op_class == softfloat_normalized) && sign && (selector & 0x40) != 0) || // negative finite
        ((op_class == softfloat_SNaN) && (selector & 0x80) != 0); // SNaN
}

static BX_CPP_INLINE int f32_fpclass(float32 op, int selector, int daz)
{
    if (daz)
        op = f32_denormal_to_zero(op);

    return fpclass(f32_class(op), f32_sign(op), selector);
}

static BX_CPP_INLINE int f64_fpclass(float64 op, int selector, int daz)
{
    if (daz)
        op = f64_denormal_to_zero(op);

    return fpclass(f64_class(op), f64_sign(op), selector);
}

static BX_CPP_INLINE float32 float32_reduce(float32 a, Bit8u scale, softfloat_status_t& status)
{
    if (a == float32_negative_inf || a == float32_positive_inf)
        return 0;

    float32 tmp = f32_roundToInt(a, scale, &status);
    return f32_sub(a, tmp, &status);
}

static BX_CPP_INLINE float64 float64_reduce(float64 a, Bit8u scale, softfloat_status_t& status)
{
    if (a == float64_negative_inf || a == float64_positive_inf)
        return 0;

    float64 tmp = f64_roundToInt(a, scale, &status);
    return f64_sub(a, tmp, &status);
}


#endif