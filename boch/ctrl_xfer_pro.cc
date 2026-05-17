#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

void BX_CPP_AttrRegparmN(3)
BX_CPU_C::load_cs(bx_selector_t* selector, bx_descriptor_t* descriptor, Bit8u cpl)
{
    // Add cpl to the selector value.
    selector->value = (0xfffc & selector->value) | cpl;

    touch_segment(selector, descriptor);

#ifdef BX_SUPPORT_CS_LIMIT_DEMOTION
    // Handle special case of CS.LIMIT demotion (new descriptor limit is
    // smaller than current one)
    if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled > descriptor->u.segment.limit_scaled)
        BX_CPU_THIS_PTR iCache.flushICacheEntries();
#endif

    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector = *selector;
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache = *descriptor;
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.rpl = cpl;
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.valid = SegValidCache;

#if BX_SUPPORT_X86_64
    if (long_mode()) {
        handleCpuModeChange();
    }
#endif

    updateFetchModeMask(/* CS reloaded */);

#if BX_CPU_LEVEL >= 4
    handleAlignmentCheck(/* CPL change */);
#endif

    // Loading CS will invalidate the EIP fetch window.
    invalidate_prefetch_q();
}

void BX_CPU_C::branch_far(bx_selector_t* selector, bx_descriptor_t* descriptor, bx_address rip, unsigned cpl)
{
#if BX_SUPPORT_MONITOR_MWAIT
    BX_CPU_THIS_PTR monitor.reset_monitorx();  // reset MONITORX after every far control transfer
#endif

#if BX_SUPPORT_X86_64
    if (long_mode() && descriptor->u.segment.l) {
        if (!IsCanonical(rip)) {
            //BX_ERROR(("branch_far: canonical RIP violation"));
            exception(BX_GP_EXCEPTION, 0);
        }
    }
    else
#endif
    {
#if BX_SUPPORT_CET
        if (ShadowStackEnabled(cpl)) {
            if (GET32H(SSP) != 0) {
                //BX_ERROR(("branch_far64: 64-bit SSP when jumping to legacy mode"));
                exception(BX_GP_EXCEPTION, 0);
            }
        }
#endif

        rip &= 0xffffffff;

        /* instruction pointer must be in code segment limit else #GP(0) */
        if (rip > descriptor->u.segment.limit_scaled) {
            //BX_ERROR(("branch_far: RIP > limit"));
            exception(BX_GP_EXCEPTION, 0);
        }
    }

    /* Load CS:IP from destination pointer */
    /* Load CS-cache with new segment descriptor */
    load_cs(selector, descriptor, cpl);

    /* Change the RIP value */
    RIP = rip;
}