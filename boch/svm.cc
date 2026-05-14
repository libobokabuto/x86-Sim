#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_SUPPORT_SVM

#include "svm.h"
#include "cpuid.h"

#include "ia_opcodes.h"

#include "debug.h"

BX_CPP_INLINE void BX_CPU_C::vmcb_write8(unsigned offset, Bit8u val_8)
{
    bx_phy_address pAddr = BX_CPU_THIS_PTR vmcbptr + offset;

    if (BX_CPU_THIS_PTR vmcbhostptr) {
        Bit8u* hostAddr = (Bit8u*)(BX_CPU_THIS_PTR vmcbhostptr | offset);
        pageWriteStampTable.decWriteStamp(pAddr, 1);
        *hostAddr = val_8;
        BX_NOTIFY_PHY_MEMORY_ACCESS(pAddr, 1, MEMTYPE(BX_CPU_THIS_PTR vmcb_memtype), BX_WRITE, BX_VMCS_ACCESS, (Bit8u*)(&val_8));
    }
    else {
        write_physical_byte(pAddr, val_8, MEMTYPE(BX_CPU_THIS_PTR vmcb_memtype), BX_VMCS_ACCESS);
    }
}

BX_CPP_INLINE void BX_CPU_C::vmcb_write16(unsigned offset, Bit16u val_16)
{
    bx_phy_address pAddr = BX_CPU_THIS_PTR vmcbptr + offset;

    if (BX_CPU_THIS_PTR vmcbhostptr) {
        Bit16u* hostAddr = (Bit16u*)(BX_CPU_THIS_PTR vmcbhostptr | offset);
        pageWriteStampTable.decWriteStamp(pAddr, 2);
        WriteHostWordToLittleEndian(hostAddr, val_16);
        BX_NOTIFY_PHY_MEMORY_ACCESS(pAddr, 2, MEMTYPE(BX_CPU_THIS_PTR vmcb_memtype), BX_WRITE, BX_VMCS_ACCESS, (Bit8u*)(&val_16));
    }
    else {
        write_physical_word(pAddr, val_16, MEMTYPE(BX_CPU_THIS_PTR vmcb_memtype), BX_VMCS_ACCESS);
    }
}


BX_CPP_INLINE void BX_CPU_C::vmcb_write32(unsigned offset, Bit32u val_32)
{
    bx_phy_address pAddr = BX_CPU_THIS_PTR vmcbptr + offset;

    if (BX_CPU_THIS_PTR vmcbhostptr) {
        Bit32u* hostAddr = (Bit32u*)(BX_CPU_THIS_PTR vmcbhostptr | offset);
        pageWriteStampTable.decWriteStamp(pAddr, 4);
        WriteHostDWordToLittleEndian(hostAddr, val_32);
        BX_NOTIFY_PHY_MEMORY_ACCESS(pAddr, 4, MEMTYPE(BX_CPU_THIS_PTR vmcb_memtype), BX_WRITE, BX_VMCS_ACCESS, (Bit8u*)(&val_32));
    }
    else {
        write_physical_dword(pAddr, val_32, MEMTYPE(BX_CPU_THIS_PTR vmcb_memtype), BX_VMCS_ACCESS);
    }
}


BX_CPP_INLINE void BX_CPU_C::vmcb_write64(unsigned offset, Bit64u val_64)
{ //185
    bx_phy_address pAddr = BX_CPU_THIS_PTR vmcbptr + offset;

    if (BX_CPU_THIS_PTR vmcbhostptr) {
        Bit64u* hostAddr = (Bit64u*)(BX_CPU_THIS_PTR vmcbhostptr | offset);
        pageWriteStampTable.decWriteStamp(pAddr, 8);
        WriteHostQWordToLittleEndian(hostAddr, val_64);
        BX_NOTIFY_PHY_MEMORY_ACCESS(pAddr, 8, MEMTYPE(BX_CPU_THIS_PTR vmcb_memtype), BX_WRITE, BX_VMCS_ACCESS, (Bit8u*)(&val_64));
    }
    else {
        write_physical_qword(pAddr, val_64, MEMTYPE(BX_CPU_THIS_PTR vmcb_memtype), BX_VMCS_ACCESS);
    }
}

