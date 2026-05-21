#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_CPU_LEVEL >= 3

void BX_CPP_AttrRegparmN(1) BX_CPU_C::branch_near32(Bit32u new_EIP)
{
    //BX_ASSERT(BX_CPU_THIS_PTR cpu_mode != BX_MODE_LONG_64);

    // check always, not only in protected mode
    if (new_EIP > BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled)
    {
        //BX_ERROR(("branch_near32: offset outside of CS limits"));
        exception(BX_GP_EXCEPTION, 0);
    }

    EIP = new_EIP;

#if BX_SUPPORT_HANDLERS_CHAINING_SPEEDUPS == 0
    // assert magic async_event to stop trace execution
    BX_CPU_THIS_PTR async_event |= BX_ASYNC_EVENT_STOP_TRACE;
#endif
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

#endif
