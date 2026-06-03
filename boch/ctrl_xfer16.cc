#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

void BX_CPP_AttrRegparmN(1) BX_CPU_C::branch_near16(Bit16u new_IP)
{
    // BX_ASSERT(BX_CPU_THIS_PTR cpu_mode != BX_MODE_LONG_64);
    if (new_IP > BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled) {
        //BX_ERROR(("branch_near16: offset outside of CS limits"));
        exception(BX_GP_EXCEPTION, 0);
    }

    EIP = new_IP;

#if BX_SUPPORT_HANDLERS_CHAINING_SPEEDUPS == 0
    // assert magic async_event to stop trace execution
    BX_CPU_THIS_PTR async_event |= BX_ASYNC_EVENT_STOP_TRACE;
#endif
}

void BX_CPU_C::call_far16(bxInstruction_c* i, Bit16u cs_raw, Bit16u disp16)
{
    BX_INSTR_FAR_BRANCH_ORIGIN();

    invalidate_prefetch_q();

#if BX_DEBUGGER
    BX_CPU_THIS_PTR show_flag |= Flag_call;
#endif

    RSP_SPECULATIVE;

    if (protected_mode()) {
        call_protected(i, cs_raw, disp16);
    }
    else {
        // CS.LIMIT can't change when in real/v8086 mode
        if (disp16 > BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled) {
            //BX_ERROR(("%s: instruction pointer not within code segment limits", i->getIaOpcodeNameShort()));
            exception(BX_GP_EXCEPTION, 0);
        }

        push_16(BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value);
        push_16(IP);

        load_seg_reg(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS], cs_raw);
        EIP = (Bit32u)disp16;
    }

    RSP_COMMIT;

    BX_INSTR_FAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_CALL,
        FAR_BRANCH_PREV_CS, FAR_BRANCH_PREV_RIP,
        BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value, RIP);
}

void BX_CPU_C::jmp_far16(bxInstruction_c* i, Bit16u cs_raw, Bit16u disp16)
{
    BX_INSTR_FAR_BRANCH_ORIGIN();

    invalidate_prefetch_q();

    // jump_protected doesn't affect RSP so it is RSP safe
    if (protected_mode()) {
        jump_protected(i, cs_raw, disp16);
    }
    else {
        // CS.LIMIT can't change when in real/v8086 mode
        if (disp16 > BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled) {
            //BX_ERROR(("%s: instruction pointer not within code segment limits", i->getIaOpcodeNameShort()));
            exception(BX_GP_EXCEPTION, 0);
        }

        load_seg_reg(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS], cs_raw);
        EIP = disp16;
    }

    BX_INSTR_FAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_JMP_INDIRECT,
        FAR_BRANCH_PREV_CS, FAR_BRANCH_PREV_RIP,
        BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value, EIP);
}