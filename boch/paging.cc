#include "bochs.h"
#include "cpu.h"
#include "memory-bochs.h"
#include "pc_system.h"
bx_phy_address BX_CPU_C::translate_linear(bx_TLB_entry* tlbEntry, bx_address laddr, unsigned user, unsigned rw)
{
	UNUSED(tlbEntry);
	UNUSED(user);
	UNUSED(rw);

#if BX_SUPPORT_X86_64
	if (!long_mode()) laddr &= 0xffffffff;
#endif

	// 当前先只补“未开启分页”的取指路径，足够跑第一条 BIOS 指令
	bx_phy_address paddress = (bx_phy_address)laddr;
	paddress = A20ADDR(paddress);

	return paddress;
}


bx_hostpageaddr_t BX_CPU_C::getHostMemAddr(bx_phy_address paddr, unsigned rw)
{
	//2491行
	return (bx_hostpageaddr_t)BX_MEM(0)->getHostMemAddr(BX_CPU_THIS, paddr, rw);
}