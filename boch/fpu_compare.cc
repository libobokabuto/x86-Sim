#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_SUPPORT_FPU

#include "ia_opcodes.h"

#include "softfloat.h"

#include "softfloat-specialize.h"

extern softfloat_status_t i387cw_to_softfloat_status_word(Bit16u control_word);

static int status_word_flags_fpu_compare(int float_relation)
{
    switch (float_relation) {
    case softfloat_relation_unordered:
        return (FPU_SW_C0 | FPU_SW_C2 | FPU_SW_C3);

    case softfloat_relation_greater:
        return (0);

    case softfloat_relation_less:
        return (FPU_SW_C0);

    case softfloat_relation_equal:
        return (FPU_SW_C3);
    }

    return (-1);        // should never get here
}

void BX_CPU_C::write_eflags_fpu_compare(int float_relation)
{
    switch (float_relation) {
    case softfloat_relation_unordered:
        setEFlagsOSZAPC(EFlagsZFMask | EFlagsPFMask | EFlagsCFMask);
        break;

    case softfloat_relation_greater:
        clearEFlagsOSZAPC();
        break;

    case softfloat_relation_less:
        clearEFlagsOSZAPC();
        assert_CF();
        break;

    case softfloat_relation_equal:
        clearEFlagsOSZAPC();
        assert_ZF();
        break;

    default:
        //BX_PANIC(("write_eflags: unknown floating point compare relation"));
        break;
    }
}

#endif