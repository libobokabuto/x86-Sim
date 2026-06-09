#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_SUPPORT_FPU

#include "ia_opcodes.h"

#define swap_values16u(a, b) { Bit16u tmp = a; a = b; b = tmp; }

extern softfloat_status_t i387cw_to_softfloat_status_word(Bit16u control_word);

#include "softfloat-specialize.h"
#include "fpu_trans.h"
#endif