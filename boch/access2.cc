#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

void BX_CPP_AttrRegparmN(3)
BX_CPU_C::write_linear_byte(unsigned s, bx_address laddr, Bit8u data)
{  //31
    bx_address lpf = LPFOf(laddr);
    bx_TLB_entry* tlbEntry = BX_DTLB_ENTRY_OF(laddr, 0);
    if (tlbEntry->lpf == lpf) {
        // See if the TLB entry privilege level allows us write access from this CPL
        if (isWriteOK(tlbEntry, USER_PL)) {
            bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
            Bit32u pageOffset = PAGE_OFFSET(laddr);
            bx_phy_address pAddr = tlbEntry->ppf | pageOffset;
            BX_NOTIFY_LIN_MEMORY_ACCESS(laddr, pAddr, 1, tlbEntry->get_memtype(), BX_WRITE, (Bit8u*)&data);
            Bit8u* hostAddr = (Bit8u*)(hostPageAddr | pageOffset);
            pageWriteStampTable.decWriteStamp(pAddr, 1);
            *hostAddr = data;
            return;
        }
    }

    if (access_write_linear(laddr, 1, CPL, BX_WRITE, 0x0, (void*)&data) < 0)
        exception(int_number(s), 0);
}

void BX_CPP_AttrRegparmN(3)
BX_CPU_C::write_linear_word(unsigned s, bx_address laddr, Bit16u data)
{
    bx_TLB_entry* tlbEntry = BX_DTLB_ENTRY_OF(laddr, 1);
#if BX_SUPPORT_ALIGNMENT_CHECK && BX_CPU_LEVEL >= 4
    bx_address lpf = AlignedAccessLPFOf(laddr, (1 & BX_CPU_THIS_PTR alignment_check_mask));
#else
    bx_address lpf = LPFOf(laddr);
#endif
    if (tlbEntry->lpf == lpf) {
        // See if the TLB entry privilege level allows us write access from this CPL
        if (isWriteOK(tlbEntry, USER_PL)) {
            bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
            Bit32u pageOffset = PAGE_OFFSET(laddr);
            bx_phy_address pAddr = tlbEntry->ppf | pageOffset;
            BX_NOTIFY_LIN_MEMORY_ACCESS(laddr, pAddr, 2, tlbEntry->get_memtype(), BX_WRITE, (Bit8u*)&data);
            Bit16u* hostAddr = (Bit16u*)(hostPageAddr | pageOffset);
            pageWriteStampTable.decWriteStamp(pAddr, 2);
            WriteHostWordToLittleEndian(hostAddr, data);
            return;
        }
    }

    if (access_write_linear(laddr, 2, CPL, BX_WRITE, 0x1, (void*)&data) < 0)
        exception(int_number(s), 0);
}

void BX_CPP_AttrRegparmN(3)
BX_CPU_C::write_linear_dword(unsigned s, bx_address laddr, Bit32u data)
{
    bx_TLB_entry* tlbEntry = BX_DTLB_ENTRY_OF(laddr, 3);
#if BX_SUPPORT_ALIGNMENT_CHECK && BX_CPU_LEVEL >= 4
    bx_address lpf = AlignedAccessLPFOf(laddr, (3 & BX_CPU_THIS_PTR alignment_check_mask));
#else
    bx_address lpf = LPFOf(laddr);
#endif
    if (tlbEntry->lpf == lpf) {
        // See if the TLB entry privilege level allows us write access from this CPL
        if (isWriteOK(tlbEntry, USER_PL)) {
            bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
            Bit32u pageOffset = PAGE_OFFSET(laddr);
            bx_phy_address pAddr = tlbEntry->ppf | pageOffset;
            BX_NOTIFY_LIN_MEMORY_ACCESS(laddr, pAddr, 4, tlbEntry->get_memtype(), BX_WRITE, (Bit8u*)&data);
            Bit32u* hostAddr = (Bit32u*)(hostPageAddr | pageOffset);
            pageWriteStampTable.decWriteStamp(pAddr, 4);
            WriteHostDWordToLittleEndian(hostAddr, data);
            return;
        }
    }

    if (access_write_linear(laddr, 4, CPL, BX_WRITE, 0x3, (void*)&data) < 0)
        exception(int_number(s), 0);
}

