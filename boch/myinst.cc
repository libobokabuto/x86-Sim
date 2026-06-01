#define NEED_CPU_REG_SHORTCUTS 1

#include "bochs.h"
#include "cpu.h"
#include "iodev.h"
#include "cpuid.h"
#define LOG_THIS BX_CPU_THIS_PTR
#if BX_SUPPORT_SVM
#include "svm.h"
#endif
#include "scalar_arith.h"
#include "ia_opcodes.h"
#include "pc_system.h"
#if BX_SUPPORT_APIC
#include "apic.h"
#endif

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
    
    if (!allow_io(i, port, 1)) {
        //BX_DEBUG(("OUT_IbAL: I/O access not allowed !"));
        exception(BX_GP_EXCEPTION, 0);
    }

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
    
    if (!allow_io(i, port, 1)) {
        //BX_DEBUG(("IN_ALIb: I/O access not allowed !"));
        exception(BX_GP_EXCEPTION, 0);
    }
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
    
    if (!allow_io(i, port, 1)) {
        //BX_DEBUG(("OUT_DXAL: I/O access not allowed !"));
        exception(BX_GP_EXCEPTION, 0);
    }
    
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

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_EwIwM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);
    write_virtual_word(i->seg(), eaddr, i->Iw());

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::IN_ALDX(bxInstruction_c* i)
{
    unsigned port = DX;
    
    if (!allow_io(i, port, 1)) {
        //BX_DEBUG(("IN_ALDX: I/O access not allowed !"));
        exception(BX_GP_EXCEPTION, 0);
    }
    
    AL = BX_INP(port, 1);

    BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SHL_EwR(bxInstruction_c* i)
{
    Bit16u result_16;
    unsigned count;
    unsigned of = 0, cf = 0;

    if (i->getIaOpcode() == BX_IA_SHL_Ew)
        count = CL;
    else
        count = i->Ib();

    count &= 0x1f; /* use only 5 LSB's */

    if (count) {
        Bit16u op1_16 = BX_READ_16BIT_REG(i->dst());

        if (count <= 16) {
            result_16 = (op1_16 << count);
            cf = (op1_16 >> (16 - count)) & 0x1;
            of = cf ^ (result_16 >> 15); // of = cf ^ result15
        }
        else {
            result_16 = 0;
        }

        BX_WRITE_16BIT_REG(i->dst(), result_16);

        SET_FLAGS_OSZAPC_LOGIC_16(result_16);
        BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);
    }

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_AXOd(bxInstruction_c* i)
{
    AX = read_virtual_word_32(i->seg(), i->Id());

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::AND_EwIwR(bxInstruction_c* i)
{
    Bit16u op1_16 = BX_READ_16BIT_REG(i->dst());
    op1_16 &= i->Iw();
    BX_WRITE_16BIT_REG(i->dst(), op1_16);

    SET_FLAGS_OSZAPC_LOGIC_16(op1_16);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OR_GwEwR(bxInstruction_c* i)
{
    Bit16u op1_16, op2_16;

    op1_16 = BX_READ_16BIT_REG(i->dst());
    op2_16 = BX_READ_16BIT_REG(i->src());
    op1_16 |= op2_16;
    BX_WRITE_16BIT_REG(i->dst(), op1_16);

    SET_FLAGS_OSZAPC_LOGIC_16(op1_16);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SHR_EbR(bxInstruction_c* i)
{
    unsigned count;

    if (i->getIaOpcode() == BX_IA_SHR_Eb)
        count = CL;
    else
        count = i->Ib();

    count &= 0x1f;

    if (count) {
        Bit8u op1_8 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());
        Bit8u result_8 = (op1_8 >> count);
        BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), result_8);

        unsigned cf = (op1_8 >> (count - 1)) & 0x1;
        // note, that of == result7 if count == 1 and
        //            of == 0       if count >= 2
        unsigned of = (((result_8 << 1) ^ result_8) >> 7) & 0x1;

        SET_FLAGS_OSZAPC_LOGIC_8(result_8);
        BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);
    }

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADD_GbEbR(bxInstruction_c* i)
{
    Bit32u op1 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());
    Bit32u op2 = BX_READ_8BIT_REGx(i->src(), i->extend8bitL());
    Bit32u sum = op1 + op2;

    BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), sum);

    SET_FLAGS_OSZAPC_ADD_8(op1, op2, sum);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MUL_ALEbR(bxInstruction_c* i)
{
    Bit8u op1 = AL;
    Bit8u op2 = BX_READ_8BIT_REGx(i->src(), i->extend8bitL());

    Bit32u product_16 = ((Bit16u)op1) * ((Bit16u)op2);

    Bit8u product_8l = (product_16 & 0xFF);
    Bit8u product_8h = product_16 >> 8;

    /* now write product back to destination */
    AX = product_16;

    /* set EFLAGS */
    SET_FLAGS_OSZAPC_LOGIC_8(product_8l);
    if (product_8h != 0)
    {
        BX_CPU_THIS_PTR oszapc.assert_flags_OxxxxC();
    }

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::IMUL_GdEdIdR(bxInstruction_c* i)
{
    Bit32s op2_32 = BX_READ_32BIT_REG(i->src());
    Bit32s op3_32 = i->Id();

    Bit64s product_64 = ((Bit64s)op2_32) * ((Bit64s)op3_32);
    Bit32u product_32 = (Bit32u)(product_64 & 0xFFFFFFFF);

    /* now write product back to destination */
    BX_WRITE_32BIT_REGZ(i->dst(), product_32);

    /* set eflags:
     * IMUL r32,r/m32,imm32: condition for clearing CF & OF:
     *   result exactly fits within r32
     */
    SET_FLAGS_OSZAPC_LOGIC_32(product_32);
    if (product_64 != (Bit32s)product_64)
    {
        BX_CPU_THIS_PTR oszapc.assert_flags_OxxxxC();
    }

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_GdEdR(bxInstruction_c* i)
{
    BX_WRITE_32BIT_REGZ(i->dst(), BX_READ_32BIT_REG(i->src()));

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADD_GdEdR(bxInstruction_c* i)
{
    Bit32u op1_32, op2_32, sum_32;

    op1_32 = BX_READ_32BIT_REG(i->dst());
    op2_32 = BX_READ_32BIT_REG(i->src());
    sum_32 = op1_32 + op2_32;

    BX_WRITE_32BIT_REGZ(i->dst(), sum_32);

    SET_FLAGS_OSZAPC_ADD_32(op1_32, op2_32, sum_32);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_CR0Rd(bxInstruction_c* i)
{
    // CPL is always 0 in real mode
    if (/* !real_mode() && */ CPL != 0) {
        //BX_ERROR(("%s: CPL!=0 not in real mode", i->getIaOpcodeNameShort()));
        exception(BX_GP_EXCEPTION, 0);
    }

    invalidate_prefetch_q();

    Bit32u val_32 = BX_READ_32BIT_REG(i->src());

    if (i->dst() == 0) {
        // CR0
#if BX_SUPPORT_VMX
        if (BX_CPU_THIS_PTR in_vmx_guest)
            val_32 = (Bit32u)VMexit_CR0_Write(i, val_32);
#endif
        if (!SetCR0(i, val_32))
            exception(BX_GP_EXCEPTION, 0);

        BX_INSTR_TLB_CNTRL(BX_CPU_ID, BX_INSTR_MOV_CR0, val_32);
    }
#if BX_CPU_LEVEL >= 6
    else {
        // AMD feature: LOCK CR0 allows CR8 access even in 32-bit mode
        WriteCR8(i, val_32);
    }
#endif

    BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_RdCR0(bxInstruction_c* i)
{
    // CPL is always 0 in real mode
    if (/* !real_mode() && */ CPL != 0) {
        //BX_ERROR(("%s: CPL!=0 not in real mode", i->getIaOpcodeNameShort()));
        exception(BX_GP_EXCEPTION, 0);
    }

    Bit32u val_32 = 0;

    if (i->src() == 0) {
        // CR0
#if BX_SUPPORT_SVM
        if (BX_CPU_THIS_PTR in_svm_guest) {
            if (SVM_CR_READ_INTERCEPTED(0))
                Svm_Vmexit(SVM_VMEXIT_CR0_READ, BX_SUPPORT_SVM_EXTENSION(BX_CPUID_SVM_DECODE_ASSIST) ? i->dst() : 0);
        }
#endif

        val_32 = (Bit32u)read_CR0(); /* correctly handle VMX */
    }
#if BX_CPU_LEVEL >= 6
    else {
        // AMD feature: LOCK CR0 allows CR8 access even in 32-bit mode
        val_32 = ReadCR8(i);
    }
#endif

    BX_WRITE_32BIT_REGZ(i->dst(), val_32);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OR_EbIbR(bxInstruction_c* i)
{
    Bit8u op1, op2 = i->Ib();

    op1 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());
    op1 |= op2;
    BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), op1);

    SET_FLAGS_OSZAPC_LOGIC_8(op1);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LGDT_Ms(bxInstruction_c* i)
{
    //BX_ASSERT(BX_CPU_THIS_PTR cpu_mode != BX_MODE_LONG_64);

    // CPL is always 0 is real mode
    if (/* !real_mode() && */ CPL != 0) {
        //BX_ERROR(("LGDT: CPL != 0 causes #GP"));
        exception(BX_GP_EXCEPTION, 0);
    }

#if BX_SUPPORT_VMX >= 2
    if (BX_CPU_THIS_PTR in_vmx_guest)
        if (BX_CPU_THIS_PTR vmcs.vmexec_ctrls2.DESCRIPTOR_TABLE_VMEXIT())
            VMexit_Instruction(i, VMX_VMEXIT_GDTR_IDTR_ACCESS, BX_WRITE);
#endif

#if BX_SUPPORT_SVM
    if (BX_CPU_THIS_PTR in_svm_guest) {
        if (SVM_INTERCEPT(SVM_INTERCEPT0_GDTR_WRITE)) Svm_Vmexit(SVM_VMEXIT_GDTR_WRITE);
    }
#endif

    Bit32u eaddr = (Bit32u)BX_CPU_RESOLVE_ADDR_32(i);

    Bit16u limit_16 = read_virtual_word_32(i->seg(), eaddr);
    Bit32u base_32 = read_virtual_dword_32(i->seg(), (eaddr + 2) & i->asize_mask());

    if (i->os32L() == 0) base_32 &= 0x00ffffff; /* ignore upper 8 bits */

    BX_CPU_THIS_PTR gdtr.limit = limit_16;
    BX_CPU_THIS_PTR gdtr.base = base_32;

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LIDT_Ms(bxInstruction_c* i)
{
    //BX_ASSERT(BX_CPU_THIS_PTR cpu_mode != BX_MODE_LONG_64);

    // CPL is always 0 is real mode
    if (/* !real_mode() && */ CPL != 0) {
        //BX_ERROR(("LIDT: CPL != 0 causes #GP"));
        exception(BX_GP_EXCEPTION, 0);
    }

#if BX_SUPPORT_VMX >= 2
    if (BX_CPU_THIS_PTR in_vmx_guest)
        if (BX_CPU_THIS_PTR vmcs.vmexec_ctrls2.DESCRIPTOR_TABLE_VMEXIT())
            VMexit_Instruction(i, VMX_VMEXIT_GDTR_IDTR_ACCESS, BX_WRITE);
#endif

#if BX_SUPPORT_SVM
    if (BX_CPU_THIS_PTR in_svm_guest) {
        if (SVM_INTERCEPT(SVM_INTERCEPT0_IDTR_WRITE)) Svm_Vmexit(SVM_VMEXIT_IDTR_WRITE);
    }
#endif

    Bit32u eaddr = (Bit32u)BX_CPU_RESOLVE_ADDR_32(i);

    Bit16u limit_16 = read_virtual_word_32(i->seg(), eaddr);
    Bit32u base_32 = read_virtual_dword_32(i->seg(), (eaddr + 2) & i->asize_mask());

    if (i->os32L() == 0) base_32 &= 0x00ffffff; /* ignore upper 8 bits */

    BX_CPU_THIS_PTR idtr.limit = limit_16;
    BX_CPU_THIS_PTR idtr.base = base_32;

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_EdIdR(bxInstruction_c* i)
{
    BX_WRITE_32BIT_REGZ(i->dst(), i->Id());

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_EdIdM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);
    write_virtual_dword(i->seg(), eaddr, i->Id());

    BX_NEXT_INSTR(i);
}

#if BX_CPU_LEVEL >= 3

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CALL_EdR(bxInstruction_c* i)
{
#if BX_DEBUGGER
    BX_CPU_THIS_PTR show_flag |= Flag_call;
#endif

    Bit32u new_EIP = BX_READ_32BIT_REG(i->dst());

    RSP_SPECULATIVE;

    /* push 32 bit EA of next instruction */
    push_32(EIP);
#if BX_SUPPORT_CET
    if (ShadowStackEnabled(CPL))
        shadow_stack_push_32(EIP);
#endif

    branch_near32(new_EIP);

    RSP_COMMIT;

#if BX_SUPPORT_CET
    track_indirect_if_not_suppressed(i, CPL);
#endif

    BX_INSTR_UCNEAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_CALL_INDIRECT, PREV_RIP, EIP);

    BX_NEXT_TRACE(i);
}

#endif

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SUB_GdEdR(bxInstruction_c* i)
{
    Bit32u op1_32, op2_32, diff_32;

    op1_32 = BX_READ_32BIT_REG(i->dst());
    op2_32 = BX_READ_32BIT_REG(i->src());
    diff_32 = op1_32 - op2_32;
    BX_WRITE_32BIT_REGZ(i->dst(), diff_32);

    SET_FLAGS_OSZAPC_SUB_32(op1_32, op2_32, diff_32);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMP_GdEdR(bxInstruction_c* i)
{
    Bit32u op1_32, op2_32, diff_32;

    op1_32 = BX_READ_32BIT_REG(i->dst());
    op2_32 = BX_READ_32BIT_REG(i->src());
    diff_32 = op1_32 - op2_32;

    SET_FLAGS_OSZAPC_SUB_32(op1_32, op2_32, diff_32);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::REP_STOSB_YbAL(bxInstruction_c* i)
{
#if BX_SUPPORT_X86_64
    if (i->as64L())
        BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::STOSB64_YbAL);
    else
#endif
        if (i->as32L()) {
            BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::STOSB32_YbAL);
            BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RDI); // always clear upper part of RDI
        }
        else {
            BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::STOSB16_YbAL);
        }

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::STOSB16_YbAL(bxInstruction_c* i)
{
    Bit16u di = DI;

    write_virtual_byte_32(BX_SEG_REG_ES, di, AL);

    if (BX_CPU_THIS_PTR get_DF()) {
        di--;
    }
    else {
        di++;
    }

    DI = di;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::STOSB32_YbAL(bxInstruction_c* i)
{
    Bit32s increment = 0;
    Bit32u edi = EDI;

#if BX_SUPPORT_REPEAT_SPEEDUPS
    /* If conditions are right, we can transfer IO to physical memory
     * in a batch, rather than one instruction at a time.
     */
    if (i->repUsedL() && !BX_CPU_THIS_PTR get_DF() && !BX_CPU_THIS_PTR async_event)
    {
        Bit32u byteCount = FastRepSTOSB(BX_SEG_REG_ES, edi, AL, ECX);
        if (byteCount) {
            // Decrement the ticks count by the number of iterations, minus
            // one, since the main cpu loop will decrement one.  Also,
            // the count is predecremented before examined, so definitely
            // don't roll it under zero.
            BX_TICKN(byteCount - 1);

            // Decrement eCX.  Note, the main loop will decrement 1 also, so
            // decrement by one less than expected, like the case above.
            RCX = ECX - (byteCount - 1);

            increment = byteCount;
        }
    }

    if (increment == 0)
#endif
    {
        write_virtual_byte(BX_SEG_REG_ES, edi, AL);

        increment = BX_CPU_THIS_PTR get_DF() ? -1 : 1;
    }

    // zero extension of RDI
    RDI = edi + increment;
}

#if BX_SUPPORT_X86_64
// 64 bit address size
void BX_CPP_AttrRegparmN(1) BX_CPU_C::STOSB64_YbAL(bxInstruction_c* i)
{
    Bit64u rdi = RDI;
    Bit32s increment = 0;

#if BX_SUPPORT_REPEAT_SPEEDUPS
    /* If conditions are right, we can transfer IO to physical memory
     * in a batch, rather than one instruction at a time.
     */
    if (i->repUsedL() && !BX_CPU_THIS_PTR get_DF() && !BX_CPU_THIS_PTR async_event)
    {
        Bit32u byteCount = FastRepSTOSB(rdi, AL, ECX);
        if (byteCount) {
            // Decrement the ticks count by the number of iterations, minus
            // one, since the main cpu loop will decrement one.  Also,
            // the count is predecremented before examined, so definitely
            // don't roll it under zero.
            BX_TICKN(byteCount - 1);

            // Decrement RCX.  Note, the main loop will decrement 1 also, so
            // decrement by one less than expected, like the case above.
            RCX -= (byteCount - 1);

            increment = byteCount;
        }
    }

    if (increment == 0)
#endif
    {
        write_linear_byte(BX_SEG_REG_ES, rdi, AL);

        increment = BX_CPU_THIS_PTR get_DF() ? -1 : 1;
    }

    RDI = rdi + increment;
}
#endif

void BX_CPP_AttrRegparmN(1) BX_CPU_C::REP_MOVSB_YbXb(bxInstruction_c* i)
{
#if BX_SUPPORT_X86_64
    if (i->as64L())
        BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::MOVSB64_YbXb);
    else
#endif
        if (i->as32L()) {
            BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::MOVSB32_YbXb);
            BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RSI); // always clear upper part of RSI/RDI
            BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RDI);
        }
        else {
            BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::MOVSB16_YbXb);
        }

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOVSB16_YbXb(bxInstruction_c* i)
{
    Bit8u temp8 = read_virtual_byte_32(i->seg(), SI);
    write_virtual_byte_32(BX_SEG_REG_ES, DI, temp8);

    if (BX_CPU_THIS_PTR get_DF()) {
        SI--;
        DI--;
    }
    else {
        SI++;
        DI++;
    }
}

