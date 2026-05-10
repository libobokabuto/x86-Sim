#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR
#if BX_SUPPORT_CET
const Bit64u BX_CET_SHADOW_STACK_ENABLED = (1 << 0);
const Bit64u BX_CET_SHADOW_STACK_WRITE_ENABLED = (1 << 1);
const Bit64u BX_CET_ENDBRANCH_ENABLED = (1 << 2);
const Bit64u BX_CET_LEGACY_INDIRECT_BRANCH_TREATMENT = (1 << 3);
const Bit64u BX_CET_ENABLE_NO_TRACK_INDIRECT_BRANCH_PREFIX = (1 << 4);
const Bit64u BX_CET_SUPPRESS_DIS = (1 << 5);
const Bit64u BX_CET_SUPPRESS_INDIRECT_BRANCH_TRACKING = (1 << 10);
const Bit64u BX_CET_WAIT_FOR_ENBRANCH = (1 << 11);
bool BX_CPP_AttrRegparmN(1) BX_CPU_C::ShadowStackEnabled(unsigned cpl)
{
    return BX_CPU_THIS_PTR cr4.get_CET() && protected_mode() &&
        BX_CPU_THIS_PTR msr.ia32_cet_control[cpl == 3] & BX_CET_SHADOW_STACK_ENABLED;
}
#endif