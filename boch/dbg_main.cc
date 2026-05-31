#define _CRT_SECURE_NO_WARNINGS
#include "bochs.h"
#include "param_names.h"
#include "debug.h"
#include "cpu.h"
#include "ia_opcodes.h"
#include "iodev.h"
#if BX_DEBUGGER
static FILE* debugger_log = NULL; //67

#define DBG_PRINTF_BUFFER_LEN 1024  //134
void dbg_printf(const char* fmt, ...)
{
    // 137
    va_list ap;
    va_start(ap, fmt);
    char buf[DBG_PRINTF_BUFFER_LEN + 1];
    vsnprintf(buf, DBG_PRINTF_BUFFER_LEN, fmt, ap);
    va_end(ap);
    if (debugger_log != NULL) {
        fprintf(debugger_log, "%s", buf);
        fflush(debugger_log);
    }
    //SIM->debug_puts(buf);
}
void bx_dbg_io_report(Bit32u port, unsigned size, unsigned op, Bit32u val)
{
    //3718
    if (bx_guard.report.io) {
        dbg_printf("event at t=" FMT_LL "d IO addr=0x%x size=%u op=%s val=0x%x\n",
            bx_pc_system.time_ticks(),
            port,
            size,
            (op == BX_READ) ? "read" : "write",
            (unsigned)val);
    }
}

bx_guard_t bx_guard;    //105

void bx_debug_break()
{ //536
    bx_guard.interrupt_requested = true;
}

void bx_dbg_lin_memory_access(unsigned cpu, bx_address lin, bx_phy_address phy, unsigned len, unsigned memtype, unsigned rw, Bit8u* data)
{
    //687
}

void bx_dbg_phy_memory_access(unsigned cpu, bx_phy_address phy, unsigned len, unsigned memtype, unsigned rw, unsigned access, Bit8u* data)
{
    //705
}

#endif /* if BX_DEBUGGER */ //最后一行