#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#include "cpuid.h"
#define LOG_THIS BX_CPU_THIS_PTR

#include "iodev.h"
#include "debug.h"
extern VMCS_Mapping vmcs_map;

extern int get_exception_class(unsigned vector);
extern int get_exception_type(unsigned vector);
extern bool exception_push_error(unsigned vector);


Bit32u BX_CPP_AttrRegparmN(1) BX_CPU_C::VMread32(unsigned encoding)
{
    //210
    /*
    Bit32u field;

    unsigned offset = BX_CPU_THIS_PTR vmcs_map->vmcs_field_offset(encoding);
    if (offset >= VMX_VMCS_AREA_SIZE)
        BX_PANIC(("VMread32: can't access encoding 0x%08x, offset=0x%x", encoding, offset));
    bx_phy_address pAddr = BX_CPU_THIS_PTR vmcsptr + offset;

    if (BX_CPU_THIS_PTR vmcshostptr) {
        Bit32u* hostAddr = (Bit32u*)(BX_CPU_THIS_PTR vmcshostptr | offset);
        field = ReadHostDWordFromLittleEndian(hostAddr);
        BX_NOTIFY_PHY_MEMORY_ACCESS(pAddr, 4, MEMTYPE(BX_CPU_THIS_PTR vmcs_memtype), BX_READ, BX_VMCS_ACCESS, (Bit8u*)(&field));
    }
    else {
        field = read_physical_dword(pAddr, MEMTYPE(BX_CPU_THIS_PTR vmcs_memtype), BX_VMCS_ACCESS);
    }

    return field;
    */
    return 0; //自己加的，源码没有这一行，原因是这里被我注释但必须要一个返回值
}

void BX_CPP_AttrRegparmN(2) BX_CPU_C::VMwrite32(unsigned encoding, Bit32u val_32)
{
    //232
    /*
    unsigned offset = BX_CPU_THIS_PTR vmcs_map->vmcs_field_offset(encoding);
    if (offset >= VMX_VMCS_AREA_SIZE)
    {//BX_PANIC(("VMwrite32: can't access encoding 0x%08x, offset=0x%x", encoding, offset));
    }
    bx_phy_address pAddr = BX_CPU_THIS_PTR vmcsptr + offset;

    if (BX_CPU_THIS_PTR vmcshostptr) {
        Bit32u* hostAddr = (Bit32u*)(BX_CPU_THIS_PTR vmcshostptr | offset);
        pageWriteStampTable.decWriteStamp(pAddr, 4);
        WriteHostDWordToLittleEndian(hostAddr, val_32);
    }
    else {
        write_physical_dword(pAddr, val_32, MEMTYPE(BX_CPU_THIS_PTR vmcs_memtype), BX_VMCS_ACCESS);
    }

    BX_NOTIFY_PHY_MEMORY_ACCESS(pAddr, 4, MEMTYPE(BX_CPU_THIS_PTR vmcs_memtype), BX_WRITE, BX_VMCS_ACCESS, (Bit8u*)(&val_32));
    */
}

void BX_CPP_AttrRegparmN(2) BX_CPU_C::VMwrite64(unsigned encoding, Bit64u val_64)
{
    /*
    BX_ASSERT(!IS_VMCS_FIELD_HI(encoding));

    unsigned offset = BX_CPU_THIS_PTR vmcs_map->vmcs_field_offset(encoding);
    if (offset >= VMX_VMCS_AREA_SIZE)
        BX_PANIC(("VMwrite64: can't access encoding 0x%08x, offset=0x%x", encoding, offset));
    bx_phy_address pAddr = BX_CPU_THIS_PTR vmcsptr + offset;

    if (BX_CPU_THIS_PTR vmcshostptr) {
        Bit64u* hostAddr = (Bit64u*)(BX_CPU_THIS_PTR vmcshostptr | offset);
        pageWriteStampTable.decWriteStamp(pAddr, 8);
        WriteHostQWordToLittleEndian(hostAddr, val_64);
    }
    else {
        write_physical_qword(pAddr, val_64, MEMTYPE(BX_CPU_THIS_PTR vmcs_memtype), BX_VMCS_ACCESS);
    }

    BX_NOTIFY_PHY_MEMORY_ACCESS(pAddr, 8, MEMTYPE(BX_CPU_THIS_PTR vmcs_memtype), BX_WRITE, BX_VMCS_ACCESS, (Bit8u*)(&val_64));
    */
}

#if BX_SUPPORT_X86_64
void BX_CPP_AttrRegparmN(2) BX_CPU_C::VMwrite_natural(unsigned encoding, bx_address val)
{/*
    //302
    VMwrite64(encoding, val);
    */
}
#else
#endif

