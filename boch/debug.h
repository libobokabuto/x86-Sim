#ifndef BX_DEBUG_H
#define BX_DEBUG_H
#include "config.h"
#include "osdep.h"
#if BX_DEBUGGER

#include "decoder.h"
#ifdef __cplusplus //37

BOCHSAPI_MSVCONLY void dbg_printf(const char* fmt, ...);
typedef enum {
    STOP_NO_REASON = 0,
    STOP_TIME_BREAK_POINT,
    STOP_READ_WATCH_POINT,
    STOP_WRITE_WATCH_POINT,
    STOP_MAGIC_BREAK_POINT,
    STOP_MODE_BREAK_POINT,
    STOP_VMEXIT_BREAK_POINT,
    STOP_CPU_HALTED,
} stop_reason_t;
BOCHSAPI_MSVCONLY void bx_debug_break(void);//222
struct bx_guard_t {//244-314

    
    unsigned guard_for;
    volatile bool interrupt_requested;
    struct {
        bool irq;
        bool a20;
        bool io;
        bool dma;
    } report;

    struct {
        bool irq;  // should process IRQs asynchronously
        bool dma;  // should process DMAs asynchronously
    } async;
};
BOCHSAPI_MSVCONLY extern bx_guard_t bx_guard; //325
void bx_dbg_io_report(Bit32u port, unsigned size, unsigned op, Bit32u val); //347
#endif //351

#endif // #if BX_DEBUGGER
#endif