void BX_CPP_AttrRegparmN(3)
BX_CPU_C::write_linear_qword(unsigned s, bx_address laddr, Bit64u data)
{
    bx_TLB_entry* tlbEntry = BX_DTLB_ENTRY_OF(laddr, 7);
#if BX_SUPPORT_ALIGNMENT_CHECK && BX_CPU_LEVEL >= 4
    bx_address lpf = AlignedAccessLPFOf(laddr, (7 & BX_CPU_THIS_PTR alignment_check_mask));
#else
    bx_address lpf = LPFOf(laddr);
#endif
    if (tlbEntry->lpf == lpf) {
        // See if the TLB entry privilege level allows us write access from this CPL
        if (isWriteOK(tlbEntry, USER_PL)) {
            bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
            Bit32u pageOffset = PAGE_OFFSET(laddr);
            bx_phy_address pAddr = tlbEntry->ppf | pageOffset;
            BX_NOTIFY_LIN_MEMORY_ACCESS(laddr, pAddr, 8, tlbEntry->get_memtype(), BX_WRITE, (Bit8u*)&data);
            Bit64u* hostAddr = (Bit64u*)(hostPageAddr | pageOffset);
            pageWriteStampTable.decWriteStamp(pAddr, 8);
            WriteHostQWordToLittleEndian(hostAddr, data);
            return;
        }
    }

    if (access_write_linear(laddr, 8, CPL, BX_WRITE, 0x7, (void*)&data) < 0)
        exception(int_number(s), 0);
}


Bit8u BX_CPP_AttrRegparmN(2)
BX_CPU_C::read_linear_byte(unsigned s, bx_address laddr)
{
    Bit8u data;

    bx_address lpf = LPFOf(laddr);
    bx_TLB_entry* tlbEntry = BX_DTLB_ENTRY_OF(laddr, 0);
    if (tlbEntry->lpf == lpf) {
        // See if the TLB entry privilege level allows us read access from this CPL
        if (isReadOK(tlbEntry, USER_PL)) {
            bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
            Bit32u pageOffset = PAGE_OFFSET(laddr);
            Bit8u* hostAddr = (Bit8u*)(hostPageAddr | pageOffset);
            data = *hostAddr;
            BX_NOTIFY_LIN_MEMORY_ACCESS(laddr, (tlbEntry->ppf | pageOffset), 1, tlbEntry->get_memtype(), BX_READ, (Bit8u*)&data);
            return data;
        }
    }

    if (access_read_linear(laddr, 1, CPL, BX_READ, 0x0, (void*)&data) < 0)
        exception(int_number(s), 0);

    return data;
}

Bit16u BX_CPP_AttrRegparmN(2)
BX_CPU_C::read_linear_word(unsigned s, bx_address laddr)
{
    Bit16u data;

    bx_TLB_entry* tlbEntry = BX_DTLB_ENTRY_OF(laddr, 1);
#if BX_SUPPORT_ALIGNMENT_CHECK && BX_CPU_LEVEL >= 4
    bx_address lpf = AlignedAccessLPFOf(laddr, (1 & BX_CPU_THIS_PTR alignment_check_mask));
#else
    bx_address lpf = LPFOf(laddr);
#endif
    if (tlbEntry->lpf == lpf) {
        // See if the TLB entry privilege level allows us read access from this CPL
        if (isReadOK(tlbEntry, USER_PL)) {
            bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
            Bit32u pageOffset = PAGE_OFFSET(laddr);
            Bit16u* hostAddr = (Bit16u*)(hostPageAddr | pageOffset);
            data = ReadHostWordFromLittleEndian(hostAddr);
            BX_NOTIFY_LIN_MEMORY_ACCESS(laddr, (tlbEntry->ppf | pageOffset), 2, tlbEntry->get_memtype(), BX_READ, (Bit8u*)&data);
            return data;
        }
    }

    if (access_read_linear(laddr, 2, CPL, BX_READ, 0x1, (void*)&data) < 0)
        exception(int_number(s), 0);

    return data;
}

