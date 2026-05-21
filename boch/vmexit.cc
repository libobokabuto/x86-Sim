#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#include "pc_system.h"

#if BX_SUPPORT_VMX

#include "ia_opcodes.h"

Bit32u gen_instruction_info(bxInstruction_c* i, Bit32u reason, bool rw_form)
{//36
    Bit32u instr_info = 0;

    switch (reason) {
    case VMX_VMEXIT_VMREAD:
    case VMX_VMEXIT_VMWRITE:
#if BX_SUPPORT_VMX >= 2
    case VMX_VMEXIT_GDTR_IDTR_ACCESS:
    case VMX_VMEXIT_LDTR_TR_ACCESS:
    case VMX_VMEXIT_INVEPT:
    case VMX_VMEXIT_INVVPID:
    case VMX_VMEXIT_INVPCID:
#endif
        if (rw_form == BX_WRITE)
            instr_info |= i->dst() << 28;
        else
            instr_info |= i->src() << 28;
        break;

    case VMX_VMEXIT_RDRAND:
    case VMX_VMEXIT_RDSEED:
    case VMX_VMEXIT_UMWAIT:
    case VMX_VMEXIT_TPAUSE:
        // bits 12:11 hold operand size
        if (i->os64L())
            instr_info |= 1 << 12;
        else if (i->as32L())
            instr_info |= 1 << 11;
        break;

    default:
        break;
    }

    // --------------------------------------
    //  instruction information field format
    // --------------------------------------
    //
    // [01:00] | Memory operand scale field (encoded)
    // [02:02] | Undefined
    // [06:03] | Reg1, undefined when memory operand
    // [09:07] | Memory operand address size
    // [10:10] | Memory/Register format (0 - mem, 1 - reg)
    // [14:11] | Reserved
    // [17:15] | Memory operand segment register field
    // [21:18] | Memory operand index field
    // [22:22] | Memory operand index field invalid
    // [26:23] | Memory operand base field
    // [27:27] | Memory operand base field invalid
    // [31:28] | Reg2, if exists
    //
    if (i->modC0()) {
        // reg/reg format
        instr_info |= (1 << 10);
        if (rw_form == BX_WRITE)
            instr_info |= i->src() << 3;
        else
            instr_info |= i->dst() << 3;
    }
    else {
        // memory format
        if (i->as64L())
            instr_info |= 1 << 8;
        else if (i->as32L())
            instr_info |= 1 << 7;

        instr_info |= i->seg() << 15;

        // index field is always initialized because of gather but not always valid
        if (i->sibIndex() != BX_NIL_REGISTER && i->sibIndex() != 4)
            instr_info |= i->sibScale() | (i->sibIndex() << 18);
        else
            instr_info |= 1 << 22; // index invalid

        if (i->sibBase() != BX_NIL_REGISTER)
            instr_info |= i->sibBase() << 23;
        else
            instr_info |= 1 << 27; // base invalid
    }

    return instr_info;
}

void BX_CPP_AttrRegparmN(3) BX_CPU_C::VMexit_Instruction(bxInstruction_c* i, Bit32u reason, bool rw_form)
{//120
    Bit64u qualification = 0;
    Bit32u instr_info = 0;

    switch (reason) {
    case VMX_VMEXIT_VMREAD:
    case VMX_VMEXIT_VMWRITE:
    case VMX_VMEXIT_VMPTRLD:
    case VMX_VMEXIT_VMPTRST:
    case VMX_VMEXIT_VMCLEAR:
    case VMX_VMEXIT_VMXON:
#if BX_SUPPORT_VMX >= 2
    case VMX_VMEXIT_GDTR_IDTR_ACCESS:
    case VMX_VMEXIT_LDTR_TR_ACCESS:
    case VMX_VMEXIT_INVEPT:
    case VMX_VMEXIT_INVVPID:
    case VMX_VMEXIT_INVPCID:
    case VMX_VMEXIT_XSAVES:
    case VMX_VMEXIT_XRSTORS:
#endif
#if BX_SUPPORT_X86_64
        if (long64_mode()) {
            qualification = (Bit64u)i->displ32s();
            if (i->sibBase() == BX_64BIT_REG_RIP)
                qualification += RIP;
        }
        else
#endif
        {
            qualification = (Bit64u)((Bit32u)i->displ32s());
            qualification &= i->asize_mask();
        }
        // fall through

    case VMX_VMEXIT_RDRAND:
    case VMX_VMEXIT_RDSEED:
    case VMX_VMEXIT_UMWAIT:
    case VMX_VMEXIT_TPAUSE:
        instr_info = gen_instruction_info(i, reason, rw_form);
        VMwrite32(VMCS_32BIT_VMEXIT_INSTRUCTION_INFO, instr_info);
        break;

    default:
        //BX_PANIC(("VMexit_Instruction reason %d", reason));
        break;
    }

    VMexit(reason, qualification);
}

