#include "bochs.h"
#include "cpu.h"
#include "iodev.h"
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

    BX_OUTP(port, BX_CPU_THIS_PTR gen_reg[0].word.byte.rl, 1);

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

    BX_CPU_THIS_PTR gen_reg[0].word.byte.rl = (Bit8u)BX_INP(port, 1);

    BX_NEXT_TRACE(i);
}
