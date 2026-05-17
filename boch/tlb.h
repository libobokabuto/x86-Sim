#pragma once

#if BX_SUPPORT_X86_64
const bx_address LPF_MASK = BX_CONST64(0xfffffffffffff000);
#else
const bx_address LPF_MASK = 0xfffff000;
#endif
BX_CPP_INLINE bx_address AlignedAccessLPFOf(bx_address laddr, unsigned alignment_mask)
{
	//47
	return laddr & (LPF_MASK | alignment_mask);
}
#define BX_DTLB_ENTRY_OF(lpf, len) (BX_CPU_THIS_PTR DTLB.get_entry_of((lpf), (len))) //58

#if BX_PHY_ADDRESS_LONG
const bx_phy_address PPF_MASK = BX_CONST64(0xfffffffffffff000); //29行
#else
const bx_phy_address PPF_MASK = 0xfffff000;
#endif

BX_CPP_INLINE Bit32u PAGE_OFFSET(bx_address laddr) //39行
{
	return Bit32u(laddr) & 0xfff;
}

BX_CPP_INLINE bx_address LPFOf(bx_address laddr) { return laddr & LPF_MASK; } //44行
BX_CPP_INLINE bx_address PPFOf(bx_phy_address paddr) { return paddr & PPF_MASK; } //45行
#define BX_ITLB_ENTRY_OF(lpf) (BX_CPU_THIS_PTR ITLB.get_entry_of(lpf)) //61行

typedef bx_ptr_equiv_t bx_hostpageaddr_t;//64行

#if BX_SUPPORT_X86_64
const bx_address BX_INVALID_TLB_ENTRY = BX_CONST64(0xffffffffffffffff);
#else
const bx_address BX_INVALID_TLB_ENTRY = 0xffffffff;
#endif

const Bit32u TLB_SysReadOK = 0x01;
const Bit32u TLB_UserReadOK = 0x02;
const Bit32u TLB_SysWriteOK = 0x04;
const Bit32u TLB_UserWriteOK = 0x08;

const Bit32u TLB_SysReadShadowStackOK = 0x10;
const Bit32u TLB_UserReadShadowStackOK = 0x20;
const Bit32u TLB_SysWriteShadowStackOK = 0x40;
const Bit32u TLB_UserWriteShadowStackOK = 0x80;

// accessBits in ITLB
const Bit32u TLB_SysExecuteOK = 0x01;
const Bit32u TLB_UserExecuteOK = 0x02;
// global
const Bit32u TLB_GlobalPage = 0x80000000;

#define isWriteOK(tlbEntry, user) \
  (tlbEntry->accessBits & (0x04 << unsigned(user)))

#define isReadOK(tlbEntry, user) \
  (tlbEntry->accessBits & (0x01 << unsigned(user)))  //116
#if BX_SUPPORT_CET
#define isShadowStackWriteOK(tlbEntry, user) \
  (tlbEntry->accessBits & (0x40 << unsigned(user)))
#define isShadowStackReadOK(tlbEntry, user) \
  (tlbEntry->accessBits & (0x10 << unsigned(user)))
#endif
enum {
	BX_MEMTYPE_UC = 0,
	BX_MEMTYPE_WC = 1,
	BX_MEMTYPE_RESERVED2 = 2,
	BX_MEMTYPE_RESERVED3 = 3,
	BX_MEMTYPE_WT = 4,
	BX_MEMTYPE_WP = 5,
	BX_MEMTYPE_WB = 6,
	BX_MEMTYPE_UC_WEAK = 7, // PAT only
	BX_MEMTYPE_INVALID = 8
};  //141

typedef unsigned BxMemtype; //143

#if BX_SUPPORT_MEMTYPE
#define MEMTYPE(memtype) (memtype)
#else
#define MEMTYPE(memtype) (BX_MEMTYPE_UC)
#endif

struct bx_TLB_entry
{
	//152行
	bx_address lpf;       // linear page frame
	bx_phy_address ppf;   // physical page frame
	bx_hostpageaddr_t hostPageAddr;
	Bit32u accessBits;
	Bit32u lpf_mask;
	bx_TLB_entry() { invalidate(); }

	BX_CPP_INLINE bool valid() const { return lpf != BX_INVALID_TLB_ENTRY; }

	BX_CPP_INLINE void invalidate() {
		lpf = BX_INVALID_TLB_ENTRY;
		accessBits = 0;
	}
	BX_CPP_INLINE Bit32u get_memtype() const { return MEMTYPE(memtype); }
};
template <unsigned size>
struct TLB {
	//179行
	bx_TLB_entry entry[size];
#if BX_CPU_LEVEL >= 5
	bool split_large;
#endif

	public:
		TLB() { flush(); }

		BX_CPP_INLINE void flush(void)
		{
			for (unsigned n = 0; n < size; n++)
			    entry[n].invalidate();

		#if BX_CPU_LEVEL >= 5
			split_large = false;  // flushing whole TLB
		#endif
		}
		BX_CPP_INLINE unsigned get_index_of(bx_address lpf, unsigned len = 0)
		{
			const Bit32u tlb_mask = ((size - 1) << 12);
			return (((unsigned(lpf) + len) & tlb_mask) >> 12);
		}

		BX_CPP_INLINE bx_TLB_entry* get_entry_of(bx_address lpf, unsigned len = 0)
		{
			return &entry[get_index_of(lpf, len)];
		}

#if BX_CPU_LEVEL >= 6
		BX_CPP_INLINE void flushNonGlobal(void)
		{
			Bit32u lpf_mask = 0;

			for (unsigned n = 0; n < size; n++) {
				bx_TLB_entry* tlbEntry = &entry[n];
				if (tlbEntry->valid()) {
					if (!(tlbEntry->accessBits & TLB_GlobalPage))
						tlbEntry->invalidate();
					else
						lpf_mask |= tlbEntry->lpf_mask;
				}
			}

			split_large = (lpf_mask > 0xfff);
		}
#endif
};