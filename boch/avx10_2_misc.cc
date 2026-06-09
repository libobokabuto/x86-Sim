#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_SUPPORT_AVX

#include "softfloat.h"
#include "ia_opcodes.h"

extern softfloat_status_t mxcsr_to_softfloat_status_word(bx_mxcsr_t mxcsr);
extern void softfloat_status_word_rc_override(softfloat_status_t& status, bxInstruction_c* i);

void BX_CPU_C::write_eflags_vcomx(int float_relation)
{
    switch (float_relation) {
    case softfloat_relation_unordered:
        setEFlagsOSZAPC(EFlagsOFMask | EFlagsSFMask | EFlagsPFMask | EFlagsCFMask);
        break;

    case softfloat_relation_greater:
        clearEFlagsOSZAPC();
        break;

    case softfloat_relation_less:
        setEFlagsOSZAPC(EFlagsOFMask | EFlagsCFMask);
        break;

    case softfloat_relation_equal:
        setEFlagsOSZAPC(EFlagsOFMask | EFlagsSFMask | EFlagsZFMask);
        break;

    default:
        //BX_PANIC(("write_eflags_vcomx: unknown floating point compare relation"));
        break;
    }
}

#include "simd_int.h"

extern float32 convert_ne_fp16_to_fp32(float16 op);
extern softfloat_status_t prepare_ne_softfloat_status_helper(bool denormals_are_zeros);

#endif