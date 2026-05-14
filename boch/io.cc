#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_SUPPORT_SVM
#include "svm.h"
#endif

#include "iodev.h"

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
