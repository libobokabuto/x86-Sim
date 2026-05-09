#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

bx_address bx_asize_mask[] = {
  0xffff,                         // as16 (asize = '00)
  0xffffffff,                     // as32 (asize = '01)
#if BX_SUPPORT_X86_64
  BX_CONST64(0xffffffffffffffff), // as64 (asize = '10)
  BX_CONST64(0xffffffffffffffff)  // as64 (asize = '11)
#endif
};

#if BX_SUPPORT_EVEX  //37
#define BX_MAX_MEM_ACCESS_LENGTH 64
#else
#if BX_SUPPORT_AVX
#define BX_MAX_MEM_ACCESS_LENGTH 32
#else
#define BX_MAX_MEM_ACCESS_LENGTH 16
#endif
#endif

// 126
bool BX_CPP_AttrRegparmN(4)
BX_CPU_C::read_virtual_checks(bx_segment_reg_t* seg, Bit32u offset, unsigned length, bool align)
{
    Bit32u upper_limit;

    length--;

    if (align) {
        Bit32u laddr = (Bit32u)(seg->cache.u.segment.base + offset);
        if (laddr & length) {
            //BX_DEBUG(("read_virtual_checks(): #GP misaligned access"));
            exception(BX_GP_EXCEPTION, 0);
        }
    }

    if (seg->cache.valid == 0) {
        //BX_DEBUG(("read_virtual_checks(): segment descriptor not valid"));
        return false;
    }

    if (seg->cache.p == 0) { /* not present */
        //BX_ERROR(("read_virtual_checks(): segment not present"));
        return false;
    }

    switch (seg->cache.type) {
    case 0: case 1: /* read only */
    case 2: case 3: /* read/write */
    case 10: case 11: /* execute/read */
    case 14: case 15: /* execute/read-only, conforming */
        if (seg->cache.u.segment.limit_scaled == 0xffffffff && seg->cache.u.segment.base == 0) {
            seg->cache.valid |= SegAccessROK | SegAccessROK4G;
            break;
        }

        if (offset > (seg->cache.u.segment.limit_scaled - length)
            || length > seg->cache.u.segment.limit_scaled)
        {
            //BX_ERROR(("read_virtual_checks(): read beyond limit"));
            return false;
        }
        if (seg->cache.u.segment.limit_scaled >= (BX_MAX_MEM_ACCESS_LENGTH - 1)) {
            // Mark cache as being OK type for succeeding reads. See notes for
            // write checks; similar code.
            seg->cache.valid |= SegAccessROK;
        }
        break;

    case 4: case 5: /* read only, expand down */
    case 6: case 7: /* read/write, expand down */
        if (seg->cache.u.segment.d_b)
            upper_limit = 0xffffffff;
        else
            upper_limit = 0x0000ffff;
        if (offset <= seg->cache.u.segment.limit_scaled ||
            offset > upper_limit || (upper_limit - offset) < length)
        {
            //BX_ERROR(("read_virtual_checks(): read beyond limit expand down"));
            return false;
        }
        break;

    case 8: case 9: /* execute only */
    case 12: case 13: /* execute only, conforming */
        /* can't read or write an execute-only segment */
        //BX_ERROR(("read_virtual_checks(): execute only"));
        return false;

    default:
        //BX_PANIC(("read_virtual_checks(): unknown descriptor type=%d", seg->cache.type));
        break;
    }

    return true;
}
//294