void BX_CPU_C::VMexit_ExtInterrupt(void)
{
    //BX_ASSERT(BX_CPU_THIS_PTR in_vmx_guest);

    VMCS_CACHE* vm = &BX_CPU_THIS_PTR vmcs;

    if (vm->pin_vmexec_ctrls.EXTERNAL_INTERRUPT_VMEXIT()) {
        if (!vm->vmexit_ctrls1.INTA_ON_VMEXIT()) {
            // interrupt wasn't acknowledged and still pending, interruption info is invalid
            VMwrite32(VMCS_32BIT_VMEXIT_INTERRUPTION_INFO, 0);
            VMexit(VMX_VMEXIT_EXTERNAL_INTERRUPT, 0);
        }
    }
}

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

enum {
    VMX_VMEXIT_IO_PORTIN = (1 << 3),
    VMX_VMEXIT_IO_INSTR_STRING = (1 << 4),
    VMX_VMEXIT_IO_INSTR_REP = (1 << 5),
    VMX_VMEXIT_IO_INSTR_IMM = (1 << 6)
};

void BX_CPU_C::VMexit_TripleFault(void)
{
    if (!BX_CPU_THIS_PTR in_vmx_guest) return;

    // VMEXIT is not considered to occur during event delivery if it results
    // in a triple fault exception (that causes VMEXIT directly)
    BX_CPU_THIS_PTR in_event = false;

    VMexit(VMX_VMEXIT_TRIPLE_FAULT, 0);
}

void BX_CPP_AttrRegparmN(2) BX_CPU_C::VMexit_TaskSwitch(Bit16u tss_selector, unsigned source)
{
    //BX_ASSERT(BX_CPU_THIS_PTR in_vmx_guest);

    VMexit(VMX_VMEXIT_TASK_SWITCH, tss_selector | (source << 30));
}

const Bit32u BX_VMX_LO_MSR_START = 0x00000000;
const Bit32u BX_VMX_LO_MSR_END = 0x00001FFF;
const Bit32u BX_VMX_HI_MSR_START = 0xC0000000;
const Bit32u BX_VMX_HI_MSR_END = 0xC0001FFF;

