#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#include "cpustats.h"

void BX_CPP_AttrRegparmN(2) BX_CPU_C::stackPrefetch(bx_address offset, unsigned len)
{
    bx_address laddr;
    unsigned pageOffset;

    INC_STACK_PREFETCH_STAT(stackPrefetch);

    BX_CPU_THIS_PTR espHostPtr = 0; // initialize with NULL pointer
    BX_CPU_THIS_PTR espPageWindowSize = 0;

    len--;

#if BX_SUPPORT_X86_64
    if (long64_mode() || (BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.valid & SegAccessWOK4G)) {
        laddr = offset;
        pageOffset = PAGE_OFFSET(offset);

        // canonical violations will miss the TLB below

        if (pageOffset + len >= 4096) // don't care for page split accesses
            return;

        BX_CPU_THIS_PTR espPageWindowSize = 4096;
    }
    else
#endif
    {
        laddr = get_laddr32(BX_SEG_REG_SS, (Bit32u)offset);
        pageOffset = PAGE_OFFSET(laddr);
        if (pageOffset + len >= 4096) // don't care for page split accesses
            return;

        Bit32u limit = BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.limit_scaled;
        Bit32u pageStart = (Bit32u)offset - pageOffset;

        if (!BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.valid) {
            //BX_ERROR(("stackPrefetch: SS not valid"));
            exception(BX_SS_EXCEPTION, 0);
        }

        //BX_ASSERT(BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.p);
        //BX_ASSERT(IS_DATA_SEGMENT_WRITEABLE(BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.type));

        // check that the begining of the page is within stack segment limits
        // problem can happen with EXPAND DOWN segments
        if (IS_DATA_SEGMENT_EXPAND_DOWN(BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.type)) {
            Bit32u upper_limit;
            if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.d_b)
                upper_limit = 0xffffffff;
            else
                upper_limit = 0x0000ffff;
            if (offset <= limit || offset > upper_limit || (upper_limit - offset) < len) {
                //BX_ERROR(("stackPrefetch(%d): access [0x%08x] > SS.limit [0x%08x] ED", len + 1, (Bit32u)offset, limit));
                exception(BX_SS_EXCEPTION, 0);
            }

            // check that the begining of the page is within stack segment limits
            // handle correctly the wrap corner case for EXPAND DOWN
            Bit32u pageEnd = pageStart + 0xfff;
            if (pageStart > limit && pageStart < pageEnd) {
                BX_CPU_THIS_PTR espPageWindowSize = 4096;
                if ((upper_limit - offset) < (4096 - pageOffset))
                    BX_CPU_THIS_PTR espPageWindowSize = (Bit32u)(upper_limit - offset + 1);
            }
        }
        else {
            if (offset > (limit - len) || len > limit) {
                //BX_ERROR(("stackPrefetch(%d): access [0x%08x] > SS.limit [0x%08x]", len + 1, (Bit32u)offset, limit));
                exception(BX_SS_EXCEPTION, 0);
            }

            if (pageStart <= limit) {
                BX_CPU_THIS_PTR espPageWindowSize = 4096;
                if ((limit - offset) < (4096 - pageOffset))
                    BX_CPU_THIS_PTR espPageWindowSize = (Bit32u)(limit - offset + 1);
            }
        }
    }

    Bit64u lpf = LPFOf(laddr);
    bx_TLB_entry* tlbEntry = BX_DTLB_ENTRY_OF(laddr, 0);
    if (tlbEntry->lpf == lpf) {
        // See if the TLB entry privilege level allows us write access from this CPL
        // Assuming that we always can read if write access is OK
        if (isWriteOK(tlbEntry, USER_PL)) {
            BX_CPU_THIS_PTR espPageBias = (bx_address)pageOffset - offset;
            BX_CPU_THIS_PTR pAddrStackPage = tlbEntry->ppf;
            BX_CPU_THIS_PTR espHostPtr = (Bit8u*)tlbEntry->hostPageAddr;
#if BX_SUPPORT_MEMTYPE
            BX_CPU_THIS_PTR espPageMemtype = tlbEntry->get_memtype();
#endif
#if BX_SUPPORT_SMP == 0
            BX_CPU_THIS_PTR espPageFineGranularityMapping = pageWriteStampTable.getFineGranularityMapping(tlbEntry->ppf);
#endif
        }
    }

    if (!BX_CPU_THIS_PTR espHostPtr || BX_CPU_THIS_PTR espPageWindowSize < 7)
        BX_CPU_THIS_PTR espPageWindowSize = 0;
    else
        BX_CPU_THIS_PTR espPageWindowSize -= 7;
}

