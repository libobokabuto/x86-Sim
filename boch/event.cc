#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_SUPPORT_SVM
#include "svm.h"
#endif


#include "iodev.h"

#include "debug.h"

void BX_CPU_C::inhibit_interrupts(unsigned mask)
{
    // Loading of SS disables interrupts until the next instruction completes
    // but only under assumption that previous instruction didn't load SS also.
    if (mask != BX_INHIBIT_INTERRUPTS_BY_MOVSS || !interrupts_inhibited(BX_INHIBIT_INTERRUPTS_BY_MOVSS)) {
        //BX_DEBUG(("inhibit interrupts mask = %d", mask));
        BX_CPU_THIS_PTR inhibit_mask = mask;
        BX_CPU_THIS_PTR inhibit_icount = get_icount() + 1; // inhibit for next instruction
    }
}

bool BX_CPU_C::interrupts_inhibited(unsigned mask)
{
    //450
    return (get_icount() <= BX_CPU_THIS_PTR inhibit_icount) && (BX_CPU_THIS_PTR inhibit_mask & mask) == mask;
}

void BX_CPU_C::deliver_SIPI(unsigned vector)
{  //455
    if (BX_CPU_THIS_PTR activity_state == BX_ACTIVITY_STATE_WAIT_FOR_SIPI) {
#if BX_SUPPORT_VMX
        if (BX_CPU_THIS_PTR in_vmx_guest)
            VMexit(VMX_VMEXIT_SIPI, vector);
#endif
        BX_CPU_THIS_PTR activity_state = BX_ACTIVITY_STATE_ACTIVE;
        RIP = 0;
        load_seg_reg(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS], vector * 0x100);
        unmask_event(BX_EVENT_INIT | BX_EVENT_SMI | BX_EVENT_NMI);
        //BX_INFO(("CPU %d started up at %04X:%08X by APIC",
            //BX_CPU_THIS_PTR bx_cpuid, vector * 0x100, EIP));
    }
    else {
        //BX_INFO(("CPU %d started up by APIC, but was not halted at that time", BX_CPU_THIS_PTR bx_cpuid));
    }
}


void BX_CPU_C::deliver_INIT(void)
{  //473
    if (!is_masked_event(BX_EVENT_INIT)) {
        signal_event(BX_EVENT_INIT);
    }
}

void BX_CPU_C::deliver_NMI(void)
{  //480
    signal_event(BX_EVENT_NMI);
}

void BX_CPU_C::deliver_SMI(void)
{
    signal_event(BX_EVENT_SMI);
}

