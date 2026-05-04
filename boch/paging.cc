#include "bochs.h"
#include "cpu.h"

bx_phy_address BX_CPU_C::translate_linear(bx_TLB_entry* tlbEntry, bx_address laddr, unsigned user, unsigned rw)
{
	//1257ÐÐ
	return 0;
}

bx_hostpageaddr_t BX_CPU_C::getHostMemAddr(bx_phy_address paddr, unsigned rw)
{
	//2491ÐÐ
	return 0;
}