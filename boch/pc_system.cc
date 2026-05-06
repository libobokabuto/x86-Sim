#include "bochs.h"
#include "cpu.h"
#include "pc_system.h"

void bx_pc_system_c::set_enable_a20(bool value)
{
#if BX_SUPPORT_A20
    enable_a20 = value ? true : false;

    if (value) {
#if BX_CPU_LEVEL < 2
        a20_mask = 0xfffff;
#elif BX_CPU_LEVEL == 2
        a20_mask = 0xffffff;
#elif BX_PHY_ADDRESS_LONG
        a20_mask = BX_CONST64(0xffffffffffffffff);
#else
        a20_mask = 0xffffffff;
#endif
    }
    else {
#if BX_PHY_ADDRESS_LONG
        a20_mask = BX_CONST64(0xffffffffffefffff);
#else
        a20_mask = 0xffefffff;
#endif
    }
#else
    UNUSED(value);
    enable_a20 = true;
    a20_mask = (bx_phy_address)(~(bx_phy_address)0);
#endif
}

bool bx_pc_system_c::get_enable_a20(void)
{
#if BX_SUPPORT_A20
    return enable_a20;
#else
    return true;
#endif
}

int bx_pc_system_c::Reset(unsigned type)
{
    set_enable_a20(1);
    BX_CPU(0)->reset(type);
    return 0;
}

