#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

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