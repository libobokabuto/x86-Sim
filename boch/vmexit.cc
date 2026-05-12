#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#include "pc_system.h"

#if BX_SUPPORT_VMX

#include "ia_opcodes.h"

void BX_CPU_C::VMexit_Event(unsigned type, unsigned vector, Bit16u errcode, bool errcode_valid, Bit64u qualification)
{ //211
    if (!BX_CPU_THIS_PTR in_vmx_guest) return;

    VMCS_CACHE* vm = &BX_CPU_THIS_PTR vmcs;
    bool vmexit = false;
    VMX_vmexit_reason reason = VMX_VMEXIT_EXCEPTION_NMI;

    switch (type) {
    case BX_EXTERNAL_INTERRUPT:
        reason = VMX_VMEXIT_EXTERNAL_INTERRUPT;
        if (vm->pin_vmexec_ctrls.EXTERNAL_INTERRUPT_VMEXIT())
            vmexit = true;
        break;

    case BX_NMI:
        if (vm->pin_vmexec_ctrls.NMI_EXITING())
            vmexit = true;
        break;

    case BX_PRIVILEGED_SOFTWARE_INTERRUPT:
    case BX_SOFTWARE_EXCEPTION:
    case BX_HARDWARE_EXCEPTION:
        //BX_ASSERT(vector < BX_CPU_HANDLED_EXCEPTIONS);
        if (vector == BX_PF_EXCEPTION) {
            // page faults are specially treated
            bool err_match = ((errcode & vm->vm_pf_mask) == vm->vm_pf_match);
            bool bitmap = (vm->vm_exceptions_bitmap >> BX_PF_EXCEPTION) & 1;
            vmexit = (err_match == bitmap);
        }
        else {
            vmexit = (vm->vm_exceptions_bitmap >> vector) & 1;
        }
        break;

    case BX_SOFTWARE_INTERRUPT:
        break; // no VMEXIT on software interrupt

    default:
        //BX_ERROR(("VMexit_Event: unknown event type %d", type));
        break;
    }

    // ----------------------------------------------------
    //              VMExit interruption info
    // ----------------------------------------------------
    // [07:00] | Interrupt/Exception vector
    // [10:08] | Interrupt/Exception type
    // [11:11] | error code pushed to the stack
    // [12:12] | NMI unblocking due to IRET
    // [30:13] | reserved
    // [31:31] | interruption info valid
    //

    if (!vmexit) {
        // record IDT vectoring information
        vm->idt_vector_error_code = errcode;
        vm->idt_vector_info = vector | (type << 8);
        if (errcode_valid)
            vm->idt_vector_info |= (1 << 11); // error code delivered

        BX_CPU_THIS_PTR nmi_unblocking_iret = false;
        return;
    }

    //BX_DEBUG(("VMEXIT: event vector 0x%02x type %d error code=0x%04x", vector, type, errcode));

    // VMEXIT is not considered to occur during event delivery if it results
    // in a double fault exception that causes VMEXIT directly
    if (vector == BX_DF_EXCEPTION)
        BX_CPU_THIS_PTR in_event = false; // clear in_event indication on #DF

    if (vector == BX_DB_EXCEPTION) {
        // qualification for debug exceptions similar to debug_trap field
        if (type == BX_PRIVILEGED_SOFTWARE_INTERRUPT)
            qualification = BX_CPU_THIS_PTR debug_trap & 0xf;
        else
            qualification = BX_CPU_THIS_PTR debug_trap & 0x0000600f;
        BX_CPU_THIS_PTR debug_trap = 0;
    }

    // interruption info:
    // -----------------
    // [7 : 0] vector
    // [10: 8] interruption type
    // [11:11] error code delivered
    // [12:12] NMI unblocking due to IRET
    // [30:13] reserved
    // [31:31] valid

    Bit32u interruption_info = vector | (type << 8);
    if (errcode_valid)
        interruption_info |= (1 << 11); // error code delivered
    interruption_info |= (1 << 31); // valid

    if (BX_CPU_THIS_PTR nmi_unblocking_iret)
        interruption_info |= (1 << 12);

    VMwrite32(VMCS_32BIT_VMEXIT_INTERRUPTION_INFO, interruption_info);
    VMwrite32(VMCS_32BIT_VMEXIT_INTERRUPTION_ERR_CODE, errcode);

    VMexit(reason, qualification);
}