// 32 bit address size
void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOVSB32_YbXb(bxInstruction_c* i)
{
    Bit32s increment = 0;

#if BX_SUPPORT_REPEAT_SPEEDUPS
    /* If conditions are right, we can transfer IO to physical memory
     * in a batch, rather than one instruction at a time */
    if (i->repUsedL() && !BX_CPU_THIS_PTR get_DF() && !BX_CPU_THIS_PTR async_event)
    {
        Bit32u byteCount = FastRepMOVSB(i->seg(), ESI, BX_SEG_REG_ES, EDI, ECX, 1);
        if (byteCount) {
            // Decrement the ticks count by the number of iterations, minus
            // one, since the main cpu loop will decrement one.  Also,
            // the count is predecremented before examined, so definitely
            // don't roll it under zero.
            BX_TICKN(byteCount - 1);

            // Decrement eCX. Note, the main loop will decrement 1 also, so
            // decrement by one less than expected, like the case above.
            RCX = ECX - (byteCount - 1);

            increment = byteCount;
        }
    }

    if (increment == 0)
#endif
    {
        Bit8u temp8 = read_virtual_byte(i->seg(), ESI);
        write_virtual_byte(BX_SEG_REG_ES, EDI, temp8);

        increment = BX_CPU_THIS_PTR get_DF() ? -1 : 1;
    }

    RSI = ESI + increment;
    RDI = EDI + increment;
}

