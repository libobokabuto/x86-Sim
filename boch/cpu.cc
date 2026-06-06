#include "bochs.h"
#include "cpu.h"
#include "MD5.h"
#include "ia_opcodes.h"
#include "cpustats.h" //29行
#include "pc_system.h"
#include "cpuid.h"
#include <stdint.h>
uint8_t g_test_buff[1024 * 1024 * 1] = { 0 };
int md5count = 0;
int cpudatalen = (sizeof(bx_gen_reg_t)) * 20;
#if BX_SUPPORT_HANDLERS_CHAINING_SPEEDUPS
#else

#define BX_SYNC_TIME_IF_SINGLE_PROCESSOR(allowed_delta) \
  if (BX_SMP_PROCESSORS == 1) BX_TICK1()  //50

#endif
jmp_buf BX_CPU_C::jmp_buf_env; //55
void BX_CPU_C::cpu_loop(void)
{
    if (setjmp(BX_CPU_THIS_PTR jmp_buf_env)) {
        // can get here only from exception function or VMEXIT
        BX_CPU_THIS_PTR icount++;
        BX_SYNC_TIME_IF_SINGLE_PROCESSOR(0);
#if BX_GDBSTUB
        if (gdbstub_instruction_epilog() || bx_dbg.gdbstub_enabled) return;
#endif
    }

    while (1){
        
        if (BX_CPU_THIS_PTR async_event) {
            if (handleAsyncEvent()) {
                // If request to return to caller ASAP.
                return;
            }
        }
        
        bxICacheEntry_c* entry = getICacheEntry();
        bxInstruction_c* i = entry->i;

        bxInstruction_c* last = i + (entry->tlen);

        for (;;) {

            // want to allow changing of the instruction inside instrumentation callback
            BX_INSTR_BEFORE_EXECUTION(BX_CPU_ID, i);
            RIP += i->ilen();

            BX_CPU_THIS_PTR trace_empty_instr = false;
            BX_CPU_THIS_PTR trace_empty_instr_line = 0;
            BX_CPU_THIS_PTR trace_empty_instr_func = NULL;

            BX_CPU_CALL_METHOD(i->execute1, (i)); // might iterate repeat instruction
            BX_CPU_THIS_PTR prev_rip = RIP; // commit new RIP
            BX_INSTR_AFTER_EXECUTION(BX_CPU_ID, i);
            BX_CPU_THIS_PTR icount++;
            if (BX_CPU_THIS_PTR icount == 8650173)
            {
                   int qwq = 0;
            }
            
            uint64_t trace_index = BX_CPU_THIS_PTR icount;
            //trace_index >= 300000
            //(trace_index % 500) < 10
            const uint64_t trace_start = 8650000;  // 从第 15 万条之后开始打印
            const uint64_t trace_step = 1000;    // 每 1000 条一个周期
            const uint64_t trace_count = 1000;      // 每个周期打印 10 条

            if (trace_index >= trace_start &&
                ((trace_index - trace_start) % trace_step) < trace_count)
            {
                unsigned char raw_md5[16];
                unsigned char nortc_md5[16];

                unsigned opcode = i->getIaOpcode();

                bool just_read_cmos = false;

                if (opcode == BX_IA_IN_ALIb && i->Ib() == 0x71) {
                    just_read_cmos = true;
                }

                if (opcode == BX_IA_IN_ALDX && DX == 0x71) {
                    just_read_cmos = true;
                }

                // 1. 原始 MD5：保留计算，但默认不打印
                memcpy(g_test_buff, BX_CPU_THIS_PTR gen_reg, cpudatalen);

                MD5_CTX raw_ctx;
                MD5Init(&raw_ctx);
                MD5Update(&raw_ctx, (unsigned char*)g_test_buff, cpudatalen);
                MD5Final(&raw_ctx, raw_md5);

                // 2. MD5_NORTC_DIRECT：只屏蔽“当前刚从 0x71 读入 AL”的直接 RTC 值
                memcpy(g_test_buff, BX_CPU_THIS_PTR gen_reg, cpudatalen);

                if (just_read_cmos) {
                    bx_gen_reg_t* md5_regs = (bx_gen_reg_t*)g_test_buff;

#if BX_SUPPORT_X86_64
                    md5_regs[0].rrx &= 0xffffffffffffff00ULL;  // 只清 AL，不清整个 RAX
#else
                    md5_regs[0].dword.erx &= 0xffffff00U;      // 只清 AL，不清整个 EAX
#endif
                }

                MD5_CTX nortc_ctx;
                MD5Init(&nortc_ctx);
                MD5Update(&nortc_ctx, (unsigned char*)g_test_buff, cpudatalen);
                MD5Final(&nortc_ctx, nortc_md5);

                printf(
                    "%llu: RIP=%016llx ia=%u cd=%lld mode=%u RAW=",
                    (unsigned long long)BX_CPU_THIS_PTR icount,
                    (unsigned long long)RIP,
                    (unsigned)opcode,
                    (long long)bx_pc_system.currCountdown,
                    (unsigned)BX_CPU_THIS_PTR cpu_mode
                );

                for (int j = 0; j < 16; j++) {
                    printf("%02x", raw_md5[j]);
                }

#if 0
                printf(" NORTC=");

                for (int j = 0; j < 16; j++) {
                    printf("%02x", nortc_md5[j]);
                }
#endif
#if BX_SUPPORT_X86_64
                printf(
                    " RAX=%016llx AL=%02x",
                    (unsigned long long)RAX,
                    (unsigned)(RAX & 0xff)
                );
                

#else
                printf(
                    " EAX=%08x AL=%02x",
                    (unsigned)EAX,
                    (unsigned)(EAX & 0xff)
                );
#endif

                printf(
                    " STUB=%u",
                    BX_CPU_THIS_PTR trace_empty_instr ? 1u : 0u
                );

                if (BX_CPU_THIS_PTR trace_empty_instr) {
                    printf(
                        " STUB_FN=%s:%d",
                        BX_CPU_THIS_PTR trace_empty_instr_func ? BX_CPU_THIS_PTR trace_empty_instr_func : "unknown",
                        BX_CPU_THIS_PTR trace_empty_instr_line
                    );
                }



                if (just_read_cmos) {
                    printf(" RTC_READ");
                }

                printf("\n");
            }



            BX_SYNC_TIME_IF_SINGLE_PROCESSOR(0);//时钟

            // note instructions generating exceptions never reach this point
#if BX_GDBSTUB
            if (gdbstub_instruction_epilog()) return;
#endif

            if (BX_CPU_THIS_PTR async_event) break;

            if (++i == last) {
                entry = getICacheEntry();
                i = entry->i;
                last = i + (entry->tlen);
            }
        }
        BX_CPU_THIS_PTR async_event &= ~BX_ASYNC_EVENT_STOP_TRACE;
    }
}