Bit32u BX_CPP_AttrRegparmN(2)
BX_CPU_C::read_linear_dword(unsigned s, bx_address laddr)
{
    Bit32u data;

    bx_TLB_entry* tlbEntry = BX_DTLB_ENTRY_OF(laddr, 3);
#if BX_SUPPORT_ALIGNMENT_CHECK && BX_CPU_LEVEL >= 4
    bx_address lpf = AlignedAccessLPFOf(laddr, (3 & BX_CPU_THIS_PTR alignment_check_mask));
#else
    bx_address lpf = LPFOf(laddr);
#endif
    if (tlbEntry->lpf == lpf) {
        // See if the TLB entry privilege level allows us read access from this CPL
        if (isReadOK(tlbEntry, USER_PL)) {
            bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
            Bit32u pageOffset = PAGE_OFFSET(laddr);
            Bit32u* hostAddr = (Bit32u*)(hostPageAddr | pageOffset);
            data = ReadHostDWordFromLittleEndian(hostAddr);
            BX_NOTIFY_LIN_MEMORY_ACCESS(laddr, (tlbEntry->ppf | pageOffset), 4, tlbEntry->get_memtype(), BX_READ, (Bit8u*)&data);
            return data;
        }
    }

    if (access_read_linear(laddr, 4, CPL, BX_READ, 0x3, (void*)&data) < 0)
        exception(int_number(s), 0);

    return data;
}

Bit64u BX_CPP_AttrRegparmN(2)
BX_CPU_C::read_linear_qword(unsigned s, bx_address laddr)
{
    Bit64u data;

    bx_TLB_entry* tlbEntry = BX_DTLB_ENTRY_OF(laddr, 7);
#if BX_SUPPORT_ALIGNMENT_CHECK && BX_CPU_LEVEL >= 4
    bx_address lpf = AlignedAccessLPFOf(laddr, (7 & BX_CPU_THIS_PTR alignment_check_mask));
#else
    bx_address lpf = LPFOf(laddr);
#endif
    if (tlbEntry->lpf == lpf) {
        // See if the TLB entry privilege level allows us read access from this CPL
        if (isReadOK(tlbEntry, USER_PL)) {
            bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
            Bit32u pageOffset = PAGE_OFFSET(laddr);
            Bit64u* hostAddr = (Bit64u*)(hostPageAddr | pageOffset);
            data = ReadHostQWordFromLittleEndian(hostAddr);
            BX_NOTIFY_LIN_MEMORY_ACCESS(laddr, (tlbEntry->ppf | pageOffset), 8, tlbEntry->get_memtype(), BX_READ, (Bit8u*)&data);
            return data;
        }
    }

    if (access_read_linear(laddr, 8, CPL, BX_READ, 0x7, (void*)&data) < 0)
        exception(int_number(s), 0);

    return data;
}

void BX_CPU_C::write_new_stack_word(bx_address laddr, unsigned curr_pl, Bit16u data)
{
    bool user = (curr_pl == 3);
    bx_TLB_entry* tlbEntry = BX_DTLB_ENTRY_OF(laddr, 1);
#if BX_SUPPORT_ALIGNMENT_CHECK && BX_CPU_LEVEL >= 4
    bx_address lpf = AlignedAccessLPFOf(laddr, (1 & BX_CPU_THIS_PTR alignment_check_mask));
#else
    bx_address lpf = LPFOf(laddr);
#endif
    if (tlbEntry->lpf == lpf) {
        // See if the TLB entry privilege level allows us write access from this CPL
        if (isWriteOK(tlbEntry, user)) {
            bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
            Bit32u pageOffset = PAGE_OFFSET(laddr);
            bx_phy_address pAddr = tlbEntry->ppf | pageOffset;
            BX_NOTIFY_LIN_MEMORY_ACCESS(laddr, pAddr, 2, tlbEntry->get_memtype(), BX_WRITE, (Bit8u*)&data);
            Bit16u* hostAddr = (Bit16u*)(hostPageAddr | pageOffset);
            pageWriteStampTable.decWriteStamp(pAddr, 2);
            WriteHostWordToLittleEndian(hostAddr, data);
            return;
        }
    }

    if (access_write_linear(laddr, 2, curr_pl, BX_WRITE, 0x1, (void*)&data) < 0)
        exception(BX_SS_EXCEPTION, 0);
}

