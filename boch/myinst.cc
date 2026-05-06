#include "bochs.h"
#include "cpu.h"

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
