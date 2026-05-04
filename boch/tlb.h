#pragma once

#if BX_SUPPORT_X86_64
const bx_address LPF_MASK = BX_CONST64(0xfffffffffffff000); //28行
#else
const bx_address LPF_MASK = 0xfffff000;
#endif
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

struct bx_TLB_entry
{
	//152行
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