void BX_CPU_C::write_new_stack_dword(bx_address laddr, unsigned curr_pl, Bit32u data)
{
    bool user = (curr_pl == 3);
    bx_TLB_entry* tlbEntry = BX_DTLB_ENTRY_OF(laddr, 3);
#if BX_SUPPORT_ALIGNMENT_CHECK && BX_CPU_LEVEL >= 4
    bx_address lpf = AlignedAccessLPFOf(laddr, (3 & BX_CPU_THIS_PTR alignment_check_mask));
#else
    bx_address lpf = LPFOf(laddr);
#endif
    if (tlbEntry->lpf == lpf) {
        // See if the TLB entry privilege level allows us write access from this CPL
        if (isWriteOK(tlbEntry, user)) {
            bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
            Bit32u pageOffset = PAGE_OFFSET(laddr);
            bx_phy_address pAddr = tlbEntry->ppf | pageOffset;
            BX_NOTIFY_LIN_MEMORY_ACCESS(laddr, pAddr, 4, tlbEntry->get_memtype(), BX_WRITE, (Bit8u*)&data);
            Bit32u* hostAddr = (Bit32u*)(hostPageAddr | pageOffset);
            pageWriteStampTable.decWriteStamp(pAddr, 4);
            WriteHostDWordToLittleEndian(hostAddr, data);
            return;
        }
    }

    if (access_write_linear(laddr, 4, curr_pl, BX_WRITE, 0x3, (void*)&data) < 0)
        exception(BX_SS_EXCEPTION, 0);
}

void BX_CPU_C::write_new_stack_qword(bx_address laddr, unsigned curr_pl, Bit64u data)
{
    bool user = (curr_pl == 3);
    bx_TLB_entry* tlbEntry = BX_DTLB_ENTRY_OF(laddr, 7);
#if BX_SUPPORT_ALIGNMENT_CHECK && BX_CPU_LEVEL >= 4
    bx_address lpf = AlignedAccessLPFOf(laddr, (7 & BX_CPU_THIS_PTR alignment_check_mask));
#else
    bx_address lpf = LPFOf(laddr);
#endif
    if (tlbEntry->lpf == lpf) {
        // See if the TLB entry privilege level allows us write access from this CPL
        if (isWriteOK(tlbEntry, user)) {
            bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
            Bit32u pageOffset = PAGE_OFFSET(laddr);
            bx_phy_address pAddr = tlbEntry->ppf | pageOffset;
            BX_NOTIFY_LIN_MEMORY_ACCESS(laddr, pAddr, 8, tlbEntry->get_memtype(), BX_WRITE, (Bit8u*)&data);
            Bit64u* hostAddr = (Bit64u*)(hostPageAddr | pageOffset);
            pageWriteStampTable.decWriteStamp(pAddr, 8);
            WriteHostQWordToLittleEndian(hostAddr, data);
            return;
        }
    }

    if (access_write_linear(laddr, 8, curr_pl, BX_WRITE, 0x7, (void*)&data) < 0)
        exception(BX_SS_EXCEPTION, 0);
}

void BX_CPU_C::write_new_stack_word(bx_segment_reg_t* seg, Bit32u offset, unsigned curr_pl, Bit16u data)
{
    Bit32u laddr;

    if (seg->cache.valid & SegAccessWOK4G) {
        goto accessOK;
    }

    if (seg->cache.valid & SegAccessWOK) {
        if (offset < seg->cache.u.segment.limit_scaled) {
        accessOK:
            laddr = (Bit32u)(seg->cache.u.segment.base) + offset;
            write_new_stack_word(laddr, curr_pl, data);
            return;
        }
    }

    // add error code when segment violation occurs when pushing into new stack
    if (!write_virtual_checks(seg, offset, 2)) {
        //BX_ERROR(("write_new_stack_word(): segment limit violation"));
        exception(BX_SS_EXCEPTION,
            seg->selector.rpl != CPL ? (seg->selector.value & 0xfffc) : 0);
    }
    goto accessOK;
}