bxICacheEntry_c* BX_CPU_C::getICacheEntry(void)
{
    bx_address eipBiased = RIP + BX_CPU_THIS_PTR eipPageBias;
    if (eipBiased >= BX_CPU_THIS_PTR eipPageWindowSize) {
        prefetch();
        eipBiased = RIP + BX_CPU_THIS_PTR eipPageBias;
    }
    INC_ICACHE_STAT(iCacheLookups);

    bx_phy_address pAddr = BX_CPU_THIS_PTR pAddrFetchPage + eipBiased;
    bxICacheEntry_c* entry = BX_CPU_THIS_PTR iCache.find_entry(pAddr, BX_CPU_THIS_PTR fetchModeMask);
    entry = NULL;

    if (entry == NULL)
    {
        // iCache miss. No validated instruction with matching fetch parameters
        // is in the iCache.
        INC_ICACHE_STAT(iCacheMisses);
        entry = serveICacheMiss((Bit32u)eipBiased, pAddr);
    }
    return entry;
}
#define BX_REPEAT_TIME_UPDATE_INTERVAL (BX_MAX_TRACE_LENGTH-1) //425
void BX_CPP_AttrRegparmN(2) BX_CPU_C::repeat(bxInstruction_c* i, BxRepIterationPtr_tR execute)
{
    // non repeated instruction
    if (!i->repUsedL()) {
        BX_CPU_CALL_REP_ITERATION(execute, (i));
        return;
    }

    //BX_ASSERT(!bx_dbg.debugger_active || BX_CPU_THIS_PTR async_event);

    BX_CPU_THIS_PTR clear_RF();

#if BX_SUPPORT_X86_64
    if (i->as64L()) {
        while (1) {
            if (RCX != 0) {
                BX_CPU_CALL_REP_ITERATION(execute, (i));
                BX_INSTR_REPEAT_ITERATION(BX_CPU_ID, i);
                RCX--;
            }
            if (RCX == 0) return;

            if (BX_CPU_THIS_PTR async_event)
                break; // exit always if debugger enabled

            BX_CPU_THIS_PTR icount++;

            BX_SYNC_TIME_IF_SINGLE_PROCESSOR(BX_REPEAT_TIME_UPDATE_INTERVAL);
        }
    }
    else
#endif
        if (i->as32L()) {
            while (1) {
                if (ECX != 0) {
                    BX_CPU_CALL_REP_ITERATION(execute, (i));
                    BX_INSTR_REPEAT_ITERATION(BX_CPU_ID, i);
                    RCX = ECX - 1;
                }
                if (ECX == 0) return;

                if (BX_CPU_THIS_PTR async_event)
                    break; // exit always if debugger enabled

                BX_CPU_THIS_PTR icount++;

                BX_SYNC_TIME_IF_SINGLE_PROCESSOR(BX_REPEAT_TIME_UPDATE_INTERVAL);
            }
        }
        else  // 16bit addrsize
        {
            while (1) {
                if (CX != 0) {
                    BX_CPU_CALL_REP_ITERATION(execute, (i));
                    BX_INSTR_REPEAT_ITERATION(BX_CPU_ID, i);
                    CX--;
                }
                if (CX == 0) return;

                if (BX_CPU_THIS_PTR async_event)
                    break; // exit always if debugger enabled

                BX_CPU_THIS_PTR icount++;

                BX_SYNC_TIME_IF_SINGLE_PROCESSOR(BX_REPEAT_TIME_UPDATE_INTERVAL);
            }
        }

    BX_CPU_THIS_PTR assert_RF();

    RIP = BX_CPU_THIS_PTR prev_rip; // repeat loop not done, restore RIP

    // assert magic async_event to stop trace execution
    BX_CPU_THIS_PTR async_event |= BX_ASYNC_EVENT_STOP_TRACE;
}

