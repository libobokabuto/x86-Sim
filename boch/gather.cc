#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_SUPPORT_AVX

bx_address BX_CPP_AttrRegparmN(2) BX_CPU_C::BxResolveGatherD(bxInstruction_c* i, unsigned element)
{
    Bit32s index = BX_READ_AVX_REG(i->sibIndex()).vmm32s(element);

    if (i->as64L())
        return (BX_READ_64BIT_REG(i->sibBase()) + (((Bit64s)index) << i->sibScale()) + i->displ32s());
    else
        return (Bit32u)(BX_READ_32BIT_REG(i->sibBase()) + (index << i->sibScale()) + i->displ32s());
}

bx_address BX_CPP_AttrRegparmN(2) BX_CPU_C::BxResolveGatherQ(bxInstruction_c* i, unsigned element)
{
    Bit64s index = BX_READ_AVX_REG(i->sibIndex()).vmm64s(element);

    if (i->as64L())
        return (BX_READ_64BIT_REG(i->sibBase()) + (index << i->sibScale()) + i->displ32s());
    else
        return (Bit32u)(BX_READ_32BIT_REG(i->sibBase()) + (index << i->sibScale()) + i->displ32s());
}
#endif