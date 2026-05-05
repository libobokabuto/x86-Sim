#include "bochs.h"
#include "cpu.h"

#include "cpustats.h" //29ÐÐ


void BX_CPU_C::cpu_loop(void)
{
    while (1){

        bxICacheEntry_c* entry = getICacheEntry();
        bxInstruction_c* i = entry->i;

        bxInstruction_c* last = i + (entry->tlen);

        if (++i == last) {
            entry = getICacheEntry();
            i = entry->i;
            last = i + (entry->tlen);
        }
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