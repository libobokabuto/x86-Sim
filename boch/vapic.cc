#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_SUPPORT_VMX && BX_SUPPORT_X86_64

#include "memory-bochs.h"

#if BX_SUPPORT_APIC
#include "apic.h"
#endif


bool BX_CPP_AttrRegparmN(1) BX_CPU_C::is_virtual_apic_page(bx_phy_address paddr)
{
    //37
    if (BX_CPU_THIS_PTR in_vmx_guest) {
        VMCS_CACHE* vm = &BX_CPU_THIS_PTR vmcs;
        if (vm->vmexec_ctrls2.VIRTUALIZE_APIC_ACCESSES())
            if (PPFOf(paddr) == vm->apic_access_page) return true;
    }

    return false;
}
bool BX_CPP_AttrRegparmN(2) BX_CPU_C::virtual_apic_access_vmexit(unsigned offset, unsigned len)
{
    //48
    if ((offset & ~0x3) != ((offset + len - 1) & ~0x3)) {
        //BX_ERROR(("Virtual APIC access at offset 0x%08x spans 32-bit boundary !", offset));
        return true;
    }

    VMCS_CACHE* vm = &BX_CPU_THIS_PTR vmcs;

    if (is_pending(BX_EVENT_VMX_VTPR_UPDATE | BX_EVENT_VMX_VEOI_UPDATE | BX_EVENT_VMX_VIRTUAL_APIC_WRITE)) {
        if (vm->apic_access != offset) {
            //BX_ERROR(("Second APIC virtualization at offset 0x%08x (first access at offset 0x%08x)", offset, vm->apic_access));
            return true;
        }
    }

    // access is not instruction fetch because cpu::prefetch will crash them
    if (!vm->vmexec_ctrls1.TPR_SHADOW() || len > 4 || offset >= 0x400)
        return true;

    BX_CPU_THIS_PTR vmcs.apic_access = offset;
    return false;
}

void BX_CPU_C::VMX_Write_Virtual_APIC(unsigned offset, int len, Bit8u* val)
{
    bx_phy_address pAddr = BX_CPU_THIS_PTR vmcs.virtual_apic_page_addr + offset;
    // must avoid recursive call to the function when VMX APIC access page = VMX Virtual Apic Page
    BX_MEM(0)->writePhysicalPage(BX_CPU_THIS, pAddr, len, val);
    BX_NOTIFY_PHY_MEMORY_ACCESS(pAddr, len, MEMTYPE(resolve_memtype(pAddr)), BX_WRITE, BX_VMX_VAPIC_ACCESS, val);
}

bx_phy_address BX_CPU_C::VMX_Virtual_Apic_Read(bx_phy_address paddr, unsigned len, void* data)
{
    //90
    VMCS_CACHE* vm = &BX_CPU_THIS_PTR vmcs;

    //BX_ASSERT(vm->vmexec_ctrls2.VIRTUALIZE_APIC_ACCESSES());
    //BX_INFO(("Virtual Apic RD 0x" FMT_ADDRX " len = %d", paddr, len));

    Bit32u offset = PAGE_OFFSET(paddr);

    bool vmexit = virtual_apic_access_vmexit(offset, len);

    // access is not instruction fetch because cpu::prefetch will crash them
    if (!vmexit) {

        if (!vm->vmexec_ctrls2.VIRTUALIZE_APIC_REGISTERS()) {
            // if 'Virtualize Apic Registers' control is disabled allow only aligned access to VTPR
            if (offset != BX_LAPIC_TPR) vmexit = true;
        }

#if BX_SUPPORT_VMX >= 2
        switch (offset & 0x3fc) {
        case BX_LAPIC_ID:
        case BX_LAPIC_VERSION:
        case BX_LAPIC_TPR:
        case BX_LAPIC_EOI:
        case BX_LAPIC_LDR:
        case BX_LAPIC_DESTINATION_FORMAT:
        case BX_LAPIC_SPURIOUS_VECTOR:
        case BX_LAPIC_ISR1:
        case BX_LAPIC_ISR2:
        case BX_LAPIC_ISR3:
        case BX_LAPIC_ISR4:
        case BX_LAPIC_ISR5:
        case BX_LAPIC_ISR6:
        case BX_LAPIC_ISR7:
        case BX_LAPIC_ISR8:
        case BX_LAPIC_TMR1:
        case BX_LAPIC_TMR2:
        case BX_LAPIC_TMR3:
        case BX_LAPIC_TMR4:
        case BX_LAPIC_TMR5:
        case BX_LAPIC_TMR6:
        case BX_LAPIC_TMR7:
        case BX_LAPIC_TMR8:
        case BX_LAPIC_IRR1:
        case BX_LAPIC_IRR2:
        case BX_LAPIC_IRR3:
        case BX_LAPIC_IRR4:
        case BX_LAPIC_IRR5:
        case BX_LAPIC_IRR6:
        case BX_LAPIC_IRR7:
        case BX_LAPIC_IRR8:
        case BX_LAPIC_ESR:
        case BX_LAPIC_ICR_LO:
        case BX_LAPIC_ICR_HI:
        case BX_LAPIC_LVT_TIMER:
        case BX_LAPIC_LVT_THERMAL:
        case BX_LAPIC_LVT_PERFMON:
        case BX_LAPIC_LVT_LINT0:
        case BX_LAPIC_LVT_LINT1:
        case BX_LAPIC_LVT_ERROR:
        case BX_LAPIC_TIMER_INITIAL_COUNT:
        case BX_LAPIC_TIMER_DIVIDE_CFG:
            break;

        default:
            vmexit = true;
            break;
        }
#endif
    }

    if (vmexit) {
        Bit32u qualification = offset |
            ((BX_CPU_THIS_PTR in_event) ? VMX_APIC_ACCESS_DURING_EVENT_DELIVERY : VMX_APIC_READ_INSTRUCTION_EXECUTION);
        VMexit(VMX_VMEXIT_APIC_ACCESS, qualification);
    }

    // remap access to virtual apic page
    paddr = vm->virtual_apic_page_addr + offset;
    BX_NOTIFY_PHY_MEMORY_ACCESS(paddr, len, MEMTYPE(resolve_memtype(paddr)), BX_READ, BX_VMX_VAPIC_ACCESS, (Bit8u*)data);
    return paddr;
}

