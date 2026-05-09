#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#include "cpuid.h"
#define LOG_THIS BX_CPU_THIS_PTR

#include "ia_opcodes.h"


#if BX_CPU_LEVEL >= 4
void BX_CPU_C::handleAlignmentCheck(void)
{
	if (CPL == 3 && BX_CPU_THIS_PTR cr0.get_AM() && BX_CPU_THIS_PTR get_AC()) {
#if BX_SUPPORT_ALIGNMENT_CHECK == 0
		BX_PANIC(("WARNING: Alignment check (#AC exception) was not compiled in !"));
#else
		BX_CPU_THIS_PTR alignment_check_mask = 0xF;
#endif
	}
#if BX_SUPPORT_ALIGNMENT_CHECK
	else {
		BX_CPU_THIS_PTR alignment_check_mask = 0;
	}
#endif
}
#endif

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BxError(bxInstruction_c* i)
{

}
void BX_CPP_AttrRegparmN(1) BX_CPU_C::BxNoFPU(bxInstruction_c* i)
{
	//456
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BxNoMMX(bxInstruction_c* i)
{
	//466
}
void BX_CPP_AttrRegparmN(1) BX_CPU_C::BxNoSSE(bxInstruction_c* i)
{
	//495
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BxNoAVX(bxInstruction_c* i)
{
	//550
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BxNoOpMask(bxInstruction_c* i)
{
	//568
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BxNoEVEX(bxInstruction_c* i)
{
	//584
}

void BX_CPU_C::handleCpuContextChange(void)
{/*
	TLB_flush();

	invalidate_prefetch_q();
	invalidate_stack_cache();

	handleInterruptMaskChange();

#if BX_CPU_LEVEL >= 4
	handleAlignmentCheck();
#endif

	handleCpuModeChange();

	handleFpuMmxModeChange();
#if BX_CPU_LEVEL >= 6
	handleSseModeChange();
#if BX_SUPPORT_AVX
	handleAvxModeChange();
#endif
#endif

*/
}
