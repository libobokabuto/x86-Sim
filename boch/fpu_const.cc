#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_SUPPORT_FPU

#include "softfloat-specialize.h"

extern const floatx80 Const_Z = packFloatx80(0, 0x0000, 0);
extern const floatx80 Const_1 = packFloatx80(0, 0x3fff, BX_CONST64(0x8000000000000000));
extern const floatx80 Const_L2T = packFloatx80(0, 0x4000, BX_CONST64(0xd49a784bcd1b8afe));
extern const floatx80 Const_L2E = packFloatx80(0, 0x3fff, BX_CONST64(0xb8aa3b295c17f0bc));
extern const floatx80 Const_PI = packFloatx80(0, 0x4000, BX_CONST64(0xc90fdaa22168c235));
extern const floatx80 Const_LG2 = packFloatx80(0, 0x3ffd, BX_CONST64(0x9a209a84fbcff799));
extern const floatx80 Const_LN2 = packFloatx80(0, 0x3ffe, BX_CONST64(0xb17217f7d1cf79ac));
extern const floatx80 Const_INF = packFloatx80(0, 0x7fff, BX_CONST64(0x8000000000000000));

/* A fast way to find out whether x is one of RC_DOWN or RC_CHOP
   (and not one of RC_RND or RC_UP).
   */
#define DOWN_OR_CHOP()  (FPU_CONTROL_WORD & FPU_CW_RC & FPU_RC_DOWN)

BX_CPP_INLINE floatx80 FPU_round_const(const floatx80& a, int adj)
{
	floatx80 result = a;
	result.signif += adj;
	return result;
}
#endif
