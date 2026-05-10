#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#include "memory-bochs.h"
#include "pc_system.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_SUPPORT_APIC
#include "apic.h"
#endif

#if BX_SUPPORT_SVM
#include "svm.h"
#endif

#include "debug.h"
#define BX_PAGING_PHY_ADDRESS_RESERVED_BITS (BX_PHY_ADDRESS_RESERVED_BITS & BX_CONST64(0xfffffffffffff)) //323

void BX_CPU_C::TLB_flush(void)
{
    /*
    INC_TLBFLUSH_STAT(tlbGlobalFlushes);

    invalidate_prefetch_q();
    invalidate_stack_cache();

    BX_CPU_THIS_PTR DTLB.flush();
    BX_CPU_THIS_PTR ITLB.flush();

#if BX_SUPPORT_MONITOR_MWAIT
    // invalidating of the TLB might change translation for monitored page
    // and cause subsequent MWAIT instruction to wait forever
    BX_CPU_THIS_PTR wakeup_monitor();
#endif

    // break all links bewteen traces
    BX_CPU_THIS_PTR iCache.breakLinks();
    */
}

#define PAGING_PAE_PDPTE_RESERVED_BITS (BX_PAGING_PHY_ADDRESS_RESERVED_BITS | BX_CONST64(0xFFF00000000001E6)) //954