#if BX_SUPPORT_X86_64
// 64 bit address size
void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOVSB64_YbXb(bxInstruction_c* i)
{
    Bit32s increment = 0;

    Bit64u rsi = RSI;
    Bit64u rdi = RDI;

#if BX_SUPPORT_REPEAT_SPEEDUPS
    /* If conditions are right, we can transfer IO to physical memory
     * in a batch, rather than one instruction at a time */
    if (i->repUsedL() && !BX_CPU_THIS_PTR get_DF() && !BX_CPU_THIS_PTR async_event)
    {
        Bit32u byteCount = FastRepMOVSB(get_laddr64(i->seg(), rsi), rdi, ECX, 1);
        if (byteCount) {
            // Decrement the ticks count by the number of iterations, minus
            // one, since the main cpu loop will decrement one.  Also,
            // the count is predecremented before examined, so definitely
            // don't roll it under zero.
            BX_TICKN(byteCount - 1);

            // Decrement RCX. Note, the main loop will decrement 1 also, so
            // decrement by one less than expected, like the case above.
            RCX -= (byteCount - 1);

            increment = byteCount;
        }
    }

    if (increment == 0)
#endif
    {
        Bit8u temp8 = read_linear_byte(i->seg(), get_laddr64(i->seg(), rsi));
        write_linear_byte(BX_SEG_REG_ES, rdi, temp8);

        increment = BX_CPU_THIS_PTR get_DF() ? -1 : 1;
    }

    RSI = rsi + increment;
    RDI = rdi + increment;
}
#endif

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JMP_Jd(bxInstruction_c* i)
{
    Bit32u new_EIP = EIP + (Bit32s)i->Id();
    branch_near32(new_EIP);
    BX_INSTR_UCNEAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_JMP, PREV_RIP, new_EIP);

    BX_LINK_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::PUSH_EdR(bxInstruction_c* i)
{
    push_32(BX_READ_32BIT_REG(i->dst()));

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SUB_EdIdR(bxInstruction_c* i)
{
    Bit32u op1_32, op2_32 = i->Id(), diff_32;

    op1_32 = BX_READ_32BIT_REG(i->dst());
    diff_32 = op1_32 - op2_32;
    BX_WRITE_32BIT_REGZ(i->dst(), diff_32);

    SET_FLAGS_OSZAPC_SUB_32(op1_32, op2_32, diff_32);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV32S_GdEdM(bxInstruction_c* i)
{
    Bit32u eaddr = (Bit32u)BX_CPU_RESOLVE_ADDR_32(i);
    Bit32u val32 = stack_read_dword(eaddr);

    BX_WRITE_32BIT_REGZ(i->dst(), val32);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::PUSH_Id(bxInstruction_c* i)
{
    push_32(i->Id());

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CALL_Jd(bxInstruction_c* i)
{
#if BX_DEBUGGER
    BX_CPU_THIS_PTR show_flag |= Flag_call;
#endif

    RSP_SPECULATIVE;

    /* push 32 bit EA of next instruction */
    push_32(EIP);
#if BX_SUPPORT_CET
    if (ShadowStackEnabled(CPL) && i->Id())
        shadow_stack_push_32(EIP);
#endif

    Bit32u new_EIP = EIP + i->Id();
    branch_near32(new_EIP);

    RSP_COMMIT;

    BX_INSTR_UCNEAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_CALL, PREV_RIP, EIP);

    BX_LINK_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::AND_EdIdR(bxInstruction_c* i)
{
    Bit32u op1_32 = BX_READ_32BIT_REG(i->dst());
    op1_32 &= i->Id();
    BX_WRITE_32BIT_REGZ(i->dst(), op1_32);

    SET_FLAGS_OSZAPC_LOGIC_32(op1_32);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMP_EdIdR(bxInstruction_c* i)
{
    Bit32u op1_32, op2_32, diff_32;

    op1_32 = BX_READ_32BIT_REG(i->dst());
    op2_32 = i->Id();
    diff_32 = op1_32 - op2_32;

    SET_FLAGS_OSZAPC_SUB_32(op1_32, op2_32, diff_32);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JNZ_Jd(bxInstruction_c* i)
{
    if (!get_ZF()) {
        Bit32u new_EIP = EIP + (Bit32s)i->Id();
        branch_near32(new_EIP);
        BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_EIP);
        BX_LINK_TRACE(i);
    }

    BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
    BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LEA_GdM(bxInstruction_c* i)
{
    Bit32u eaddr = (Bit32u)BX_CPU_RESOLVE_ADDR(i);

    BX_WRITE_32BIT_REGZ(i->dst(), eaddr);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::PUSH_EdM(bxInstruction_c* i)
{
    Bit32u eaddr = (Bit32u)BX_CPU_RESOLVE_ADDR_32(i);

    Bit32u op1_32 = read_virtual_dword_32(i->seg(), eaddr);

    push_32(op1_32);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::DEC_EdR(bxInstruction_c* i)
{
    Bit32u erx = --BX_READ_32BIT_REG(i->dst());
    SET_FLAGS_OSZAP_SUB_32(erx + 1, 0, erx);
    BX_CLEAR_64BIT_HIGH(i->dst());

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV32S_EdGdM(bxInstruction_c* i)
{
    Bit32u eaddr = (Bit32u)BX_CPU_RESOLVE_ADDR_32(i);

    stack_write_dword(eaddr, BX_READ_32BIT_REG(i->src()));

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::TEST_EdGdR(bxInstruction_c* i)
{
    Bit32u op1_32, op2_32;

    op1_32 = BX_READ_32BIT_REG(i->dst());
    op2_32 = BX_READ_32BIT_REG(i->src());
    op1_32 &= op2_32;

    SET_FLAGS_OSZAPC_LOGIC_32(op1_32);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JLE_Jd(bxInstruction_c* i)
{
    if (get_ZF() || (getB_SF() != getB_OF())) {
        Bit32u new_EIP = EIP + (Bit32s)i->Id();
        branch_near32(new_EIP);
        BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_EIP);
        BX_LINK_TRACE(i);
    }

    BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
    BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JZ_Jd(bxInstruction_c* i)
{
    if (get_ZF()) {
        Bit32u new_EIP = EIP + (Bit32s)i->Id();
        branch_near32(new_EIP);
        BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_EIP);
        BX_LINK_TRACE(i);
    }

    BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
    BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MUL_EAXEdR(bxInstruction_c* i)
{
    Bit32u op1_32 = EAX;
    Bit32u op2_32 = BX_READ_32BIT_REG(i->src());

    Bit64u product_64 = ((Bit64u)op1_32) * ((Bit64u)op2_32);
    Bit32u product_32l = GET32L(product_64);
    Bit32u product_32h = GET32H(product_64);

    /* now write product back to destination */
    RAX = product_32l;
    RDX = product_32h;

    /* set EFLAGS */
    SET_FLAGS_OSZAPC_LOGIC_32(product_32l);
    if (product_32h != 0)
    {
        BX_CPU_THIS_PTR oszapc.assert_flags_OxxxxC();
    }

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV32_EdGdM(bxInstruction_c* i)
{
    Bit32u eaddr = (Bit32u)BX_CPU_RESOLVE_ADDR_32(i);

    write_virtual_dword_32(i->seg(), eaddr, BX_READ_32BIT_REG(i->src()));

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_OdAL(bxInstruction_c* i)
{
    write_virtual_byte_32(i->seg(), i->Id(), AL);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JNBE_Jd(bxInstruction_c* i)
{
    if (!(get_CF() || get_ZF())) {
        Bit32u new_EIP = EIP + (Bit32s)i->Id();
        branch_near32(new_EIP);
        BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_EIP);
        BX_LINK_TRACE(i);
    }

    BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
    BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::INC_EdR(bxInstruction_c* i)
{
    Bit32u erx = ++BX_READ_32BIT_REG(i->dst());
    SET_FLAGS_OSZAP_ADD_32(erx - 1, 0, erx);
    BX_CLEAR_64BIT_HIGH(i->dst());

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMP_EbIbM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    Bit32u op1_8 = read_virtual_byte(i->seg(), eaddr);
    Bit32u op2_8 = i->Ib();
    Bit32u diff_8 = op1_8 - op2_8;

    SET_FLAGS_OSZAPC_SUB_8(op1_8, op2_8, diff_8);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SUB_GdEdM(bxInstruction_c* i)
{
    Bit32u op1_32, op2_32, diff_32;

    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    op1_32 = BX_READ_32BIT_REG(i->dst());
    op2_32 = read_virtual_dword(i->seg(), eaddr);
    diff_32 = op1_32 - op2_32;
    BX_WRITE_32BIT_REGZ(i->dst(), diff_32);

    SET_FLAGS_OSZAPC_SUB_32(op1_32, op2_32, diff_32);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADD_EdIdR(bxInstruction_c* i)
{
    Bit32u op1_32, op2_32, sum_32;

    op1_32 = BX_READ_32BIT_REG(i->dst());
    op2_32 = i->Id();
    sum_32 = op1_32 + op2_32;

    BX_WRITE_32BIT_REGZ(i->dst(), sum_32);

    SET_FLAGS_OSZAPC_ADD_32(op1_32, op2_32, sum_32);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::POP_EdR(bxInstruction_c* i)
{
    BX_WRITE_32BIT_REGZ(i->dst(), pop_32());

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::RETnear32_Iw(bxInstruction_c* i)
{
    //BX_ASSERT(BX_CPU_THIS_PTR cpu_mode != BX_MODE_LONG_64);

#if BX_DEBUGGER
    BX_CPU_THIS_PTR show_flag |= Flag_ret;
#endif

    RSP_SPECULATIVE;

    Bit32u return_EIP = pop_32();
#if BX_SUPPORT_CET
    if (ShadowStackEnabled(CPL)) {
        Bit32u shadow_EIP = shadow_stack_pop_32();
        if (shadow_EIP != return_EIP)
            exception(BX_CP_EXCEPTION, BX_CP_NEAR_RET);
    }
#endif

    if (return_EIP > BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled)
    {
        //BX_ERROR(("%s: offset outside of CS limits", i->getIaOpcodeNameShort()));
        exception(BX_GP_EXCEPTION, 0);
    }
    EIP = return_EIP;

    Bit16u imm16 = i->Iw();
    if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.d_b)
        ESP += imm16;
    else
        SP += imm16;

    RSP_COMMIT;

    BX_INSTR_UCNEAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_RET, PREV_RIP, EIP);

    BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOVSX_GdEbM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    Bit8u op2_8 = read_virtual_byte(i->seg(), eaddr);

    /* sign extend byte op2 into dword op1 */
    BX_WRITE_32BIT_REGZ(i->dst(), (Bit8s)op2_8);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::NOP(bxInstruction_c* i)
{
    // No operation.

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LOAD_Ed(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);
    TMP32 = read_virtual_dword(i->seg(), eaddr);
    BX_CPU_CALL_METHOD(i->execute2(), (i));
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JMP_EdR(bxInstruction_c* i)
{
    Bit32u new_EIP = BX_READ_32BIT_REG(i->dst());
    branch_near32(new_EIP);
    BX_INSTR_UCNEAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_JMP_INDIRECT, PREV_RIP, new_EIP);

#if BX_SUPPORT_CET
    track_indirect_if_not_suppressed(i, CPL);
#endif

    BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV32_GdEdM(bxInstruction_c* i)
{
    Bit32u eaddr = (Bit32u)BX_CPU_RESOLVE_ADDR_32(i);
    Bit32u val32 = read_virtual_dword_32(i->seg(), eaddr);

    BX_WRITE_32BIT_REGZ(i->dst(), val32);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JNLE_Jd(bxInstruction_c* i)
{
    if (!get_ZF() && (getB_SF() == getB_OF())) {
        Bit32u new_EIP = EIP + (Bit32s)i->Id();
        branch_near32(new_EIP);
        BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_EIP);
        BX_LINK_TRACE(i);
    }

    BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
    BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::PUSHF_Fw(bxInstruction_c* i)
{
#if BX_SUPPORT_SVM
    if (BX_CPU_THIS_PTR in_svm_guest) {
        if (SVM_INTERCEPT(SVM_INTERCEPT0_PUSHF)) Svm_Vmexit(SVM_VMEXIT_PUSHF);
    }
#endif

    Bit16u flags = (Bit16u)read_eflags();

    if (v8086_mode()) {
        if (BX_CPU_THIS_PTR get_IOPL() < 3) {
#if BX_CPU_LEVEL >= 5
            if (BX_CPU_THIS_PTR cr4.get_VME()) {
                flags |= EFlagsIOPLMask;
                if (BX_CPU_THIS_PTR get_VIF())
                    flags |= EFlagsIFMask;
                else
                    flags &= ~EFlagsIFMask;
            }
            else
#endif
            {
                //BX_DEBUG(("PUSHFW: #GP(0) in v8086 (no VME) mode"));
                exception(BX_GP_EXCEPTION, 0);
            }
        }
    }

    push_16(flags);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::POPF_Fw(bxInstruction_c* i)
{
#if BX_SUPPORT_SVM
    if (BX_CPU_THIS_PTR in_svm_guest) {
        if (SVM_INTERCEPT(SVM_INTERCEPT0_POPF)) Svm_Vmexit(SVM_VMEXIT_POPF);
    }
#endif

    // Build a mask of the following bits:
    // x,NT,IOPL,OF,DF,IF,TF,SF,ZF,x,AF,x,PF,x,CF
    Bit32u changeMask = EFlagsOSZAPCMask | EFlagsTFMask | EFlagsDFMask | EFlagsNTMask;

    RSP_SPECULATIVE;

    Bit16u flags16 = pop_16();

    if (protected_mode()) {
        if (CPL == 0)
            changeMask |= EFlagsIOPLMask;
        if (CPL <= BX_CPU_THIS_PTR get_IOPL())
            changeMask |= EFlagsIFMask;
    }
    else if (v8086_mode()) {
        if (BX_CPU_THIS_PTR get_IOPL() < 3) {
#if BX_CPU_LEVEL >= 5
            if (BX_CPU_THIS_PTR cr4.get_VME()) {

                if (((flags16 & EFlagsIFMask) && BX_CPU_THIS_PTR get_VIP()) ||
                    (flags16 & EFlagsTFMask))
                {
                    //BX_ERROR(("POPFW: #GP(0) in VME mode"));
                    exception(BX_GP_EXCEPTION, 0);
                }

                // IF, IOPL unchanged, EFLAGS.VIF = TMP_FLAGS.IF
                changeMask |= EFlagsVIFMask;
                Bit32u flags32 = (Bit32u)flags16;
                if (flags32 & EFlagsIFMask) flags32 |= EFlagsVIFMask;
                writeEFlags(flags32, changeMask);
                RSP_COMMIT;

                BX_NEXT_INSTR(i);
            }
#endif
            //BX_DEBUG(("POPFW: #GP(0) in v8086 (no VME) mode"));
            exception(BX_GP_EXCEPTION, 0);
        }

        changeMask |= EFlagsIFMask;
    }
    else {
        // All non-reserved flags can be modified
        changeMask |= (EFlagsIOPLMask | EFlagsIFMask);
    }

    writeEFlags((Bit32u)flags16, changeMask);

    RSP_COMMIT;

    BX_NEXT_INSTR(i);
}

#if BX_CPU_LEVEL >= 3
void BX_CPP_AttrRegparmN(1) BX_CPU_C::PUSHF_Fd(bxInstruction_c* i)
{
#if BX_SUPPORT_SVM
    if (BX_CPU_THIS_PTR in_svm_guest) {
        if (SVM_INTERCEPT(SVM_INTERCEPT0_PUSHF)) Svm_Vmexit(SVM_VMEXIT_PUSHF);
    }
#endif

    if (v8086_mode() && (BX_CPU_THIS_PTR get_IOPL() < 3)) {
        //BX_DEBUG(("PUSHFD: #GP(0) in v8086 mode"));
        exception(BX_GP_EXCEPTION, 0);
    }

    // VM & RF flags cleared in image stored on the stack
    push_32(read_eflags() & 0x00fcffff);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::POPF_Fd(bxInstruction_c* i)
{
#if BX_SUPPORT_SVM
    if (BX_CPU_THIS_PTR in_svm_guest) {
        if (SVM_INTERCEPT(SVM_INTERCEPT0_POPF)) Svm_Vmexit(SVM_VMEXIT_POPF);
    }
#endif

    // Build a mask of the following bits:
    // ID,VIP,VIF,AC,VM,RF,x,NT,IOPL,OF,DF,IF,TF,SF,ZF,x,AF,x,PF,x,CF
    Bit32u changeMask = EFlagsOSZAPCMask | EFlagsTFMask |
        EFlagsDFMask | EFlagsNTMask | EFlagsRFMask;
#if BX_CPU_LEVEL >= 4
    changeMask |= (EFlagsIDMask | EFlagsACMask);  // ID/AC
#endif

    RSP_SPECULATIVE;

    // RF is always zero after the execution of POPF.
    Bit32u flags32 = pop_32() & ~EFlagsRFMask;

    if (protected_mode()) {
        // IOPL changed only if (CPL == 0),
        // IF changed only if (CPL <= EFLAGS.IOPL),
        // VIF, VIP, VM are unaffected
        if (CPL == 0)
            changeMask |= EFlagsIOPLMask;
        if (CPL <= BX_CPU_THIS_PTR get_IOPL())
            changeMask |= EFlagsIFMask;
    }
    else if (v8086_mode()) {
        if (BX_CPU_THIS_PTR get_IOPL() < 3) {
            //BX_ERROR(("POPFD: #GP(0) in v8086 mode"));
            exception(BX_GP_EXCEPTION, 0);
        }
        // v8086-mode: VM, IOPL, VIP, VIF are unaffected
        changeMask |= EFlagsIFMask;
    }
    else { // Real-mode
        // VIF, VIP, VM are unaffected
        changeMask |= (EFlagsIOPLMask | EFlagsIFMask);
    }

    writeEFlags(flags32, changeMask);

    RSP_COMMIT;

    BX_NEXT_INSTR(i);
}
#endif  // BX_CPU_LEVEL >= 3

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SUB_EdGdM(bxInstruction_c* i)
{
    Bit32u op1_32, op2_32, diff_32;

    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    op1_32 = read_RMW_virtual_dword(i->seg(), eaddr);
    op2_32 = BX_READ_32BIT_REG(i->src());
    diff_32 = op1_32 - op2_32;
    write_RMW_linear_dword(diff_32);

    SET_FLAGS_OSZAPC_SUB_32(op1_32, op2_32, diff_32);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OR_GbEbR(bxInstruction_c* i)
{
    Bit8u op1, op2;

    op1 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());
    op2 = BX_READ_8BIT_REGx(i->src(), i->extend8bitL());
    op1 |= op2;
    BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), op1);

    SET_FLAGS_OSZAPC_LOGIC_8(op1);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOVZX_GdEbR(bxInstruction_c* i)
{
    Bit8u op2_8 = BX_READ_8BIT_REGx(i->src(), i->extend8bitL());

    /* zero extend byte op2 into dword op1 */
    BX_WRITE_32BIT_REGZ(i->dst(), (Bit32u)op2_8);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::DIV_EAXEdR(bxInstruction_c* i)
{
    Bit32u op2_32 = BX_READ_32BIT_REG(i->src());
    if (op2_32 == 0) {
        exception(BX_DE_EXCEPTION, 0);
    }

    Bit64u op1_64 = GET64_FROM_HI32_LO32(EDX, EAX);

    Bit64u quotient_64 = op1_64 / op2_32;
    Bit32u remainder_32 = (Bit32u)(op1_64 % op2_32);
    Bit32u quotient_32l = (Bit32u)(quotient_64 & 0xFFFFFFFF);

    if (quotient_64 != quotient_32l)
    {
        exception(BX_DE_EXCEPTION, 0);
    }

    /* set EFLAGS:
     * DIV affects the following flags: O,S,Z,A,P,C are undefined
     */

     /* now write quotient back to destination */
    RAX = quotient_32l;
    RDX = remainder_32;

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OR_GdEdR(bxInstruction_c* i)
{
    Bit32u op1_32, op2_32;

    op1_32 = BX_READ_32BIT_REG(i->dst());
    op2_32 = BX_READ_32BIT_REG(i->src());
    op1_32 |= op2_32;
    BX_WRITE_32BIT_REGZ(i->dst(), op1_32);

    SET_FLAGS_OSZAPC_LOGIC_32(op1_32);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JBE_Jd(bxInstruction_c* i)
{
    if (get_CF() || get_ZF()) {
        Bit32u new_EIP = EIP + (Bit32s)i->Id();
        branch_near32(new_EIP);
        BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_EIP);
        BX_LINK_TRACE(i);
    }

    BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
    BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMP_GdEdM(bxInstruction_c* i)
{
    Bit32u op1_32, op2_32, diff_32;

    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    op1_32 = BX_READ_32BIT_REG(i->dst());
    op2_32 = read_virtual_dword(i->seg(), eaddr);
    diff_32 = op1_32 - op2_32;

    SET_FLAGS_OSZAPC_SUB_32(op1_32, op2_32, diff_32);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JS_Jd(bxInstruction_c* i)
{
    if (get_SF()) {
        Bit32u new_EIP = EIP + (Bit32s)i->Id();
        branch_near32(new_EIP);
        BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_EIP);
        BX_LINK_TRACE(i);
    }

    BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
    BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SHRD_EdGdR(bxInstruction_c* i)
{
    Bit32u op1_32, op2_32, result_32;
    unsigned count;
    unsigned cf, of;

    if (i->getIaOpcode() == BX_IA_SHRD_EdGd)
        count = CL;
    else // BX_IA_SHRD_EdGdIb
        count = i->Ib();

    count &= 0x1f; // use only 5 LSB's

    if (!count) {
        BX_CLEAR_64BIT_HIGH(i->dst()); // always clear upper part of the register
    }
    else {
        op1_32 = BX_READ_32BIT_REG(i->dst());
        op2_32 = BX_READ_32BIT_REG(i->src());

        result_32 = (op2_32 << (32 - count)) | (op1_32 >> count);

        BX_WRITE_32BIT_REGZ(i->dst(), result_32);

        SET_FLAGS_OSZAPC_LOGIC_32(result_32);

        cf = (op1_32 >> (count - 1)) & 0x1;
        of = ((result_32 << 1) ^ result_32) >> 31; // of = result30 ^ result31
        BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);
    }

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SHR_EdR(bxInstruction_c* i)
{
    unsigned count;

    if (i->getIaOpcode() == BX_IA_SHR_Ed)
        count = CL;
    else
        count = i->Ib();

    count &= 0x1f;

    if (!count) {
        BX_CLEAR_64BIT_HIGH(i->dst()); // always clear upper part of the register
    }
    else {
        Bit32u op1_32 = BX_READ_32BIT_REG(i->dst());
        Bit32u result_32 = (op1_32 >> count);
        BX_WRITE_32BIT_REGZ(i->dst(), result_32);

        unsigned cf = (op1_32 >> (count - 1)) & 0x1;
        // note, that of == result31 if count == 1 and
        //            of == 0        if count >= 2
        unsigned of = ((result_32 << 1) ^ result_32) >> 31;

        SET_FLAGS_OSZAPC_LOGIC_32(result_32);
        BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);
    }

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CPUID(bxInstruction_c* i)
{
#if BX_CPU_LEVEL >= 4

#if BX_SUPPORT_VMX
    if (BX_CPU_THIS_PTR in_vmx_guest) {
        VMexit(VMX_VMEXIT_CPUID, 0);
    }
#endif

#if BX_SUPPORT_SVM
    if (BX_CPU_THIS_PTR in_svm_guest) {
        if (SVM_INTERCEPT(SVM_INTERCEPT0_CPUID)) Svm_Vmexit(SVM_VMEXIT_CPUID);
    }
#endif

    struct cpuid_function_t leaf;
    BX_CPU_THIS_PTR cpuid->get_cpuid_leaf(EAX, ECX, &leaf);

    RAX = leaf.eax;
    RBX = leaf.ebx;
    RCX = leaf.ecx;
    RDX = leaf.edx;
#endif

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_OdEAX(bxInstruction_c* i)
{
    write_virtual_dword_32(i->seg(), i->Id(), EAX);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_EAXOd(bxInstruction_c* i)
{
    RAX = read_virtual_dword_32(i->seg(), i->Id());

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::TEST_EbIbM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    Bit8u op1 = read_virtual_byte(i->seg(), eaddr);
    op1 &= i->Ib();
    SET_FLAGS_OSZAPC_LOGIC_8(op1);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOVZX_GdEwM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    Bit16u op2_16 = read_virtual_word(i->seg(), eaddr);

    /* zero extend word op2 into dword op1 */
    BX_WRITE_32BIT_REGZ(i->dst(), (Bit32u)op2_16);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::RDMSR(bxInstruction_c* i)
{
#if BX_CPU_LEVEL >= 5
    // CPL is always 0 in real mode
    if (/* !real_mode() && */ CPL != 0) {
        //BX_ERROR(("%s: CPL != 0 not in real mode", i->getIaOpcodeNameShort()));
        exception(BX_GP_EXCEPTION, 0);
    }

    Bit32u index = ECX;
#if BX_SUPPORT_X86_64 && BX_SUPPORT_AVX
    if (i->getIaOpcode() == BX_IA_RDMSR_EqId) index = i->Id();
#endif
    Bit64u val64 = 0;

#if BX_SUPPORT_SVM
    if (BX_CPU_THIS_PTR in_svm_guest) {
        if (SVM_INTERCEPT(SVM_INTERCEPT0_MSR)) SvmInterceptMSR(BX_READ, index);
    }
#endif

#if BX_SUPPORT_VMX
    if (BX_CPU_THIS_PTR in_vmx_guest)
        VMexit_MSR(VMX_VMEXIT_RDMSR, index);
#endif

    if (!rdmsr(index, &val64))
        exception(BX_GP_EXCEPTION, 0);

#if BX_SUPPORT_X86_64 && BX_SUPPORT_AVX
    if (i->getIaOpcode() == BX_IA_RDMSR_EqId) {
        BX_WRITE_64BIT_REG(i->dst(), val64);
    }
    else
#endif
    {
        RAX = GET32L(val64);
        RDX = GET32H(val64);
    }
#endif

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::TEST_EbIbR(bxInstruction_c* i)
{
    Bit8u op1 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());
    op1 &= i->Ib();
    SET_FLAGS_OSZAPC_LOGIC_8(op1);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JB_Jd(bxInstruction_c* i)
{
    if (get_CF()) {
        Bit32u new_EIP = EIP + (Bit32s)i->Id();
        branch_near32(new_EIP);
        BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_EIP);
        BX_LINK_TRACE(i);
    }

    BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
    BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::WRMSR(bxInstruction_c* i)
{
#if BX_CPU_LEVEL >= 5
    // CPL is always 0 in real mode
    if (/* !real_mode() && */ CPL != 0) {
        //BX_ERROR(("%s: CPL != 0 not in real mode", i->getIaOpcodeNameShort()));
        exception(BX_GP_EXCEPTION, 0);
    }

    invalidate_prefetch_q();

    Bit64u val_64;
    Bit32u index;

#if BX_SUPPORT_X86_64 && BX_SUPPORT_AVX
    if (i->getIaOpcode() == BX_IA_WRMSRNS_IdEq) {
        val_64 = BX_READ_64BIT_REG(i->src());
        index = i->Id();
    }
    else
#endif
    {
        val_64 = GET64_FROM_HI32_LO32(EDX, EAX);
        index = ECX;
    }

#if BX_SUPPORT_SVM
    if (BX_CPU_THIS_PTR in_svm_guest) {
        if (SVM_INTERCEPT(SVM_INTERCEPT0_MSR)) SvmInterceptMSR(BX_WRITE, index);
    }
#endif

#if BX_SUPPORT_VMX
    if (BX_CPU_THIS_PTR in_vmx_guest)
        VMexit_MSR(VMX_VMEXIT_WRMSR, index);
#endif

    if (!wrmsr(index, val_64))
        exception(BX_GP_EXCEPTION, 0);
#endif

    BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::NOT_EdR(bxInstruction_c* i)
{
    Bit32u op1_32 = BX_READ_32BIT_REG(i->dst());
    op1_32 = ~op1_32;
    BX_WRITE_32BIT_REGZ(i->dst(), op1_32);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JL_Jd(bxInstruction_c* i)
{
    if (getB_SF() != getB_OF()) {
        Bit32u new_EIP = EIP + (Bit32s)i->Id();
        branch_near32(new_EIP);
        BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_EIP);
        BX_LINK_TRACE(i);
    }

    BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
    BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OR_EdIdR(bxInstruction_c* i)
{
    Bit32u op1_32 = BX_READ_32BIT_REG(i->dst());
    op1_32 |= i->Id();
    BX_WRITE_32BIT_REGZ(i->dst(), op1_32);

    SET_FLAGS_OSZAPC_LOGIC_32(op1_32);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMP_EdIdM(bxInstruction_c* i)
{
    Bit32u op1_32, op2_32, diff_32;

    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    op1_32 = read_virtual_dword(i->seg(), eaddr);
    op2_32 = i->Id();
    diff_32 = op1_32 - op2_32;

    SET_FLAGS_OSZAPC_SUB_32(op1_32, op2_32, diff_32);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OUT_DXEAX(bxInstruction_c* i)
{
    unsigned port = DX;

    if (!allow_io(i, port, 4)) {
        //BX_DEBUG(("OUT_DXEAX: I/O access not allowed !"));
        exception(BX_GP_EXCEPTION, 0);
    }

    BX_OUTP(port, EAX, 4);

    BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::IN_AXDX(bxInstruction_c* i)
{
    unsigned port = DX;

    if (!allow_io(i, port, 2)) {
        //BX_DEBUG(("IN_AXDX: I/O access not allowed !"));
        exception(BX_GP_EXCEPTION, 0);
    }

    AX = BX_INP(port, 2);

    BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMP_EwIwR(bxInstruction_c* i)
{
    Bit32u op1_16 = BX_READ_16BIT_REG(i->dst());
    Bit32u op2_16 = i->Iw();
    Bit32u diff_16 = op1_16 - op2_16;

    SET_FLAGS_OSZAPC_SUB_16(op1_16, op2_16, diff_16);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SAR_EdR(bxInstruction_c* i)
{
    unsigned count;

    if (i->getIaOpcode() == BX_IA_SAR_Ed)
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
        Bit32u result_32 = ((Bit32s)op1_32) >> count;

        BX_WRITE_32BIT_REGZ(i->dst(), result_32);

        SET_FLAGS_OSZAPC_LOGIC_32(result_32);
        unsigned cf = (op1_32 >> (count - 1)) & 1;
        BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(0, cf); /* signed overflow cannot happen in SAR instruction */
    }

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JNL_Jd(bxInstruction_c* i)
{
    if (getB_SF() == getB_OF()) {
        Bit32u new_EIP = EIP + (Bit32s)i->Id();
        branch_near32(new_EIP);
        BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_EIP);
        BX_LINK_TRACE(i);
    }

    BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
    BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADD_EbGbM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    Bit32u op1 = read_RMW_virtual_byte(i->seg(), eaddr);
    Bit32u op2 = BX_READ_8BIT_REGx(i->src(), i->extend8bitL());
    Bit32u sum = op1 + op2;

    write_RMW_linear_byte(sum);

    SET_FLAGS_OSZAPC_ADD_8(op1, op2, sum);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::REP_CMPSB_XbYb(bxInstruction_c* i)
{
#if BX_SUPPORT_X86_64
    if (i->as64L()) {
        BX_CPU_THIS_PTR repeat_ZF(i, &BX_CPU_C::CMPSB64_XbYb);
    }
    else
#endif
        if (i->as32L()) {
            BX_CPU_THIS_PTR repeat_ZF(i, &BX_CPU_C::CMPSB32_XbYb);
            BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RSI); // always clear upper part of RSI/RDI
            BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RDI);
        }
        else {
            BX_CPU_THIS_PTR repeat_ZF(i, &BX_CPU_C::CMPSB16_XbYb);
        }

    BX_NEXT_INSTR(i);
}

/* 16 bit address size */
void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMPSB16_XbYb(bxInstruction_c* i)
{
    Bit8u op1_8, op2_8, diff_8;

    Bit16u si = SI;
    Bit16u di = DI;

    op1_8 = read_virtual_byte_32(i->seg(), si);
    op2_8 = read_virtual_byte_32(BX_SEG_REG_ES, di);

    diff_8 = op1_8 - op2_8;

    SET_FLAGS_OSZAPC_SUB_8(op1_8, op2_8, diff_8);

    if (BX_CPU_THIS_PTR get_DF()) {
        si--;
        di--;
    }
    else {
        si++;
        di++;
    }

    DI = di;
    SI = si;
}

/* 32 bit address size */
void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMPSB32_XbYb(bxInstruction_c* i)
{
    Bit8u op1_8, op2_8, diff_8;

    Bit32u esi = ESI;
    Bit32u edi = EDI;

    op1_8 = read_virtual_byte(i->seg(), esi);
    op2_8 = read_virtual_byte(BX_SEG_REG_ES, edi);

    diff_8 = op1_8 - op2_8;

    SET_FLAGS_OSZAPC_SUB_8(op1_8, op2_8, diff_8);

    if (BX_CPU_THIS_PTR get_DF()) {
        esi--;
        edi--;
    }
    else {
        esi++;
        edi++;
    }

    // zero extension of RSI/RDI
    RDI = edi;
    RSI = esi;
}

#if BX_SUPPORT_X86_64
/* 64 bit address size */
void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMPSB64_XbYb(bxInstruction_c* i)
{
    Bit8u op1_8, op2_8, diff_8;

    Bit64u rsi = RSI;
    Bit64u rdi = RDI;

    op1_8 = read_linear_byte(i->seg(), get_laddr64(i->seg(), rsi));
    op2_8 = read_linear_byte(BX_SEG_REG_ES, rdi);

    diff_8 = op1_8 - op2_8;

    SET_FLAGS_OSZAPC_SUB_8(op1_8, op2_8, diff_8);

    if (BX_CPU_THIS_PTR get_DF()) {
        rsi--;
        rdi--;
    }
    else {
        rsi++;
        rdi++;
    }

    RDI = rdi;
    RSI = rsi;
}
#endif

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOVZX_GdEbM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    Bit8u op2_8 = read_virtual_byte(i->seg(), eaddr);

    /* zero extend byte op2 into dword op1 */
    BX_WRITE_32BIT_REGZ(i->dst(), (Bit32u)op2_8);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADD_EdIdM(bxInstruction_c* i)
{
    Bit32u op1_32, op2_32, sum_32;

    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    op1_32 = read_RMW_virtual_dword(i->seg(), eaddr);
    op2_32 = i->Id();
    sum_32 = op1_32 + op2_32;
    write_RMW_linear_dword(sum_32);

    SET_FLAGS_OSZAPC_ADD_32(op1_32, op2_32, sum_32);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OR_GdEdM(bxInstruction_c* i)
{
    Bit32u op1_32, op2_32;

    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    op1_32 = BX_READ_32BIT_REG(i->dst());
    op2_32 = read_virtual_dword(i->seg(), eaddr);
    op1_32 |= op2_32;
    BX_WRITE_32BIT_REGZ(i->dst(), op1_32);

    SET_FLAGS_OSZAPC_LOGIC_32(op1_32);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::IN_EAXDX(bxInstruction_c* i)
{
    unsigned port = DX;

    if (!allow_io(i, port, 4)) {
        //BX_DEBUG(("IN_EAXDX: I/O access not allowed !"));
        exception(BX_GP_EXCEPTION, 0);
    }

    RAX = BX_INP(port, 4);

    BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::NEG_EdR(bxInstruction_c* i)
{
    Bit32u op1_32 = BX_READ_32BIT_REG(i->dst());
    op1_32 = -op1_32;
    BX_WRITE_32BIT_REGZ(i->dst(), op1_32);

    SET_FLAGS_OSZAPC_SUB_32(0, 0 - op1_32, op1_32);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::AND_GdEdR(bxInstruction_c* i)
{
    Bit32u op1_32, op2_32;

    op1_32 = BX_READ_32BIT_REG(i->dst());
    op2_32 = BX_READ_32BIT_REG(i->src());
    op1_32 &= op2_32;
    BX_WRITE_32BIT_REGZ(i->dst(), op1_32);

    SET_FLAGS_OSZAPC_LOGIC_32(op1_32);

    BX_NEXT_INSTR(i);
}








