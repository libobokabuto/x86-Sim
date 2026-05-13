#define NEED_CPU_REG_SHORTCUTS 1

#include "bochs.h"
#include "cpu.h"
#include "iodev.h"
#define LOG_THIS BX_CPU_THIS_PTR
#if BX_SUPPORT_SVM
#include "svm.h"
#endif

#include "ia_opcodes.h"

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JMP_Ap(bxInstruction_c* i)
{
    //BX_ASSERT(BX_CPU_THIS_PTR cpu_mode != BX_MODE_LONG_64);

    Bit32u disp32;
    Bit16u cs_raw;

    if (i->os32L()) {
        disp32 = i->Id();
    }
    else {
        disp32 = i->Iw();
    }
    cs_raw = i->Iw2();

    jmp_far32(i, cs_raw, disp32);

    BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ZERO_IDIOM_GwR(bxInstruction_c* i)
{
    BX_WRITE_16BIT_REG(i->dst(), 0);
    SET_FLAGS_OSZAPC_LOGIC_16(0);
    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OUT_IbAL(bxInstruction_c* i)
{
    unsigned port = i->Ib();
    /*
    if (!allow_io(i, port, 1)) {
        BX_DEBUG(("OUT_IbAL: I/O access not allowed !"));
        exception(BX_GP_EXCEPTION, 0);
    }*/

    BX_OUTP(port, AL, 1);

    BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_EbIbR(bxInstruction_c* i)
{
    BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), i->Ib());

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::IN_ALIb(bxInstruction_c* i)
{
    unsigned port = i->Ib();
    /*
    if (!allow_io(i, port, 1)) {
        BX_DEBUG(("IN_ALIb: I/O access not allowed !"));
        exception(BX_GP_EXCEPTION, 0);
    }*/
    //AL = BX_INP(port, 1);
    AL = BX_INP(port, 1);

    BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_GbEbR(bxInstruction_c* i)
{
    Bit8u op2 = BX_READ_8BIT_REGx(i->src(), i->extend8bitL());
    BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), op2);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMP_EbIbR(bxInstruction_c* i)
{
    Bit32u op1_8 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());
    Bit32u op2_8 = i->Ib();
    Bit32u diff_8 = op1_8 - op2_8;

    SET_FLAGS_OSZAPC_SUB_8(op1_8, op2_8, diff_8);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JZ_Jw(bxInstruction_c* i)
{
    if (get_ZF()) {
        Bit16u new_IP = (Bit16u)(EIP + i->Iw());
        branch_near16(new_IP);
        BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_IP);
        BX_NEXT_TRACE(i);
    }
    BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CLI(bxInstruction_c* i)
{
    Bit32u IOPL = BX_CPU_THIS_PTR get_IOPL();

    if (protected_mode())
    {
#if BX_CPU_LEVEL >= 5
        if (BX_CPU_THIS_PTR cr4.get_PVI() && (CPL == 3))
        {
            if (IOPL < 3) {
                BX_CPU_THIS_PTR clear_VIF();
                BX_NEXT_INSTR(i);
            }
        }
        else
#endif
        {
            if (IOPL < CPL) {
                //BX_DEBUG(("CLI: IOPL < CPL in protected mode"));
                exception(BX_GP_EXCEPTION, 0);
            }
        }
    }
    else if (v8086_mode())
    {
        if (IOPL != 3) {
#if BX_CPU_LEVEL >= 5
            if (BX_CPU_THIS_PTR cr4.get_VME()) {
                BX_CPU_THIS_PTR clear_VIF();
                BX_NEXT_INSTR(i);
            }
#endif
            //BX_DEBUG(("CLI: IOPL != 3 in v8086 mode"));
            exception(BX_GP_EXCEPTION, 0);
        }
    }

    BX_CPU_THIS_PTR clear_IF();

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_EwIwR(bxInstruction_c* i)
{
    BX_WRITE_16BIT_REG(i->dst(), i->Iw());

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_GwEwR(bxInstruction_c* i)
{
    BX_WRITE_16BIT_REG(i->dst(), BX_READ_16BIT_REG(i->src()));

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_SwEw(bxInstruction_c* i)
{
    Bit16u op2_16;

    /* Attempt to load CS or nonexisting segment register */
    if (i->dst() >= 6 || i->dst() == BX_SEG_REG_CS) {
        //BX_INFO(("MOV_EwSw: can't use this segment register %d", i->dst()));
        exception(BX_UD_EXCEPTION, 0);
    }

    if (i->modC0()) {
        op2_16 = BX_READ_16BIT_REG(i->src());
    }
    else {
        bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);
        /* pointer, segment address pair */
        op2_16 = read_virtual_word(i->seg(), eaddr);  //里面子函数的子函数没补全
    }

    load_seg_reg(&BX_CPU_THIS_PTR sregs[i->dst()], op2_16);

    if (i->dst() == BX_SEG_REG_SS) {
        // MOV SS inhibits interrupts, debug exceptions and single-step
        // trap exceptions until the execution boundary following the
        // next instruction is reached.
        // Same code as POP_SS()
        inhibit_interrupts(BX_INHIBIT_INTERRUPTS_BY_MOVSS);
    }

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_EbGbM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    write_virtual_byte(i->seg(), eaddr, BX_READ_8BIT_REGx(i->src(), i->extend8bitL()));//里面子函数的子函数没补全

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JNZ_Jw(bxInstruction_c* i)
{
    if (!get_ZF()) {
        Bit16u new_IP = IP + i->Iw();
        branch_near16(new_IP);
        BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_IP);
        BX_LINK_TRACE(i);
    }

    BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
    BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CLD(bxInstruction_c* i)
{
    BX_CPU_THIS_PTR clear_DF();

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::REP_STOSW_YwAX(bxInstruction_c* i)
{
#if BX_SUPPORT_X86_64
    if (i->as64L())
        BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::STOSW64_YwAX);
    else
#endif
        if (i->as32L()) {
            BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::STOSW32_YwAX);
            BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RDI); // always clear upper part of RDI
        }
        else {
            BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::STOSW16_YwAX);
        }

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CALL_Jw(bxInstruction_c* i)
{
#if BX_DEBUGGER
    BX_CPU_THIS_PTR show_flag |= Flag_call;
#endif

    RSP_SPECULATIVE;

    /* push 16 bit EA of next instruction */
    push_16(IP);
#if BX_SUPPORT_CET
    if (ShadowStackEnabled(CPL) && i->Iw())
        shadow_stack_push_32(IP);
#endif

    Bit16u new_IP = IP + i->Iw();
    branch_near16(new_IP);

    RSP_COMMIT;

    BX_INSTR_UCNEAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_RET, PREV_RIP, EIP);

    BX_LINK_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::STOSW16_YwAX(bxInstruction_c* i)
{
    Bit16u di = DI;

    write_virtual_word_32(BX_SEG_REG_ES, di, AX);

    if (BX_CPU_THIS_PTR get_DF()) {
        di -= 2;
    }
    else {
        di += 2;
    }

    DI = di;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::STOSW32_YwAX(bxInstruction_c* i)
{
    Bit32u edi = EDI;

    write_virtual_word(BX_SEG_REG_ES, edi, AX);

    if (BX_CPU_THIS_PTR get_DF()) {
        edi -= 2;
    }
    else {
        edi += 2;
    }

    // zero extension of RDI
    RDI = edi;
}

#if BX_SUPPORT_X86_64
/* 16 bit opsize mode, 32 bit address size */
void BX_CPP_AttrRegparmN(1) BX_CPU_C::STOSW64_YwAX(bxInstruction_c* i)
{
    Bit64u rdi = RDI;

    write_linear_word(BX_SEG_REG_ES, rdi, AX);

    if (BX_CPU_THIS_PTR get_DF()) {
        rdi -= 2;
    }
    else {
        rdi += 2;
    }

    RDI = rdi;
}
#endif

void BX_CPP_AttrRegparmN(1) BX_CPU_C::PUSH_EwR(bxInstruction_c* i)
{
    push_16(BX_READ_16BIT_REG(i->dst()));

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADD_EwIwR(bxInstruction_c* i)
{
    Bit32u op1_16 = BX_READ_16BIT_REG(i->dst());
    Bit32u op2_16 = i->Iw();
    Bit32u sum_16 = op1_16 + op2_16;
    BX_WRITE_16BIT_REG(i->dst(), sum_16);

    SET_FLAGS_OSZAPC_ADD_16(op1_16, op2_16, sum_16);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_EwSwR(bxInstruction_c* i)
{
    /* Illegal to use nonexisting segments */
    if (i->src() >= 6) {
        //BX_INFO(("MOV_EwSw: using of nonexisting segment register %d", i->src()));
        exception(BX_UD_EXCEPTION, 0);
    }

    Bit16u seg_reg = BX_CPU_THIS_PTR sregs[i->src()].selector.value;

    if (i->os32L()) {
        BX_WRITE_32BIT_REGZ(i->dst(), seg_reg);
    }
    else {
        BX_WRITE_16BIT_REG(i->dst(), seg_reg);
    }

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::RETnear16_Iw(bxInstruction_c* i)
{
    //BX_ASSERT(BX_CPU_THIS_PTR cpu_mode != BX_MODE_LONG_64);

#if BX_DEBUGGER
    BX_CPU_THIS_PTR show_flag |= Flag_ret;
#endif

    RSP_SPECULATIVE;

    Bit16u return_IP = pop_16();
#if BX_SUPPORT_CET
    if (ShadowStackEnabled(CPL)) {
        Bit32u shadow_IP = shadow_stack_pop_32();
        if (shadow_IP != Bit32u(return_IP))
            exception(BX_CP_EXCEPTION, BX_CP_NEAR_RET);
    }
#endif

    if (return_IP > BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled)
    {
        //BX_ERROR(("%s: offset outside of CS limits", i->getIaOpcodeNameShort()));
        exception(BX_GP_EXCEPTION, 0);
    }

    EIP = return_IP;

    Bit16u imm16 = i->Iw();

    if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.d_b) /* 32bit stack */
        ESP += imm16;
    else
        SP += imm16;

    RSP_COMMIT;

    BX_INSTR_UCNEAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_RET, PREV_RIP, EIP);

    BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::PUSH16_Sw(bxInstruction_c* i)
{
    push_16(BX_CPU_THIS_PTR sregs[i->src()].selector.value);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::POP_EwR(bxInstruction_c* i)
{
    BX_WRITE_16BIT_REG(i->dst(), pop_16());

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::INC_EwR(bxInstruction_c* i)
{
    Bit32u rx = ++BX_READ_16BIT_REG(i->dst());
    SET_FLAGS_OSZAP_ADD_16(rx - 1, 0, rx);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_EwGwM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    write_virtual_word(i->seg(), eaddr, BX_READ_16BIT_REG(i->src()));

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LEA_GwM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    BX_WRITE_16BIT_REG(i->dst(), (Bit16u)eaddr);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_GbEbM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    Bit8u val8 = read_virtual_byte(i->seg(), eaddr);
    BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), val8);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::AND_EbIbR(bxInstruction_c* i)
{
    Bit8u op1, op2 = i->Ib();

    op1 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());
    op1 &= op2;
    BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), op1);

    SET_FLAGS_OSZAPC_LOGIC_8(op1);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JMP_Jw(bxInstruction_c* i)
{
    Bit16u new_IP = IP + i->Iw();
    branch_near16(new_IP);
    BX_INSTR_UCNEAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_JMP, PREV_RIP, new_IP);

    BX_LINK_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_GwEwM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);
    Bit16u val16 = read_virtual_word(i->seg(), eaddr);
    BX_WRITE_16BIT_REG(i->dst(), val16);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::TEST_EbGbR(bxInstruction_c* i)
{
    Bit8u op1, op2;

    op1 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());
    op2 = BX_READ_8BIT_REGx(i->src(), i->extend8bitL());
    op1 &= op2;

    SET_FLAGS_OSZAPC_LOGIC_8(op1);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::TEST_EwGwR(bxInstruction_c* i)
{
    Bit16u op1_16, op2_16;

    op1_16 = BX_READ_16BIT_REG(i->dst());
    op2_16 = BX_READ_16BIT_REG(i->src());
    op1_16 &= op2_16;
    SET_FLAGS_OSZAPC_LOGIC_16(op1_16);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::XOR_GbEbR(bxInstruction_c* i)
{
    Bit8u op1, op2;

    op1 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());
    op2 = BX_READ_8BIT_REGx(i->src(), i->extend8bitL());
    op1 ^= op2;
    BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), op1);

    SET_FLAGS_OSZAPC_LOGIC_8(op1);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::PUSH_EwM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);
    Bit16u op1_16 = read_virtual_word(i->seg(), eaddr);

    push_16(op1_16);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OUT_DXAL(bxInstruction_c* i)
{
    unsigned port = DX;
    /*
    if (!allow_io(i, port, 1)) {
        //BX_DEBUG(("OUT_DXAL: I/O access not allowed !"));
        exception(BX_GP_EXCEPTION, 0);
    }
    */
    BX_OUTP(port, AL, 1);

    BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JNBE_Jw(bxInstruction_c* i)
{
    if (!(get_CF() || get_ZF())) {
        Bit16u new_IP = IP + i->Iw();
        branch_near16(new_IP);
        BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_IP);
        BX_LINK_TRACE(i);
    }

    BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
    BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::DEC_EwR(bxInstruction_c* i)
{
    Bit32u rx = --BX_READ_16BIT_REG(i->dst());
    SET_FLAGS_OSZAP_SUB_16(rx + 1, 0, rx);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LDS_GwMp(bxInstruction_c* i)
{
    load_segw(i, BX_SEG_REG_DS);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::POP16_Sw(bxInstruction_c* i)
{
    RSP_SPECULATIVE;

    Bit16u selector = pop_16();
    load_seg_reg(&BX_CPU_THIS_PTR sregs[i->dst()], selector);

    RSP_COMMIT;

    if (i->dst() == BX_SEG_REG_SS) {
        // POP SS inhibits interrupts, debug exceptions and single-step
        // trap exceptions until the execution boundary following the
        // next instruction is reached.
        // Same code as MOV_SwEw()
        inhibit_interrupts(BX_INHIBIT_INTERRUPTS_BY_MOVSS);
    }

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SHL_EdR(bxInstruction_c* i)
{
    unsigned count;

    if (i->getIaOpcode() == BX_IA_SHL_Ed)
        count = CL;
    else
        count = i->Ib();

    count &= 0x1f;

    if (!count) {
        BX_CLEAR_64BIT_HIGH(i->dst()); // always clear upper part of the register
    }
    else {
        Bit32u op1_32 = BX_READ_32BIT_REG(i->dst());

        /* count < 32, since only lower 5 bits used */
        Bit32u result_32 = (op1_32 << count);

        BX_WRITE_32BIT_REGZ(i->dst(), result_32);

        unsigned cf = (op1_32 >> (32 - count)) & 0x1;
        unsigned of = cf ^ (result_32 >> 31);

        SET_FLAGS_OSZAPC_LOGIC_32(result_32);
        BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);
    }

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::REP_STOSD_YdEAX(bxInstruction_c* i)
{
#if BX_SUPPORT_X86_64
    if (i->as64L())
        BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::STOSD64_YdEAX);
    else
#endif
        if (i->as32L()) {
            BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::STOSD32_YdEAX);
            BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RDI); // always clear upper part of RDI
        }
        else {
            BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::STOSD16_YdEAX);
        }

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::STOSD16_YdEAX(bxInstruction_c* i)
{
    Bit16u di = DI;

    write_virtual_dword_32(BX_SEG_REG_ES, di, EAX);

    if (BX_CPU_THIS_PTR get_DF()) {
        di -= 4;
    }
    else {
        di += 4;
    }

    DI = di;
}

/* 32 bit opsize mode, 32 bit address size */
void BX_CPP_AttrRegparmN(1) BX_CPU_C::STOSD32_YdEAX(bxInstruction_c* i)
{
    Bit32u edi = EDI;

    write_virtual_dword(BX_SEG_REG_ES, edi, EAX);

    if (BX_CPU_THIS_PTR get_DF()) {
        edi -= 4;
    }
    else {
        edi += 4;
    }

    // zero extension of RDI
    RDI = edi;
}

#if BX_SUPPORT_X86_64

/* 32 bit opsize mode, 32 bit address size */
void BX_CPP_AttrRegparmN(1) BX_CPU_C::STOSD64_YdEAX(bxInstruction_c* i)
{
    Bit64u rdi = RDI;

    write_linear_dword(BX_SEG_REG_ES, rdi, EAX);

    if (BX_CPU_THIS_PTR get_DF()) {
        rdi -= 4;
    }
    else {
        rdi += 4;
    }

    RDI = rdi;
}
#endif

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LOOP16_Jb(bxInstruction_c* i)
{
    // it is impossible to get this instruction in long mode
    //BX_ASSERT(i->as64L() == 0);

    if (i->as32L()) {
        Bit32u count = ECX;

        count--;
        if (count != 0) {
            Bit16u new_IP = IP + i->Iw();
            branch_near16(new_IP);
            BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_IP);
        }
#if BX_INSTRUMENTATION
        else {
            BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
        }
#endif

        ECX = count;
    }
    else {
        Bit16u count = CX;

        count--;
        if (count != 0) {
            Bit16u new_IP = IP + i->Iw();
            branch_near16(new_IP);
            BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_IP);
        }
#if BX_INSTRUMENTATION
        else {
            BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
        }
#endif

        CX = count;
    }

    BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ZERO_IDIOM_GdR(bxInstruction_c* i)
{
    BX_WRITE_32BIT_REGZ(i->dst(), 0);
    SET_FLAGS_OSZAPC_LOGIC_32(0);
    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_OdAX(bxInstruction_c* i)
{
    write_virtual_word_32(i->seg(), i->Id(), AX);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_EbIbM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    write_virtual_byte(i->seg(), eaddr, i->Ib());

    BX_NEXT_INSTR(i);
}