#if BX_SUPPORT_X86_64
bool BX_CPP_AttrRegparmN(3) BX_CPU_C::IsCanonicalAccess(bx_address laddr, unsigned rw, bool user)
{
    if (!IsCanonical(laddr)) {
        return false;
    }

    if (long64_mode() && BX_CPU_THIS_PTR cr4.get_LASS()) {
        // laddr[63] == 0 user, laddr[63] == 1 supervisor
        bool access_user_space = (laddr >> 63) == 0;
        if (user) {
            // When LASS is enabled, linear user accesses to supervisor space are blocked
            if (!access_user_space) {
                //BX_ERROR(("User access LASS canonical violation for address 0x" FMT_LL "x rw=%d", laddr, rw));
                return false;
            }
            return true;
        }

        // A supervisor-mode instruction fetch causes a LASS violation if it would accesses a linear address[63] == 0
        // A supervisor-mode data access causes a LASS violation only if supervisor-mode access protection is enabled 
        // (CR4.SMAP = 1) and RFLAGS.AC = 0 or the access implicitly accesses a system data structure.
        if (rw == BX_EXECUTE || (BX_CPU_THIS_PTR cr4.get_SMAP() && !BX_CPU_THIS_PTR get_AC())) {
            if (access_user_space) {
                //BX_ERROR(("Supervisor access LASS canonical violation for address 0x" FMT_LL "x rw=%d", laddr, rw));
                return false;
            }
        }
    }

    return true;
}
#endif
//328-431
int BX_CPU_C::access_read_linear(bx_address laddr, unsigned len, unsigned curr_pl, unsigned xlate_rw, Bit32u ac_mask, void* data)
{
#if BX_SUPPORT_CET
    //BX_ASSERT(xlate_rw == BX_READ || xlate_rw == BX_RW || xlate_rw == BX_SHADOW_STACK_READ || xlate_rw == BX_SHADOW_STACK_RW);
#else
    //BX_ASSERT(xlate_rw == BX_READ || xlate_rw == BX_RW);
#endif

    bool user = (curr_pl == 3);

#if BX_SUPPORT_X86_64
    if (!IsCanonicalAccess(laddr, xlate_rw, user)) {
        //BX_ERROR(("access_read_linear(): canonical failure"));
        return -1;
    }
#endif

    Bit32u pageOffset = PAGE_OFFSET(laddr);

#if BX_CPU_LEVEL >= 4 && BX_SUPPORT_ALIGNMENT_CHECK
    if (BX_CPU_THIS_PTR alignment_check() && user) {
        if (pageOffset & ac_mask) {
            //BX_ERROR(("access_read_linear(): #AC misaligned access"));
            exception(BX_AC_EXCEPTION, 0);
        }
    }
#endif

    bx_TLB_entry* tlbEntry = BX_DTLB_ENTRY_OF(laddr, 0);

    /* check for reference across multiple pages */
    if ((pageOffset + len) <= 4096) {
        // Access within single page.
        BX_CPU_THIS_PTR address_xlation.paddress1 = translate_linear(tlbEntry, laddr, user, xlate_rw);
        BX_CPU_THIS_PTR address_xlation.pages = 1;
#if BX_SUPPORT_MEMTYPE
        BX_CPU_THIS_PTR address_xlation.memtype1 = tlbEntry->get_memtype();
#endif
        access_read_physical(BX_CPU_THIS_PTR address_xlation.paddress1, len, data);
        BX_NOTIFY_LIN_MEMORY_ACCESS(laddr, BX_CPU_THIS_PTR address_xlation.paddress1, len, tlbEntry->get_memtype(), xlate_rw, (Bit8u*)data);

#if BX_X86_DEBUGGER
        hwbreakpoint_match(laddr, len, xlate_rw);
#endif
    }
    else {
        // access across 2 pages
        BX_CPU_THIS_PTR address_xlation.len1 = 4096 - pageOffset;
        BX_CPU_THIS_PTR address_xlation.len2 = len - BX_CPU_THIS_PTR address_xlation.len1;
        BX_CPU_THIS_PTR address_xlation.pages = 2;
        bx_address laddr2 = laddr + BX_CPU_THIS_PTR address_xlation.len1;
#if BX_SUPPORT_X86_64
        if (!long64_mode()) laddr2 &= 0xffffffff; /* handle linear address wrap in legacy mode */
        else {
            if (!IsCanonicalAccess(laddr2, xlate_rw, user)) {
                //BX_ERROR(("access_read_linear(): canonical failure for second half of page split access"));
                return -1;
            }
        }
#endif

        bx_TLB_entry* tlbEntry2 = BX_DTLB_ENTRY_OF(laddr2, 0);

        BX_CPU_THIS_PTR address_xlation.paddress1 = translate_linear(tlbEntry, laddr, user, xlate_rw);
        BX_CPU_THIS_PTR address_xlation.paddress2 = translate_linear(tlbEntry2, laddr2, user, xlate_rw);
#if BX_SUPPORT_MEMTYPE
        BX_CPU_THIS_PTR address_xlation.memtype1 = tlbEntry->get_memtype();
        BX_CPU_THIS_PTR address_xlation.memtype2 = tlbEntry2->get_memtype();
#endif

#ifdef BX_LITTLE_ENDIAN
        access_read_physical(BX_CPU_THIS_PTR address_xlation.paddress1,
            BX_CPU_THIS_PTR address_xlation.len1, data);
        BX_NOTIFY_LIN_MEMORY_ACCESS(laddr, BX_CPU_THIS_PTR address_xlation.paddress1,
            BX_CPU_THIS_PTR address_xlation.len1, tlbEntry->get_memtype(),
            xlate_rw, (Bit8u*)data);
        access_read_physical(BX_CPU_THIS_PTR address_xlation.paddress2,
            BX_CPU_THIS_PTR address_xlation.len2,
            ((Bit8u*)data) + BX_CPU_THIS_PTR address_xlation.len1);
        BX_NOTIFY_LIN_MEMORY_ACCESS(laddr2, BX_CPU_THIS_PTR address_xlation.paddress2,
            BX_CPU_THIS_PTR address_xlation.len2, tlbEntry2->get_memtype(),
            xlate_rw, ((Bit8u*)data) + BX_CPU_THIS_PTR address_xlation.len1);
#else // BX_BIG_ENDIAN
        access_read_physical(BX_CPU_THIS_PTR address_xlation.paddress1,
            BX_CPU_THIS_PTR address_xlation.len1,
            ((Bit8u*)data) + (len - BX_CPU_THIS_PTR address_xlation.len1));
        BX_NOTIFY_LIN_MEMORY_ACCESS(laddr, BX_CPU_THIS_PTR address_xlation.paddress1,
            BX_CPU_THIS_PTR address_xlation.len1, tlbEntry->get_memtype(),
            xlate_rw, ((Bit8u*)data) + (len - BX_CPU_THIS_PTR address_xlation.len1));
        access_read_physical(BX_CPU_THIS_PTR address_xlation.paddress2,
            BX_CPU_THIS_PTR address_xlation.len2, data);
        BX_NOTIFY_LIN_MEMORY_ACCESS(laddr2, BX_CPU_THIS_PTR address_xlation.paddress2,
            BX_CPU_THIS_PTR address_xlation.len2, tlbEntry2->get_memtype(),
            xlate_rw, (Bit8u*)data);
#endif

#if BX_X86_DEBUGGER
        hwbreakpoint_match(laddr, BX_CPU_THIS_PTR address_xlation.len1, xlate_rw);
        hwbreakpoint_match(laddr2, BX_CPU_THIS_PTR address_xlation.len2, xlate_rw);
#endif
    }

    return 0;
}
int BX_CPU_C::int_number(unsigned s)
{
    if (s == BX_SEG_REG_SS)
        return BX_SS_EXCEPTION;
    else
        return BX_GP_EXCEPTION;
}