void BX_CPU_C::VMX_Virtual_Apic_Write(bx_phy_address paddr, unsigned len, void* data)
{  //245
    VMCS_CACHE* vm = &BX_CPU_THIS_PTR vmcs;

    //BX_ASSERT(vm->vmexec_ctrls2.VIRTUALIZE_APIC_ACCESSES());
    //BX_INFO(("Virtual Apic WR 0x" FMT_ADDRX " len = %d", paddr, len));

    Bit32u offset = PAGE_OFFSET(paddr);

    bool vmexit = virtual_apic_access_vmexit(offset, len);

    if (!vmexit) {

        if (offset == BX_LAPIC_TPR) {
            Bit8u vtpr = *((Bit8u*)data);
            VMX_Write_Virtual_APIC(BX_LAPIC_TPR, vtpr);
            signal_event(BX_EVENT_VMX_VTPR_UPDATE);
            return;
        }

#if BX_SUPPORT_VMX >= 2
        if (vm->vmexec_ctrls2.VIRTUAL_INT_DELIVERY()) {
            if (offset == BX_LAPIC_EOI) {
                signal_event(BX_EVENT_VMX_VEOI_UPDATE);
            }
        }

        bool virtualize_access = false;
        switch (offset & 0x3fc) {
        case BX_LAPIC_ID:
        case BX_LAPIC_TPR:
        case BX_LAPIC_LDR:
        case BX_LAPIC_DESTINATION_FORMAT:
        case BX_LAPIC_SPURIOUS_VECTOR:
        case BX_LAPIC_ESR:
        case BX_LAPIC_ICR_HI:
        case BX_LAPIC_LVT_TIMER:
        case BX_LAPIC_LVT_THERMAL:
        case BX_LAPIC_LVT_PERFMON:
        case BX_LAPIC_LVT_LINT0:
        case BX_LAPIC_LVT_LINT1:
        case BX_LAPIC_LVT_ERROR:
        case BX_LAPIC_TIMER_INITIAL_COUNT:
        case BX_LAPIC_TIMER_DIVIDE_CFG:
            if (vm->vmexec_ctrls2.VIRTUALIZE_APIC_REGISTERS()) {
                virtualize_access = true;
            }
            break;

        case BX_LAPIC_EOI:
        case BX_LAPIC_ICR_LO:
            if (vm->vmexec_ctrls2.VIRTUALIZE_APIC_REGISTERS() || vm->vmexec_ctrls2.VIRTUAL_INT_DELIVERY()) {
                virtualize_access = true;
            }
            break;

        default:
            break;
        }

        if (virtualize_access) {
            VMX_Write_Virtual_APIC(offset, len, (Bit8u*)data);
            signal_event(BX_EVENT_VMX_VIRTUAL_APIC_WRITE);
            return;
        }
#endif
    }

    Bit32u qualification = offset |
        ((BX_CPU_THIS_PTR in_event) ? VMX_APIC_ACCESS_DURING_EVENT_DELIVERY : VMX_APIC_WRITE_INSTRUCTION_EXECUTION);
    VMexit(VMX_VMEXIT_APIC_ACCESS, qualification);
}

#endif