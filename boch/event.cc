#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_SUPPORT_SVM
#include "svm.h"
#endif


#include "iodev.h"

#include "debug.h"

void BX_CPU_C::inhibit_interrupts(unsigned mask)
{
    // Loading of SS disables interrupts until the next instruction completes
    // but only under assumption that previous instruction didn't load SS also.
    if (mask != BX_INHIBIT_INTERRUPTS_BY_MOVSS || !interrupts_inhibited(BX_INHIBIT_INTERRUPTS_BY_MOVSS)) {
        //BX_DEBUG(("inhibit interrupts mask = %d", mask));
        BX_CPU_THIS_PTR inhibit_mask = mask;
        BX_CPU_THIS_PTR inhibit_icount = get_icount() + 1; // inhibit for next instruction
    }
}

bool BX_CPU_C::interrupts_inhibited(unsigned mask)
{
    //450
    return (get_icount() <= BX_CPU_THIS_PTR inhibit_icount) && (BX_CPU_THIS_PTR inhibit_mask & mask) == mask;
}