void BX_CPU_C::VMabort(VMX_vmabort_code error_code)
{
    //416
    /*
    VMwrite32(VMCS_VMX_ABORT_FIELD_ENCODING, (Bit32u)error_code);

#if BX_SUPPORT_VMX >= 2
    // Deactivate VMX preemtion timer
    BX_CPU_THIS_PTR lapic->deactivate_vmx_preemption_timer();
#endif

    shutdown();
    */
}
Bit32u BX_CPU_C::VMenterLoadCheckGuestState(Bit64u* qualification)
{
    /*
    //1365
    int n;

    VMCS_GUEST_STATE guest;
    VMCS_CACHE* vm = &BX_CPU_THIS_PTR vmcs;

    *qualification = VMENTER_ERR_NO_ERROR;

    //
    // Load and Check Guest State from VMCS
    //

    guest.rflags = VMread_natural(VMCS_GUEST_RFLAGS);
    // RFLAGS reserved bits [63:22], bit 15, bit 5, bit 3 must be zero
    if (guest.rflags & BX_CONST64(0xFFFFFFFFFFC08028)) {
        BX_ERROR(("VMENTER FAIL: RFLAGS reserved bits are set"));
        return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
    }
    // RFLAGS[1] must be always set
    if ((guest.rflags & 0x2) == 0) {
        BX_ERROR(("VMENTER FAIL: RFLAGS[1] cleared"));
        return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
    }

    bool v8086_guest = false;
    if (guest.rflags & EFlagsVMMask)
        v8086_guest = true;

    bool x86_64_guest = vm->vmentry_ctrls.X86_64_GUEST(); // can't be set if X86_64 is not supported (checked before)
#if BX_SUPPORT_X86_64
    if (x86_64_guest) BX_DEBUG(("VMENTER to x86-64 guest"));
#endif

    if (x86_64_guest && v8086_guest) {
        BX_ERROR(("VMENTER FAIL: Enter to x86-64 guest with RFLAGS.VM"));
        return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
    }

    guest.cr0 = VMread_natural(VMCS_GUEST_CR0);

#if BX_SUPPORT_VMX >= 2
    if (vm->vmexec_ctrls2.UNRESTRICTED_GUEST()) {
        if (~guest.cr0 & (VMX_MSR_CR0_FIXED0 & ~(BX_CR0_PE_MASK | BX_CR0_PG_MASK))) {
            BX_ERROR(("VMENTER FAIL: VMCS guest invalid CR0"));
            return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }

        bool pe = (guest.cr0 & BX_CR0_PE_MASK) != 0;
        bool pg = (guest.cr0 & BX_CR0_PG_MASK) != 0;
        if (pg && !pe) {
            BX_ERROR(("VMENTER FAIL: VMCS unrestricted guest CR0.PG without CR0.PE"));
            return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }
    }
    else
#endif
    {
        if (~guest.cr0 & VMX_MSR_CR0_FIXED0) {
            BX_ERROR(("VMENTER FAIL: VMCS guest invalid CR0"));
            return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }
    }

    if (guest.cr0 & ~VMX_MSR_CR0_FIXED1) {
        BX_ERROR(("VMENTER FAIL: VMCS guest invalid CR0"));
        return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
    }

#if BX_SUPPORT_VMX >= 2
    bool real_mode_guest = false;
    if (!(guest.cr0 & BX_CR0_PE_MASK))
        real_mode_guest = true;
#endif

    guest.cr3 = VMread_natural(VMCS_GUEST_CR3);
#if BX_SUPPORT_X86_64
    if (!IsValidPhyAddr(guest.cr3)) {
        BX_ERROR(("VMENTER FAIL: VMCS guest invalid CR3"));
        return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
    }
#endif

    guest.cr4 = VMread_natural(VMCS_GUEST_CR4);
    if (~guest.cr4 & VMX_MSR_CR4_FIXED0) {
        BX_ERROR(("VMENTER FAIL: VMCS guest invalid CR4"));
        return VMXERR_VMENTRY_INVALID_VM_HOST_STATE_FIELD;
    }

    if (guest.cr4 & ~VMX_MSR_CR4_FIXED1) {
        BX_ERROR(("VMENTER FAIL: VMCS guest invalid CR4"));
        return VMXERR_VMENTRY_INVALID_VM_HOST_STATE_FIELD;
    }

#if BX_SUPPORT_X86_64
    if (x86_64_guest) {
        if ((guest.cr4 & BX_CR4_PAE_MASK) == 0) {
            BX_ERROR(("VMENTER FAIL: VMCS guest CR4.PAE=0 in x86-64 mode"));
            return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }
    }
    else {
        if (guest.cr4 & BX_CR4_PCIDE_MASK) {
            BX_ERROR(("VMENTER FAIL: VMCS CR4.PCIDE set in 32-bit guest"));
            return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }
    }

    if (vm->vmentry_ctrls.LOAD_DBG_CTRLS()) {
        guest.dr7 = VMread_natural(VMCS_GUEST_DR7);
        if (GET32H(guest.dr7)) {
            BX_ERROR(("VMENTER FAIL: VMCS guest invalid DR7"));
            return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }
    }
#endif

#if BX_SUPPORT_CET
    if ((guest.cr4 & BX_CR4_CET_MASK) && (guest.cr0 & BX_CR0_WP_MASK) == 0) {
        BX_ERROR(("VMENTER FAIL: VMCS guest CR4.CET=1 when CR0.WP=0"));
        return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
    }

    if (vm->vmentry_ctrls.LOAD_GUEST_CET_STATE()) {
        guest.msr_ia32_s_cet = VMread_natural(VMCS_GUEST_IA32_S_CET);
        if (!IsCanonical(guest.msr_ia32_s_cet) || (!x86_64_guest && GET32H(guest.msr_ia32_s_cet))) {
            BX_ERROR(("VMFAIL: VMCS guest IA32_S_CET/EB_LEG_BITMAP_BASE non canonical or invalid"));
            return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }

        if (is_invalid_cet_control(guest.msr_ia32_s_cet)) {
            BX_ERROR(("VMFAIL: VMCS guest IA32_S_CET invalid"));
            return VMXERR_VMENTRY_INVALID_VM_HOST_STATE_FIELD;
        }

        guest.ssp = VMread_natural(VMCS_GUEST_SSP);
        if (!IsCanonical(guest.ssp) || (!x86_64_guest && GET32H(guest.ssp))) {
            BX_ERROR(("VMFAIL: VMCS guest SSP non canonical or invalid"));
            return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }
        if ((guest.ssp & 0x3) != 0) {
            BX_ERROR(("VMFAIL: VMCS guest SSP[1:0] not zero"));
            return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }

        guest.interrupt_ssp_table_address = VMread_natural(VMCS_GUEST_INTERRUPT_SSP_TABLE_ADDR);
        if (!IsCanonical(guest.interrupt_ssp_table_address)) {
            BX_ERROR(("VMFAIL: VMCS guest INTERRUPT_SSP_TABLE_ADDR non canonical or invalid"));
            return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }
    }
#endif

#if BX_SUPPORT_PKEYS
    if (vm->vmentry_ctrls.LOAD_GUEST_PKRS()) {
        guest.pkrs = VMread64(VMCS_64BIT_GUEST_IA32_PKRS);
        if (GET32H(guest.pkrs) != 0) {
            BX_ERROR(("VMFAIL: invalid guest IA32_PKRS value"));
            return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }
    }
#endif

    //
    // Load and Check Guest State from VMCS - Segment Registers
    //

    for (n = 0; n < 6; n++) {
        Bit16u selector = VMread16(VMCS_16BIT_GUEST_ES_SELECTOR + 2 * n);
        bx_address base = (bx_address)VMread_natural(VMCS_GUEST_ES_BASE + 2 * n);
        Bit32u limit = VMread32(VMCS_32BIT_GUEST_ES_LIMIT + 2 * n);
        Bit32u ar = VMread32(VMCS_32BIT_GUEST_ES_ACCESS_RIGHTS + 2 * n);
        ar = vmx_unpack_ar_field(ar, BX_CPU_THIS_PTR vmcs_map->get_access_rights_format());
        bool invalid = (ar >> 16) & 1;

        set_segment_ar_data(&guest.sregs[n], !invalid,
            (Bit16u)selector, base, limit, (Bit16u)ar);

        if (v8086_guest) {
            // guest in V8086 mode
            if (base != ((bx_address)(selector << 4))) {
                BX_ERROR(("VMENTER FAIL: VMCS v8086 guest bad %s.BASE", segname[n]));
                return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
            }
            if (limit != 0xffff) {
                BX_ERROR(("VMENTER FAIL: VMCS v8086 guest %s.LIMIT != 0xFFFF", segname[n]));
                return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
            }
            // present, expand-up read/write accessed, segment, DPL=3
            if (ar != 0xF3) {
                BX_ERROR(("VMENTER FAIL: VMCS v8086 guest %s.AR != 0xF3", segname[n]));
                return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
            }

            continue; // go to next segment register
        }

#if BX_SUPPORT_X86_64
        if (n >= BX_SEG_REG_FS) {
            if (!IsCanonical(base)) {
                BX_ERROR(("VMENTER FAIL: VMCS guest %s.BASE non canonical", segname[n]));
                return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
            }
        }
#endif

        if (n != BX_SEG_REG_CS && invalid)
            continue;

#if BX_SUPPORT_X86_64
        if (n == BX_SEG_REG_SS && (selector & BX_SELECTOR_RPL_MASK) == 0) {
            // SS is allowed to be NULL selector if going to 64-bit guest
            if (x86_64_guest && guest.sregs[BX_SEG_REG_CS].cache.u.segment.l)
                continue;
        }

        if (n < BX_SEG_REG_FS) {
            if (GET32H(base) != 0) {
                BX_ERROR(("VMENTER FAIL: VMCS guest %s.BASE > 32 bit", segname[n]));
                return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
            }
        }
#endif

        if (!guest.sregs[n].cache.segment) {
            BX_ERROR(("VMENTER FAIL: VMCS guest %s not segment", segname[n]));
            return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }

        if (!guest.sregs[n].cache.p) {
            BX_ERROR(("VMENTER FAIL: VMCS guest %s not present", segname[n]));
            return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }

        if (!IsLimitAccessRightsConsistent(limit, ar)) {
            BX_ERROR(("VMENTER FAIL: VMCS guest %s.AR/LIMIT malformed", segname[n]));
            return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }

        if (n == BX_SEG_REG_CS) {
            // CS checks
            switch (guest.sregs[BX_SEG_REG_CS].cache.type) {
            case BX_CODE_EXEC_ONLY_ACCESSED:
            case BX_CODE_EXEC_READ_ACCESSED:              // non-conforming segment
            case BX_CODE_EXEC_ONLY_CONFORMING_ACCESSED:
            case BX_CODE_EXEC_READ_CONFORMING_ACCESSED:   // conforming segment
                break;
#if BX_SUPPORT_VMX >= 2
            case BX_DATA_READ_WRITE_ACCESSED:
                if (vm->vmexec_ctrls2.UNRESTRICTED_GUEST()) {
                    if (guest.sregs[BX_SEG_REG_CS].cache.dpl != 0) {
                        BX_ERROR(("VMENTER FAIL: VMCS unrestricted guest CS.DPL != 0"));
                        return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
                    }
                    break;
                }
                // fall through
#endif
            default:
                BX_ERROR(("VMENTER FAIL: VMCS guest CS.TYPE"));
                return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
            }

#if BX_SUPPORT_X86_64
            if (x86_64_guest) {
                if (guest.sregs[BX_SEG_REG_CS].cache.u.segment.d_b && guest.sregs[BX_SEG_REG_CS].cache.u.segment.l) {
                    BX_ERROR(("VMENTER FAIL: VMCS x86_64 guest wrong CS.D_B/L"));
                    return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
                }
            }
#endif
        }
        else if (n == BX_SEG_REG_SS) {
            // SS checks
            switch (guest.sregs[BX_SEG_REG_SS].cache.type) {
            case BX_DATA_READ_WRITE_ACCESSED:
            case BX_DATA_READ_WRITE_EXPAND_DOWN_ACCESSED:
                break;
            default:
                BX_ERROR(("VMENTER FAIL: VMCS guest SS.TYPE"));
                return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
            }
        }
        else {
            // DS, ES, FS, GS
            if ((guest.sregs[n].cache.type & 0x1) == 0) {
                BX_ERROR(("VMENTER FAIL: VMCS guest %s not ACCESSED", segname[n]));
                return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
            }

            if (guest.sregs[n].cache.type & 0x8) {
                if ((guest.sregs[n].cache.type & 0x2) == 0) {
                    BX_ERROR(("VMENTER FAIL: VMCS guest CODE segment %s not READABLE", segname[n]));
                    return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
                }
            }

            if (!vm->vmexec_ctrls2.UNRESTRICTED_GUEST()) {
                if (guest.sregs[n].cache.type <= 11) {
                    // data segment or non-conforming code segment
                    if (guest.sregs[n].selector.rpl > guest.sregs[n].cache.dpl) {
                        BX_ERROR(("VMENTER FAIL: VMCS guest non-conforming %s.RPL < %s.DPL", segname[n], segname[n]));
                        return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
                    }
                }
            }
        }
    }

    switch (guest.sregs[BX_SEG_REG_CS].cache.type) {
    case BX_CODE_EXEC_ONLY_ACCESSED:
    case BX_CODE_EXEC_READ_ACCESSED:              // non-conforming segment
        if (guest.sregs[BX_SEG_REG_CS].cache.dpl != guest.sregs[BX_SEG_REG_SS].cache.dpl) {
            BX_ERROR(("VMENTER FAIL: VMCS guest non-conforming CS.DPL <> SS.DPL"));
            return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }
        break;
    case BX_CODE_EXEC_ONLY_CONFORMING_ACCESSED:
    case BX_CODE_EXEC_READ_CONFORMING_ACCESSED:   // conforming segment
        if (guest.sregs[BX_SEG_REG_CS].cache.dpl > guest.sregs[BX_SEG_REG_SS].cache.dpl) {
            BX_ERROR(("VMENTER FAIL: VMCS guest non-conforming CS.DPL > SS.DPL"));
            return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }
        break;
    }

    if (!v8086_guest) {
        if (!vm->vmexec_ctrls2.UNRESTRICTED_GUEST()) {
            if (guest.sregs[BX_SEG_REG_SS].selector.rpl != guest.sregs[BX_SEG_REG_CS].selector.rpl) {
                BX_ERROR(("VMENTER FAIL: VMCS guest CS.RPL != SS.RPL"));
                return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
            }
            if (guest.sregs[BX_SEG_REG_SS].selector.rpl != guest.sregs[BX_SEG_REG_SS].cache.dpl) {
                BX_ERROR(("VMENTER FAIL: VMCS guest SS.RPL <> SS.DPL"));
                return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
            }
        }
#if BX_SUPPORT_VMX >= 2
        else { // unrestricted guest
            if (real_mode_guest || guest.sregs[BX_SEG_REG_CS].cache.type == BX_DATA_READ_WRITE_ACCESSED) {
                if (guest.sregs[BX_SEG_REG_SS].cache.dpl != 0) {
                    BX_ERROR(("VMENTER FAIL: VMCS unrestricted guest SS.DPL != 0"));
                    return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
                }
            }
        }
#endif
    }

    //
    // Load and Check Guest State from VMCS - GDTR/IDTR
    //

    Bit64u gdtr_base = VMread_natural(VMCS_GUEST_GDTR_BASE);
    Bit32u gdtr_limit = VMread32(VMCS_32BIT_GUEST_GDTR_LIMIT);
    Bit64u idtr_base = VMread_natural(VMCS_GUEST_IDTR_BASE);
    Bit32u idtr_limit = VMread32(VMCS_32BIT_GUEST_IDTR_LIMIT);

#if BX_SUPPORT_X86_64
    if (!IsCanonical(gdtr_base) || !IsCanonical(idtr_base)) {
        BX_ERROR(("VMENTER FAIL: VMCS guest IDTR/IDTR.BASE non canonical"));
        return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
    }
#endif
    if (gdtr_limit > 0xffff || idtr_limit > 0xffff) {
        BX_ERROR(("VMENTER FAIL: VMCS guest GDTR/IDTR limit > 0xFFFF"));
        return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
    }

    //
    // Load and Check Guest State from VMCS - LDTR
    //

    Bit16u ldtr_selector = VMread16(VMCS_16BIT_GUEST_LDTR_SELECTOR);
    Bit64u ldtr_base = VMread_natural(VMCS_GUEST_LDTR_BASE);
    Bit32u ldtr_limit = VMread32(VMCS_32BIT_GUEST_LDTR_LIMIT);
    Bit32u ldtr_ar = VMread32(VMCS_32BIT_GUEST_LDTR_ACCESS_RIGHTS);
    ldtr_ar = vmx_unpack_ar_field(ldtr_ar, BX_CPU_THIS_PTR vmcs_map->get_access_rights_format());
    bool ldtr_invalid = (ldtr_ar >> 16) & 1;
    if (set_segment_ar_data(&guest.ldtr, !ldtr_invalid,
        (Bit16u)ldtr_selector, ldtr_base, ldtr_limit, (Bit16u)(ldtr_ar)))
    {
        // ldtr is valid
        if (guest.ldtr.selector.ti) {
            BX_ERROR(("VMENTER FAIL: VMCS guest LDTR.TI set"));
            return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }
        if (guest.ldtr.cache.type != BX_SYS_SEGMENT_LDT) {
            BX_ERROR(("VMENTER FAIL: VMCS guest incorrect LDTR type (%d)", guest.ldtr.cache.type));
            return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }
        if (guest.ldtr.cache.segment) {
            BX_ERROR(("VMENTER FAIL: VMCS guest LDTR is not system segment"));
            return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }
        if (!guest.ldtr.cache.p) {
            BX_ERROR(("VMENTER FAIL: VMCS guest LDTR not present"));
            return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }
        if (!IsLimitAccessRightsConsistent(ldtr_limit, ldtr_ar)) {
            BX_ERROR(("VMENTER FAIL: VMCS guest LDTR.AR/LIMIT malformed"));
            return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }
#if BX_SUPPORT_X86_64
        if (!IsCanonical(ldtr_base)) {
            BX_ERROR(("VMENTER FAIL: VMCS guest LDTR.BASE non canonical"));
            return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }
#endif
    }

    //
    // Load and Check Guest State from VMCS - TR
    //

    Bit16u tr_selector = VMread16(VMCS_16BIT_GUEST_TR_SELECTOR);
    Bit64u tr_base = VMread_natural(VMCS_GUEST_TR_BASE);
    Bit32u tr_limit = VMread32(VMCS_32BIT_GUEST_TR_LIMIT);
    Bit32u tr_ar = VMread32(VMCS_32BIT_GUEST_TR_ACCESS_RIGHTS);
    tr_ar = vmx_unpack_ar_field(tr_ar, BX_CPU_THIS_PTR vmcs_map->get_access_rights_format());
    bool tr_invalid = (tr_ar >> 16) & 1;

#if BX_SUPPORT_X86_64
    if (!IsCanonical(tr_base)) {
        BX_ERROR(("VMENTER FAIL: VMCS guest TR.BASE non canonical"));
        return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
    }
#endif

    set_segment_ar_data(&guest.tr, !tr_invalid,
        (Bit16u)tr_selector, tr_base, tr_limit, (Bit16u)(tr_ar));

    if (tr_invalid) {
        BX_ERROR(("VMENTER FAIL: VMCS guest TR invalid"));
        return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
    }
    if (guest.tr.selector.ti) {
        BX_ERROR(("VMENTER FAIL: VMCS guest TR.TI set"));
        return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
    }
    if (guest.tr.cache.segment) {
        BX_ERROR(("VMENTER FAIL: VMCS guest TR is not system segment"));
        return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
    }
    if (!guest.tr.cache.p) {
        BX_ERROR(("VMENTER FAIL: VMCS guest TR not present"));
        return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
    }
    if (!IsLimitAccessRightsConsistent(tr_limit, tr_ar)) {
        BX_ERROR(("VMENTER FAIL: VMCS guest TR.AR/LIMIT malformed"));
        return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
    }

    switch (guest.tr.cache.type) {
    case BX_SYS_SEGMENT_BUSY_386_TSS:
        break;
    case BX_SYS_SEGMENT_BUSY_286_TSS:
        if (!x86_64_guest) break;
        // fall through
    default:
        BX_ERROR(("VMENTER FAIL: VMCS guest incorrect TR type"));
        return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
    }

    //
    // Load and Check Guest State from VMCS - MSRS
    //

    guest.ia32_debugctl_msr = VMread64(VMCS_64BIT_GUEST_IA32_DEBUGCTL);
    guest.smbase = VMread32(VMCS_32BIT_GUEST_SMBASE);

    guest.sysenter_esp_msr = VMread_natural(VMCS_GUEST_IA32_SYSENTER_ESP_MSR);
    guest.sysenter_eip_msr = VMread_natural(VMCS_GUEST_IA32_SYSENTER_EIP_MSR);
    guest.sysenter_cs_msr = VMread32(VMCS_32BIT_GUEST_IA32_SYSENTER_CS_MSR);

#if BX_SUPPORT_X86_64
    if (!IsCanonical(guest.sysenter_esp_msr)) {
        BX_ERROR(("VMENTER FAIL: VMCS guest SYSENTER_ESP_MSR non canonical"));
        return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
    }
    if (!IsCanonical(guest.sysenter_eip_msr)) {
        BX_ERROR(("VMENTER FAIL: VMCS guest SYSENTER_EIP_MSR non canonical"));
        return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
    }
#endif

#if BX_SUPPORT_VMX >= 2
    if (vm->vmentry_ctrls.LOAD_PAT_MSR()) {
        guest.pat_msr = VMread64(VMCS_64BIT_GUEST_IA32_PAT);
        if (!isValidMSR_PAT(guest.pat_msr)) {
            BX_ERROR(("VMENTER FAIL: invalid Memory Type in guest MSR_PAT"));
            return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }
    }

    if (vm->vmentry_ctrls.LOAD_GUEST_IA32_SPEC_CTRL()) {
        guest.ia32_spec_ctrl_msr = VMread64(VMCS_64BIT_GUEST_IA32_SPEC_CTRL);
        if (!isValidMSR_IA32_SPEC_CTRL(guest.ia32_spec_ctrl_msr)) {
            BX_ERROR(("VMFAIL: invalid value in guest MSR_IA32_SPEC_CTRL"));
            return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }
    }
#endif

    guest.rip = VMread_natural(VMCS_GUEST_RIP);
    guest.rsp = VMread_natural(VMCS_GUEST_RSP);

#if BX_SUPPORT_VMX >= 2 && BX_SUPPORT_X86_64
    if (vm->vmentry_ctrls.LOAD_EFER_MSR()) {
        guest.efer_msr = VMread64(VMCS_64BIT_GUEST_IA32_EFER);
        if (guest.efer_msr & ~((Bit64u)BX_CPU_THIS_PTR efer_suppmask)) {
            BX_ERROR(("VMENTER FAIL: VMCS guest EFER reserved bits set !"));
            return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }
        bool lme = (guest.efer_msr >> 8) & 0x1;
        bool lma = (guest.efer_msr >> 10) & 0x1;
        if (lma != x86_64_guest) {
            BX_ERROR(("VMENTER FAIL: VMCS guest EFER.LMA doesn't match x86_64_guest !"));
            return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }
        if (lma != lme && (guest.cr0 & BX_CR0_PG_MASK) != 0) {
            BX_ERROR(("VMENTER FAIL: VMCS guest EFER (0x%08x) inconsistent value !", (Bit32u)guest.efer_msr));
            return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }
    }

    if (!x86_64_guest || !guest.sregs[BX_SEG_REG_CS].cache.u.segment.l) {
        if (GET32H(guest.rip) != 0) {
            BX_ERROR(("VMENTER FAIL: VMCS guest RIP > 32 bit"));
            return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }
    }
#endif

    //
    // Load and Check Guest Non-Registers State from VMCS
    //

    vm->vmcs_linkptr = VMread64(VMCS_64BIT_GUEST_LINK_POINTER);
    if (vm->vmcs_linkptr != BX_INVALID_VMCSPTR) {
        if (!IsValidPageAlignedPhyAddr(vm->vmcs_linkptr)) {
            *qualification = (Bit64u)VMENTER_ERR_GUEST_STATE_LINK_POINTER;
            BX_ERROR(("VMFAIL: VMCS link pointer malformed"));
            return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }

        Bit32u revision = VMXReadRevisionID((bx_phy_address)vm->vmcs_linkptr);
        if (vm->vmexec_ctrls2.VMCS_SHADOWING()) {
            if ((revision & BX_VMCS_SHADOW_BIT_MASK) == 0) {
                *qualification = (Bit64u)VMENTER_ERR_GUEST_STATE_LINK_POINTER;
                BX_ERROR(("VMFAIL: VMCS link pointer must indicate shadow VMCS revision ID = %d", revision));
                return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
            }
            revision &= ~BX_VMCS_SHADOW_BIT_MASK;
        }
        if (revision != BX_CPU_THIS_PTR vmcs_map->get_vmcs_revision_id()) {
            *qualification = (Bit64u)VMENTER_ERR_GUEST_STATE_LINK_POINTER;
            BX_ERROR(("VMFAIL: VMCS link pointer incorrect revision ID %d != %d", revision, BX_CPU_THIS_PTR vmcs_map->get_vmcs_revision_id()));
            return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }

        if (!BX_CPU_THIS_PTR in_smm || vm->vmentry_ctrls.SMM_ENTER()) {
            if (vm->vmcs_linkptr == BX_CPU_THIS_PTR vmcsptr) {
                *qualification = (Bit64u)VMENTER_ERR_GUEST_STATE_LINK_POINTER;
                BX_ERROR(("VMFAIL: VMCS link pointer equal to current VMCS pointer"));
                return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
            }
        }
        else {
            if (vm->vmcs_linkptr == BX_CPU_THIS_PTR vmxonptr) {
                *qualification = (Bit64u)VMENTER_ERR_GUEST_STATE_LINK_POINTER;
                BX_ERROR(("VMFAIL: VMCS link pointer equal to VMXON pointer"));
                return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
            }
        }
    }

    guest.tmpDR6 = (Bit32u)VMread_natural(VMCS_GUEST_PENDING_DBG_EXCEPTIONS);
    if (guest.tmpDR6 & BX_CONST64(0xFFFFFFFFFFFFAFF0)) {
        BX_ERROR(("VMENTER FAIL: VMCS guest tmpDR6 reserved bits"));
        return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
    }

    guest.activity_state = VMread32(VMCS_32BIT_GUEST_ACTIVITY_STATE);
    if (guest.activity_state > BX_VMX_LAST_ACTIVITY_STATE) {
        BX_ERROR(("VMENTER FAIL: VMCS guest activity state %d", guest.activity_state));
        return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
    }

    if (guest.activity_state == BX_ACTIVITY_STATE_HLT) {
        if (guest.sregs[BX_SEG_REG_SS].cache.dpl != 0) {
            BX_ERROR(("VMENTER FAIL: VMCS guest HLT state with SS.DPL=%d", guest.sregs[BX_SEG_REG_SS].cache.dpl));
            return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }
    }

    guest.interruptibility_state = VMread32(VMCS_32BIT_GUEST_INTERRUPTIBILITY_STATE);
    if (guest.interruptibility_state & ~BX_VMX_INTERRUPTIBILITY_STATE_MASK) {
        BX_ERROR(("VMENTER FAIL: VMCS guest interruptibility state broken"));
        return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
    }

    if (guest.interruptibility_state & 0x3) {
        if (guest.activity_state != BX_ACTIVITY_STATE_ACTIVE) {
            BX_ERROR(("VMENTER FAIL: VMCS guest interruptibility state broken when entering non active CPU state %d", guest.activity_state));
            return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }
    }

    if ((guest.interruptibility_state & BX_VMX_INTERRUPTS_BLOCKED_BY_STI) &&
        (guest.interruptibility_state & BX_VMX_INTERRUPTS_BLOCKED_BY_MOV_SS))
    {
        BX_ERROR(("VMENTER FAIL: VMCS guest interruptibility state broken"));
        return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
    }

#if BX_SUPPORT_UINTR
    if (vm->vmentry_ctrls.LOAD_UINV()) {
        guest.uintr_uinv = VMread16(VMCS_16BIT_GUEST_UINV);
        if (guest.uintr_uinv >= 256) {
            BX_ERROR(("VMENTER FAIL: VMCS guest INTR.UINV=%d > 256", guest.uintr_uinv));
            return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }
    }
#endif

    if ((guest.rflags & EFlagsIFMask) == 0) {
        if (guest.interruptibility_state & BX_VMX_INTERRUPTS_BLOCKED_BY_STI) {
            BX_ERROR(("VMENTER FAIL: VMCS guest interrupts can't be blocked by STI when EFLAGS.IF = 0"));
            return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }
    }

    if (VMENTRY_INJECTING_EVENT(vm->vmentry_interr_info)) {
        unsigned event_type = (vm->vmentry_interr_info >> 8) & 7;
        unsigned vector = vm->vmentry_interr_info & 0xff;
        if (event_type == BX_EXTERNAL_INTERRUPT) {
            if ((guest.interruptibility_state & 0x3) != 0 || (guest.rflags & EFlagsIFMask) == 0) {
                BX_ERROR(("VMENTER FAIL: VMCS guest interrupts blocked when injecting external interrupt"));
                return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
            }
        }
        if (event_type == BX_NMI) {
            if ((guest.interruptibility_state & 0x3) != 0) {
                BX_ERROR(("VMENTER FAIL: VMCS guest interrupts blocked when injecting NMI"));
                return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
            }
        }
        if (guest.activity_state == BX_ACTIVITY_STATE_WAIT_FOR_SIPI) {
            BX_ERROR(("VMENTER FAIL: No guest interruptions are allowed when entering Wait-For-Sipi state"));
            return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }
        if (guest.activity_state == BX_ACTIVITY_STATE_SHUTDOWN && event_type != BX_NMI && vector != BX_MC_EXCEPTION) {
            BX_ERROR(("VMENTER FAIL: Only NMI or #MC guest interruption is allowed when entering shutdown state"));
            return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }
    }

    if (vm->vmentry_ctrls.SMM_ENTER()) {
        if (!(guest.interruptibility_state & BX_VMX_INTERRUPTS_BLOCKED_SMI_BLOCKED)) {
            BX_ERROR(("VMENTER FAIL: VMCS SMM guest should block SMI"));
            return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }

        if (guest.activity_state == BX_ACTIVITY_STATE_WAIT_FOR_SIPI) {
            BX_ERROR(("VMENTER FAIL: The activity state must not indicate the wait-for-SIPI state if entering to SMM guest"));
            return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }
    }

    if (guest.interruptibility_state & BX_VMX_INTERRUPTS_BLOCKED_SMI_BLOCKED) {
        if (!BX_CPU_THIS_PTR in_smm) {
            BX_ERROR(("VMENTER FAIL: VMCS SMI blocked when not in SMM mode"));
            return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }
    }

    if (!x86_64_guest && (guest.cr4 & BX_CR4_PAE_MASK) != 0 && (guest.cr0 & BX_CR0_PG_MASK) != 0) {
#if BX_SUPPORT_VMX >= 2
        if (vm->vmexec_ctrls2.EPT_ENABLE()) {
            for (n = 0; n < 4; n++)
                guest.pdptr[n] = VMread64(VMCS_64BIT_GUEST_IA32_PDPTE0 + 2 * n);

            if (!CheckPDPTR(guest.pdptr)) {
                *qualification = VMENTER_ERR_GUEST_STATE_PDPTR_LOADING;
                BX_ERROR(("VMENTER: EPT Guest State PDPTRs Checks Failed"));
                return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
            }
        }
        else
#endif
        {
            if (!CheckPDPTR(guest.cr3)) {
                *qualification = VMENTER_ERR_GUEST_STATE_PDPTR_LOADING;
                BX_ERROR(("VMENTER: Guest State PDPTRs Checks Failed"));
                return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
            }
        }
    }

    //
    // Load Guest State -> VMENTER
    //

#if BX_SUPPORT_X86_64
#if BX_SUPPORT_VMX >= 2
  // modify EFER.LMA / EFER.LME before setting CR4

  // It is recommended that 64-bit VMM software use the 1-settings of the "load IA32_EFER"
  // VM entry control and the "save IA32_EFER" VM-exit control. If VMentry is establishing
  // CR0.PG=0 and if the "IA-32e mode guest" and "load IA32_EFER" VM entry controls are
  // both 0, VM entry leaves IA32_EFER.LME unmodified (i.e., the host value will persist
  // in the guest) -- Quote from Intel SDM
    if (vm->vmentry_ctrls.LOAD_EFER_MSR()) {
        BX_CPU_THIS_PTR efer.set32((Bit32u)guest.efer_msr);
    }
    else
#endif
    {
        if (x86_64_guest) {
            BX_CPU_THIS_PTR efer.set32(BX_CPU_THIS_PTR efer.get32() | (BX_EFER_LME_MASK | BX_EFER_LMA_MASK));
        }
        else {
            // when loading unrestricted guest with CR0.PG=0 EFER.LME is unmodified
            // (i.e., the host value will persist in the guest)
            if (guest.cr0 & BX_CR0_PG_MASK)
                BX_CPU_THIS_PTR efer.set32(BX_CPU_THIS_PTR efer.get32() & ~(BX_EFER_LME_MASK | BX_EFER_LMA_MASK));
            else
                BX_CPU_THIS_PTR efer.set32(BX_CPU_THIS_PTR efer.get32() & ~BX_EFER_LMA_MASK);
        }
    }
#endif

    // keep bits ET(4), reserved bits 15:6, 17, 28:19, NW(29), CD(30)
#define VMX_KEEP_CR0_BITS 0x7FFAFFD0

    guest.cr0 = (BX_CPU_THIS_PTR cr0.get32() & VMX_KEEP_CR0_BITS) | (guest.cr0 & ~VMX_KEEP_CR0_BITS);

    if (!check_CR0(guest.cr0)) {
        BX_PANIC(("VMENTER CR0 is broken !"));
    }
    if (!check_CR4(guest.cr4)) {
        BX_PANIC(("VMENTER CR4 is broken !"));
    }

    BX_CPU_THIS_PTR cr0.set32((Bit32u)guest.cr0);
    BX_CPU_THIS_PTR cr4.set32((Bit32u)guest.cr4);
    BX_CPU_THIS_PTR cr3 = guest.cr3;

#if BX_SUPPORT_VMX >= 2
    if (vm->vmexec_ctrls2.EPT_ENABLE()) {
        // load PDPTR only in PAE legacy mode
        if (BX_CPU_THIS_PTR cr0.get_PG() && BX_CPU_THIS_PTR cr4.get_PAE() && !x86_64_guest) {
            for (n = 0; n < 4; n++)
                BX_CPU_THIS_PTR PDPTR_CACHE.entry[n] = guest.pdptr[n];
        }
    }
#endif

    if (vm->vmentry_ctrls.LOAD_DBG_CTRLS()) {
        // always clear bits 15:14 and set bit 10
        BX_CPU_THIS_PTR dr7.set32((guest.dr7 & ~0xc000) | 0x400);
    }

    RIP = BX_CPU_THIS_PTR prev_rip = guest.rip;
    RSP = guest.rsp;

#if BX_SUPPORT_CET
    if (vm->vmentry_ctrls.LOAD_GUEST_CET_STATE()) {
        SSP = guest.ssp;
        BX_CPU_THIS_PTR msr.ia32_interrupt_ssp_table = guest.interrupt_ssp_table_address;
        BX_CPU_THIS_PTR msr.ia32_cet_control[0] = guest.msr_ia32_s_cet;
    }
#endif

#if BX_SUPPORT_PKEYS
    if (vm->vmentry_ctrls.LOAD_GUEST_PKRS()) {
        set_PKeys(BX_CPU_THIS_PTR pkru, guest.pkrs);
    }
#endif

    BX_CPU_THIS_PTR async_event = 0;

    setEFlags((Bit32u)guest.rflags);

#ifdef BX_SUPPORT_CS_LIMIT_DEMOTION
    // Handle special case of CS.LIMIT demotion (new descriptor limit is
    // smaller than current one)
    if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled > guest.sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled)
        BX_CPU_THIS_PTR iCache.flushICacheEntries();
#endif

    for (unsigned segreg = 0; segreg < 6; segreg++)
        BX_CPU_THIS_PTR sregs[segreg] = guest.sregs[segreg];

    // SS.DPL is always loaded from the SS access-rights field. This will be the current privilege level (CPL) after the VM entry completes.
    CPL = guest.sregs[BX_SEG_REG_SS].cache.dpl;

    BX_CPU_THIS_PTR gdtr.base = gdtr_base;
    BX_CPU_THIS_PTR gdtr.limit = gdtr_limit;
    BX_CPU_THIS_PTR idtr.base = idtr_base;
    BX_CPU_THIS_PTR idtr.limit = idtr_limit;

    BX_CPU_THIS_PTR ldtr = guest.ldtr;
    BX_CPU_THIS_PTR tr = guest.tr;

    BX_CPU_THIS_PTR msr.sysenter_esp_msr = guest.sysenter_esp_msr;
    BX_CPU_THIS_PTR msr.sysenter_eip_msr = guest.sysenter_eip_msr;
    BX_CPU_THIS_PTR msr.sysenter_cs_msr = guest.sysenter_cs_msr;

#if BX_SUPPORT_UINTR
    if (vm->vmentry_ctrls.LOAD_UINV()) {
        BX_CPU_THIS_PTR uintr.uinv = guest.uintr_uinv;
    }
#endif

#if BX_SUPPORT_VMX >= 2
    if (vm->vmentry_ctrls.LOAD_PAT_MSR()) {
        BX_CPU_THIS_PTR msr.pat = guest.pat_msr;
    }

    if (vm->vmentry_ctrls.LOAD_GUEST_IA32_SPEC_CTRL()) {
        BX_CPU_THIS_PTR msr.ia32_spec_ctrl = guest.ia32_spec_ctrl_msr;
    }
#endif

#if BX_SUPPORT_VMX >= 2
    vm->ple.last_pause_time = vm->ple.first_pause_time = 0;
#endif

    //
    // Load Guest Non-Registers State -> VMENTER
    //

    if (vm->vmentry_ctrls.SMM_ENTER())
        BX_PANIC(("VMENTER: entry to SMM is not implemented yet !"));

    if (VMENTRY_INJECTING_EVENT(vm->vmentry_interr_info)) {
        // the VMENTRY injecting event to the guest
        BX_CPU_THIS_PTR inhibit_mask = 0; // do not block interrupts
        BX_CPU_THIS_PTR debug_trap = 0;
        guest.activity_state = BX_ACTIVITY_STATE_ACTIVE;
    }
    else {
        if (guest.tmpDR6 & (1 << 12))
            BX_CPU_THIS_PTR debug_trap = guest.tmpDR6 & 0x0000400F;
        else
            BX_CPU_THIS_PTR debug_trap = guest.tmpDR6 & 0x00004000;
        if (BX_CPU_THIS_PTR debug_trap) {
            BX_CPU_THIS_PTR debug_trap |= BX_DEBUG_TRAP_HIT;
            BX_CPU_THIS_PTR async_event = 1;
        }

        if (guest.interruptibility_state & BX_VMX_INTERRUPTS_BLOCKED_BY_STI)
            inhibit_interrupts(BX_INHIBIT_INTERRUPTS);
        else if (guest.interruptibility_state & BX_VMX_INTERRUPTS_BLOCKED_BY_MOV_SS)
            inhibit_interrupts(BX_INHIBIT_INTERRUPTS_BY_MOVSS);
        else
            BX_CPU_THIS_PTR inhibit_mask = 0;
    }

    unmask_event(BX_EVENT_VMX_VIRTUAL_NMI | BX_EVENT_NMI);
    if (guest.interruptibility_state & BX_VMX_INTERRUPTS_BLOCKED_NMI_BLOCKED) {
        if (vm->pin_vmexec_ctrls.VIRTUAL_NMI())
            mask_event(BX_EVENT_VMX_VIRTUAL_NMI);
        else
            mask_event(BX_EVENT_NMI);
    }

    if (vm->vmexec_ctrls1.MONITOR_TRAP_FLAG()) {
        signal_event(BX_EVENT_VMX_MONITOR_TRAP_FLAG);
        mask_event(BX_EVENT_VMX_MONITOR_TRAP_FLAG);
        BX_CPU_THIS_PTR async_event = 1;
    }

    if (vm->vmexec_ctrls1.NMI_WINDOW_EXITING())
        signal_event(BX_EVENT_VMX_VIRTUAL_NMI);

    if (vm->vmexec_ctrls1.INTERRUPT_WINDOW_VMEXIT())
        signal_event(BX_EVENT_VMX_INTERRUPT_WINDOW_EXITING);

    handleCpuContextChange();

#if BX_SUPPORT_MONITOR_MWAIT
    BX_CPU_THIS_PTR monitor.reset_monitor();
#endif

    BX_INSTR_TLB_CNTRL(BX_CPU_ID, BX_INSTR_CONTEXT_SWITCH, 0);

    if (guest.activity_state) {
        BX_DEBUG(("VMEntry to non-active CPU state %d", guest.activity_state));
        enter_sleep_state(guest.activity_state);
    }

    return VMXERR_NO_ERROR;
    */
return 0; //自己加的，源码没有这一行，原因是这里被我注释但必须要一个返回值
}