BX_CPP_INLINE void BX_CPU_C::svm_segment_write(bx_segment_reg_t* seg, unsigned offset)
{
    Bit32u selector = seg->selector.value;
    bx_address base = seg->cache.u.segment.base;
    Bit32u limit = seg->cache.u.segment.limit_scaled;
    Bit32u attr = (seg->cache.valid) ?
        (get_descriptor_h(&seg->cache) & 0x00f0ff00) : 0;

    vmcb_write16(offset, selector);
    vmcb_write16(offset + 2, ((attr >> 8) & 0xff) | ((attr >> 12) & 0xf00));
    vmcb_write32(offset + 4, limit);
    vmcb_write64(offset + 8, base);
}


void BX_CPU_C::SvmExitLoadHostState(SVM_HOST_STATE* host)
{ //246
    BX_CPU_THIS_PTR tsc_offset = 0;

    for (unsigned n = 0; n < 4; n++) {
        BX_CPU_THIS_PTR sregs[n] = host->sregs[n];
        // we don't save selector details so parse selector again after loading
        parse_selector(BX_CPU_THIS_PTR sregs[n].selector.value, &BX_CPU_THIS_PTR sregs[n].selector);
    }

    BX_CPU_THIS_PTR gdtr = host->gdtr;
    BX_CPU_THIS_PTR idtr = host->idtr;

    BX_CPU_THIS_PTR efer = host->efer;
    BX_CPU_THIS_PTR cr0.set32(host->cr0.get32() | BX_CR0_PE_MASK); // always set the CR0.PE
    BX_CPU_THIS_PTR cr3 = host->cr3;
    BX_CPU_THIS_PTR cr4 = host->cr4;

    if (BX_CPU_THIS_PTR cr0.get_PG() && BX_CPU_THIS_PTR cr4.get_PAE() && !long_mode()) {
        if (!CheckPDPTR(BX_CPU_THIS_PTR cr3)) {
            //BX_ERROR(("SvmExitLoadHostState(): PDPTR check failed !"));
            shutdown();
        }
    }

    BX_CPU_THIS_PTR msr.pat = host->pat_msr;

    BX_CPU_THIS_PTR dr7.set32(0x00000400);

    setEFlags(host->eflags & ~EFlagsVMMask); // ignore saved copy of EFLAGS.VM

    RIP = BX_CPU_THIS_PTR prev_rip = host->rip;
    RSP = host->rsp;
    RAX = host->rax;

    CPL = 0;

    handleCpuContextChange();

#if BX_SUPPORT_MONITOR_MWAIT
    BX_CPU_THIS_PTR monitor.reset_monitor();
#endif

    BX_INSTR_TLB_CNTRL(BX_CPU_ID, BX_INSTR_CONTEXT_SWITCH, 0);
}

