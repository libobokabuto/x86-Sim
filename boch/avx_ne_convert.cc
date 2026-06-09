#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_SUPPORT_AVX

#include "softfloat.h"

// FP32: s|eeeeeeee|mmmmmmmmmmmmmmmmmmmmmmm
// BF16: s|eeeeeeee|mmmmmmm
//  F16: s|eeeee|mmmmmmmmmm

softfloat_status_t prepare_ne_softfloat_status_helper(bool denormals_are_zeros = false)
{
	softfloat_status_t status;

	status.softfloat_roundingMode = softfloat_round_near_even;
	status.softfloat_exceptionFlags = 0;
	status.softfloat_exceptionMasks = softfloat_all_exceptions_mask;
	status.softfloat_suppressException = softfloat_all_exceptions_mask;
	status.softfloat_flush_underflow_to_zero = true;
	// by default input denormals not converted to zero and handled normally
	status.softfloat_denormals_are_zeros = denormals_are_zeros;

	return status;
}

float32 convert_ne_fp16_to_fp32(float16 op)
{
	static softfloat_status_t status = prepare_ne_softfloat_status_helper();
	return f16_to_f32(op, &status);
}

#endif