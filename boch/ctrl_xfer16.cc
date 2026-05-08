#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

void BX_CPP_AttrRegparmN(1) BX_CPU_C::branch_near16(Bit16u new_IP)
{
    //BX_ASSERT(BX_CPU_THIS_PTR cpu_mode != BX_MODE_LONG_64);
    if (new_IP > BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled) {
        //BX_ERROR(("branch_near16: offset outside of CS limits"));
        exception(BX_GP_EXCEPTION, 0);
    }

    //invalidate_prefetch_q();
    EIP = new_IP;

#if BX_SUPPORT_HANDLERS_CHAINING_SPEEDUPS == 0
    // assert magic async_event to stop trace execution
    BX_CPU_THIS_PTR async_event |= BX_ASYNC_EVENT_STOP_TRACE;
#endif
}