void BX_CPU_C::SvmExitSaveGuestState(void)
{  //292
    for (unsigned n = 0; n < 4; n++) {
        svm_segment_write(&BX_CPU_THIS_PTR sregs[n], SVM_GUEST_ES_SELECTOR + n * 0x10);
    }

    vmcb_write64(SVM_GUEST_GDTR_BASE, BX_CPU_THIS_PTR gdtr.base);
    vmcb_write16(SVM_GUEST_GDTR_LIMIT, BX_CPU_THIS_PTR gdtr.limit);

    vmcb_write64(SVM_GUEST_IDTR_BASE, BX_CPU_THIS_PTR idtr.base);
    vmcb_write16(SVM_GUEST_IDTR_LIMIT, BX_CPU_THIS_PTR idtr.limit);

    vmcb_write64(SVM_GUEST_EFER_MSR, BX_CPU_THIS_PTR efer.get32());
    vmcb_write64(SVM_GUEST_CR0, BX_CPU_THIS_PTR cr0.get32());
    vmcb_write64(SVM_GUEST_CR2, BX_CPU_THIS_PTR cr2);
    vmcb_write64(SVM_GUEST_CR3, BX_CPU_THIS_PTR cr3);
    vmcb_write64(SVM_GUEST_CR4, BX_CPU_THIS_PTR cr4.get32());

    vmcb_write64(SVM_GUEST_DR6, BX_CPU_THIS_PTR dr6.get32());
    vmcb_write64(SVM_GUEST_DR7, BX_CPU_THIS_PTR dr7.get32());

    vmcb_write64(SVM_GUEST_RFLAGS, read_eflags());
    vmcb_write64(SVM_GUEST_RAX, RAX);
    vmcb_write64(SVM_GUEST_RSP, RSP);
    vmcb_write64(SVM_GUEST_RIP, RIP);

    vmcb_write8(SVM_GUEST_CPL, CPL);

    vmcb_write8(SVM_CONTROL_INTERRUPT_SHADOW, interrupts_inhibited(BX_INHIBIT_INTERRUPTS));

    SVM_CONTROLS* ctrls = &BX_CPU_THIS_PTR vmcb->ctrls;

    if (ctrls->nested_paging) {
        vmcb_write64(SVM_GUEST_PAT, BX_CPU_THIS_PTR msr.pat.u64);
    }

    vmcb_write8(SVM_CONTROL_VTPR, ctrls->v_tpr);
    vmcb_write8(SVM_CONTROL_VIRQ, is_pending(BX_EVENT_SVM_VIRQ_PENDING));
    clear_event(BX_EVENT_SVM_VIRQ_PENDING);
}


void BX_CPU_C::Svm_Vmexit(int reason, Bit64u exitinfo1, Bit64u exitinfo2)
{  //614
    //BX_DEBUG(("SVM VMEXIT reason=%d exitinfo1=%08x%08x exitinfo2=%08x%08x", reason,
        //GET32H(exitinfo1), GET32L(exitinfo1), GET32H(exitinfo2), GET32L(exitinfo2)));

    if (!BX_CPU_THIS_PTR in_svm_guest) {
        if (reason != SVM_VMEXIT_INVALID) {
            //BX_PANIC(("PANIC: VMEXIT %d not in SVM guest mode !", reason));
        }
    }

    if (BX_SUPPORT_SVM_EXTENSION(BX_CPUID_SVM_NRIP_SAVE))
        vmcb_write64(SVM_CONTROL64_NRIP, RIP);

    // VMEXITs are FAULT-like: restore RIP/RSP to value before VMEXIT occurred
    RIP = BX_CPU_THIS_PTR prev_rip;
    if (BX_CPU_THIS_PTR speculative_rsp)
        RSP = BX_CPU_THIS_PTR prev_rsp;
    BX_CPU_THIS_PTR speculative_rsp = false;

    if (BX_SUPPORT_SVM_EXTENSION(BX_CPUID_SVM_DECODE_ASSIST)) {
        //
        // In the case of a Nested #PF or intercepted #PF, guest instruction bytes at
        // guest CS:RIP are stored into the 16-byte wide field Guest Instruction Bytes.
        // Up to 15 bytes are recorded, read from guest CS:RIP. The number of bytes
        // fetched is put into the first byte of this field. Zero indicates that no
        // bytes were fetched.
        //
        // This field is filled in only during data page faults. Instruction-fetch
        // page faults provide no additional information. All other intercepts clear
        // bits 0:7 in this field to zero.
        //

        if ((reason == SVM_VMEXIT_PF_EXCEPTION || reason == SVM_VMEXIT_NPF) && !(exitinfo1 & 0x10))
        {
            // TODO
        }
        else {
            vmcb_write8(SVM_CONTROL64_GUEST_INSTR_BYTES, 0);
        }
    }

    mask_event(BX_EVENT_SVM_VIRQ_PENDING);

    BX_CPU_THIS_PTR in_svm_guest = false;
    BX_CPU_THIS_PTR svm_gif = false;

    //
    // STEP 0: Update exit reason
    //

    SVM_CONTROLS* ctrls = &BX_CPU_THIS_PTR vmcb->ctrls;

    vmcb_write64(SVM_CONTROL64_EXITCODE, (Bit64u)((Bit64s)reason));
    vmcb_write64(SVM_CONTROL64_EXITINFO1, exitinfo1);
    vmcb_write64(SVM_CONTROL64_EXITINFO2, exitinfo2);

    // clean interrupt injection field
    vmcb_write32(SVM_CONTROL32_EVENT_INJECTION, ctrls->eventinj & ~0x80000000);

    if (BX_CPU_THIS_PTR in_event) {
        vmcb_write32(SVM_CONTROL32_EXITINTINFO, ctrls->exitintinfo | 0x80000000);
        vmcb_write32(SVM_CONTROL32_EXITINTINFO_ERROR_CODE, ctrls->exitintinfo_error_code);
        BX_CPU_THIS_PTR in_event = false;
    }
    else {
        vmcb_write32(SVM_CONTROL32_EXITINTINFO, 0);
    }

    //
    // Step 1: Save guest state in the VMCB
    //
    SvmExitSaveGuestState();

    //
    // Step 2:
    //
    SvmExitLoadHostState(&BX_CPU_THIS_PTR vmcb->host_state);

    //
    // STEP 3: Go back to SVM host
    //

    BX_CPU_THIS_PTR EXT = 0;
    BX_CPU_THIS_PTR last_exception_type = 0;

#if BX_DEBUGGER
    if (bx_dbg.debugger_active) {
        if (BX_CPU_THIS_PTR vmexit_break) {
            BX_CPU_THIS_PTR stop_reason = STOP_VMEXIT_BREAK_POINT;
            bx_debug_break(); // trap into debugger
        }
    }
#endif

    longjmp(BX_CPU_THIS_PTR jmp_buf_env, 1); // go back to main decode loop
}