Bit64u BX_CPP_AttrRegparmN(1)
BX_CPU_C::system_read_qword(bx_address laddr)
{
    Bit64u data;

    bx_address lpf = LPFOf(laddr);
    bx_TLB_entry* tlbEntry = BX_DTLB_ENTRY_OF(laddr, 7);
    if (tlbEntry->lpf == lpf) {
        // See if the TLB entry privilege level allows us read access from CPL=0
        if (isReadOK(tlbEntry, 0)) {
            bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
            Bit32u pageOffset = PAGE_OFFSET(laddr);
            Bit64u* hostAddr = (Bit64u*)(hostPageAddr | pageOffset);
            data = ReadHostQWordFromLittleEndian(hostAddr);
            BX_NOTIFY_LIN_MEMORY_ACCESS(laddr, (tlbEntry->ppf | pageOffset), 8, tlbEntry->get_memtype(), BX_READ, (Bit8u*)&data);
            return data;
        }
    }

    if (access_read_linear(laddr, 8, 0, BX_READ, 0x0, (void*)&data) < 0)
        exception(BX_GP_EXCEPTION, 0);

    return data;
}

void BX_CPP_AttrRegparmN(2)
BX_CPU_C::system_write_byte(bx_address laddr, Bit8u data)
{
    /*
    bx_address lpf = LPFOf(laddr);
    bx_TLB_entry* tlbEntry = BX_DTLB_ENTRY_OF(laddr, 0);
    if (tlbEntry->lpf == lpf) {
        // See if the TLB entry privilege level allows us write access from CPL=0
        if (isWriteOK(tlbEntry, 0)) {
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

    if (access_write_linear(laddr, 1, 0, BX_WRITE, 0x0, (void*)&data) < 0)
        exception(BX_GP_EXCEPTION, 0);
        */
}
