#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_SUPPORT_SVM
#include "svm.h"
#endif
void BX_CPU_C::handleInterruptMaskChange(void)
{
    if (BX_CPU_THIS_PTR get_IF()) {
        // EFLAGS.IF was set, unmask all affected events
        unmask_event(BX_EVENT_VMX_INTERRUPT_WINDOW_EXITING |
            BX_EVENT_PENDING_INTR |
            BX_EVENT_PENDING_LAPIC_INTR |
            BX_EVENT_PENDING_VMX_VIRTUAL_INTR);

#if BX_SUPPORT_SVM
        if (BX_CPU_THIS_PTR in_svm_guest) {
            if ((SVM_V_INTR_PRIO > SVM_V_TPR) || SVM_V_IGNORE_TPR)
                unmask_event(BX_EVENT_SVM_VIRQ_PENDING);
        }
#endif

#if BX_SUPPORT_UINTR
        if (!uintr_masked()) unmask_event(BX_EVENT_PENDING_UINTR);
#endif

        return;
    }

    // EFLAGS.IF was cleared, some events like INTR would be masked

#if BX_SUPPORT_VMX
    if (BX_CPU_THIS_PTR in_vmx_guest && BX_CPU_THIS_PTR vmcs.pin_vmexec_ctrls.EXTERNAL_INTERRUPT_VMEXIT()) {
        // if 'External-interrupt exiting' control is set, the value of EFLAGS.IF
        // doesn't affect interrupt blocking
        mask_event(BX_EVENT_VMX_INTERRUPT_WINDOW_EXITING | BX_EVENT_PENDING_VMX_VIRTUAL_INTR);
        unmask_event(BX_EVENT_PENDING_INTR | BX_EVENT_PENDING_LAPIC_INTR);
#if BX_SUPPORT_UINTR
        if (!uintr_masked()) unmask_event(BX_EVENT_PENDING_UINTR);
#endif
        return;
    }
#endif

#if BX_SUPPORT_SVM
    if (BX_CPU_THIS_PTR in_svm_guest && SVM_V_INTR_MASKING) {
        if (!SVM_HOST_IF)
            mask_event(BX_EVENT_PENDING_INTR | BX_EVENT_PENDING_LAPIC_INTR | BX_EVENT_PENDING_UINTR);

        mask_event(BX_EVENT_SVM_VIRQ_PENDING);
    }
    else
#endif
    {
        mask_event(BX_EVENT_VMX_INTERRUPT_WINDOW_EXITING |
            BX_EVENT_PENDING_INTR |
            BX_EVENT_PENDING_LAPIC_INTR |
            BX_EVENT_PENDING_UINTR |
            BX_EVENT_PENDING_VMX_VIRTUAL_INTR |
            BX_EVENT_SVM_VIRQ_PENDING);
    }
}