Bit32u BX_CPU_C::LoadMSRs(Bit32u msr_cnt, bx_phy_address pAddr)
{
    //2337
    /*
    Bit64u msr_lo, msr_hi;

    for (Bit32u msr = 1; msr <= msr_cnt; msr++) {
        msr_lo = read_physical_qword(pAddr, MEMTYPE(resolve_memtype(pAddr)), BX_VMX_LOAD_MSR_ACCESS);
        msr_hi = read_physical_qword(pAddr + 8, MEMTYPE(resolve_memtype(pAddr)), BX_VMX_LOAD_MSR_ACCESS);
        pAddr += 16; // to next MSR

        if (GET32H(msr_lo)) {
            //BX_ERROR(("VMX LoadMSRs %d: broken msr index 0x" FMT_LL "x", msr, msr_lo));
            return msr;
        }

        Bit32u index = GET32L(msr_lo);

#if BX_SUPPORT_X86_64
        if (index == BX_MSR_FSBASE || index == BX_MSR_GSBASE) {
            //BX_ERROR(("VMX LoadMSRs %d: unable to restore FSBASE or GSBASE", msr));
            return msr;
        }
#endif

        if (is_x2apic_msr_range(index)) {
            //BX_ERROR(("VMX LoadMSRs %d: unable to restore X2APIC range MSR %x", msr, index));
            return msr;
        }

        if (!wrmsr(index, msr_hi)) {
            //BX_ERROR(("VMX LoadMSRs %d: unable to set up MSR %x", msr, index));
            return msr;
        }
    }

    return 0;
    */
    return 0; //自己加的，源码没有这一行，原因是这里被我注释但必须要一个返回值
}