#if BX_SUPPORT_VMX >= 2  //686
void BX_CPU_C::Virtualization_Exception(Bit64u qualification, Bit64u guest_physical, Bit64u guest_linear)
{
    //BX_ASSERT(BX_CPU_THIS_PTR in_vmx_guest);

    // A convertible EPT violation causes a virtualization exception if the following all hold:
    //  - CR0.PE is set
    //  - the logical processor is not in the process of delivering an event through the IDT
    //  - the 32 bits at offset 4 in the virtualization-exception information area are all 0
    //  - the EPT violation cause a shadow stack to become prematurely busy

    if (!BX_CPU_THIS_PTR cr0.get_PE() || BX_CPU_THIS_PTR in_event) return;

    VMCS_CACHE* vm = &BX_CPU_THIS_PTR vmcs;

#if BX_SUPPORT_CET
    if (vm->shadow_stack_prematurely_busy) return;
#endif

    BxMemtype ve_info_memtype = BX_MEMTYPE_INVALID;
#if BX_SUPPORT_MEMTYPE
    ve_info_memtype = resolve_memtype(vm->ve_info_addr);
#endif
    Bit32u magic = read_physical_dword(vm->ve_info_addr + 4, MEMTYPE(ve_info_memtype), BX_ACCESS_REASON_NOT_SPECIFIED);
    if (magic != 0) return;

    struct ve_info {
        Bit32u reason; // always VMX_VMEXIT_EPT_VIOLATION
        Bit32u magic;
        Bit64u qualification;
        Bit64u guest_linear_addr;
        Bit64u guest_physical_addr;
        Bit16u eptp_index;
    } ve_info = { VMX_VMEXIT_EPT_VIOLATION, 0xffffffff, qualification, guest_linear, guest_physical, vm->eptp_index };

    write_physical_dword(vm->ve_info_addr, ve_info.reason, MEMTYPE(ve_info_memtype), BX_ACCESS_REASON_NOT_SPECIFIED);
    write_physical_dword(vm->ve_info_addr + 4, ve_info.magic, MEMTYPE(ve_info_memtype), BX_ACCESS_REASON_NOT_SPECIFIED);
    write_physical_qword(vm->ve_info_addr + 8, ve_info.qualification, MEMTYPE(ve_info_memtype), BX_ACCESS_REASON_NOT_SPECIFIED);
    write_physical_qword(vm->ve_info_addr + 16, ve_info.guest_linear_addr, MEMTYPE(ve_info_memtype), BX_ACCESS_REASON_NOT_SPECIFIED);
    write_physical_qword(vm->ve_info_addr + 24, ve_info.guest_physical_addr, MEMTYPE(ve_info_memtype), BX_ACCESS_REASON_NOT_SPECIFIED);
    write_physical_qword(vm->ve_info_addr + 32, ve_info.eptp_index, MEMTYPE(ve_info_memtype), BX_ACCESS_REASON_NOT_SPECIFIED);

    exception(BX_VE_EXCEPTION, 0);
}

void BX_CPU_C::vmx_page_modification_logging(Bit64u guest_laddr, Bit64u guest_paddr, unsigned dirty_update)
{
    VMCS_CACHE* vm = &BX_CPU_THIS_PTR vmcs;

    if (vm->pml_index >= 512) {
        Bit32u vmexit_qualification = 0;
        if (BX_CPU_THIS_PTR nmi_unblocking_iret)
            vmexit_qualification |= (1 << 12);

        if (vm->vmexit_ctrls2.SHADOW_STACK_PREMATURELY_BUSY_CTRL()) {
            VMwrite_natural(VMCS_GUEST_LINEAR_ADDR, guest_laddr);
        }
        VMexit(VMX_VMEXIT_PML_LOGFULL, vmexit_qualification);
    }

    if (dirty_update) {
        Bit64u pAddr = vm->pml_address + 8 * vm->pml_index;
        write_physical_qword(pAddr, LPFOf(guest_paddr), MEMTYPE(resolve_memtype(pAddr)), BX_VMX_PML_WRITE);
        vm->pml_index--;
    }
}
#endif

#endif // BX_SUPPORT_VMX