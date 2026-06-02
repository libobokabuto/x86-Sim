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

#define BX_SETCC_HANDLERS(mem_name, reg_name, expr)             \
void BX_CPP_AttrRegparmN(1) BX_CPU_C::mem_name(bxInstruction_c* i) \
{                                                               \
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);                  \
    Bit8u result_8 = (expr);                                    \
    write_virtual_byte(i->seg(), eaddr, result_8);              \
    BX_NEXT_INSTR(i);                                           \
}                                                               \
                                                                \
void BX_CPP_AttrRegparmN(1) BX_CPU_C::reg_name(bxInstruction_c* i) \
{                                                               \
    Bit8u result_8 = (expr);                                    \
    BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), result_8);   \
    BX_NEXT_INSTR(i);                                           \
}

BX_SETCC_HANDLERS(SETO_EbM, SETO_EbR, getB_OF())
BX_SETCC_HANDLERS(SETNO_EbM, SETNO_EbR, !getB_OF())
BX_SETCC_HANDLERS(SETB_EbM, SETB_EbR, getB_CF())
BX_SETCC_HANDLERS(SETNB_EbM, SETNB_EbR, !getB_CF())
BX_SETCC_HANDLERS(SETZ_EbM, SETZ_EbR, getB_ZF())
BX_SETCC_HANDLERS(SETNZ_EbM, SETNZ_EbR, !getB_ZF())
BX_SETCC_HANDLERS(SETBE_EbM, SETBE_EbR, (getB_CF() | getB_ZF()))
BX_SETCC_HANDLERS(SETNBE_EbM, SETNBE_EbR, !(getB_CF() | getB_ZF()))
BX_SETCC_HANDLERS(SETS_EbM, SETS_EbR, getB_SF())
BX_SETCC_HANDLERS(SETNS_EbM, SETNS_EbR, !getB_SF())
BX_SETCC_HANDLERS(SETP_EbM, SETP_EbR, getB_PF())
BX_SETCC_HANDLERS(SETNP_EbM, SETNP_EbR, !getB_PF())
BX_SETCC_HANDLERS(SETL_EbM, SETL_EbR, (getB_SF() ^ getB_OF()))
BX_SETCC_HANDLERS(SETNL_EbM, SETNL_EbR, !(getB_SF() ^ getB_OF()))
BX_SETCC_HANDLERS(SETLE_EbM, SETLE_EbR, (getB_ZF() | (getB_SF() ^ getB_OF())))
BX_SETCC_HANDLERS(SETNLE_EbM, SETNLE_EbR, !(getB_ZF() | (getB_SF() ^ getB_OF())))

#undef BX_SETCC_HANDLERS

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOVZX_GwEbM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    Bit8u op2_8 = read_virtual_byte(i->seg(), eaddr);

    BX_WRITE_16BIT_REG(i->dst(), (Bit16u)op2_8);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOVZX_GwEbR(bxInstruction_c* i)
{
    Bit8u op2_8 = BX_READ_8BIT_REGx(i->src(), i->extend8bitL());

    BX_WRITE_16BIT_REG(i->dst(), (Bit16u)op2_8);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOVZX_GdEwR(bxInstruction_c* i)
{
    Bit16u op2_16 = BX_READ_16BIT_REG(i->src());

    BX_WRITE_32BIT_REGZ(i->dst(), (Bit32u)op2_16);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOVSX_GwEbM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    Bit8u op2_8 = read_virtual_byte(i->seg(), eaddr);

    BX_WRITE_16BIT_REG(i->dst(), (Bit8s)op2_8);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOVSX_GwEbR(bxInstruction_c* i)
{
    Bit8u op2_8 = BX_READ_8BIT_REGx(i->src(), i->extend8bitL());

    BX_WRITE_16BIT_REG(i->dst(), (Bit8s)op2_8);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOVSX_GdEbR(bxInstruction_c* i)
{
    Bit8u op2_8 = BX_READ_8BIT_REGx(i->src(), i->extend8bitL());

    BX_WRITE_32BIT_REGZ(i->dst(), (Bit8s)op2_8);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOVSX_GdEwM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    Bit16u op2_16 = read_virtual_word(i->seg(), eaddr);

    BX_WRITE_32BIT_REGZ(i->dst(), (Bit16s)op2_16);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOVSX_GdEwR(bxInstruction_c* i)
{
    Bit16u op2_16 = BX_READ_16BIT_REG(i->src());

    BX_WRITE_32BIT_REGZ(i->dst(), (Bit16s)op2_16);

    BX_NEXT_INSTR(i);
}
void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOVZX_GdEbR(bxInstruction_c* i)
{
    Bit8u op2_8 = BX_READ_8BIT_REGx(i->src(), i->extend8bitL());

    /* zero extend byte op2 into dword op1 */
    BX_WRITE_32BIT_REGZ(i->dst(), (Bit32u)op2_8);

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

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BSF_GwEwR(bxInstruction_c* i)
{
  Bit16u op2_16 = BX_READ_16BIT_REG(i->src());

  if (op2_16 == 0) {
    assert_ZF(); /* op1_16 undefined */
  }
  else {
    Bit16u op1_16 = 0;
    while ((op2_16 & 0x1) == 0) {
      op1_16++;
      op2_16 >>= 1;
    }
    SET_FLAGS_OSZAPC_LOGIC_16(op1_16);
    clear_ZF();

    BX_WRITE_16BIT_REG(i->dst(), op1_16);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BSR_GwEwR(bxInstruction_c* i)
{
  Bit16u op2_16 = BX_READ_16BIT_REG(i->src());

  if (op2_16 == 0) {
    assert_ZF(); /* op1_16 undefined */
  }
  else {
    Bit16u op1_16 = 15;
    while ((op2_16 & 0x8000) == 0) {
      op1_16--;
      op2_16 <<= 1;
    }

    SET_FLAGS_OSZAPC_LOGIC_16(op1_16);
    clear_ZF();

    BX_WRITE_16BIT_REG(i->dst(), op1_16);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BT_EwGwM(bxInstruction_c* i)
{
  bx_address op1_addr;
  Bit16u op1_16, op2_16, index;
  Bit32s displacement32;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op2_16 = BX_READ_16BIT_REG(i->src());
  index = op2_16 & 0xf;
  displacement32 = ((Bit16s) (op2_16&0xfff0)) / 16;
  op1_addr = eaddr + 2 * displacement32;

  /* pointer, segment address pair */
  op1_16 = read_virtual_word(i->seg(), op1_addr & i->asize_mask());

  set_CF((op1_16 >> index) & 0x01);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BT_EwGwR(bxInstruction_c* i)
{
  Bit16u op1_16, op2_16;

  op1_16 = BX_READ_16BIT_REG(i->dst());
  op2_16 = BX_READ_16BIT_REG(i->src());
  op2_16 &= 0xf;
  set_CF((op1_16 >> op2_16) & 0x01);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BTS_EwGwM(bxInstruction_c* i)
{
  bx_address op1_addr;
  Bit16u op1_16, op2_16, index;
  Bit32s displacement32;
  bool bit_i;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op2_16 = BX_READ_16BIT_REG(i->src());
  index = op2_16 & 0xf;
  displacement32 = ((Bit16s) (op2_16 & 0xfff0)) / 16;
  op1_addr = eaddr + 2 * displacement32;

  /* pointer, segment address pair */
  op1_16 = read_RMW_virtual_word(i->seg(), op1_addr & i->asize_mask());
  bit_i = (op1_16 >> index) & 0x01;
  op1_16 |= (1 << index);
  write_RMW_linear_word(op1_16);

  set_CF(bit_i);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BTS_EwGwR(bxInstruction_c* i)
{
  Bit16u op1_16, op2_16;

  op1_16 = BX_READ_16BIT_REG(i->dst());
  op2_16 = BX_READ_16BIT_REG(i->src());
  op2_16 &= 0xf;
  set_CF((op1_16 >> op2_16) & 0x01);
  op1_16 |= (1 << op2_16);

  /* now write result back to the destination */
  BX_WRITE_16BIT_REG(i->dst(), op1_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BTR_EwGwM(bxInstruction_c* i)
{
  bx_address op1_addr;
  Bit16u op1_16, op2_16, index;
  Bit32s displacement32;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op2_16 = BX_READ_16BIT_REG(i->src());
  index = op2_16 & 0xf;
  displacement32 = ((Bit16s) (op2_16&0xfff0)) / 16;
  op1_addr = eaddr + 2 * displacement32;

  /* pointer, segment address pair */
  op1_16 = read_RMW_virtual_word(i->seg(), op1_addr & i->asize_mask());
  bool temp_cf = (op1_16 >> index) & 0x01;
  op1_16 &= ~(1 << index);

  /* now write back to destination */
  write_RMW_linear_word(op1_16);

  set_CF(temp_cf);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BTR_EwGwR(bxInstruction_c* i)
{
  Bit16u op1_16, op2_16;

  op1_16 = BX_READ_16BIT_REG(i->dst());
  op2_16 = BX_READ_16BIT_REG(i->src());
  op2_16 &= 0xf;
  set_CF((op1_16 >> op2_16) & 0x01);
  op1_16 &= ~(1 << op2_16);

  /* now write result back to the destination */
  BX_WRITE_16BIT_REG(i->dst(), op1_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BTC_EwGwM(bxInstruction_c* i)
{
  bx_address op1_addr;
  Bit16u op1_16, op2_16, index_16;
  Bit16s displacement16;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op2_16 = BX_READ_16BIT_REG(i->src());
  index_16 = op2_16 & 0xf;
  displacement16 = ((Bit16s) (op2_16 & 0xfff0)) / 16;
  op1_addr = eaddr + 2 * displacement16;

  op1_16 = read_RMW_virtual_word(i->seg(), op1_addr & i->asize_mask());
  bool temp_CF = (op1_16 >> index_16) & 0x01;
  op1_16 ^= (1 << index_16);  /* toggle bit */
  write_RMW_linear_word(op1_16);

  set_CF(temp_CF);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BTC_EwGwR(bxInstruction_c* i)
{
  Bit16u op1_16, op2_16;

  op1_16 = BX_READ_16BIT_REG(i->dst());
  op2_16 = BX_READ_16BIT_REG(i->src());
  op2_16 &= 0xf;

  bool temp_CF = (op1_16 >> op2_16) & 0x01;
  op1_16 ^= (1 << op2_16);  /* toggle bit */
  BX_WRITE_16BIT_REG(i->dst(), op1_16);

  set_CF(temp_CF);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BT_EwIbM(bxInstruction_c* i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit16u op1_16 = read_virtual_word(i->seg(), eaddr);
  Bit8u  op2_8  = i->Ib() & 0xf;

  set_CF((op1_16 >> op2_8) & 0x01);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BT_EwIbR(bxInstruction_c* i)
{
  Bit16u op1_16 = BX_READ_16BIT_REG(i->dst());
  Bit8u  op2_8  = i->Ib() & 0xf;

  set_CF((op1_16 >> op2_8) & 0x01);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BTS_EwIbM(bxInstruction_c* i)
{
  Bit8u op2_8 = i->Ib() & 0xf;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit16u op1_16 = read_RMW_virtual_word(i->seg(), eaddr);
  bool temp_CF = (op1_16 >> op2_8) & 0x01;
  op1_16 |= (1 << op2_8);
  write_RMW_linear_word(op1_16);

  set_CF(temp_CF);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BTS_EwIbR(bxInstruction_c* i)
{
  Bit8u op2_8 = i->Ib() & 0xf;

  Bit16u op1_16 = BX_READ_16BIT_REG(i->dst());
  bool temp_CF = (op1_16 >> op2_8) & 0x01;
  op1_16 |= (1 << op2_8);
  BX_WRITE_16BIT_REG(i->dst(), op1_16);

  set_CF(temp_CF);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BTC_EwIbM(bxInstruction_c* i)
{
  Bit8u op2_8 = i->Ib() & 0xf;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit16u op1_16 = read_RMW_virtual_word(i->seg(), eaddr);
  bool temp_CF = (op1_16 >> op2_8) & 0x01;
  op1_16 ^= (1 << op2_8);  /* toggle bit */
  write_RMW_linear_word(op1_16);

  set_CF(temp_CF);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BTC_EwIbR(bxInstruction_c* i)
{
  Bit8u op2_8 = i->Ib() & 0xf;

  Bit16u op1_16 = BX_READ_16BIT_REG(i->dst());
  bool temp_CF = (op1_16 >> op2_8) & 0x01;
  op1_16 ^= (1 << op2_8);  /* toggle bit */
  BX_WRITE_16BIT_REG(i->dst(), op1_16);

  set_CF(temp_CF);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BTR_EwIbM(bxInstruction_c* i)
{
  Bit8u op2_8 = i->Ib() & 0xf;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit16u op1_16 = read_RMW_virtual_word(i->seg(), eaddr);
  bool temp_CF = (op1_16 >> op2_8) & 0x01;
  op1_16 &= ~(1 << op2_8);
  write_RMW_linear_word(op1_16);

  set_CF(temp_CF);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BTR_EwIbR(bxInstruction_c* i)
{
  Bit8u op2_8 = i->Ib() & 0xf;

  Bit16u op1_16 = BX_READ_16BIT_REG(i->dst());
  bool temp_CF = (op1_16 >> op2_8) & 0x01;
  op1_16 &= ~(1 << op2_8);
  BX_WRITE_16BIT_REG(i->dst(), op1_16);

  set_CF(temp_CF);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BSF_GdEdR(bxInstruction_c* i)
{
  Bit32u op2_32 = BX_READ_32BIT_REG(i->src());

  if (op2_32 == 0) {
    assert_ZF(); /* op1_32 undefined */
  }
  else {
    Bit32u op1_32 = 0;
    while ((op2_32 & 0x1) == 0) {
      op1_32++;
      op2_32 >>= 1;
    }
    SET_FLAGS_OSZAPC_LOGIC_32(op1_32);
    clear_ZF();

    BX_WRITE_32BIT_REGZ(i->dst(), op1_32);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BSR_GdEdR(bxInstruction_c* i)
{
  Bit32u op2_32 = BX_READ_32BIT_REG(i->src());

  if (op2_32 == 0) {
    assert_ZF(); /* op1_32 undefined */
  }
  else {
    Bit32u op1_32 = 31;
    while ((op2_32 & 0x80000000) == 0) {
      op1_32--;
      op2_32 <<= 1;
    }

    SET_FLAGS_OSZAPC_LOGIC_32(op1_32);
    clear_ZF();

    BX_WRITE_32BIT_REGZ(i->dst(), op1_32);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BT_EdGdM(bxInstruction_c* i)
{
  bx_address op1_addr;
  Bit32u op1_32, op2_32, index;
  Bit32s displacement32;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op2_32 = BX_READ_32BIT_REG(i->src());
  index = op2_32 & 0x1f;
  displacement32 = ((Bit32s) (op2_32&0xffffffe0)) / 32;
  op1_addr = eaddr + 4 * displacement32;

  /* pointer, segment address pair */
  op1_32 = read_virtual_dword(i->seg(), op1_addr & i->asize_mask());

  set_CF((op1_32 >> index) & 0x01);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BT_EdGdR(bxInstruction_c* i)
{
  Bit32u op1_32, op2_32;

  op1_32 = BX_READ_32BIT_REG(i->dst());
  op2_32 = BX_READ_32BIT_REG(i->src());
  op2_32 &= 0x1f;

  set_CF((op1_32 >> op2_32) & 0x01);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BTS_EdGdM(bxInstruction_c* i)
{
  bx_address op1_addr;
  Bit32u op1_32, op2_32, index;
  Bit32s displacement32;
  bool bit_i;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op2_32 = BX_READ_32BIT_REG(i->src());
  index = op2_32 & 0x1f;
  displacement32 = ((Bit32s) (op2_32&0xffffffe0)) / 32;
  op1_addr = eaddr + 4 * displacement32;

  /* pointer, segment address pair */
  op1_32 = read_RMW_virtual_dword(i->seg(), op1_addr & i->asize_mask());

  bit_i = (op1_32 >> index) & 0x01;
  op1_32 |= (1 << index);

  write_RMW_linear_dword(op1_32);

  set_CF(bit_i);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BTS_EdGdR(bxInstruction_c* i)
{
  Bit32u op1_32, op2_32;

  op1_32 = BX_READ_32BIT_REG(i->dst());
  op2_32 = BX_READ_32BIT_REG(i->src());
  op2_32 &= 0x1f;
  set_CF((op1_32 >> op2_32) & 0x01);
  op1_32 |= (1 << op2_32);

  /* now write result back to the destination */
  BX_WRITE_32BIT_REGZ(i->dst(), op1_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BTR_EdGdM(bxInstruction_c* i)
{
  bx_address op1_addr;
  Bit32u op1_32, op2_32, index;
  Bit32s displacement32;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op2_32 = BX_READ_32BIT_REG(i->src());
  index = op2_32 & 0x1f;
  displacement32 = ((Bit32s) (op2_32&0xffffffe0)) / 32;
  op1_addr = eaddr + 4 * displacement32;

  /* pointer, segment address pair */
  op1_32 = read_RMW_virtual_dword(i->seg(), op1_addr & i->asize_mask());

  bool temp_cf = (op1_32 >> index) & 0x01;
  op1_32 &= ~(1 << index);

  /* now write back to destination */
  write_RMW_linear_dword(op1_32);

  set_CF(temp_cf);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BTR_EdGdR(bxInstruction_c* i)
{
  Bit32u op1_32, op2_32;

  op1_32 = BX_READ_32BIT_REG(i->dst());
  op2_32 = BX_READ_32BIT_REG(i->src());
  op2_32 &= 0x1f;
  set_CF((op1_32 >> op2_32) & 0x01);
  op1_32 &= ~(1 << op2_32);

  /* now write result back to the destination */
  BX_WRITE_32BIT_REGZ(i->dst(), op1_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BTC_EdGdM(bxInstruction_c* i)
{
  bx_address op1_addr;
  Bit32u op1_32, op2_32, index_32;
  Bit32s displacement32;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op2_32 = BX_READ_32BIT_REG(i->src());
  index_32 = op2_32 & 0x1f;

  displacement32 = ((Bit32s) (op2_32 & 0xffffffe0)) / 32;
  op1_addr = eaddr + 4 * displacement32;

  op1_32 = read_RMW_virtual_dword(i->seg(), op1_addr & i->asize_mask());
  bool temp_CF = (op1_32 >> index_32) & 0x01;
  op1_32 ^= (1 << index_32);  /* toggle bit */
  set_CF(temp_CF);

  write_RMW_linear_dword(op1_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BTC_EdGdR(bxInstruction_c* i)
{
  Bit32u op1_32, op2_32;

  op1_32 = BX_READ_32BIT_REG(i->dst());
  op2_32 = BX_READ_32BIT_REG(i->src());
  op2_32 &= 0x1f;

  bool temp_CF = (op1_32 >> op2_32) & 0x01;
  op1_32 ^= (1 << op2_32);  /* toggle bit */
  set_CF(temp_CF);

  BX_WRITE_32BIT_REGZ(i->dst(), op1_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BT_EdIbM(bxInstruction_c* i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_32 = read_virtual_dword(i->seg(), eaddr);
  Bit8u  op2_8  = i->Ib() & 0x1f;

  set_CF((op1_32 >> op2_8) & 0x01);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BT_EdIbR(bxInstruction_c* i)
{
  Bit32u op1_32 = BX_READ_32BIT_REG(i->dst());
  Bit8u  op2_8  = i->Ib() & 0x1f;

  set_CF((op1_32 >> op2_8) & 0x01);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BTS_EdIbM(bxInstruction_c* i)
{
  Bit8u op2_8 = i->Ib() & 0x1f;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_32 = read_RMW_virtual_dword(i->seg(), eaddr);
  bool temp_CF = (op1_32 >> op2_8) & 0x01;
  op1_32 |= (1 << op2_8);
  write_RMW_linear_dword(op1_32);

  set_CF(temp_CF);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BTS_EdIbR(bxInstruction_c* i)
{
  Bit8u op2_8 = i->Ib() & 0x1f;

  Bit32u op1_32 = BX_READ_32BIT_REG(i->dst());
  bool temp_CF = (op1_32 >> op2_8) & 0x01;
  op1_32 |= (1 << op2_8);
  BX_WRITE_32BIT_REGZ(i->dst(), op1_32);

  set_CF(temp_CF);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BTC_EdIbM(bxInstruction_c* i)
{
  Bit8u op2_8 = i->Ib() & 0x1f;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_32 = read_RMW_virtual_dword(i->seg(), eaddr);
  bool temp_CF = (op1_32 >> op2_8) & 0x01;
  op1_32 ^= (1 << op2_8);  /* toggle bit */
  write_RMW_linear_dword(op1_32);

  set_CF(temp_CF);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BTC_EdIbR(bxInstruction_c* i)
{
  Bit8u op2_8 = i->Ib() & 0x1f;

  Bit32u op1_32 = BX_READ_32BIT_REG(i->dst());
  bool temp_CF = (op1_32 >> op2_8) & 0x01;
  op1_32 ^= (1 << op2_8);  /* toggle bit */
  BX_WRITE_32BIT_REGZ(i->dst(), op1_32);

  set_CF(temp_CF);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BTR_EdIbM(bxInstruction_c* i)
{
  Bit8u op2_8 = i->Ib() & 0x1f;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_32 = read_RMW_virtual_dword(i->seg(), eaddr);
  bool temp_CF = (op1_32 >> op2_8) & 0x01;
  op1_32 &= ~(1 << op2_8);
  write_RMW_linear_dword(op1_32);

  set_CF(temp_CF);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BTR_EdIbR(bxInstruction_c* i)
{
  Bit8u op2_8 = i->Ib() & 0x1f;

  Bit32u op1_32 = BX_READ_32BIT_REG(i->dst());
  bool temp_CF = (op1_32 >> op2_8) & 0x01;
  op1_32 &= ~(1 << op2_8);
  BX_WRITE_32BIT_REGZ(i->dst(), op1_32);

  set_CF(temp_CF);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SHLD_EwGwM(bxInstruction_c* i)
{
  Bit32u temp_32, result_32;
  unsigned count;
  unsigned of, cf;

  /* op1:op2 << count.  result stored in op1 */
  if (i->getIaOpcode() == BX_IA_SHLD_EwGw)
    count = CL;
  else // BX_IA_SHLD_EwGwIb
    count = i->Ib();

  count &= 0x1f; // use only 5 LSB's

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_16 = (Bit32u) read_RMW_virtual_word(i->seg(), eaddr);

  if (count) {
    Bit32u op2_16 = (Bit32u) BX_READ_16BIT_REG(i->src());

    /* count < 32, since only lower 5 bits used */
    temp_32 = (op1_16 << 16) | (op2_16); // double formed by op1:op2
    result_32 = temp_32 << count;

    // hack to act like x86 SHLD when count > 16
    if (count > 16) {
      // for Pentium processor, when count > 16, actually shifting op1:op2:op2 << count,
      // it is the same as shifting op2:op2 by count-16
      // For P6 and later (CPU_LEVEL >= 6), when count > 16, actually shifting op1:op2:op1 << count,
      // which is the same as shifting op2:op1 by count-16
      // The behavior is undefined so both ways are correct, we prefer P6 way of implementation
      result_32 |= (op1_16 << (count - 16));
    }

    Bit16u result_16 = (Bit16u)(result_32 >> 16);

    write_RMW_linear_word(result_16);

    SET_FLAGS_OSZAPC_LOGIC_16(result_16);

    cf = (temp_32 >> (32 - count)) & 0x1;
    of = cf ^ (result_16 >> 15); // of = cf ^ result15
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SHLD_EwGwR(bxInstruction_c* i)
{
  Bit32u temp_32, result_32;
  unsigned count;
  unsigned of, cf;

  /* op1:op2 << count.  result stored in op1 */
  if (i->getIaOpcode() == BX_IA_SHLD_EwGw)
    count = CL;
  else // BX_IA_SHLD_EwGwIb
    count = i->Ib();

  count &= 0x1f; // use only 5 LSB's

  if (count) {
    Bit32u op1_16 = (Bit32u) BX_READ_16BIT_REG(i->dst());
    Bit32u op2_16 = (Bit32u) BX_READ_16BIT_REG(i->src());

    /* count < 32, since only lower 5 bits used */
    temp_32 = (op1_16 << 16) | (op2_16); // double formed by op1:op2
    result_32 = temp_32 << count;

    // hack to act like x86 SHLD when count > 16
    if (count > 16) {
      // for Pentium processor, when count > 16, actually shifting op1:op2:op2 << count,
      // it is the same as shifting op2:op2 by count-16
      // For P6 and later (CPU_LEVEL >= 6), when count > 16, actually shifting op1:op2:op1 << count,
      // which is the same as shifting op2:op1 by count-16
      // The behavior is undefined so both ways are correct, we prefer P6 way of implementation
      result_32 |= (op1_16 << (count - 16));
    }

    Bit16u result_16 = (Bit16u)(result_32 >> 16);

    BX_WRITE_16BIT_REG(i->dst(), result_16);

    SET_FLAGS_OSZAPC_LOGIC_16(result_16);

    cf = (temp_32 >> (32 - count)) & 0x1;
    of = cf ^ (result_16 >> 15); // of = cf ^ result15
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SHRD_EwGwM(bxInstruction_c* i)
{
  Bit32u temp_32, result_32;
  unsigned count;
  unsigned cf, of;

  if (i->getIaOpcode() == BX_IA_SHRD_EwGw)
    count = CL;
  else // BX_IA_SHRD_EwGwIb
    count = i->Ib();

  count &= 0x1f; /* use only 5 LSB's */

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_16 = (Bit32u) read_RMW_virtual_word(i->seg(), eaddr);

  if (count) {
    Bit32u op2_16 = (Bit32u) BX_READ_16BIT_REG(i->src());

    /* count < 32, since only lower 5 bits used */
    temp_32 = (op2_16 << 16) | op1_16; // double formed by op2:op1
    result_32 = temp_32 >> count;

    // hack to act like x86 SHRD when count > 16
    if (count > 16) {
      // for Pentium processor, when count > 16, actually shifting op2:op2:op1 >> count,
      // it is the same as shifting op2:op2 by count-16
      // For P6 and later (CPU_LEVEL >= 6), when count > 16, actually shifting op1:op2:op1 >> count,
      // which is the same as shifting op1:op2 by count-16
      // The behavior is undefined so both ways are correct, we prefer P6 way of implementation
      result_32 |= (op1_16 << (32 - count));
    }

    Bit16u result_16 = (Bit16u) result_32;

    write_RMW_linear_word(result_16);

    SET_FLAGS_OSZAPC_LOGIC_16(result_16);

    cf = (op1_16 >> (count - 1)) & 0x1;
    of = ((Bit16u)((result_16 << 1) ^ result_16) >> 15) & 0x1; // of = result14 ^ result15
    if (count > 16) cf = (op2_16 >> (count - 17)) & 0x1; // undefined flags behavior matching real HW
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SHRD_EwGwR(bxInstruction_c* i)
{
  Bit32u temp_32, result_32;
  unsigned count;
  unsigned cf, of;

  if (i->getIaOpcode() == BX_IA_SHRD_EwGw)
    count = CL;
  else // BX_IA_SHRD_EwGwIb
    count = i->Ib();

  count &= 0x1f; /* use only 5 LSB's */

  if (count) {
    Bit32u op1_16 = (Bit32u) BX_READ_16BIT_REG(i->dst());
    Bit32u op2_16 = (Bit32u) BX_READ_16BIT_REG(i->src());

    /* count < 32, since only lower 5 bits used */
    temp_32 = (op2_16 << 16) | op1_16; // double formed by op2:op1
    result_32 = temp_32 >> count;

    // hack to act like x86 SHRD when count > 16
    if (count > 16) {
      // for Pentium processor, when count > 16, actually shifting op2:op2:op1 >> count,
      // it is the same as shifting op2:op2 by count-16
      // For P6 and later (CPU_LEVEL >= 6), when count > 16, actually shifting op1:op2:op1 >> count,
      // which is the same as shifting op1:op2 by count-16
      // The behavior is undefined so both ways are correct, we prefer P6 way of implementation
      result_32 |= (op1_16 << (32 - count));
    }

    Bit16u result_16 = (Bit16u) result_32;

    BX_WRITE_16BIT_REG(i->dst(), result_16);

    SET_FLAGS_OSZAPC_LOGIC_16(result_16);

    cf = (op1_16 >> (count - 1)) & 0x1;
    of = ((Bit16u)((result_16 << 1) ^ result_16) >> 15) & 0x1; // of = result14 ^ result15
    if (count > 16) cf = (op2_16 >> (count - 17)) & 0x1; // undefined flags behavior matching real HW
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SHLD_EdGdM(bxInstruction_c* i)
{
  unsigned count;
  unsigned of, cf;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_32 = read_RMW_virtual_dword(i->seg(), eaddr);

  if (i->getIaOpcode() == BX_IA_SHLD_EdGd)
    count = CL;
  else // BX_IA_SHLD_EdGdIb
    count = i->Ib();

  count &= 0x1f; // use only 5 LSB's

  if (count) {
    Bit32u op2_32 = BX_READ_32BIT_REG(i->src());

    Bit32u result_32 = (op1_32 << count) | (op2_32 >> (32 - count));

    write_RMW_linear_dword(result_32);

    SET_FLAGS_OSZAPC_LOGIC_32(result_32);

    cf = (op1_32 >> (32 - count)) & 0x1;
    of = cf ^ (result_32 >> 31); // of = cf ^ result31
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SHLD_EdGdR(bxInstruction_c* i)
{
  Bit32u op1_32, op2_32, result_32;
  unsigned count;
  unsigned of, cf;

  if (i->getIaOpcode() == BX_IA_SHLD_EdGd)
    count = CL;
  else // BX_IA_SHLD_EdGdIb
    count = i->Ib();

  count &= 0x1f; // use only 5 LSB's

  if (!count) {
    BX_CLEAR_64BIT_HIGH(i->dst()); // always clear upper part of the register
  }
  else {
    op1_32 = BX_READ_32BIT_REG(i->dst());
    op2_32 = BX_READ_32BIT_REG(i->src());

    result_32 = (op1_32 << count) | (op2_32 >> (32 - count));

    BX_WRITE_32BIT_REGZ(i->dst(), result_32);

    SET_FLAGS_OSZAPC_LOGIC_32(result_32);

    cf = (op1_32 >> (32 - count)) & 0x1;
    of = cf ^ (result_32 >> 31); // of = cf ^ result31
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SHRD_EdGdM(bxInstruction_c* i)
{
  unsigned count;
  unsigned cf, of;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_32 = read_RMW_virtual_dword(i->seg(), eaddr);

  if (i->getIaOpcode() == BX_IA_SHRD_EdGd)
    count = CL;
  else // BX_IA_SHRD_EdGdIb
    count = i->Ib();

  count &= 0x1f; // use only 5 LSB's

  if (count) {
    Bit32u op2_32 = BX_READ_32BIT_REG(i->src());

    Bit32u result_32 = (op2_32 << (32 - count)) | (op1_32 >> count);

    write_RMW_linear_dword(result_32);

    SET_FLAGS_OSZAPC_LOGIC_32(result_32);

    cf = (op1_32 >> (count - 1)) & 0x1;
    of = ((result_32 << 1) ^ result_32) >> 31; // of = result30 ^ result31
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BSWAP_RX(bxInstruction_c* i)
{
  BX_WRITE_16BIT_REG(i->dst(), 0);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BSWAP_ERX(bxInstruction_c* i)
{
  Bit32u val32 = BX_READ_32BIT_REG(i->dst());

  BX_WRITE_32BIT_REGZ(i->dst(), bx_bswap32(val32));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::XADD_EbGbM(bxInstruction_c* i)
{
  /* XADD dst(r/m8), src(r8)
   * temp <-- src + dst         | sum = op2 + op1
   * src  <-- dst               | op2 = op1
   * dst  <-- tmp               | op1 = sum
   */

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1 = read_RMW_virtual_byte(i->seg(), eaddr);
  Bit32u op2 = BX_READ_8BIT_REGx(i->src(), i->extend8bitL());
  Bit32u sum = op1 + op2;

  write_RMW_linear_byte(sum);

  /* and write destination into source */
  BX_WRITE_8BIT_REGx(i->src(), i->extend8bitL(), op1);

  SET_FLAGS_OSZAPC_ADD_8(op1, op2, sum);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::XADD_EbGbR(bxInstruction_c* i)
{
  /* XADD dst(r/m8), src(r8)
   * temp <-- src + dst         | sum = op2 + op1
   * src  <-- dst               | op2 = op1
   * dst  <-- tmp               | op1 = sum
   */

  Bit32u op1 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());
  Bit32u op2 = BX_READ_8BIT_REGx(i->src(), i->extend8bitL());
  Bit32u sum = op1 + op2;

  // and write destination into source
  // Note: if both op1 & op2 are registers, the last one written
  //       should be the sum, as op1 & op2 may be the same register.
  //       For example:  XADD AL, AL
  BX_WRITE_8BIT_REGx(i->src(), i->extend8bitL(), op1);
  BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), sum);

  SET_FLAGS_OSZAPC_ADD_8(op1, op2, sum);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMPXCHG_EbGbM(bxInstruction_c* i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_8 = read_RMW_virtual_byte(i->seg(), eaddr);
  Bit32u diff_8 = AL - op1_8;

  SET_FLAGS_OSZAPC_SUB_8(AL, op1_8, diff_8);

  if (diff_8 == 0) {  // if accumulator == dest
    // dest <-- src
    Bit32u op2_8 = BX_READ_8BIT_REGx(i->src(), i->extend8bitL());
    write_RMW_linear_byte(op2_8);
  }
  else {
    // accumulator <-- dest
    write_RMW_linear_byte(op1_8);
    AL = op1_8;
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMPXCHG_EbGbR(bxInstruction_c* i)
{
  Bit32u op1_8 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());
  Bit32u diff_8 = AL - op1_8;

  SET_FLAGS_OSZAPC_SUB_8(AL, op1_8, diff_8);

  if (diff_8 == 0) {  // if accumulator == dest
    // dest <-- src
    Bit32u op2_8 = BX_READ_8BIT_REGx(i->src(), i->extend8bitL());
    BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), op2_8);
  }
  else {
    // accumulator <-- dest
    AL = op1_8;
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::XADD_EwGwM(bxInstruction_c* i)
{
  /* XADD dst(r/m), src(r)
   * temp <-- src + dst         | sum = op2 + op1
   * src  <-- dst               | op2 = op1
   * dst  <-- tmp               | op1 = sum
   */

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_16 = read_RMW_virtual_word(i->seg(), eaddr);
  Bit32u op2_16 = BX_READ_16BIT_REG(i->src());
  Bit32u sum_16 = op1_16 + op2_16;

  write_RMW_linear_word(sum_16);

  /* and write destination into source */
  BX_WRITE_16BIT_REG(i->src(), op1_16);

  SET_FLAGS_OSZAPC_ADD_16(op1_16, op2_16, sum_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::XADD_EwGwR(bxInstruction_c* i)
{
  /* XADD dst(r/m), src(r)
   * temp <-- src + dst         | sum = op2 + op1
   * src  <-- dst               | op2 = op1
   * dst  <-- tmp               | op1 = sum
   */

  Bit32u op1_16 = BX_READ_16BIT_REG(i->dst());
  Bit32u op2_16 = BX_READ_16BIT_REG(i->src());
  Bit32u sum_16 = op1_16 + op2_16;

  // and write destination into source
  // Note: if both op1 & op2 are registers, the last one written
  //       should be the sum, as op1 & op2 may be the same register.
  //       For example:  XADD AL, AL
  BX_WRITE_16BIT_REG(i->src(), op1_16);
  BX_WRITE_16BIT_REG(i->dst(), sum_16);

  SET_FLAGS_OSZAPC_ADD_16(op1_16, op2_16, sum_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMPXCHG_EwGwM(bxInstruction_c* i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit16u op1_16 = read_RMW_virtual_word(i->seg(), eaddr);
  Bit16u diff_16 = AX - op1_16;

  SET_FLAGS_OSZAPC_SUB_16(AX, op1_16, diff_16);

  if (diff_16 == 0) {  // if accumulator == dest
    // dest <-- src
    write_RMW_linear_word(BX_READ_16BIT_REG(i->src()));
  }
  else {
    // accumulator <-- dest
    write_RMW_linear_word(op1_16);
    AX = op1_16;
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMPXCHG_EwGwR(bxInstruction_c* i)
{
  Bit16u op1_16 = BX_READ_16BIT_REG(i->dst());
  Bit16u diff_16 = AX - op1_16;

  SET_FLAGS_OSZAPC_SUB_16(AX, op1_16, diff_16);

  if (diff_16 == 0) {  // if accumulator == dest
    // dest <-- src
    BX_WRITE_16BIT_REG(i->dst(), BX_READ_16BIT_REG(i->src()));
  }
  else {
    // accumulator <-- dest
    AX = op1_16;
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::XADD_EdGdM(bxInstruction_c* i)
{
  Bit32u op1_32, op2_32, sum_32;

  /* XADD dst(r/m), src(r)
   * temp <-- src + dst         | sum = op2 + op1
   * src  <-- dst               | op2 = op1
   * dst  <-- tmp               | op1 = sum
   */

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op1_32 = read_RMW_virtual_dword(i->seg(), eaddr);
  op2_32 = BX_READ_32BIT_REG(i->src());
  sum_32 = op1_32 + op2_32;
  write_RMW_linear_dword(sum_32);

  /* and write destination into source */
  BX_WRITE_32BIT_REGZ(i->src(), op1_32);

  SET_FLAGS_OSZAPC_ADD_32(op1_32, op2_32, sum_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::XADD_EdGdR(bxInstruction_c* i)
{
  Bit32u op1_32, op2_32, sum_32;

  /* XADD dst(r/m), src(r)
   * temp <-- src + dst         | sum = op2 + op1
   * src  <-- dst               | op2 = op1
   * dst  <-- tmp               | op1 = sum
   */

  op1_32 = BX_READ_32BIT_REG(i->dst());
  op2_32 = BX_READ_32BIT_REG(i->src());
  sum_32 = op1_32 + op2_32;

  // and write destination into source
  // Note: if both op1 & op2 are registers, the last one written
  //       should be the sum, as op1 & op2 may be the same register.
  //       For example:  XADD AL, AL
  BX_WRITE_32BIT_REGZ(i->src(), op1_32);
  BX_WRITE_32BIT_REGZ(i->dst(), sum_32);

  SET_FLAGS_OSZAPC_ADD_32(op1_32, op2_32, sum_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMPXCHG_EdGdM(bxInstruction_c* i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_32 = read_RMW_virtual_dword(i->seg(), eaddr);
  Bit32u diff_32 = EAX - op1_32;
  SET_FLAGS_OSZAPC_SUB_32(EAX, op1_32, diff_32);

  if (diff_32 == 0) {  // if accumulator == dest
    // dest <-- src
    write_RMW_linear_dword(BX_READ_32BIT_REG(i->src()));
  }
  else {
    // accumulator <-- dest
    write_RMW_linear_dword(op1_32);
    RAX = op1_32;
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMPXCHG_EdGdR(bxInstruction_c* i)
{
  Bit32u op1_32 = BX_READ_32BIT_REG(i->dst());
  Bit32u diff_32 = EAX - op1_32;
  SET_FLAGS_OSZAPC_SUB_32(EAX, op1_32, diff_32);

  if (diff_32 == 0) {  // if accumulator == dest
    // dest <-- src
    BX_WRITE_32BIT_REGZ(i->dst(), BX_READ_32BIT_REG(i->src()));
  }
  else {
    // accumulator <-- dest
    RAX = op1_32;
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMPXCHG8B(bxInstruction_c* i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  // check write permission for following write
  Bit64u op1_64 = read_RMW_virtual_qword(i->seg(), eaddr);
  Bit64u op2_64 = GET64_FROM_HI32_LO32(EDX, EAX);

  if (op1_64 == op2_64) {  // if accumulator == dest
    // dest <-- src (ECX:EBX)
    op2_64 = GET64_FROM_HI32_LO32(ECX, EBX);
    write_RMW_linear_qword(op2_64);
    assert_ZF();
  }
  else {
    // accumulator <-- dest
    write_RMW_linear_qword(op1_64);
    RAX = GET32L(op1_64);
    RDX = GET32H(op1_64);
    clear_ZF();
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVO_GwEwR(bxInstruction_c* i)
{
  if (get_OF())
    BX_WRITE_16BIT_REG(i->dst(), BX_READ_16BIT_REG(i->src()));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVNO_GwEwR(bxInstruction_c* i)
{
  if (!get_OF())
    BX_WRITE_16BIT_REG(i->dst(), BX_READ_16BIT_REG(i->src()));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVB_GwEwR(bxInstruction_c* i)
{
  if (get_CF())
    BX_WRITE_16BIT_REG(i->dst(), BX_READ_16BIT_REG(i->src()));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVNB_GwEwR(bxInstruction_c* i)
{
  if (!get_CF())
    BX_WRITE_16BIT_REG(i->dst(), BX_READ_16BIT_REG(i->src()));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVZ_GwEwR(bxInstruction_c* i)
{
  if (get_ZF())
    BX_WRITE_16BIT_REG(i->dst(), BX_READ_16BIT_REG(i->src()));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVNZ_GwEwR(bxInstruction_c* i)
{
  if (!get_ZF())
    BX_WRITE_16BIT_REG(i->dst(), BX_READ_16BIT_REG(i->src()));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVBE_GwEwR(bxInstruction_c* i)
{
  if (get_CF() || get_ZF())
    BX_WRITE_16BIT_REG(i->dst(), BX_READ_16BIT_REG(i->src()));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVNBE_GwEwR(bxInstruction_c* i)
{
  if (! (get_CF() || get_ZF()))
    BX_WRITE_16BIT_REG(i->dst(), BX_READ_16BIT_REG(i->src()));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVS_GwEwR(bxInstruction_c* i)
{
  if (get_SF())
    BX_WRITE_16BIT_REG(i->dst(), BX_READ_16BIT_REG(i->src()));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVNS_GwEwR(bxInstruction_c* i)
{
  if (!get_SF())
    BX_WRITE_16BIT_REG(i->dst(), BX_READ_16BIT_REG(i->src()));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVP_GwEwR(bxInstruction_c* i)
{
  if (get_PF())
    BX_WRITE_16BIT_REG(i->dst(), BX_READ_16BIT_REG(i->src()));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVNP_GwEwR(bxInstruction_c* i)
{
  if (!get_PF())
    BX_WRITE_16BIT_REG(i->dst(), BX_READ_16BIT_REG(i->src()));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVL_GwEwR(bxInstruction_c* i)
{
  if (getB_SF() != getB_OF())
    BX_WRITE_16BIT_REG(i->dst(), BX_READ_16BIT_REG(i->src()));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVNL_GwEwR(bxInstruction_c* i)
{
  if (getB_SF() == getB_OF())
    BX_WRITE_16BIT_REG(i->dst(), BX_READ_16BIT_REG(i->src()));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVLE_GwEwR(bxInstruction_c* i)
{
  if (get_ZF() || (getB_SF() != getB_OF()))
    BX_WRITE_16BIT_REG(i->dst(), BX_READ_16BIT_REG(i->src()));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVNLE_GwEwR(bxInstruction_c* i)
{
  if (! get_ZF() && (getB_SF() == getB_OF()))
    BX_WRITE_16BIT_REG(i->dst(), BX_READ_16BIT_REG(i->src()));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVO_GdEdR(bxInstruction_c* i)
{
  if (get_OF())
    BX_WRITE_32BIT_REGZ(i->dst(), BX_READ_32BIT_REG(i->src()));

  BX_CLEAR_64BIT_HIGH(i->dst()); // always clear upper part of the register

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVNO_GdEdR(bxInstruction_c* i)
{
  if (!get_OF())
    BX_WRITE_32BIT_REGZ(i->dst(), BX_READ_32BIT_REG(i->src()));

  BX_CLEAR_64BIT_HIGH(i->dst()); // always clear upper part of the register

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVB_GdEdR(bxInstruction_c* i)
{
  if (get_CF())
    BX_WRITE_32BIT_REGZ(i->dst(), BX_READ_32BIT_REG(i->src()));

  BX_CLEAR_64BIT_HIGH(i->dst()); // always clear upper part of the register

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVNB_GdEdR(bxInstruction_c* i)
{
  if (!get_CF())
    BX_WRITE_32BIT_REGZ(i->dst(), BX_READ_32BIT_REG(i->src()));

  BX_CLEAR_64BIT_HIGH(i->dst()); // always clear upper part of the register

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVZ_GdEdR(bxInstruction_c* i)
{
  if (get_ZF())
    BX_WRITE_32BIT_REGZ(i->dst(), BX_READ_32BIT_REG(i->src()));

  BX_CLEAR_64BIT_HIGH(i->dst()); // always clear upper part of the register

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVNZ_GdEdR(bxInstruction_c* i)
{
  if (!get_ZF())
    BX_WRITE_32BIT_REGZ(i->dst(), BX_READ_32BIT_REG(i->src()));

  BX_CLEAR_64BIT_HIGH(i->dst()); // always clear upper part of the register

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVBE_GdEdR(bxInstruction_c* i)
{
  if (get_CF() || get_ZF())
    BX_WRITE_32BIT_REGZ(i->dst(), BX_READ_32BIT_REG(i->src()));

  BX_CLEAR_64BIT_HIGH(i->dst()); // always clear upper part of the register

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVNBE_GdEdR(bxInstruction_c* i)
{
  if (! (get_CF() || get_ZF()))
    BX_WRITE_32BIT_REGZ(i->dst(), BX_READ_32BIT_REG(i->src()));

  BX_CLEAR_64BIT_HIGH(i->dst()); // always clear upper part of the register

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVS_GdEdR(bxInstruction_c* i)
{
  if (get_SF())
    BX_WRITE_32BIT_REGZ(i->dst(), BX_READ_32BIT_REG(i->src()));

  BX_CLEAR_64BIT_HIGH(i->dst()); // always clear upper part of the register

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVNS_GdEdR(bxInstruction_c* i)
{
  if (!get_SF())
    BX_WRITE_32BIT_REGZ(i->dst(), BX_READ_32BIT_REG(i->src()));

  BX_CLEAR_64BIT_HIGH(i->dst()); // always clear upper part of the register

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVP_GdEdR(bxInstruction_c* i)
{
  if (get_PF())
    BX_WRITE_32BIT_REGZ(i->dst(), BX_READ_32BIT_REG(i->src()));

  BX_CLEAR_64BIT_HIGH(i->dst()); // always clear upper part of the register

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVNP_GdEdR(bxInstruction_c* i)
{
  if (!get_PF())
    BX_WRITE_32BIT_REGZ(i->dst(), BX_READ_32BIT_REG(i->src()));

  BX_CLEAR_64BIT_HIGH(i->dst()); // always clear upper part of the register

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVL_GdEdR(bxInstruction_c* i)
{
  if (getB_SF() != getB_OF())
    BX_WRITE_32BIT_REGZ(i->dst(), BX_READ_32BIT_REG(i->src()));

  BX_CLEAR_64BIT_HIGH(i->dst()); // always clear upper part of the register

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVNL_GdEdR(bxInstruction_c* i)
{
  if (getB_SF() == getB_OF())
    BX_WRITE_32BIT_REGZ(i->dst(), BX_READ_32BIT_REG(i->src()));

  BX_CLEAR_64BIT_HIGH(i->dst()); // always clear upper part of the register

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVLE_GdEdR(bxInstruction_c* i)
{
  if (get_ZF() || (getB_SF() != getB_OF()))
    BX_WRITE_32BIT_REGZ(i->dst(), BX_READ_32BIT_REG(i->src()));

  BX_CLEAR_64BIT_HIGH(i->dst()); // always clear upper part of the register

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVNLE_GdEdR(bxInstruction_c* i)
{
  if (! get_ZF() && (getB_SF() == getB_OF()))
    BX_WRITE_32BIT_REGZ(i->dst(), BX_READ_32BIT_REG(i->src()));

  BX_CLEAR_64BIT_HIGH(i->dst()); // always clear upper part of the register

  BX_NEXT_INSTR(i);
}

#if BX_SUPPORT_X86_64

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BSF_GqEqR(bxInstruction_c* i)
{
  Bit64u op2_64 = BX_READ_64BIT_REG(i->src());

  if (op2_64 == 0) {
    assert_ZF(); /* op1_64 undefined */
  }
  else {
    Bit64u op1_64 = 0;
    while ((op2_64 & BX_CONST64(0x1)) == 0) {
      op1_64++;
      op2_64 >>= 1;
    }
    SET_FLAGS_OSZAPC_LOGIC_64(op1_64);
    clear_ZF();

    BX_WRITE_64BIT_REG(i->dst(), op1_64);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BSR_GqEqR(bxInstruction_c* i)
{
  Bit64u op2_64 = BX_READ_64BIT_REG(i->src());

  if (op2_64 == 0) {
    assert_ZF(); /* op1_64 undefined */
  }
  else {
    Bit64u op1_64 = 63;
    while ((op2_64 & BX_CONST64(0x8000000000000000)) == 0) {
      op1_64--;
      op2_64 <<= 1;
    }

    SET_FLAGS_OSZAPC_LOGIC_64(op1_64);
    clear_ZF();

    BX_WRITE_64BIT_REG(i->dst(), op1_64);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BT_EqGqM(bxInstruction_c* i)
{
  bx_address op1_addr;
  Bit64u op1_64, op2_64;
  Bit64s displacement64;
  Bit64u index;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  op2_64 = BX_READ_64BIT_REG(i->src());
  index = op2_64 & 0x3f;
  displacement64 = ((Bit64s) (op2_64 & BX_CONST64(0xffffffffffffffc0))) / 64;
  op1_addr = eaddr + 8 * displacement64;
  if (! i->as64L())
    op1_addr = (Bit32u) op1_addr;

  /* pointer, segment address pair */
  op1_64 = read_linear_qword(i->seg(), get_laddr64(i->seg(), op1_addr));

  set_CF((op1_64 >> index) & 0x01);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BT_EqGqR(bxInstruction_c* i)
{
  Bit64u op1_64, op2_64;

  op1_64 = BX_READ_64BIT_REG(i->dst());
  op2_64 = BX_READ_64BIT_REG(i->src());
  op2_64 &= 0x3f;
  set_CF((op1_64 >> op2_64) & 0x01);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BTS_EqGqM(bxInstruction_c* i)
{
  bx_address op1_addr;
  Bit64u op1_64, op2_64, index;
  Bit64s displacement64;
  bool bit_i;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  op2_64 = BX_READ_64BIT_REG(i->src());
  index = op2_64 & 0x3f;
  displacement64 = ((Bit64s) (op2_64 & BX_CONST64(0xffffffffffffffc0))) / 64;
  op1_addr = eaddr + 8 * displacement64;
  if (! i->as64L())
    op1_addr = (Bit32u) op1_addr;

  /* pointer, segment address pair */
  op1_64 = read_RMW_linear_qword(i->seg(), get_laddr64(i->seg(), op1_addr));
  bit_i = (op1_64 >> index) & 0x01;
  op1_64 |= (((Bit64u) 1) << index);
  write_RMW_linear_qword(op1_64);

  set_CF(bit_i);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BTS_EqGqR(bxInstruction_c* i)
{
  Bit64u op1_64, op2_64;

  op1_64 = BX_READ_64BIT_REG(i->dst());
  op2_64 = BX_READ_64BIT_REG(i->src());
  op2_64 &= 0x3f;
  set_CF((op1_64 >> op2_64) & 0x01);
  op1_64 |= (((Bit64u) 1) << op2_64);

  /* now write result back to the destination */
  BX_WRITE_64BIT_REG(i->dst(), op1_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BTR_EqGqM(bxInstruction_c* i)
{
  bx_address op1_addr;
  Bit64u op1_64, op2_64, index;
  Bit64s displacement64;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  op2_64 = BX_READ_64BIT_REG(i->src());
  index = op2_64 & 0x3f;
  displacement64 = ((Bit64s) (op2_64 & BX_CONST64(0xffffffffffffffc0))) / 64;
  op1_addr = eaddr + 8 * displacement64;
  if (! i->as64L())
    op1_addr = (Bit32u) op1_addr;

  /* pointer, segment address pair */
  op1_64 = read_RMW_linear_qword(i->seg(), get_laddr64(i->seg(), op1_addr));
  bool temp_cf = (op1_64 >> index) & 0x01;
  op1_64 &= ~(((Bit64u) 1) << index);
  /* now write back to destination */
  write_RMW_linear_qword(op1_64);

  set_CF(temp_cf);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BTR_EqGqR(bxInstruction_c* i)
{
  Bit64u op1_64, op2_64;

  op1_64 = BX_READ_64BIT_REG(i->dst());
  op2_64 = BX_READ_64BIT_REG(i->src());
  op2_64 &= 0x3f;
  set_CF((op1_64 >> op2_64) & 0x01);
  op1_64 &= ~(((Bit64u) 1) << op2_64);

  /* now write result back to the destination */
  BX_WRITE_64BIT_REG(i->dst(), op1_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BTC_EqGqM(bxInstruction_c* i)
{
  bx_address op1_addr;
  Bit64u op1_64, op2_64;
  Bit64s displacement64;
  Bit64u index;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  op2_64 = BX_READ_64BIT_REG(i->src());
  index = op2_64 & 0x3f;
  displacement64 = ((Bit64s) (op2_64 & BX_CONST64(0xffffffffffffffc0))) / 64;
  op1_addr = eaddr + 8 * displacement64;
  if (! i->as64L())
    op1_addr = (Bit32u) op1_addr;

  op1_64 = read_RMW_linear_qword(i->seg(), get_laddr64(i->seg(), op1_addr));
  bool temp_CF = (op1_64 >> index) & 0x01;
  op1_64 ^= (((Bit64u) 1) << index);  /* toggle bit */
  set_CF(temp_CF);

  write_RMW_linear_qword(op1_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BTC_EqGqR(bxInstruction_c* i)
{
  Bit64u op1_64, op2_64;

  op1_64 = BX_READ_64BIT_REG(i->dst());
  op2_64 = BX_READ_64BIT_REG(i->src());
  op2_64 &= 0x3f;

  bool temp_CF = (op1_64 >> op2_64) & 0x01;
  op1_64 ^= (((Bit64u) 1) << op2_64);  /* toggle bit */
  set_CF(temp_CF);

  BX_WRITE_64BIT_REG(i->dst(), op1_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BT_EqIbM(bxInstruction_c* i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  Bit64u op1_64 = read_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));
  Bit8u  op2_8  = i->Ib() & 0x3f;

  set_CF((op1_64 >> op2_8) & 0x01);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BT_EqIbR(bxInstruction_c* i)
{
  Bit64u op1_64 = BX_READ_64BIT_REG(i->dst());
  Bit8u  op2_8  = i->Ib() & 0x3f;

  set_CF((op1_64 >> op2_8) & 0x01);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BTS_EqIbM(bxInstruction_c* i)
{
  Bit8u op2_8 = i->Ib() & 0x3f;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  Bit64u op1_64 = read_RMW_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));
  bool temp_CF = (op1_64 >> op2_8) & 0x01;
  op1_64 |= (((Bit64u) 1) << op2_8);
  write_RMW_linear_qword(op1_64);

  set_CF(temp_CF);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BTS_EqIbR(bxInstruction_c* i)
{
  Bit8u op2_8 = i->Ib() & 0x3f;

  Bit64u op1_64 = BX_READ_64BIT_REG(i->dst());
  bool temp_CF = (op1_64 >> op2_8) & 0x01;
  op1_64 |= (((Bit64u) 1) << op2_8);
  BX_WRITE_64BIT_REG(i->dst(), op1_64);

  set_CF(temp_CF);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BTC_EqIbM(bxInstruction_c* i)
{
  Bit8u op2_8 = i->Ib() & 0x3f;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  Bit64u op1_64 = read_RMW_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));
  bool temp_CF = (op1_64 >> op2_8) & 0x01;
  op1_64 ^= (((Bit64u) 1) << op2_8);  /* toggle bit */
  write_RMW_linear_qword(op1_64);

  set_CF(temp_CF);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BTC_EqIbR(bxInstruction_c* i)
{
  Bit8u op2_8 = i->Ib() & 0x3f;

  Bit64u op1_64 = BX_READ_64BIT_REG(i->dst());
  bool temp_CF = (op1_64 >> op2_8) & 0x01;
  op1_64 ^= (((Bit64u) 1) << op2_8);  /* toggle bit */
  BX_WRITE_64BIT_REG(i->dst(), op1_64);

  set_CF(temp_CF);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BTR_EqIbM(bxInstruction_c* i)
{
  Bit8u op2_8 = i->Ib() & 0x3f;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  Bit64u op1_64 = read_RMW_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));
  bool temp_CF = (op1_64 >> op2_8) & 0x01;
  op1_64 &= ~(((Bit64u) 1) << op2_8);
  write_RMW_linear_qword(op1_64);

  set_CF(temp_CF);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BTR_EqIbR(bxInstruction_c* i)
{
  Bit8u op2_8 = i->Ib() & 0x3f;

  Bit64u op1_64 = BX_READ_64BIT_REG(i->dst());
  bool temp_CF = (op1_64 >> op2_8) & 0x01;
  op1_64 &= ~(((Bit64u) 1) << op2_8);
  BX_WRITE_64BIT_REG(i->dst(), op1_64);

  set_CF(temp_CF);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SHLD_EqGqM(bxInstruction_c* i)
{
  Bit64u op1_64, op2_64, result_64;
  unsigned count;
  unsigned cf, of;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  /* pointer, segment address pair */
  op1_64 = read_RMW_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));

  if (i->getIaOpcode() == BX_IA_SHLD_EqGq)
    count = CL;
  else // BX_IA_SHLD_EqGqIb
    count = i->Ib();

  count &= 0x3f; // use only 6 LSB's

  if (count) {
    op2_64 = BX_READ_64BIT_REG(i->src());

    result_64 = (op1_64 << count) | (op2_64 >> (64 - count));

    write_RMW_linear_qword(result_64);

    SET_FLAGS_OSZAPC_LOGIC_64(result_64);

    cf = (op1_64 >> (64 - count)) & 0x1;
    of = cf ^ (result_64 >> 63); // of = cf ^ result63
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SHLD_EqGqR(bxInstruction_c* i)
{
  Bit64u op1_64, op2_64, result_64;
  unsigned count;
  unsigned cf, of;

  if (i->getIaOpcode() == BX_IA_SHLD_EqGq)
    count = CL;
  else // BX_IA_SHLD_EqGqIb
    count = i->Ib();

  count &= 0x3f; // use only 6 LSB's

  if (count) {
    op1_64 = BX_READ_64BIT_REG(i->dst());
    op2_64 = BX_READ_64BIT_REG(i->src());

    result_64 = (op1_64 << count) | (op2_64 >> (64 - count));

    BX_WRITE_64BIT_REG(i->dst(), result_64);

    SET_FLAGS_OSZAPC_LOGIC_64(result_64);

    cf = (op1_64 >> (64 - count)) & 0x1;
    of = cf ^ (result_64 >> 63); // of = cf ^ result63
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SHRD_EqGqM(bxInstruction_c* i)
{
  Bit64u op1_64, op2_64, result_64;
  unsigned count;
  unsigned cf, of;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  /* pointer, segment address pair */
  op1_64 = read_RMW_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));

  if (i->getIaOpcode() == BX_IA_SHRD_EqGq)
    count = CL;
  else // BX_IA_SHRD_EqGqIb
    count = i->Ib();

  count &= 0x3f; // use only 6 LSB's

  if (count) {
    op2_64 = BX_READ_64BIT_REG(i->src());

    result_64 = (op2_64 << (64 - count)) | (op1_64 >> count);

    write_RMW_linear_qword(result_64);

    SET_FLAGS_OSZAPC_LOGIC_64(result_64);

    cf = (op1_64 >> (count - 1)) & 0x1;
    of = ((result_64 << 1) ^ result_64) >> 63; // of = result62 ^ result63
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SHRD_EqGqR(bxInstruction_c* i)
{
  Bit64u op1_64, op2_64, result_64;
  unsigned count;
  unsigned cf, of;

  if (i->getIaOpcode() == BX_IA_SHRD_EqGq)
    count = CL;
  else // BX_IA_SHRD_EqGqIb
    count = i->Ib();

  count &= 0x3f; // use only 6 LSB's

  if (count) {
    op1_64 = BX_READ_64BIT_REG(i->dst());
    op2_64 = BX_READ_64BIT_REG(i->src());

    result_64 = (op2_64 << (64 - count)) | (op1_64 >> count);

    BX_WRITE_64BIT_REG(i->dst(), result_64);

    SET_FLAGS_OSZAPC_LOGIC_64(result_64);

    cf = (op1_64 >> (count - 1)) & 0x1;
    of = ((result_64 << 1) ^ result_64) >> 63; // of = result62 ^ result63
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BSWAP_RRX(bxInstruction_c* i)
{
  Bit64u val64 = BX_READ_64BIT_REG(i->dst());

  BX_WRITE_64BIT_REG(i->dst(), bx_bswap64(val64));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::XADD_EqGqM(bxInstruction_c* i)
{
  Bit64u op1_64, op2_64, sum_64;

  /* XADD dst(r/m), src(r)
   * temp <-- src + dst         | sum = op2 + op1
   * src  <-- dst               | op2 = op1
   * dst  <-- tmp               | op1 = sum
   */

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  /* pointer, segment address pair */
  op1_64 = read_RMW_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));
  op2_64 = BX_READ_64BIT_REG(i->src());
  sum_64 = op1_64 + op2_64;
  write_RMW_linear_qword(sum_64);

  /* and write destination into source */
  BX_WRITE_64BIT_REG(i->src(), op1_64);

  SET_FLAGS_OSZAPC_ADD_64(op1_64, op2_64, sum_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::XADD_EqGqR(bxInstruction_c* i)
{
  Bit64u op1_64, op2_64, sum_64;

  /* XADD dst(r/m), src(r)
   * temp <-- src + dst         | sum = op2 + op1
   * src  <-- dst               | op2 = op1
   * dst  <-- tmp               | op1 = sum
   */

  op1_64 = BX_READ_64BIT_REG(i->dst());
  op2_64 = BX_READ_64BIT_REG(i->src());
  sum_64 = op1_64 + op2_64;

  // and write destination into source
  // Note: if both op1 & op2 are registers, the last one written
  //       should be the sum, as op1 & op2 may be the same register.
  //       For example:  XADD AL, AL
  BX_WRITE_64BIT_REG(i->src(), op1_64);
  BX_WRITE_64BIT_REG(i->dst(), sum_64);

  SET_FLAGS_OSZAPC_ADD_64(op1_64, op2_64, sum_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMPXCHG_EqGqM(bxInstruction_c* i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  Bit64u op1_64 = read_RMW_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));
  Bit64u diff_64 = RAX - op1_64;
  SET_FLAGS_OSZAPC_SUB_64(RAX, op1_64, diff_64);

  if (diff_64 == 0) {  // if accumulator == dest
    // dest <-- src
    write_RMW_linear_qword(BX_READ_64BIT_REG(i->src()));
  }
  else {
    // accumulator <-- dest
    write_RMW_linear_qword(op1_64);
    RAX = op1_64;
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMPXCHG_EqGqR(bxInstruction_c* i)
{
  Bit64u op1_64 = BX_READ_64BIT_REG(i->dst());
  Bit64u diff_64 = RAX - op1_64;
  SET_FLAGS_OSZAPC_SUB_64(RAX, op1_64, diff_64);

  if (diff_64 == 0) {  // if accumulator == dest
    // dest <-- src
    BX_WRITE_64BIT_REG(i->dst(), BX_READ_64BIT_REG(i->src()));
  }
  else {
    // accumulator <-- dest
    RAX = op1_64;
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMPXCHG16B(bxInstruction_c* i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  Bit64u op1_64_lo, op1_64_hi, diff;

  // check write permission for following write
  read_RMW_linear_dqword_aligned_64(i->seg(), get_laddr64(i->seg(), eaddr), &op1_64_hi, &op1_64_lo);

  diff  = RAX - op1_64_lo;
  diff |= RDX - op1_64_hi;

  if (diff == 0) {  // if accumulator == dest
    write_RMW_linear_dqword(RCX, RBX);
    assert_ZF();
  }
  else {
    clear_ZF();
    write_RMW_linear_dqword(op1_64_hi, op1_64_lo);
    // accumulator <-- dest
    RAX = op1_64_lo;
    RDX = op1_64_hi;
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVO_GqEqR(bxInstruction_c* i)
{
  if (get_OF())
    BX_WRITE_64BIT_REG(i->dst(), BX_READ_64BIT_REG(i->src()));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVNO_GqEqR(bxInstruction_c* i)
{
  if (!get_OF())
    BX_WRITE_64BIT_REG(i->dst(), BX_READ_64BIT_REG(i->src()));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVB_GqEqR(bxInstruction_c* i)
{
  if (get_CF())
    BX_WRITE_64BIT_REG(i->dst(), BX_READ_64BIT_REG(i->src()));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVNB_GqEqR(bxInstruction_c* i)
{
  if (!get_CF())
    BX_WRITE_64BIT_REG(i->dst(), BX_READ_64BIT_REG(i->src()));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVZ_GqEqR(bxInstruction_c* i)
{
  if (get_ZF())
    BX_WRITE_64BIT_REG(i->dst(), BX_READ_64BIT_REG(i->src()));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVNZ_GqEqR(bxInstruction_c* i)
{
  if (!get_ZF())
    BX_WRITE_64BIT_REG(i->dst(), BX_READ_64BIT_REG(i->src()));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVBE_GqEqR(bxInstruction_c* i)
{
  if (get_CF() || get_ZF())
    BX_WRITE_64BIT_REG(i->dst(), BX_READ_64BIT_REG(i->src()));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVNBE_GqEqR(bxInstruction_c* i)
{
  if (! (get_CF() || get_ZF()))
    BX_WRITE_64BIT_REG(i->dst(), BX_READ_64BIT_REG(i->src()));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVS_GqEqR(bxInstruction_c* i)
{
  if (get_SF())
    BX_WRITE_64BIT_REG(i->dst(), BX_READ_64BIT_REG(i->src()));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVNS_GqEqR(bxInstruction_c* i)
{
  if (!get_SF())
    BX_WRITE_64BIT_REG(i->dst(), BX_READ_64BIT_REG(i->src()));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVP_GqEqR(bxInstruction_c* i)
{
  if (get_PF())
    BX_WRITE_64BIT_REG(i->dst(), BX_READ_64BIT_REG(i->src()));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVNP_GqEqR(bxInstruction_c* i)
{
  if (!get_PF())
    BX_WRITE_64BIT_REG(i->dst(), BX_READ_64BIT_REG(i->src()));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVL_GqEqR(bxInstruction_c* i)
{
  if (getB_SF() != getB_OF())
    BX_WRITE_64BIT_REG(i->dst(), BX_READ_64BIT_REG(i->src()));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVNL_GqEqR(bxInstruction_c* i)
{
  if (getB_SF() == getB_OF())
    BX_WRITE_64BIT_REG(i->dst(), BX_READ_64BIT_REG(i->src()));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVLE_GqEqR(bxInstruction_c* i)
{
  if (get_ZF() || (getB_SF() != getB_OF()))
    BX_WRITE_64BIT_REG(i->dst(), BX_READ_64BIT_REG(i->src()));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMOVNLE_GqEqR(bxInstruction_c* i)
{
  if (! get_ZF() && (getB_SF() == getB_OF()))
    BX_WRITE_64BIT_REG(i->dst(), BX_READ_64BIT_REG(i->src()));

  BX_NEXT_INSTR(i);
}

#endif

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
    op1_32 = 0 - op1_32;
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

void BX_CPP_AttrRegparmN(1) BX_CPU_C::REP_INSB_YbDX(bxInstruction_c *i)
{
  if (! allow_io(i, DX, 1)) {
    //BX_DEBUG(("INSB_YbDX: I/O access not allowed !"));
    exception(BX_GP_EXCEPTION, 0);
  }

#if BX_SUPPORT_X86_64
  if (i->as64L()) {
    BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::INSB64_YbDX);
  }
  else
#endif
  if (i->as32L()) {
    BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::INSB32_YbDX);
    BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RDI); // always clear upper part of RDI
  }
  else {
    BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::INSB16_YbDX);
  }

  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::INSB16_YbDX(bxInstruction_c *i)
{
  // trigger any segment or page faults before reading from IO port
  Bit8u value8 = read_RMW_virtual_byte_32(BX_SEG_REG_ES, DI); // no lock

  value8 = BX_INP(DX, 1);

  write_RMW_linear_byte(value8);

  if (BX_CPU_THIS_PTR get_DF())
    DI--;
  else
    DI++;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::INSB32_YbDX(bxInstruction_c *i)
{
  // trigger any segment or page faults before reading from IO port
  Bit8u value8 = read_RMW_virtual_byte(BX_SEG_REG_ES, EDI); // no lock

  value8 = BX_INP(DX, 1);

  /* no seg override possible */
  write_RMW_linear_byte(value8);

  if (BX_CPU_THIS_PTR get_DF()) {
    RDI = EDI - 1;
  }
  else {
    RDI = EDI + 1;
  }
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::INSB64_YbDX(bxInstruction_c *i)
{
  // trigger any segment or page faults before reading from IO port
  Bit8u value8 = read_RMW_linear_byte(BX_SEG_REG_ES, RDI); // no lock

  value8 = BX_INP(DX, 1);

  write_RMW_linear_byte(value8);

  if (BX_CPU_THIS_PTR get_DF())
    RDI--;
  else
    RDI++;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::REP_INSW_YwDX(bxInstruction_c *i)
{
  if (! allow_io(i, DX, 2)) {
    //BX_DEBUG(("INSW_YwDX: I/O access not allowed !"));
    exception(BX_GP_EXCEPTION, 0);
  }

#if BX_SUPPORT_X86_64
  if (i->as64L()) {
    BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::INSW64_YwDX);
  }
  else
#endif
  if (i->as32L()) {
    BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::INSW32_YwDX);
    BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RDI); // always clear upper part of RDI
  }
  else {
    BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::INSW16_YwDX);
  }

  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::INSW16_YwDX(bxInstruction_c *i)
{
  // trigger any segment or page faults before reading from IO port
  Bit16u value16 = read_RMW_virtual_word_32(BX_SEG_REG_ES, DI); // no lock

  value16 = BX_INP(DX, 2);

  write_RMW_linear_word(value16);

  if (BX_CPU_THIS_PTR get_DF())
    DI -= 2;
  else
    DI += 2;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::INSW32_YwDX(bxInstruction_c *i)
{
  Bit16u value16=0;
  Bit32u edi = EDI;
  unsigned increment = 2;

#if BX_SUPPORT_REPEAT_SPEEDUPS
  /* If conditions are right, we can transfer IO to physical memory
   * in a batch, rather than one instruction at a time.
   */
  if (i->repUsedL() && !BX_CPU_THIS_PTR async_event)
  {
    Bit32u wordCount = ECX;
    BX_ASSERT(wordCount > 0);
    wordCount = FastRepINSW(edi, DX, wordCount);
    if (wordCount) {
      // Decrement the ticks count by the number of iterations, minus
      // one, since the main cpu loop will decrement one.  Also,
      // the count is predecremented before examined, so defintely
      // don't roll it under zero.
      BX_TICKN(wordCount-1);
      RCX = ECX - (wordCount-1);
      increment = wordCount << 1; // count * 2.
    }
    else {
      // trigger any segment or page faults before reading from IO port
      value16 = read_RMW_virtual_word(BX_SEG_REG_ES, edi); // no lock

      value16 = BX_INP(DX, 2);

      write_RMW_linear_word(value16);
    }
  }
  else
#endif
  {
    // trigger any segment or page faults before reading from IO port
    value16 = read_RMW_virtual_word_32(BX_SEG_REG_ES, edi); // no lock

    value16 = BX_INP(DX, 2);

    write_RMW_linear_word(value16);
  }

  if (BX_CPU_THIS_PTR get_DF())
    RDI = EDI - increment;
  else
    RDI = EDI + increment;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::INSW64_YwDX(bxInstruction_c *i)
{
  // trigger any segment or page faults before reading from IO port
  Bit16u value16 = read_RMW_linear_word(BX_SEG_REG_ES, RDI); // no lock

  value16 = BX_INP(DX, 2);

  write_RMW_linear_word(value16);

  if (BX_CPU_THIS_PTR get_DF())
    RDI -= 2;
  else
    RDI += 2;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::REP_INSD_YdDX(bxInstruction_c *i)
{
  if (! allow_io(i, DX, 4)) {
    //BX_DEBUG(("INSD_YdDX: I/O access not allowed !"));
    exception(BX_GP_EXCEPTION, 0);
  }

#if BX_SUPPORT_X86_64
  if (i->as64L()) {
    BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::INSD64_YdDX);
  }
  else
#endif
  if (i->as32L()) {
    BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::INSD32_YdDX);
    BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RDI); // always clear upper part of RDI
  }
  else {
    BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::INSD16_YdDX);
  }

  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::INSD16_YdDX(bxInstruction_c *i)
{
  // trigger any segment or page faults before reading from IO port
  Bit32u value32 = read_RMW_virtual_dword_32(BX_SEG_REG_ES, DI); // no lock

  value32 = BX_INP(DX, 4);

  write_RMW_linear_dword(value32);

  if (BX_CPU_THIS_PTR get_DF())
    DI -= 4;
  else
    DI += 4;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::INSD32_YdDX(bxInstruction_c *i)
{
  // trigger any segment or page faults before reading from IO port
  Bit32u value32 = read_RMW_virtual_dword(BX_SEG_REG_ES, EDI); // no lock

  value32 = BX_INP(DX, 4);

  write_RMW_linear_dword(value32);

  if (BX_CPU_THIS_PTR get_DF())
    RDI = EDI - 4;
  else
    RDI = EDI + 4;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::INSD64_YdDX(bxInstruction_c *i)
{
  // trigger any segment or page faults before reading from IO port
  Bit32u value32 = read_RMW_linear_dword(BX_SEG_REG_ES, RDI); // no lock

  value32 = BX_INP(DX, 4);

  write_RMW_linear_dword(value32);

  if (BX_CPU_THIS_PTR get_DF())
    RDI -= 4;
  else
    RDI += 4;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::REP_OUTSB_DXXb(bxInstruction_c *i)
{
  if (! allow_io(i, DX, 1)) {
    //BX_DEBUG(("OUTSB_DXXb: I/O access not allowed !"));
    exception(BX_GP_EXCEPTION, 0);
  }

#if BX_SUPPORT_X86_64
  if (i->as64L()) {
    BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::OUTSB64_DXXb);
  }
  else
#endif
  if (i->as32L()) {
    BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::OUTSB32_DXXb);
    BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RSI); // always clear upper part of RSI
  }
  else {
    BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::OUTSB16_DXXb);
  }

  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OUTSB16_DXXb(bxInstruction_c *i)
{
  Bit8u value8 = read_virtual_byte_32(i->seg(), SI);
  BX_OUTP(DX, value8, 1);

  if (BX_CPU_THIS_PTR get_DF())
    SI--;
  else
    SI++;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OUTSB32_DXXb(bxInstruction_c *i)
{
  Bit8u value8 = read_virtual_byte(i->seg(), ESI);
  BX_OUTP(DX, value8, 1);

  if (BX_CPU_THIS_PTR get_DF())
    RSI = ESI - 1;
  else
    RSI = ESI + 1;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OUTSB64_DXXb(bxInstruction_c *i)
{
  Bit8u value8 = read_linear_byte(i->seg(), get_laddr64(i->seg(), RSI));
  BX_OUTP(DX, value8, 1);

  if (BX_CPU_THIS_PTR get_DF())
    RSI--;
  else
    RSI++;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::REP_OUTSW_DXXw(bxInstruction_c *i)
{
  if (! allow_io(i, DX, 2)) {
    //BX_DEBUG(("OUTSW_DXXw: I/O access not allowed !"));
    exception(BX_GP_EXCEPTION, 0);
  }

#if BX_SUPPORT_X86_64
  if (i->as64L()) {
    BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::OUTSW64_DXXw);
  }
  else
#endif
  if (i->as32L()) {
    BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::OUTSW32_DXXw);
    BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RSI); // always clear upper part of RSI
  }
  else {
    BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::OUTSW16_DXXw);
  }

  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OUTSW16_DXXw(bxInstruction_c *i)
{
  Bit16u value16 = read_virtual_word_32(i->seg(), SI);
  BX_OUTP(DX, value16, 2);

  if (BX_CPU_THIS_PTR get_DF())
    SI -= 2;
  else
    SI += 2;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OUTSW32_DXXw(bxInstruction_c *i)
{
  Bit16u value16;
  Bit32u esi = ESI;
  unsigned increment = 2;

#if BX_SUPPORT_REPEAT_SPEEDUPS
  /* If conditions are right, we can transfer IO to physical memory
   * in a batch, rather than one instruction at a time.
   */
  if (i->repUsedL() && !BX_CPU_THIS_PTR async_event) {
    Bit32u wordCount = ECX;
    wordCount = FastRepOUTSW(i->seg(), esi, DX, wordCount);
    if (wordCount) {
      // Decrement eCX.  Note, the main loop will decrement 1 also, so
      // decrement by one less than expected, like the case above.
      BX_TICKN(wordCount-1); // Main cpu loop also decrements one more.
      RCX = ECX - (wordCount-1);
      increment = wordCount << 1; // count * 2.
    }
    else {
      value16 = read_virtual_word(i->seg(), esi);
      BX_OUTP(DX, value16, 2);
    }
  }
  else
#endif
  {
    value16 = read_virtual_word(i->seg(), esi);
    BX_OUTP(DX, value16, 2);
  }

  if (BX_CPU_THIS_PTR get_DF())
    RSI = ESI - increment;
  else
    RSI = ESI + increment;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OUTSW64_DXXw(bxInstruction_c *i)
{
  Bit16u value16 = read_linear_word(i->seg(), get_laddr64(i->seg(), RSI));
  BX_OUTP(DX, value16, 2);

  if (BX_CPU_THIS_PTR get_DF())
    RSI -= 2;
  else
    RSI += 2;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::REP_OUTSD_DXXd(bxInstruction_c *i)
{
  if (! allow_io(i, DX, 4)) {
    //BX_DEBUG(("OUTSD_DXXd: I/O access not allowed !"));
    exception(BX_GP_EXCEPTION, 0);
  }

#if BX_SUPPORT_X86_64
  if (i->as64L()) {
    BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::OUTSD64_DXXd);
  }
  else
#endif
  if (i->as32L()) {
    BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::OUTSD32_DXXd);
    BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RSI); // always clear upper part of RSI
  }
  else {
    BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::OUTSD16_DXXd);
  }

  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OUTSD16_DXXd(bxInstruction_c *i)
{
  Bit32u value32 = read_virtual_dword_32(i->seg(), SI);
  BX_OUTP(DX, value32, 4);

  if (BX_CPU_THIS_PTR get_DF())
    SI -= 4;
  else
    SI += 4;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OUTSD32_DXXd(bxInstruction_c *i)
{
  Bit32u value32 = read_virtual_dword(i->seg(), ESI);
  BX_OUTP(DX, value32, 4);

  if (BX_CPU_THIS_PTR get_DF())
    RSI = ESI - 4;
  else
    RSI = ESI + 4;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OUTSD64_DXXd(bxInstruction_c *i)
{
  Bit32u value32 = read_linear_dword(i->seg(), get_laddr64(i->seg(), RSI));
  BX_OUTP(DX, value32, 4);

  if (BX_CPU_THIS_PTR get_DF())
    RSI -= 4;
  else
    RSI += 4;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::IN_AXIb(bxInstruction_c *i)
{
  unsigned port = i->Ib();

  if (! allow_io(i, port, 2)) {
    //BX_DEBUG(("IN_AXIb: I/O access not allowed !"));
    exception(BX_GP_EXCEPTION, 0);
  }

  AX = BX_INP(port, 2);

  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::IN_EAXIb(bxInstruction_c *i)
{
  unsigned port = i->Ib();

  if (! allow_io(i, port, 4)) {
    //BX_DEBUG(("IN_EAXIb: I/O access not allowed !"));
    exception(BX_GP_EXCEPTION, 0);
  }

  RAX = BX_INP(port, 4);

  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OUT_IbAX(bxInstruction_c *i)
{
  unsigned port = i->Ib();

  if (! allow_io(i, port, 2)) {
    //BX_DEBUG(("OUT_IbAX: I/O access not allowed !"));
    exception(BX_GP_EXCEPTION, 0);
  }

  BX_OUTP(port, AX, 2);

  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OUT_IbEAX(bxInstruction_c *i)
{
  unsigned port = i->Ib();

  if (! allow_io(i, port, 4)) {
    //BX_DEBUG(("OUT_IbEAX: I/O access not allowed !"));
    exception(BX_GP_EXCEPTION, 0);
  }

  BX_OUTP(port, EAX, 4);

  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OUT_DXAX(bxInstruction_c *i)
{
  unsigned port = DX;

  if (! allow_io(i, port, 2)) {
    //BX_DEBUG(("OUT_DXAX: I/O access not allowed !"));
    exception(BX_GP_EXCEPTION, 0);
  }

  BX_OUTP(port, AX, 2);

  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::REP_MOVSW_YwXw(bxInstruction_c *i)
{
#if BX_SUPPORT_X86_64
  if (i->as64L())
    BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::MOVSW64_YwXw);
  else
#endif
  if (i->as32L()) {
    BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::MOVSW32_YwXw);
    BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RSI); // always clear upper part of RSI/RDI
    BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RDI);
  }
  else {
    BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::MOVSW16_YwXw);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOVSW16_YwXw(bxInstruction_c *i)
{
  Bit16u si = SI;
  Bit16u di = DI;

  Bit16u temp16 = read_virtual_word_32(i->seg(), si);
  write_virtual_word_32(BX_SEG_REG_ES, di, temp16);

  if (BX_CPU_THIS_PTR get_DF()) {
    si -= 2;
    di -= 2;
  }
  else {
    si += 2;
    di += 2;
  }

  SI = si;
  DI = di;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOVSW32_YwXw(bxInstruction_c *i)
{
  Bit32u esi = ESI;
  Bit32u edi = EDI;

  Bit16u temp16 = read_virtual_word(i->seg(), esi);
  write_virtual_word(BX_SEG_REG_ES, edi, temp16);

  if (BX_CPU_THIS_PTR get_DF()) {
    esi -= 2;
    edi -= 2;
  }
  else {
    esi += 2;
    edi += 2;
  }

  // zero extension of RSI/RDI
  RSI = esi;
  RDI = edi;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOVSW64_YwXw(bxInstruction_c *i)
{
  Bit64u rsi = RSI;
  Bit64u rdi = RDI;

  Bit16u temp16 = read_linear_word(i->seg(), get_laddr64(i->seg(), rsi));
  write_linear_word(BX_SEG_REG_ES, rdi, temp16);

  if (BX_CPU_THIS_PTR get_DF()) {
    rsi -= 2;
    rdi -= 2;
  }
  else {
    rsi += 2;
    rdi += 2;
  }

  RSI = rsi;
  RDI = rdi;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::REP_MOVSD_YdXd(bxInstruction_c *i)
{
#if BX_SUPPORT_X86_64
  if (i->as64L())
    BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::MOVSD64_YdXd);
  else
#endif
  if (i->as32L()) {
    BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::MOVSD32_YdXd);
    BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RSI); // always clear upper part of RSI/RDI
    BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RDI);
  }
  else {
    BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::MOVSD16_YdXd);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOVSD16_YdXd(bxInstruction_c *i)
{
  Bit16u si = SI;
  Bit16u di = DI;

  Bit32u temp32 = read_virtual_dword_32(i->seg(), si);
  write_virtual_dword_32(BX_SEG_REG_ES, di, temp32);

  if (BX_CPU_THIS_PTR get_DF()) {
    si -= 4;
    di -= 4;
  }
  else {
    si += 4;
    di += 4;
  }

  SI = si;
  DI = di;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOVSD32_YdXd(bxInstruction_c *i)
{
  Bit32s increment = 0;

  Bit32u esi = ESI;
  Bit32u edi = EDI;

#if BX_SUPPORT_REPEAT_SPEEDUPS
  /* If conditions are right, we can transfer IO to physical memory
   * in a batch, rather than one instruction at a time.
   */
  if (i->repUsedL() && !BX_CPU_THIS_PTR get_DF() && !BX_CPU_THIS_PTR async_event)
  {
    Bit32u byteCount = FastRepMOVSB(i->seg(), esi, BX_SEG_REG_ES, edi, ECX*4, 4);
    if (byteCount) {
      Bit32u dwordCount = byteCount >> 2;

      // Decrement the ticks count by the number of iterations, minus
      // one, since the main cpu loop will decrement one.  Also,
      // the count is predecremented before examined, so definitely
      // don't roll it under zero.
      BX_TICKN(dwordCount-1);

      // Decrement eCX. Note, the main loop will decrement 1 also, so
      // decrement by one less than expected, like the case above.
      RCX = ECX - (dwordCount-1);

      increment = byteCount;
    }
  }

  if (increment == 0)
#endif
  {
    Bit32u temp32 = read_virtual_dword(i->seg(), esi);
    write_virtual_dword(BX_SEG_REG_ES, edi, temp32);

    increment = BX_CPU_THIS_PTR get_DF() ? -4 : 4;
  }

  // zero extension of RSI/RDI
  RSI = esi + increment;
  RDI = edi + increment;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOVSD64_YdXd(bxInstruction_c *i)
{
  Bit32s increment = 0;

  Bit64u rsi = RSI;
  Bit64u rdi = RDI;

#if BX_SUPPORT_REPEAT_SPEEDUPS
  /* If conditions are right, we can transfer IO to physical memory
   * in a batch, rather than one instruction at a time.
   */
  if (i->repUsedL() && !BX_CPU_THIS_PTR get_DF() && !BX_CPU_THIS_PTR async_event)
  {
    Bit32u byteCount = FastRepMOVSB(get_laddr64(i->seg(), rsi), rdi, ECX*4, 4);
    if (byteCount) {
      Bit32u dwordCount = byteCount >> 2;

      // Decrement the ticks count by the number of iterations, minus
      // one, since the main cpu loop will decrement one.  Also,
      // the count is predecremented before examined, so definitely
      // don't roll it under zero.
      BX_TICKN(dwordCount-1);

      // Decrement RCX. Note, the main loop will decrement 1 also, so
      // decrement by one less than expected, like the case above.
      RCX -= (dwordCount-1);

      increment = byteCount;
    }
  }

  if (increment == 0)
#endif
  {
    Bit32u temp32 = read_linear_dword(i->seg(), get_laddr64(i->seg(), rsi));
    write_linear_dword(BX_SEG_REG_ES, rdi, temp32);

    increment = BX_CPU_THIS_PTR get_DF() ? -4 : 4;
  }

  RSI = rsi + increment;
  RDI = rdi + increment;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::REP_CMPSW_XwYw(bxInstruction_c *i)
{
#if BX_SUPPORT_X86_64
  if (i->as64L()) {
    BX_CPU_THIS_PTR repeat_ZF(i, &BX_CPU_C::CMPSW64_XwYw);
  }
  else
#endif
  if (i->as32L()) {
    BX_CPU_THIS_PTR repeat_ZF(i, &BX_CPU_C::CMPSW32_XwYw);
    BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RSI); // always clear upper part of RSI/RDI
    BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RDI);
  }
  else {
    BX_CPU_THIS_PTR repeat_ZF(i, &BX_CPU_C::CMPSW16_XwYw);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMPSW16_XwYw(bxInstruction_c *i)
{
  Bit16u op1_16, op2_16, diff_16;

  Bit16u si = SI;
  Bit16u di = DI;

  op1_16 = read_virtual_word_32(i->seg(), si);
  op2_16 = read_virtual_word_32(BX_SEG_REG_ES, di);

  diff_16 = op1_16 - op2_16;

  SET_FLAGS_OSZAPC_SUB_16(op1_16, op2_16, diff_16);

  if (BX_CPU_THIS_PTR get_DF()) {
    si -= 2;
    di -= 2;
  }
  else {
    si += 2;
    di += 2;
  }

  DI = di;
  SI = si;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMPSW32_XwYw(bxInstruction_c *i)
{
  Bit16u op1_16, op2_16, diff_16;

  Bit32u esi = ESI;
  Bit32u edi = EDI;

  op1_16 = read_virtual_word(i->seg(), esi);
  op2_16 = read_virtual_word(BX_SEG_REG_ES, edi);

  diff_16 = op1_16 - op2_16;

  SET_FLAGS_OSZAPC_SUB_16(op1_16, op2_16, diff_16);

  if (BX_CPU_THIS_PTR get_DF()) {
    esi -= 2;
    edi -= 2;
  }
  else {
    esi += 2;
    edi += 2;
  }

  // zero extension of RSI/RDI
  RDI = edi;
  RSI = esi;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMPSW64_XwYw(bxInstruction_c *i)
{
  Bit16u op1_16, op2_16, diff_16;

  Bit64u rsi = RSI;
  Bit64u rdi = RDI;

  op1_16 = read_linear_word(i->seg(), get_laddr64(i->seg(), rsi));
  op2_16 = read_linear_word(BX_SEG_REG_ES, rdi);

  diff_16 = op1_16 - op2_16;

  SET_FLAGS_OSZAPC_SUB_16(op1_16, op2_16, diff_16);

  if (BX_CPU_THIS_PTR get_DF()) {
    rsi -= 2;
    rdi -= 2;
  }
  else {
    rsi += 2;
    rdi += 2;
  }

  RDI = rdi;
  RSI = rsi;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::REP_CMPSD_XdYd(bxInstruction_c *i)
{
#if BX_SUPPORT_X86_64
  if (i->as64L()) {
    BX_CPU_THIS_PTR repeat_ZF(i, &BX_CPU_C::CMPSD64_XdYd);
  }
  else
#endif
  if (i->as32L()) {
    BX_CPU_THIS_PTR repeat_ZF(i, &BX_CPU_C::CMPSD32_XdYd);
    BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RSI); // always clear upper part of RSI/RDI
    BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RDI);
  }
  else {
    BX_CPU_THIS_PTR repeat_ZF(i, &BX_CPU_C::CMPSD16_XdYd);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMPSD16_XdYd(bxInstruction_c *i)
{
  Bit32u op1_32, op2_32, diff_32;

  Bit16u si = SI;
  Bit16u di = DI;

  op1_32 = read_virtual_dword_32(i->seg(), si);
  op2_32 = read_virtual_dword_32(BX_SEG_REG_ES, di);

  diff_32 = op1_32 - op2_32;

  SET_FLAGS_OSZAPC_SUB_32(op1_32, op2_32, diff_32);

  if (BX_CPU_THIS_PTR get_DF()) {
    si -= 4;
    di -= 4;
  }
  else {
    si += 4;
    di += 4;
  }

  DI = di;
  SI = si;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMPSD32_XdYd(bxInstruction_c *i)
{
  Bit32u op1_32, op2_32, diff_32;

  Bit32u esi = ESI;
  Bit32u edi = EDI;

  op1_32 = read_virtual_dword(i->seg(), esi);
  op2_32 = read_virtual_dword(BX_SEG_REG_ES, edi);

  diff_32 = op1_32 - op2_32;

  SET_FLAGS_OSZAPC_SUB_32(op1_32, op2_32, diff_32);

  if (BX_CPU_THIS_PTR get_DF()) {
    esi -= 4;
    edi -= 4;
  }
  else {
    esi += 4;
    edi += 4;
  }

  // zero extension of RSI/RDI
  RDI = edi;
  RSI = esi;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMPSD64_XdYd(bxInstruction_c *i)
{
  Bit32u op1_32, op2_32, diff_32;

  Bit64u rsi = RSI;
  Bit64u rdi = RDI;

  op1_32 = read_linear_dword(i->seg(), get_laddr64(i->seg(), rsi));
  op2_32 = read_linear_dword(BX_SEG_REG_ES, rdi);

  diff_32 = op1_32 - op2_32;

  SET_FLAGS_OSZAPC_SUB_32(op1_32, op2_32, diff_32);

  if (BX_CPU_THIS_PTR get_DF()) {
    rsi -= 4;
    rdi -= 4;
  }
  else {
    rsi += 4;
    rdi += 4;
  }

  RDI = rdi;
  RSI = rsi;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::REP_SCASB_ALYb(bxInstruction_c *i)
{
#if BX_SUPPORT_X86_64
  if (i->as64L()) {
    BX_CPU_THIS_PTR repeat_ZF(i, &BX_CPU_C::SCASB64_ALYb);
  }
  else
#endif
  if (i->as32L()) {
    BX_CPU_THIS_PTR repeat_ZF(i, &BX_CPU_C::SCASB32_ALYb);
    BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RDI); // always clear upper part of RDI
  }
  else {
    BX_CPU_THIS_PTR repeat_ZF(i, &BX_CPU_C::SCASB16_ALYb);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SCASB16_ALYb(bxInstruction_c *i)
{
  Bit8u op1_8 = AL, op2_8, diff_8;

  Bit16u di = DI;

  op2_8 = read_virtual_byte_32(BX_SEG_REG_ES, di);

  diff_8 = op1_8 - op2_8;

  SET_FLAGS_OSZAPC_SUB_8(op1_8, op2_8, diff_8);

  if (BX_CPU_THIS_PTR get_DF()) {
    di--;
  }
  else {
    di++;
  }

  DI = di;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SCASB32_ALYb(bxInstruction_c *i)
{
  Bit8u op1_8 = AL, op2_8, diff_8;

  Bit32u edi = EDI;

  op2_8 = read_virtual_byte(BX_SEG_REG_ES, edi);
  diff_8 = op1_8 - op2_8;

  SET_FLAGS_OSZAPC_SUB_8(op1_8, op2_8, diff_8);

  if (BX_CPU_THIS_PTR get_DF()) {
    edi--;
  }
  else {
    edi++;
  }

  // zero extension of RDI
  RDI = edi;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SCASB64_ALYb(bxInstruction_c *i)
{
  Bit8u op1_8 = AL, op2_8, diff_8;

  Bit64u rdi = RDI;

  op2_8 = read_virtual_byte(BX_SEG_REG_ES, rdi);

  diff_8 = op1_8 - op2_8;

  SET_FLAGS_OSZAPC_SUB_8(op1_8, op2_8, diff_8);

  if (BX_CPU_THIS_PTR get_DF()) {
    rdi--;
  }
  else {
    rdi++;
  }

  RDI = rdi;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::REP_SCASW_AXYw(bxInstruction_c *i)
{
#if BX_SUPPORT_X86_64
  if (i->as64L()) {
    BX_CPU_THIS_PTR repeat_ZF(i, &BX_CPU_C::SCASW64_AXYw);
  }
  else
#endif
  if (i->as32L()) {
    BX_CPU_THIS_PTR repeat_ZF(i, &BX_CPU_C::SCASW32_AXYw);
    BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RDI); // always clear upper part of RDI
  }
  else {
    BX_CPU_THIS_PTR repeat_ZF(i, &BX_CPU_C::SCASW16_AXYw);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SCASW16_AXYw(bxInstruction_c *i)
{
  Bit16u op1_16 = AX, op2_16, diff_16;

  Bit16u di = DI;

  op2_16 = read_virtual_word_32(BX_SEG_REG_ES, di);
  diff_16 = op1_16 - op2_16;

  SET_FLAGS_OSZAPC_SUB_16(op1_16, op2_16, diff_16);

  if (BX_CPU_THIS_PTR get_DF()) {
    di -= 2;
  }
  else {
    di += 2;
  }

  DI = di;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SCASW32_AXYw(bxInstruction_c *i)
{
  Bit16u op1_16 = AX, op2_16, diff_16;

  Bit32u edi = EDI;

  op2_16 = read_virtual_word(BX_SEG_REG_ES, edi);
  diff_16 = op1_16 - op2_16;

  SET_FLAGS_OSZAPC_SUB_16(op1_16, op2_16, diff_16);

  if (BX_CPU_THIS_PTR get_DF()) {
    edi -= 2;
  }
  else {
    edi += 2;
  }

  // zero extension of RDI
  RDI = edi;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SCASW64_AXYw(bxInstruction_c *i)
{
  Bit16u op1_16 = AX, op2_16, diff_16;

  Bit64u rdi = RDI;

  op2_16 = read_virtual_word(BX_SEG_REG_ES, rdi);

  diff_16 = op1_16 - op2_16;

  SET_FLAGS_OSZAPC_SUB_16(op1_16, op2_16, diff_16);

  if (BX_CPU_THIS_PTR get_DF()) {
    rdi -= 2;
  }
  else {
    rdi += 2;
  }

  RDI = rdi;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::REP_SCASD_EAXYd(bxInstruction_c *i)
{
#if BX_SUPPORT_X86_64
  if (i->as64L()) {
    BX_CPU_THIS_PTR repeat_ZF(i, &BX_CPU_C::SCASD64_EAXYd);
  }
  else
#endif
  if (i->as32L()) {
    BX_CPU_THIS_PTR repeat_ZF(i, &BX_CPU_C::SCASD32_EAXYd);
    BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RDI); // always clear upper part of RDI
  }
  else {
    BX_CPU_THIS_PTR repeat_ZF(i, &BX_CPU_C::SCASD16_EAXYd);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SCASD16_EAXYd(bxInstruction_c *i)
{
  Bit32u op1_32 = EAX, op2_32, diff_32;

  Bit16u di = DI;

  op2_32 = read_virtual_dword_32(BX_SEG_REG_ES, di);
  diff_32 = op1_32 - op2_32;

  SET_FLAGS_OSZAPC_SUB_32(op1_32, op2_32, diff_32);

  if (BX_CPU_THIS_PTR get_DF()) {
    di -= 4;
  }
  else {
    di += 4;
  }

  DI = di;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SCASD32_EAXYd(bxInstruction_c *i)
{
  Bit32u op1_32 = EAX, op2_32, diff_32;

  Bit32u edi = EDI;

  op2_32 = read_virtual_dword(BX_SEG_REG_ES, edi);
  diff_32 = op1_32 - op2_32;

  SET_FLAGS_OSZAPC_SUB_32(op1_32, op2_32, diff_32);

  if (BX_CPU_THIS_PTR get_DF()) {
    edi -= 4;
  }
  else {
    edi += 4;
  }

  // zero extension of RDI
  RDI = edi;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SCASD64_EAXYd(bxInstruction_c *i)
{
  Bit32u op1_32 = EAX, op2_32, diff_32;

  Bit64u rdi = RDI;

  op2_32 = read_virtual_dword(BX_SEG_REG_ES, rdi);

  diff_32 = op1_32 - op2_32;

  SET_FLAGS_OSZAPC_SUB_32(op1_32, op2_32, diff_32);

  if (BX_CPU_THIS_PTR get_DF()) {
    rdi -= 4;
  }
  else {
    rdi += 4;
  }

  RDI = rdi;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::REP_LODSB_ALXb(bxInstruction_c *i)
{
#if BX_SUPPORT_X86_64
  if (i->as64L())
    BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::LODSB64_ALXb);
  else
#endif
  if (i->as32L()) {
    BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::LODSB32_ALXb);
    BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RSI); // always clear upper part of RSI
  }
  else {
    BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::LODSB16_ALXb);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LODSB16_ALXb(bxInstruction_c *i)
{
  Bit16u si = SI;

  AL = read_virtual_byte_32(i->seg(), si);

  if (BX_CPU_THIS_PTR get_DF()) {
    si--;
  }
  else {
    si++;
  }

  SI = si;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LODSB32_ALXb(bxInstruction_c *i)
{
  Bit32u esi = ESI;

  AL = read_virtual_byte(i->seg(), esi);

  if (BX_CPU_THIS_PTR get_DF()) {
    esi--;
  }
  else {
    esi++;
  }

  // zero extension of RSI
  RSI = esi;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LODSB64_ALXb(bxInstruction_c *i)
{
  Bit64u rsi = RSI;

  AL = read_linear_byte(i->seg(), get_laddr64(i->seg(), rsi));

  if (BX_CPU_THIS_PTR get_DF()) {
    rsi--;
  }
  else {
    rsi++;
  }

  RSI = rsi;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::REP_LODSW_AXXw(bxInstruction_c *i)
{
#if BX_SUPPORT_X86_64
  if (i->as64L())
    BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::LODSW64_AXXw);
  else
#endif
  if (i->as32L()) {
    BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::LODSW32_AXXw);
    BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RSI); // always clear upper part of RSI
  }
  else {
    BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::LODSW16_AXXw);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LODSW16_AXXw(bxInstruction_c *i)
{
  Bit16u si = SI;

  AX = read_virtual_word_32(i->seg(), si);

  if (BX_CPU_THIS_PTR get_DF()) {
    si -= 2;
  }
  else {
    si += 2;
  }

  SI = si;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LODSW32_AXXw(bxInstruction_c *i)
{
  Bit32u esi = ESI;

  AX = read_virtual_word(i->seg(), esi);

  if (BX_CPU_THIS_PTR get_DF()) {
    esi -= 2;
  }
  else {
    esi += 2;
  }

  // zero extension of RSI
  RSI = esi;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LODSW64_AXXw(bxInstruction_c *i)
{
  Bit64u rsi = RSI;

  AX = read_linear_word(i->seg(), get_laddr64(i->seg(), rsi));

  if (BX_CPU_THIS_PTR get_DF()) {
    rsi -= 2;
  }
  else {
    rsi += 2;
  }

  RSI = rsi;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::REP_LODSD_EAXXd(bxInstruction_c *i)
{
#if BX_SUPPORT_X86_64
  if (i->as64L())
    BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::LODSD64_EAXXd);
  else
#endif
  if (i->as32L()) {
    BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::LODSD32_EAXXd);
    BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RSI); // always clear upper part of RSI
  }
  else {
    BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::LODSD16_EAXXd);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LODSD16_EAXXd(bxInstruction_c *i)
{
  Bit16u si = SI;

  RAX = read_virtual_dword_32(i->seg(), si);

  if (BX_CPU_THIS_PTR get_DF()) {
    si -= 4;
  }
  else {
    si += 4;
  }

  SI = si;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LODSD32_EAXXd(bxInstruction_c *i)
{
  Bit32u esi = ESI;

  RAX = read_virtual_dword(i->seg(), esi);

  if (BX_CPU_THIS_PTR get_DF()) {
    esi -= 4;
  }
  else {
    esi += 4;
  }

  // zero extension of RSI
  RSI = esi;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LODSD64_EAXXd(bxInstruction_c *i)
{
  Bit64u rsi = RSI;

  RAX = read_linear_dword(i->seg(), get_laddr64(i->seg(), rsi));

  if (BX_CPU_THIS_PTR get_DF()) {
    rsi -= 4;
  }
  else {
    rsi += 4;
  }

  RSI = rsi;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JO_Jw(bxInstruction_c *i)
{
  if (get_OF()) {
    Bit16u new_IP = IP + i->Iw();
    branch_near16(new_IP);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_IP);
    BX_LINK_TRACE(i);
  }

  BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
  BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JNO_Jw(bxInstruction_c *i)
{
  if (! get_OF()) {
    Bit16u new_IP = IP + i->Iw();
    branch_near16(new_IP);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_IP);
    BX_LINK_TRACE(i);
  }

  BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
  BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JB_Jw(bxInstruction_c *i)
{
  if (get_CF()) {
    Bit16u new_IP = IP + i->Iw();
    branch_near16(new_IP);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_IP);
    BX_LINK_TRACE(i);
  }

  BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
  BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JNB_Jw(bxInstruction_c *i)
{
  if (! get_CF()) {
    Bit16u new_IP = IP + i->Iw();
    branch_near16(new_IP);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_IP);
    BX_LINK_TRACE(i);
  }

  BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
  BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JBE_Jw(bxInstruction_c *i)
{
  if (get_CF() || get_ZF()) {
    Bit16u new_IP = IP + i->Iw();
    branch_near16(new_IP);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_IP);
    BX_LINK_TRACE(i);
  }

  BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
  BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JS_Jw(bxInstruction_c *i)
{
  if (get_SF()) {
    Bit16u new_IP = IP + i->Iw();
    branch_near16(new_IP);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_IP);
    BX_LINK_TRACE(i);
  }

  BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
  BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JNS_Jw(bxInstruction_c *i)
{
  if (! get_SF()) {
    Bit16u new_IP = IP + i->Iw();
    branch_near16(new_IP);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_IP);
    BX_LINK_TRACE(i);
  }

  BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
  BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JP_Jw(bxInstruction_c *i)
{
  if (get_PF()) {
    Bit16u new_IP = IP + i->Iw();
    branch_near16(new_IP);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_IP);
    BX_LINK_TRACE(i);
  }

  BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
  BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JNP_Jw(bxInstruction_c *i)
{
  if (! get_PF()) {
    Bit16u new_IP = IP + i->Iw();
    branch_near16(new_IP);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_IP);
    BX_LINK_TRACE(i);
  }

  BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
  BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JL_Jw(bxInstruction_c *i)
{
  if (getB_SF() != getB_OF()) {
    Bit16u new_IP = IP + i->Iw();
    branch_near16(new_IP);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_IP);
    BX_LINK_TRACE(i);
  }

  BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
  BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JNL_Jw(bxInstruction_c *i)
{
  if (getB_SF() == getB_OF()) {
    Bit16u new_IP = IP + i->Iw();
    branch_near16(new_IP);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_IP);
    BX_LINK_TRACE(i);
  }

  BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
  BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JLE_Jw(bxInstruction_c *i)
{
  if (get_ZF() || (getB_SF() != getB_OF())) {
    Bit16u new_IP = IP + i->Iw();
    branch_near16(new_IP);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_IP);
    BX_LINK_TRACE(i);
  }

  BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
  BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JNLE_Jw(bxInstruction_c *i)
{
  if (! get_ZF() && (getB_SF() == getB_OF())) {
    Bit16u new_IP = IP + i->Iw();
    branch_near16(new_IP);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_IP);
    BX_LINK_TRACE(i);
  }

  BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
  BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JO_Jd(bxInstruction_c *i)
{
  if (get_OF()) {
    Bit32u new_EIP = EIP + (Bit32s) i->Id();
    branch_near32(new_EIP);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_EIP);
    BX_LINK_TRACE(i);
  }

  BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
  BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JNO_Jd(bxInstruction_c *i)
{
  if (! get_OF()) {
    Bit32u new_EIP = EIP + (Bit32s) i->Id();
    branch_near32(new_EIP);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_EIP);
    BX_LINK_TRACE(i);
  }

  BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
  BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JNB_Jd(bxInstruction_c *i)
{
  if (! get_CF()) {
    Bit32u new_EIP = EIP + (Bit32s) i->Id();
    branch_near32(new_EIP);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_EIP);
    BX_LINK_TRACE(i);
  }

  BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
  BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JNS_Jd(bxInstruction_c *i)
{
  if (! get_SF()) {
    Bit32u new_EIP = EIP + (Bit32s) i->Id();
    branch_near32(new_EIP);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_EIP);
    BX_LINK_TRACE(i);
  }

  BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
  BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JP_Jd(bxInstruction_c *i)
{
  if (get_PF()) {
    Bit32u new_EIP = EIP + (Bit32s) i->Id();
    branch_near32(new_EIP);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_EIP);
    BX_LINK_TRACE(i);
  }

  BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
  BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JNP_Jd(bxInstruction_c *i)
{
  if (! get_PF()) {
    Bit32u new_EIP = EIP + (Bit32s) i->Id();
    branch_near32(new_EIP);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_EIP);
    BX_LINK_TRACE(i);
  }

  BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
  BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JCXZ_Jb(bxInstruction_c *i)
{
  // it is impossible to get this instruction in long mode
  //BX_ASSERT(i->as64L() == 0);

  Bit32u temp_ECX;

  if (i->as32L())
    temp_ECX = ECX;
  else
    temp_ECX = CX;

  if (temp_ECX == 0) {
    Bit16u new_IP = IP + i->Iw();
    branch_near16(new_IP);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_IP);
    BX_LINK_TRACE(i);
  }

  BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JECXZ_Jb(bxInstruction_c *i)
{
  // it is impossible to get this instruction in long mode
  //BX_ASSERT(i->as64L() == 0);

  Bit32u temp_ECX;

  if (i->as32L())
    temp_ECX = ECX;
  else
    temp_ECX = CX;

  if (temp_ECX == 0) {
    Bit32u new_EIP = EIP + (Bit32s) i->Id();
    branch_near32(new_EIP);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_EIP);
    BX_LINK_TRACE(i);
  }

  BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LOOPE16_Jb(bxInstruction_c *i)
{
  // it is impossible to get this instruction in long mode
  //BX_ASSERT(i->as64L() == 0);

  if (i->as32L()) {
    Bit32u count = ECX;

    count--;
    if (count != 0 && get_ZF()) {
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
    if (count != 0 && get_ZF()) {
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

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LOOPNE16_Jb(bxInstruction_c *i)
{
  // it is impossible to get this instruction in long mode
  //BX_ASSERT(i->as64L() == 0);

  if (i->as32L()) {
    Bit32u count = ECX;

    count--;
    if (count != 0 && (get_ZF()==0)) {
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
    if (count != 0 && (get_ZF()==0)) {
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

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LOOP32_Jb(bxInstruction_c *i)
{
  // it is impossible to get this instruction in long mode
  //BX_ASSERT(i->as64L() == 0);

  if (i->as32L()) {
    Bit32u count = ECX;

    count--;
    if (count != 0) {
      Bit32u new_EIP = EIP + (Bit32s) i->Id();
      branch_near32(new_EIP);
      BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_EIP);
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
      Bit32u new_EIP = EIP + (Bit32s) i->Id();
      branch_near32(new_EIP);
      BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_EIP);
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

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LOOPE32_Jb(bxInstruction_c *i)
{
  // it is impossible to get this instruction in long mode
  //BX_ASSERT(i->as64L() == 0);

  if (i->as32L()) {
    Bit32u count = ECX;

    count--;
    if (count != 0 && get_ZF()) {
      Bit32u new_EIP = EIP + (Bit32s) i->Id();
      branch_near32(new_EIP);
      BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_EIP);
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
    if (count != 0 && get_ZF()) {
      Bit32u new_EIP = EIP + (Bit32s) i->Id();
      branch_near32(new_EIP);
      BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_EIP);
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

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LOOPNE32_Jb(bxInstruction_c *i)
{
  // it is impossible to get this instruction in long mode
  //BX_ASSERT(i->as64L() == 0);

  if (i->as32L()) {
    Bit32u count = ECX;

    count--;
    if (count != 0 && (get_ZF()==0)) {
      Bit32u new_EIP = EIP + (Bit32s) i->Id();
      branch_near32(new_EIP);
      BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_EIP);
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
    if (count != 0 && (get_ZF()==0)) {
      Bit32u new_EIP = EIP + (Bit32s) i->Id();
      branch_near32(new_EIP);
      BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, new_EIP);
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

void BX_CPP_AttrRegparmN(1) BX_CPU_C::INT3(bxInstruction_c *i)
{
  BX_INSTR_FAR_BRANCH_ORIGIN();

  // INT 3 is not IOPL sensitive

#if BX_SUPPORT_VMX
  VMexit_Event(BX_SOFTWARE_EXCEPTION, 3, 0, 0);
#endif

#if BX_SUPPORT_SVM
  SvmInterceptException(BX_SOFTWARE_EXCEPTION, 3, 0, 0);
#endif

#if BX_DEBUGGER
  BX_CPU_THIS_PTR show_flag |= Flag_softint;
#endif

  // interrupt is not RSP safe
  interrupt(3, BX_SOFTWARE_EXCEPTION, 0, 0);

  BX_INSTR_FAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_INT,
                      FAR_BRANCH_PREV_CS, FAR_BRANCH_PREV_RIP,
                      BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value, RIP);

  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::INT_Ib(bxInstruction_c *i)
{
  Bit8u vector = i->Ib();

  BX_INSTR_FAR_BRANCH_ORIGIN();

#if BX_SUPPORT_VMX
  VMexit_Event(BX_SOFTWARE_INTERRUPT, vector, 0, 0);
#endif

#if BX_SUPPORT_SVM
  if (BX_CPU_THIS_PTR in_svm_guest) {
    if (SVM_INTERCEPT(SVM_INTERCEPT0_SOFTINT))
      Svm_Vmexit(SVM_VMEXIT_SOFTWARE_INTERRUPT, BX_SUPPORT_SVM_EXTENSION(BX_CPUID_SVM_DECODE_ASSIST) ? vector : 0);
  }
#endif

#ifdef SHOW_EXIT_STATUS
  if ((vector == 0x21) && (AH == 0x4c)) {
    BX_INFO(("INT 21/4C called AL=0x%02x, BX=0x%04x", (unsigned) AL, (unsigned) BX));
  }
#endif

#if BX_DEBUGGER
  BX_CPU_THIS_PTR show_flag |= Flag_softint;
#endif

  interrupt(vector, BX_SOFTWARE_INTERRUPT, 0, 0);

  BX_INSTR_FAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_INT,
                      FAR_BRANCH_PREV_CS, FAR_BRANCH_PREV_RIP,
                      BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value, RIP);

  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::INTO(bxInstruction_c *i)
{
  if (get_OF()) {
    BX_INSTR_FAR_BRANCH_ORIGIN();

#if BX_SUPPORT_VMX
    VMexit_Event(BX_SOFTWARE_EXCEPTION, 4, 0, 0);
#endif

#if BX_SUPPORT_SVM
    SvmInterceptException(BX_SOFTWARE_EXCEPTION, 4, 0, 0);
#endif

#if BX_DEBUGGER
    BX_CPU_THIS_PTR show_flag |= Flag_softint;
#endif

    // interrupt is not RSP safe
    interrupt(4, BX_SOFTWARE_EXCEPTION, 0, 0);

    BX_INSTR_FAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_INT,
                        FAR_BRANCH_PREV_CS, FAR_BRANCH_PREV_RIP,
                        BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value, RIP);
  }

  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::IRET16(bxInstruction_c *i)
{
  BX_INSTR_FAR_BRANCH_ORIGIN();

  invalidate_prefetch_q();

#if BX_SUPPORT_SVM
  if (BX_CPU_THIS_PTR in_svm_guest) {
    if (SVM_INTERCEPT(SVM_INTERCEPT0_IRET)) Svm_Vmexit(SVM_VMEXIT_IRET);
  }
#endif

#if BX_SUPPORT_VMX
  if (BX_CPU_THIS_PTR in_vmx_guest)
    if (is_masked_event(BX_CPU_THIS_PTR vmcs.pin_vmexec_ctrls.VIRTUAL_NMI() ? BX_EVENT_VMX_VIRTUAL_NMI : BX_EVENT_NMI))
      BX_CPU_THIS_PTR nmi_unblocking_iret = true;

  if (BX_CPU_THIS_PTR in_vmx_guest && BX_CPU_THIS_PTR vmcs.pin_vmexec_ctrls.NMI_EXITING()) {
    if (BX_CPU_THIS_PTR vmcs.pin_vmexec_ctrls.VIRTUAL_NMI()) unmask_event(BX_EVENT_VMX_VIRTUAL_NMI);
  }
  else
#endif
    unmask_event(BX_EVENT_NMI);

#if BX_DEBUGGER
  BX_CPU_THIS_PTR show_flag |= Flag_iret;
#endif

#if BX_SUPPORT_MONITOR_MWAIT
  BX_CPU_THIS_PTR monitor.reset_umonitor();
#endif

  RSP_SPECULATIVE;

  if (protected_mode()) {
    iret_protected(i);
  }
  else {
    if (v8086_mode()) {
      // IOPL check in stack_return_from_v86()
      iret16_stack_return_from_v86(i);
    }
    else {
      Bit16u ip     = pop_16();
      Bit16u cs_raw = pop_16(); // #SS has higher priority
      Bit16u flags  = pop_16();

      // CS.LIMIT can't change when in real/v8086 mode
      if(ip > BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled) {
        //BX_ERROR(("%s: instruction pointer not within code segment limits", i->getIaOpcodeNameShort()));
        exception(BX_GP_EXCEPTION, 0);
      }

      load_seg_reg(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS], cs_raw);
      EIP = (Bit32u) ip;
      write_flags(flags, /* change IOPL? */ 1, /* change IF? */ 1);
    }
  }

  RSP_COMMIT;

#if BX_SUPPORT_VMX
  BX_CPU_THIS_PTR nmi_unblocking_iret = false;
#endif

  BX_INSTR_FAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_IRET,
                      FAR_BRANCH_PREV_CS, FAR_BRANCH_PREV_RIP,
                      BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value, EIP);

  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::IRET32(bxInstruction_c *i)
{
  //BX_ASSERT(BX_CPU_THIS_PTR cpu_mode != BX_MODE_LONG_64);

  invalidate_prefetch_q();

  BX_INSTR_FAR_BRANCH_ORIGIN();

#if BX_SUPPORT_SVM
  if (BX_CPU_THIS_PTR in_svm_guest) {
    if (SVM_INTERCEPT(SVM_INTERCEPT0_IRET)) Svm_Vmexit(SVM_VMEXIT_IRET);
  }
#endif

#if BX_SUPPORT_VMX
  if (BX_CPU_THIS_PTR in_vmx_guest)
    if (is_masked_event(BX_CPU_THIS_PTR vmcs.pin_vmexec_ctrls.VIRTUAL_NMI() ? BX_EVENT_VMX_VIRTUAL_NMI : BX_EVENT_NMI))
      BX_CPU_THIS_PTR nmi_unblocking_iret = true;

  if (BX_CPU_THIS_PTR in_vmx_guest && BX_CPU_THIS_PTR vmcs.pin_vmexec_ctrls.NMI_EXITING()) {
    if (BX_CPU_THIS_PTR vmcs.pin_vmexec_ctrls.VIRTUAL_NMI()) unmask_event(BX_EVENT_VMX_VIRTUAL_NMI);
  }
  else
#endif
    unmask_event(BX_EVENT_NMI);

#if BX_DEBUGGER
  BX_CPU_THIS_PTR show_flag |= Flag_iret;
#endif

#if BX_SUPPORT_MONITOR_MWAIT
  BX_CPU_THIS_PTR monitor.reset_umonitor();
#endif

  RSP_SPECULATIVE;

  if (protected_mode()) {
    iret_protected(i);
  }
  else {
    if (v8086_mode()) {
      // IOPL check in stack_return_from_v86()
      iret32_stack_return_from_v86(i);
    }
    else {
      Bit32u eip      =          pop_32();
      Bit16u cs_raw   = (Bit16u) pop_32(); // #SS has higher priority
      Bit32u eflags32 =          pop_32();

      // CS.LIMIT can't change when in real/v8086 mode
      if (eip > BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled) {
        //BX_ERROR(("%s: instruction pointer not within code segment limits", i->getIaOpcodeNameShort()));
        exception(BX_GP_EXCEPTION, 0);
      }

      load_seg_reg(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS], cs_raw);
      EIP = eip;
      writeEFlags(eflags32, 0x00257fd5); // VIF, VIP, VM unchanged
    }
  }

  RSP_COMMIT;

#if BX_SUPPORT_VMX
  BX_CPU_THIS_PTR nmi_unblocking_iret = false;
#endif

  BX_INSTR_FAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_IRET,
                      FAR_BRANCH_PREV_CS, FAR_BRANCH_PREV_RIP,
                      BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value, EIP);

  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::RETfar16_Iw(bxInstruction_c *i)
{
  BX_INSTR_FAR_BRANCH_ORIGIN();

  invalidate_prefetch_q();

#if BX_DEBUGGER
  BX_CPU_THIS_PTR show_flag |= Flag_ret;
#endif

  Bit16s imm16 = (Bit16s) i->Iw();

  RSP_SPECULATIVE;

  if (protected_mode()) {
    return_protected(i, imm16);
  }
  else {
    Bit16u ip     = pop_16();
    Bit16u cs_raw = pop_16();

    // CS.LIMIT can't change when in real/v8086 mode
    if (ip > BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled) {
      //BX_ERROR(("%s: instruction pointer not within code segment limits", i->getIaOpcodeNameShort()));
      exception(BX_GP_EXCEPTION, 0);
    }

    load_seg_reg(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS], cs_raw);
    EIP = (Bit32u) ip;

    if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.d_b)
      ESP += imm16;
    else
       SP += imm16;
  }

  RSP_COMMIT;

  BX_INSTR_FAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_RET,
                      FAR_BRANCH_PREV_CS, FAR_BRANCH_PREV_RIP,
                      BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value, RIP);

  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::RETfar32_Iw(bxInstruction_c *i)
{
  invalidate_prefetch_q();

  BX_INSTR_FAR_BRANCH_ORIGIN();

#if BX_DEBUGGER
  BX_CPU_THIS_PTR show_flag |= Flag_ret;
#endif

  Bit16u imm16 = i->Iw();

  RSP_SPECULATIVE;

  if (protected_mode()) {
    return_protected(i, imm16);
  }
  else {
    Bit32u eip    =          pop_32();
    Bit16u cs_raw = (Bit16u) pop_32(); /* 32bit pop, MSW discarded */

    // CS.LIMIT can't change when in real/v8086 mode
    if (eip > BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled) {
      //BX_ERROR(("%s: instruction pointer not within code segment limits", i->getIaOpcodeNameShort()));
      exception(BX_GP_EXCEPTION, 0);
    }

    load_seg_reg(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS], cs_raw);
    EIP = eip;

    if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.d_b)
      ESP += imm16;
    else
       SP += imm16;
  }

  RSP_COMMIT;

  BX_INSTR_FAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_RET,
                      FAR_BRANCH_PREV_CS, FAR_BRANCH_PREV_RIP,
                      BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value, RIP);

  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CLC(bxInstruction_c *i)
{
  clear_CF();

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::STC(bxInstruction_c *i)
{
  assert_CF();

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMC(bxInstruction_c *i)
{
  set_CF(! get_CF());

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::STD(bxInstruction_c *i)
{
  BX_CPU_THIS_PTR assert_DF();

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::STI(bxInstruction_c *i)
{
  Bit32u IOPL = BX_CPU_THIS_PTR get_IOPL();

  if (protected_mode())
  {
#if BX_CPU_LEVEL >= 5
    if (BX_CPU_THIS_PTR cr4.get_PVI())
    {
      if (CPL == 3 && IOPL < 3) {
        if (! BX_CPU_THIS_PTR get_VIP())
        {
          BX_CPU_THIS_PTR assert_VIF();
          BX_NEXT_INSTR(i);
        }

        //BX_DEBUG(("STI: #GP(0) in VME mode"));
        exception(BX_GP_EXCEPTION, 0);
      }
    }
#endif
    if (CPL > IOPL) {
      //BX_DEBUG(("STI: CPL > IOPL in protected mode"));
      exception(BX_GP_EXCEPTION, 0);
    }
  }
  else if (v8086_mode())
  {
    if (IOPL != 3) {
#if BX_CPU_LEVEL >= 5
      if (BX_CPU_THIS_PTR cr4.get_VME() && BX_CPU_THIS_PTR get_VIP() == 0)
      {
        BX_CPU_THIS_PTR assert_VIF();
        BX_NEXT_INSTR(i);
      }
#endif
      //BX_DEBUG(("STI: IOPL != 3 in v8086 mode"));
      exception(BX_GP_EXCEPTION, 0);
    }
  }

  if (! BX_CPU_THIS_PTR get_IF()) {
    BX_CPU_THIS_PTR assert_IF();
    inhibit_interrupts(BX_INHIBIT_INTERRUPTS);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LAHF(bxInstruction_c *i)
{
  AH = read_eflags() & 0xFF;

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SAHF(bxInstruction_c *i)
{
  set_SF((AH & 0x80) >> 7);
  set_ZF((AH & 0x40) >> 6);
  set_AF((AH & 0x10) >> 4);
  set_CF (AH & 0x01);
  set_PF((AH & 0x04) >> 2);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::HLT(bxInstruction_c *i)
{
  // CPL is always 0 in real mode
  if (/* !real_mode() && */ CPL!=0) {
    //BX_DEBUG(("HLT: %s priveledge check failed, CPL=%d, generate #GP(0)",
        //cpu_mode_string(BX_CPU_THIS_PTR cpu_mode), CPL));
    exception(BX_GP_EXCEPTION, 0);
  }

  if (! BX_CPU_THIS_PTR get_IF()) {
    //BX_INFO(("WARNING: HLT instruction with IF=0!"));
  }

#if BX_SUPPORT_VMX
  if (BX_CPU_THIS_PTR in_vmx_guest) {
    if (BX_CPU_THIS_PTR vmcs.vmexec_ctrls1.HLT_VMEXIT()) {
      VMexit(VMX_VMEXIT_HLT, 0);
    }
  }
#endif

#if BX_SUPPORT_SVM
  if (BX_CPU_THIS_PTR in_svm_guest) {
    if (SVM_INTERCEPT(SVM_INTERCEPT0_HLT)) Svm_Vmexit(SVM_VMEXIT_HLT);
  }
#endif

  // stops instruction execution and places the processor in a
  // HALT state. An enabled interrupt, NMI, or reset will resume
  // execution. If interrupt (including NMI) is used to resume
  // execution after HLT, the saved CS:eIP points to instruction
  // following HLT.
  enter_sleep_state(BX_ACTIVITY_STATE_HLT);

  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CLTS(bxInstruction_c *i)
{
  // CPL is always 0 in real mode
  if (/* !real_mode() && */ CPL!=0) {
    //BX_ERROR(("%s: priveledge check failed, generate #GP(0)", i->getIaOpcodeNameShort()));
    exception(BX_GP_EXCEPTION, 0);
  }

#if BX_SUPPORT_VMX
  if (BX_CPU_THIS_PTR in_vmx_guest) {
    if(VMexit_CLTS()) {
      BX_NEXT_TRACE(i);
    }
  }
#endif

#if BX_SUPPORT_SVM
  if (BX_CPU_THIS_PTR in_svm_guest) {
    if(SVM_CR_WRITE_INTERCEPTED(0)) Svm_Vmexit(SVM_VMEXIT_CR0_WRITE);
  }
#endif

  BX_CPU_THIS_PTR cr0.set_TS(0);

  handleFpuMmxModeChange();
#if BX_CPU_LEVEL >= 6
  handleSseModeChange();
#if BX_SUPPORT_AVX
  handleAvxModeChange();
#endif
#endif

  BX_NEXT_TRACE(i);
}

#if BX_SUPPORT_SVM
const Bit64u BX_SVM_CR_WRITE_MASK = (BX_CONST64(1) << 63);
#endif

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_CR3Rd(bxInstruction_c *i)
{
  // CPL is always 0 in real mode
  if (/* !real_mode() && */ CPL!=0) {
    //BX_ERROR(("%s: CPL!=0 not in real mode", i->getIaOpcodeNameShort()));
    exception(BX_GP_EXCEPTION, 0);
  }

  invalidate_prefetch_q();

  Bit32u val_32 = BX_READ_32BIT_REG(i->src());

#if BX_SUPPORT_VMX
  if (BX_CPU_THIS_PTR in_vmx_guest)
    VMexit_CR3_Write(i, val_32);
#endif

#if BX_SUPPORT_SVM
  if (BX_CPU_THIS_PTR in_svm_guest) {
    if(SVM_CR_WRITE_INTERCEPTED(3)) {
      if (BX_SUPPORT_SVM_EXTENSION(BX_CPUID_SVM_DECODE_ASSIST))
        Svm_Vmexit(SVM_VMEXIT_CR3_WRITE, BX_SVM_CR_WRITE_MASK | i->src());
      else
        Svm_Vmexit(SVM_VMEXIT_CR3_WRITE);
    }
  }
#endif

#if BX_CPU_LEVEL >= 6
  if (BX_CPU_THIS_PTR cr0.get_PG() && BX_CPU_THIS_PTR cr4.get_PAE() && !long_mode()) {
    if (! CheckPDPTR(val_32)) {
      //BX_ERROR(("%s: PDPTR check failed !", i->getIaOpcodeNameShort()));
      exception(BX_GP_EXCEPTION, 0);
    }
  }
#endif

  if (! SetCR3(val_32))
    exception(BX_GP_EXCEPTION, 0);

  BX_INSTR_TLB_CNTRL(BX_CPU_ID, BX_INSTR_MOV_CR3, val_32);

  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_RdCR3(bxInstruction_c *i)
{
  // CPL is always 0 in real mode
  if (/* !real_mode() && */ CPL!=0) {
    //BX_ERROR(("%s: CPL!=0 not in real mode", i->getIaOpcodeNameShort()));
    exception(BX_GP_EXCEPTION, 0);
  }

#if BX_SUPPORT_SVM
  if (BX_CPU_THIS_PTR in_svm_guest) {
    if(SVM_CR_READ_INTERCEPTED(3))
      Svm_Vmexit(SVM_VMEXIT_CR3_READ, BX_SUPPORT_SVM_EXTENSION(BX_CPUID_SVM_DECODE_ASSIST) ? i->dst() : 0);
  }
#endif

#if BX_SUPPORT_VMX
  if (BX_CPU_THIS_PTR in_vmx_guest)
    VMexit_CR3_Read(i);
#endif

  Bit32u val_32 = (Bit32u) BX_CPU_THIS_PTR cr3;

  BX_WRITE_32BIT_REGZ(i->dst(), val_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_CR4Rd(bxInstruction_c *i)
{
#if BX_CPU_LEVEL >= 5
  // CPL is always 0 in real mode
  if (/* !real_mode() && */ CPL!=0) {
    //BX_ERROR(("%s: CPL!=0 not in real mode", i->getIaOpcodeNameShort()));
    exception(BX_GP_EXCEPTION, 0);
  }

  invalidate_prefetch_q();

  Bit32u val_32 = BX_READ_32BIT_REG(i->src());
#if BX_SUPPORT_VMX
  if (BX_CPU_THIS_PTR in_vmx_guest)
    val_32 = (Bit32u) VMexit_CR4_Write(i, val_32);
#endif
  if (! SetCR4(i, val_32))
    exception(BX_GP_EXCEPTION, 0);

  BX_INSTR_TLB_CNTRL(BX_CPU_ID, BX_INSTR_MOV_CR4, val_32);
#endif

  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_RdCR4(bxInstruction_c *i)
{
#if BX_CPU_LEVEL >= 5
  // CPL is always 0 in real mode
  if (/* !real_mode() && */ CPL!=0) {
    //BX_ERROR(("%s: CPL!=0 not in real mode", i->getIaOpcodeNameShort()));
    exception(BX_GP_EXCEPTION, 0);
  }

#if BX_SUPPORT_SVM
  if (BX_CPU_THIS_PTR in_svm_guest) {
    if(SVM_CR_READ_INTERCEPTED(4))
      Svm_Vmexit(SVM_VMEXIT_CR4_READ, BX_SUPPORT_SVM_EXTENSION(BX_CPUID_SVM_DECODE_ASSIST) ? i->dst() : 0);
  }
#endif

  Bit32u val_32 = (Bit32u) read_CR4(); /* correctly handle VMX */

  BX_WRITE_32BIT_REGZ(i->dst(), val_32);
#endif

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SGDT_Ms(bxInstruction_c *i)
{
  //BX_ASSERT(BX_CPU_THIS_PTR cpu_mode != BX_MODE_LONG_64);

#if BX_CPU_LEVEL >= 5
  if (CPL!=0 && BX_CPU_THIS_PTR cr4.get_UMIP()) {
    //BX_ERROR(("SGDT: CPL != 0 causes #GP when CR4.UMIP set"));
    exception(BX_GP_EXCEPTION, 0);
  }
#endif

#if BX_SUPPORT_VMX >= 2
  if (BX_CPU_THIS_PTR in_vmx_guest)
    if (BX_CPU_THIS_PTR vmcs.vmexec_ctrls2.DESCRIPTOR_TABLE_VMEXIT())
      VMexit_Instruction(i, VMX_VMEXIT_GDTR_IDTR_ACCESS, BX_READ);
#endif

#if BX_SUPPORT_SVM
  if (BX_CPU_THIS_PTR in_svm_guest) {
    if (SVM_INTERCEPT(SVM_INTERCEPT0_GDTR_READ)) Svm_Vmexit(SVM_VMEXIT_GDTR_READ);
  }
#endif

  Bit16u limit_16 = BX_CPU_THIS_PTR gdtr.limit;
  Bit32u base_32  = (Bit32u) BX_CPU_THIS_PTR gdtr.base;

  Bit32u eaddr = (Bit32u) BX_CPU_RESOLVE_ADDR_32(i);

  write_virtual_word_32(i->seg(), eaddr, limit_16);
  write_virtual_dword_32(i->seg(), (eaddr+2) & i->asize_mask(), base_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SIDT_Ms(bxInstruction_c *i)
{
#if BX_CPU_LEVEL >= 5
  if (CPL!=0 && BX_CPU_THIS_PTR cr4.get_UMIP()) {
    //BX_ERROR(("SIDT: CPL != 0 causes #GP when CR4.UMIP set"));
    exception(BX_GP_EXCEPTION, 0);
  }
#endif

 // BX_ASSERT(BX_CPU_THIS_PTR cpu_mode != BX_MODE_LONG_64);

#if BX_SUPPORT_VMX >= 2
  if (BX_CPU_THIS_PTR in_vmx_guest)
    if (BX_CPU_THIS_PTR vmcs.vmexec_ctrls2.DESCRIPTOR_TABLE_VMEXIT())
      VMexit_Instruction(i, VMX_VMEXIT_GDTR_IDTR_ACCESS, BX_READ);
#endif

#if BX_SUPPORT_SVM
  if (BX_CPU_THIS_PTR in_svm_guest) {
    if (SVM_INTERCEPT(SVM_INTERCEPT0_IDTR_READ)) Svm_Vmexit(SVM_VMEXIT_IDTR_READ);
  }
#endif

  Bit16u limit_16 = BX_CPU_THIS_PTR idtr.limit;
  Bit32u base_32  = (Bit32u) BX_CPU_THIS_PTR idtr.base;

  Bit32u eaddr = (Bit32u) BX_CPU_RESOLVE_ADDR_32(i);

  write_virtual_word_32(i->seg(), eaddr, limit_16);
  write_virtual_dword_32(i->seg(), (eaddr+2) & i->asize_mask(), base_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LMSW_Ew(bxInstruction_c *i)
{
  Bit16u msw;

  // CPL is always 0 in real mode
  if (/* !real_mode() && */ CPL!=0) {
    //BX_ERROR(("%s: CPL!=0 not in real mode", i->getIaOpcodeNameShort()));
    exception(BX_GP_EXCEPTION, 0);
  }

#if BX_SUPPORT_SVM
  if (BX_CPU_THIS_PTR in_svm_guest) {
    if(SVM_CR_WRITE_INTERCEPTED(0)) Svm_Vmexit(SVM_VMEXIT_CR0_WRITE);
  }
#endif

  if (i->modC0()) {
    msw = BX_READ_16BIT_REG(i->src());
  }
  else {
    /* use RMAddr(i) to save address for VMexit */
    RMAddr(i) = BX_CPU_RESOLVE_ADDR(i);
    /* pointer, segment address pair */
    msw = read_virtual_word(i->seg(), RMAddr(i));
  }

  // LMSW does not affect PG,CD,NW,AM,WP,NE,ET bits, and cannot clear PE

#if BX_SUPPORT_VMX
  if (BX_CPU_THIS_PTR in_vmx_guest)
    msw = VMexit_LMSW(i, msw);
#endif

  // LMSW cannot clear PE
  if (BX_CPU_THIS_PTR cr0.get_PE())
    msw |= BX_CR0_PE_MASK; // adjust PE bit to current value of 1

  msw &= 0xf; // LMSW only affects last 4 flags

  Bit32u cr0 = (BX_CPU_THIS_PTR cr0.get32() & 0xfffffff0) | msw;
  if (! SetCR0(i, cr0))
    exception(BX_GP_EXCEPTION, 0);

  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SMSW_EwR(bxInstruction_c *i)
{
#if BX_CPU_LEVEL >= 5
  if (CPL!=0 && BX_CPU_THIS_PTR cr4.get_UMIP()) {
    //BX_ERROR(("SMSW: CPL != 0 causes #GP when CR4.UMIP set"));
    exception(BX_GP_EXCEPTION, 0);
  }
#endif

  Bit32u msw = (Bit32u) read_CR0();  // handle CR0 shadow in VMX

  if (i->os32L()) {
    BX_WRITE_32BIT_REGZ(i->dst(), msw);
  }
  else {
    BX_WRITE_16BIT_REG(i->dst(), msw & 0xffff);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SMSW_EwM(bxInstruction_c *i)
{
#if BX_CPU_LEVEL >= 5
  if (CPL!=0 && BX_CPU_THIS_PTR cr4.get_UMIP()) {
    //BX_ERROR(("SMSW: CPL != 0 causes #GP when CR4.UMIP set"));
    exception(BX_GP_EXCEPTION, 0);
  }
#endif

  Bit16u msw = read_CR0() & 0xffff;   // handle CR0 shadow in VMX
  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);
  write_virtual_word(i->seg(), eaddr, msw);

  BX_NEXT_INSTR(i);
}



void BX_CPP_AttrRegparmN(1) BX_CPU_C::TEST_EbGbM(bxInstruction_c* i)
{
    Bit8u op1, op2;

    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    op1 = read_virtual_byte(i->seg(), eaddr);
    op2 = BX_READ_8BIT_REGx(i->src(), i->extend8bitL());
    op1 &= op2;

    SET_FLAGS_OSZAPC_LOGIC_8(op1);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::TEST_EwGwM(bxInstruction_c* i)
{
    Bit16u op1_16, op2_16;

    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    op1_16 = read_virtual_word(i->seg(), eaddr);
    op2_16 = BX_READ_16BIT_REG(i->src());
    op1_16 &= op2_16;
    SET_FLAGS_OSZAPC_LOGIC_16(op1_16);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::TEST_EdGdM(bxInstruction_c* i)
{
    Bit32u op1_32, op2_32;

    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    op1_32 = read_virtual_dword(i->seg(), eaddr);
    op2_32 = BX_READ_32BIT_REG(i->src());
    op1_32 &= op2_32;

    SET_FLAGS_OSZAPC_LOGIC_32(op1_32);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::XCHG_EbGbR(bxInstruction_c* i)
{
    Bit8u op1 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());
    Bit8u op2 = BX_READ_8BIT_REGx(i->src(), i->extend8bitL());

    BX_WRITE_8BIT_REGx(i->src(), i->extend8bitL(), op1);
    BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), op2);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::XCHG_EwGwR(bxInstruction_c* i)
{
    Bit16u op1_16 = BX_READ_16BIT_REG(i->dst());
    Bit16u op2_16 = BX_READ_16BIT_REG(i->src());

    BX_WRITE_16BIT_REG(i->src(), op1_16);
    BX_WRITE_16BIT_REG(i->dst(), op2_16);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::XCHG_EdGdR(bxInstruction_c* i)
{
    Bit32u op1_32 = BX_READ_32BIT_REG(i->dst());
    Bit32u op2_32 = BX_READ_32BIT_REG(i->src());

    BX_WRITE_32BIT_REGZ(i->src(), op1_32);
    BX_WRITE_32BIT_REGZ(i->dst(), op2_32);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::XCHG_EbGbM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    Bit8u op1 = read_RMW_virtual_byte(i->seg(), eaddr);
    Bit8u op2 = BX_READ_8BIT_REGx(i->src(), i->extend8bitL());

    write_RMW_linear_byte(op2);
    BX_WRITE_8BIT_REGx(i->src(), i->extend8bitL(), op1);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::XCHG_EwGwM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);
    Bit16u op1_16 = read_RMW_virtual_word(i->seg(), eaddr);
    write_RMW_linear_word(BX_READ_16BIT_REG(i->src()));
    BX_WRITE_16BIT_REG(i->src(), op1_16);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::XCHG_EdGdM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);
    Bit32u op1_32 = read_RMW_virtual_dword(i->seg(), eaddr);
    write_RMW_linear_dword(BX_READ_32BIT_REG(i->src()));
    BX_WRITE_32BIT_REGZ(i->src(), op1_32);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CBW(bxInstruction_c* i)
{
    AX = (Bit8s) AL;

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CWD(bxInstruction_c* i)
{
    if (AX & 0x8000) {
        DX = 0xFFFF;
    }
    else {
        DX = 0x0000;
    }

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::NOT_EbM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    Bit8u op1_8 = read_RMW_virtual_byte(i->seg(), eaddr);
    op1_8 = ~op1_8;
    write_RMW_linear_byte(op1_8);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::NOT_EwM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    Bit16u op1_16 = read_RMW_virtual_word(i->seg(), eaddr);
    op1_16 = ~op1_16;
    write_RMW_linear_word(op1_16);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::NOT_EdM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    Bit32u op1_32 = read_RMW_virtual_dword(i->seg(), eaddr);
    op1_32 = ~op1_32;
    write_RMW_linear_dword(op1_32);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::NOT_EbR(bxInstruction_c* i)
{
    Bit8u op1_8 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());
    op1_8 = ~op1_8;
    BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), op1_8);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::NOT_EwR(bxInstruction_c* i)
{
    Bit16u op1_16 = BX_READ_16BIT_REG(i->dst());
    op1_16 = ~op1_16;
    BX_WRITE_16BIT_REG(i->dst(), op1_16);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::NEG_EbM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    Bit32u op1_8 = read_RMW_virtual_byte(i->seg(), eaddr);
    op1_8 = 0 - op1_8;
    write_RMW_linear_byte(op1_8);

    SET_FLAGS_OSZAPC_SUB_8(0, 0 - op1_8, op1_8);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::NEG_EwM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    Bit32u op1_16 = read_RMW_virtual_word(i->seg(), eaddr);
    op1_16 = 0 - op1_16;
    write_RMW_linear_word(op1_16);

    SET_FLAGS_OSZAPC_SUB_16(0, 0 - op1_16, op1_16);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::NEG_EdM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    Bit32u op1_32 = read_RMW_virtual_dword(i->seg(), eaddr);
    op1_32 = 0 - op1_32;
    write_RMW_linear_dword(op1_32);

    SET_FLAGS_OSZAPC_SUB_32(0, 0 - op1_32, op1_32);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::NEG_EbR(bxInstruction_c* i)
{
    Bit32u op1_8 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());
    op1_8 = 0 - op1_8;
    BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), op1_8);

    SET_FLAGS_OSZAPC_SUB_8(0, 0 - op1_8, op1_8);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::NEG_EwR(bxInstruction_c* i)
{
    Bit32u op1_16 = BX_READ_16BIT_REG(i->dst());
    op1_16 = 0 - op1_16;
    BX_WRITE_16BIT_REG(i->dst(), op1_16);

    SET_FLAGS_OSZAPC_SUB_16(0, 0 - op1_16, op1_16);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::TEST_EwIwR(bxInstruction_c* i)
{
    Bit16u op1_16 = BX_READ_16BIT_REG(i->dst());
    op1_16 &= i->Iw();
    SET_FLAGS_OSZAPC_LOGIC_16(op1_16);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::TEST_EdIdR(bxInstruction_c* i)
{
    Bit32u op1_32 = BX_READ_32BIT_REG(i->dst());
    op1_32 &= i->Id();
    SET_FLAGS_OSZAPC_LOGIC_32(op1_32);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::TEST_EwIwM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    Bit16u op1_16 = read_virtual_word(i->seg(), eaddr);
    op1_16 &= i->Iw();
    SET_FLAGS_OSZAPC_LOGIC_16(op1_16);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::TEST_EdIdM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    Bit32u op1_32 = read_virtual_dword(i->seg(), eaddr);
    op1_32 &= i->Id();
    SET_FLAGS_OSZAPC_LOGIC_32(op1_32);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::INC_EbR(bxInstruction_c* i)
{
    Bit32u op1_8 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());
    op1_8++;
    BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), op1_8);

    SET_FLAGS_OSZAP_ADD_8(op1_8 - 1, 0, op1_8);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::DEC_EbR(bxInstruction_c* i)
{
    Bit32u op1_8 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());
    op1_8--;
    BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), op1_8);

    SET_FLAGS_OSZAP_SUB_8(op1_8 + 1, 0, op1_8);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::INC_EbM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    Bit32u op1_8 = read_RMW_virtual_byte(i->seg(), eaddr);
    op1_8++;
    write_RMW_linear_byte(op1_8);

    SET_FLAGS_OSZAP_ADD_8(op1_8 - 1, 0, op1_8);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::INC_EwM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    Bit32u op1_16 = read_RMW_virtual_word(i->seg(), eaddr);
    op1_16++;
    write_RMW_linear_word(op1_16);

    SET_FLAGS_OSZAP_ADD_16(op1_16 - 1, 0, op1_16);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::INC_EdM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    Bit32u op1_32 = read_RMW_virtual_dword(i->seg(), eaddr);
    op1_32++;
    write_RMW_linear_dword(op1_32);

    SET_FLAGS_OSZAP_ADD_32(op1_32 - 1, 0, op1_32);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::DEC_EbM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    Bit32u op1_8 = read_RMW_virtual_byte(i->seg(), eaddr);
    op1_8--;
    write_RMW_linear_byte(op1_8);

    SET_FLAGS_OSZAP_SUB_8(op1_8 + 1, 0, op1_8);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::DEC_EwM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    Bit32u op1_16 = read_RMW_virtual_word(i->seg(), eaddr);
    op1_16--;
    write_RMW_linear_word(op1_16);

    SET_FLAGS_OSZAP_SUB_16(op1_16 + 1, 0, op1_16);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::DEC_EdM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    Bit32u op1_32 = read_RMW_virtual_dword(i->seg(), eaddr);
    op1_32--;
    write_RMW_linear_dword(op1_32);

    SET_FLAGS_OSZAP_SUB_32(op1_32 + 1, 0, op1_32);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CWDE(bxInstruction_c* i)
{
    Bit32u tmp = (Bit16s) AX;
    RAX = tmp;

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CDQ(bxInstruction_c* i)
{
    if (EAX & 0x80000000) {
        RDX = 0xFFFFFFFF;
    }
    else {
        RDX = 0x00000000;
    }

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::TEST_EqGqR(bxInstruction_c* i)
{
    Bit64u op1_64, op2_64;

    op1_64 = BX_READ_64BIT_REG(i->dst());
    op2_64 = BX_READ_64BIT_REG(i->src());
    op1_64 &= op2_64;

    SET_FLAGS_OSZAPC_LOGIC_64(op1_64);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::TEST_EqGqM(bxInstruction_c* i)
{
    Bit64u op1_64, op2_64;

    bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

    op1_64 = read_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));
    op2_64 = BX_READ_64BIT_REG(i->src());
    op1_64 &= op2_64;

    SET_FLAGS_OSZAPC_LOGIC_64(op1_64);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::TEST_RAXId(bxInstruction_c* i)
{
    Bit64u op1_64 = RAX;
    op1_64 &= (Bit32s) i->Id();

    SET_FLAGS_OSZAPC_LOGIC_64(op1_64);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::XCHG_EqGqR(bxInstruction_c* i)
{
    Bit64u op1_64 = BX_READ_64BIT_REG(i->dst());
    Bit64u op2_64 = BX_READ_64BIT_REG(i->src());

    BX_WRITE_64BIT_REG(i->src(), op1_64);
    BX_WRITE_64BIT_REG(i->dst(), op2_64);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::XCHG_EqGqM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);
    Bit64u op1_64 = read_RMW_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));
    write_RMW_linear_qword(BX_READ_64BIT_REG(i->src()));
    BX_WRITE_64BIT_REG(i->src(), op1_64);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::NOT_EqM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

    Bit64u op1_64 = read_RMW_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));
    op1_64 = ~op1_64;
    write_RMW_linear_qword(op1_64);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::NEG_EqM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

    Bit64u op1_64 = read_RMW_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));
    op1_64 = 0 - op1_64;
    write_RMW_linear_qword(op1_64);

    SET_FLAGS_OSZAPC_SUB_64(0, 0 - op1_64, op1_64);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::NOT_EqR(bxInstruction_c* i)
{
    Bit64u op1_64 = BX_READ_64BIT_REG(i->dst());
    op1_64 = ~op1_64;
    BX_WRITE_64BIT_REG(i->dst(), op1_64);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::NEG_EqR(bxInstruction_c* i)
{
    Bit64u op1_64 = BX_READ_64BIT_REG(i->dst());
    op1_64 = 0 - op1_64;
    BX_WRITE_64BIT_REG(i->dst(), op1_64);

    SET_FLAGS_OSZAPC_SUB_64(0, 0 - op1_64, op1_64);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::TEST_EqIdR(bxInstruction_c* i)
{
    Bit64u op1_64, op2_64;

    op1_64 = BX_READ_64BIT_REG(i->dst());
    op2_64 = (Bit32s) i->Id();
    op1_64 &= op2_64;

    SET_FLAGS_OSZAPC_LOGIC_64(op1_64);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::TEST_EqIdM(bxInstruction_c* i)
{
    Bit64u op1_64, op2_64;

    bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

    op1_64 = read_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));
    op2_64 = (Bit32s) i->Id();
    op1_64 &= op2_64;

    SET_FLAGS_OSZAPC_LOGIC_64(op1_64);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::INC_EqM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

    Bit64u op1_64 = read_RMW_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));
    op1_64++;
    write_RMW_linear_qword(op1_64);

    SET_FLAGS_OSZAP_ADD_64(op1_64 - 1, 0, op1_64);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::DEC_EqM(bxInstruction_c* i)
{
    bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

    Bit64u op1_64 = read_RMW_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));
    op1_64--;
    write_RMW_linear_qword(op1_64);

    SET_FLAGS_OSZAP_SUB_64(op1_64 + 1, 0, op1_64);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::INC_EqR(bxInstruction_c* i)
{
    Bit64u rrx = ++BX_READ_64BIT_REG(i->dst());
    SET_FLAGS_OSZAP_ADD_64(rrx - 1, 0, rrx);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::DEC_EqR(bxInstruction_c* i)
{
    Bit64u rrx = --BX_READ_64BIT_REG(i->dst());
    SET_FLAGS_OSZAP_SUB_64(rrx + 1, 0, rrx);

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CDQE(bxInstruction_c* i)
{
    RAX = (Bit32s) EAX;

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CQO(bxInstruction_c* i)
{
    if (RAX & BX_CONST64(0x8000000000000000))
        RDX = BX_CONST64(0xffffffffffffffff);
    else
        RDX = 0;

    BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADC_GbEbR(bxInstruction_c *i)
{
  Bit32u op1 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());
  Bit32u op2 = BX_READ_8BIT_REGx(i->src(), i->extend8bitL());
  Bit32u sum = op1 + op2 + getB_CF();

  BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), sum);

  SET_FLAGS_OSZAPC_ADD_8(op1, op2, sum);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SBB_GbEbR(bxInstruction_c *i)
{
  Bit32u op1_8 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());
  Bit32u op2_8 = BX_READ_8BIT_REGx(i->src(), i->extend8bitL());
  Bit32u diff_8 = op1_8 - (op2_8 + getB_CF());

  BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), diff_8);

  SET_FLAGS_OSZAPC_SUB_8(op1_8, op2_8, diff_8);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::AND_GbEbR(bxInstruction_c *i)
{
  Bit8u op1, op2;

  op1 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());
  op2 = BX_READ_8BIT_REGx(i->src(), i->extend8bitL());
  op1 &= op2;
  BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), op1);

  SET_FLAGS_OSZAPC_LOGIC_8(op1);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SUB_GbEbR(bxInstruction_c *i)
{
  Bit32u op1_8 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());
  Bit32u op2_8 = BX_READ_8BIT_REGx(i->src(), i->extend8bitL());
  Bit32u diff_8 = op1_8 - op2_8;

  BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), diff_8);

  SET_FLAGS_OSZAPC_SUB_8(op1_8, op2_8, diff_8);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMP_GbEbR(bxInstruction_c *i)
{
  Bit32u op1_8 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());
  Bit32u op2_8 = BX_READ_8BIT_REGx(i->src(), i->extend8bitL());
  Bit32u diff_8 = op1_8 - op2_8;

  SET_FLAGS_OSZAPC_SUB_8(op1_8, op2_8, diff_8);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADD_GbEbM(bxInstruction_c *i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());
  Bit32u op2 = read_virtual_byte(i->seg(), eaddr);
  Bit32u sum = op1 + op2;

  BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), sum);

  SET_FLAGS_OSZAPC_ADD_8(op1, op2, sum);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADC_GbEbM(bxInstruction_c *i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());
  Bit32u op2 = read_virtual_byte(i->seg(), eaddr);
  Bit32u sum = op1 + op2 + getB_CF();

  BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), sum);

  SET_FLAGS_OSZAPC_ADD_8(op1, op2, sum);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SBB_GbEbM(bxInstruction_c *i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_8 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());
  Bit32u op2_8 = read_virtual_byte(i->seg(), eaddr);
  Bit32u diff_8 = op1_8 - (op2_8 + getB_CF());

  BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), diff_8);

  SET_FLAGS_OSZAPC_SUB_8(op1_8, op2_8, diff_8);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::AND_GbEbM(bxInstruction_c *i)
{
  Bit8u op1, op2;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op1 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());
  op2 = read_virtual_byte(i->seg(), eaddr);
  op1 &= op2;
  BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), op1);

  SET_FLAGS_OSZAPC_LOGIC_8(op1);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SUB_GbEbM(bxInstruction_c *i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_8 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());
  Bit32u op2_8 = read_virtual_byte(i->seg(), eaddr);
  Bit32u diff_8 = op1_8 - op2_8;

  BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), diff_8);

  SET_FLAGS_OSZAPC_SUB_8(op1_8, op2_8, diff_8);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::XOR_GbEbM(bxInstruction_c *i)
{
  Bit8u op1, op2;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op1 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());
  op2 = read_virtual_byte(i->seg(), eaddr);
  op1 ^= op2;
  BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), op1);

  SET_FLAGS_OSZAPC_LOGIC_8(op1);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMP_GbEbM(bxInstruction_c *i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_8 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());
  Bit32u op2_8 = read_virtual_byte(i->seg(), eaddr);
  Bit32u diff_8 = op1_8 - op2_8;

  SET_FLAGS_OSZAPC_SUB_8(op1_8, op2_8, diff_8);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADD_EbIbM(bxInstruction_c *i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1 = read_RMW_virtual_byte(i->seg(), eaddr);
  Bit32u op2 = i->Ib();
  Bit32u sum = op1 + op2;

  write_RMW_linear_byte(sum);

  SET_FLAGS_OSZAPC_ADD_8(op1, op2, sum);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADC_EbIbM(bxInstruction_c *i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1 = read_RMW_virtual_byte(i->seg(), eaddr);
  Bit32u op2 = i->Ib();
  Bit32u sum = op1 + op2 + getB_CF();

  write_RMW_linear_byte(sum);

  SET_FLAGS_OSZAPC_ADD_8(op1, op2, sum);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SBB_EbIbM(bxInstruction_c *i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_8 = read_RMW_virtual_byte(i->seg(), eaddr);
  Bit32u op2_8 = i->Ib();
  Bit32u diff_8 = op1_8 - (op2_8 + getB_CF());
  write_RMW_linear_byte(diff_8);

  SET_FLAGS_OSZAPC_SUB_8(op1_8, op2_8, diff_8);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::AND_EbIbM(bxInstruction_c *i)
{
  Bit8u op1, op2 = i->Ib();

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op1 = read_RMW_virtual_byte(i->seg(), eaddr);
  op1 &= op2;
  write_RMW_linear_byte(op1);

  SET_FLAGS_OSZAPC_LOGIC_8(op1);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SUB_EbIbM(bxInstruction_c *i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_8 = read_RMW_virtual_byte(i->seg(), eaddr);
  Bit32u op2_8 = i->Ib();
  Bit32u diff_8 = op1_8 - op2_8;

  write_RMW_linear_byte(diff_8);

  SET_FLAGS_OSZAPC_SUB_8(op1_8, op2_8, diff_8);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::XOR_EbIbM(bxInstruction_c *i)
{
  Bit8u op1, op2 = i->Ib();

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op1 = read_RMW_virtual_byte(i->seg(), eaddr);
  op1 ^= op2;
  write_RMW_linear_byte(op1);

  SET_FLAGS_OSZAPC_LOGIC_8(op1);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADD_EbIbR(bxInstruction_c *i)
{
  Bit32u op1 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());
  Bit32u op2 = i->Ib();
  Bit32u sum = op1 + op2;

  BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), sum);

  SET_FLAGS_OSZAPC_ADD_8(op1, op2, sum);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADC_EbIbR(bxInstruction_c *i)
{
  Bit32u op1 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());
  Bit32u op2 = i->Ib();
  Bit32u sum = op1 + op2 + getB_CF();

  BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), sum);

  SET_FLAGS_OSZAPC_ADD_8(op1, op2, sum);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SBB_EbIbR(bxInstruction_c *i)
{
  Bit32u op1_8 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());
  Bit32u op2_8 = i->Ib();
  Bit32u diff_8 = op1_8 - (op2_8 + getB_CF());
  BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), diff_8);

  SET_FLAGS_OSZAPC_SUB_8(op1_8, op2_8, diff_8);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SUB_EbIbR(bxInstruction_c *i)
{
  Bit32u op1_8 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());
  Bit32u op2_8 = i->Ib();
  Bit32u diff_8 = op1_8 - op2_8;

  BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), diff_8);

  SET_FLAGS_OSZAPC_SUB_8(op1_8, op2_8, diff_8);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::XOR_EbIbR(bxInstruction_c *i)
{
  Bit8u op1, op2 = i->Ib();

  op1 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());
  op1 ^= op2;
  BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), op1);

  SET_FLAGS_OSZAPC_LOGIC_8(op1);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADC_EbGbM(bxInstruction_c *i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1 = read_RMW_virtual_byte(i->seg(), eaddr);
  Bit32u op2 = BX_READ_8BIT_REGx(i->src(), i->extend8bitL());
  Bit32u sum = op1 + op2 + getB_CF();

  write_RMW_linear_byte(sum);

  SET_FLAGS_OSZAPC_ADD_8(op1, op2, sum);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SBB_EbGbM(bxInstruction_c *i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_8 = read_RMW_virtual_byte(i->seg(), eaddr);
  Bit32u op2_8 = BX_READ_8BIT_REGx(i->src(), i->extend8bitL());
  Bit32u diff_8 = op1_8 - (op2_8 + getB_CF());

  write_RMW_linear_byte(diff_8);

  SET_FLAGS_OSZAPC_SUB_8(op1_8, op2_8, diff_8);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::AND_EbGbM(bxInstruction_c *i)
{
  Bit8u op1, op2;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op1 = read_RMW_virtual_byte(i->seg(), eaddr);
  op2 = BX_READ_8BIT_REGx(i->src(), i->extend8bitL());
  op1 &= op2;
  write_RMW_linear_byte(op1);

  SET_FLAGS_OSZAPC_LOGIC_8(op1);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SUB_EbGbM(bxInstruction_c *i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_8 = read_RMW_virtual_byte(i->seg(), eaddr);
  Bit32u op2_8 = BX_READ_8BIT_REGx(i->src(), i->extend8bitL());
  Bit32u diff_8 = op1_8 - op2_8;

  write_RMW_linear_byte(diff_8);

  SET_FLAGS_OSZAPC_SUB_8(op1_8, op2_8, diff_8);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::XOR_EbGbM(bxInstruction_c *i)
{
  Bit8u op1, op2;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op1 = read_RMW_virtual_byte(i->seg(), eaddr);
  op2 = BX_READ_8BIT_REGx(i->src(), i->extend8bitL());
  op1 ^= op2;
  write_RMW_linear_byte(op1);

  SET_FLAGS_OSZAPC_LOGIC_8(op1);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMP_EbGbM(bxInstruction_c *i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_8 = read_virtual_byte(i->seg(), eaddr);
  Bit32u op2_8 = BX_READ_8BIT_REGx(i->src(), i->extend8bitL());
  Bit32u diff_8 = op1_8 - op2_8;

  SET_FLAGS_OSZAPC_SUB_8(op1_8, op2_8, diff_8);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADD_EwIwM(bxInstruction_c *i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_16 = read_RMW_virtual_word(i->seg(), eaddr);
  Bit32u op2_16 = i->Iw();
  Bit32u sum_16 = op1_16 + op2_16;

  write_RMW_linear_word(sum_16);

  SET_FLAGS_OSZAPC_ADD_16(op1_16, op2_16, sum_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADC_EwIwM(bxInstruction_c *i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_16 = read_RMW_virtual_word(i->seg(), eaddr);
  Bit32u op2_16 = i->Iw();
  Bit32u sum_16 = op1_16 + op2_16 + getB_CF();

  write_RMW_linear_word(sum_16);

  SET_FLAGS_OSZAPC_ADD_16(op1_16, op2_16, sum_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SBB_EwIwM(bxInstruction_c *i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_16 = read_RMW_virtual_word(i->seg(), eaddr);
  Bit32u op2_16 = i->Iw();
  Bit32u diff_16 = op1_16 - (op2_16 + getB_CF());

  write_RMW_linear_word(diff_16);

  SET_FLAGS_OSZAPC_SUB_16(op1_16, op2_16, diff_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::AND_EwIwM(bxInstruction_c *i)
{
  Bit16u op1_16;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op1_16 = read_RMW_virtual_word(i->seg(), eaddr);
  op1_16 &= i->Iw();
  write_RMW_linear_word(op1_16);

  SET_FLAGS_OSZAPC_LOGIC_16(op1_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SUB_EwIwM(bxInstruction_c *i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_16 = read_RMW_virtual_word(i->seg(), eaddr);
  Bit32u op2_16 = i->Iw();
  Bit32u diff_16 = op1_16 - op2_16;

  write_RMW_linear_word(diff_16);

  SET_FLAGS_OSZAPC_SUB_16(op1_16, op2_16, diff_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::XOR_EwIwM(bxInstruction_c *i)
{
  Bit16u op1_16;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op1_16 = read_RMW_virtual_word(i->seg(), eaddr);
  op1_16 ^= i->Iw();
  write_RMW_linear_word(op1_16);

  SET_FLAGS_OSZAPC_LOGIC_16(op1_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMP_EwIwM(bxInstruction_c *i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_16 = read_virtual_word(i->seg(), eaddr);
  Bit32u op2_16 = i->Iw();
  Bit32u diff_16 = op1_16 - op2_16;

  SET_FLAGS_OSZAPC_SUB_16(op1_16, op2_16, diff_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADC_EwIwR(bxInstruction_c *i)
{
  Bit32u op1_16 = BX_READ_16BIT_REG(i->dst());
  Bit32u op2_16 = i->Iw();
  Bit32u sum_16 = op1_16 + op2_16 + getB_CF();

  BX_WRITE_16BIT_REG(i->dst(), sum_16);

  SET_FLAGS_OSZAPC_ADD_16(op1_16, op2_16, sum_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SBB_EwIwR(bxInstruction_c *i)
{
  Bit32u op1_16 = BX_READ_16BIT_REG(i->dst());
  Bit32u op2_16 = i->Iw();
  Bit32u diff_16 = op1_16 - (op2_16 + getB_CF());

  BX_WRITE_16BIT_REG(i->dst(), diff_16);

  SET_FLAGS_OSZAPC_SUB_16(op1_16, op2_16, diff_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SUB_EwIwR(bxInstruction_c *i)
{
  Bit32u op1_16 = BX_READ_16BIT_REG(i->dst());
  Bit32u op2_16 = i->Iw();
  Bit32u diff_16 = op1_16 - op2_16;

  BX_WRITE_16BIT_REG(i->dst(), diff_16);

  SET_FLAGS_OSZAPC_SUB_16(op1_16, op2_16, diff_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::XOR_EwIwR(bxInstruction_c *i)
{
  Bit16u op1_16 = BX_READ_16BIT_REG(i->dst());
  op1_16 ^= i->Iw();
  BX_WRITE_16BIT_REG(i->dst(), op1_16);

  SET_FLAGS_OSZAPC_LOGIC_16(op1_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADC_EdIdM(bxInstruction_c *i)
{
  Bit32u op1_32, op2_32 = i->Id(), sum_32, temp_CF = getB_CF();

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op1_32 = read_RMW_virtual_dword(i->seg(), eaddr);
  sum_32 = op1_32 + op2_32 + temp_CF;
  write_RMW_linear_dword(sum_32);

  SET_FLAGS_OSZAPC_ADD_32(op1_32, op2_32, sum_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SBB_EdIdM(bxInstruction_c *i)
{
  Bit32u op1_32, op2_32 = i->Id(), diff_32, temp_CF = getB_CF();

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op1_32 = read_RMW_virtual_dword(i->seg(), eaddr);
  diff_32 = op1_32 - (op2_32 + temp_CF);
  write_RMW_linear_dword(diff_32);

  SET_FLAGS_OSZAPC_SUB_32(op1_32, op2_32, diff_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::AND_EdIdM(bxInstruction_c *i)
{
  Bit32u op1_32;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op1_32 = read_RMW_virtual_dword(i->seg(), eaddr);
  op1_32 &= i->Id();
  write_RMW_linear_dword(op1_32);

  SET_FLAGS_OSZAPC_LOGIC_32(op1_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SUB_EdIdM(bxInstruction_c *i)
{
  Bit32u op1_32, op2_32 = i->Id(), diff_32;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op1_32 = read_RMW_virtual_dword(i->seg(), eaddr);
  diff_32 = op1_32 - op2_32;
  write_RMW_linear_dword(diff_32);

  SET_FLAGS_OSZAPC_SUB_32(op1_32, op2_32, diff_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::XOR_EdIdM(bxInstruction_c *i)
{
  Bit32u op1_32;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op1_32 = read_RMW_virtual_dword(i->seg(), eaddr);
  op1_32 ^= i->Id();
  write_RMW_linear_dword(op1_32);

  SET_FLAGS_OSZAPC_LOGIC_32(op1_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADC_EdIdR(bxInstruction_c *i)
{
  Bit32u op1_32, op2_32 = i->Id(), sum_32, temp_CF = getB_CF();

  op1_32 = BX_READ_32BIT_REG(i->dst());
  sum_32 = op1_32 + op2_32 + temp_CF;
  BX_WRITE_32BIT_REGZ(i->dst(), sum_32);

  SET_FLAGS_OSZAPC_ADD_32(op1_32, op2_32, sum_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SBB_EdIdR(bxInstruction_c *i)
{
  Bit32u op1_32, op2_32 = i->Id(), diff_32, temp_CF = getB_CF();

  op1_32 = BX_READ_32BIT_REG(i->dst());
  diff_32 = op1_32 - (op2_32 + temp_CF);
  BX_WRITE_32BIT_REGZ(i->dst(), diff_32);

  SET_FLAGS_OSZAPC_SUB_32(op1_32, op2_32, diff_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::XOR_EdIdR(bxInstruction_c *i)
{
  Bit32u op1_32 = BX_READ_32BIT_REG(i->dst());
  op1_32 ^= i->Id();
  BX_WRITE_32BIT_REGZ(i->dst(), op1_32);

  SET_FLAGS_OSZAPC_LOGIC_32(op1_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADD_EwGwM(bxInstruction_c *i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_16 = read_RMW_virtual_word(i->seg(), eaddr);
  Bit32u op2_16 = BX_READ_16BIT_REG(i->src());
  Bit32u sum_16 = op1_16 + op2_16;

  write_RMW_linear_word(sum_16);

  SET_FLAGS_OSZAPC_ADD_16(op1_16, op2_16, sum_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADC_EwGwM(bxInstruction_c *i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_16 = read_RMW_virtual_word(i->seg(), eaddr);
  Bit32u op2_16 = BX_READ_16BIT_REG(i->src());
  Bit32u sum_16 = op1_16 + op2_16 + getB_CF();

  write_RMW_linear_word(sum_16);

  SET_FLAGS_OSZAPC_ADD_16(op1_16, op2_16, sum_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SBB_EwGwM(bxInstruction_c *i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_16 = read_RMW_virtual_word(i->seg(), eaddr);
  Bit32u op2_16 = BX_READ_16BIT_REG(i->src());
  Bit32u diff_16 = op1_16 - (op2_16 + getB_CF());

  write_RMW_linear_word(diff_16);

  SET_FLAGS_OSZAPC_SUB_16(op1_16, op2_16, diff_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::AND_EwGwM(bxInstruction_c *i)
{
  Bit16u op1_16, op2_16;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op1_16 = read_RMW_virtual_word(i->seg(), eaddr);
  op2_16 = BX_READ_16BIT_REG(i->src());
  op1_16 &= op2_16;
  write_RMW_linear_word(op1_16);

  SET_FLAGS_OSZAPC_LOGIC_16(op1_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SUB_EwGwM(bxInstruction_c *i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_16 = read_RMW_virtual_word(i->seg(), eaddr);
  Bit32u op2_16 = BX_READ_16BIT_REG(i->src());
  Bit32u diff_16 = op1_16 - op2_16;

  write_RMW_linear_word(diff_16);

  SET_FLAGS_OSZAPC_SUB_16(op1_16, op2_16, diff_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::XOR_EwGwM(bxInstruction_c *i)
{
  Bit16u op1_16, op2_16;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op1_16 = read_RMW_virtual_word(i->seg(), eaddr);
  op2_16 = BX_READ_16BIT_REG(i->src());
  op1_16 ^= op2_16;
  write_RMW_linear_word(op1_16);

  SET_FLAGS_OSZAPC_LOGIC_16(op1_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMP_EwGwM(bxInstruction_c *i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_16 = read_virtual_word(i->seg(), eaddr);
  Bit32u op2_16 = BX_READ_16BIT_REG(i->src());
  Bit32u diff_16 = op1_16 - op2_16;

  SET_FLAGS_OSZAPC_SUB_16(op1_16, op2_16, diff_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADD_EdGdM(bxInstruction_c *i)
{
  Bit32u op1_32, op2_32, sum_32;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op1_32 = read_RMW_virtual_dword(i->seg(), eaddr);
  op2_32 = BX_READ_32BIT_REG(i->src());
  sum_32 = op1_32 + op2_32;
  write_RMW_linear_dword(sum_32);

  SET_FLAGS_OSZAPC_ADD_32(op1_32, op2_32, sum_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADC_EdGdM(bxInstruction_c *i)
{
  Bit32u op1_32, op2_32, sum_32, temp_CF = getB_CF();

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op1_32 = read_RMW_virtual_dword(i->seg(), eaddr);
  op2_32 = BX_READ_32BIT_REG(i->src());
  sum_32 = op1_32 + op2_32 + temp_CF;
  write_RMW_linear_dword(sum_32);

  SET_FLAGS_OSZAPC_ADD_32(op1_32, op2_32, sum_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SBB_EdGdM(bxInstruction_c *i)
{
  Bit32u op1_32, op2_32, diff_32, temp_CF = getB_CF();

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op1_32 = read_RMW_virtual_dword(i->seg(), eaddr);
  op2_32 = BX_READ_32BIT_REG(i->src());
  diff_32 = op1_32 - (op2_32 + temp_CF);
  write_RMW_linear_dword(diff_32);

  SET_FLAGS_OSZAPC_SUB_32(op1_32, op2_32, diff_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::AND_EdGdM(bxInstruction_c *i)
{
  Bit32u op1_32, op2_32;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op1_32 = read_RMW_virtual_dword(i->seg(), eaddr);
  op2_32 = BX_READ_32BIT_REG(i->src());
  op1_32 &= op2_32;
  write_RMW_linear_dword(op1_32);

  SET_FLAGS_OSZAPC_LOGIC_32(op1_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::XOR_EdGdM(bxInstruction_c *i)
{
  Bit32u op1_32, op2_32;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op1_32 = read_RMW_virtual_dword(i->seg(), eaddr);
  op2_32 = BX_READ_32BIT_REG(i->src());
  op1_32 ^= op2_32;
  write_RMW_linear_dword(op1_32);

  SET_FLAGS_OSZAPC_LOGIC_32(op1_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMP_EdGdM(bxInstruction_c *i)
{
  Bit32u op1_32, op2_32, diff_32;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op1_32 = read_virtual_dword(i->seg(), eaddr);
  op2_32 = BX_READ_32BIT_REG(i->src());
  diff_32 = op1_32 - op2_32;

  SET_FLAGS_OSZAPC_SUB_32(op1_32, op2_32, diff_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADD_GwEwR(bxInstruction_c *i)
{
  Bit32u op1_16 = BX_READ_16BIT_REG(i->dst());
  Bit32u op2_16 = BX_READ_16BIT_REG(i->src());
  Bit32u sum_16 = op1_16 + op2_16;

  BX_WRITE_16BIT_REG(i->dst(), sum_16);

  SET_FLAGS_OSZAPC_ADD_16(op1_16, op2_16, sum_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADC_GwEwR(bxInstruction_c *i)
{
  Bit32u op1_16 = BX_READ_16BIT_REG(i->dst());
  Bit32u op2_16 = BX_READ_16BIT_REG(i->src());
  Bit32u sum_16 = op1_16 + op2_16 + getB_CF();

  BX_WRITE_16BIT_REG(i->dst(), sum_16);

  SET_FLAGS_OSZAPC_ADD_16(op1_16, op2_16, sum_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SBB_GwEwR(bxInstruction_c *i)
{
  Bit32u op1_16 = BX_READ_16BIT_REG(i->dst());
  Bit32u op2_16 = BX_READ_16BIT_REG(i->src());
  Bit32u diff_16 = op1_16 - (op2_16 + getB_CF());

  BX_WRITE_16BIT_REG(i->dst(), diff_16);

  SET_FLAGS_OSZAPC_SUB_16(op1_16, op2_16, diff_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::AND_GwEwR(bxInstruction_c *i)
{
  Bit16u op1_16, op2_16;

  op1_16 = BX_READ_16BIT_REG(i->dst());
  op2_16 = BX_READ_16BIT_REG(i->src());
  op1_16 &= op2_16;
  BX_WRITE_16BIT_REG(i->dst(), op1_16);

  SET_FLAGS_OSZAPC_LOGIC_16(op1_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SUB_GwEwR(bxInstruction_c *i)
{
  Bit32u op1_16 = BX_READ_16BIT_REG(i->dst());
  Bit32u op2_16 = BX_READ_16BIT_REG(i->src());
  Bit32u diff_16 = op1_16 - op2_16;

  BX_WRITE_16BIT_REG(i->dst(), diff_16);

  SET_FLAGS_OSZAPC_SUB_16(op1_16, op2_16, diff_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::XOR_GwEwR(bxInstruction_c *i)
{
  Bit16u op1_16, op2_16;

  op1_16 = BX_READ_16BIT_REG(i->dst());
  op2_16 = BX_READ_16BIT_REG(i->src());
  op1_16 ^= op2_16;
  BX_WRITE_16BIT_REG(i->dst(), op1_16);

  SET_FLAGS_OSZAPC_LOGIC_16(op1_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMP_GwEwR(bxInstruction_c *i)
{
  Bit32u op1_16 = BX_READ_16BIT_REG(i->dst());
  Bit32u op2_16 = BX_READ_16BIT_REG(i->src());
  Bit32u diff_16 = op1_16 - op2_16;

  SET_FLAGS_OSZAPC_SUB_16(op1_16, op2_16, diff_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADD_GwEwM(bxInstruction_c *i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_16 = BX_READ_16BIT_REG(i->dst());
  Bit32u op2_16 = read_virtual_word(i->seg(), eaddr);
  Bit32u sum_16 = op1_16 + op2_16;

  BX_WRITE_16BIT_REG(i->dst(), sum_16);

  SET_FLAGS_OSZAPC_ADD_16(op1_16, op2_16, sum_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADC_GwEwM(bxInstruction_c *i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_16 = BX_READ_16BIT_REG(i->dst());
  Bit32u op2_16 = read_virtual_word(i->seg(), eaddr);
  Bit32u sum_16 = op1_16 + op2_16 + getB_CF();

  BX_WRITE_16BIT_REG(i->dst(), sum_16);

  SET_FLAGS_OSZAPC_ADD_16(op1_16, op2_16, sum_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SBB_GwEwM(bxInstruction_c *i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_16 = BX_READ_16BIT_REG(i->dst());
  Bit32u op2_16 = read_virtual_word(i->seg(), eaddr);
  Bit32u diff_16 = op1_16 - (op2_16 + getB_CF());

  BX_WRITE_16BIT_REG(i->dst(), diff_16);

  SET_FLAGS_OSZAPC_SUB_16(op1_16, op2_16, diff_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::AND_GwEwM(bxInstruction_c *i)
{
  Bit16u op1_16, op2_16;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op1_16 = BX_READ_16BIT_REG(i->dst());
  op2_16 = read_virtual_word(i->seg(), eaddr);
  op1_16 &= op2_16;
  BX_WRITE_16BIT_REG(i->dst(), op1_16);

  SET_FLAGS_OSZAPC_LOGIC_16(op1_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SUB_GwEwM(bxInstruction_c *i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_16 = BX_READ_16BIT_REG(i->dst());
  Bit32u op2_16 = read_virtual_word(i->seg(), eaddr);
  Bit32u diff_16 = op1_16 - op2_16;

  BX_WRITE_16BIT_REG(i->dst(), diff_16);

  SET_FLAGS_OSZAPC_SUB_16(op1_16, op2_16, diff_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::XOR_GwEwM(bxInstruction_c *i)
{
  Bit16u op1_16, op2_16;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op1_16 = BX_READ_16BIT_REG(i->dst());
  op2_16 = read_virtual_word(i->seg(), eaddr);
  op1_16 ^= op2_16;
  BX_WRITE_16BIT_REG(i->dst(), op1_16);

  SET_FLAGS_OSZAPC_LOGIC_16(op1_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMP_GwEwM(bxInstruction_c *i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_16 = BX_READ_16BIT_REG(i->dst());
  Bit32u op2_16 = read_virtual_word(i->seg(), eaddr);
  Bit32u diff_16 = op1_16 - op2_16;

  SET_FLAGS_OSZAPC_SUB_16(op1_16, op2_16, diff_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADC_GdEdR(bxInstruction_c *i)
{
  Bit32u op1_32, op2_32, sum_32, temp_CF = getB_CF();

  op1_32 = BX_READ_32BIT_REG(i->dst());
  op2_32 = BX_READ_32BIT_REG(i->src());
  sum_32 = op1_32 + op2_32 + temp_CF;
  BX_WRITE_32BIT_REGZ(i->dst(), sum_32);

  SET_FLAGS_OSZAPC_ADD_32(op1_32, op2_32, sum_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SBB_GdEdR(bxInstruction_c *i)
{
  Bit32u op1_32, op2_32, diff_32, temp_CF = getB_CF();

  op1_32 = BX_READ_32BIT_REG(i->dst());
  op2_32 = BX_READ_32BIT_REG(i->src());
  diff_32 = op1_32 - (op2_32 + temp_CF);
  BX_WRITE_32BIT_REGZ(i->dst(), diff_32);

  SET_FLAGS_OSZAPC_SUB_32(op1_32, op2_32, diff_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::XOR_GdEdR(bxInstruction_c *i)
{
  Bit32u op1_32, op2_32;

  op1_32 = BX_READ_32BIT_REG(i->dst());
  op2_32 = BX_READ_32BIT_REG(i->src());
  op1_32 ^= op2_32;
  BX_WRITE_32BIT_REGZ(i->dst(), op1_32);

  SET_FLAGS_OSZAPC_LOGIC_32(op1_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADD_GdEdM(bxInstruction_c *i)
{
  Bit32u op1_32, op2_32, sum_32;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op1_32 = BX_READ_32BIT_REG(i->dst());
  op2_32 = read_virtual_dword(i->seg(), eaddr);
  sum_32 = op1_32 + op2_32;

  BX_WRITE_32BIT_REGZ(i->dst(), sum_32);

  SET_FLAGS_OSZAPC_ADD_32(op1_32, op2_32, sum_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADC_GdEdM(bxInstruction_c *i)
{
  Bit32u op1_32, op2_32, sum_32, temp_CF = getB_CF();

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op1_32 = BX_READ_32BIT_REG(i->dst());
  op2_32 = read_virtual_dword(i->seg(), eaddr);
  sum_32 = op1_32 + op2_32 + temp_CF;
  BX_WRITE_32BIT_REGZ(i->dst(), sum_32);

  SET_FLAGS_OSZAPC_ADD_32(op1_32, op2_32, sum_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SBB_GdEdM(bxInstruction_c *i)
{
  Bit32u op1_32, op2_32, diff_32, temp_CF = getB_CF();

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op1_32 = BX_READ_32BIT_REG(i->dst());
  op2_32 = read_virtual_dword(i->seg(), eaddr);
  diff_32 = op1_32 - (op2_32 + temp_CF);
  BX_WRITE_32BIT_REGZ(i->dst(), diff_32);

  SET_FLAGS_OSZAPC_SUB_32(op1_32, op2_32, diff_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::AND_GdEdM(bxInstruction_c *i)
{
  Bit32u op1_32, op2_32;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op1_32 = BX_READ_32BIT_REG(i->dst());
  op2_32 = read_virtual_dword(i->seg(), eaddr);
  op1_32 &= op2_32;
  BX_WRITE_32BIT_REGZ(i->dst(), op1_32);

  SET_FLAGS_OSZAPC_LOGIC_32(op1_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::XOR_GdEdM(bxInstruction_c *i)
{
  Bit32u op1_32, op2_32;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op1_32 = BX_READ_32BIT_REG(i->dst());
  op2_32 = read_virtual_dword(i->seg(), eaddr);
  op1_32 ^= op2_32;
  BX_WRITE_32BIT_REGZ(i->dst(), op1_32);

  SET_FLAGS_OSZAPC_LOGIC_32(op1_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ROL_EbR(bxInstruction_c *i)
{
  unsigned count;
  unsigned bit0, bit7;

  if (i->getIaOpcode() == BX_IA_ROL_Eb)
    count = CL;
  else
    count = i->Ib();

  Bit8u op1_8 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());

  if ((count & 0x07) == 0) {
    if (count & 0x18) {
      bit0 = (op1_8 &  1);
      bit7 = (op1_8 >> 7);
      BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(bit0 ^ bit7, bit0);
    }
  }
  else {
    count &= 0x7; // use only lowest 3 bits

    Bit8u result_8 = rol8(op1_8, count);

    BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), result_8);

    /* set eflags:
     * ROL count affects the following flags: C, O
     */
    bit0 = (result_8 &  1);
    bit7 = (result_8 >> 7);

    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(bit0 ^ bit7, bit0);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ROR_EbR(bxInstruction_c *i)
{
  unsigned count;
  unsigned bit6, bit7;

  if (i->getIaOpcode() == BX_IA_ROR_Eb)
    count = CL;
  else
    count = i->Ib();

  Bit8u op1_8 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());

  if ((count & 0x07) == 0) {
    if (count & 0x18) {
      bit6 = (op1_8 >> 6) & 1;
      bit7 = (op1_8 >> 7) & 1;

      BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(bit6 ^ bit7, bit7);
    }
  }
  else {
    count &= 0x7; /* use only bottom 3 bits */

    Bit8u result_8 = ror8(op1_8, count);

    BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), result_8);

    /* set eflags:
     * ROR count affects the following flags: C, O
     */
    bit6 = (result_8 >> 6) & 1;
    bit7 = (result_8 >> 7) & 1;

    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(bit6 ^ bit7, bit7);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::RCL_EbR(bxInstruction_c *i)
{
  Bit8u result_8;
  unsigned count;
  unsigned of, cf;

  if (i->getIaOpcode() == BX_IA_RCL_Eb)
    count = CL;
  else
    count = i->Ib();

  count = (count & 0x1f) % 9;

  if (! count) {
    BX_NEXT_INSTR(i);
  }

  Bit8u op1_8 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());

  unsigned temp_CF = getB_CF();

  if (count==1) {
    result_8 = (op1_8 << 1) | temp_CF;
  }
  else {
    result_8 = (op1_8 << count) | (temp_CF << (count - 1)) |
               (op1_8 >> (9 - count));
  }

  BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), result_8);

  cf = (op1_8 >> (8 - count)) & 0x01;
  of = cf ^ (result_8 >> 7);  // of = cf ^ result7
  BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::RCR_EbR(bxInstruction_c *i)
{
  unsigned count;
  unsigned cf, of;

  if (i->getIaOpcode() == BX_IA_RCR_Eb)
    count = CL;
  else
    count = i->Ib();

  count = (count & 0x1f) % 9;

  if (count) {
    Bit8u op1_8 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());

    unsigned temp_CF = getB_CF();

    Bit8u result_8 = (op1_8 >> count) | (temp_CF << (8 - count)) |
                     (op1_8 << (9 - count));

    BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), result_8);

    cf = (op1_8 >> (count - 1)) & 0x1;
    of = (((result_8 << 1) ^ result_8) >> 7) & 0x1; // of = result6 ^ result7
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SHL_EbR(bxInstruction_c *i)
{
  Bit8u result_8;
  unsigned count;
  unsigned of = 0, cf = 0;

  if (i->getIaOpcode() == BX_IA_SHL_Eb)
    count = CL;
  else
    count = i->Ib();

  count &= 0x1f;

  if (!count) {
    BX_NEXT_INSTR(i);
  }

  Bit8u op1_8 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());

  if (count <= 8) {
    result_8 = (op1_8 << count);
    cf = (op1_8 >> (8 - count)) & 0x1;
    of = cf ^ (result_8 >> 7);
  }
  else {
    result_8 = 0;
  }

  BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), result_8);

  SET_FLAGS_OSZAPC_LOGIC_8(result_8);
  BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SAR_EbR(bxInstruction_c *i)
{
  unsigned count;

  if (i->getIaOpcode() == BX_IA_SAR_Eb)
    count = CL;
  else
    count = i->Ib();

  count &= 0x1f;

  if (count) {
    Bit8u op1_8 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());
    Bit8u result_8 = ((Bit8s) op1_8) >> count;
    BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), result_8);

    unsigned cf = (((Bit8s) op1_8) >> (count - 1)) & 0x1;

    SET_FLAGS_OSZAPC_LOGIC_8(result_8);
    /* signed overflow cannot happen in SAR instruction */
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(0, cf);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ROL_EbM(bxInstruction_c *i)
{
  unsigned count;
  unsigned bit0, bit7;

  if (i->getIaOpcode() == BX_IA_ROL_Eb)
    count = CL;
  else
    count = i->Ib();

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);
  /* pointer, segment address pair */
  Bit8u op1_8 = read_RMW_virtual_byte(i->seg(), eaddr);

  if ((count & 0x07) == 0) {
    if (count & 0x18) {
      bit0 = (op1_8 &  1);
      bit7 = (op1_8 >> 7);
      BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(bit0 ^ bit7, bit0);
    }
  }
  else {
    count &= 0x7; // use only lowest 3 bits

    Bit8u result_8 = rol8(op1_8, count);

    write_RMW_linear_byte(result_8);

    /* set eflags:
     * ROL count affects the following flags: C, O
     */
    bit0 = (result_8 &  1);
    bit7 = (result_8 >> 7);

    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(bit0 ^ bit7, bit0);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ROR_EbM(bxInstruction_c *i)
{
  unsigned count;
  unsigned bit6, bit7;

  if (i->getIaOpcode() == BX_IA_ROR_Eb)
    count = CL;
  else
    count = i->Ib();

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);
  /* pointer, segment address pair */
  Bit8u op1_8 = read_RMW_virtual_byte(i->seg(), eaddr);

  if ((count & 0x07) == 0) {
    if (count & 0x18) {
      bit6 = (op1_8 >> 6) & 1;
      bit7 = (op1_8 >> 7) & 1;

      BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(bit6 ^ bit7, bit7);
    }
  }
  else {
    count &= 0x7; /* use only bottom 3 bits */

    Bit8u result_8 = ror8(op1_8, count);

    write_RMW_linear_byte(result_8);

    /* set eflags:
     * ROR count affects the following flags: C, O
     */
    bit6 = (result_8 >> 6) & 1;
    bit7 = (result_8 >> 7) & 1;

    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(bit6 ^ bit7, bit7);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::RCL_EbM(bxInstruction_c *i)
{
  Bit8u result_8;
  unsigned count;
  unsigned of, cf;

  if (i->getIaOpcode() == BX_IA_RCL_Eb)
    count = CL;
  else
    count = i->Ib();

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);
  /* pointer, segment address pair */
  Bit8u op1_8 = read_RMW_virtual_byte(i->seg(), eaddr);

  count = (count & 0x1f) % 9;

  if (! count) {
    BX_NEXT_INSTR(i);
  }

  unsigned temp_CF = getB_CF();

  if (count==1) {
    result_8 = (op1_8 << 1) | temp_CF;
  }
  else {
    result_8 = (op1_8 << count) | (temp_CF << (count - 1)) |
               (op1_8 >> (9 - count));
  }

  write_RMW_linear_byte(result_8);

  cf = (op1_8 >> (8 - count)) & 0x01;
  of = cf ^ (result_8 >> 7);  // of = cf ^ result7
  BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::RCR_EbM(bxInstruction_c *i)
{
  unsigned count;
  unsigned cf, of;

  if (i->getIaOpcode() == BX_IA_RCR_Eb)
    count = CL;
  else
    count = i->Ib();

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);
  /* pointer, segment address pair */
  Bit8u op1_8 = read_RMW_virtual_byte(i->seg(), eaddr);

  count = (count & 0x1f) % 9;

  if (count) {
    unsigned temp_CF = getB_CF();

    Bit8u result_8 = (op1_8 >> count) | (temp_CF << (8 - count)) |
                     (op1_8 << (9 - count));

    write_RMW_linear_byte(result_8);

    cf = (op1_8 >> (count - 1)) & 0x1;
    of = (((result_8 << 1) ^ result_8) >> 7) & 0x1; // of = result6 ^ result7
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SHL_EbM(bxInstruction_c *i)
{
  Bit8u result_8;
  unsigned count;
  unsigned of = 0, cf = 0;

  if (i->getIaOpcode() == BX_IA_SHL_Eb)
    count = CL;
  else
    count = i->Ib();

  count &= 0x1f;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);
  /* pointer, segment address pair */
  Bit8u op1_8 = read_RMW_virtual_byte(i->seg(), eaddr);

  if (!count) {
    BX_NEXT_INSTR(i);
  }

  if (count <= 8) {
    result_8 = (op1_8 << count);
    cf = (op1_8 >> (8 - count)) & 0x1;
    of = cf ^ (result_8 >> 7);
  }
  else {
    result_8 = 0;
  }

  write_RMW_linear_byte(result_8);

  SET_FLAGS_OSZAPC_LOGIC_8(result_8);
  BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SHR_EbM(bxInstruction_c *i)
{
  unsigned count;

  if (i->getIaOpcode() == BX_IA_SHR_Eb)
    count = CL;
  else
    count = i->Ib();

  count &= 0x1f;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);
  /* pointer, segment address pair */
  Bit8u op1_8 = read_RMW_virtual_byte(i->seg(), eaddr);

  if (count) {
    Bit8u result_8 = (op1_8 >> count);

    write_RMW_linear_byte(result_8);

    unsigned cf = (op1_8 >> (count - 1)) & 0x1;
    // note, that of == result7 if count == 1 and
    //            of == 0       if count >= 2
    unsigned of = (((result_8 << 1) ^ result_8) >> 7) & 0x1;

    SET_FLAGS_OSZAPC_LOGIC_8(result_8);
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SAR_EbM(bxInstruction_c *i)
{
  unsigned count;

  if (i->getIaOpcode() == BX_IA_SAR_Eb)
    count = CL;
  else
    count = i->Ib();

  count &= 0x1f;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);
  /* pointer, segment address pair */
  Bit8u op1_8 = read_RMW_virtual_byte(i->seg(), eaddr);

  if (count) {
    Bit8u result_8 = ((Bit8s) op1_8) >> count;

    write_RMW_linear_byte(result_8);

    unsigned cf = (((Bit8s) op1_8) >> (count - 1)) & 0x1;

    SET_FLAGS_OSZAPC_LOGIC_8(result_8);
    /* signed overflow cannot happen in SAR instruction */
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(0, cf);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ROL_EwM(bxInstruction_c *i)
{
  unsigned count;
  unsigned bit0, bit15;

  if (i->getIaOpcode() == BX_IA_ROL_Ew)
    count = CL;
  else
    count = i->Ib();

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);
  /* pointer, segment address pair */
  Bit16u op1_16 = read_RMW_virtual_word(i->seg(), eaddr);

  if ((count & 0x0f) == 0) {
    if (count & 0x10) {
      bit0  = (op1_16 & 0x1);
      bit15 = (op1_16 >> 15);
      // of = cf ^ result15
      BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(bit0 ^ bit15, bit0);
    }
  }
  else {
    count &= 0x0f; // only use bottom 4 bits

    Bit16u result_16 = rol16(op1_16, count);

    write_RMW_linear_word(result_16);

    bit0  = (result_16 & 0x1);
    bit15 = (result_16 >> 15);
    // of = cf ^ result15
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(bit0 ^ bit15, bit0);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ROR_EwM(bxInstruction_c *i)
{
  unsigned count;
  unsigned bit14, bit15;

  if (i->getIaOpcode() == BX_IA_ROR_Ew)
    count = CL;
  else
    count = i->Ib();

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);
  /* pointer, segment address pair */
  Bit16u op1_16 = read_RMW_virtual_word(i->seg(), eaddr);

  if ((count & 0x0f) == 0) {
    if (count & 0x10) {
      bit14 = (op1_16 >> 14) & 1;
      bit15 = (op1_16 >> 15) & 1;
      // of = result14 ^ result15
      BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(bit14 ^ bit15, bit15);
    }
  }
  else {
    count &= 0x0f;  // use only 4 LSB's

    Bit16u result_16 = ror16(op1_16, count);

    write_RMW_linear_word(result_16);

    bit14 = (result_16 >> 14) & 1;
    bit15 = (result_16 >> 15) & 1;
    // of = result14 ^ result15
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(bit14 ^ bit15, bit15);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::RCL_EwM(bxInstruction_c *i)
{
  Bit16u result_16;
  unsigned count;
  unsigned of, cf;

  if (i->getIaOpcode() == BX_IA_RCL_Ew)
    count = CL;
  else
    count = i->Ib();

  count = (count & 0x1f) % 17;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);
  /* pointer, segment address pair */
  Bit16u op1_16 = read_RMW_virtual_word(i->seg(), eaddr);

  unsigned temp_CF = getB_CF();

  if (count) {
    if (count==1) {
      result_16 = (op1_16 << 1) | temp_CF;
    }
    else if (count==16) {
      result_16 = (temp_CF << 15) | (op1_16 >> 1);
    }
    else { // 2..15
      result_16 = (op1_16 << count) | (temp_CF << (count - 1)) |
                  (op1_16 >> (17 - count));
    }

    write_RMW_linear_word(result_16);

    cf = (op1_16 >> (16 - count)) & 0x1;
    of = cf ^ (result_16 >> 15); // of = cf ^ result15
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::RCR_EwM(bxInstruction_c *i)
{
  unsigned count;
  unsigned of, cf;

  if (i->getIaOpcode() == BX_IA_RCR_Ew)
    count = CL;
  else
    count = i->Ib();

  count = (count & 0x1f) % 17;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);
  /* pointer, segment address pair */
  Bit16u op1_16 = read_RMW_virtual_word(i->seg(), eaddr);

  if (count) {
    unsigned temp_CF = getB_CF();

    Bit16u result_16 = (op1_16 >> count) | (temp_CF << (16 - count)) |
                       (op1_16 << (17 - count));

    write_RMW_linear_word(result_16);

    cf = (op1_16 >> (count - 1)) & 0x1;
    of = ((Bit16u)((result_16 << 1) ^ result_16) >> 15) & 0x1; // of = result15 ^ result14
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SHL_EwM(bxInstruction_c *i)
{
  Bit16u result_16;
  unsigned count;
  unsigned of = 0, cf = 0;

  if (i->getIaOpcode() == BX_IA_SHL_Ew)
    count = CL;
  else
    count = i->Ib();

  count &= 0x1f; /* use only 5 LSB's */

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);
  /* pointer, segment address pair */
  Bit16u op1_16 = read_RMW_virtual_word(i->seg(), eaddr);

  if (count) {
    if (count <= 16) {
      result_16 = (op1_16 << count);
      cf = (op1_16 >> (16 - count)) & 0x1;
      of = cf ^ (result_16 >> 15); // of = cf ^ result15
    }
    else {
      result_16 = 0;
    }

    write_RMW_linear_word(result_16);

    SET_FLAGS_OSZAPC_LOGIC_16(result_16);
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SHR_EwM(bxInstruction_c *i)
{
  unsigned count;
  unsigned of, cf;

  if (i->getIaOpcode() == BX_IA_SHR_Ew)
    count = CL;
  else
    count = i->Ib();

  count &= 0x1f; /* use only 5 LSB's */

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);
  /* pointer, segment address pair */
  Bit16u op1_16 = read_RMW_virtual_word(i->seg(), eaddr);

  if (count) {
    Bit16u result_16 = (op1_16 >> count);

    write_RMW_linear_word(result_16);

    cf = (op1_16 >> (count - 1)) & 0x1;
    // note, that of == result15 if count == 1 and
    //            of == 0        if count >= 2
    of = ((Bit16u)((result_16 << 1) ^ result_16) >> 15) & 0x1;

    SET_FLAGS_OSZAPC_LOGIC_16(result_16);
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SAR_EwM(bxInstruction_c *i)
{
  unsigned count, cf;

  if (i->getIaOpcode() == BX_IA_SAR_Ew)
    count = CL;
  else
    count = i->Ib();

  count &= 0x1f;  /* use only 5 LSB's */

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);
  Bit16u op1_16 = read_RMW_virtual_word(i->seg(), eaddr);

  if (count) {
    Bit16u result_16 = ((Bit16s) op1_16) >> count;

    cf = (((Bit16s) op1_16) >> (count - 1)) & 0x1;

    SET_FLAGS_OSZAPC_LOGIC_16(result_16);
    /* signed overflow cannot happen in SAR instruction */
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(0, cf);

    write_RMW_linear_word(result_16);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ROL_EwR(bxInstruction_c *i)
{
  unsigned count;
  unsigned bit0, bit15;

  if (i->getIaOpcode() == BX_IA_ROL_Ew)
    count = CL;
  else
    count = i->Ib();

  Bit16u op1_16 = BX_READ_16BIT_REG(i->dst());

  if ((count & 0x0f) == 0) {
    if (count & 0x10) {
      bit0  = (op1_16 & 0x1);
      bit15 = (op1_16 >> 15);
      // of = cf ^ result15
      BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(bit0 ^ bit15, bit0);
    }
  }
  else {
    count &= 0x0f; // only use bottom 4 bits

    Bit16u result_16 = rol16(op1_16, count);

    BX_WRITE_16BIT_REG(i->dst(), result_16);

    bit0  = (result_16 & 0x1);
    bit15 = (result_16 >> 15);
    // of = cf ^ result15
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(bit0 ^ bit15, bit0);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ROR_EwR(bxInstruction_c *i)
{
  unsigned count;
  unsigned bit14, bit15;

  if (i->getIaOpcode() == BX_IA_ROR_Ew)
    count = CL;
  else
    count = i->Ib();

  Bit16u op1_16 = BX_READ_16BIT_REG(i->dst());

  if ((count & 0x0f) == 0) {
    if (count & 0x10) {
      bit14 = (op1_16 >> 14) & 1;
      bit15 = (op1_16 >> 15) & 1;
      // of = result14 ^ result15
      BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(bit14 ^ bit15, bit15);
    }
  }
  else {
    count &= 0x0f;  // use only 4 LSB's

    Bit16u result_16 = ror16(op1_16, count);

    BX_WRITE_16BIT_REG(i->dst(), result_16);

    bit14 = (result_16 >> 14) & 1;
    bit15 = (result_16 >> 15) & 1;
    // of = result14 ^ result15
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(bit14 ^ bit15, bit15);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::RCL_EwR(bxInstruction_c *i)
{
  Bit16u result_16;
  unsigned count;
  unsigned of, cf;

  if (i->getIaOpcode() == BX_IA_RCL_Ew)
    count = CL;
  else
    count = i->Ib();

  count = (count & 0x1f) % 17;

  unsigned temp_CF = getB_CF();

  if (count) {
    Bit16u op1_16 = BX_READ_16BIT_REG(i->dst());

    if (count==1) {
      result_16 = (op1_16 << 1) | temp_CF;
    }
    else if (count==16) {
      result_16 = (temp_CF << 15) | (op1_16 >> 1);
    }
    else { // 2..15
      result_16 = (op1_16 << count) | (temp_CF << (count - 1)) |
                  (op1_16 >> (17 - count));
    }

    BX_WRITE_16BIT_REG(i->dst(), result_16);

    cf = (op1_16 >> (16 - count)) & 0x1;
    of = cf ^ (result_16 >> 15); // of = cf ^ result15
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::RCR_EwR(bxInstruction_c *i)
{
  unsigned count;
  unsigned of, cf;

  if (i->getIaOpcode() == BX_IA_RCR_Ew)
    count = CL;
  else
    count = i->Ib();

  count = (count & 0x1f) % 17;

  if (count) {
    Bit16u op1_16 = BX_READ_16BIT_REG(i->dst());

    unsigned temp_CF = getB_CF();

    Bit16u result_16 = (op1_16 >> count) | (temp_CF << (16 - count)) |
                       (op1_16 << (17 - count));

    BX_WRITE_16BIT_REG(i->dst(), result_16);

    cf = (op1_16 >> (count - 1)) & 0x1;
    of = ((Bit16u)((result_16 << 1) ^ result_16) >> 15) & 0x1; // of = result15 ^ result14
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SHR_EwR(bxInstruction_c *i)
{
  unsigned count;
  unsigned of, cf;

  if (i->getIaOpcode() == BX_IA_SHR_Ew)
    count = CL;
  else
    count = i->Ib();

  count &= 0x1f; /* use only 5 LSB's */

  if (count) {
    Bit16u op1_16 = BX_READ_16BIT_REG(i->dst());
    Bit16u result_16 = (op1_16 >> count);
    BX_WRITE_16BIT_REG(i->dst(), result_16);

    cf = (op1_16 >> (count - 1)) & 0x1;
    // note, that of == result15 if count == 1 and
    //            of == 0        if count >= 2
    of = ((Bit16u)((result_16 << 1) ^ result_16) >> 15) & 0x1;

    SET_FLAGS_OSZAPC_LOGIC_16(result_16);
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SAR_EwR(bxInstruction_c *i)
{
  unsigned count, cf;

  if (i->getIaOpcode() == BX_IA_SAR_Ew)
    count = CL;
  else
    count = i->Ib();

  count &= 0x1f;  /* use only 5 LSB's */

  if (count) {
    Bit16u op1_16 = BX_READ_16BIT_REG(i->dst());
    Bit16u result_16 = ((Bit16s) op1_16) >> count;
    BX_WRITE_16BIT_REG(i->dst(), result_16);

    cf = (((Bit16s) op1_16) >> (count - 1)) & 0x1;

    SET_FLAGS_OSZAPC_LOGIC_16(result_16);
    /* signed overflow cannot happen in SAR instruction */
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(0, cf);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ROL_EdM(bxInstruction_c *i)
{
  unsigned count;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_32 = read_RMW_virtual_dword(i->seg(), eaddr);

  if (i->getIaOpcode() == BX_IA_ROL_Ed)
    count = CL;
  else
    count = i->Ib();

  count &= 0x1f;

  if (count) {
    Bit32u result_32 = rol32(op1_32, count);

    write_RMW_linear_dword(result_32);

    unsigned bit0  = (result_32 & 0x1);
    unsigned bit31 = (result_32 >> 31);
    // of = cf ^ result31
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(bit0 ^ bit31, bit0);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ROR_EdM(bxInstruction_c *i)
{
  unsigned count;
  unsigned bit31, bit30;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_32 = read_RMW_virtual_dword(i->seg(), eaddr);

  if (i->getIaOpcode() == BX_IA_ROR_Ed)
    count = CL;
  else
    count = i->Ib();

  count &= 0x1f;

  if (count) {
    Bit32u result_32 = ror32(op1_32, count);

    write_RMW_linear_dword(result_32);

    bit31 = (result_32 >> 31) & 1;
    bit30 = (result_32 >> 30) & 1;
    // of = result30 ^ result31
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(bit30 ^ bit31, bit31);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::RCL_EdM(bxInstruction_c *i)
{
  Bit32u result_32;
  unsigned count;
  unsigned cf, of;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_32 = read_RMW_virtual_dword(i->seg(), eaddr);

  if (i->getIaOpcode() == BX_IA_RCL_Ed)
    count = CL;
  else
    count = i->Ib();

  count &= 0x1f;
  if (!count) {
    BX_NEXT_INSTR(i);
  }

  Bit32u temp_CF = getB_CF();

  if (count==1) {
    result_32 = (op1_32 << 1) | temp_CF;
  }
  else {
    result_32 = (op1_32 << count) | (temp_CF << (count - 1)) |
                (op1_32 >> (33 - count));
  }

  write_RMW_linear_dword(result_32);

  cf = (op1_32 >> (32 - count)) & 0x1;
  of = cf ^ (result_32 >> 31); // of = cf ^ result31
  BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::RCR_EdM(bxInstruction_c *i)
{
  Bit32u result_32;
  unsigned count;
  unsigned cf, of;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_32 = read_RMW_virtual_dword(i->seg(), eaddr);

  if (i->getIaOpcode() == BX_IA_RCR_Ed)
    count = CL;
  else
    count = i->Ib();

  count &= 0x1f;

  if (!count) {
    BX_NEXT_INSTR(i);
  }

  Bit32u temp_CF = getB_CF();

  if (count==1) {
    result_32 = (op1_32 >> 1) | (temp_CF << 31);
  }
  else {
    result_32 = (op1_32 >> count) | (temp_CF << (32 - count)) |
                (op1_32 << (33 - count));
  }

  write_RMW_linear_dword(result_32);

  cf = (op1_32 >> (count - 1)) & 0x1;
  of = ((result_32 << 1) ^ result_32) >> 31; // of = result30 ^ result31
  BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SHL_EdM(bxInstruction_c *i)
{
  unsigned count;
  unsigned cf, of;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_32 = read_RMW_virtual_dword(i->seg(), eaddr);

  if (i->getIaOpcode() == BX_IA_SHL_Ed)
    count = CL;
  else
    count = i->Ib();

  count &= 0x1f;

  if (count) {
    /* count < 32, since only lower 5 bits used */
    Bit32u result_32 = (op1_32 << count);

    write_RMW_linear_dword(result_32);

    cf = (op1_32 >> (32 - count)) & 0x1;
    of = cf ^ (result_32 >> 31);
    SET_FLAGS_OSZAPC_LOGIC_32(result_32);
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SHR_EdM(bxInstruction_c *i)
{
  unsigned count;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_32 = read_RMW_virtual_dword(i->seg(), eaddr);

  if (i->getIaOpcode() == BX_IA_SHR_Ed)
    count = CL;
  else
    count = i->Ib();

  count &= 0x1f;

  if (count) {
    Bit32u result_32 = (op1_32 >> count);

    write_RMW_linear_dword(result_32);

    unsigned cf = (op1_32 >> (count - 1)) & 0x1;
    // note, that of == result31 if count == 1 and
    //            of == 0        if count >= 2
    unsigned of = ((result_32 << 1) ^ result_32) >> 31;

    SET_FLAGS_OSZAPC_LOGIC_32(result_32);
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SAR_EdM(bxInstruction_c *i)
{
  unsigned count;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit32u op1_32 = read_RMW_virtual_dword(i->seg(), eaddr);

  if (i->getIaOpcode() == BX_IA_SAR_Ed)
    count = CL;
  else
    count = i->Ib();

  count &= 0x1f;

  if (count) {
    /* count < 32, since only lower 5 bits used */
    Bit32u result_32 = ((Bit32s) op1_32) >> count;

    write_RMW_linear_dword(result_32);

    SET_FLAGS_OSZAPC_LOGIC_32(result_32);
    unsigned cf = (op1_32 >> (count - 1)) & 1;
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(0, cf); /* signed overflow cannot happen in SAR instruction */
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ROL_EdR(bxInstruction_c *i)
{
  Bit32u op1_32, result_32;
  unsigned count;
  unsigned bit0, bit31;

  if (i->getIaOpcode() == BX_IA_ROL_Ed)
    count = CL;
  else
    count = i->Ib();

  count &= 0x1f;

  if (!count) {
    BX_CLEAR_64BIT_HIGH(i->dst()); // always clear upper part of the register
  }
  else {
    op1_32 = BX_READ_32BIT_REG(i->dst());
    result_32 = rol32(op1_32, count);
    BX_WRITE_32BIT_REGZ(i->dst(), result_32);

    bit0  = (result_32 & 0x1);
    bit31 = (result_32 >> 31);
    // of = cf ^ result31
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(bit0 ^ bit31, bit0);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ROR_EdR(bxInstruction_c *i)
{
  Bit32u op1_32, result_32;
  unsigned count;
  unsigned bit31, bit30;

  if (i->getIaOpcode() == BX_IA_ROR_Ed)
    count = CL;
  else
    count = i->Ib();

  count &= 0x1f;

  if (!count) {
    BX_CLEAR_64BIT_HIGH(i->dst()); // always clear upper part of the register
  }
  else {
    op1_32 = BX_READ_32BIT_REG(i->dst());
    result_32 = ror32(op1_32, count);
    BX_WRITE_32BIT_REGZ(i->dst(), result_32);

    bit31 = (result_32 >> 31) & 1;
    bit30 = (result_32 >> 30) & 1;
    // of = result30 ^ result31
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(bit30 ^ bit31, bit31);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::RCL_EdR(bxInstruction_c *i)
{
  Bit32u result_32;
  unsigned count;
  unsigned cf, of;

  if (i->getIaOpcode() == BX_IA_RCL_Ed)
    count = CL;
  else
    count = i->Ib();

  count &= 0x1f;
  if (!count) {
    BX_CLEAR_64BIT_HIGH(i->dst()); // always clear upper part of the register
    BX_NEXT_INSTR(i);
  }

  Bit32u op1_32 = BX_READ_32BIT_REG(i->dst());

  Bit32u temp_CF = getB_CF();

  if (count==1) {
    result_32 = (op1_32 << 1) | temp_CF;
  }
  else {
    result_32 = (op1_32 << count) | (temp_CF << (count - 1)) |
                (op1_32 >> (33 - count));
  }

  BX_WRITE_32BIT_REGZ(i->dst(), result_32);

  cf = (op1_32 >> (32 - count)) & 0x1;
  of = cf ^ (result_32 >> 31); // of = cf ^ result31
  BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::RCR_EdR(bxInstruction_c *i)
{
  Bit32u result_32;
  unsigned count;
  unsigned cf, of;

  if (i->getIaOpcode() == BX_IA_RCR_Ed)
    count = CL;
  else
    count = i->Ib();

  count &= 0x1f;

  if (!count) {
    BX_CLEAR_64BIT_HIGH(i->dst()); // always clear upper part of the register
    BX_NEXT_INSTR(i);
  }

  Bit32u op1_32 = BX_READ_32BIT_REG(i->dst());

  Bit32u temp_CF = getB_CF();

  if (count==1) {
    result_32 = (op1_32 >> 1) | (temp_CF << 31);
  }
  else {
    result_32 = (op1_32 >> count) | (temp_CF << (32 - count)) |
                (op1_32 << (33 - count));
  }

  BX_WRITE_32BIT_REGZ(i->dst(), result_32);

  cf = (op1_32 >> (count - 1)) & 0x1;
  of = ((result_32 << 1) ^ result_32) >> 31; // of = result30 ^ result31
  BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADD_GqEqR(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64, sum_64;

  op1_64 = BX_READ_64BIT_REG(i->dst());
  op2_64 = BX_READ_64BIT_REG(i->src());
  sum_64 = op1_64 + op2_64;
  BX_WRITE_64BIT_REG(i->dst(), sum_64);

  SET_FLAGS_OSZAPC_ADD_64(op1_64, op2_64, sum_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADC_GqEqR(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64, sum_64;

  op1_64 = BX_READ_64BIT_REG(i->dst());
  op2_64 = BX_READ_64BIT_REG(i->src());
  sum_64 = op1_64 + op2_64 + getB_CF();

  BX_WRITE_64BIT_REG(i->dst(), sum_64);

  SET_FLAGS_OSZAPC_ADD_64(op1_64, op2_64, sum_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SBB_GqEqR(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64, diff_64;

  op1_64 = BX_READ_64BIT_REG(i->dst());
  op2_64 = BX_READ_64BIT_REG(i->src());
  diff_64 = op1_64 - (op2_64 + getB_CF());

  BX_WRITE_64BIT_REG(i->dst(), diff_64);

  SET_FLAGS_OSZAPC_SUB_64(op1_64, op2_64, diff_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::AND_GqEqR(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64;

  op1_64 = BX_READ_64BIT_REG(i->dst());
  op2_64 = BX_READ_64BIT_REG(i->src());
  op1_64 &= op2_64;

  BX_WRITE_64BIT_REG(i->dst(), op1_64);

  SET_FLAGS_OSZAPC_LOGIC_64(op1_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SUB_GqEqR(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64, diff_64;

  op1_64 = BX_READ_64BIT_REG(i->dst());
  op2_64 = BX_READ_64BIT_REG(i->src());
  diff_64 = op1_64 - op2_64;

  BX_WRITE_64BIT_REG(i->dst(), diff_64);

  SET_FLAGS_OSZAPC_SUB_64(op1_64, op2_64, diff_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::XOR_GqEqR(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64;

  op1_64 = BX_READ_64BIT_REG(i->dst());
  op2_64 = BX_READ_64BIT_REG(i->src());
  op1_64 ^= op2_64;

  BX_WRITE_64BIT_REG(i->dst(), op1_64);

  SET_FLAGS_OSZAPC_LOGIC_64(op1_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMP_GqEqR(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64, diff_64;

  op1_64 = BX_READ_64BIT_REG(i->dst());
  op2_64 = BX_READ_64BIT_REG(i->src());
  diff_64 = op1_64 - op2_64;

  SET_FLAGS_OSZAPC_SUB_64(op1_64, op2_64, diff_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADD_GqEqM(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64, sum_64;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  op1_64 = BX_READ_64BIT_REG(i->dst());
  op2_64 = read_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));
  sum_64 = op1_64 + op2_64;
  BX_WRITE_64BIT_REG(i->dst(), sum_64);

  SET_FLAGS_OSZAPC_ADD_64(op1_64, op2_64, sum_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADC_GqEqM(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64, sum_64;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  op1_64 = BX_READ_64BIT_REG(i->dst());
  op2_64 = read_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));
  sum_64 = op1_64 + op2_64 + getB_CF();

  BX_WRITE_64BIT_REG(i->dst(), sum_64);

  SET_FLAGS_OSZAPC_ADD_64(op1_64, op2_64, sum_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SBB_GqEqM(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64, diff_64;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  op1_64 = BX_READ_64BIT_REG(i->dst());
  op2_64 = read_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));
  diff_64 = op1_64 - (op2_64 + getB_CF());

  BX_WRITE_64BIT_REG(i->dst(), diff_64);

  SET_FLAGS_OSZAPC_SUB_64(op1_64, op2_64, diff_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::AND_GqEqM(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  op1_64 = BX_READ_64BIT_REG(i->dst());
  op2_64 = read_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));
  op1_64 &= op2_64;

  BX_WRITE_64BIT_REG(i->dst(), op1_64);

  SET_FLAGS_OSZAPC_LOGIC_64(op1_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SUB_GqEqM(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64, diff_64;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  op1_64 = BX_READ_64BIT_REG(i->dst());
  op2_64 = read_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));
  diff_64 = op1_64 - op2_64;

  BX_WRITE_64BIT_REG(i->dst(), diff_64);

  SET_FLAGS_OSZAPC_SUB_64(op1_64, op2_64, diff_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::XOR_GqEqM(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  op1_64 = BX_READ_64BIT_REG(i->dst());
  op2_64 = read_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));
  op1_64 ^= op2_64;

  BX_WRITE_64BIT_REG(i->dst(), op1_64);

  SET_FLAGS_OSZAPC_LOGIC_64(op1_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMP_GqEqM(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64, diff_64;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  op1_64 = BX_READ_64BIT_REG(i->dst());
  op2_64 = read_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));
  diff_64 = op1_64 - op2_64;

  SET_FLAGS_OSZAPC_SUB_64(op1_64, op2_64, diff_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADD_EqGqM(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64, sum_64;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  /* pointer, segment address pair */
  op1_64 = read_RMW_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));
  op2_64 = BX_READ_64BIT_REG(i->src());
  sum_64 = op1_64 + op2_64;
  write_RMW_linear_qword(sum_64);

  SET_FLAGS_OSZAPC_ADD_64(op1_64, op2_64, sum_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADC_EqGqM(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64, sum_64;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  /* pointer, segment address pair */
  op1_64 = read_RMW_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));
  op2_64 = BX_READ_64BIT_REG(i->src());
  sum_64 = op1_64 + op2_64 + getB_CF();
  write_RMW_linear_qword(sum_64);

  SET_FLAGS_OSZAPC_ADD_64(op1_64, op2_64, sum_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SBB_EqGqM(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64, diff_64;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  /* pointer, segment address pair */
  op1_64 = read_RMW_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));
  op2_64 = BX_READ_64BIT_REG(i->src());
  diff_64 = op1_64 - (op2_64 + getB_CF());
  write_RMW_linear_qword(diff_64);

  SET_FLAGS_OSZAPC_SUB_64(op1_64, op2_64, diff_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::AND_EqGqM(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  op1_64 = read_RMW_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));
  op2_64 = BX_READ_64BIT_REG(i->src());
  op1_64 &= op2_64;
  write_RMW_linear_qword(op1_64);

  SET_FLAGS_OSZAPC_LOGIC_64(op1_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SUB_EqGqM(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64, diff_64;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  /* pointer, segment address pair */
  op1_64 = read_RMW_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));
  op2_64 = BX_READ_64BIT_REG(i->src());
  diff_64 = op1_64 - op2_64;
  write_RMW_linear_qword(diff_64);

  SET_FLAGS_OSZAPC_SUB_64(op1_64, op2_64, diff_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::XOR_EqGqM(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  op1_64 = read_RMW_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));
  op2_64 = BX_READ_64BIT_REG(i->src());
  op1_64 ^= op2_64;
  write_RMW_linear_qword(op1_64);

  SET_FLAGS_OSZAPC_LOGIC_64(op1_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMP_EqGqM(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64, diff_64;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  op1_64 = read_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));
  op2_64 = BX_READ_64BIT_REG(i->src());
  diff_64 = op1_64 - op2_64;

  SET_FLAGS_OSZAPC_SUB_64(op1_64, op2_64, diff_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADD_EqIdM(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64, sum_64;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  /* pointer, segment address pair */
  op1_64 = read_RMW_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));
  op2_64 = (Bit32s) i->Id();
  sum_64 = op1_64 + op2_64;
  write_RMW_linear_qword(sum_64);

  SET_FLAGS_OSZAPC_ADD_64(op1_64, op2_64, sum_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADC_EqIdM(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64, sum_64;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  /* pointer, segment address pair */
  op1_64 = read_RMW_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));
  op2_64 = (Bit32s) i->Id();
  sum_64 = op1_64 + op2_64 + getB_CF();
  write_RMW_linear_qword(sum_64);

  SET_FLAGS_OSZAPC_ADD_64(op1_64, op2_64, sum_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SBB_EqIdM(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64, diff_64;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  /* pointer, segment address pair */
  op1_64 = read_RMW_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));
  op2_64 = (Bit32s) i->Id();
  diff_64 = op1_64 - (op2_64 + getB_CF());
  write_RMW_linear_qword(diff_64);

  SET_FLAGS_OSZAPC_SUB_64(op1_64, op2_64, diff_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::AND_EqIdM(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64 = (Bit32s) i->Id();

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  op1_64 = read_RMW_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));
  op1_64 &= op2_64;
  write_RMW_linear_qword(op1_64);

  SET_FLAGS_OSZAPC_LOGIC_64(op1_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SUB_EqIdM(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64, diff_64;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  /* pointer, segment address pair */
  op1_64 = read_RMW_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));
  op2_64 = (Bit32s) i->Id();
  diff_64 = op1_64 - op2_64;
  write_RMW_linear_qword(diff_64);

  SET_FLAGS_OSZAPC_SUB_64(op1_64, op2_64, diff_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::XOR_EqIdM(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64 = (Bit32s) i->Id();

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  op1_64 = read_RMW_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));
  op1_64 ^= op2_64;
  write_RMW_linear_qword(op1_64);

  SET_FLAGS_OSZAPC_LOGIC_64(op1_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMP_EqIdM(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64, diff_64;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  op1_64 = read_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));
  op2_64 = (Bit32s) i->Id();
  diff_64 = op1_64 - op2_64;

  SET_FLAGS_OSZAPC_SUB_64(op1_64, op2_64, diff_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADD_EqIdR(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64, sum_64;

  op1_64 = BX_READ_64BIT_REG(i->dst());
  op2_64 = (Bit32s) i->Id();
  sum_64 = op1_64 + op2_64;
  BX_WRITE_64BIT_REG(i->dst(), sum_64);

  SET_FLAGS_OSZAPC_ADD_64(op1_64, op2_64, sum_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADC_EqIdR(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64, sum_64;

  op1_64 = BX_READ_64BIT_REG(i->dst());
  op2_64 = (Bit32s) i->Id();
  sum_64 = op1_64 + op2_64 + getB_CF();
  BX_WRITE_64BIT_REG(i->dst(), sum_64);

  SET_FLAGS_OSZAPC_ADD_64(op1_64, op2_64, sum_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SBB_EqIdR(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64, diff_64;

  op1_64 = BX_READ_64BIT_REG(i->dst());
  op2_64 = (Bit32s) i->Id();
  diff_64 = op1_64 - (op2_64 + getB_CF());
  BX_WRITE_64BIT_REG(i->dst(), diff_64);

  SET_FLAGS_OSZAPC_SUB_64(op1_64, op2_64, diff_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::AND_EqIdR(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64 = (Bit32s) i->Id();

  op1_64 = BX_READ_64BIT_REG(i->dst());
  op1_64 &= op2_64;
  BX_WRITE_64BIT_REG(i->dst(), op1_64);

  SET_FLAGS_OSZAPC_LOGIC_64(op1_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SUB_EqIdR(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64, diff_64;

  op1_64 = BX_READ_64BIT_REG(i->dst());
  op2_64 = (Bit32s) i->Id();
  diff_64 = op1_64 - op2_64;
  BX_WRITE_64BIT_REG(i->dst(), diff_64);

  SET_FLAGS_OSZAPC_SUB_64(op1_64, op2_64, diff_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::XOR_EqIdR(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64 = (Bit32s) i->Id();

  op1_64 = BX_READ_64BIT_REG(i->dst());
  op1_64 ^= op2_64;
  BX_WRITE_64BIT_REG(i->dst(), op1_64);

  SET_FLAGS_OSZAPC_LOGIC_64(op1_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMP_EqIdR(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64, diff_64;

  op1_64 = BX_READ_64BIT_REG(i->dst());
  op2_64 = (Bit32s) i->Id();
  diff_64 = op1_64 - op2_64;

  SET_FLAGS_OSZAPC_SUB_64(op1_64, op2_64, diff_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ROL_EqM(bxInstruction_c *i)
{
  unsigned count;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  Bit64u op1_64 = read_RMW_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));

  if (i->getIaOpcode() == BX_IA_ROL_Eq)
    count = CL;
  else
    count = i->Ib();

  count &= 0x3f;

  if (count) {
    Bit64u result_64 = rol64(op1_64, count);

    write_RMW_linear_qword(result_64);

    unsigned bit0  = (result_64 & 0x1);
    unsigned bit63 = (result_64 >> 63);
    // of = cf ^ result63
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(bit0 ^ bit63, bit0);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ROR_EqM(bxInstruction_c *i)
{
  unsigned count;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  Bit64u op1_64 = read_RMW_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));

  if (i->getIaOpcode() == BX_IA_ROR_Eq)
    count = CL;
  else
    count = i->Ib();

  count &= 0x3f;

  if (count) {
    Bit64u result_64 = ror64(op1_64, count);

    write_RMW_linear_qword(result_64);

    unsigned bit63 = (result_64 >> 63) & 1;
    unsigned bit62 = (result_64 >> 62) & 1;
    // of = result62 ^ result63
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(bit62 ^ bit63, bit63);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::RCL_EqM(bxInstruction_c *i)
{
  Bit64u result_64;
  unsigned count;
  unsigned cf, of;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  Bit64u op1_64 = read_RMW_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));

  if (i->getIaOpcode() == BX_IA_RCL_Eq)
    count = CL;
  else
    count = i->Ib();

  count &= 0x3f;

  if (!count) {
    BX_NEXT_INSTR(i);
  }

  Bit64u temp_CF = getB_CF();

  if (count==1) {
    result_64 = (op1_64 << 1) | temp_CF;
  }
  else {
    result_64 = (op1_64 << count) | (temp_CF << (count - 1)) |
                (op1_64 >> (65 - count));
  }

  write_RMW_linear_qword(result_64);

  cf = (op1_64 >> (64 - count)) & 0x1;
  of = cf ^ (result_64 >> 63); // of = cf ^ result63
  BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::RCR_EqM(bxInstruction_c *i)
{
  Bit64u result_64;
  unsigned count;
  unsigned of, cf;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  Bit64u op1_64 = read_RMW_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));

  if (i->getIaOpcode() == BX_IA_RCR_Eq)
    count = CL;
  else
    count = i->Ib();

  count &= 0x3f;

  if (!count) {
    BX_NEXT_INSTR(i);
  }

  Bit64u temp_CF = getB_CF();

  if (count==1) {
    result_64 = (op1_64 >> 1) | (temp_CF << 63);
  }
  else {
    result_64 = (op1_64 >> count) | (temp_CF << (64 - count)) |
                (op1_64 << (65 - count));
  }

  write_RMW_linear_qword(result_64);

  cf = (op1_64 >> (count - 1)) & 0x1;
  of = ((result_64 << 1) ^ result_64) >> 63;
  BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SHL_EqM(bxInstruction_c *i)
{
  unsigned count;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  Bit64u op1_64 = read_RMW_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));

  if (i->getIaOpcode() == BX_IA_SHL_Eq)
    count = CL;
  else
    count = i->Ib();

  count &= 0x3f;

  if (count) {
    /* count < 64, since only lower 6 bits used */
    Bit64u result_64 = (op1_64 << count);

    unsigned cf = (op1_64 >> (64 - count)) & 0x1;
    unsigned of = cf ^ (result_64 >> 63);

    write_RMW_linear_qword(result_64);

    SET_FLAGS_OSZAPC_LOGIC_64(result_64);
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SHR_EqM(bxInstruction_c *i)
{
  unsigned count;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  Bit64u op1_64 = read_RMW_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));

  if (i->getIaOpcode() == BX_IA_SHR_Eq)
    count = CL;
  else
    count = i->Ib();

  count &= 0x3f;

  if (count) {
    Bit64u result_64 = (op1_64 >> count);

    write_RMW_linear_qword(result_64);

    unsigned cf = (op1_64 >> (count - 1)) & 0x1;
    // note, that of == result63 if count == 1 and
    //            of == 0        if count >= 2
    unsigned of = ((result_64 << 1) ^ result_64) >> 63;

    SET_FLAGS_OSZAPC_LOGIC_64(result_64);
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SAR_EqM(bxInstruction_c *i)
{
  unsigned count;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  Bit64u op1_64 = read_RMW_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));

  if (i->getIaOpcode() == BX_IA_SAR_Eq)
    count = CL;
  else
    count = i->Ib();

  count &= 0x3f;

  if (count) {
    /* count < 64, since only lower 6 bits used */
    Bit64u result_64 = ((Bit64s) op1_64) >> count;

    write_RMW_linear_qword(result_64);

    SET_FLAGS_OSZAPC_LOGIC_64(result_64);
    unsigned cf = (op1_64 >> (count - 1)) & 1;
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(0, cf); /* signed overflow cannot happen in SAR instruction */
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ROL_EqR(bxInstruction_c *i)
{
  unsigned count;

  if (i->getIaOpcode() == BX_IA_ROL_Eq)
    count = CL;
  else
    count = i->Ib();

  count &= 0x3f;

  if (count) {
    Bit64u op1_64 = BX_READ_64BIT_REG(i->dst());
    Bit64u result_64 = rol64(op1_64, count);
    BX_WRITE_64BIT_REG(i->dst(), result_64);

    unsigned bit0  = (result_64 & 0x1);
    unsigned bit63 = (result_64 >> 63);
    // of = cf ^ result63
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(bit0 ^ bit63, bit0);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ROR_EqR(bxInstruction_c *i)
{
  unsigned count;

  if (i->getIaOpcode() == BX_IA_ROR_Eq)
    count = CL;
  else
    count = i->Ib();

  count &= 0x3f;

  if (count) {
    Bit64u op1_64 = BX_READ_64BIT_REG(i->dst());
    Bit64u result_64 = ror64(op1_64, count);
    BX_WRITE_64BIT_REG(i->dst(), result_64);

    unsigned bit63 = (result_64 >> 63) & 1;
    unsigned bit62 = (result_64 >> 62) & 1;
    // of = result62 ^ result63
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(bit62 ^ bit63, bit63);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::RCL_EqR(bxInstruction_c *i)
{
  Bit64u result_64;
  unsigned count;
  unsigned cf, of;

  if (i->getIaOpcode() == BX_IA_RCL_Eq)
    count = CL;
  else
    count = i->Ib();

  count &= 0x3f;

  if (!count) {
    BX_NEXT_INSTR(i);
  }

  Bit64u op1_64 = BX_READ_64BIT_REG(i->dst());

  Bit64u temp_CF = getB_CF();

  if (count==1) {
    result_64 = (op1_64 << 1) | temp_CF;
  }
  else {
    result_64 = (op1_64 << count) | (temp_CF << (count - 1)) |
                (op1_64 >> (65 - count));
  }

  BX_WRITE_64BIT_REG(i->dst(), result_64);

  cf = (op1_64 >> (64 - count)) & 0x1;
  of = cf ^ (result_64 >> 63); // of = cf ^ result63
  BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::RCR_EqR(bxInstruction_c *i)
{
  Bit64u result_64;
  unsigned count;
  unsigned of, cf;

  if (i->getIaOpcode() == BX_IA_RCR_Eq)
    count = CL;
  else
    count = i->Ib();

  count &= 0x3f;

  if (!count) {
    BX_NEXT_INSTR(i);
  }

  Bit64u op1_64 = BX_READ_64BIT_REG(i->dst());

  Bit64u temp_CF = getB_CF();

  if (count==1) {
    result_64 = (op1_64 >> 1) | (temp_CF << 63);
  }
  else {
    result_64 = (op1_64 >> count) | (temp_CF << (64 - count)) |
                (op1_64 << (65 - count));
  }

  BX_WRITE_64BIT_REG(i->dst(), result_64);

  cf = (op1_64 >> (count - 1)) & 0x1;
  of = ((result_64 << 1) ^ result_64) >> 63;
  BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SHL_EqR(bxInstruction_c *i)
{
  Bit64u op1_64, result_64;
  unsigned count;
  unsigned cf, of;

  if (i->getIaOpcode() == BX_IA_SHL_Eq)
    count = CL;
  else
    count = i->Ib();

  count &= 0x3f;

  if (count) {
    op1_64 = BX_READ_64BIT_REG(i->dst());
    /* count < 64, since only lower 6 bits used */
    result_64 = (op1_64 << count);
    BX_WRITE_64BIT_REG(i->dst(), result_64);

    cf = (op1_64 >> (64 - count)) & 0x1;
    of = cf ^ (result_64 >> 63);
    SET_FLAGS_OSZAPC_LOGIC_64(result_64);
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SHR_EqR(bxInstruction_c *i)
{
  unsigned count;

  if (i->getIaOpcode() == BX_IA_SHR_Eq)
    count = CL;
  else
    count = i->Ib();

  count &= 0x3f;

  if (count) {
    Bit64u op1_64 = BX_READ_64BIT_REG(i->dst());
    Bit64u result_64 = (op1_64 >> count);
    BX_WRITE_64BIT_REG(i->dst(), result_64);

    unsigned cf = (op1_64 >> (count - 1)) & 0x1;
    // note, that of == result63 if count == 1 and
    //            of == 0        if count >= 2
    unsigned of = ((result_64 << 1) ^ result_64) >> 63;

    SET_FLAGS_OSZAPC_LOGIC_64(result_64);
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(of, cf);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SAR_EqR(bxInstruction_c *i)
{
  unsigned count;

  if (i->getIaOpcode() == BX_IA_SAR_Eq)
    count = CL;
  else
    count = i->Ib();

  count &= 0x3f;

  if (count) {
    Bit64u op1_64 = BX_READ_64BIT_REG(i->dst());

    /* count < 64, since only lower 6 bits used */
    Bit64u result_64 = ((Bit64s) op1_64) >> count;

    BX_WRITE_64BIT_REG(i->dst(), result_64);

    SET_FLAGS_OSZAPC_LOGIC_64(result_64);
    unsigned cf = (op1_64 >> (count - 1)) & 1;
    BX_CPU_THIS_PTR oszapc.set_flags_OxxxxC(0, cf); /* signed overflow cannot happen in SAR instruction */
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::DAA(bxInstruction_c *i)
{
  Bit8u tmpAL = AL;
  int   tmpCF = 0, tmpAF = 0;

  /* Validated against Intel Pentium family hardware. */

  // DAA affects the following flags: S,Z,A,P,C

  if (((tmpAL & 0x0F) > 0x09) || get_AF())
  {
    tmpCF = ((AL > 0xF9) || get_CF());
    AL = AL + 0x06;
    tmpAF = 1;
  }

  if ((tmpAL > 0x99) || get_CF())
  {
    AL = AL + 0x60;
    tmpCF = 1;
  }

  SET_FLAGS_OSZAPC_LOGIC_8(AL);
  set_CF(tmpCF);
  set_AF(tmpAF);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::DAS(bxInstruction_c *i)
{
  /* The algorithm for DAS is fashioned after the pseudo code in the
   * Pentium Processor Family Developer's Manual, volume 3.  It seems
   * to have changed from earlier processor's manuals.  I'm not sure
   * if this is a correction in the algorithm printed, or Intel has
   * changed the handling of instruction. Validated against Intel
   * Pentium family hardware.
   */

  Bit8u tmpAL = AL;
  int tmpCF = 0, tmpAF = 0;

  /* DAS effect the following flags: A,C,S,Z,P */

  if (((tmpAL & 0x0F) > 0x09) || get_AF())
  {
    tmpCF = (AL < 0x06) || get_CF();
    AL = AL - 0x06;
    tmpAF = 1;
  }

  if ((tmpAL > 0x99) || get_CF())
  {
    AL = AL - 0x60;
    tmpCF = 1;
  }

  SET_FLAGS_OSZAPC_LOGIC_8(AL);
  set_CF(tmpCF);
  set_AF(tmpAF);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::AAA(bxInstruction_c *i)
{
  int tmpCF = 0, tmpAF = 0;

  /*
   *  Note: This instruction incorrectly documented in Intel's materials.
   *        The right description is:
   *
   *    IF (((AL and 0FH) > 9) or (AF==1)
   *    THEN
   *        IF CPU<286 THEN {  AL <- AL+6 }
   *                   ELSE {  AX <- AX+6 }
   *        AH <- AH+1
   *        CF <- 1
   *        AF <- 1
   *    ELSE
   *        CF <- 0
   *        AF <- 0
   *    ENDIF
   *	AL <- AL and 0Fh
   */

  /* Validated against Intel Pentium family hardware. */

  /* AAA affects the following flags: A,C */

  if (((AL & 0x0f) > 9) || get_AF())
  {
    AX = AX + 0x106;
    tmpAF = tmpCF = 1;
  }

  AL = AL & 0x0f;

  /* AAA affects also the following flags: Z,S,O,P */
  /* modification of the flags is undocumented */

  /* The following behaviour seems to match the P6 and
     its derived processors. */
  SET_FLAGS_OSZAPC_LOGIC_8(AL);
  set_CF(tmpCF);
  set_AF(tmpAF);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::AAS(bxInstruction_c *i)
{
  int tmpCF = 0, tmpAF = 0;

  /* AAS affects the following flags: A,C */

  if (((AL & 0x0F) > 0x09) || get_AF())
  {
    AX = AX - 0x106;
    tmpAF = tmpCF = 1;
  }

  AL = AL & 0x0f;

  /* AAS affects also the following flags: Z,S,O,P */
  /* modification of the flags is undocumented */

  /* The following behaviour seems to match the P6 and
     its derived processors. */
  SET_FLAGS_OSZAPC_LOGIC_8(AL);
  set_CF(tmpCF);
  set_AF(tmpAF);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::AAM(bxInstruction_c *i)
{
  Bit8u al, imm8 = i->Ib();

  if (imm8 == 0)
    exception(BX_DE_EXCEPTION, 0);

  al = AL;
  AH = al / imm8;
  AL = al % imm8;

  /* modification of flags A,C,O is undocumented */
  /* The following behaviour seems to match the P6 and
     its derived processors. */
  SET_FLAGS_OSZAPC_LOGIC_8(AL);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::AAD(bxInstruction_c *i)
{
  Bit16u tmp = AH;
  tmp *= i->Ib();
  tmp += AL;

  AX = (tmp & 0xff);

  /* modification of flags A,C,O is undocumented */
  /* The following behaviour seems to match the P6 and
     its derived processors. */
  SET_FLAGS_OSZAPC_LOGIC_8(AL);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ARPL_EwGw(bxInstruction_c *i)
{
  Bit16u op2_16, op1_16;

  if (! protected_mode()) {
    exception(BX_UD_EXCEPTION, 0);
  }

  /* op1_16 is a register or memory reference */
  if (i->modC0()) {
    op1_16 = BX_READ_16BIT_REG(i->dst());
  }
  else {
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);
    /* pointer, segment address pair */
    op1_16 = read_RMW_virtual_word(i->seg(), eaddr);
  }

  op2_16 = BX_READ_16BIT_REG(i->src());

  if ((op1_16 & 0x03) < (op2_16 & 0x03)) {
    op1_16 = (op1_16 & 0xfffc) | (op2_16 & 0x03);
    /* now write back to destination */
    if (i->modC0()) {
      BX_WRITE_16BIT_REG(i->dst(), op1_16);
    }
    else {
      write_RMW_linear_word(op1_16);
    }
    assert_ZF();
  }
  else {
    clear_ZF();
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BOUND_GwMa(bxInstruction_c *i)
{
  Bit16s op1_16 = BX_READ_16BIT_REG(i->dst());

  Bit32u eaddr = (Bit32u) BX_CPU_RESOLVE_ADDR_32(i);

  Bit16s bound_min = (Bit16s) read_virtual_word_32(i->seg(), eaddr);
  Bit16s bound_max = (Bit16s) read_virtual_word_32(i->seg(), (eaddr+2) & i->asize_mask());

  if (op1_16 < bound_min || op1_16 > bound_max) {
    exception(BX_BR_EXCEPTION, 0);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BOUND_GdMa(bxInstruction_c *i)
{
  Bit32s op1_32 = BX_READ_32BIT_REG(i->dst());

  Bit32u eaddr = (Bit32u) BX_CPU_RESOLVE_ADDR_32(i);

  Bit32s bound_min = (Bit32s) read_virtual_dword_32(i->seg(), eaddr);
  Bit32s bound_max = (Bit32s) read_virtual_dword_32(i->seg(), (eaddr+4) & i->asize_mask());

  if (op1_32 < bound_min || op1_32 > bound_max) {
    exception(BX_BR_EXCEPTION, 0);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_EwSwM(bxInstruction_c *i)
{
  /* Illegal to use nonexisting segments */
  if (i->src() >= 6) {
    exception(BX_UD_EXCEPTION, 0);
  }

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  Bit16u seg_reg = BX_CPU_THIS_PTR sregs[i->src()].selector.value;
  write_virtual_word(i->seg(), eaddr, seg_reg);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_ALOd(bxInstruction_c *i)
{
  AL = read_virtual_byte_32(i->seg(), i->Id());

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ENTER16_IwIb(bxInstruction_c *i)
{
  Bit16u imm16 = i->Iw();
  Bit8u level = i->Ib2();
  level &= 0x1F;

  RSP_SPECULATIVE;

  push_16(BP);
  Bit16u frame_ptr16 = SP;

  if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.d_b) {
    Bit32u ebp = EBP; // Use temp copy for case of exception.

    if (level > 0) {
      /* do level-1 times */
      while (--level) {
        ebp -= 2;
        Bit16u temp16 = stack_read_word(ebp);
        push_16(temp16);
      }

      /* push(frame pointer) */
      push_16(frame_ptr16);
    }

    ESP -= imm16;

    // ENTER finishes with memory write check on the final stack pointer
    // the memory is touched but no write actually occurs
    // emulate it by doing RMW read access from SS:ESP
    read_RMW_virtual_word_32(BX_SEG_REG_SS, ESP); // no lock, should be touch only

    BP = frame_ptr16;
  }
  else {
    Bit16u bp = BP;

    if (level > 0) {
      /* do level-1 times */
      while (--level) {
        bp -= 2;
        Bit16u temp16 = stack_read_word(bp);
        push_16(temp16);
      }

      /* push(frame pointer) */
      push_16(frame_ptr16);
    }

    SP -= imm16;

    // ENTER finishes with memory write check on the final stack pointer
    // the memory is touched but no write actually occurs
    // emulate it by doing RMW read access from SS:SP
    read_RMW_virtual_word_32(BX_SEG_REG_SS, SP); // no lock, should be touch only
  }

  BP = frame_ptr16;

  RSP_COMMIT;

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ENTER32_IwIb(bxInstruction_c *i)
{
  Bit16u imm16 = i->Iw();
  Bit8u level = i->Ib2();
  level &= 0x1F;

  RSP_SPECULATIVE;

  push_32(EBP);
  Bit32u frame_ptr32 = ESP;

  if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.d_b) {
    Bit32u ebp = EBP;  // Use temp copy for case of exception.

    if (level > 0) {
      /* do level-1 times */
      while (--level) {
        ebp -= 4;
        Bit32u temp32 = stack_read_dword(ebp);
        push_32(temp32);
      }

      /* push(frame pointer) */
      push_32(frame_ptr32);
    }

    ESP -= imm16;

    // ENTER finishes with memory write check on the final stack pointer
    // the memory is touched but no write actually occurs
    // emulate it by doing RMW read access from SS:ESP
    read_RMW_virtual_dword_32(BX_SEG_REG_SS, ESP); // no lock, should be touch only
  }
  else {
    Bit16u bp = BP;

    if (level > 0) {
      /* do level-1 times */
      while (--level) {
        bp -= 4;
        Bit32u temp32 = stack_read_dword(bp);
        push_32(temp32);
      }

      /* push(frame pointer) */
      push_32(frame_ptr32);
    }

    SP -= imm16;

    // ENTER finishes with memory write check on the final stack pointer
    // the memory is touched but no write actually occurs
    // emulate it by doing RMW read access from SS:SP
    read_RMW_virtual_dword_32(BX_SEG_REG_SS, SP); // no lock, should be touch only
  }

  EBP = frame_ptr32;

  RSP_COMMIT;

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LEAVE16(bxInstruction_c *i)
{

  Bit16u value16;

  if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.d_b) {
    value16 = stack_read_word(EBP);
    ESP = EBP + 2;
  }
  else {
    value16 = stack_read_word(BP);
    SP = BP + 2;
  }

  BP = value16;

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LEAVE32(bxInstruction_c *i)
{

  Bit32u value32;

  if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.d_b) {
    value32 = stack_read_dword(EBP);
    ESP = EBP + 4;
  }
  else {
    value32 = stack_read_dword(BP);
    SP = BP + 4;
  }

  EBP = value32;

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::INT1(bxInstruction_c *i)
{
  BX_INSTR_FAR_BRANCH_ORIGIN();

#if BX_SUPPORT_VMX
  VMexit_Event(BX_PRIVILEGED_SOFTWARE_INTERRUPT, BX_DB_EXCEPTION, 0, 0);
#endif

#if BX_SUPPORT_SVM
  if (BX_CPU_THIS_PTR in_svm_guest) {
    if (SVM_INTERCEPT(SVM_INTERCEPT1_ICEBP)) Svm_Vmexit(SVM_VMEXIT_ICEBP);
  }
#endif

  BX_CPU_THIS_PTR EXT = 1;

  // interrupt is not RSP safe
  interrupt(1, BX_PRIVILEGED_SOFTWARE_INTERRUPT, 0, 0);

  BX_INSTR_FAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_INT,
                      FAR_BRANCH_PREV_CS, FAR_BRANCH_PREV_RIP,
                      BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value, RIP);

  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SALC(bxInstruction_c *i)
{
  if (get_CF()) {
    AL = 0xff;
  }
  else {
    AL = 0x00;
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::XLAT(bxInstruction_c *i)
{
#if BX_SUPPORT_X86_64
  if (i->as64L()) {
    AL = read_linear_byte(i->seg(), get_laddr64(i->seg(), RBX + AL));
  }
  else
#endif
  {
    AL = read_virtual_byte(i->seg(), (EBX + AL) & i->asize_mask());
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LAR_GvEw(bxInstruction_c *i)
{
  /* for 16 bit operand size mode */
  Bit16u raw_selector;
  bx_descriptor_t descriptor;
  bx_selector_t selector;
  Bit32u dword1, dword2;
#if BX_SUPPORT_X86_64
  Bit32u dword3 = 0;
#endif

  if (! protected_mode()) {
    exception(BX_UD_EXCEPTION, 0);
  }

  if (i->modC0()) {
    raw_selector = BX_READ_16BIT_REG(i->src());
  }
  else {
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);
    /* pointer, segment address pair */
    raw_selector = read_virtual_word(i->seg(), eaddr);
  }

  /* if selector null, clear ZF and done */
  if ((raw_selector & 0xfffc) == 0) {
    clear_ZF();
    BX_NEXT_INSTR(i);
  }

  parse_selector(raw_selector, &selector);

  if (!fetch_raw_descriptor2(&selector, &dword1, &dword2)) {
    clear_ZF();
    BX_NEXT_INSTR(i);
  }

  parse_descriptor(dword1, dword2, &descriptor);

  if (descriptor.valid==0) {
    clear_ZF();
    BX_NEXT_INSTR(i);
  }

  /* if source selector is visible at CPL & RPL,
   * within the descriptor table, and of type accepted by LAR instruction,
   * then load register with segment limit and set ZF
   */

  if (descriptor.segment) { /* normal segment */
    if (IS_CODE_SEGMENT(descriptor.type) && IS_CODE_SEGMENT_CONFORMING(descriptor.type)) {
      /* ignore DPL for conforming segments */
    }
    else {
      if (descriptor.dpl < CPL || descriptor.dpl < selector.rpl) {
        clear_ZF();
        BX_NEXT_INSTR(i);
      }
    }
  }
  else { /* system or gate segment */
    switch (descriptor.type) {
      case BX_SYS_SEGMENT_AVAIL_286_TSS:
      case BX_SYS_SEGMENT_BUSY_286_TSS:
      case BX_286_CALL_GATE:
      case BX_TASK_GATE:
        if (long_mode()) {
          clear_ZF();
          BX_NEXT_INSTR(i);
        }
        /* fall through */
      case BX_SYS_SEGMENT_LDT:
      case BX_SYS_SEGMENT_AVAIL_386_TSS:
      case BX_SYS_SEGMENT_BUSY_386_TSS:
      case BX_386_CALL_GATE:
#if BX_SUPPORT_X86_64
        if (long64_mode() || (descriptor.type == BX_386_CALL_GATE && long_mode()) ) {
          if (!fetch_raw_descriptor2_64(&selector, &dword1, &dword2, &dword3)) {
            clear_ZF();
            BX_NEXT_INSTR(i);
          }
        }
#endif
        break;
      default: /* rest not accepted types to LAR */
        clear_ZF();
        BX_NEXT_INSTR(i);
    }

    if (descriptor.dpl < CPL || descriptor.dpl < selector.rpl) {
      clear_ZF();
      BX_NEXT_INSTR(i);
    }
  }

  assert_ZF();
  if (i->os32L()) {
    /* masked by 00FxFF00, where x is undefined */
    BX_WRITE_32BIT_REGZ(i->dst(), dword2 & 0x00ffff00);
  }
  else {
    BX_WRITE_16BIT_REG(i->dst(), dword2 & 0xff00);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LSL_GvEw(bxInstruction_c *i)
{
  /* for 16 bit operand size mode */
  Bit16u raw_selector;
  Bit32u limit32;
  bx_selector_t selector;
  Bit32u dword1, dword2;
#if BX_SUPPORT_X86_64
  Bit32u dword3 = 0;
#endif

  if (! protected_mode()) {
    exception(BX_UD_EXCEPTION, 0);
  }

  if (i->modC0()) {
    raw_selector = BX_READ_16BIT_REG(i->src());
  }
  else {
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);
    /* pointer, segment address pair */
    raw_selector = read_virtual_word(i->seg(), eaddr);
  }

  /* if selector null, clear ZF and done */
  if ((raw_selector & 0xfffc) == 0) {
    clear_ZF();
    BX_NEXT_INSTR(i);
  }

  parse_selector(raw_selector, &selector);

  if (!fetch_raw_descriptor2(&selector, &dword1, &dword2)) {
    clear_ZF();
    BX_NEXT_INSTR(i);
  }

  Bit32u descriptor_dpl = (dword2 >> 13) & 0x03;

  if ((dword2 & 0x00001000) == 0) { // system segment
    Bit32u type = (dword2 >> 8) & 0x0000000f;
    switch (type) {
      case BX_SYS_SEGMENT_AVAIL_286_TSS:
      case BX_SYS_SEGMENT_BUSY_286_TSS:
        if (long_mode()) {
          clear_ZF();
          BX_NEXT_INSTR(i);
        }
        /* fall through */
      case BX_SYS_SEGMENT_LDT:
      case BX_SYS_SEGMENT_AVAIL_386_TSS:
      case BX_SYS_SEGMENT_BUSY_386_TSS:
#if BX_SUPPORT_X86_64
        if (long64_mode()) {
          if (!fetch_raw_descriptor2_64(&selector, &dword1, &dword2, &dword3)) {
            clear_ZF();
            BX_NEXT_INSTR(i);
          }
        }
#endif
        if (descriptor_dpl < CPL || descriptor_dpl < selector.rpl) {
          clear_ZF();
          BX_NEXT_INSTR(i);
        }
        limit32 = (dword1 & 0x0000ffff) | (dword2 & 0x000f0000);
        if (dword2 & 0x00800000)
          limit32 = (limit32 << 12) | 0x00000fff;
        break;
      default: /* rest not accepted types to LSL */
        clear_ZF();
        BX_NEXT_INSTR(i);
    }
  }
  else { // data & code segment
    limit32 = (dword1 & 0x0000ffff) | (dword2 & 0x000f0000);
    if (dword2 & 0x00800000)
      limit32 = (limit32 << 12) | 0x00000fff;
    if ((dword2 & 0x00000c00) != 0x00000c00) {
      // non-conforming code segment
      if (descriptor_dpl < CPL || descriptor_dpl < selector.rpl) {
        clear_ZF();
        BX_NEXT_INSTR(i);
      }
    }
  }

  /* all checks pass, limit32 is now byte granular, write to op1 */
  assert_ZF();

  if (i->os32L()) {
    BX_WRITE_32BIT_REGZ(i->dst(), limit32);
  }
  else {
    // chop off upper 16 bits
    BX_WRITE_16BIT_REG(i->dst(), (Bit16u) limit32);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_CR2Rd(bxInstruction_c *i)
{
  // CPL is always 0 in real mode
  if (/* !real_mode() && */ CPL!=0) {
    exception(BX_GP_EXCEPTION, 0);
  }

#if BX_SUPPORT_SVM
  if (BX_CPU_THIS_PTR in_svm_guest) {
    if(SVM_CR_WRITE_INTERCEPTED(2)) {
      if (BX_SUPPORT_SVM_EXTENSION(BX_CPUID_SVM_DECODE_ASSIST))
        Svm_Vmexit(SVM_VMEXIT_CR2_WRITE, BX_SVM_CR_WRITE_MASK | i->src());
      else
        Svm_Vmexit(SVM_VMEXIT_CR2_WRITE);
    }
  }
#endif

  BX_CPU_THIS_PTR cr2 = BX_READ_32BIT_REG(i->src());

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_RdCR2(bxInstruction_c *i)
{
  // CPL is always 0 in real mode
  if (/* !real_mode() && */ CPL!=0) {
    exception(BX_GP_EXCEPTION, 0);
  }

#if BX_SUPPORT_SVM
  if (BX_CPU_THIS_PTR in_svm_guest) {
    if(SVM_CR_READ_INTERCEPTED(2))
      Svm_Vmexit(SVM_VMEXIT_CR2_READ, BX_SUPPORT_SVM_EXTENSION(BX_CPUID_SVM_DECODE_ASSIST) ? i->dst() : 0);
  }
#endif

  BX_WRITE_32BIT_REGZ(i->dst(), (Bit32u) BX_CPU_THIS_PTR cr2);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_DdRd(bxInstruction_c *i)
{
#if BX_SUPPORT_VMX
  if (BX_CPU_THIS_PTR in_vmx_guest)
    VMexit_DR_Access(0 /* write */, i->dst(), i->src());
#endif

#if BX_CPU_LEVEL >= 5
  if (BX_CPU_THIS_PTR cr4.get_DE()) {
    if ((i->dst() & 0xE) == 4) {
      exception(BX_UD_EXCEPTION, 0);
    }
  }
#endif

  // Note: processor clears GD upon entering debug exception
  // handler, to allow access to the debug registers
  if (BX_CPU_THIS_PTR dr7.get_GD()) {
    BX_CPU_THIS_PTR debug_trap |= BX_DEBUG_DR_ACCESS_BIT;
    exception(BX_DB_EXCEPTION, 0);
  }

  // CPL is always 0 in real mode
  if (/* !real_mode() && */ CPL!=0) {
    exception(BX_GP_EXCEPTION, 0);
  }

#if BX_SUPPORT_SVM
  if (BX_CPU_THIS_PTR in_svm_guest) {
    if (SVM_DR_WRITE_INTERCEPTED(i->dst()))
      Svm_Vmexit(SVM_VMEXIT_DR0_WRITE + i->dst(), BX_SUPPORT_SVM_EXTENSION(BX_CPUID_SVM_DECODE_ASSIST) ? i->src() : 0);
  }
#endif

  invalidate_prefetch_q();

  Bit32u val_32 = BX_READ_32BIT_REG(i->src());

  switch (i->dst()) {
    case 0: // DR0
    case 1: // DR1
    case 2: // DR2
    case 3: // DR3
      BX_CPU_THIS_PTR dr[i->dst()] = val_32;
      TLB_invlpg(val_32);
      break;

    case 4: // DR4
      // DR4 aliased to DR6 by default. With Debug Extensions on,
      // access to DR4 causes #UD
    case 6: // DR6
#if BX_CPU_LEVEL <= 4
      // On 386/486 bit12 is settable
      BX_CPU_THIS_PTR dr6.val32 = (BX_CPU_THIS_PTR dr6.val32 & 0xffff0ff0) |
                            (val_32 & 0x0000f00f);
#else
      // On Pentium+, bit12 is always zero
      BX_CPU_THIS_PTR dr6.val32 = (BX_CPU_THIS_PTR dr6.val32 & 0xffff0ff0) |
                            (val_32 & 0x0000e00f);
#endif
      break;

    case 5: // DR5
      // DR5 aliased to DR7 by default. With Debug Extensions on,
      // access to DR5 causes #UD
    case 7: // DR7
      // Note: 486+ ignore GE and LE flags.  On the 386, exact
      // data breakpoint matching does not occur unless it is enabled
      // by setting the LE and/or GE flags.

#if BX_CPU_LEVEL <= 4
      // 386/486: you can play with all the bits except b10 is always 1
      BX_CPU_THIS_PTR dr7.set32(val_32 | 0x00000400);
#else
      // Pentium+: bits15,14,12 are hardwired to 0, rest are settable.
      // Even bits 11,10 are changeable though reserved.
      BX_CPU_THIS_PTR dr7.set32((val_32 & 0xffff2fff) | 0x00000400);
#endif

      TLB_flush(); // the DR7 write could enable some breakpoints
      break;

    default:
      exception(BX_UD_EXCEPTION, 0);
  }

  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_RdDd(bxInstruction_c *i)
{
  Bit32u val_32;

#if BX_SUPPORT_VMX
  if (BX_CPU_THIS_PTR in_vmx_guest)
    VMexit_DR_Access(1 /* read */, i->src(), i->dst());
#endif

#if BX_CPU_LEVEL >= 5
  if (BX_CPU_THIS_PTR cr4.get_DE()) {
    if ((i->src() & 0xE) == 4) {
      exception(BX_UD_EXCEPTION, 0);
    }
  }
#endif

  // Note: processor clears GD upon entering debug exception
  // handler, to allow access to the debug registers
  if (BX_CPU_THIS_PTR dr7.get_GD()) {
    BX_CPU_THIS_PTR debug_trap |= BX_DEBUG_DR_ACCESS_BIT;
    exception(BX_DB_EXCEPTION, 0);
  }

  // CPL is always 0 in real mode
  if (/* !real_mode() && */ CPL!=0) {
    exception(BX_GP_EXCEPTION, 0);
  }

#if BX_SUPPORT_SVM
  if (BX_CPU_THIS_PTR in_svm_guest) {
    if (SVM_DR_READ_INTERCEPTED(i->src()))
      Svm_Vmexit(SVM_VMEXIT_DR0_READ + i->src(), BX_SUPPORT_SVM_EXTENSION(BX_CPUID_SVM_DECODE_ASSIST) ? i->dst() : 0);
  }
#endif

  switch (i->src()) {
    case 0: // DR0
    case 1: // DR1
    case 2: // DR2
    case 3: // DR3
      val_32 = (Bit32u) BX_CPU_THIS_PTR dr[i->src()];
      break;

    case 4: // DR4
      // DR4 aliased to DR6 by default. With Debug Extensions ON,
      // access to DR4 causes #UD
    case 6: // DR6
      val_32 = BX_CPU_THIS_PTR dr6.get32();
      break;

    case 5: // DR5
      // DR5 aliased to DR7 by default. With Debug Extensions ON,
      // access to DR5 causes #UD
    case 7: // DR7
      val_32 = BX_CPU_THIS_PTR dr7.get32();
      break;

    default:
      exception(BX_UD_EXCEPTION, 0);
  }

  BX_WRITE_32BIT_REGZ(i->dst(), val_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LES_GwMp(bxInstruction_c *i)
{
  load_segw(i, BX_SEG_REG_ES);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LSS_GwMp(bxInstruction_c *i)
{
  load_segw(i, BX_SEG_REG_SS);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LFS_GwMp(bxInstruction_c *i)
{
  load_segw(i, BX_SEG_REG_FS);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LGS_GwMp(bxInstruction_c *i)
{
  load_segw(i, BX_SEG_REG_GS);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LES_GdMp(bxInstruction_c *i)
{
  load_segd(i, BX_SEG_REG_ES);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LDS_GdMp(bxInstruction_c *i)
{
  load_segd(i, BX_SEG_REG_DS);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LSS_GdMp(bxInstruction_c *i)
{
  load_segd(i, BX_SEG_REG_SS);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LFS_GdMp(bxInstruction_c *i)
{
  load_segd(i, BX_SEG_REG_FS);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LGS_GdMp(bxInstruction_c *i)
{
  load_segd(i, BX_SEG_REG_GS);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OR_GbEbR(bxInstruction_c *i)
{
  Bit8u op1, op2;

  op1 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());
  op2 = BX_READ_8BIT_REGx(i->src(), i->extend8bitL());
  op1 |= op2;
  BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), op1);

  SET_FLAGS_OSZAPC_LOGIC_8(op1);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OR_GbEbM(bxInstruction_c *i)
{
  Bit8u op1, op2;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op1 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());
  op2 = read_virtual_byte(i->seg(), eaddr);
  op1 |= op2;
  BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), op1);

  SET_FLAGS_OSZAPC_LOGIC_8(op1);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OR_EbIbM(bxInstruction_c *i)
{
  Bit8u op1, op2 = i->Ib();

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op1 = read_RMW_virtual_byte(i->seg(), eaddr);
  op1 |= op2;
  write_RMW_linear_byte(op1);

  SET_FLAGS_OSZAPC_LOGIC_8(op1);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OR_EbIbR(bxInstruction_c *i)
{
  Bit8u op1, op2 = i->Ib();

  op1 = BX_READ_8BIT_REGx(i->dst(), i->extend8bitL());
  op1 |= op2;
  BX_WRITE_8BIT_REGx(i->dst(), i->extend8bitL(), op1);

  SET_FLAGS_OSZAPC_LOGIC_8(op1);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OR_EbGbM(bxInstruction_c *i)
{
  Bit8u op1, op2;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op1 = read_RMW_virtual_byte(i->seg(), eaddr);
  op2 = BX_READ_8BIT_REGx(i->src(), i->extend8bitL());
  op1 |= op2;
  write_RMW_linear_byte(op1);

  SET_FLAGS_OSZAPC_LOGIC_8(op1);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OR_EwIwM(bxInstruction_c *i)
{
  Bit16u op1_16;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op1_16 = read_RMW_virtual_word(i->seg(), eaddr);
  op1_16 |= i->Iw();
  write_RMW_linear_word(op1_16);

  SET_FLAGS_OSZAPC_LOGIC_16(op1_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OR_EwIwR(bxInstruction_c *i)
{
  Bit16u op1_16 = BX_READ_16BIT_REG(i->dst());
  op1_16 |= i->Iw();
  BX_WRITE_16BIT_REG(i->dst(), op1_16);

  SET_FLAGS_OSZAPC_LOGIC_16(op1_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OR_EdIdM(bxInstruction_c *i)
{
  Bit32u op1_32;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op1_32 = read_RMW_virtual_dword(i->seg(), eaddr);
  op1_32 |= i->Id();
  write_RMW_linear_dword(op1_32);

  SET_FLAGS_OSZAPC_LOGIC_32(op1_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OR_EdIdR(bxInstruction_c *i)
{
  Bit32u op1_32 = BX_READ_32BIT_REG(i->dst());
  op1_32 |= i->Id();
  BX_WRITE_32BIT_REGZ(i->dst(), op1_32);

  SET_FLAGS_OSZAPC_LOGIC_32(op1_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OR_EwGwM(bxInstruction_c *i)
{
  Bit16u op1_16, op2_16;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op1_16 = read_RMW_virtual_word(i->seg(), eaddr);
  op2_16 = BX_READ_16BIT_REG(i->src());
  op1_16 |= op2_16;
  write_RMW_linear_word(op1_16);

  SET_FLAGS_OSZAPC_LOGIC_16(op1_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OR_EdGdM(bxInstruction_c *i)
{
  Bit32u op1_32, op2_32;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op1_32 = read_RMW_virtual_dword(i->seg(), eaddr);
  op2_32 = BX_READ_32BIT_REG(i->src());
  op1_32 |= op2_32;
  write_RMW_linear_dword(op1_32);

  SET_FLAGS_OSZAPC_LOGIC_32(op1_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OR_GwEwR(bxInstruction_c *i)
{
  Bit16u op1_16, op2_16;

  op1_16 = BX_READ_16BIT_REG(i->dst());
  op2_16 = BX_READ_16BIT_REG(i->src());
  op1_16 |= op2_16;
  BX_WRITE_16BIT_REG(i->dst(), op1_16);

  SET_FLAGS_OSZAPC_LOGIC_16(op1_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OR_GwEwM(bxInstruction_c *i)
{
  Bit16u op1_16, op2_16;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

  op1_16 = BX_READ_16BIT_REG(i->dst());
  op2_16 = read_virtual_word(i->seg(), eaddr);
  op1_16 |= op2_16;
  BX_WRITE_16BIT_REG(i->dst(), op1_16);

  SET_FLAGS_OSZAPC_LOGIC_16(op1_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OR_GdEdR(bxInstruction_c *i)
{
  Bit32u op1_32, op2_32;

  op1_32 = BX_READ_32BIT_REG(i->dst());
  op2_32 = BX_READ_32BIT_REG(i->src());
  op1_32 |= op2_32;
  BX_WRITE_32BIT_REGZ(i->dst(), op1_32);

  SET_FLAGS_OSZAPC_LOGIC_32(op1_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OR_GdEdM(bxInstruction_c *i)
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

void BX_CPP_AttrRegparmN(1) BX_CPU_C::IMUL_GdEdR(bxInstruction_c *i)
{
  Bit32s op1_32 = BX_READ_32BIT_REG(i->dst());
  Bit32s op2_32 = BX_READ_32BIT_REG(i->src());

  Bit64s product_64 = ((Bit64s) op1_32) * ((Bit64s) op2_32);
  Bit32u product_32 = (Bit32u)(product_64 & 0xFFFFFFFF);

  /* now write product back to destination */
  BX_WRITE_32BIT_REGZ(i->dst(), product_32);

  /* set eflags:
   * IMUL r32,r/m32: condition for clearing CF & OF:
   *   result exactly fits within r32
   */
  SET_FLAGS_OSZAPC_LOGIC_32(product_32);
  if(product_64 != (Bit32s) product_64)
  {
    BX_CPU_THIS_PTR oszapc.assert_flags_OxxxxC();
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::IMUL_GdEdIdR(bxInstruction_c *i)
{
  Bit32s op2_32 = BX_READ_32BIT_REG(i->src());
  Bit32s op3_32 = i->Id();

  Bit64s product_64 = ((Bit64s) op2_32) * ((Bit64s) op3_32);
  Bit32u product_32 = (Bit32u)(product_64 & 0xFFFFFFFF);

  /* now write product back to destination */
  BX_WRITE_32BIT_REGZ(i->dst(), product_32);

  /* set eflags:
   * IMUL r32,r/m32,imm32: condition for clearing CF & OF:
   *   result exactly fits within r32
   */
  SET_FLAGS_OSZAPC_LOGIC_32(product_32);
  if(product_64 != (Bit32s) product_64)
  {
    BX_CPU_THIS_PTR oszapc.assert_flags_OxxxxC();
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MUL_ALEbR(bxInstruction_c *i)
{
  Bit8u op1 = AL;
  Bit8u op2 = BX_READ_8BIT_REGx(i->src(), i->extend8bitL());

  Bit32u product_16 = ((Bit16u) op1) * ((Bit16u) op2);

  Bit8u product_8l = (product_16 & 0xFF);
  Bit8u product_8h =  product_16 >> 8;

  /* now write product back to destination */
  AX = product_16;

  /* set EFLAGS */
  SET_FLAGS_OSZAPC_LOGIC_8(product_8l);
  if(product_8h != 0)
  {
    BX_CPU_THIS_PTR oszapc.assert_flags_OxxxxC();
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::IMUL_ALEbR(bxInstruction_c *i)
{
  Bit8s op1 = AL;
  Bit8s op2 = BX_READ_8BIT_REGx(i->src(), i->extend8bitL());

  Bit16s product_16 = op1 * op2;
  Bit8u  product_8 = (product_16 & 0xFF);

  /* now write product back to destination */
  AX = product_16;

  /* set EFLAGS:
   * IMUL r/m8: condition for clearing CF & OF:
   *   AX = sign-extend of AL to 16 bits
   */

  SET_FLAGS_OSZAPC_LOGIC_8(product_8);
  if(product_16 != (Bit8s) product_16)
  {
    BX_CPU_THIS_PTR oszapc.assert_flags_OxxxxC();
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::DIV_ALEbR(bxInstruction_c *i)
{
  Bit8u op2 = BX_READ_8BIT_REGx(i->src(), i->extend8bitL());
  if (op2 == 0) {
    exception(BX_DE_EXCEPTION, 0);
  }

  Bit16u op1 = AX;

  Bit16u quotient_16 = op1 / op2;
  Bit8u remainder_8 = op1 % op2;
  Bit8u quotient_8l = quotient_16 & 0xFF;

  if (quotient_16 != quotient_8l)
  {
    exception(BX_DE_EXCEPTION, 0);
  }

  /* now write quotient back to destination */
  AL = quotient_8l;
  AH = remainder_8;

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::IDIV_ALEbR(bxInstruction_c *i)
{
  Bit16s op1 = AX;

  /* check MIN_INT case */
  if (op1 == ((Bit16s)0x8000))
    exception(BX_DE_EXCEPTION, 0);

  Bit8s op2 = BX_READ_8BIT_REGx(i->src(), i->extend8bitL());

  if (op2 == 0)
    exception(BX_DE_EXCEPTION, 0);

  Bit16s quotient_16 = op1 / op2;
  Bit8s remainder_8 = op1 % op2;
  Bit8s quotient_8l = quotient_16 & 0xFF;

  if (quotient_16 != quotient_8l)
    exception(BX_DE_EXCEPTION, 0);

  /* now write quotient back to destination */
  AL = quotient_8l;
  AH = remainder_8;

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MUL_EAXEdR(bxInstruction_c *i)
{
  Bit32u op1_32 = EAX;
  Bit32u op2_32 = BX_READ_32BIT_REG(i->src());

  Bit64u product_64  = ((Bit64u) op1_32) * ((Bit64u) op2_32);
  Bit32u product_32l = GET32L(product_64);
  Bit32u product_32h = GET32H(product_64);

  /* now write product back to destination */
  RAX = product_32l;
  RDX = product_32h;

  /* set EFLAGS */
  SET_FLAGS_OSZAPC_LOGIC_32(product_32l);
  if(product_32h != 0)
  {
    BX_CPU_THIS_PTR oszapc.assert_flags_OxxxxC();
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::IMUL_EAXEdR(bxInstruction_c *i)
{
  Bit32s op1_32 = EAX;
  Bit32s op2_32 = BX_READ_32BIT_REG(i->src());

  Bit64s product_64  = ((Bit64s) op1_32) * ((Bit64s) op2_32);
  Bit32u product_32l = GET32L(product_64);
  Bit32u product_32h = GET32H(product_64);

  /* now write product back to destination */
  RAX = product_32l;
  RDX = product_32h;

  /* set eflags:
   * IMUL r/m32: condition for clearing CF & OF:
   *   EDX:EAX = sign-extend of EAX
   */
  SET_FLAGS_OSZAPC_LOGIC_32(product_32l);
  if(product_64 != (Bit32s)product_64)
  {
    BX_CPU_THIS_PTR oszapc.assert_flags_OxxxxC();
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::DIV_EAXEdR(bxInstruction_c *i)
{
  Bit32u op2_32 = BX_READ_32BIT_REG(i->src());
  if (op2_32 == 0) {
    exception(BX_DE_EXCEPTION, 0);
  }

  Bit64u op1_64 = GET64_FROM_HI32_LO32(EDX, EAX);

  Bit64u quotient_64  = op1_64 / op2_32;
  Bit32u remainder_32 = (Bit32u) (op1_64 % op2_32);
  Bit32u quotient_32l = (Bit32u) (quotient_64 & 0xFFFFFFFF);

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

void BX_CPP_AttrRegparmN(1) BX_CPU_C::IDIV_EAXEdR(bxInstruction_c *i)
{
  Bit64s op1_64 = GET64_FROM_HI32_LO32(EDX, EAX);

  /* check MIN_INT case */
  if (op1_64 == ((Bit64s)BX_CONST64(0x8000000000000000)))
    exception(BX_DE_EXCEPTION, 0);

  Bit32s op2_32 = BX_READ_32BIT_REG(i->src());

  if (op2_32 == 0)
    exception(BX_DE_EXCEPTION, 0);

  Bit64s quotient_64  = op1_64 / op2_32;
  Bit32s remainder_32 = (Bit32s) (op1_64 % op2_32);
  Bit32s quotient_32l = (Bit32s) (quotient_64 & 0xFFFFFFFF);

  if (quotient_64 != quotient_32l)
  {
    exception(BX_DE_EXCEPTION, 0);
  }

  /* set EFLAGS:
   * IDIV affects the following flags: O,S,Z,A,P,C are undefined
   */

  /* now write quotient back to destination */
  RAX = (Bit32u) quotient_32l;
  RDX = (Bit32u) remainder_32;

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SLDT_Ew(bxInstruction_c *i)
{
  if (! protected_mode()) {
    exception(BX_UD_EXCEPTION, 0);
  }

#if BX_CPU_LEVEL >= 5
  if (CPL!=0 && BX_CPU_THIS_PTR cr4.get_UMIP()) {
    exception(BX_GP_EXCEPTION, 0);
  }
#endif

#if BX_SUPPORT_VMX >= 2
  if (BX_CPU_THIS_PTR in_vmx_guest)
    if (BX_CPU_THIS_PTR vmcs.vmexec_ctrls2.DESCRIPTOR_TABLE_VMEXIT())
      VMexit_Instruction(i, VMX_VMEXIT_LDTR_TR_ACCESS, BX_READ);
#endif

#if BX_SUPPORT_SVM
  if (BX_CPU_THIS_PTR in_svm_guest) {
    if (SVM_INTERCEPT(SVM_INTERCEPT0_LDTR_READ)) Svm_Vmexit(SVM_VMEXIT_LDTR_READ);
  }
#endif

  Bit16u val16 = BX_CPU_THIS_PTR ldtr.selector.value;
  if (i->modC0()) {
    if (i->os32L()) {
      BX_WRITE_32BIT_REGZ(i->dst(), val16);
    }
    else {
      BX_WRITE_16BIT_REG(i->dst(), val16);
    }
  }
  else {
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);
    /* pointer, segment address pair */
    write_virtual_word(i->seg(), eaddr, val16);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::STR_Ew(bxInstruction_c *i)
{
  if (! protected_mode()) {
    exception(BX_UD_EXCEPTION, 0);
  }

#if BX_CPU_LEVEL >= 5
  if (CPL!=0 && BX_CPU_THIS_PTR cr4.get_UMIP()) {
    exception(BX_GP_EXCEPTION, 0);
  }
#endif

#if BX_SUPPORT_VMX >= 2
  if (BX_CPU_THIS_PTR in_vmx_guest)
    if (BX_CPU_THIS_PTR vmcs.vmexec_ctrls2.DESCRIPTOR_TABLE_VMEXIT())
      VMexit_Instruction(i, VMX_VMEXIT_LDTR_TR_ACCESS, BX_READ);
#endif

#if BX_SUPPORT_SVM
  if (BX_CPU_THIS_PTR in_svm_guest) {
    if (SVM_INTERCEPT(SVM_INTERCEPT0_TR_READ)) Svm_Vmexit(SVM_VMEXIT_TR_READ);
  }
#endif

  Bit16u val16 = BX_CPU_THIS_PTR tr.selector.value;
  if (i->modC0()) {
    if (i->os32L()) {
      BX_WRITE_32BIT_REGZ(i->dst(), val16);
    }
    else {
      BX_WRITE_16BIT_REG(i->dst(), val16);
    }
  }
  else {
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);
    /* pointer, segment address pair */
    write_virtual_word(i->seg(), eaddr, val16);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LLDT_Ew(bxInstruction_c *i)
{
  /* protected mode */
  bx_descriptor_t  descriptor;
  bx_selector_t    selector;
  Bit16u raw_selector;
  Bit32u dword1, dword2;
#if BX_SUPPORT_X86_64
  Bit32u dword3 = 0;
#endif

  if (! protected_mode()) {
    exception(BX_UD_EXCEPTION, 0);
  }

  if (CPL != 0) {
    exception(BX_GP_EXCEPTION, 0);
  }

#if BX_SUPPORT_VMX >= 2
  if (BX_CPU_THIS_PTR in_vmx_guest)
    if (BX_CPU_THIS_PTR vmcs.vmexec_ctrls2.DESCRIPTOR_TABLE_VMEXIT())
      VMexit_Instruction(i, VMX_VMEXIT_LDTR_TR_ACCESS, BX_WRITE);
#endif

#if BX_SUPPORT_SVM
  if (BX_CPU_THIS_PTR in_svm_guest) {
    if (SVM_INTERCEPT(SVM_INTERCEPT0_LDTR_WRITE)) Svm_Vmexit(SVM_VMEXIT_LDTR_WRITE);
  }
#endif

  if (i->modC0()) {
    raw_selector = BX_READ_16BIT_REG(i->src());
  }
  else {
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);
    /* pointer, segment address pair */
    raw_selector = read_virtual_word(i->seg(), eaddr);
  }

  /* if selector is NULL, invalidate and done */
  if ((raw_selector & 0xfffc) == 0) {
    BX_CPU_THIS_PTR ldtr.selector.value = raw_selector;
    BX_CPU_THIS_PTR ldtr.cache.valid = 0;
    BX_NEXT_INSTR(i);
  }

  /* parse fields in selector */
  parse_selector(raw_selector, &selector);

  // #GP(selector) if the selector operand does not point into GDT
  if (selector.ti != 0) {
    exception(BX_GP_EXCEPTION, raw_selector & 0xfffc);
  }

  /* fetch descriptor; call handles out of limits checks */
#if BX_SUPPORT_X86_64
  if (BX_CPU_THIS_PTR cpu_mode == BX_MODE_LONG_64) {
    fetch_raw_descriptor_64(&selector, &dword1, &dword2, &dword3, BX_GP_EXCEPTION);
  }
  else
#endif
  {
    fetch_raw_descriptor(&selector, &dword1, &dword2, BX_GP_EXCEPTION);
  }

  parse_descriptor(dword1, dword2, &descriptor);

  /* if selector doesn't point to an LDT descriptor #GP(selector) */
  if (descriptor.valid == 0 || descriptor.segment ||
         descriptor.type != BX_SYS_SEGMENT_LDT)
  {
    exception(BX_GP_EXCEPTION, raw_selector & 0xfffc);
  }

  /* #NP(selector) if LDT descriptor is not present */
  if (! IS_PRESENT(descriptor)) {
    exception(BX_NP_EXCEPTION, raw_selector & 0xfffc);
  }

#if BX_SUPPORT_X86_64
  if (BX_CPU_THIS_PTR cpu_mode == BX_MODE_LONG_64) {
    descriptor.u.segment.base |= (Bit64u(dword3) << 32);
     if (!IsCanonical(descriptor.u.segment.base)) {
      exception(BX_GP_EXCEPTION, raw_selector & 0xfffc);
    }
  }
#endif

  BX_CPU_THIS_PTR ldtr.selector = selector;
  BX_CPU_THIS_PTR ldtr.cache = descriptor;
  BX_CPU_THIS_PTR ldtr.cache.valid = SegValidCache;

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LTR_Ew(bxInstruction_c *i)
{
  bx_descriptor_t descriptor;
  bx_selector_t selector;
  Bit16u raw_selector;
  Bit32u dword1, dword2;
#if BX_SUPPORT_X86_64
  Bit32u dword3 = 0;
#endif

  if (! protected_mode()) {
    exception(BX_UD_EXCEPTION, 0);
  }

  if (CPL != 0) {
    exception(BX_GP_EXCEPTION, 0);
  }

#if BX_SUPPORT_VMX >= 2
  if (BX_CPU_THIS_PTR in_vmx_guest)
    if (BX_CPU_THIS_PTR vmcs.vmexec_ctrls2.DESCRIPTOR_TABLE_VMEXIT())
      VMexit_Instruction(i, VMX_VMEXIT_LDTR_TR_ACCESS, BX_WRITE);
#endif

#if BX_SUPPORT_SVM
  if (BX_CPU_THIS_PTR in_svm_guest) {
    if (SVM_INTERCEPT(SVM_INTERCEPT0_TR_WRITE)) Svm_Vmexit(SVM_VMEXIT_TR_WRITE);
  }
#endif

  if (i->modC0()) {
    raw_selector = BX_READ_16BIT_REG(i->src());
  }
  else {
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);
    /* pointer, segment address pair */
    raw_selector = read_virtual_word(i->seg(), eaddr);
  }

  /* if selector is NULL, invalidate and done */
  if ((raw_selector & BX_SELECTOR_RPL_MASK) == 0) {
    exception(BX_GP_EXCEPTION, 0);
  }

  /* parse fields in selector, then check for null selector */
  parse_selector(raw_selector, &selector);

  if (selector.ti) {
    exception(BX_GP_EXCEPTION, raw_selector & 0xfffc);
  }

  /* fetch descriptor; call handles out of limits checks */
#if BX_SUPPORT_X86_64
  if (BX_CPU_THIS_PTR cpu_mode == BX_MODE_LONG_64) {
    fetch_raw_descriptor_64(&selector, &dword1, &dword2, &dword3, BX_GP_EXCEPTION);
  }
  else
#endif
  {
    fetch_raw_descriptor(&selector, &dword1, &dword2, BX_GP_EXCEPTION);
  }

  parse_descriptor(dword1, dword2, &descriptor);

  /* #GP(selector) if object is not a TSS or is already busy */
  if (descriptor.valid==0 || descriptor.segment ||
         (descriptor.type!=BX_SYS_SEGMENT_AVAIL_286_TSS &&
          descriptor.type!=BX_SYS_SEGMENT_AVAIL_386_TSS))
  {
    exception(BX_GP_EXCEPTION, raw_selector & 0xfffc);
  }

#if BX_SUPPORT_X86_64
  if (long_mode() && descriptor.type!=BX_SYS_SEGMENT_AVAIL_386_TSS) {
    exception(BX_GP_EXCEPTION, raw_selector & 0xfffc);
  }
#endif

  /* #NP(selector) if TSS descriptor is not present */
  if (! IS_PRESENT(descriptor)) {
    exception(BX_NP_EXCEPTION, raw_selector & 0xfffc);
  }

#if BX_SUPPORT_X86_64
  if (BX_CPU_THIS_PTR cpu_mode == BX_MODE_LONG_64) {
    descriptor.u.segment.base |= (Bit64u(dword3) << 32);
     if (!IsCanonical(descriptor.u.segment.base)) {
      exception(BX_GP_EXCEPTION, raw_selector & 0xfffc);
    }
  }
#endif

  BX_CPU_THIS_PTR tr.selector = selector;
  BX_CPU_THIS_PTR tr.cache    = descriptor;
  BX_CPU_THIS_PTR tr.cache.valid = SegValidCache;
  // tr.cache.type should not have busy bit, or it would not get
  // through the conditions above.
  BX_CPU_THIS_PTR tr.cache.type |= 2; // mark as busy

  /* mark as busy, should be done tomically using RMW */
  if (!(dword2 & 0x0200)) {
    dword2 |= 0x0200; /* set busy bit */
    system_write_dword(BX_CPU_THIS_PTR gdtr.base + selector.index*8 + 4, dword2);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::VERR_Ew(bxInstruction_c *i)
{
  /* for 16 bit operand size mode */
  Bit16u raw_selector;
  bx_descriptor_t descriptor;
  bx_selector_t selector;
  Bit32u dword1, dword2;

  if (! protected_mode()) {
    exception(BX_UD_EXCEPTION, 0);
  }

  if (i->modC0()) {
    raw_selector = BX_READ_16BIT_REG(i->src());
  }
  else {
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);
    /* pointer, segment address pair */
    raw_selector = read_virtual_word(i->seg(), eaddr);
  }

  /* if selector null, clear ZF and done */
  if ((raw_selector & 0xfffc) == 0) {
    clear_ZF();
    BX_NEXT_INSTR(i);
  }

  /* if source selector is visible at CPL & RPL,
   * within the descriptor table, and of type accepted by VERR instruction,
   * then load register with segment limit and set ZF */
  parse_selector(raw_selector, &selector);

  if (!fetch_raw_descriptor2(&selector, &dword1, &dword2)) {
    /* not within descriptor table */
    clear_ZF();
    BX_NEXT_INSTR(i);
  }

  parse_descriptor(dword1, dword2, &descriptor);

  if (descriptor.segment==0) { /* system or gate descriptor */
    clear_ZF();  /* inaccessible */
    BX_NEXT_INSTR(i);
  }

  if (descriptor.valid==0) {
    clear_ZF();  /* inaccessible */
    BX_NEXT_INSTR(i);
  }

  /* normal data/code segment */
  if (IS_CODE_SEGMENT(descriptor.type)) { /* code segment */
    /* ignore DPL for readable conforming segments */
    if (IS_CODE_SEGMENT_CONFORMING(descriptor.type) &&
        IS_CODE_SEGMENT_READABLE(descriptor.type))
    {
      assert_ZF(); /* accessible */
      BX_NEXT_INSTR(i);
    }
    if (!IS_CODE_SEGMENT_READABLE(descriptor.type)) {
      clear_ZF();  /* inaccessible */
      BX_NEXT_INSTR(i);
    }
    /* readable, non-conforming code segment */
    if ((descriptor.dpl<CPL) || (descriptor.dpl<selector.rpl)) {
      clear_ZF();  /* inaccessible */
    }
    else {
      assert_ZF(); /* accessible */
    }
  }
  else { /* data segment */
    if ((descriptor.dpl<CPL) || (descriptor.dpl<selector.rpl)) {
      clear_ZF(); /* not accessible */
    }
    else {
      assert_ZF(); /* accessible */
    }
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::VERW_Ew(bxInstruction_c *i)
{
  /* for 16 bit operand size mode */
  Bit16u raw_selector;
  bx_descriptor_t descriptor;
  bx_selector_t selector;
  Bit32u dword1, dword2;

  if (! protected_mode()) {
    exception(BX_UD_EXCEPTION, 0);
  }

  if (i->modC0()) {
    raw_selector = BX_READ_16BIT_REG(i->src());
  }
  else {
    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);
    /* pointer, segment address pair */
    raw_selector = read_virtual_word(i->seg(), eaddr);
  }

  /* if selector null, clear ZF and done */
  if ((raw_selector & 0xfffc) == 0) {
    clear_ZF();
    BX_NEXT_INSTR(i);
  }

  /* if source selector is visible at CPL & RPL,
   * within the descriptor table, and of type accepted by VERW instruction,
   * then load register with segment limit and set ZF */
  parse_selector(raw_selector, &selector);

  if (!fetch_raw_descriptor2(&selector, &dword1, &dword2)) {
    /* not within descriptor table */
    clear_ZF();
    BX_NEXT_INSTR(i);
  }

  parse_descriptor(dword1, dword2, &descriptor);

  /* rule out system segments & code segments */
  if (descriptor.segment==0 || IS_CODE_SEGMENT(descriptor.type)) {
    clear_ZF();
    BX_NEXT_INSTR(i);
  }

  if (descriptor.valid==0) {
    clear_ZF();
    BX_NEXT_INSTR(i);
  }

  /* data segment */
  if (IS_DATA_SEGMENT_WRITEABLE(descriptor.type)) { /* writable */
    if ((descriptor.dpl<CPL) || (descriptor.dpl<selector.rpl)) {
      clear_ZF(); /* not accessible */
    }
    else {
      assert_ZF();  /* accessible */
    }
  }
  else {
    clear_ZF(); /* not accessible */
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ANDN_GdBdEdR(bxInstruction_c *i)
{
  Bit32u op1_32 = BX_READ_32BIT_REG(i->src1());
  Bit32u op2_32 = BX_READ_32BIT_REG(i->src2());

  op1_32 = ~op1_32 & op2_32;

  SET_FLAGS_OSZAxC_LOGIC_32(op1_32); // keep PF unchanged

  BX_WRITE_32BIT_REGZ(i->dst(), op1_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MULX_GdBdEdR(bxInstruction_c *i)
{
  Bit32u op1_32 = EDX;
  Bit32u op2_32 = BX_READ_32BIT_REG(i->src2());
  Bit64u product_64  = ((Bit64u) op1_32) * ((Bit64u) op2_32);

  BX_WRITE_32BIT_REGZ(i->src1(), GET32L(product_64));
  BX_WRITE_32BIT_REGZ(i->dst(), GET32H(product_64));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BLSI_BdEdR(bxInstruction_c *i)
{
  Bit32u op1_32 = BX_READ_32BIT_REG(i->src());
  bool tmpCF = (op1_32 != 0);

  op1_32 = (Bit32u(0) - op1_32) & op1_32;

  SET_FLAGS_OSZAxC_LOGIC_32(op1_32); // keep PF unchanged
  set_CF(tmpCF);

  BX_WRITE_32BIT_REGZ(i->dst(), op1_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BLSMSK_BdEdR(bxInstruction_c *i)
{
  Bit32u op1_32 = BX_READ_32BIT_REG(i->src());
  bool tmpCF = (op1_32 == 0);

  op1_32 = (op1_32-1) ^ op1_32;

  SET_FLAGS_OSZAxC_LOGIC_32(op1_32); // keep PF unchanged
  set_CF(tmpCF);

  BX_WRITE_32BIT_REGZ(i->dst(), op1_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BLSR_BdEdR(bxInstruction_c *i)
{
  Bit32u op1_32 = BX_READ_32BIT_REG(i->src());
  bool tmpCF = (op1_32 == 0);

  op1_32 = (op1_32-1) & op1_32;

  SET_FLAGS_OSZAxC_LOGIC_32(op1_32); // keep PF unchanged
  set_CF(tmpCF);

  BX_WRITE_32BIT_REGZ(i->dst(), op1_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::RORX_GdEdIbR(bxInstruction_c *i)
{
  Bit32u op1_32 = BX_READ_32BIT_REG(i->src());

  unsigned count = i->Ib() & 0x1f;
  if (count) {
    op1_32 = ror32(op1_32, count);
  }

  BX_WRITE_32BIT_REGZ(i->dst(), op1_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SHLX_GdEdBdR(bxInstruction_c *i)
{
  Bit32u op1_32 = BX_READ_32BIT_REG(i->src1());

  unsigned count = BX_READ_32BIT_REG(i->src2()) & 0x1f;
  if (count)
    op1_32 <<= count;

  BX_WRITE_32BIT_REGZ(i->dst(), op1_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SHRX_GdEdBdR(bxInstruction_c *i)
{
  Bit32u op1_32 = BX_READ_32BIT_REG(i->src1());

  unsigned count = BX_READ_32BIT_REG(i->src2()) & 0x1f;
  if (count)
    op1_32 >>= count;

  BX_WRITE_32BIT_REGZ(i->dst(), op1_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SARX_GdEdBdR(bxInstruction_c *i)
{
  Bit32u op1_32 = BX_READ_32BIT_REG(i->src1());

  unsigned count = BX_READ_32BIT_REG(i->src2()) & 0x1f;
  if (count) {
    /* count < 32, since only lower 5 bits used */
    op1_32 = ((Bit32s) op1_32) >> count;
  }

  BX_WRITE_32BIT_REGZ(i->dst(), op1_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BEXTR_GdEdBdR(bxInstruction_c *i)
{
  Bit16u control = BX_READ_16BIT_REG(i->src2());
  unsigned start = control & 0xff;
  unsigned len   = control >> 8;

  Bit32u op1_32 = bextrd(BX_READ_32BIT_REG(i->src1()), start, len);
  SET_FLAGS_OSZAPC_LOGIC_32(op1_32);
  BX_WRITE_32BIT_REGZ(i->dst(), op1_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BZHI_GdEdBdR(bxInstruction_c *i)
{
  unsigned control = BX_READ_8BIT_REGL(i->src2());
  bool tmpCF = 0;
  Bit32u op1_32 = BX_READ_32BIT_REG(i->src1());

  if (control < 32) {
    Bit32u mask = (1 << control) - 1;
    op1_32 &= mask;
  }
  else {
    tmpCF = 1;
  }

  SET_FLAGS_OSZAxC_LOGIC_32(op1_32); // keep PF unchanged
  set_CF(tmpCF);

  BX_WRITE_32BIT_REGZ(i->dst(), op1_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::PEXT_GdBdEdR(bxInstruction_c *i)
{
  Bit32u op1_32 = BX_READ_32BIT_REG(i->src1());
  Bit32u op2_32 = BX_READ_32BIT_REG(i->src2()), result_32 = 0;

  Bit32u wr_mask = 0x1;

  for (; op2_32 != 0; op2_32 >>= 1)
  {
    if (op2_32 & 0x1) {
      if (op1_32 & 0x1) result_32 |= wr_mask;
      wr_mask <<= 1;
    }
    op1_32 >>= 1;
  }

  BX_WRITE_32BIT_REGZ(i->dst(), result_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::PDEP_GdBdEdR(bxInstruction_c *i)
{
  Bit32u op1_32 = BX_READ_32BIT_REG(i->src1());
  Bit32u op2_32 = BX_READ_32BIT_REG(i->src2()), result_32 = 0;

  Bit32u wr_mask = 0x1;

  for (; op2_32 != 0; op2_32 >>= 1)
  {
    if (op2_32 & 0x1) {
      if (op1_32 & 0x1) result_32 |= wr_mask;
      op1_32 >>= 1;
    }
    wr_mask <<= 1;
  }

  BX_WRITE_32BIT_REGZ(i->dst(), result_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ANDN_GqBqEqR(bxInstruction_c *i)
{
  Bit64u op1_64 = BX_READ_64BIT_REG(i->src1());
  Bit64u op2_64 = BX_READ_64BIT_REG(i->src2());

  op1_64 = ~op1_64 & op2_64;

  SET_FLAGS_OSZAxC_LOGIC_64(op1_64); // keep PF unchanged

  BX_WRITE_64BIT_REG(i->dst(), op1_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MULX_GqBqEqR(bxInstruction_c *i)
{
  Bit64u op1_64 = RDX;
  Bit64u op2_64 = BX_READ_64BIT_REG(i->src2());

  Bit128u product_128;

  // product_128 = ((Bit128u) op1_64) * ((Bit128u) op2_64);
  // product_64l = (Bit64u) (product_128 & 0xFFFFFFFFFFFFFFFF);
  // product_64h = (Bit64u) (product_128 >> 64);

  long_mul(&product_128,op1_64,op2_64);

  BX_WRITE_64BIT_REG(i->src1(), product_128.lo);
  BX_WRITE_64BIT_REG(i->dst(),  product_128.hi);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BLSI_BqEqR(bxInstruction_c *i)
{
  Bit64u op1_64 = BX_READ_64BIT_REG(i->src());
  bool tmpCF = (op1_64 != 0);

  op1_64 = (Bit64u(0) - op1_64) & op1_64;

  SET_FLAGS_OSZAxC_LOGIC_64(op1_64); // keep PF unchanged
  set_CF(tmpCF);

  BX_WRITE_64BIT_REG(i->dst(), op1_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BLSMSK_BqEqR(bxInstruction_c *i)
{
  Bit64u op1_64 = BX_READ_64BIT_REG(i->src());
  bool tmpCF = (op1_64 == 0);

  op1_64 = (op1_64-1) ^ op1_64;

  SET_FLAGS_OSZAxC_LOGIC_64(op1_64); // keep PF unchanged
  set_CF(tmpCF);

  BX_WRITE_64BIT_REG(i->dst(), op1_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BLSR_BqEqR(bxInstruction_c *i)
{
  Bit64u op1_64 = BX_READ_64BIT_REG(i->src());
  bool tmpCF = (op1_64 == 0);

  op1_64 = (op1_64-1) & op1_64;

  SET_FLAGS_OSZAxC_LOGIC_64(op1_64); // keep PF unchanged
  set_CF(tmpCF);

  BX_WRITE_64BIT_REG(i->dst(), op1_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::RORX_GqEqIbR(bxInstruction_c *i)
{
  Bit64u op1_64 = BX_READ_64BIT_REG(i->src());

  unsigned count = i->Ib() & 0x3f;
  if (count) {
    op1_64 = ror64(op1_64, count);
  }

  BX_WRITE_64BIT_REG(i->dst(), op1_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SHLX_GqEqBqR(bxInstruction_c *i)
{
  Bit64u op1_64 = BX_READ_64BIT_REG(i->src1());

  unsigned count = BX_READ_32BIT_REG(i->src2()) & 0x3f;
  if (count)
    op1_64 <<= count;

  BX_WRITE_64BIT_REG(i->dst(), op1_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SHRX_GqEqBqR(bxInstruction_c *i)
{
  Bit64u op1_64 = BX_READ_64BIT_REG(i->src1());

  unsigned count = BX_READ_32BIT_REG(i->src2()) & 0x3f;
  if (count)
    op1_64 >>= count;

  BX_WRITE_64BIT_REG(i->dst(), op1_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SARX_GqEqBqR(bxInstruction_c *i)
{
  Bit64u op1_64 = BX_READ_64BIT_REG(i->src1());

  unsigned count = BX_READ_32BIT_REG(i->src2()) & 0x3f;
  if (count) {
    /* count < 64, since only lower 6 bits used */
    op1_64 = ((Bit64s) op1_64) >> count;
  }

  BX_WRITE_64BIT_REG(i->dst(), op1_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BEXTR_GqEqBqR(bxInstruction_c *i)
{
  Bit16u control = BX_READ_16BIT_REG(i->src2());
  unsigned start = control & 0xff;
  unsigned len   = control >> 8;

  Bit64u op1_64 = bextrq(BX_READ_64BIT_REG(i->src1()), start, len);
  SET_FLAGS_OSZAPC_LOGIC_64(op1_64);
  BX_WRITE_64BIT_REG(i->dst(), op1_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BZHI_GqEqBqR(bxInstruction_c *i)
{
  unsigned control = BX_READ_8BIT_REGL(i->src2());
  bool tmpCF = 0;
  Bit64u op1_64 = BX_READ_64BIT_REG(i->src1());

  if (control < 64) {
    Bit64u mask = (BX_CONST64(1) << control) - 1;
    op1_64 &= mask;
  }
  else {
    tmpCF = 1;
  }

  SET_FLAGS_OSZAxC_LOGIC_64(op1_64); // keep PF unchanged
  set_CF(tmpCF);

  BX_WRITE_64BIT_REG(i->dst(), op1_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::PEXT_GqBqEqR(bxInstruction_c *i)
{
  Bit64u op1_64 = BX_READ_64BIT_REG(i->src1());
  Bit64u op2_64 = BX_READ_64BIT_REG(i->src2()), result_64 = 0;

  Bit64u wr_mask = 0x1;

  for (; op2_64 != 0; op2_64 >>= 1)
  {
    if (op2_64 & 0x1) {
      if (op1_64 & 0x1) result_64 |= wr_mask;
      wr_mask <<= 1;
    }
    op1_64 >>= 1;
  }

  BX_WRITE_64BIT_REG(i->dst(), result_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::PDEP_GqBqEqR(bxInstruction_c *i)
{
  Bit64u op1_64 = BX_READ_64BIT_REG(i->src1());
  Bit64u op2_64 = BX_READ_64BIT_REG(i->src2()), result_64 = 0;

  Bit64u wr_mask = 0x1;

  for (; op2_64 != 0; op2_64 >>= 1)
  {
    if (op2_64 & 0x1) {
      if (op1_64 & 0x1) result_64 |= wr_mask;
      op1_64 >>= 1;
    }
    wr_mask <<= 1;
  }

  BX_WRITE_64BIT_REG(i->dst(), result_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BEXTR_GdEdIdR(bxInstruction_c *i)
{
  Bit16u control = (Bit16u) i->Id();
  unsigned start = control & 0xff;
  unsigned len   = control >> 8;

  Bit32u op1_32 = bextrd(BX_READ_32BIT_REG(i->src()), start, len);
  SET_FLAGS_OSZAPC_LOGIC_32(op1_32);
  BX_WRITE_32BIT_REGZ(i->dst(), op1_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BLCFILL_BdEdR(bxInstruction_c *i)
{
  Bit32u op_32 = BX_READ_32BIT_REG(i->src());

  Bit32u result_32 = (op_32 + 1) & op_32;

  SET_FLAGS_OSZAPC_LOGIC_32(result_32);
  set_CF((op_32 + 1) == 0);

  BX_WRITE_32BIT_REGZ(i->dst(), result_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BLCI_BdEdR(bxInstruction_c *i)
{
  Bit32u op_32 = BX_READ_32BIT_REG(i->src());

  Bit32u result_32 = ~(op_32 + 1) | op_32;

  SET_FLAGS_OSZAPC_LOGIC_32(result_32);
  set_CF((op_32 + 1) == 0);

  BX_WRITE_32BIT_REGZ(i->dst(), result_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BLCIC_BdEdR(bxInstruction_c *i)
{
  Bit32u op_32 = BX_READ_32BIT_REG(i->src());

  Bit32u result_32 = (op_32 + 1) & ~op_32;

  SET_FLAGS_OSZAPC_LOGIC_32(result_32);
  set_CF((op_32 + 1) == 0);

  BX_WRITE_32BIT_REGZ(i->dst(), result_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BLCMSK_BdEdR(bxInstruction_c *i)
{
  Bit32u op_32 = BX_READ_32BIT_REG(i->src());

  Bit32u result_32 = (op_32 + 1) ^ op_32;

  SET_FLAGS_OSZAPC_LOGIC_32(result_32);
  set_CF((op_32 + 1) == 0);

  BX_WRITE_32BIT_REGZ(i->dst(), result_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BLCS_BdEdR(bxInstruction_c *i)
{
  Bit32u op_32 = BX_READ_32BIT_REG(i->src());

  Bit32u result_32 = (op_32 + 1) | op_32;

  SET_FLAGS_OSZAPC_LOGIC_32(result_32);
  set_CF((op_32 + 1) == 0);

  BX_WRITE_32BIT_REGZ(i->dst(), result_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BLSFILL_BdEdR(bxInstruction_c *i)
{
  Bit32u op_32 = BX_READ_32BIT_REG(i->src());

  Bit32u result_32 = (op_32 - 1) | op_32;

  SET_FLAGS_OSZAPC_LOGIC_32(result_32);
  set_CF(op_32 == 0);

  BX_WRITE_32BIT_REGZ(i->dst(), result_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BLSIC_BdEdR(bxInstruction_c *i)
{
  Bit32u op_32 = BX_READ_32BIT_REG(i->src());

  Bit32u result_32 = (op_32 - 1) | ~op_32;

  SET_FLAGS_OSZAPC_LOGIC_32(result_32);
  set_CF(op_32 == 0);

  BX_WRITE_32BIT_REGZ(i->dst(), result_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::T1MSKC_BdEdR(bxInstruction_c *i)
{
  Bit32u op_32 = BX_READ_32BIT_REG(i->src());

  Bit32u result_32 = (op_32 + 1) | ~op_32;

  SET_FLAGS_OSZAPC_LOGIC_32(result_32);
  set_CF((op_32 + 1) == 0);

  BX_WRITE_32BIT_REGZ(i->dst(), result_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::TZMSK_BdEdR(bxInstruction_c *i)
{
  Bit32u op_32 = BX_READ_32BIT_REG(i->src());

  Bit32u result_32 = (op_32 - 1) & ~op_32;

  SET_FLAGS_OSZAPC_LOGIC_32(result_32);
  set_CF(op_32 == 0);

  BX_WRITE_32BIT_REGZ(i->dst(), result_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BEXTR_GqEqIdR(bxInstruction_c *i)
{
  Bit16u control = (Bit16u) i->Id();
  unsigned start = control & 0xff;
  unsigned len   = control >> 8;

  Bit64u op1_64 = bextrq(BX_READ_64BIT_REG(i->src()), start, len);
  SET_FLAGS_OSZAPC_LOGIC_64(op1_64);
  BX_WRITE_64BIT_REG(i->dst(), op1_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BLCFILL_BqEqR(bxInstruction_c *i)
{
  Bit64u op_64 = BX_READ_64BIT_REG(i->src());

  Bit64u result_64 = (op_64 + 1) & op_64;

  SET_FLAGS_OSZAPC_LOGIC_64(result_64);
  set_CF((op_64 + 1) == 0);

  BX_WRITE_64BIT_REG(i->dst(), result_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BLCI_BqEqR(bxInstruction_c *i)
{
  Bit64u op_64 = BX_READ_64BIT_REG(i->src());

  Bit64u result_64 = ~(op_64 + 1) | op_64;

  SET_FLAGS_OSZAPC_LOGIC_64(result_64);
  set_CF((op_64 + 1) == 0);

  BX_WRITE_64BIT_REG(i->dst(), result_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BLCIC_BqEqR(bxInstruction_c *i)
{
  Bit64u op_64 = BX_READ_64BIT_REG(i->src());

  Bit64u result_64 = (op_64 + 1) & ~op_64;

  SET_FLAGS_OSZAPC_LOGIC_64(result_64);
  set_CF((op_64 + 1) == 0);

  BX_WRITE_64BIT_REG(i->dst(), result_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BLCMSK_BqEqR(bxInstruction_c *i)
{
  Bit64u op_64 = BX_READ_64BIT_REG(i->src());

  Bit64u result_64 = (op_64 + 1) ^ op_64;

  SET_FLAGS_OSZAPC_LOGIC_64(result_64);
  set_CF((op_64 + 1) == 0);

  BX_WRITE_64BIT_REG(i->dst(), result_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BLCS_BqEqR(bxInstruction_c *i)
{
  Bit64u op_64 = BX_READ_64BIT_REG(i->src());

  Bit64u result_64 = (op_64 + 1) | op_64;

  SET_FLAGS_OSZAPC_LOGIC_64(result_64);
  set_CF((op_64 + 1) == 0);

  BX_WRITE_64BIT_REG(i->dst(), result_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BLSFILL_BqEqR(bxInstruction_c *i)
{
  Bit64u op_64 = BX_READ_64BIT_REG(i->src());

  Bit64u result_64 = (op_64 - 1) | op_64;

  SET_FLAGS_OSZAPC_LOGIC_64(result_64);
  set_CF(op_64 == 0);

  BX_WRITE_64BIT_REG(i->dst(), result_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BLSIC_BqEqR(bxInstruction_c *i)
{
  Bit64u op_64 = BX_READ_64BIT_REG(i->src());

  Bit64u result_64 = (op_64 - 1) | ~op_64;

  SET_FLAGS_OSZAPC_LOGIC_64(result_64);
  set_CF(op_64 == 0);

  BX_WRITE_64BIT_REG(i->dst(), result_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::T1MSKC_BqEqR(bxInstruction_c *i)
{
  Bit64u op_64 = BX_READ_64BIT_REG(i->src());

  Bit64u result_64 = (op_64 + 1) | ~op_64;

  SET_FLAGS_OSZAPC_LOGIC_64(result_64);
  set_CF((op_64 + 1) == 0);

  BX_WRITE_64BIT_REG(i->dst(), result_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::TZMSK_BqEqR(bxInstruction_c *i)
{
  Bit64u op_64 = BX_READ_64BIT_REG(i->src());

  Bit64u result_64 = (op_64 - 1) & ~op_64;

  SET_FLAGS_OSZAPC_LOGIC_64(result_64);
  set_CF(op_64 == 0);

  BX_WRITE_64BIT_REG(i->dst(), result_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LZCNT_GwEwR(bxInstruction_c *i)
{
  Bit16u op1_16 = BX_READ_16BIT_REG(i->src());
  Bit16u result_16 = (Bit16u) lzcntw(op1_16);

  set_CF(! op1_16);
  set_ZF(! result_16);

  BX_WRITE_16BIT_REG(i->dst(), result_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LZCNT_GdEdR(bxInstruction_c *i)
{
  Bit32u op1_32 = BX_READ_32BIT_REG(i->src());
  Bit32u result_32 = lzcntd(op1_32);

  set_CF(! op1_32);
  set_ZF(! result_32);

  BX_WRITE_32BIT_REGZ(i->dst(), result_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LZCNT_GqEqR(bxInstruction_c *i)
{
  Bit64u op1_64 = BX_READ_64BIT_REG(i->src());
  Bit64u result_64 = lzcntq(op1_64);

  set_CF(! op1_64);
  set_ZF(! result_64);

  BX_WRITE_64BIT_REG(i->dst(), result_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::TZCNT_GwEwR(bxInstruction_c *i)
{
  Bit16u op1_16 = BX_READ_16BIT_REG(i->src());
  Bit16u result_16 = (Bit16u) tzcntw(op1_16);

  set_CF(! op1_16);
  set_ZF(! result_16);

  BX_WRITE_16BIT_REG(i->dst(), result_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::TZCNT_GdEdR(bxInstruction_c *i)
{
  Bit32u op1_32 = BX_READ_32BIT_REG(i->src());
  Bit32u result_32 = tzcntd(op1_32);

  set_CF(! op1_32);
  set_ZF(! result_32);

  BX_WRITE_32BIT_REGZ(i->dst(), result_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::TZCNT_GqEqR(bxInstruction_c *i)
{
  Bit64u op1_64 = BX_READ_64BIT_REG(i->src());
  Bit64u result_64 = tzcntq(op1_64);

  set_CF(! op1_64);
  set_ZF(! result_64);

  BX_WRITE_64BIT_REG(i->dst(), result_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MUL_AXEwR(bxInstruction_c *i)
{
  Bit16u op1_16 = AX;
  Bit16u op2_16 = BX_READ_16BIT_REG(i->src());

  Bit32u product_32  = ((Bit32u) op1_16) * ((Bit32u) op2_16);
  Bit16u product_16l = (product_32 & 0xFFFF);
  Bit16u product_16h =  product_32 >> 16;

  /* now write product back to destination */
  AX = product_16l;
  DX = product_16h;

  /* set EFLAGS */
  SET_FLAGS_OSZAPC_LOGIC_16(product_16l);
  if(product_16h != 0)
  {
    BX_CPU_THIS_PTR oszapc.assert_flags_OxxxxC();
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::IMUL_AXEwR(bxInstruction_c *i)
{
  Bit16s op1_16 = AX;
  Bit16s op2_16 = BX_READ_16BIT_REG(i->src());

  Bit32s product_32  = ((Bit32s) op1_16) * ((Bit32s) op2_16);
  Bit16u product_16l = (product_32 & 0xFFFF);
  Bit16u product_16h = product_32 >> 16;

  /* now write product back to destination */
  AX = product_16l;
  DX = product_16h;

  /* set eflags:
   * IMUL r/m16: condition for clearing CF & OF:
   *   DX:AX = sign-extend of AX
   */
  SET_FLAGS_OSZAPC_LOGIC_16(product_16l);
  if(product_32 != (Bit16s)product_32)
  {
    BX_CPU_THIS_PTR oszapc.assert_flags_OxxxxC();
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::DIV_AXEwR(bxInstruction_c *i)
{
  Bit16u op2_16 = BX_READ_16BIT_REG(i->src());
  if (op2_16 == 0)
    exception(BX_DE_EXCEPTION, 0);

  Bit32u op1_32 = (((Bit32u) DX) << 16) | ((Bit32u) AX);

  Bit32u quotient_32  = op1_32 / op2_16;
  Bit16u remainder_16 = op1_32 % op2_16;
  Bit16u quotient_16l = quotient_32 & 0xFFFF;

  if (quotient_32 != quotient_16l)
    exception(BX_DE_EXCEPTION, 0);

  /* now write quotient back to destination */
  AX = quotient_16l;
  DX = remainder_16;

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::IDIV_AXEwR(bxInstruction_c *i)
{
  Bit32s op1_32 = ((((Bit32u) DX) << 16) | ((Bit32u) AX));

  /* check MIN_INT case */
  if (op1_32 == ((Bit32s)0x80000000))
    exception(BX_DE_EXCEPTION, 0);

  Bit16s op2_16 = BX_READ_16BIT_REG(i->src());

  if (op2_16 == 0)
    exception(BX_DE_EXCEPTION, 0);

  Bit32s quotient_32  = op1_32 / op2_16;
  Bit16s remainder_16 = op1_32 % op2_16;
  Bit16s quotient_16l = quotient_32 & 0xFFFF;

  if (quotient_32 != quotient_16l)
    exception(BX_DE_EXCEPTION, 0);

  /* now write quotient back to destination */
  AX = quotient_16l;
  DX = remainder_16;

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::IMUL_GwEwIwR(bxInstruction_c *i)
{
  Bit16s op2_16 = BX_READ_16BIT_REG(i->src());
  Bit16s op3_16 = i->Iw();

  Bit32s product_32  = op2_16 * op3_16;
  Bit16u product_16 = (product_32 & 0xFFFF);

  /* now write product back to destination */
  BX_WRITE_16BIT_REG(i->dst(), product_16);

  /* set eflags:
   * IMUL r16,r/m16,imm16: condition for clearing CF & OF:
   *   result exactly fits within r16
   */
  SET_FLAGS_OSZAPC_LOGIC_16(product_16);
  if(product_32 != (Bit16s) product_32)
  {
    BX_CPU_THIS_PTR oszapc.assert_flags_OxxxxC();
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::IMUL_GwEwR(bxInstruction_c *i)
{
  Bit16s op1_16 = BX_READ_16BIT_REG(i->dst());
  Bit16s op2_16 = BX_READ_16BIT_REG(i->src());

  Bit32s product_32 = op1_16 * op2_16;
  Bit16u product_16 = (product_32 & 0xFFFF);

  /* now write product back to destination */
  BX_WRITE_16BIT_REG(i->dst(), product_16);

  /* set eflags:
   * IMUL r16,r/m16: condition for clearing CF & OF:
   *   result exactly fits within r16
   */
  SET_FLAGS_OSZAPC_LOGIC_16(product_16);
  if(product_32 != (Bit16s) product_32)
  {
    BX_CPU_THIS_PTR oszapc.assert_flags_OxxxxC();
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::POPCNT_GwEwR(bxInstruction_c *i)
{
  Bit16u op_16 = popcntw(BX_READ_16BIT_REG(i->src()));

  clearEFlagsOSZAPC();
  if (! op_16) assert_ZF();

  BX_WRITE_16BIT_REG(i->dst(), op_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::POPCNT_GdEdR(bxInstruction_c *i)
{
  Bit32u op_32 = popcntd(BX_READ_32BIT_REG(i->src()));

  clearEFlagsOSZAPC();
  if (! op_32) assert_ZF();

  BX_WRITE_32BIT_REGZ(i->dst(), op_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::POPCNT_GqEqR(bxInstruction_c *i)
{
  Bit32u op_32 = popcntq(BX_READ_64BIT_REG(i->src()));

  clearEFlagsOSZAPC();
  if (! op_32) assert_ZF();

  BX_WRITE_32BIT_REGZ(i->dst(), op_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADCX_GdEdR(bxInstruction_c *i)
{
  Bit32u op1_32 = BX_READ_32BIT_REG(i->dst());
  Bit32u op2_32 = BX_READ_32BIT_REG(i->src());
  Bit32u sum_32 = op1_32 + op2_32 + getB_CF();

  BX_WRITE_32BIT_REGZ(i->dst(), sum_32);

  Bit32u carry_out = ADD_COUT_VEC(op1_32, op2_32, sum_32);
  set_CF(carry_out >> 31);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADOX_GdEdR(bxInstruction_c *i)
{
  Bit32u op1_32 = BX_READ_32BIT_REG(i->dst());
  Bit32u op2_32 = BX_READ_32BIT_REG(i->src());
  Bit32u sum_32 = op1_32 + op2_32 + getB_OF();

  BX_WRITE_32BIT_REGZ(i->dst(), sum_32);

  Bit32u carry_out = ADD_COUT_VEC(op1_32, op2_32, sum_32);
  set_OF(carry_out >> 31);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADCX_GqEqR(bxInstruction_c *i)
{
  Bit64u op1_64 = BX_READ_64BIT_REG(i->dst());
  Bit64u op2_64 = BX_READ_64BIT_REG(i->src());
  Bit64u sum_64 = op1_64 + op2_64 + getB_CF();

  BX_WRITE_64BIT_REG(i->dst(), sum_64);

  Bit64u carry_out = ADD_COUT_VEC(op1_64, op2_64, sum_64);
  set_CF(carry_out >> 63);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ADOX_GqEqR(bxInstruction_c *i)
{
  Bit64u op1_64 = BX_READ_64BIT_REG(i->dst());
  Bit64u op2_64 = BX_READ_64BIT_REG(i->src());
  Bit64u sum_64 = op1_64 + op2_64 + getB_OF();

  BX_WRITE_64BIT_REG(i->dst(), sum_64);

  Bit64u carry_out = ADD_COUT_VEC(op1_64, op2_64, sum_64);
  set_OF(carry_out >> 63);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OR_GqEqR(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64;

  op1_64 = BX_READ_64BIT_REG(i->dst());
  op2_64 = BX_READ_64BIT_REG(i->src());
  op1_64 |= op2_64;

  BX_WRITE_64BIT_REG(i->dst(), op1_64);

  SET_FLAGS_OSZAPC_LOGIC_64(op1_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OR_GqEqM(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  op1_64 = BX_READ_64BIT_REG(i->dst());
  op2_64 = read_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));
  op1_64 |= op2_64;

  BX_WRITE_64BIT_REG(i->dst(), op1_64);

  SET_FLAGS_OSZAPC_LOGIC_64(op1_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OR_EqGqM(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  op1_64 = read_RMW_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));
  op2_64 = BX_READ_64BIT_REG(i->src());
  op1_64 |= op2_64;
  write_RMW_linear_qword(op1_64);

  SET_FLAGS_OSZAPC_LOGIC_64(op1_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OR_EqIdM(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64 = (Bit32s) i->Id();

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  op1_64 = read_RMW_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));
  op1_64 |= op2_64;
  write_RMW_linear_qword(op1_64);

  SET_FLAGS_OSZAPC_LOGIC_64(op1_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OR_EqIdR(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64 = (Bit32s) i->Id();

  op1_64 = BX_READ_64BIT_REG(i->dst());
  op1_64 |= op2_64;
  BX_WRITE_64BIT_REG(i->dst(), op1_64);

  SET_FLAGS_OSZAPC_LOGIC_64(op1_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LEA_GqM(bxInstruction_c *i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  BX_WRITE_64BIT_REG(i->dst(), eaddr);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_RAXOq(bxInstruction_c *i)
{
  RAX = read_linear_qword(i->seg(), get_laddr64(i->seg(), i->Iq()));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_OqRAX(bxInstruction_c *i)
{
  write_linear_qword(i->seg(), get_laddr64(i->seg(), i->Iq()), RAX);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_EAXOq(bxInstruction_c *i)
{
  RAX = read_linear_dword(i->seg(), get_laddr64(i->seg(), i->Iq()));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_OqEAX(bxInstruction_c *i)
{
  write_linear_dword(i->seg(), get_laddr64(i->seg(), i->Iq()), EAX);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_AXOq(bxInstruction_c *i)
{
  AX = read_linear_word(i->seg(), get_laddr64(i->seg(), i->Iq()));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_OqAX(bxInstruction_c *i)
{
  write_linear_word(i->seg(), get_laddr64(i->seg(), i->Iq()), AX);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_ALOq(bxInstruction_c *i)
{
  AL = read_linear_byte(i->seg(), get_laddr64(i->seg(), i->Iq()));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_OqAL(bxInstruction_c *i)
{
  write_linear_byte(i->seg(), get_laddr64(i->seg(), i->Iq()), AL);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_EqGqM(bxInstruction_c *i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  write_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr), BX_READ_64BIT_REG(i->src()));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_GqEqR(bxInstruction_c *i)
{
  BX_WRITE_64BIT_REG(i->dst(), BX_READ_64BIT_REG(i->src()));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_GqEqM(bxInstruction_c *i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);
  Bit64u val64 = read_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));

  BX_WRITE_64BIT_REG(i->dst(), val64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_EqIdR(bxInstruction_c *i)
{
  Bit64u op_64 = (Bit32s) i->Id();
  BX_WRITE_64BIT_REG(i->dst(), op_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_EqIdM(bxInstruction_c *i)
{
  Bit64u op_64 = (Bit32s) i->Id();

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  write_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr), op_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV64S_EqGqM(bxInstruction_c *i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  stack_write_qword(eaddr, BX_READ_64BIT_REG(i->src()));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV64S_GqEqM(bxInstruction_c *i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  BX_WRITE_64BIT_REG(i->dst(), stack_read_qword(eaddr));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::REP_MOVSQ_YqXq(bxInstruction_c *i)
{
  if (i->as64L()) {
    BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::MOVSQ64_YqXq);
  }
  else {
    BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::MOVSQ32_YqXq);
    BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RSI); // always clear upper part of RSI/RDI
    BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RDI);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::REP_CMPSQ_XqYq(bxInstruction_c *i)
{
  if (i->as64L()) {
    BX_CPU_THIS_PTR repeat_ZF(i, &BX_CPU_C::CMPSQ64_XqYq);
  }
  else {
    BX_CPU_THIS_PTR repeat_ZF(i, &BX_CPU_C::CMPSQ32_XqYq);
    BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RSI); // always clear upper part of RSI/RDI
    BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RDI);
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::REP_STOSQ_YqRAX(bxInstruction_c *i)
{
  if (i->as64L()) {
    BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::STOSQ64_YqRAX);
  }
  else {
    BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::STOSQ32_YqRAX);
    BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RDI); // always clear upper part of RDI
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::REP_LODSQ_RAXXq(bxInstruction_c *i)
{
  if (i->as64L()) {
    BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::LODSQ64_RAXXq);
  }
  else {
    BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::LODSQ32_RAXXq);
    BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RSI); // always clear upper part of RSI
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::REP_SCASQ_RAXYq(bxInstruction_c *i)
{
  if (i->as64L()) {
    BX_CPU_THIS_PTR repeat_ZF(i, &BX_CPU_C::SCASQ64_RAXYq);
  }
  else {
    BX_CPU_THIS_PTR repeat_ZF(i, &BX_CPU_C::SCASQ32_RAXYq);
    BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RDI); // always clear upper part of RDI
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMPSQ32_XqYq(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64, diff_64;

  Bit32u esi = ESI;
  Bit32u edi = EDI;

  op1_64 = read_linear_qword(i->seg(), get_laddr64(i->seg(), esi));
  op2_64 = read_linear_qword(BX_SEG_REG_ES, edi);

  diff_64 = op1_64 - op2_64;

  SET_FLAGS_OSZAPC_SUB_64(op1_64, op2_64, diff_64);

  if (BX_CPU_THIS_PTR get_DF()) {
    esi -= 8;
    edi -= 8;
  }
  else {
    esi += 8;
    edi += 8;
  }

  // zero extension of RSI/RDI
  RDI = edi;
  RSI = esi;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CMPSQ64_XqYq(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64, diff_64;

  Bit64u rsi = RSI;
  Bit64u rdi = RDI;

  op1_64 = read_linear_qword(i->seg(), get_laddr64(i->seg(), rsi));
  op2_64 = read_linear_qword(BX_SEG_REG_ES, rdi);

  diff_64 = op1_64 - op2_64;

  SET_FLAGS_OSZAPC_SUB_64(op1_64, op2_64, diff_64);

  if (BX_CPU_THIS_PTR get_DF()) {
    rsi -= 8;
    rdi -= 8;
  }
  else {
    rsi += 8;
    rdi += 8;
  }

  RDI = rdi;
  RSI = rsi;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SCASQ32_RAXYq(bxInstruction_c *i)
{
  Bit64u op1_64 = RAX, op2_64, diff_64;

  Bit32u edi = EDI;

  op2_64 = read_virtual_qword(BX_SEG_REG_ES, edi);

  diff_64 = op1_64 - op2_64;

  SET_FLAGS_OSZAPC_SUB_64(op1_64, op2_64, diff_64);

  if (BX_CPU_THIS_PTR get_DF()) {
    edi -= 8;
  }
  else {
    edi += 8;
  }

  // zero extension of RDI
  RDI = edi;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SCASQ64_RAXYq(bxInstruction_c *i)
{
  Bit64u op1_64 = RAX, op2_64, diff_64;

  Bit64u rdi = RDI;

  op2_64 = read_virtual_qword(BX_SEG_REG_ES, rdi);

  diff_64 = op1_64 - op2_64;

  SET_FLAGS_OSZAPC_SUB_64(op1_64, op2_64, diff_64);

  if (BX_CPU_THIS_PTR get_DF()) {
    rdi -= 8;
  }
  else {
    rdi += 8;
  }

  RDI = rdi;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LODSQ32_RAXXq(bxInstruction_c *i)
{
  Bit32u esi = ESI;

  RAX = read_linear_qword(i->seg(), get_laddr64(i->seg(), esi));

  if (BX_CPU_THIS_PTR get_DF()) {
    esi -= 8;
  }
  else {
    esi += 8;
  }

  // zero extension of RSI
  RSI = esi;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LODSQ64_RAXXq(bxInstruction_c *i)
{
  Bit64u rsi = RSI;

  RAX = read_linear_qword(i->seg(), get_laddr64(i->seg(), rsi));

  if (BX_CPU_THIS_PTR get_DF()) {
    rsi -= 8;
  }
  else {
    rsi += 8;
  }

  RSI = rsi;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::STOSQ32_YqRAX(bxInstruction_c *i)
{
  Bit32u edi = EDI;

  write_linear_qword(BX_SEG_REG_ES, edi, RAX);

  if (BX_CPU_THIS_PTR get_DF()) {
    edi -= 8;
  }
  else {
    edi += 8;
  }

  // zero extension of RDI
  RDI = edi;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::STOSQ64_YqRAX(bxInstruction_c *i)
{
  Bit64u rdi = RDI;

  write_linear_qword(BX_SEG_REG_ES, rdi, RAX);

  if (BX_CPU_THIS_PTR get_DF()) {
    rdi -= 8;
  }
  else {
    rdi += 8;
  }

  RDI = rdi;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOVSQ32_YqXq(bxInstruction_c *i)
{
  Bit32u esi = ESI;
  Bit32u edi = EDI;

  Bit64u temp64 = read_linear_qword(i->seg(), get_laddr64(i->seg(), esi));
  write_linear_qword(BX_SEG_REG_ES, edi, temp64);

  if (BX_CPU_THIS_PTR get_DF()) {
    esi -= 8;
    edi -= 8;
  }
  else {
    esi += 8;
    edi += 8;
  }

  // zero extension of RSI/RDI
  RSI = esi;
  RDI = edi;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOVSQ64_YqXq(bxInstruction_c *i)
{
  Bit32s increment = 0;

  Bit64u rsi = RSI;
  Bit64u rdi = RDI;

#if BX_SUPPORT_REPEAT_SPEEDUPS
  /* If conditions are right, we can transfer IO to physical memory
   * in a batch, rather than one instruction at a time.
   */
  if (i->repUsedL() && !BX_CPU_THIS_PTR get_DF() && !BX_CPU_THIS_PTR async_event)
  {
    Bit32u byteCount = FastRepMOVSB(get_laddr64(i->seg(), rsi), rdi, ECX*8, 8);
    if (byteCount) {
      Bit32u qwordCount = byteCount >> 3;

      // Decrement the ticks count by the number of iterations, minus
      // one, since the main cpu loop will decrement one.  Also,
      // the count is predecremented before examined, so definitely
      // don't roll it under zero.
      BX_TICKN(qwordCount-1);

      // Decrement RCX. Note, the main loop will decrement 1 also, so
      // decrement by one less than expected, like the case above.
      RCX -= (qwordCount-1);

      increment = byteCount;
    }
  }

  if (increment == 0)
#endif
  {
    Bit64u temp64 = read_linear_qword(i->seg(), get_laddr64(i->seg(), rsi));
    write_linear_qword(BX_SEG_REG_ES, rdi, temp64);

    increment = BX_CPU_THIS_PTR get_DF() ? -8 : 8;
  }

  RSI = rsi + increment;
  RDI = rdi + increment;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CALL_Jq(bxInstruction_c *i)
{
  Bit64u new_RIP = RIP + (Bit32s) i->Id();

  RSP_SPECULATIVE;

  /* push 64 bit EA of next instruction */
  push_64(RIP);
#if BX_SUPPORT_CET
  if (ShadowStackEnabled(CPL) && i->Id())
    shadow_stack_push_64(RIP);
#endif

  if (! IsCanonical(new_RIP)) {
    exception(BX_GP_EXCEPTION, 0);
  }

  RIP = new_RIP;

  RSP_COMMIT;

  BX_INSTR_UCNEAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_CALL, PREV_RIP, RIP);

  BX_LINK_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JMP_Jq(bxInstruction_c *i)
{
  Bit64u new_RIP = RIP + (Bit32s) i->Id();

  if (! IsCanonical(new_RIP)) {
    exception(BX_GP_EXCEPTION, 0);
  }

  RIP = new_RIP;

  BX_INSTR_UCNEAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_JMP, PREV_RIP, RIP);

  BX_LINK_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JO_Jq(bxInstruction_c *i)
{
  if (get_OF()) {
    branch_near64(i);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, RIP);
    BX_LINK_TRACE(i);
  }

  BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
  BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JNO_Jq(bxInstruction_c *i)
{
  if (! get_OF()) {
    branch_near64(i);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, RIP);
    BX_LINK_TRACE(i);
  }

  BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
  BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JB_Jq(bxInstruction_c *i)
{
  if (get_CF()) {
    branch_near64(i);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, RIP);
    BX_LINK_TRACE(i);
  }

  BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
  BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JNB_Jq(bxInstruction_c *i)
{
  if (! get_CF()) {
    branch_near64(i);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, RIP);
    BX_LINK_TRACE(i);
  }

  BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
  BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JZ_Jq(bxInstruction_c *i)
{
  if (get_ZF()) {
    branch_near64(i);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, RIP);
    BX_LINK_TRACE(i);
  }

  BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
  BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JNZ_Jq(bxInstruction_c *i)
{
  if (! get_ZF()) {
    branch_near64(i);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, RIP);
    BX_LINK_TRACE(i);
  }

  BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
  BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JBE_Jq(bxInstruction_c *i)
{
  if (get_CF() || get_ZF()) {
    branch_near64(i);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, RIP);
    BX_LINK_TRACE(i);
  }

  BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
  BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JNBE_Jq(bxInstruction_c *i)
{
  if (! (get_CF() || get_ZF())) {
    branch_near64(i);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, RIP);
    BX_LINK_TRACE(i);
  }

  BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
  BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JS_Jq(bxInstruction_c *i)
{
  if (get_SF()) {
    branch_near64(i);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, RIP);
    BX_LINK_TRACE(i);
  }

  BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
  BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JNS_Jq(bxInstruction_c *i)
{
  if (! get_SF()) {
    branch_near64(i);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, RIP);
    BX_LINK_TRACE(i);
  }

  BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
  BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JP_Jq(bxInstruction_c *i)
{
  if (get_PF()) {
    branch_near64(i);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, RIP);
    BX_LINK_TRACE(i);
  }

  BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
  BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JNP_Jq(bxInstruction_c *i)
{
  if (! get_PF()) {
    branch_near64(i);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, RIP);
    BX_LINK_TRACE(i);
  }

  BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
  BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JL_Jq(bxInstruction_c *i)
{
  if (getB_SF() != getB_OF()) {
    branch_near64(i);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, RIP);
    BX_LINK_TRACE(i);
  }

  BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
  BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JNL_Jq(bxInstruction_c *i)
{
  if (getB_SF() == getB_OF()) {
    branch_near64(i);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, RIP);
    BX_LINK_TRACE(i);
  }

  BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
  BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JLE_Jq(bxInstruction_c *i)
{
  if (get_ZF() || (getB_SF() != getB_OF())) {
    branch_near64(i);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, RIP);
    BX_LINK_TRACE(i);
  }

  BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
  BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JNLE_Jq(bxInstruction_c *i)
{
  if (! get_ZF() && (getB_SF() == getB_OF())) {
    branch_near64(i);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, RIP);
    BX_LINK_TRACE(i);
  }

  BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
  BX_NEXT_INSTR(i); // trace can continue over non-taken branch
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::ENTER64_IwIb(bxInstruction_c *i)
{
  Bit8u level = i->Ib2();
  level &= 0x1F;

  Bit64u temp_RSP = RSP, temp_RBP = RBP;

  temp_RSP -= 8;
  stack_write_qword(temp_RSP, temp_RBP);

  Bit64u frame_ptr64 = temp_RSP;

  if (level > 0) {
    /* do level-1 times */
    while (--level) {
      temp_RBP -= 8;
      Bit64u temp64 = stack_read_qword(temp_RBP);
      temp_RSP -= 8;
      stack_write_qword(temp_RSP, temp64);
    } /* while (--level) */

    /* push(frame pointer) */
    temp_RSP -= 8;
    stack_write_qword(temp_RSP, frame_ptr64);
  } /* if (level > 0) ... */

  temp_RSP -= i->Iw();

  // ENTER finishes with memory write check on the final stack pointer
  // the memory is touched but no write actually occurs
  // emulate it by doing RMW read access from SS:RSP
  read_RMW_linear_qword(BX_SEG_REG_SS, temp_RSP); // no lock, should be touch only

  RBP = frame_ptr64;
  RSP = temp_RSP;

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LEAVE64(bxInstruction_c *i)
{
  // restore frame pointer
  Bit64u temp64 = stack_read_qword(RBP);
  RSP = RBP + 8;
  RBP = temp64;

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::IRET64(bxInstruction_c *i)
{
  invalidate_prefetch_q();

  BX_INSTR_FAR_BRANCH_ORIGIN();

#if BX_SUPPORT_SVM
  if (BX_CPU_THIS_PTR in_svm_guest) {
    if (SVM_INTERCEPT(SVM_INTERCEPT0_IRET)) Svm_Vmexit(SVM_VMEXIT_IRET);
  }
#endif

#if BX_SUPPORT_VMX
  if (BX_CPU_THIS_PTR in_vmx_guest)
    if (is_masked_event(BX_CPU_THIS_PTR vmcs.pin_vmexec_ctrls.VIRTUAL_NMI() ? BX_EVENT_VMX_VIRTUAL_NMI : BX_EVENT_NMI))
      BX_CPU_THIS_PTR nmi_unblocking_iret = true;

  if (BX_CPU_THIS_PTR in_vmx_guest && BX_CPU_THIS_PTR vmcs.pin_vmexec_ctrls.NMI_EXITING()) {
    if (BX_CPU_THIS_PTR vmcs.pin_vmexec_ctrls.VIRTUAL_NMI()) unmask_event(BX_EVENT_VMX_VIRTUAL_NMI);
  }
  else
#endif
    unmask_event(BX_EVENT_NMI);

#if BX_SUPPORT_MONITOR_MWAIT
  BX_CPU_THIS_PTR monitor.reset_umonitor();
#endif

  RSP_SPECULATIVE;

  long_iret(i);

  RSP_COMMIT;

#if BX_SUPPORT_VMX
  BX_CPU_THIS_PTR nmi_unblocking_iret = false;
#endif

  BX_INSTR_FAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_IRET,
                      FAR_BRANCH_PREV_CS, FAR_BRANCH_PREV_RIP,
                      BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value, RIP);

  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_CR0Rq(bxInstruction_c *i)
{
  if (CPL!=0) {
    exception(BX_GP_EXCEPTION, 0);
  }

  invalidate_prefetch_q();

  Bit64u val_64 = BX_READ_64BIT_REG(i->src());

  if (i->dst() == 0) {
    // CR0
#if BX_SUPPORT_VMX
    if (BX_CPU_THIS_PTR in_vmx_guest)
      val_64 = VMexit_CR0_Write(i, val_64);
#endif
    if (! SetCR0(i, val_64))
      exception(BX_GP_EXCEPTION, 0);

    BX_INSTR_TLB_CNTRL(BX_CPU_ID, BX_INSTR_MOV_CR0, (Bit32u) val_64);
  }
  else {
    WriteCR8(i, val_64);
  }

  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_CR2Rq(bxInstruction_c *i)
{
  if (i->dst() != 2) {
    exception(BX_UD_EXCEPTION, 0);
  }

  if (CPL!=0) {
    exception(BX_GP_EXCEPTION, 0);
  }

#if BX_SUPPORT_SVM
  if (BX_CPU_THIS_PTR in_svm_guest) {
    if(SVM_CR_WRITE_INTERCEPTED(2)) {
      if (BX_SUPPORT_SVM_EXTENSION(BX_CPUID_SVM_DECODE_ASSIST))
        Svm_Vmexit(SVM_VMEXIT_CR2_WRITE, BX_SVM_CR_WRITE_MASK | i->src());
      else
        Svm_Vmexit(SVM_VMEXIT_CR2_WRITE);
    }
  }
#endif

  BX_CPU_THIS_PTR cr2 = BX_READ_64BIT_REG(i->src());

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_CR3Rq(bxInstruction_c *i)
{
  if (i->dst() != 3) {
    exception(BX_UD_EXCEPTION, 0);
  }

  if (CPL!=0) {
    exception(BX_GP_EXCEPTION, 0);
  }

  invalidate_prefetch_q();

  Bit64u val_64 = BX_READ_64BIT_REG(i->src());

#if BX_SUPPORT_VMX
  if (BX_CPU_THIS_PTR in_vmx_guest)
    VMexit_CR3_Write(i, val_64);
#endif

#if BX_SUPPORT_SVM
  if (BX_CPU_THIS_PTR in_svm_guest) {
    if(SVM_CR_WRITE_INTERCEPTED(3)) {
      if (BX_SUPPORT_SVM_EXTENSION(BX_CPUID_SVM_DECODE_ASSIST))
        Svm_Vmexit(SVM_VMEXIT_CR3_WRITE, BX_SVM_CR_WRITE_MASK | i->src());
      else
        Svm_Vmexit(SVM_VMEXIT_CR3_WRITE);
    }
  }
#endif

  // allow bit 63 (hint that TLB doesn't need to be cleared) to be set when
  // PCIDE is set, but ignore the hint if given
  if (BX_CPU_THIS_PTR cr4.get_PCIDE()) {
    val_64 &= ~(BX_CONST64(1)<<63);
  }

  if (! SetCR3(val_64))
    exception(BX_GP_EXCEPTION, 0);

  BX_INSTR_TLB_CNTRL(BX_CPU_ID, BX_INSTR_MOV_CR3, val_64);

  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_CR4Rq(bxInstruction_c *i)
{
  if (i->dst() != 4) {
    exception(BX_UD_EXCEPTION, 0);
  }

  if (CPL!=0) {
    exception(BX_GP_EXCEPTION, 0);
  }

  invalidate_prefetch_q();

  Bit64u val_64 = BX_READ_64BIT_REG(i->src());
#if BX_SUPPORT_VMX
  if (BX_CPU_THIS_PTR in_vmx_guest)
    val_64 = VMexit_CR4_Write(i, val_64);
#endif
  if (! SetCR4(i, val_64))
    exception(BX_GP_EXCEPTION, 0);

  BX_INSTR_TLB_CNTRL(BX_CPU_ID, BX_INSTR_MOV_CR4, (Bit32u) val_64);

  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_RqCR0(bxInstruction_c *i)
{
  if (CPL!=0) {
    exception(BX_GP_EXCEPTION, 0);
  }

  Bit64u val_64;

  if (i->src() == 0) {
    // CR0
#if BX_SUPPORT_SVM
    if (BX_CPU_THIS_PTR in_svm_guest) {
      if(SVM_CR_READ_INTERCEPTED(0))
        Svm_Vmexit(SVM_VMEXIT_CR0_READ, BX_SUPPORT_SVM_EXTENSION(BX_CPUID_SVM_DECODE_ASSIST) ? i->dst() : 0);
    }
#endif

    val_64 = read_CR0(); /* correctly handle VMX */
  }
  else {
    // CR8
    val_64 = ReadCR8(i);
  }

  BX_WRITE_64BIT_REG(i->dst(), val_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_RqCR2(bxInstruction_c *i)
{
  if (i->src() != 2) {
    exception(BX_UD_EXCEPTION, 0);
  }

  if (CPL!=0) {
    exception(BX_GP_EXCEPTION, 0);
  }

#if BX_SUPPORT_SVM
  if (BX_CPU_THIS_PTR in_svm_guest) {
    if(SVM_CR_READ_INTERCEPTED(2))
      Svm_Vmexit(SVM_VMEXIT_CR2_READ, BX_SUPPORT_SVM_EXTENSION(BX_CPUID_SVM_DECODE_ASSIST) ? i->dst() : 0);
  }
#endif

  BX_WRITE_64BIT_REG(i->dst(), BX_CPU_THIS_PTR cr2);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_RqCR3(bxInstruction_c *i)
{
  if (i->src() != 3) {
    exception(BX_UD_EXCEPTION, 0);
  }

  if (CPL!=0) {
    exception(BX_GP_EXCEPTION, 0);
  }

#if BX_SUPPORT_SVM
  if (BX_CPU_THIS_PTR in_svm_guest) {
    if(SVM_CR_READ_INTERCEPTED(3))
      Svm_Vmexit(SVM_VMEXIT_CR3_READ, BX_SUPPORT_SVM_EXTENSION(BX_CPUID_SVM_DECODE_ASSIST) ? i->dst() : 0);
  }
#endif

#if BX_SUPPORT_VMX
  if (BX_CPU_THIS_PTR in_vmx_guest)
    VMexit_CR3_Read(i);
#endif

  BX_WRITE_64BIT_REG(i->dst(), BX_CPU_THIS_PTR cr3);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_RqCR4(bxInstruction_c *i)
{
  if (i->src() != 4) {
    exception(BX_UD_EXCEPTION, 0);
  }

  if (CPL!=0) {
    exception(BX_GP_EXCEPTION, 0);
  }

#if BX_SUPPORT_SVM
  if (BX_CPU_THIS_PTR in_svm_guest) {
    if(SVM_CR_READ_INTERCEPTED(4))
      Svm_Vmexit(SVM_VMEXIT_CR4_READ, BX_SUPPORT_SVM_EXTENSION(BX_CPUID_SVM_DECODE_ASSIST) ? i->dst() : 0);
  }
#endif

  Bit64u val_64 = read_CR4(); /* correctly handle VMX */

  BX_WRITE_64BIT_REG(i->dst(), val_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_DqRq(bxInstruction_c *i)
{
#if BX_SUPPORT_VMX
  if (BX_CPU_THIS_PTR in_vmx_guest)
    VMexit_DR_Access(0 /* write */, i->dst(), i->src());
#endif

  if (BX_CPU_THIS_PTR cr4.get_DE()) {
    if ((i->dst() & 0xE) == 4) {
      exception(BX_UD_EXCEPTION, 0);
    }
  }

  if (i->dst() >= 8) {
    exception(BX_UD_EXCEPTION, 0);
  }

  // Note: processor clears GD upon entering debug exception
  // handler, to allow access to the debug registers
  if (BX_CPU_THIS_PTR dr7.get_GD()) {
    BX_CPU_THIS_PTR debug_trap |= BX_DEBUG_DR_ACCESS_BIT;
    exception(BX_DB_EXCEPTION, 0);
  }

  /* #GP(0) if CPL is not 0 */
  if (CPL != 0) {
    exception(BX_GP_EXCEPTION, 0);
  }

#if BX_SUPPORT_SVM
  if (BX_CPU_THIS_PTR in_svm_guest) {
    if (SVM_DR_WRITE_INTERCEPTED(i->dst()))
      Svm_Vmexit(SVM_VMEXIT_DR0_WRITE + i->dst(), BX_SUPPORT_SVM_EXTENSION(BX_CPUID_SVM_DECODE_ASSIST) ? i->src() : 0);
  }
#endif

  invalidate_prefetch_q();

  Bit64u val_64 = BX_READ_64BIT_REG(i->src());

  switch (i->dst()) {
    case 0: // DR0
    case 1: // DR1
    case 2: // DR2
    case 3: // DR3
      BX_CPU_THIS_PTR dr[i->dst()] = val_64;
      TLB_invlpg(val_64);
      break;

    case 4: // DR4
      // DR4 aliased to DR6 by default. With Debug Extensions ON,
      // access to DR4 causes #UD
    case 6: // DR6
      if (GET32H(val_64)) {
        exception(BX_GP_EXCEPTION, 0);
      }
      // On Pentium+, bit12 is always zero
      BX_CPU_THIS_PTR dr6.val32 = (BX_CPU_THIS_PTR dr6.val32 & 0xffff0ff0) |
                            (val_64 & 0x0000e00f);
      break;

    case 5: // DR5
      // DR5 aliased to DR7 by default. With Debug Extensions ON,
      // access to DR5 causes #UD
    case 7: // DR7
      // Note: 486+ ignore GE and LE flags.  On the 386, exact
      // data breakpoint matching does not occur unless it is enabled
      // by setting the LE and/or GE flags.

      if (GET32H(val_64)) {
        exception(BX_GP_EXCEPTION, 0);
      }

      // Pentium+: bits15,14,12 are hardwired to 0, rest are settable.
      // Even bits 11,10 are changeable though reserved.
      BX_CPU_THIS_PTR dr7.set32((val_64 & 0xffff2fff) | 0x00000400);

      TLB_flush(); // the DR7 write could enable some breakpoints
      break;

    default:
      exception(BX_UD_EXCEPTION, 0);
  }

  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_RqDq(bxInstruction_c *i)
{
  Bit64u val_64;

#if BX_SUPPORT_VMX
  if (BX_CPU_THIS_PTR in_vmx_guest)
    VMexit_DR_Access(1 /* read */, i->src(), i->dst());
#endif

  if (BX_CPU_THIS_PTR cr4.get_DE()) {
    if ((i->src() & 0xE) == 4) {
      exception(BX_UD_EXCEPTION, 0);
    }
  }

  if (i->src() >= 8) {
    exception(BX_UD_EXCEPTION, 0);
  }

  // Note: processor clears GD upon entering debug exception
  // handler, to allow access to the debug registers
  if (BX_CPU_THIS_PTR dr7.get_GD()) {
    BX_CPU_THIS_PTR debug_trap |= BX_DEBUG_DR_ACCESS_BIT;
    exception(BX_DB_EXCEPTION, 0);
  }

  /* #GP(0) if CPL is not 0 */
  if (CPL != 0) {
    exception(BX_GP_EXCEPTION, 0);
  }

#if BX_SUPPORT_SVM
  if (BX_CPU_THIS_PTR in_svm_guest) {
    if (SVM_DR_READ_INTERCEPTED(i->src()))
      Svm_Vmexit(SVM_VMEXIT_DR0_READ + i->src(), BX_SUPPORT_SVM_EXTENSION(BX_CPUID_SVM_DECODE_ASSIST) ? i->dst() : 0);
  }
#endif

  switch (i->src()) {
    case 0: // DR0
    case 1: // DR1
    case 2: // DR2
    case 3: // DR3
      val_64 = BX_CPU_THIS_PTR dr[i->src()];
      break;

    case 4: // DR4
      // DR4 aliased to DR6 by default. With Debug Extensions ON,
      // access to DR4 causes #UD
    case 6: // DR6
      val_64 = BX_CPU_THIS_PTR dr6.get32();
      break;

    case 5: // DR5
      // DR5 aliased to DR7 by default. With Debug Extensions ON,
      // access to DR5 causes #UD
    case 7: // DR7
      val_64 = BX_CPU_THIS_PTR dr7.get32();
      break;

    default:
      exception(BX_UD_EXCEPTION, 0);
  }

  BX_WRITE_64BIT_REG(i->dst(), val_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV64_GdEdM(bxInstruction_c *i)
{
  Bit64u eaddr = BX_CPU_RESOLVE_ADDR_64(i);
  Bit32u val32 = read_linear_dword(i->seg(), get_laddr64(i->seg(), eaddr));
  BX_WRITE_32BIT_REGZ(i->dst(), val32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV64_EdGdM(bxInstruction_c *i)
{
  Bit64u eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  write_linear_dword(i->seg(), get_laddr64(i->seg(), eaddr), BX_READ_32BIT_REG(i->src()));

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOVZX_GqEbM(bxInstruction_c *i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  Bit8u op2_8 = read_linear_byte(i->seg(), get_laddr64(i->seg(), eaddr));

  /* zero extend byte op2 into qword op1 */
  BX_WRITE_64BIT_REG(i->dst(), (Bit64u) op2_8);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOVZX_GqEwM(bxInstruction_c *i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  Bit16u op2_16 = read_linear_word(i->seg(), get_laddr64(i->seg(), eaddr));

  /* zero extend word op2 into qword op1 */
  BX_WRITE_64BIT_REG(i->dst(), (Bit64u) op2_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOVSX_GqEbM(bxInstruction_c *i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  Bit8u op2_8 = read_linear_byte(i->seg(), get_laddr64(i->seg(), eaddr));

  /* sign extend byte op2 into qword op1 */
  BX_WRITE_64BIT_REG(i->dst(), (Bit8s) op2_8);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOVSX_GqEwM(bxInstruction_c *i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  Bit16u op2_16 = read_linear_word(i->seg(), get_laddr64(i->seg(), eaddr));

  /* sign extend word op2 into qword op1 */
  BX_WRITE_64BIT_REG(i->dst(), (Bit16s) op2_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOVSX_GqEdM(bxInstruction_c *i)
{
  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  Bit32u op2_32 = read_linear_dword(i->seg(), get_laddr64(i->seg(), eaddr));

  /* sign extend word op2 into qword op1 */
  BX_WRITE_64BIT_REG(i->dst(), (Bit32s) op2_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOVZX_GqEbR(bxInstruction_c *i)
{
  Bit8u op2_8 = BX_READ_8BIT_REGx(i->src(), i->extend8bitL());

  /* zero extend byte op2 into qword op1 */
  BX_WRITE_64BIT_REG(i->dst(), (Bit64u) op2_8);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOVZX_GqEwR(bxInstruction_c *i)
{
  Bit16u op2_16 = BX_READ_16BIT_REG(i->src());

  /* zero extend word op2 into qword op1 */
  BX_WRITE_64BIT_REG(i->dst(), (Bit64u) op2_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOVSX_GqEbR(bxInstruction_c *i)
{
  Bit8u op2_8 = BX_READ_8BIT_REGx(i->src(), i->extend8bitL());

  /* sign extend byte op2 into qword op1 */
  BX_WRITE_64BIT_REG(i->dst(), (Bit8s) op2_8);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOVSX_GqEwR(bxInstruction_c *i)
{
  Bit16u op2_16 = BX_READ_16BIT_REG(i->src());

  /* sign extend word op2 into qword op1 */
  BX_WRITE_64BIT_REG(i->dst(), (Bit16s) op2_16);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOVSX_GqEdR(bxInstruction_c *i)
{
  Bit32u op2_32 = BX_READ_32BIT_REG(i->src());

  /* sign extend word op2 into qword op1 */
  BX_WRITE_64BIT_REG(i->dst(), (Bit32s) op2_32);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MUL_RAXEqR(bxInstruction_c *i)
{
  Bit128u product_128;

  Bit64u op1_64 = RAX;
  Bit64u op2_64 = BX_READ_64BIT_REG(i->src());

  // product_128 = ((Bit128u) op1_64) * ((Bit128u) op2_64);
  // product_64l = (Bit64u) (product_128 & 0xFFFFFFFFFFFFFFFF);
  // product_64h = (Bit64u) (product_128 >> 64);

  long_mul(&product_128,op1_64,op2_64);

  /* now write product back to destination */
  RAX = product_128.lo;
  RDX = product_128.hi;

  /* set EFLAGS */
  SET_FLAGS_OSZAPC_LOGIC_64(product_128.lo);
  if(product_128.hi != 0)
  {
    BX_CPU_THIS_PTR oszapc.assert_flags_OxxxxC();
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::IMUL_RAXEqR(bxInstruction_c *i)
{
  Bit128s product_128;

  Bit64s op1_64 = RAX;
  Bit64s op2_64 = BX_READ_64BIT_REG(i->src());

  // product_128 = ((Bit128s) op1_64) * ((Bit128s) op2_64);
  // product_64l = (Bit64u) (product_128 & 0xFFFFFFFFFFFFFFFF);
  // product_64h = (Bit64u) (product_128 >> 64);

  long_imul(&product_128,op1_64,op2_64);

  /* now write product back to destination */
  RAX = product_128.lo;
  RDX = product_128.hi;

  /* set eflags:
   * IMUL r/m64: condition for clearing CF & OF:
   *   RDX:RAX = sign-extend of RAX
   */

  SET_FLAGS_OSZAPC_LOGIC_64(product_128.lo);

  /* magic compare between RDX:RAX and sign extended RAX */
  if (((Bit64u)(product_128.hi) + (product_128.lo >> 63)) != 0) {
    BX_CPU_THIS_PTR oszapc.assert_flags_OxxxxC();
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::DIV_RAXEqR(bxInstruction_c *i)
{
  Bit64u remainder_64, quotient_64l;
  Bit128u op1_128, quotient_128;

  Bit64u op2_64 = BX_READ_64BIT_REG(i->src());
  if (op2_64 == 0) {
    exception(BX_DE_EXCEPTION, 0);
  }

  op1_128.lo = RAX;
  op1_128.hi = RDX;

  // quotient_128 = op1_128 / op2_64;
  // remainder_64 = (Bit64u) (op1_128 % op2_64);
  // quotient_64l = (Bit64u) (quotient_128 & 0xFFFFFFFFFFFFFFFF);

  long_div(&quotient_128,&remainder_64,&op1_128,op2_64);
  quotient_64l = quotient_128.lo;

  if (quotient_128.hi != 0)
    exception(BX_DE_EXCEPTION, 0);

  /* set EFLAGS:
   * DIV affects the following flags: O,S,Z,A,P,C are undefined
   */

  /* now write quotient back to destination */
  RAX = quotient_64l;
  RDX = remainder_64;

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::IDIV_RAXEqR(bxInstruction_c *i)
{
  Bit64s remainder_64, quotient_64l;
  Bit128s op1_128, quotient_128;

  op1_128.lo = RAX;
  op1_128.hi = RDX;

  /* check MIN_INT case */
  if ((op1_128.hi == (Bit64s) BX_CONST64(0x8000000000000000)) && (!op1_128.lo))
    exception(BX_DE_EXCEPTION, 0);

  Bit64s op2_64 = BX_READ_64BIT_REG(i->src());

  if (op2_64 == 0) {
    exception(BX_DE_EXCEPTION, 0);
  }

  // quotient_128 = op1_128 / op2_64;
  // remainder_64 = (Bit64s) (op1_128 % op2_64);
  // quotient_64l = (Bit64s) (quotient_128 & 0xFFFFFFFFFFFFFFFF);

  long_idiv(&quotient_128,&remainder_64,&op1_128,op2_64);
  quotient_64l = quotient_128.lo;

  if ((!(quotient_128.lo & BX_CONST64(0x8000000000000000)) && quotient_128.hi != (Bit64s) 0) ||
       ((quotient_128.lo & BX_CONST64(0x8000000000000000)) && quotient_128.hi != (Bit64s) BX_CONST64(0xffffffffffffffff)))
  {
    exception(BX_DE_EXCEPTION, 0);
  }

  /* set EFLAGS:
   * IDIV affects the following flags: O,S,Z,A,P,C are undefined
   */

  /* now write quotient back to destination */
  RAX = quotient_64l;
  RDX = remainder_64;

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::IMUL_GqEqR(bxInstruction_c *i)
{
  Bit128s product_128;

  Bit64s op1_64 = BX_READ_64BIT_REG(i->dst());
  Bit64s op2_64 = BX_READ_64BIT_REG(i->src());

  long_imul(&product_128,op1_64,op2_64);

  /* now write product back to destination */
  BX_WRITE_64BIT_REG(i->dst(), product_128.lo);

  SET_FLAGS_OSZAPC_LOGIC_64(product_128.lo);

  if (((Bit64u)(product_128.hi) + (product_128.lo >> 63)) != 0) {
    BX_CPU_THIS_PTR oszapc.assert_flags_OxxxxC();
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::IMUL_GqEqIdR(bxInstruction_c *i)
{
  Bit128s product_128;

  Bit64s op1_64 = BX_READ_64BIT_REG(i->src());
  Bit64s op2_64 = (Bit32s) i->Id();

  long_imul(&product_128,op1_64,op2_64);

  /* now write product back to destination */
  BX_WRITE_64BIT_REG(i->dst(), product_128.lo);

  SET_FLAGS_OSZAPC_LOGIC_64(product_128.lo);

  if (((Bit64u)(product_128.hi) + (product_128.lo >> 63)) != 0) {
    BX_CPU_THIS_PTR oszapc.assert_flags_OxxxxC();
  }

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CALL_EqR(bxInstruction_c *i)
{

  Bit64u new_RIP = BX_READ_64BIT_REG(i->dst());

  RSP_SPECULATIVE;

  /* push 64 bit EA of next instruction */
  push_64(RIP);
#if BX_SUPPORT_CET
  if (ShadowStackEnabled(CPL))
    shadow_stack_push_64(RIP);
#endif

  if (! IsCanonical(new_RIP))
  {
    exception(BX_GP_EXCEPTION, 0);
  }

  RIP = new_RIP;

  RSP_COMMIT;

#if BX_SUPPORT_CET
  track_indirect_if_not_suppressed(i, CPL);
#endif

  BX_INSTR_UCNEAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_CALL_INDIRECT, PREV_RIP, RIP);

  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::CALL64_Ep(bxInstruction_c *i)
{
  invalidate_prefetch_q();

  BX_INSTR_FAR_BRANCH_ORIGIN();

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  /* pointer, segment address pair */
  Bit64u op1_64 = read_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));
  Bit16u cs_raw = read_linear_word(i->seg(), get_laddr64(i->seg(), (eaddr+8) & i->asize_mask()));

  RSP_SPECULATIVE;

  call_protected(i, cs_raw, op1_64);

  RSP_COMMIT;

  BX_INSTR_FAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_CALL_INDIRECT,
                      FAR_BRANCH_PREV_CS, FAR_BRANCH_PREV_RIP,
                      BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value, RIP);

  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JMP_EqR(bxInstruction_c *i)
{
  Bit64u op1_64 = BX_READ_64BIT_REG(i->dst());

  if (! IsCanonical(op1_64)) {
    exception(BX_GP_EXCEPTION, 0);
  }

  RIP = op1_64;
  BX_INSTR_UCNEAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_JMP_INDIRECT, PREV_RIP, RIP);

#if BX_SUPPORT_CET
  track_indirect_if_not_suppressed(i, CPL);
#endif

  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JMP64_Ep(bxInstruction_c *i)
{
  invalidate_prefetch_q();

  BX_INSTR_FAR_BRANCH_ORIGIN();

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  Bit64u op1_64 = read_linear_qword(i->seg(), get_laddr64(i->seg(), eaddr));
  Bit16u cs_raw = read_linear_word(i->seg(), get_laddr64(i->seg(), (eaddr+8) & i->asize_mask()));

  jump_protected(i, cs_raw, op1_64);

  BX_INSTR_FAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_JMP_INDIRECT,
                      FAR_BRANCH_PREV_CS, FAR_BRANCH_PREV_RIP,
                      BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value, RIP);

  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::PUSHF_Fq(bxInstruction_c *i)
{
#if BX_SUPPORT_SVM
  if (BX_CPU_THIS_PTR in_svm_guest) {
    if (SVM_INTERCEPT(SVM_INTERCEPT0_PUSHF)) Svm_Vmexit(SVM_VMEXIT_PUSHF);
  }
#endif

  // VM & RF flags cleared in image stored on the stack
  push_64(read_eflags() & 0x00fcffff);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::POPF_Fq(bxInstruction_c *i)
{
#if BX_SUPPORT_SVM
  if (BX_CPU_THIS_PTR in_svm_guest) {
    if (SVM_INTERCEPT(SVM_INTERCEPT0_POPF)) Svm_Vmexit(SVM_VMEXIT_POPF);
  }
#endif

  // Build a mask of the following bits:
  // ID,VIP,VIF,AC,VM,RF,x,NT,IOPL,OF,DF,IF,TF,SF,ZF,x,AF,x,PF,x,CF
  Bit32u changeMask = EFlagsOSZAPCMask | EFlagsTFMask | EFlagsDFMask
                        | EFlagsNTMask | EFlagsRFMask | EFlagsACMask
                        | EFlagsIDMask;

  BX_ASSERT (protected_mode());

  // RF is always zero after the execution of POPF.
  Bit32u eflags32 = (Bit32u) pop_64() & ~EFlagsRFMask;

  if (CPL==0)
    changeMask |= EFlagsIOPLMask;
  if (CPL <= BX_CPU_THIS_PTR get_IOPL())
    changeMask |= EFlagsIFMask;

  // VIF, VIP, VM are unaffected
  writeEFlags(eflags32, changeMask);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::RETnear64_Iw(bxInstruction_c *i)
{

  RSP_SPECULATIVE;

  Bit64u return_RIP = pop_64();
#if BX_SUPPORT_CET
  if (ShadowStackEnabled(CPL)) {
    Bit64u shadow_RIP = shadow_stack_pop_64();
    if (shadow_RIP != return_RIP)
      exception(BX_CP_EXCEPTION, BX_CP_NEAR_RET);
  }
#endif

  if (! IsCanonical(return_RIP)) {
    exception(BX_GP_EXCEPTION, 0);
  }

  RIP = return_RIP;
  RSP += i->Iw();

  RSP_COMMIT;

  BX_INSTR_UCNEAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_RET, PREV_RIP, RIP);

  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::RETfar64_Iw(bxInstruction_c *i)
{
  invalidate_prefetch_q();

  BX_INSTR_FAR_BRANCH_ORIGIN();

  RSP_SPECULATIVE;

  // return_protected is RSP safe
  return_protected(i, i->Iw());

  RSP_COMMIT;

  BX_INSTR_FAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_RET,
                      FAR_BRANCH_PREV_CS, FAR_BRANCH_PREV_RIP,
                      BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value, RIP);

  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MOV_RRXIq(bxInstruction_c *i)
{
  BX_WRITE_64BIT_REG(i->dst(), i->Iq());

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LSS_GqMp(bxInstruction_c *i)
{
  load_segq(i, BX_SEG_REG_SS);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LFS_GqMp(bxInstruction_c *i)
{
  load_segq(i, BX_SEG_REG_FS);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LGS_GqMp(bxInstruction_c *i)
{
  load_segq(i, BX_SEG_REG_GS);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SGDT64_Ms(bxInstruction_c *i)
{
  if (CPL!=0 && BX_CPU_THIS_PTR cr4.get_UMIP()) {
    exception(BX_GP_EXCEPTION, 0);
  }

#if BX_SUPPORT_VMX >= 2
  if (BX_CPU_THIS_PTR in_vmx_guest)
    if (BX_CPU_THIS_PTR vmcs.vmexec_ctrls2.DESCRIPTOR_TABLE_VMEXIT())
      VMexit_Instruction(i, VMX_VMEXIT_GDTR_IDTR_ACCESS, BX_READ);
#endif

#if BX_SUPPORT_SVM
  if (BX_CPU_THIS_PTR in_svm_guest) {
    if (SVM_INTERCEPT(SVM_INTERCEPT0_GDTR_READ)) Svm_Vmexit(SVM_VMEXIT_GDTR_READ);
  }
#endif

  Bit16u limit_16 = BX_CPU_THIS_PTR gdtr.limit;
  Bit64u base_64  = BX_CPU_THIS_PTR gdtr.base;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  write_linear_word(i->seg(), get_laddr64(i->seg(), eaddr), limit_16);
  write_linear_qword(i->seg(), get_laddr64(i->seg(), (eaddr+2) & i->asize_mask()), base_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SIDT64_Ms(bxInstruction_c *i)
{
  if (CPL!=0 && BX_CPU_THIS_PTR cr4.get_UMIP()) {
    exception(BX_GP_EXCEPTION, 0);
  }

#if BX_SUPPORT_VMX >= 2
  if (BX_CPU_THIS_PTR in_vmx_guest)
    if (BX_CPU_THIS_PTR vmcs.vmexec_ctrls2.DESCRIPTOR_TABLE_VMEXIT())
      VMexit_Instruction(i, VMX_VMEXIT_GDTR_IDTR_ACCESS, BX_READ);
#endif

#if BX_SUPPORT_SVM
  if (BX_CPU_THIS_PTR in_svm_guest) {
    if (SVM_INTERCEPT(SVM_INTERCEPT0_IDTR_READ)) Svm_Vmexit(SVM_VMEXIT_IDTR_READ);
  }
#endif

  Bit16u limit_16 = BX_CPU_THIS_PTR idtr.limit;
  Bit64u base_64  = BX_CPU_THIS_PTR idtr.base;

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  write_linear_word(i->seg(), get_laddr64(i->seg(), eaddr), limit_16);
  write_linear_qword(i->seg(), get_laddr64(i->seg(), (eaddr+2) & i->asize_mask()), base_64);

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LGDT64_Ms(bxInstruction_c *i)
{

  if (CPL!=0) {
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

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  Bit64u base_64 = read_linear_qword(i->seg(), get_laddr64(i->seg(), (eaddr + 2) & i->asize_mask()));
  if (! IsCanonical(base_64)) {
    exception(BX_GP_EXCEPTION, 0);
  }
  Bit16u limit_16 = read_linear_word(i->seg(), get_laddr64(i->seg(), eaddr));

  BX_CPU_THIS_PTR gdtr.limit = limit_16;
  BX_CPU_THIS_PTR gdtr.base = base_64;

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LIDT64_Ms(bxInstruction_c *i)
{

  if (CPL != 0) {
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

  bx_address eaddr = BX_CPU_RESOLVE_ADDR_64(i);

  Bit64u base_64 = read_linear_qword(i->seg(), get_laddr64(i->seg(), (eaddr + 2) & i->asize_mask()));
  if (! IsCanonical(base_64)) {
    exception(BX_GP_EXCEPTION, 0);
  }
  Bit16u limit_16 = read_linear_word(i->seg(), get_laddr64(i->seg(), eaddr));

  BX_CPU_THIS_PTR idtr.limit = limit_16;
  BX_CPU_THIS_PTR idtr.base = base_64;

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::SWAPGS(bxInstruction_c *i)
{
  if(CPL != 0)
    exception(BX_GP_EXCEPTION, 0);

  Bit64u temp_GS_base = MSR_GSBASE;
  MSR_GSBASE = BX_CPU_THIS_PTR msr.kernelgsbase;
  BX_CPU_THIS_PTR msr.kernelgsbase = temp_GS_base;

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::RDFSBASE_Ed(bxInstruction_c *i)
{
  if (! BX_CPU_THIS_PTR cr4.get_FSGSBASE())
    exception(BX_UD_EXCEPTION, 0);

  BX_WRITE_32BIT_REGZ(i->dst(), (Bit32u) MSR_FSBASE);
  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::RDGSBASE_Ed(bxInstruction_c *i)
{
  if (! BX_CPU_THIS_PTR cr4.get_FSGSBASE())
    exception(BX_UD_EXCEPTION, 0);

  BX_WRITE_32BIT_REGZ(i->dst(), (Bit32u) MSR_GSBASE);
  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::RDFSBASE_Eq(bxInstruction_c *i)
{
  if (! BX_CPU_THIS_PTR cr4.get_FSGSBASE())
    exception(BX_UD_EXCEPTION, 0);

  BX_WRITE_64BIT_REG(i->dst(), MSR_FSBASE);
  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::RDGSBASE_Eq(bxInstruction_c *i)
{
  if (! BX_CPU_THIS_PTR cr4.get_FSGSBASE())
    exception(BX_UD_EXCEPTION, 0);

  BX_WRITE_64BIT_REG(i->dst(), MSR_GSBASE);
  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::WRFSBASE_Ed(bxInstruction_c *i)
{
  if (! BX_CPU_THIS_PTR cr4.get_FSGSBASE())
    exception(BX_UD_EXCEPTION, 0);

  // 32-bit value is always canonical
  MSR_FSBASE = BX_READ_32BIT_REG(i->src());

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::WRGSBASE_Ed(bxInstruction_c *i)
{
  if (! BX_CPU_THIS_PTR cr4.get_FSGSBASE())
    exception(BX_UD_EXCEPTION, 0);

  // 32-bit value is always canonical
  MSR_GSBASE = BX_READ_32BIT_REG(i->src());

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::WRFSBASE_Eq(bxInstruction_c *i)
{
  if (! BX_CPU_THIS_PTR cr4.get_FSGSBASE())
    exception(BX_UD_EXCEPTION, 0);

  Bit64u fsbase = BX_READ_64BIT_REG(i->src());
  if (!IsCanonical(fsbase)) {
    exception(BX_GP_EXCEPTION, 0);
  }
  MSR_FSBASE = fsbase;

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::WRGSBASE_Eq(bxInstruction_c *i)
{
  if (! BX_CPU_THIS_PTR cr4.get_FSGSBASE())
    exception(BX_UD_EXCEPTION, 0);

  Bit64u gsbase = BX_READ_64BIT_REG(i->src());
  if (!IsCanonical(gsbase)) {
    exception(BX_GP_EXCEPTION, 0);
  }
  MSR_GSBASE = gsbase;

  BX_NEXT_INSTR(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LOOPNE64_Jb(bxInstruction_c *i)
{
  if (i->as64L()) {
    Bit64u count = RCX;

    if (((--count) != 0) && (get_ZF()==0)) {
      branch_near64(i);
      BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, RIP);
    }
#if BX_INSTRUMENTATION
    else {
      BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
    }
#endif

    RCX = count;
  }
  else {
    Bit32u count = ECX;

    if (((--count) != 0) && (get_ZF()==0)) {
      branch_near64(i);
      BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, RIP);
    }
#if BX_INSTRUMENTATION
    else {
      BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
    }
#endif

    RCX = count;
  }

  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LOOPE64_Jb(bxInstruction_c *i)
{
  if (i->as64L()) {
    Bit64u count = RCX;

    if (((--count) != 0) && get_ZF()) {
      branch_near64(i);
      BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, RIP);
    }
#if BX_INSTRUMENTATION
    else {
      BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
    }
#endif

    RCX = count;
  }
  else {
    Bit32u count = ECX;

    if (((--count) != 0) && get_ZF()) {
      branch_near64(i);
      BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, RIP);
    }
#if BX_INSTRUMENTATION
    else {
      BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
    }
#endif

    RCX = count;
  }

  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::LOOP64_Jb(bxInstruction_c *i)
{
  if (i->as64L()) {
    Bit64u count = RCX;

    if ((--count) != 0) {
      branch_near64(i);
      BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, RIP);
    }
#if BX_INSTRUMENTATION
    else {
      BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
    }
#endif

    RCX = count;
  }
  else {
    Bit32u count = ECX;

    if ((--count) != 0) {
      branch_near64(i);
      BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, RIP);
    }
#if BX_INSTRUMENTATION
    else {
      BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
    }
#endif

    RCX = count;
  }

  BX_NEXT_TRACE(i);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::JRCXZ_Jb(bxInstruction_c *i)
{
  Bit64u temp_RCX;

  if (i->as64L())
    temp_RCX = RCX;
  else
    temp_RCX = ECX;

  if (temp_RCX == 0) {
    branch_near64(i);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, PREV_RIP, RIP);
    BX_LINK_TRACE(i);
  }

  BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID, PREV_RIP);
  BX_NEXT_TRACE(i);
}