void BX_CPU_C::SvmInterceptException(unsigned type, unsigned vector, Bit16u errcode, bool errcode_valid, Bit64u qualification)
{
    if (!BX_CPU_THIS_PTR in_svm_guest) return;

    //BX_ASSERT(vector < 32);

    SVM_CONTROLS* ctrls = &BX_CPU_THIS_PTR vmcb->ctrls;

    //BX_ASSERT(type == BX_HARDWARE_EXCEPTION || type == BX_SOFTWARE_EXCEPTION);

    if (!SVM_EXCEPTION_INTERCEPTED(vector)) {

        // -----------------------------------------
        //              EXITINTINFO
        // -----------------------------------------
        // [07:00] | Interrupt/Exception vector
        // [10:08] | Interrupt/Exception type
        // [11:11] | error code pushed to the stack
        // [30:12] | reserved
        // [31:31] | interruption info valid
        //

        // record IDT vectoring information
        ctrls->exitintinfo_error_code = errcode;
        ctrls->exitintinfo = vector | (BX_HARDWARE_EXCEPTION << 8);
        if (errcode_valid)
            BX_CPU_THIS_PTR vmcb->ctrls.exitintinfo |= (1 << 11); // error code delivered
        return;
    }

    //BX_ERROR(("SVM VMEXIT: event vector 0x%02x type %d error code=0x%04x", vector, type, errcode));

    // VMEXIT is not considered to occur during event delivery if it results
    // in a double fault exception that causes VMEXIT directly
    if (vector == BX_DF_EXCEPTION)
        BX_CPU_THIS_PTR in_event = false; // clear in_event indication on #DF

    BX_CPU_THIS_PTR debug_trap = 0; // clear debug_trap field
    BX_CPU_THIS_PTR inhibit_mask = 0;

    Svm_Vmexit(SVM_VMEXIT_EXCEPTION + vector, (errcode_valid ? errcode : 0), qualification);
}