void BX_CPP_AttrRegparmN(2) BX_CPU_C::repeat_ZF(bxInstruction_c* i, BxRepIterationPtr_tR execute)
{
    unsigned rep = i->lockRepUsedValue();

    // non repeated instruction
    if (rep < 2) {
        BX_CPU_CALL_REP_ITERATION(execute, (i));
        return;
    }

    BX_CPU_THIS_PTR clear_RF();

    if (rep == 3) { /* repeat prefix 0xF3 */
#if BX_SUPPORT_X86_64
        if (i->as64L()) {
            while (1) {
                if (RCX != 0) {
                    BX_CPU_CALL_REP_ITERATION(execute, (i));
                    BX_INSTR_REPEAT_ITERATION(BX_CPU_ID, i);
                    RCX--;
                }
                if (!get_ZF() || RCX == 0) return;

                if (BX_CPU_THIS_PTR async_event)
                    break; // exit always if debugger enabled

                BX_CPU_THIS_PTR icount++;

                BX_SYNC_TIME_IF_SINGLE_PROCESSOR(BX_REPEAT_TIME_UPDATE_INTERVAL);
            }
        }
        else
#endif
            if (i->as32L()) {
                while (1) {
                    if (ECX != 0) {
                        BX_CPU_CALL_REP_ITERATION(execute, (i));
                        BX_INSTR_REPEAT_ITERATION(BX_CPU_ID, i);
                        RCX = ECX - 1;
                    }
                    if (!get_ZF() || ECX == 0) return;

                    if (BX_CPU_THIS_PTR async_event)
                        break; // exit always if debugger enabled

                    BX_CPU_THIS_PTR icount++;

                    BX_SYNC_TIME_IF_SINGLE_PROCESSOR(BX_REPEAT_TIME_UPDATE_INTERVAL);
                }
            }
            else  // 16bit addrsize
            {
                while (1) {
                    if (CX != 0) {
                        BX_CPU_CALL_REP_ITERATION(execute, (i));
                        BX_INSTR_REPEAT_ITERATION(BX_CPU_ID, i);
                        CX--;
                    }
                    if (!get_ZF() || CX == 0) return;

                    if (BX_CPU_THIS_PTR async_event)
                        break; // exit always if debugger enabled

                    BX_CPU_THIS_PTR icount++;

                    BX_SYNC_TIME_IF_SINGLE_PROCESSOR(BX_REPEAT_TIME_UPDATE_INTERVAL);
                }
            }
    }
    else {          /* repeat prefix 0xF2 */
#if BX_SUPPORT_X86_64
        if (i->as64L()) {
            while (1) {
                if (RCX != 0) {
                    BX_CPU_CALL_REP_ITERATION(execute, (i));
                    BX_INSTR_REPEAT_ITERATION(BX_CPU_ID, i);
                    RCX--;
                }
                if (get_ZF() || RCX == 0) return;

                if (BX_CPU_THIS_PTR async_event)
                    break; // exit always if debugger enabled

                BX_CPU_THIS_PTR icount++;

                BX_SYNC_TIME_IF_SINGLE_PROCESSOR(BX_REPEAT_TIME_UPDATE_INTERVAL);
            }
        }
        else
#endif
            if (i->as32L()) {
                while (1) {
                    if (ECX != 0) {
                        BX_CPU_CALL_REP_ITERATION(execute, (i));
                        BX_INSTR_REPEAT_ITERATION(BX_CPU_ID, i);
                        RCX = ECX - 1;
                    }
                    if (get_ZF() || ECX == 0) return;

                    if (BX_CPU_THIS_PTR async_event)
                        break; // exit always if debugger enabled

                    BX_CPU_THIS_PTR icount++;

                    BX_SYNC_TIME_IF_SINGLE_PROCESSOR(BX_REPEAT_TIME_UPDATE_INTERVAL);
                }
            }
            else  // 16bit addrsize
            {
                while (1) {
                    if (CX != 0) {
                        BX_CPU_CALL_REP_ITERATION(execute, (i));
                        BX_INSTR_REPEAT_ITERATION(BX_CPU_ID, i);
                        CX--;
                    }
                    if (get_ZF() || CX == 0) return;

                    if (BX_CPU_THIS_PTR async_event)
                        break; // exit always if debugger enabled

                    BX_CPU_THIS_PTR icount++;

                    BX_SYNC_TIME_IF_SINGLE_PROCESSOR(BX_REPEAT_TIME_UPDATE_INTERVAL);
                }
            }
    }

    BX_CPU_THIS_PTR assert_RF();

    RIP = BX_CPU_THIS_PTR prev_rip; // repeat loop not done, restore RIP

    // assert magic async_event to stop trace execution
    BX_CPU_THIS_PTR async_event |= BX_ASYNC_EVENT_STOP_TRACE;
}