void BX_CPU_C::write_new_stack_dword(bx_segment_reg_t* seg, Bit32u offset, unsigned curr_pl, Bit32u data)
{
    Bit32u laddr;

    if (seg->cache.valid & SegAccessWOK4G) {
        goto accessOK;
    }

    if (seg->cache.valid & SegAccessWOK) {
        if (offset < (seg->cache.u.segment.limit_scaled - 2)) {
        accessOK:
            laddr = (Bit32u)(seg->cache.u.segment.base) + offset;
            write_new_stack_dword(laddr, curr_pl, data);
            return;
        }
    }

    // add error code when segment violation occurs when pushing into new stack
    if (!write_virtual_checks(seg, offset, 4)) {
        //BX_ERROR(("write_new_stack_dword(): segment limit violation"));
        exception(BX_SS_EXCEPTION,
            seg->selector.rpl != CPL ? (seg->selector.value & 0xfffc) : 0);
    }
    goto accessOK;
}

void BX_CPU_C::write_new_stack_qword(bx_segment_reg_t* seg, Bit32u offset, unsigned curr_pl, Bit64u data)
{
    Bit32u laddr;

    if (seg->cache.valid & SegAccessWOK4G) {
        goto accessOK;
    }

    if (seg->cache.valid & SegAccessWOK) {
        if (offset <= (seg->cache.u.segment.limit_scaled - 7)) {
        accessOK:
            laddr = (Bit32u)(seg->cache.u.segment.base) + offset;
            write_new_stack_qword(laddr, curr_pl, data);
            return;
        }
    }

    // add error code when segment violation occurs when pushing into new stack
    if (!write_virtual_checks(seg, offset, 8)) {
        //BX_ERROR(("write_new_stack_qword(): segment limit violation"));
        exception(BX_SS_EXCEPTION,
            seg->selector.rpl != CPL ? (seg->selector.value & 0xfffc) : 0);
    }
    goto accessOK;
}

#if BX_SUPPORT_CET  //1126
Bit32u BX_CPP_AttrRegparmN(2) BX_CPU_C::shadow_stack_read_dword(bx_address offset, unsigned curr_pl)
{
    Bit32u data;

    bool user = (curr_pl == 3);
    bx_TLB_entry* tlbEntry = BX_DTLB_ENTRY_OF(offset, 3);
    bx_address lpf = AlignedAccessLPFOf(offset, 3);
    if (tlbEntry->lpf == lpf) {
        // See if the TLB entry privilege level allows us read access from this CPL
        if (isShadowStackReadOK(tlbEntry, user)) {
            bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
            Bit32u pageOffset = PAGE_OFFSET(offset);
            Bit32u* hostAddr = (Bit32u*)(hostPageAddr | pageOffset);
            data = ReadHostDWordFromLittleEndian(hostAddr);
            BX_NOTIFY_LIN_MEMORY_ACCESS(offset, (tlbEntry->ppf | pageOffset), 4, tlbEntry->get_memtype(), BX_SHADOW_STACK_READ, (Bit8u*)&data);
            return data;
        }
    }

    if (access_read_linear(offset, 4, curr_pl, BX_SHADOW_STACK_READ, 0, (void*)&data) < 0)
        exception(BX_GP_EXCEPTION, 0);

    return data;
}

Bit64u BX_CPP_AttrRegparmN(2) BX_CPU_C::shadow_stack_read_qword(bx_address offset, unsigned curr_pl)
{
    Bit64u data;

    bool user = (curr_pl == 3);
    bx_TLB_entry* tlbEntry = BX_DTLB_ENTRY_OF(offset, 7);
    bx_address lpf = AlignedAccessLPFOf(offset, 7);
    if (tlbEntry->lpf == lpf) {
        // See if the TLB entry privilege level allows us read access from this CPL
        if (isShadowStackReadOK(tlbEntry, user)) {
            bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
            Bit32u pageOffset = PAGE_OFFSET(offset);
            Bit64u* hostAddr = (Bit64u*)(hostPageAddr | pageOffset);
            data = ReadHostQWordFromLittleEndian(hostAddr);
            BX_NOTIFY_LIN_MEMORY_ACCESS(offset, (tlbEntry->ppf | pageOffset), 8, tlbEntry->get_memtype(), BX_SHADOW_STACK_READ, (Bit8u*)&data);
            return data;
        }
    }

    if (access_read_linear(offset, 8, curr_pl, BX_SHADOW_STACK_READ, 0, (void*)&data) < 0)
        exception(BX_GP_EXCEPTION, 0);

    return data;
}

