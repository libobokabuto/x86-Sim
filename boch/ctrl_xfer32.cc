#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

void BX_CPP_AttrRegparmN(1) BX_CPU_C::branch_near16(Bit16u new_IP)
{
    if (new_IP > BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled) {
        exception(BX_GP_EXCEPTION, 0);
    }

    invalidate_prefetch_q();
    EIP = new_IP;
}


void BX_CPU_C::jmp_far32(bxInstruction_c* i, Bit16u cs_raw, Bit32u disp32)
{
    BX_INSTR_FAR_BRANCH_ORIGIN();

    invalidate_prefetch_q();

    // jump_protected doesn't affect ESP so it is ESP safe
    if (protected_mode()) {
        jump_protected(i, cs_raw, disp32);
    }
    else {
        // CS.LIMIT can't change when in real/v8086 mode
        if (disp32 > BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled) {
           // BX_ERROR(("%s: instruction pointer not within code segment limits", i->getIaOpcodeNameShort()));
            exception(BX_GP_EXCEPTION, 0);
        }

        load_seg_reg(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS], cs_raw);
        EIP = disp32;
    }

    BX_INSTR_FAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_JMP,
        FAR_BRANCH_PREV_CS, FAR_BRANCH_PREV_RIP,
        BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value, EIP);
}
