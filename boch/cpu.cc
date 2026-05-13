#include "bochs.h"
#include "cpu.h"
#include "MD5.h"
#include "cpustats.h" //29ÐÐ
#include "pc_system.h"
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
    while (1){

        bxICacheEntry_c* entry = getICacheEntry();
        bxInstruction_c* i = entry->i;

        bxInstruction_c* last = i + (entry->tlen);

        for (;;) {

            // want to allow changing of the instruction inside instrumentation callback
            BX_INSTR_BEFORE_EXECUTION(BX_CPU_ID, i);
            RIP += i->ilen();
            BX_CPU_CALL_METHOD(i->execute1, (i)); // might iterate repeat instruction
            BX_CPU_THIS_PTR prev_rip = RIP; // commit new RIP
            BX_INSTR_AFTER_EXECUTION(BX_CPU_ID, i);
            BX_CPU_THIS_PTR icount++;
            if (BX_CPU_THIS_PTR icount == 1789)
            {
                int qwq = 0;
            }
            

            if (1)
            {
                int memdatalen = 8;

                memcpy(g_test_buff, BX_CPU_THIS_PTR gen_reg, cpudatalen);
                uint8_t* pTarGet = g_test_buff + cpudatalen;

                unsigned char* qwq = (unsigned char*)g_test_buff;
                unsigned char decrypt[16];
                MD5_CTX md5;
                MD5Init(&md5);
                //MD5Update(&md5, qwq, (32 + 1) * 8);
                MD5Update(&md5, qwq, cpudatalen);

                MD5Final(&md5, decrypt);
                printf("%05d: ", BX_CPU_THIS_PTR icount);
                for (int j = 0; j < 16; j++)
                {
                    printf("%02x", decrypt[j]);
                }
                printf("\n");

                md5count++;
            }



            //BX_SYNC_TIME_IF_SINGLE_PROCESSOR(0);//Ê±ÖÓ

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
#if BX_X86_DEBUGGER //µ÷ÊÔif(0)
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