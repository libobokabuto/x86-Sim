#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#include "cpuid.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_SUPPORT_SVM
#include "svm.h"
#endif

#include "ia_opcodes.h"

#if BX_CPU_LEVEL >= 6

bool BX_CPU_C::xsave_x87_state_xinuse(void)
{
	return false;
}

void BX_CPU_C::xsave_x87_state(bxInstruction_c* i, bx_address offset)
{
}

void BX_CPU_C::xrstor_x87_state(bxInstruction_c* i, bx_address offset)
{
}

void BX_CPU_C::xrstor_init_x87_state(void)
{
}

bool BX_CPU_C::xsave_sse_state_xinuse(void)
{
	return false;
}

void BX_CPU_C::xsave_sse_state(bxInstruction_c* i, bx_address offset)
{
}

void BX_CPU_C::xrstor_sse_state(bxInstruction_c* i, bx_address offset)
{
}

void BX_CPU_C::xrstor_init_sse_state(void)
{
}

#if BX_SUPPORT_AVX
bool BX_CPU_C::xsave_ymm_state_xinuse(void)
{
	return false;
}

void BX_CPU_C::xsave_ymm_state(bxInstruction_c* i, bx_address offset)
{
}

void BX_CPU_C::xrstor_ymm_state(bxInstruction_c* i, bx_address offset)
{
}

void BX_CPU_C::xrstor_init_ymm_state(void)
{
}

#if BX_SUPPORT_EVEX
bool BX_CPU_C::xsave_opmask_state_xinuse(void)
{
	return false;
}

void BX_CPU_C::xsave_opmask_state(bxInstruction_c* i, bx_address offset)
{
}

void BX_CPU_C::xrstor_opmask_state(bxInstruction_c* i, bx_address offset)
{
}

void BX_CPU_C::xrstor_init_opmask_state(void)
{
}

bool BX_CPU_C::xsave_zmm_hi256_state_xinuse(void)
{
	return false;
}

void BX_CPU_C::xsave_zmm_hi256_state(bxInstruction_c* i, bx_address offset)
{
}

void BX_CPU_C::xrstor_zmm_hi256_state(bxInstruction_c* i, bx_address offset)
{
}

void BX_CPU_C::xrstor_init_zmm_hi256_state(void)
{
}

bool BX_CPU_C::xsave_hi_zmm_state_xinuse(void)
{
	return false;
}

void BX_CPU_C::xsave_hi_zmm_state(bxInstruction_c* i, bx_address offset)
{
}

void BX_CPU_C::xrstor_hi_zmm_state(bxInstruction_c* i, bx_address offset)
{
}

void BX_CPU_C::xrstor_init_hi_zmm_state(void)
{
}
#endif
#endif

#if BX_SUPPORT_PKEYS
bool BX_CPU_C::xsave_pkru_state_xinuse(void)
{
	return false;
}

void BX_CPU_C::xsave_pkru_state(bxInstruction_c* i, bx_address offset)
{
}

void BX_CPU_C::xrstor_pkru_state(bxInstruction_c* i, bx_address offset)
{
}

void BX_CPU_C::xrstor_init_pkru_state(void)
{
}
#endif

#if BX_SUPPORT_CET
bool BX_CPU_C::xsave_cet_u_state_xinuse(void)
{
	return false;
}

void BX_CPU_C::xsave_cet_u_state(bxInstruction_c* i, bx_address offset)
{
}

void BX_CPU_C::xrstor_cet_u_state(bxInstruction_c* i, bx_address offset)
{
}

void BX_CPU_C::xrstor_init_cet_u_state(void)
{
}

bool BX_CPU_C::xsave_cet_s_state_xinuse(void)
{
	return false;
}

void BX_CPU_C::xsave_cet_s_state(bxInstruction_c* i, bx_address offset)
{
}

void BX_CPU_C::xrstor_cet_s_state(bxInstruction_c* i, bx_address offset)
{
}

void BX_CPU_C::xrstor_init_cet_s_state(void)
{
}
#endif

#if BX_SUPPORT_UINTR
bool BX_CPU_C::xsave_uintr_state_xinuse(void)
{
	return false;
}

void BX_CPU_C::xsave_uintr_state(bxInstruction_c* i, bx_address offset)
{
}

void BX_CPU_C::xrstor_uintr_state(bxInstruction_c* i, bx_address offset)
{
}

void BX_CPU_C::xrstor_init_uintr_state(void)
{
}
#endif

#if BX_SUPPORT_AMX
bool BX_CPU_C::xsave_tilecfg_state_xinuse(void)
{
	return false;
}

void BX_CPU_C::xsave_tilecfg_state(bxInstruction_c* i, bx_address offset)
{
}

void BX_CPU_C::xrstor_tilecfg_state(bxInstruction_c* i, bx_address offset)
{
}

void BX_CPU_C::xrstor_init_tilecfg_state(void)
{
}

bool BX_CPU_C::xsave_tiledata_state_xinuse(void)
{
	return false;
}

void BX_CPU_C::xsave_tiledata_state(bxInstruction_c* i, bx_address offset)
{
}

void BX_CPU_C::xrstor_tiledata_state(bxInstruction_c* i, bx_address offset)
{
}

void BX_CPU_C::xrstor_init_tiledata_state(void)
{
}
#endif
#endif