Bit32u BX_CPU_C::StoreMSRs(Bit32u msr_cnt, bx_phy_address pAddr)
{
    //2374
    /*
    Bit64u msr_hi;

    for (Bit32u msr = 1; msr <= msr_cnt; msr++) {
        Bit64u msr_lo = read_physical_qword(pAddr, MEMTYPE(resolve_memtype(pAddr)), BX_VMX_STORE_MSR_ACCESS);
        if (GET32H(msr_lo)) {
            //BX_ERROR(("VMX StoreMSRs %d: broken msr index 0x" FMT_LL "x", msr, msr_lo));
            return msr;
        }

        Bit32u index = GET32L(msr_lo);

        if (is_x2apic_msr_range(index)) {
            //BX_ERROR(("VMX StoreMSRs %d: unable to save X2APIC range MSR %x", msr, index));
            return msr;
        }

        if (!rdmsr(index, &msr_hi)) {
            //BX_ERROR(("VMX StoreMSRs %d: unable to read MSR %x", msr, index));
            return msr;
        }

        write_physical_qword(pAddr + 8, msr_hi, MEMTYPE(resolve_memtype(pAddr)), BX_VMX_STORE_MSR_ACCESS);

        pAddr += 16; // to next MSR
    }

    return 0;
    */
    return 0; //自己加的，源码没有这一行，原因是这里被我注释但必须要一个返回值
}

