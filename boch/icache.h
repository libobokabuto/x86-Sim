#pragma once
class bxInstruction_c;

class bxPageWriteStampTable
{
	//29
	Bit32u* fineGranularityMapping;
public:


BX_CPP_INLINE static Bit32u hash(bx_phy_address pAddr) {
	//41
	// can share writeStamps between multiple pages if >32 bit phy address
	return ((Bit32u)pAddr) >> 12;
}

BX_CPP_INLINE void markICacheMask(bx_phy_address pAddr, Bit32u mask)
{
	//59
	fineGranularityMapping[hash(pAddr)] |= mask;
}

};
#define BxICacheEntries (64  * 1024) //104行
#define BxICacheMemPool (576 * 1024) //105行

struct bxICacheEntry_c  //107行
{
	bx_phy_address pAddr; // Physical address of the instruction
	Bit32u traceMask;

	Bit32u tlen;          // Trace length in instructions
	bxInstruction_c* i;
};

#define BX_MAX_TRACE_LENGTH 32  //116

static const bx_phy_address BX_ICACHE_INVALID_PHY_ADDRESS = bx_phy_address(-1);//118

class BOCHSAPI bxICache_c { //122-199行
public:
	bxICacheEntry_c entry[BxICacheEntries];
	bxInstruction_c mpool[BxICacheMemPool];
	unsigned mpindex;

#define BX_ICACHE_PAGE_SPLIT_ENTRIES 8 
	struct pageSplitEntryIndex {
		bx_phy_address ppf; // Physical address of 2nd page of the trace
		bxICacheEntry_c* e; // Pointer to icache entry
	} pageSplitIndex[BX_ICACHE_PAGE_SPLIT_ENTRIES];
	int nextPageSplitIndex;

	BX_CPP_INLINE bxICacheEntry_c* find_entry(bx_phy_address pAddr, unsigned fetchModeMask)
	{
		return 0;
	}

	BX_CPP_INLINE static unsigned hash(bx_phy_address pAddr, unsigned fetchModeMask)
	{
		//140
		//  return ((pAddr + (pAddr << 2) + (pAddr>>6)) & (BxICacheEntries-1)) ^ fetchModeMask;
		return ((pAddr) & (BxICacheEntries - 1)) ^ fetchModeMask;
	}

	BX_CPP_INLINE void alloc_trace(bxICacheEntry_c* e)
	{
		//146
		// took +1 garbend for instruction chaining speedup (end-of-trace opcode)
		if (0) {
			
		}
		e->i = &mpool[mpindex];
		e->tlen = 0;
	}
	BX_CPP_INLINE void commit_trace(unsigned len) { mpindex += len; } //156

	BX_CPP_INLINE void commit_page_split_trace(bx_phy_address paddr, bxICacheEntry_c* e)
	{
		mpindex += e->tlen;

		// register page split entry
		if (pageSplitIndex[nextPageSplitIndex].ppf != BX_ICACHE_INVALID_PHY_ADDRESS)
			//pageSplitIndex[nextPageSplitIndex].e->pAddr = BX_ICACHE_INVALID_PHY_ADDRESS;

		//pageSplitIndex[nextPageSplitIndex].ppf = paddr;
		//pageSplitIndex[nextPageSplitIndex].e = e;

		nextPageSplitIndex = (nextPageSplitIndex + 1) & (BX_ICACHE_PAGE_SPLIT_ENTRIES - 1);
	}

	BX_CPP_INLINE bxICacheEntry_c* get_entry(bx_phy_address pAddr, unsigned fetchModeMask)
	{
		//176
		return &(entry[hash(pAddr, fetchModeMask)]);
	}
};
