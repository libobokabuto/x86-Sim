#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_SUPPORT_CET
void BX_CPP_AttrRegparmN(1) BX_CPU_C::shadow_stack_switch(bx_address new_SSP)
{
    SSP = new_SSP;

    if (SSP & 0x7) {
        //BX_ERROR(("shadow_stack_switch: SSP is not aligned to 8 byte boundary"));
        exception(BX_GP_EXCEPTION, 0);
    }
    if (!long64_mode() && GET32H(SSP) != 0) {
        //BX_ERROR(("shadow_stack_switch: 64-bit SSP not in 64-bit mode"));
        exception(BX_GP_EXCEPTION, 0);
    }
    if (!shadow_stack_atomic_set_busy(SSP, CPL)) {
        //BX_ERROR(("shadow_stack_switch: failure to set busy bit"));
        exception(BX_GP_EXCEPTION, 0);
    }
}

void BX_CPP_AttrRegparmN(3) BX_CPU_C::call_far_shadow_stack_push(Bit16u cs, bx_address lip, bx_address old_ssp)
{
#if BX_SUPPORT_VMX
    if (BX_CPU_THIS_PTR in_vmx_guest)
        BX_CPU_THIS_PTR vmcs.shadow_stack_prematurely_busy = true;
#endif

    if (SSP & 0x7) {
        shadow_stack_write_dword(SSP - 4, CPL, 0);
        SSP &= ~BX_CONST64(0x7);
    }

    shadow_stack_push_64(cs);
    shadow_stack_push_64(lip);
    shadow_stack_push_64(old_ssp);

#if BX_SUPPORT_VMX
    if (BX_CPU_THIS_PTR in_vmx_guest)
        BX_CPU_THIS_PTR vmcs.shadow_stack_prematurely_busy = false;
#endif
}

#endif // BX_SUPPORT_CET