void BX_CPU_C::VMexitSaveGuestState(Bit32u reason, Bit32u vector)
{
    //2450
    /*
    VMCS_CACHE* vm = &BX_CPU_THIS_PTR vmcs;
    int n;

    VMwrite_natural(VMCS_GUEST_CR0, BX_CPU_THIS_PTR cr0.get32());
    VMwrite_natural(VMCS_GUEST_CR3, BX_CPU_THIS_PTR cr3);
    VMwrite_natural(VMCS_GUEST_CR4, BX_CPU_THIS_PTR cr4.get32());

#if BX_SUPPORT_VMX >= 2
    if (vm->vmexec_ctrls2.EPT_ENABLE()) {
        // save only if guest running in legacy PAE mode
        if (BX_CPU_THIS_PTR cr0.get_PG() && BX_CPU_THIS_PTR cr4.get_PAE() && !long_mode()) {
            for (n = 0; n < 4; n++) {
                VMwrite64(VMCS_64BIT_GUEST_IA32_PDPTE0 + 2 * n, BX_CPU_THIS_PTR PDPTR_CACHE.entry[n]);
            }
        }
    }
#endif

    if (vm->vmexit_ctrls1.SAVE_DBG_CTRLS())
        VMwrite_natural(VMCS_GUEST_DR7, BX_CPU_THIS_PTR dr7.get32());

    VMwrite_natural(VMCS_GUEST_RIP, RIP);
    VMwrite_natural(VMCS_GUEST_RSP, RSP);
    VMwrite_natural(VMCS_GUEST_RFLAGS, VMexitReadEFLAGS(reason, vector));

#if BX_SUPPORT_CET
    if (BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_CET)) {
        VMwrite_natural(VMCS_GUEST_IA32_S_CET, BX_CPU_THIS_PTR msr.ia32_cet_control[0]);
        VMwrite_natural(VMCS_GUEST_INTERRUPT_SSP_TABLE_ADDR, BX_CPU_THIS_PTR msr.ia32_interrupt_ssp_table);
        VMwrite_natural(VMCS_GUEST_SSP, SSP);
    }
#endif

#if BX_SUPPORT_UINTR
    if (BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_UINTR)) {
        VMwrite16(VMCS_16BIT_GUEST_UINV, BX_CPU_THIS_PTR uintr.uinv);
    }
#endif

#if BX_SUPPORT_PKEYS
    if (BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_PKS)) {
        VMwrite64(VMCS_64BIT_GUEST_IA32_PKRS, BX_CPU_THIS_PTR pkrs);
    }
#endif

    for (n = 0; n < 6; n++) {
        Bit32u selector = BX_CPU_THIS_PTR sregs[n].selector.value;
        bool invalid = !BX_CPU_THIS_PTR sregs[n].cache.valid;
        bx_address base = BX_CPU_THIS_PTR sregs[n].cache.u.segment.base;
        Bit32u limit = BX_CPU_THIS_PTR sregs[n].cache.u.segment.limit_scaled;
        Bit32u ar = (get_descriptor_h(&BX_CPU_THIS_PTR sregs[n].cache) & 0x00f0ff00) >> 8;
        ar = vmx_pack_ar_field(ar | (invalid << 16), BX_CPU_THIS_PTR vmcs_map->get_access_rights_format());

        VMwrite16(VMCS_16BIT_GUEST_ES_SELECTOR + 2 * n, selector);
        VMwrite32(VMCS_32BIT_GUEST_ES_ACCESS_RIGHTS + 2 * n, ar);
        VMwrite_natural(VMCS_GUEST_ES_BASE + 2 * n, base);
        VMwrite32(VMCS_32BIT_GUEST_ES_LIMIT + 2 * n, limit);
    }

    // save guest LDTR
    Bit32u ldtr_selector = BX_CPU_THIS_PTR ldtr.selector.value;
    bool ldtr_invalid = !BX_CPU_THIS_PTR ldtr.cache.valid;
    bx_address ldtr_base = BX_CPU_THIS_PTR ldtr.cache.u.segment.base;
    Bit32u ldtr_limit = BX_CPU_THIS_PTR ldtr.cache.u.segment.limit_scaled;
    Bit32u ldtr_ar = (get_descriptor_h(&BX_CPU_THIS_PTR ldtr.cache) & 0x00f0ff00) >> 8;
    ldtr_ar = vmx_pack_ar_field(ldtr_ar | (ldtr_invalid << 16), BX_CPU_THIS_PTR vmcs_map->get_access_rights_format());

    VMwrite16(VMCS_16BIT_GUEST_LDTR_SELECTOR, ldtr_selector);
    VMwrite32(VMCS_32BIT_GUEST_LDTR_ACCESS_RIGHTS, ldtr_ar);
    VMwrite_natural(VMCS_GUEST_LDTR_BASE, ldtr_base);
    VMwrite32(VMCS_32BIT_GUEST_LDTR_LIMIT, ldtr_limit);

    // save guest TR
    Bit32u tr_selector = BX_CPU_THIS_PTR tr.selector.value;
    bool tr_invalid = !BX_CPU_THIS_PTR tr.cache.valid;
    bx_address tr_base = BX_CPU_THIS_PTR tr.cache.u.segment.base;
    Bit32u tr_limit = BX_CPU_THIS_PTR tr.cache.u.segment.limit_scaled;
    Bit32u tr_ar = (get_descriptor_h(&BX_CPU_THIS_PTR tr.cache) & 0x00f0ff00) >> 8;
    tr_ar = vmx_pack_ar_field(tr_ar | (tr_invalid << 16), BX_CPU_THIS_PTR vmcs_map->get_access_rights_format());

    VMwrite16(VMCS_16BIT_GUEST_TR_SELECTOR, tr_selector);
    VMwrite32(VMCS_32BIT_GUEST_TR_ACCESS_RIGHTS, tr_ar);
    VMwrite_natural(VMCS_GUEST_TR_BASE, tr_base);
    VMwrite32(VMCS_32BIT_GUEST_TR_LIMIT, tr_limit);

    VMwrite_natural(VMCS_GUEST_GDTR_BASE, BX_CPU_THIS_PTR gdtr.base);
    VMwrite32(VMCS_32BIT_GUEST_GDTR_LIMIT, BX_CPU_THIS_PTR gdtr.limit);
    VMwrite_natural(VMCS_GUEST_IDTR_BASE, BX_CPU_THIS_PTR idtr.base);
    VMwrite32(VMCS_32BIT_GUEST_IDTR_LIMIT, BX_CPU_THIS_PTR idtr.limit);

    VMwrite_natural(VMCS_GUEST_IA32_SYSENTER_ESP_MSR, BX_CPU_THIS_PTR msr.sysenter_esp_msr);
    VMwrite_natural(VMCS_GUEST_IA32_SYSENTER_EIP_MSR, BX_CPU_THIS_PTR msr.sysenter_eip_msr);
    VMwrite32(VMCS_32BIT_GUEST_IA32_SYSENTER_CS_MSR, BX_CPU_THIS_PTR msr.sysenter_cs_msr);

#if BX_SUPPORT_VMX >= 2
    if (vm->vmexit_ctrls1.STORE_PAT_MSR())
        VMwrite64(VMCS_64BIT_GUEST_IA32_PAT, BX_CPU_THIS_PTR msr.pat.u64);
#if BX_SUPPORT_X86_64
    if (vm->vmexit_ctrls1.STORE_EFER_MSR())
        VMwrite64(VMCS_64BIT_GUEST_IA32_EFER, BX_CPU_THIS_PTR efer.get32());
#endif
#endif

    if (vm->vmexec_ctrls3.VIRTUALIZE_IA32_SPEC_CTRL())
        VMwrite64(VMCS_64BIT_CONTROL_IA32_SPEC_CTRL_SHADOW, vm->ia32_spec_ctrl_shadow);

    // The pending debug exceptions field is saved as *clear* for all VM exits except the following:
    //   - VMexit caused by an INIT signal, a machine-check exception, or a SMI
    //   - Trap like VMexit ["TPR below threshold", "Virtualized EOI", "APIC write", "MTF" or "Bus-lock detected"]
    //   - VM exits that are not caused by Debug Exceptions and occur while there is MOV-SS blocking of debug exceptions
    bool clear_tmpDR6 = !interrupts_inhibited(BX_INHIBIT_DEBUG);
    switch (reason) {
    case VMX_VMEXIT_INIT:
    case VMX_VMEXIT_SMI:
    case VMX_VMEXIT_TPR_THRESHOLD:
    case VMX_VMEXIT_VIRTUALIZED_EOI:
    case VMX_VMEXIT_APIC_WRITE:
    case VMX_VMEXIT_BUS_LOCK:
    case VMX_VMEXIT_MONITOR_TRAP_FLAG:
        clear_tmpDR6 = false;
        break;
    default:
        break;
    }

    if (clear_tmpDR6) {
        VMwrite_natural(VMCS_GUEST_PENDING_DBG_EXCEPTIONS, 0);
    }
    else {
        Bit32u tmpDR6 = BX_CPU_THIS_PTR debug_trap & 0x0000400f;
        if (tmpDR6 & 0xf) tmpDR6 |= (1 << 12);
        VMwrite_natural(VMCS_GUEST_PENDING_DBG_EXCEPTIONS, tmpDR6);
    }

    // effectively wakeup from MWAIT state on VMEXIT
    if (BX_CPU_THIS_PTR activity_state >= BX_VMX_LAST_ACTIVITY_STATE)
        VMwrite32(VMCS_32BIT_GUEST_ACTIVITY_STATE, BX_ACTIVITY_STATE_ACTIVE);
    else
        VMwrite32(VMCS_32BIT_GUEST_ACTIVITY_STATE, BX_CPU_THIS_PTR activity_state);

    Bit32u interruptibility_state = 0;
    if (interrupts_inhibited(BX_INHIBIT_INTERRUPTS)) {
        if (interrupts_inhibited(BX_INHIBIT_DEBUG))
            interruptibility_state |= BX_VMX_INTERRUPTS_BLOCKED_BY_MOV_SS;
        else
            interruptibility_state |= BX_VMX_INTERRUPTS_BLOCKED_BY_STI;
    }

    // Do not set BX_VMX_INTERRUPTS_BLOCKED_SMI_BLOCKED (as the dual-monitor
    // treatment is unimplemented).
    // "VM exits that end outside system-management mode (SMM) save bit 2 (blocking by SMI)
    //  as 0 regardless of the state of such blocking before the VM exit."

    if (vm->pin_vmexec_ctrls.VIRTUAL_NMI()) {
        if (is_masked_event(BX_EVENT_VMX_VIRTUAL_NMI))
            interruptibility_state |= BX_VMX_INTERRUPTS_BLOCKED_NMI_BLOCKED;
    }
    else {
        if (is_masked_event(BX_EVENT_NMI))
            interruptibility_state |= BX_VMX_INTERRUPTS_BLOCKED_NMI_BLOCKED;
    }

    VMwrite32(VMCS_32BIT_GUEST_INTERRUPTIBILITY_STATE, interruptibility_state);

#if BX_SUPPORT_VMX >= 2
    if (VMX_MSR_MISC & VMX_MISC_STORE_LMA_TO_X86_64_GUEST_VMENTRY_CONTROL) {
        // VMEXITs store the value of EFER.LMA into the x86-64 guest VMENTRY control
        // must be set if unrestricted guest is supported
        if (long_mode())
            vm->vmentry_ctrls.set(VMX_VMENTRY_CTRL1_X86_64_GUEST);
        else
            vm->vmentry_ctrls.clear(VMX_VMENTRY_CTRL1_X86_64_GUEST);

        VMwrite32(VMCS_32BIT_CONTROL_VMENTRY_CONTROLS, vm->vmentry_ctrls.get());
    }

    // Deactivate VMX preemtion timer
    BX_CPU_THIS_PTR lapic->deactivate_vmx_preemption_timer();
    clear_event(BX_EVENT_VMX_PREEMPTION_TIMER_EXPIRED);
    // Store back to VMCS
    if (vm->vmexit_ctrls1.STORE_VMX_PREEMPTION_TIMER())
        VMwrite32(VMCS_32BIT_GUEST_PREEMPTION_TIMER_VALUE, BX_CPU_THIS_PTR lapic->read_vmx_preemption_timer());

    if (vm->vmexec_ctrls2.VIRTUAL_INT_DELIVERY()) {
        VMwrite16(VMCS_16BIT_GUEST_INTERRUPT_STATUS, (((Bit16u)vm->svi) << 8) | vm->rvi);
    }

    if (vm->vmexec_ctrls2.PML_ENABLE()) {
        VMwrite16(VMCS_16BIT_GUEST_PML_INDEX, vm->pml_index);
    }
#endif
    */
}