void BX_CPP_AttrRegparmN(3) BX_CPU_C::VMexit_IO(bxInstruction_c* i, unsigned port, unsigned len)
{ //385
    //BX_ASSERT(BX_CPU_THIS_PTR in_vmx_guest);
    //BX_ASSERT(port <= 0xFFFF);

    bool vmexit = false;

    VMCS_CACHE* vm = &BX_CPU_THIS_PTR vmcs;

    if (vm->vmexec_ctrls1.IO_BITMAPS()) {
        // always VMEXIT on port "wrap around" case
        if ((port + len) > 0x10000) vmexit = true;
        else {
            Bit8u bitmap[2];
            bx_phy_address pAddr;

            if ((port & 0x7fff) + len > 0x8000) {
                // special case - the IO access split cross both I/O bitmaps
                pAddr = vm->io_bitmap_addr[0] + 0xfff;
                bitmap[0] = read_physical_byte(pAddr, MEMTYPE(resolve_memtype(pAddr)), BX_IO_BITMAP_ACCESS);

                pAddr = vm->io_bitmap_addr[1];
                bitmap[1] = read_physical_byte(pAddr, MEMTYPE(resolve_memtype(pAddr)), BX_IO_BITMAP_ACCESS);
            }
            else {
                // access_read_physical cannot read 2 bytes cross 4K boundary :(
                pAddr = vm->io_bitmap_addr[(port >> 15) & 1] + ((port & 0x7fff) / 8);
                bitmap[0] = read_physical_byte(pAddr, MEMTYPE(resolve_memtype(pAddr)), BX_IO_BITMAP_ACCESS);

                pAddr++;
                bitmap[1] = read_physical_byte(pAddr, MEMTYPE(resolve_memtype(pAddr)), BX_IO_BITMAP_ACCESS);
            }

            Bit16u combined_bitmap = bitmap[1];
            combined_bitmap = (combined_bitmap << 8) | bitmap[0];

            unsigned mask = ((1 << len) - 1) << (port & 7);
            if (combined_bitmap & mask) vmexit = true;
        }
    }
    else if (vm->vmexec_ctrls1.IO_VMEXIT()) vmexit = true;

    if (vmexit) {
        //BX_DEBUG(("VMEXIT: I/O port 0x%04x", port));

        Bit32u qualification = 0;

        switch (i->getIaOpcode()) {
        case BX_IA_IN_ALIb:
        case BX_IA_IN_AXIb:
        case BX_IA_IN_EAXIb:
            qualification = VMX_VMEXIT_IO_PORTIN | VMX_VMEXIT_IO_INSTR_IMM;
            break;

        case BX_IA_OUT_IbAL:
        case BX_IA_OUT_IbAX:
        case BX_IA_OUT_IbEAX:
            qualification = VMX_VMEXIT_IO_INSTR_IMM;
            break;

        case BX_IA_IN_ALDX:
        case BX_IA_IN_AXDX:
        case BX_IA_IN_EAXDX:
            qualification = VMX_VMEXIT_IO_PORTIN; // no immediate
            break;

        case BX_IA_OUT_DXAL:
        case BX_IA_OUT_DXAX:
        case BX_IA_OUT_DXEAX:
            qualification = 0; // PORTOUT, no immediate
            break;

        case BX_IA_REP_INSB_YbDX:
        case BX_IA_REP_INSW_YwDX:
        case BX_IA_REP_INSD_YdDX:
            qualification = VMX_VMEXIT_IO_PORTIN | VMX_VMEXIT_IO_INSTR_STRING;
            if (i->repUsedL())
                qualification |= VMX_VMEXIT_IO_INSTR_REP;
            break;

        case BX_IA_REP_OUTSB_DXXb:
        case BX_IA_REP_OUTSW_DXXw:
        case BX_IA_REP_OUTSD_DXXd:
            qualification = VMX_VMEXIT_IO_INSTR_STRING; // PORTOUT
            if (i->repUsedL())
                qualification |= VMX_VMEXIT_IO_INSTR_REP;
            break;

        default:
            //BX_PANIC(("VMexit_IO: I/O instruction %s unknown", i->getIaOpcodeNameShort()));
            break;
        }

        if (qualification & VMX_VMEXIT_IO_INSTR_STRING) {
            bx_address asize_mask = (bx_address)i->asize_mask(), laddr;

            if (qualification & VMX_VMEXIT_IO_PORTIN)
                laddr = get_laddr(BX_SEG_REG_ES, RDI & asize_mask);
            else  // PORTOUT
                laddr = get_laddr(i->seg(), RSI & asize_mask);

            VMwrite_natural(VMCS_GUEST_LINEAR_ADDR, laddr);

            Bit32u instruction_info = i->seg() << 15;
            if (i->as64L())
                instruction_info |= (1 << 8);
            else if (i->as32L())
                instruction_info |= (1 << 7);

            VMwrite32(VMCS_32BIT_VMEXIT_INSTRUCTION_INFO, instruction_info);
        }

        VMexit(VMX_VMEXIT_IO_INSTRUCTION, qualification | (len - 1) | (port << 16));
    }
}

bx_address BX_CPP_AttrRegparmN(2) BX_CPU_C::VMexit_CR0_Write(bxInstruction_c* i, bx_address val)
{//565
    //BX_ASSERT(BX_CPU_THIS_PTR in_vmx_guest);

    VMCS_CACHE* vm = &BX_CPU_THIS_PTR vmcs;

    if ((vm->vm_cr0_mask & vm->vm_cr0_read_shadow) != (vm->vm_cr0_mask & val))
    {
        //BX_DEBUG(("VMEXIT: CR0 write"));
        Bit64u qualification = i->src() << 8;
        VMexit(VMX_VMEXIT_CR_ACCESS, qualification);
    }

    // keep untouched all the bits set in CR0 mask
    return (BX_CPU_THIS_PTR cr0.get32() & vm->vm_cr0_mask) | (val & ~vm->vm_cr0_mask);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::VMexit_CR8_Read(bxInstruction_c* i)
{
    //BX_ASSERT(BX_CPU_THIS_PTR in_vmx_guest);

    if (BX_CPU_THIS_PTR vmcs.vmexec_ctrls1.CR8_READ_VMEXIT()) {
        //BX_DEBUG(("VMEXIT: CR8 read"));
        Bit64u qualification = 8 | (VMX_VMEXIT_CR_ACCESS_CR_READ << 4) | (i->dst() << 8);
        VMexit(VMX_VMEXIT_CR_ACCESS, qualification);
    }
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::VMexit_CR8_Write(bxInstruction_c* i)
{//638
    //BX_ASSERT(BX_CPU_THIS_PTR in_vmx_guest);

    if (BX_CPU_THIS_PTR vmcs.vmexec_ctrls1.CR8_WRITE_VMEXIT()) {
        //BX_DEBUG(("VMEXIT: CR8 write"));
        Bit64u qualification = 8 | (i->src() << 8);
        VMexit(VMX_VMEXIT_CR_ACCESS, qualification);
    }
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