enum {
    SVM_VMEXIT_IO_PORTIN = (1 << 0),
    SVM_VMEXIT_IO_INSTR_STRING = (1 << 2),
    SVM_VMEXIT_IO_INSTR_REP = (1 << 3),
    SVM_VMEXIT_IO_INSTR_LEN8 = (1 << 4),
    SVM_VMEXIT_IO_INSTR_LEN16 = (1 << 5),
    SVM_VMEXIT_IO_INSTR_LEN32 = (1 << 6),
    SVM_VMEXIT_IO_INSTR_ASIZE16 = (1 << 7),
    SVM_VMEXIT_IO_INSTR_ASIZE32 = (1 << 8),
    SVM_VMEXIT_IO_INSTR_ASIZE64 = (1 << 9)
};

void BX_CPU_C::SvmInterceptIO(bxInstruction_c* i, unsigned port, unsigned len)
{
    if (!BX_CPU_THIS_PTR in_svm_guest) return;

    if (!SVM_INTERCEPT(SVM_INTERCEPT0_IO)) return;

    Bit8u bitmap[2];
    bx_phy_address pAddr;

    // access_read_physical cannot read 2 bytes cross 4K boundary :(
    pAddr = BX_CPU_THIS_PTR vmcb->ctrls.iopm_base + (port / 8);
    bitmap[0] = read_physical_byte(pAddr, MEMTYPE(resolve_memtype(pAddr)), BX_IO_BITMAP_ACCESS);

    pAddr++;
    bitmap[1] = read_physical_byte(pAddr, MEMTYPE(resolve_memtype(pAddr)), BX_IO_BITMAP_ACCESS);

    Bit16u combined_bitmap = bitmap[1];
    combined_bitmap = (combined_bitmap << 8) | bitmap[0];

    unsigned mask = ((1 << len) - 1) << (port & 7);
    if (combined_bitmap & mask) {
        //BX_ERROR(("SVM VMEXIT: I/O port 0x%04x", port));

        Bit32u qualification = 0;

        switch (i->getIaOpcode()) {
        case BX_IA_IN_ALIb:
        case BX_IA_IN_AXIb:
        case BX_IA_IN_EAXIb:
        case BX_IA_IN_ALDX:
        case BX_IA_IN_AXDX:
        case BX_IA_IN_EAXDX:
            qualification = SVM_VMEXIT_IO_PORTIN;
            break;

        case BX_IA_OUT_IbAL:
        case BX_IA_OUT_IbAX:
        case BX_IA_OUT_IbEAX:
        case BX_IA_OUT_DXAL:
        case BX_IA_OUT_DXAX:
        case BX_IA_OUT_DXEAX:
            qualification = 0; // PORTOUT
            break;

        case BX_IA_REP_INSB_YbDX:
        case BX_IA_REP_INSW_YwDX:
        case BX_IA_REP_INSD_YdDX:
            qualification = SVM_VMEXIT_IO_PORTIN | SVM_VMEXIT_IO_INSTR_STRING;
            if (i->repUsedL())
                qualification |= SVM_VMEXIT_IO_INSTR_REP;
            break;

        case BX_IA_REP_OUTSB_DXXb:
        case BX_IA_REP_OUTSW_DXXw:
        case BX_IA_REP_OUTSD_DXXd:
            qualification = SVM_VMEXIT_IO_INSTR_STRING; // PORTOUT
            if (i->repUsedL())
                qualification |= SVM_VMEXIT_IO_INSTR_REP;
            break;

        default:
            //BX_PANIC(("VMexit_IO: I/O instruction %s unknown", i->getIaOpcodeNameShort()));
            break;
        }

        qualification |= (port << 16);
        if (len == 1)
            qualification |= SVM_VMEXIT_IO_INSTR_LEN8;
        else if (len == 2)
            qualification |= SVM_VMEXIT_IO_INSTR_LEN16;
        else if (len == 4)
            qualification |= SVM_VMEXIT_IO_INSTR_LEN32;

        if (i->as64L())
            qualification |= SVM_VMEXIT_IO_INSTR_ASIZE64;
        else if (i->as32L())
            qualification |= SVM_VMEXIT_IO_INSTR_ASIZE32;
        else
            qualification |= SVM_VMEXIT_IO_INSTR_ASIZE16;

        Svm_Vmexit(SVM_VMEXIT_IO, qualification, RIP);
    }
}

#endif // BX_SUPPORT_SVM
