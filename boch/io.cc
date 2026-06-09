#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_SUPPORT_SVM
#include "svm.h"
#endif

#include "iodev.h"

#if BX_SUPPORT_REPEAT_SPEEDUPS

Bit32u BX_CPU_C::FastRepINSW(Bit32u dstOff, Bit16u port, Bit32u wordCount)
{
    Bit32u wordsFitDst;
    signed int pointerDelta;
    Bit8u* hostAddrDst;
    unsigned count;
    bx_address laddrDst;

    BX_ASSERT(BX_CPU_THIS_PTR cpu_mode != BX_MODE_LONG_64);

    bx_segment_reg_t* dstSegPtr = &BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES];
    if (dstSegPtr->cache.valid & SegAccessWOK4G) {
        laddrDst = dstOff;
    }
    else {
        if (!(dstSegPtr->cache.valid & SegAccessWOK))
            return 0;
        if ((dstOff | 0xfff) > dstSegPtr->cache.u.segment.limit_scaled)
            return 0;

        laddrDst = get_laddr32(BX_SEG_REG_ES, dstOff);
    }

    // check that the address is word aligned
    if (laddrDst & 1) return 0;

    hostAddrDst = v2h_write_byte(laddrDst, USER_PL);
    // Check that native host access was not vetoed for that page
    if (!hostAddrDst) return 0;

    // See how many words can fit in the rest of this page.
    if (BX_CPU_THIS_PTR get_DF()) {
        // Counting downward
        // 1st word must cannot cross page boundary because it is word aligned
        wordsFitDst = (2 + (PAGE_OFFSET(laddrDst))) >> 1;
        pointerDelta = -2;
    }
    else {
        // Counting upward
        wordsFitDst = (0x1000 - PAGE_OFFSET(laddrDst)) >> 1;
        pointerDelta = 2;
    }

    // Restrict word count to the number that will fit in this page.
    if (wordCount > wordsFitDst)
        wordCount = wordsFitDst;

    // If after all the restrictions, there is anything left to do...
    if (wordCount) {
        for (count = 0; count < wordCount; ) {
            bx_devices.bulkIOQuantumsTransferred = 0;
            if (BX_CPU_THIS_PTR get_DF() == 0) { // Only do accel for DF=0
                bx_devices.bulkIOHostAddr = hostAddrDst;
                bx_devices.bulkIOQuantumsRequested = (wordCount - count);
            }
            else
                bx_devices.bulkIOQuantumsRequested = 0;
            Bit16u temp16 = BX_INP(port, 2);
            if (bx_devices.bulkIOQuantumsTransferred) {
                hostAddrDst = bx_devices.bulkIOHostAddr;
                count += bx_devices.bulkIOQuantumsTransferred;
            }
            else {
                WriteHostWordToLittleEndian((Bit16u*)hostAddrDst, temp16);
                hostAddrDst += pointerDelta;
                count++;
            }
            // Terminate early if there was an event.
            if (BX_CPU_THIS_PTR async_event) break;
        }

        // Reset for next non-bulk IO
        bx_devices.bulkIOQuantumsRequested = 0;

        return count;
    }

    return 0;
}

Bit32u BX_CPU_C::FastRepOUTSW(unsigned srcSeg, Bit32u srcOff, Bit16u port, Bit32u wordCount)
{
    Bit32u wordsFitSrc;
    signed int pointerDelta;
    Bit8u* hostAddrSrc;
    unsigned count;
    bx_address laddrSrc;

    BX_ASSERT(BX_CPU_THIS_PTR cpu_mode != BX_MODE_LONG_64);

    bx_segment_reg_t* srcSegPtr = &BX_CPU_THIS_PTR sregs[srcSeg];
    if (srcSegPtr->cache.valid & SegAccessROK4G) {
        laddrSrc = srcOff;
    }
    else {
        if (!(srcSegPtr->cache.valid & SegAccessROK))
            return 0;
        if ((srcOff | 0xfff) > srcSegPtr->cache.u.segment.limit_scaled)
            return 0;

        laddrSrc = get_laddr32(srcSeg, srcOff);
    }

    // check that the address is word aligned
    if (laddrSrc & 1) return 0;

    hostAddrSrc = v2h_read_byte(laddrSrc, USER_PL);
    // Check that native host access was not vetoed for that page
    if (!hostAddrSrc) return 0;

    // See how many words can fit in the rest of this page.
    if (BX_CPU_THIS_PTR get_DF()) {
        // Counting downward
        // 1st word must cannot cross page boundary because it is word aligned
        wordsFitSrc = (2 + (PAGE_OFFSET(laddrSrc))) >> 1;
        pointerDelta = (unsigned)-2;
    }
    else {
        // Counting upward
        wordsFitSrc = (0x1000 - PAGE_OFFSET(laddrSrc)) >> 1;
        pointerDelta = 2;
    }

    // Restrict word count to the number that will fit in this page.
    if (wordCount > wordsFitSrc)
        wordCount = wordsFitSrc;

    // If after all the restrictions, there is anything left to do...
    if (wordCount) {
        for (count = 0; count < wordCount; ) {
            bx_devices.bulkIOQuantumsTransferred = 0;
            if (BX_CPU_THIS_PTR get_DF() == 0) { // Only do accel for DF=0
                bx_devices.bulkIOHostAddr = hostAddrSrc;
                bx_devices.bulkIOQuantumsRequested = (wordCount - count);
            }
            else
                bx_devices.bulkIOQuantumsRequested = 0;
            Bit16u temp16 = ReadHostWordFromLittleEndian((Bit16u*)hostAddrSrc);
            BX_OUTP(port, temp16, 2);
            if (bx_devices.bulkIOQuantumsTransferred) {
                hostAddrSrc = bx_devices.bulkIOHostAddr;
                count += bx_devices.bulkIOQuantumsTransferred;
            }
            else {
                hostAddrSrc += pointerDelta;
                count++;
            }
            // Terminate early if there was an event.
            if (BX_CPU_THIS_PTR async_event) break;
        }

        // Reset for next non-bulk IO
        bx_devices.bulkIOQuantumsRequested = 0;

        return count;
    }

    return 0;
}

