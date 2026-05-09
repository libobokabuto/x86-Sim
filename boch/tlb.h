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


#define isReadOK(tlbEntry, user) \
  (tlbEntry->accessBits & (0x01 << unsigned(user)))  //116

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

	BX_CPP_INLINE Bit32u get_memtype() const { return MEMTYPE(memtype); }
};
template <unsigned size>
struct TLB {
	//179行
	bx_TLB_entry entry[size];
	public:
		BX_CPP_INLINE unsigned get_index_of(bx_address lpf, unsigned len = 0)
		{
			const Bit32u tlb_mask = ((size - 1) << 12);
			return (((unsigned(lpf) + len) & tlb_mask) >> 12);
		}

		BX_CPP_INLINE bx_TLB_entry* get_entry_of(bx_address lpf, unsigned len = 0)
		{
			return &entry[get_index_of(lpf, len)];
		}
};