void BX_CPP_AttrRegparmN(2) BX_CPU_C::stack_write_word(bx_address offset, Bit16u data)
{  //161
    bx_address espBiased = offset + BX_CPU_THIS_PTR espPageBias;

    if (espBiased >= BX_CPU_THIS_PTR espPageWindowSize) {
        stackPrefetch(offset, 2);
        espBiased = offset + BX_CPU_THIS_PTR espPageBias;
    }

    if (BX_CPU_THIS_PTR espHostPtr) {
        Bit16u* hostPageAddr = (Bit16u*)(BX_CPU_THIS_PTR espHostPtr + espBiased);
        bx_phy_address pAddr = BX_CPU_THIS_PTR pAddrStackPage + espBiased;
#if BX_CPU_LEVEL >= 4 && BX_SUPPORT_ALIGNMENT_CHECK
        if (BX_CPU_THIS_PTR alignment_check() && (pAddr & 1) != 0) {
            //BX_ERROR(("stack_write_word(): #AC misaligned access"));
            exception(BX_AC_EXCEPTION, 0);
        }
#endif
        BX_NOTIFY_LIN_MEMORY_ACCESS(get_laddr(BX_SEG_REG_SS, offset), pAddr, 2,
            MEMTYPE(BX_CPU_THIS_PTR espPageMemtype), BX_WRITE, (Bit8u*)&data);

#if BX_SUPPORT_SMP == 0
        if (BX_CPU_THIS_PTR espPageFineGranularityMapping)
#endif
            pageWriteStampTable.decWriteStamp(pAddr, 2);

        WriteHostWordToLittleEndian(hostPageAddr, data);
    }
    else {
        write_virtual_word(BX_SEG_REG_SS, offset, data);
    }
}

Bit16u BX_CPP_AttrRegparmN(1) BX_CPU_C::stack_read_word(bx_address offset)
{  //282
    bx_address espBiased = offset + BX_CPU_THIS_PTR espPageBias;

    if (espBiased >= BX_CPU_THIS_PTR espPageWindowSize) {
        stackPrefetch(offset, 2);
        espBiased = offset + BX_CPU_THIS_PTR espPageBias;
    }

    if (BX_CPU_THIS_PTR espHostPtr) {
        Bit16u* hostPageAddr = (Bit16u*)(BX_CPU_THIS_PTR espHostPtr + espBiased), data;
#if BX_CPU_LEVEL >= 4 && BX_SUPPORT_ALIGNMENT_CHECK
        if (BX_CPU_THIS_PTR alignment_check()) {
            bx_phy_address pAddr = BX_CPU_THIS_PTR pAddrStackPage + espBiased;
            if (pAddr & 1) {
                //BX_ERROR(("stack_read_word(): #AC misaligned access"));
                exception(BX_AC_EXCEPTION, 0);
            }
        }
#endif
        data = ReadHostWordFromLittleEndian(hostPageAddr);
        BX_NOTIFY_LIN_MEMORY_ACCESS(get_laddr(BX_SEG_REG_SS, offset),
            (BX_CPU_THIS_PTR pAddrStackPage + espBiased), 2,
            MEMTYPE(BX_CPU_THIS_PTR espPageMemtype), BX_READ, (Bit8u*)&data);
        return data;
    }
    else {
        return read_virtual_word(BX_SEG_REG_SS, offset);
    }
}