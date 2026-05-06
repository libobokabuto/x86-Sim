#pragma once
BOCHSAPI extern class bx_pc_system_c bx_pc_system; //31
class BOCHSAPI bx_pc_system_c {
public:
	bx_phy_address a20_mask;
};
#if BX_SUPPORT_A20
#  define A20ADDR(x)                ((bx_phy_address)(x) & bx_pc_system.a20_mask)
#else
#  define A20ADDR(x)                ((bx_phy_address)(x))
#endif