void BX_CPU_C::VMexitLoadHostState(void)
{
    /*
    //2645
    VMCS_CACHE* vm = &BX_CPU_THIS_PTR vmcs;
    VMCS_HOST_STATE* host_state = &BX_CPU_THIS_PTR vmcs.host_state;
    BX_CPU_THIS_PTR tsc_offset = 0;

    bool x86_64_host = vm->vmexit_ctrls1.X86_64_HOST();
#if BX_SUPPORT_X86_64
    if (x86_64_host)
        //BX_DEBUG(("VMEXIT to x86-64 host"));

#if BX_SUPPORT_VMX >= 2
    // modify EFER.LMA / EFER.LME before setting CR4
    if (vm->vmexit_ctrls1.LOAD_EFER_MSR()) {
        BX_CPU_THIS_PTR efer.set32((Bit32u)host_state->efer_msr);
    }
    else
#endif
    {
        if (x86_64_host)
            BX_CPU_THIS_PTR efer.set32(BX_CPU_THIS_PTR efer.get32() | (BX_EFER_LME_MASK | BX_EFER_LMA_MASK));
        else
            BX_CPU_THIS_PTR efer.set32(BX_CPU_THIS_PTR efer.get32() & ~(BX_EFER_LME_MASK | BX_EFER_LMA_MASK));
    }
#endif

    // ET, CD, NW, 28:19, 17, 15:6, and VMX fixed bits not modified Section 19.8
    host_state->cr0 = (BX_CPU_THIS_PTR cr0.get32() & VMX_KEEP_CR0_BITS) | (host_state->cr0 & ~VMX_KEEP_CR0_BITS);

    if (!check_CR0(host_state->cr0)) {
        //BX_PANIC(("VMEXIT CR0 is broken !"));
    }
    if (!check_CR4(host_state->cr4)) {
        //BX_PANIC(("VMEXIT CR4 is broken !"));
    }

    BX_CPU_THIS_PTR cr0.set32((Bit32u)host_state->cr0);
    BX_CPU_THIS_PTR cr4.set32((Bit32u)host_state->cr4);
    BX_CPU_THIS_PTR cr3 = host_state->cr3;

    if (!x86_64_host && BX_CPU_THIS_PTR cr4.get_PAE()) {
        if (!CheckPDPTR(host_state->cr3)) {
            //BX_ERROR(("VMABORT: host PDPTRs are corrupted !"));
            VMabort(VMABORT_HOST_PDPTR_CORRUPTED);
        }
    }

    BX_CPU_THIS_PTR dr7.set32(0x00000400);

    BX_CPU_THIS_PTR msr.sysenter_cs_msr = host_state->sysenter_cs_msr;
    BX_CPU_THIS_PTR msr.sysenter_esp_msr = host_state->sysenter_esp_msr;
    BX_CPU_THIS_PTR msr.sysenter_eip_msr = host_state->sysenter_eip_msr;

#if BX_SUPPORT_VMX >= 2
    if (vm->vmexit_ctrls1.LOAD_PAT_MSR()) {
        BX_CPU_THIS_PTR msr.pat = host_state->pat_msr;
    }
    if (vm->vmexit_ctrls2.LOAD_HOST_IA32_SPEC_CTRL()) {
        BX_CPU_THIS_PTR msr.ia32_spec_ctrl = host_state->ia32_spec_ctrl_msr;
    }
#endif

    // CS selector loaded from VMCS
    //    valid   <= 1
    //    base    <= 0
    //    limit   <= 0xffffffff, g <= 1
    //    present <= 1
    //    dpl     <= 0
    //    type    <= segment, BX_CODE_EXEC_READ_ACCESSED
    //    d_b     <= loaded from 'host-address space size' VMEXIT control
    //    l       <= loaded from 'host-address space size' VMEXIT control

    parse_selector(host_state->segreg_selector[BX_SEG_REG_CS],
        &BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector);

    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.valid = SegValidCache | SegAccessROK | SegAccessWOK | SegAccessROK4G | SegAccessWOK4G;
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.p = 1;
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.dpl = 0;
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.segment = 1;  
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.type = BX_CODE_EXEC_READ_ACCESSED;

    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.base = 0;
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled = 0xffffffff;
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.avl = 0;
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.g = 1; 
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.d_b = !x86_64_host;
#if BX_SUPPORT_X86_64
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.l = x86_64_host;
#endif

    // DATA selector loaded from VMCS
    //    valid   <= if selector is not all-zero
    //    base    <= 0
    //    limit   <= 0xffffffff, g <= 1
    //    present <= 1
    //    dpl     <= 0
    //    type    <= segment, BX_DATA_READ_WRITE_ACCESSED
    //    d_b     <= 1
    //    l       <= 0

    for (unsigned segreg = 0; segreg < 6; segreg++)
    {
        if (segreg == BX_SEG_REG_CS) continue;

        parse_selector(host_state->segreg_selector[segreg],
            &BX_CPU_THIS_PTR sregs[segreg].selector);

        if (!host_state->segreg_selector[segreg]) {
            BX_CPU_THIS_PTR sregs[segreg].cache.valid = 0;
        }
        else {
            BX_CPU_THIS_PTR sregs[segreg].cache.valid = SegValidCache;
            BX_CPU_THIS_PTR sregs[segreg].cache.p = 1;
            BX_CPU_THIS_PTR sregs[segreg].cache.dpl = 0;
            BX_CPU_THIS_PTR sregs[segreg].cache.segment = 1;  
            BX_CPU_THIS_PTR sregs[segreg].cache.type = BX_DATA_READ_WRITE_ACCESSED;
            BX_CPU_THIS_PTR sregs[segreg].cache.u.segment.base = 0;
            BX_CPU_THIS_PTR sregs[segreg].cache.u.segment.limit_scaled = 0xffffffff;
            BX_CPU_THIS_PTR sregs[segreg].cache.u.segment.avl = 0;
            BX_CPU_THIS_PTR sregs[segreg].cache.u.segment.g = 1; 
            BX_CPU_THIS_PTR sregs[segreg].cache.u.segment.d_b = 1;
#if BX_SUPPORT_X86_64
            BX_CPU_THIS_PTR sregs[segreg].cache.u.segment.l = 0;
#endif
        }
    }

    // SS.DPL always clear
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.dpl = 0;

    if (x86_64_host || BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.valid)
        BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.u.segment.base = host_state->fs_base;

    if (x86_64_host || BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.valid)
        BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.u.segment.base = host_state->gs_base;

    // TR selector loaded from VMCS
    parse_selector(host_state->tr_selector, &BX_CPU_THIS_PTR tr.selector);

    BX_CPU_THIS_PTR tr.cache.valid = SegValidCache; 
    BX_CPU_THIS_PTR tr.cache.p = 1; 
    BX_CPU_THIS_PTR tr.cache.dpl = 0; 
    BX_CPU_THIS_PTR tr.cache.segment = 0; 
    BX_CPU_THIS_PTR tr.cache.type = BX_SYS_SEGMENT_BUSY_386_TSS;
    BX_CPU_THIS_PTR tr.cache.u.segment.base = host_state->tr_base;
    BX_CPU_THIS_PTR tr.cache.u.segment.limit_scaled = 0x67;
    BX_CPU_THIS_PTR tr.cache.u.segment.avl = 0;
    BX_CPU_THIS_PTR tr.cache.u.segment.g = 0; 

    // unusable LDTR
    BX_CPU_THIS_PTR ldtr.selector.value = 0x0000;
    BX_CPU_THIS_PTR ldtr.selector.index = 0x0000;
    BX_CPU_THIS_PTR ldtr.selector.ti = 0;
    BX_CPU_THIS_PTR ldtr.selector.rpl = 0;
    BX_CPU_THIS_PTR ldtr.cache.valid = 0; 

    BX_CPU_THIS_PTR gdtr.base = host_state->gdtr_base;
    BX_CPU_THIS_PTR gdtr.limit = 0xFFFF;

    BX_CPU_THIS_PTR idtr.base = host_state->idtr_base;
    BX_CPU_THIS_PTR idtr.limit = 0xFFFF;

    RIP = BX_CPU_THIS_PTR prev_rip = host_state->rip;
    RSP = host_state->rsp;

#if BX_SUPPORT_CET
    if (vm->vmexit_ctrls1.LOAD_HOST_CET_STATE()) {
        SSP = host_state->ssp;
        BX_CPU_THIS_PTR msr.ia32_interrupt_ssp_table = host_state->interrupt_ssp_table_address;
        BX_CPU_THIS_PTR msr.ia32_cet_control[0] = host_state->msr_ia32_s_cet;
    }
#endif

#if BX_SUPPORT_UINTR
    if (vm->vmexit_ctrls1.CLEAR_UINV()) {
        BX_CPU_THIS_PTR uintr.uinv = 0;
    }
#endif

#if BX_SUPPORT_PKEYS
    if (vm->vmexit_ctrls1.LOAD_HOST_PKRS()) {
        set_PKeys(BX_CPU_THIS_PTR pkru, host_state->pkrs);
    }
#endif

    BX_CPU_THIS_PTR inhibit_mask = 0;
    BX_CPU_THIS_PTR debug_trap = 0;

    // set flags directly, avoid setEFlags side effects
    BX_CPU_THIS_PTR eflags = 0x2; // Bit1 is always set
    // Update lazy flags state
    clearEFlagsOSZAPC();

    BX_CPU_THIS_PTR activity_state = BX_ACTIVITY_STATE_ACTIVE;

    handleCpuContextChange();

#if BX_SUPPORT_MONITOR_MWAIT
    BX_CPU_THIS_PTR monitor.reset_monitor();
#endif

    BX_INSTR_TLB_CNTRL(BX_CPU_ID, BX_INSTR_CONTEXT_SWITCH, 0);

    */
}

