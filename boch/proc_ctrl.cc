#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#include "cpuid.h"
#define LOG_THIS BX_CPU_THIS_PTR

#include "ia_opcodes.h"

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