#endif

bool BX_CPP_AttrRegparmN(3) BX_CPU_C::allow_io(bxInstruction_c* i, Bit16u port, unsigned len)
{
    /* If CPL <= IOPL, then all IO portesses are accessible.
     * Otherwise, must check the IO permission map on >286.
     * On the 286, there is no IO permissions map */

    static bool port_e9_hack_all_rings = 1;
    if (0xe9 == port && port_e9_hack_all_rings)
        return(1); // port e9 hack can be used by unprivileged code

#if BX_SUPPORT_IODEBUG
    static bool iodebug_all_rings = 1;
    if (0x8A00 == (port & 0xfffe) && iodebug_all_rings)
        return(1); // iodebug ports (0x8A00 & 0x8A01) can be used by unprivileged code
#endif /* if BX_SUPPORT_IODEBUG */

    if (BX_CPU_THIS_PTR cr0.get_PE() && (BX_CPU_THIS_PTR get_VM() || (CPL > BX_CPU_THIS_PTR get_IOPL())))
    {
        if (BX_CPU_THIS_PTR tr.cache.valid == 0 ||
            (BX_CPU_THIS_PTR tr.cache.type != BX_SYS_SEGMENT_AVAIL_386_TSS &&
                BX_CPU_THIS_PTR tr.cache.type != BX_SYS_SEGMENT_BUSY_386_TSS))
        {
            //BX_ERROR(("allow_io(): TR doesn't point to a valid 32bit TSS, TR.TYPE=%u", BX_CPU_THIS_PTR tr.cache.type));
            return(0);
        }

        if (BX_CPU_THIS_PTR tr.cache.u.segment.limit_scaled < 103) {
            //BX_ERROR(("allow_io(): TR.limit < 103"));
            return(0);
        }

        Bit32u io_base = system_read_word(BX_CPU_THIS_PTR tr.cache.u.segment.base + 102);

        if ((io_base + port / 8) >= BX_CPU_THIS_PTR tr.cache.u.segment.limit_scaled) {
            //BX_DEBUG(("allow_io(): IO port %x (len %d) outside TSS IO permission map (base=%x, limit=%x) #GP(0)",
                //port, len, io_base, BX_CPU_THIS_PTR tr.cache.u.segment.limit_scaled));
            return(0);
        }

        Bit16u permission16 = system_read_word(BX_CPU_THIS_PTR tr.cache.u.segment.base + io_base + port / 8);

        unsigned bit_index = port & 0x7;
        unsigned mask = (1 << len) - 1;
        if ((permission16 >> bit_index) & mask)
            return(0);
    }

#if BX_SUPPORT_SVM
    if (BX_CPU_THIS_PTR in_svm_guest) {
        if (SVM_INTERCEPT(SVM_INTERCEPT0_IO)) SvmInterceptIO(i, port, len);
    }
#endif

#if BX_SUPPORT_VMX
    if (BX_CPU_THIS_PTR in_vmx_guest)
        VMexit_IO(i, port, len);
#endif

#if BX_X86_DEBUGGER && BX_CPU_LEVEL >= 5
    iobreakpoint_match(port, len);
#endif

    return(1);
}