bool BX_CPP_AttrRegparmN(1) BX_CPU_C::CheckPDPTR(bx_phy_address cr3_val)
{
	// with Nested Paging PDPTRs are not loaded for guest page tables but
	// accessed on demand as part of the guest page walk
    
//#if BX_SUPPORT_SVM
//	if (BX_CPU_THIS_PTR in_svm_guest && SVM_NESTED_PAGING_ENABLED)
//		return true;
//#endif
//
//	cr3_val &= 0xffffffe0;
//#if BX_SUPPORT_VMX >= 2
//	if (BX_CPU_THIS_PTR in_vmx_guest) {
//		if (BX_CPU_THIS_PTR vmcs.vmexec_ctrls2.EPT_ENABLE())
//        
//    
//			cr3_val = translate_guest_physical(cr3_val, 0, false /* laddr_valid */, true /* page walk */, 0, 0, 0, BX_READ);
//            }
// #endif
//
//	Bit64u pdptr[4];
//	unsigned n;
//
//	for (n = 0; n < 4; n++) {
//		// read and check PDPTE entries
//		bx_phy_address pdpe_entry_addr = (bx_phy_address)(cr3_val | (n << 3));
//		pdptr[n] = read_physical_qword(pdpe_entry_addr, BX_MEMTYPE_INVALID, AccessReason(BX_PDPTR0_ACCESS + n));
//
//		if (pdptr[n] & 0x1) {
//			if (pdptr[n] & PAGING_PAE_PDPTE_RESERVED_BITS) return false;
//		}
//	}
//
//	// load new PDPTRs
//	for (n = 0; n < 4; n++)
//		BX_CPU_THIS_PTR PDPTR_CACHE.entry[n] = pdptr[n];
//
//	return true; /* PDPTRs are fine */
    return 0; //自己加的，源码没有这一行，原因是这里被我注释但必须要一个返回值
}
bx_phy_address BX_CPU_C::translate_guest_physical(bx_phy_address guest_paddr, bx_address guest_laddr, bool guest_laddr_valid,
    bool is_page_walk, bool user_page, bool writeable_page, bool nx_page, unsigned rw, bool supervisor_shadow_stack, bool* spp_page)
{//1924
    /*
    VMCS_CACHE* vm = &BX_CPU_THIS_PTR vmcs;
    bx_phy_address entry_addr[4], ppf = LPFOf(vm->eptptr);
    Bit64u entry[4];
    int leaf;

#if BX_SUPPORT_MEMTYPE
    // The MTRRs have no effect on the memory type used for an access to an EPT paging structures.
    BxMemtype eptptr_memtype = BX_CPU_THIS_PTR cr0.get_CD() ? (BX_MEMTYPE_UC) : BxMemtype(vm->eptptr & 0x7);
#endif

    Bit64u offset_mask = BX_CONST64(0x0000ffffffffffff);
    Bit32u combined_access = 0x7, access_mask = 0;
    if (vm->vmexec_ctrls2.MBE_CTRL())
        combined_access |= BX_EPT_MBE_USER_EXECUTE;

    BX_DEBUG(("EPT walk for guest paddr 0x" FMT_PHY_ADDRX, guest_paddr));

    // when EPT A/D enabled treat guest page table accesses as writes
    if (BX_VMX_EPT_ACCESS_DIRTY_ENABLED && is_page_walk && guest_laddr_valid)
        rw = BX_WRITE;

    if (rw == BX_EXECUTE) {
        if (vm->vmexec_ctrls2.MBE_CTRL()) {
            access_mask |= user_page ? BX_EPT_MBE_USER_EXECUTE : BX_EPT_MBE_SUPERVISOR_EXECUTE;
        }
        else {
            access_mask |= BX_EPT_EXECUTE;
        }
    }
    if (rw & 1) access_mask |= BX_EPT_WRITE; // write or r-m-w
    if ((rw & 3) == BX_READ) access_mask |= BX_EPT_READ;  // handle correctly shadow stack reads

    Bit32u vmexit_reason = 0;

    for (leaf = BX_LEVEL_PML4;; --leaf) {
        entry_addr[leaf] = ppf + ((guest_paddr >> (9 + 9 * leaf)) & 0xff8);
        entry[leaf] = read_physical_qword(entry_addr[leaf], MEMTYPE(eptptr_memtype), AccessReason(BX_EPT_PTE_ACCESS + leaf));

        offset_mask >>= 9;
        Bit64u curr_entry = entry[leaf];
        Bit32u curr_access_mask = curr_entry & 0x7;
        if (vm->vmexec_ctrls2.MBE_CTRL()) {
            curr_access_mask |= (curr_entry & BX_EPT_MBE_USER_EXECUTE);
        }

        if (curr_access_mask == BX_EPT_ENTRY_NOT_PRESENT) {
            BX_DEBUG(("EPT %s: not present", bx_paging_level[leaf]));
            vmexit_reason = VMX_VMEXIT_EPT_VIOLATION;
            break;
        }

        if ((curr_access_mask & (BX_EPT_READ | BX_EPT_WRITE)) == BX_EPT_ENTRY_WRITE_ONLY) {
            BX_DEBUG(("EPT %s: EPT misconfiguration access_mask=%x", bx_paging_level[leaf], curr_access_mask));
            vmexit_reason = VMX_VMEXIT_EPT_MISCONFIGURATION;
            break;
        }

        extern bool isMemTypeValidMTRR(unsigned memtype);
        if (!isMemTypeValidMTRR((curr_entry >> 3) & 7)) {
            BX_DEBUG(("EPT %s: EPT misconfiguration memtype=%d",
                bx_paging_level[leaf], (unsigned)((curr_entry >> 3) & 7)));
            vmexit_reason = VMX_VMEXIT_EPT_MISCONFIGURATION;
            break;
        }

        if (curr_entry & PAGING_EPT_RESERVED_BITS) {
            BX_DEBUG(("EPT %s: reserved bit is set 0x" FMT_ADDRX64 "(reserved: " FMT_ADDRX64 ")", bx_paging_level[leaf], curr_entry, curr_entry & PAGING_EPT_RESERVED_BITS));
            vmexit_reason = VMX_VMEXIT_EPT_MISCONFIGURATION;
            break;
        }

        ppf = curr_entry & BX_CONST64(0x000ffffffffff000);

        if (leaf == BX_LEVEL_PTE) break;

        if (curr_entry & 0x80) {
            if (leaf > (BX_LEVEL_PDE + !!is_cpu_extension_supported(BX_ISA_1G_PAGES))) {
                BX_DEBUG(("EPT %s: PS bit set !", bx_paging_level[leaf]));
                vmexit_reason = VMX_VMEXIT_EPT_MISCONFIGURATION;
                break;
            }

            ppf &= BX_CONST64(0x000fffffffffe000);
            if (ppf & offset_mask) {
                BX_DEBUG(("EPT %s: reserved bit is set: 0x" FMT_ADDRX64, bx_paging_level[leaf], curr_entry));
                vmexit_reason = VMX_VMEXIT_EPT_MISCONFIGURATION;
                break;
            }

            // Make up the physical page frame address
            ppf += (bx_phy_address)(guest_paddr & offset_mask);
            break;
        }

        // EPT non leaf entry, check for reserved bits
        if ((curr_entry >> 3) & 0xf) {
            BX_DEBUG(("EPT %s: EPT misconfiguration, reserved bits set for non-leaf entry", bx_paging_level[leaf]));
            vmexit_reason = VMX_VMEXIT_EPT_MISCONFIGURATION;
            break;
        }

        combined_access &= curr_access_mask;
    }

    // defer final combined_access calculation (with leaf entry) until CET is handled

    if (!vmexit_reason) {
#if BX_SUPPORT_CET
        if (BX_VMX_EPT_SUPERVISOR_SHADOW_STACK_CTRL_ENABLED && supervisor_shadow_stack) {
            // The EPT.R bit is set in all EPT paging-structure entry controlling the translation
            // The EPT.W bit is set in all EPT paging-structure entry controlling the translation ignoring the leaf entry (allowed for shadow stack write access)
            // The SSS bit (bit 60) is 1 in the EPT paging-structure entry maps the page
            bool supervisor_shadow_stack_page = ((combined_access & BX_EPT_ENTRY_READ_WRITE) == BX_EPT_ENTRY_READ_WRITE) &&
                ((entry[leaf] & BX_EPT_READ) != 0) &&
                ept_supervisor_shadow_stack_page_bit(entry[leaf]);
            if (!supervisor_shadow_stack_page) {
                BX_ERROR(("VMEXIT: supervisor shadow stack access to non supervisor shadow stack page"));
                vmexit_reason = VMX_VMEXIT_EPT_VIOLATION;
            }
        }
        else
#endif
        {
            combined_access &= entry[leaf];
            if ((access_mask & combined_access) != access_mask) {
                vmexit_reason = VMX_VMEXIT_EPT_VIOLATION;
                if (vm->vmexec_ctrls2.SUBPAGE_WR_PROTECT_CTRL() && ept_spp_bit(entry[leaf]) && leaf == BX_LEVEL_PTE) {
                    // if cumulative read-access bit is 0, the write access is not eligible for SPP
                    if ((access_mask & BX_EPT_WRITE) != 0 && (combined_access & BX_EPT_ENTRY_READ_WRITE) == BX_EPT_ENTRY_READ_ONLY && guest_laddr_valid && !is_page_walk) {
                        if (spp_walk(guest_paddr, guest_laddr, BX_MEMTYPE_WB)) { // memory type indicated in IA32_VMX_BASIC MSR
                            if (spp_page) *spp_page = true;
                            vmexit_reason = 0;
                        }
                    }
                }
            }
        }
    }

    if (vmexit_reason) {
        BX_ERROR(("VMEXIT: EPT %s for guest paddr 0x" FMT_PHY_ADDRX " laddr 0x" FMT_ADDRX,
            (vmexit_reason == VMX_VMEXIT_EPT_VIOLATION) ? "violation" : "misconfig", guest_paddr, guest_laddr));

        Bit32u vmexit_qualification = 0;

        // no VMExit qualification for EPT Misconfiguration VMExit
        if (vmexit_reason == VMX_VMEXIT_EPT_VIOLATION) {
            combined_access &= entry[leaf];
            vmexit_qualification = access_mask | (combined_access << 3);
            if (vm->vmexec_ctrls2.MBE_CTRL() && (rw == BX_EXECUTE)) {
                vmexit_qualification &= (0x3f); // reset all bit bits beyond [5:0]
                vmexit_qualification |= (1 << 2); // bit2 indicate the operation was instruction fetch
                if (combined_access & BX_EPT_MBE_USER_EXECUTE)
                    vmexit_qualification |= (1 << 6);
            }
            if (guest_laddr_valid) {
                vmexit_qualification |= (1 << 7);
                if (!is_page_walk) {
                    vmexit_qualification |= (1 << 8);
                    if (BX_SUPPORT_VMX_EXTENSION(BX_VMX_MBE_CONTROL)) {
                        // support of MBE control implies support of advanced VM-exit information for EPT violations
                        if (user_page)
                            vmexit_qualification |= (1 << 9);
                        if (writeable_page)
                            vmexit_qualification |= (1 << 10);
                        if (nx_page)
                            vmexit_qualification |= (1 << 11);
                    }
                }
            }
            if (BX_CPU_THIS_PTR nmi_unblocking_iret)
                vmexit_qualification |= (1 << 12);
#if BX_SUPPORT_CET
            if (rw & 4) // shadow stack access
                vmexit_qualification |= (1 << 13);

            if (BX_VMX_EPT_SUPERVISOR_SHADOW_STACK_CTRL_ENABLED && ept_supervisor_shadow_stack_page_bit(entry[leaf]))
                vmexit_qualification |= (1 << 14);
#endif
            if (vm->vmexec_ctrls2.EPT_VIOLATION_EXCEPTION()) {
                if (!ept_suppress_ept_violation_exception_bit(entry[leaf]))
                    Virtualization_Exception(vmexit_qualification, guest_paddr, guest_laddr);
            }
        }

        VMwrite64(VMCS_64BIT_GUEST_PHYSICAL_ADDR, guest_paddr);
        VMwrite_natural(VMCS_GUEST_LINEAR_ADDR, guest_laddr);
        VMexit(vmexit_reason, vmexit_qualification);
    }

    if (BX_VMX_EPT_ACCESS_DIRTY_ENABLED) {
        // write access and Dirty-bit is not set in the leaf entry
        unsigned dirty_update = (rw & 1) && !(entry[leaf] & 0x200);
        if (vm->vmexec_ctrls2.PML_ENABLE())
            vmx_page_modification_logging(guest_laddr, guest_paddr, dirty_update);

        update_ept_access_dirty(entry_addr, entry, MEMTYPE(eptptr_memtype), leaf, rw & 1);
    }

    Bit32u page_offset = PAGE_OFFSET(guest_paddr);
    return ppf | page_offset;
    */
return 0; //自己加的，源码没有这一行，原因是这里被我注释但必须要一个返回值
}

