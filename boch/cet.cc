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

bool is_invalid_cet_control(bx_address val)
{
    if ((val & (BX_CET_SUPPRESS_INDIRECT_BRANCH_TRACKING | BX_CET_WAIT_FOR_ENBRANCH)) ==
        (BX_CET_SUPPRESS_INDIRECT_BRANCH_TRACKING | BX_CET_WAIT_FOR_ENBRANCH)) return true;

    if (val & 0x3c0) return true; // reserved bits check
    return false;
}

bool BX_CPP_AttrRegparmN(1) BX_CPU_C::ShadowStackEnabled(unsigned cpl)
{
    return BX_CPU_THIS_PTR cr4.get_CET() && protected_mode() &&
        BX_CPU_THIS_PTR msr.ia32_cet_control[cpl == 3] & BX_CET_SHADOW_STACK_ENABLED;
}

bool BX_CPP_AttrRegparmN(1) BX_CPU_C::ShadowStackWriteEnabled(unsigned cpl)
{
    return BX_CPU_THIS_PTR cr4.get_CET() && protected_mode() &&
        (BX_CPU_THIS_PTR msr.ia32_cet_control[cpl == 3] & (BX_CET_SHADOW_STACK_ENABLED | BX_CET_SHADOW_STACK_WRITE_ENABLED)) == (BX_CET_SHADOW_STACK_ENABLED | BX_CET_SHADOW_STACK_WRITE_ENABLED);
}

bool BX_CPP_AttrRegparmN(1) BX_CPU_C::EndbranchEnabled(unsigned cpl)
{
    return BX_CPU_THIS_PTR cr4.get_CET() && protected_mode() &&
        BX_CPU_THIS_PTR msr.ia32_cet_control[cpl == 3] & BX_CET_ENDBRANCH_ENABLED;
}

bool BX_CPP_AttrRegparmN(1) BX_CPU_C::EndbranchEnabledAndNotSuppressed(unsigned cpl)
{
    return BX_CPU_THIS_PTR cr4.get_CET() && protected_mode() &&
        (BX_CPU_THIS_PTR msr.ia32_cet_control[cpl == 3] & (BX_CET_ENDBRANCH_ENABLED | BX_CET_SUPPRESS_INDIRECT_BRANCH_TRACKING)) == BX_CET_ENDBRANCH_ENABLED;
}


void BX_CPP_AttrRegparmN(1) BX_CPU_C::track_indirect(unsigned cpl)
{
    if (EndbranchEnabled(cpl)) {
        BX_CPU_THIS_PTR msr.ia32_cet_control[cpl == 3] |= BX_CET_WAIT_FOR_ENBRANCH;
        BX_CPU_THIS_PTR msr.ia32_cet_control[cpl == 3] &= ~BX_CET_SUPPRESS_INDIRECT_BRANCH_TRACKING;
    }
}

void BX_CPP_AttrRegparmN(2) BX_CPU_C::track_indirect_if_not_suppressed(bxInstruction_c* i, unsigned cpl)
{
    if (EndbranchEnabledAndNotSuppressed(cpl)) {
        if (i->segOverrideCet() == BX_SEG_REG_DS && (BX_CPU_THIS_PTR msr.ia32_cet_control[cpl == 3] & BX_CET_ENABLE_NO_TRACK_INDIRECT_BRANCH_PREFIX) != 0)
            return;

        BX_CPU_THIS_PTR msr.ia32_cet_control[cpl == 3] |= BX_CET_WAIT_FOR_ENBRANCH;
    }
}

void BX_CPP_AttrRegparmN(2) BX_CPU_C::reset_endbranch_tracker(unsigned cpl, bool suppress)
{
    BX_CPU_THIS_PTR msr.ia32_cet_control[cpl == 3] &= ~(BX_CET_WAIT_FOR_ENBRANCH | BX_CET_SUPPRESS_INDIRECT_BRANCH_TRACKING);
    if (suppress && !(BX_CPU_THIS_PTR msr.ia32_cet_control[cpl == 3] & BX_CET_SUPPRESS_DIS))
        BX_CPU_THIS_PTR msr.ia32_cet_control[cpl == 3] |= BX_CET_SUPPRESS_INDIRECT_BRANCH_TRACKING;
}

bool BX_CPP_AttrRegparmN(1) BX_CPU_C::LegacyEndbranchTreatment(unsigned cpl)
{
    if (BX_CPU_THIS_PTR msr.ia32_cet_control[cpl == 3] & BX_CET_LEGACY_INDIRECT_BRANCH_TREATMENT)
    {
        bx_address lip = get_laddr(BX_SEG_REG_CS, RIP);
        bx_address bitmap_addr = LPFOf(BX_CPU_THIS_PTR msr.ia32_cet_control[cpl == 3]) + ((lip & BX_CONST64(0xFFFFFFFFFFFF)) >> 15);
        unsigned bitmap_index = (lip >> 12) & 0x7;
        Bit8u bitmap = system_read_byte(bitmap_addr);
        if ((bitmap & (1 << bitmap_index)) != 0) {
            reset_endbranch_tracker(cpl, true);
            return false;
        }
    }
    return true;
}


#endif //最后一行