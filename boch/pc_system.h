#pragma once

class BOCHSAPI bx_pc_system_c;

BOCHSAPI extern class bx_pc_system_c bx_pc_system; //31

class BOCHSAPI bx_pc_system_c { //37
private:
	struct {
		Bit32u     currCountdown;
		Bit32u     currCountdownPeriod;
		Bit64u     ticksTotal;
	};
public:
	bool enable_a20;
	bx_phy_address a20_mask;

	void set_enable_a20(bool value);
	bool get_enable_a20(void);
	int Reset(unsigned type);
	static BX_CPP_INLINE Bit64u time_ticks() {
		return bx_pc_system.ticksTotal +
			Bit64u(bx_pc_system.currCountdownPeriod - bx_pc_system.currCountdown);
	}
	void    outp(Bit16u addr, Bit32u value, unsigned io_len) BX_CPP_AttrRegparmN(3);

}; //189
#define BX_SET_ENABLE_A20(enabled)  bx_pc_system.set_enable_a20(enabled)
#define BX_GET_ENABLE_A20()         bx_pc_system.get_enable_a20()

#if BX_SUPPORT_A20
#  define A20ADDR(x)                ((bx_phy_address)(x) & bx_pc_system.a20_mask)
#else
#  define A20ADDR(x)                ((bx_phy_address)(x))
#endif