void BX_CPP_AttrRegparmN(3) BX_CPU_C::shadow_stack_write_dword(bx_address offset, unsigned curr_pl, Bit32u data)
{  //1177
    bool user = (curr_pl == 3);
    bx_TLB_entry* tlbEntry = BX_DTLB_ENTRY_OF(offset, 3);
    bx_address lpf = AlignedAccessLPFOf(offset, 3);
    if (tlbEntry->lpf == lpf) {
        // See if the TLB entry privilege level allows us write access from this CPL
        if (isShadowStackWriteOK(tlbEntry, user)) {
            bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
            Bit32u pageOffset = PAGE_OFFSET(offset);
            bx_phy_address pAddr = tlbEntry->ppf | pageOffset;
            BX_NOTIFY_LIN_MEMORY_ACCESS(offset, pAddr, 4, tlbEntry->get_memtype(), BX_SHADOW_STACK_WRITE, (Bit8u*)&data);
            Bit32u* hostAddr = (Bit32u*)(hostPageAddr | pageOffset);
            pageWriteStampTable.decWriteStamp(pAddr, 4);
            WriteHostDWordToLittleEndian(hostAddr, data);
            return;
        }
    }

    if (access_write_linear(offset, 4, curr_pl, BX_SHADOW_STACK_WRITE, 0, (void*)&data) < 0)
        exception(BX_GP_EXCEPTION, 0);
}

void BX_CPP_AttrRegparmN(3) BX_CPU_C::shadow_stack_write_qword(bx_address offset, unsigned curr_pl, Bit64u data)
{
    bool user = (curr_pl == 3);
    bx_TLB_entry* tlbEntry = BX_DTLB_ENTRY_OF(offset, 7);
    bx_address lpf = AlignedAccessLPFOf(offset, 7);
    if (tlbEntry->lpf == lpf) {
        // See if the TLB entry privilege level allows us write access from this CPL
        if (isShadowStackWriteOK(tlbEntry, user)) {
            bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
            Bit32u pageOffset = PAGE_OFFSET(offset);
            bx_phy_address pAddr = tlbEntry->ppf | pageOffset;
            BX_NOTIFY_LIN_MEMORY_ACCESS(offset, pAddr, 8, tlbEntry->get_memtype(), BX_SHADOW_STACK_WRITE, (Bit8u*)&data);
            Bit64u* hostAddr = (Bit64u*)(hostPageAddr | pageOffset);
            pageWriteStampTable.decWriteStamp(pAddr, 8);
            WriteHostQWordToLittleEndian(hostAddr, data);
            return;
        }
    }

    if (access_write_linear(offset, 8, curr_pl, BX_SHADOW_STACK_WRITE, 0, (void*)&data) < 0)
        exception(BX_GP_EXCEPTION, 0);
}

bool BX_CPP_AttrRegparmN(4) BX_CPU_C::shadow_stack_lock_cmpxchg8b(bx_address offset, unsigned curr_pl, Bit64u data, Bit64u expected_data)
{
    Bit64u val64 = shadow_stack_read_qword(offset, curr_pl); // should be locked and RMW
    if (val64 == expected_data) {
        shadow_stack_write_qword(offset, curr_pl, data);
        return true;
    }
    else {
        shadow_stack_write_qword(offset, curr_pl, val64);
        return false;
    }
}

bool BX_CPP_AttrRegparmN(2) BX_CPU_C::shadow_stack_atomic_set_busy(bx_address offset, unsigned curr_pl)
{
    return shadow_stack_lock_cmpxchg8b(offset, curr_pl, offset | 0x1, offset);
}

bool BX_CPP_AttrRegparmN(2) BX_CPU_C::shadow_stack_atomic_clear_busy(bx_address offset, unsigned curr_pl)
{
    return shadow_stack_lock_cmpxchg8b(offset, curr_pl, offset, offset | 0x1);
}
#endif  //1245