void BX_CPU_C::VMexit(Bit32u reason, Bit64u qualification)
{
    
    //2849
    VMCS_CACHE* vm = &BX_CPU_THIS_PTR vmcs;

    if (!BX_CPU_THIS_PTR in_vmx || !BX_CPU_THIS_PTR in_vmx_guest) {
        if ((reason & 0x80000000) == 0)
        {//BX_PANIC(("PANIC: VMEXIT not in VMX guest mode !"));
        }

    }

    BX_INSTR_VMEXIT(BX_CPU_ID, reason, qualification);

    //
    // STEP 0: Update VMEXIT reason
    //

    // Extra vmexit reason bits:
    //    [25] Shadow stack prematurely busy
    //    [26] VMM bus lock detection (not implemented yet)
    //    [27] Enclave mode (not supported)
    //    [28] Pending MTF vmexit (only set by SMM VMEXIT which not implemented)
    //    [29] VmExit from VMX root operation (hannpen only in SMM VMEXIT which not implemented) 
#if BX_SUPPORT_CET
    if (vm->vmexit_ctrls2.SHADOW_STACK_PREMATURELY_BUSY_CTRL()) {
        if (vm->shadow_stack_prematurely_busy)
            reason |= (1 << 25);
    }
    vm->shadow_stack_prematurely_busy = false;
#endif

    VMwrite32(VMCS_32BIT_VMEXIT_REASON, reason);
    VMwrite_natural(VMCS_VMEXIT_QUALIFICATION, qualification);

    // clipping with 0xf not really necessary but keep it for safety
    VMwrite32(VMCS_32BIT_VMEXIT_INSTRUCTION_LENGTH, (RIP - BX_CPU_THIS_PTR prev_rip) & 0xf);

    reason &= 0xffff; /* keep only basic VMEXIT reason */

    if (reason >= VMX_VMEXIT_LAST_REASON)
    {//BX_PANIC(("PANIC: broken VMEXIT reason %d", reason));
    }
    else
    {//BX_DEBUG(("VMEXIT reason = %d (%s) qualification=0x" FMT_LL "x", reason, VMX_vmexit_reason_name[reason], qualification));
    }

    Bit32u vector = 0;
    if (reason == VMX_VMEXIT_EXCEPTION_NMI) {
        vector = VMread32(VMCS_32BIT_VMEXIT_INTERRUPTION_INFO);
        vector &= 0xFF;
    }

    if (vm->vmexec_ctrls3.ENABLE_MSRLIST()) {
        VMwrite64(VMCS_64BIT_MSR_DATA, ((reason == VMX_VMEXIT_WRMSRLIST) && vm->vmexec_ctrls1.MSR_BITMAPS()) ? vm->msr_data : 0);
        vm->msr_data = 0;
    }

    if (reason != VMX_VMEXIT_EXCEPTION_NMI && reason != VMX_VMEXIT_EXTERNAL_INTERRUPT) {
        VMwrite32(VMCS_32BIT_VMEXIT_INTERRUPTION_INFO, 0);
    }

    if (BX_CPU_THIS_PTR in_event) {
        VMwrite32(VMCS_32BIT_IDT_VECTORING_INFO, vm->idt_vector_info | 0x80000000);
        VMwrite32(VMCS_32BIT_IDT_VECTORING_ERR_CODE, vm->idt_vector_error_code);
        BX_CPU_THIS_PTR in_event = false;
    }
    else {
        VMwrite32(VMCS_32BIT_IDT_VECTORING_INFO, 0);
    }

    BX_CPU_THIS_PTR nmi_unblocking_iret = false;

    // VMEXITs are FAULT-like: restore RIP/RSP to value before VMEXIT occurred
    if (!IS_TRAP_LIKE_VMEXIT(reason)) {
        RIP = BX_CPU_THIS_PTR prev_rip;
        if (BX_CPU_THIS_PTR speculative_rsp) {
            RSP = BX_CPU_THIS_PTR prev_rsp;
#if BX_SUPPORT_CET
            SSP = BX_CPU_THIS_PTR prev_ssp;
#endif
        }
    }
    BX_CPU_THIS_PTR speculative_rsp = false;

    //
    // STEP 1: Saving Guest State to VMCS
    //
    if (reason != VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE && reason != VMX_VMEXIT_VMENTRY_FAILURE_MSR) {
        // clear VMENTRY interruption info field
        VMwrite32(VMCS_32BIT_CONTROL_VMENTRY_INTERRUPTION_INFO, vm->vmentry_interr_info & ~0x80000000);

        VMexitSaveGuestState(reason, vector);

        Bit32u msr = StoreMSRs(vm->vmexit_msr_store_cnt, vm->vmexit_msr_store_addr);
        if (msr) {
            //BX_ERROR(("VMABORT: Error when saving guest MSR number %d", msr));
            VMabort(VMABORT_SAVING_GUEST_MSRS_FAILURE);
        }
    }

    BX_CPU_THIS_PTR in_vmx_guest = false;

    // entering VMX root mode: clear possibly pending guest VMX events
    clear_event(BX_EVENT_VMX_VTPR_UPDATE |
        BX_EVENT_VMX_VEOI_UPDATE |
        BX_EVENT_VMX_VIRTUAL_APIC_WRITE |
        BX_EVENT_VMX_MONITOR_TRAP_FLAG |
        BX_EVENT_VMX_INTERRUPT_WINDOW_EXITING |
        BX_EVENT_VMX_PREEMPTION_TIMER_EXPIRED |
        BX_EVENT_VMX_VIRTUAL_NMI |
        BX_EVENT_PENDING_VMX_VIRTUAL_INTR);

    //
    // STEP 2: Load Host State
    //
    VMexitLoadHostState();

    //
    // STEP 3: Load Host MSR registers
    //

    Bit32u msr = LoadMSRs(vm->vmexit_msr_load_cnt, vm->vmexit_msr_load_addr);
    if (msr) {
        //BX_ERROR(("VMABORT: Error when loading host MSR number %d", msr));
        VMabort(VMABORT_LOADING_HOST_MSRS);
    }

    //
    // STEP 4: Go back to VMX host
    //

    mask_event(BX_EVENT_INIT); // INIT is disabled in VMX root mode
    if (reason == VMX_VMEXIT_EXCEPTION_NMI && vector == 2) {
        if (vector == 2) mask_event(BX_EVENT_NMI);
    }

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

    if (!IS_TRAP_LIKE_VMEXIT(reason)) {
        longjmp(BX_CPU_THIS_PTR jmp_buf_env, 1); // go back to main decode loop
    }
}


