#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_SUPPORT_EVEX

#include "softfloat.h"
#include "bf16.h"
#include "simd_bf16.h"
#include "simd_int.h"

static BX_CPP_INLINE int bf16_fpclass(bfloat16 op, int selector)
{
	extern int fpclass(softfloat_class_t op_class, int sign, int selector);

	op = bf16_denormal_to_zero(op); // always assume DAZ

	return fpclass(bf16_class(op), bf16_sign(op), selector);
}

static BX_CPP_INLINE bfloat16 bfloat16_reduce(bfloat16 a, Bit8u scale)
{
	const bfloat16 bfloat16_negative_inf = 0xff80;
	const bfloat16 bfloat16_positive_inf = 0x7f80;

	if (a == bfloat16_negative_inf || a == bfloat16_positive_inf)
		return 0;

	bfloat16 tmp = bf16_roundToInt(a, scale);
	return bf16_sub(a, tmp);
}


#include "bf16-compare.h"

/* Comparison predicate for VCMPPBF16 instruction */
bfloat16_compare_method avx_compare_bf16[16] = {
  bf16_eq_ordered,
  bf16_lt_ordered,
  bf16_le_ordered,
  bf16_unordered,
  bf16_neq_unordered,
  bf16_nlt_unordered,
  bf16_nle_unordered,
  bf16_ordered,
  bf16_eq_unordered,
  bf16_nge_unordered,
  bf16_ngt_unordered,
  bf16_false,
  bf16_neq_ordered,
  bf16_ge_ordered,
  bf16_gt_ordered,
  bf16_true,
};

#endif