bx_phy_address BX_CPU_C::translate_linear(bx_TLB_entry* tlbEntry, bx_address laddr, unsigned user, unsigned rw)
{
	UNUSED(tlbEntry);
	UNUSED(user);
	UNUSED(rw);

#if BX_SUPPORT_X86_64
	if (!long_mode()) laddr &= 0xffffffff;
#endif
    //现在要补
	// 当前先只补“未开启分页”的取指路径，足够跑第一条 BIOS 指令
	bx_phy_address paddress = (bx_phy_address)laddr;
	paddress = A20ADDR(paddress);

	return paddress;
}


bx_hostpageaddr_t BX_CPU_C::getHostMemAddr(bx_phy_address paddr, unsigned rw)
{
	//2491行
	return (bx_hostpageaddr_t)BX_MEM(0)->getHostMemAddr(BX_CPU_THIS, paddr, rw);
}

void BX_CPU_C::access_read_physical(bx_phy_address paddr, unsigned len, void* data)
{
	//2506
    /*
#if BX_SUPPORT_VMX && BX_SUPPORT_X86_64
	if (is_virtual_apic_page(paddr)) {
		paddr = VMX_Virtual_Apic_Read(paddr, len, data);
	}
#endif

#if BX_SUPPORT_APIC
	if (BX_CPU_THIS_PTR lapic->is_selected(paddr)) {
		BX_CPU_THIS_PTR lapic->read(paddr, data, len);
		return;
	}
#endif

	BX_MEM(0)->readPhysicalPage(BX_CPU_THIS, paddr, len, data);
    */
}

Bit64u BX_CPU_C::read_physical_qword(bx_phy_address paddr, BxMemtype memtype, AccessReason reason)
{
    //2548
    Bit64u data;
    access_read_physical(paddr, 8, (Bit8u*)(&data));
    BX_NOTIFY_PHY_MEMORY_ACCESS(paddr, 8, memtype, BX_READ, reason, (Bit8u*)(&data));
    return data;
}

void BX_CPU_C::access_write_physical(bx_phy_address paddr, unsigned len, void* data)
{   //2556
    /*
#if BX_SUPPORT_VMX && BX_SUPPORT_X86_64
    if (is_virtual_apic_page(paddr)) {
        VMX_Virtual_Apic_Write(paddr, len, data);
        return;
    }
#endif

#if BX_SUPPORT_APIC
    if (BX_CPU_THIS_PTR lapic->is_selected(paddr)) {
        BX_CPU_THIS_PTR lapic->write(paddr, data, len);
        return;
    }
#endif

    BX_MEM(0)->writePhysicalPage(BX_CPU_THIS, paddr, len, data);

    */
}
