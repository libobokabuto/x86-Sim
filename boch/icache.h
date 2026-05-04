#pragma once
class bxInstruction_c;

struct bxICacheEntry_c  //107ÐÐ
{
	bx_phy_address pAddr; // Physical address of the instruction
	Bit32u traceMask;

	Bit32u tlen;          // Trace length in instructions
	bxInstruction_c* i;
};

class BOCHSAPI bxICache_c { //122-199ÐÐ
public:
	BX_CPP_INLINE bxICacheEntry_c* find_entry(bx_phy_address pAddr, unsigned fetchModeMask)
	{
		return 0;
	}
};
