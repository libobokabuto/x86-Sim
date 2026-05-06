#pragma once
BOCHSAPI extern class bx_pc_system_c bx_pc_system; //31

class BOCHSAPI bx_pc_system_c {
public:
	bool enable_a20;
	bx_phy_address a20_mask;

	void set_enable_a20(bool value);
	bool get_enable_a20(void);
	int Reset(unsigned type);
};
#define BX_SET_ENABLE_A20(enabled)  bx_pc_system.set_enable_a20(enabled)
#define BX_GET_ENABLE_A20()         bx_pc_system.get_enable_a20()
#if BX_SUPPORT_A20
#  define A20ADDR(x)                ((bx_phy_address)(x) & bx_pc_system.a20_mask)
#else
#  define A20ADDR(x)                ((bx_phy_address)(x))
#endif