void BX_CPU_C::prefetch(void)
{
    bx_address laddr;
    unsigned pageOffset;

    INC_ICACHE_STAT(iCachePrefetch);

#if BX_SUPPORT_X86_64
    if (0) {
        
    }
    else
#endif
    {

#if BX_CPU_LEVEL >= 5
    
#endif

        BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RIP); /* avoid 32-bit EIP wrap */
        laddr = get_laddr32(BX_SEG_REG_CS, EIP);
        pageOffset = PAGE_OFFSET(laddr);

        // Calculate RIP at the beginning of the page.
        BX_CPU_THIS_PTR eipPageBias = (bx_address)pageOffset - EIP;

        Bit32u limit = BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled;

        BX_CPU_THIS_PTR eipPageWindowSize = 4096;
        if (limit + BX_CPU_THIS_PTR eipPageBias < 4096) {
            BX_CPU_THIS_PTR eipPageWindowSize = (Bit32u)(limit + BX_CPU_THIS_PTR eipPageBias + 1);
        }
    }
#if BX_X86_DEBUGGER //调试if(0)
    if (0) {
    }
    else {
        clear_event(BX_EVENT_CODE_BREAKPOINT_ASSIST);
    }
#endif

    BX_CPU_THIS_PTR clear_RF();

    bx_address lpf = LPFOf(laddr);
    bx_TLB_entry* tlbEntry = BX_ITLB_ENTRY_OF(laddr);
    Bit8u* fetchPtr = 0;
    if (0) {
        
    }
    else {
        bx_phy_address pAddr = translate_linear(tlbEntry, laddr, USER_PL, BX_EXECUTE);
        BX_CPU_THIS_PTR pAddrFetchPage = PPFOf(pAddr);
    }

    if (fetchPtr) {
        BX_CPU_THIS_PTR eipFetchPtr = fetchPtr;
    }
    else {
        BX_CPU_THIS_PTR eipFetchPtr = (const Bit8u*)getHostMemAddr(BX_CPU_THIS_PTR pAddrFetchPage, BX_EXECUTE);

    }
}

