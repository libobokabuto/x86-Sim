#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#include "cpuid.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_SUPPORT_VMX >= 2
void BX_CPU_C::vmfunc_eptp_switching(void)
{
    Bit32u eptp_list_entry = ECX;

    if (eptp_list_entry >= 512) {
        //BX_ERROR(("vmfunc_eptp_switching: invalid EPTP list entry %d", eptp_list_entry));
        VMexit(VMX_VMEXIT_VMFUNC, 0);
    }

    VMCS_CACHE* vm = &BX_CPU_THIS_PTR vmcs;
    bx_phy_address paddr = vm->eptp_list_address + 8 * ECX;

    Bit64u temp_eptp = read_physical_qword(paddr, MEMTYPE(resolve_memtype(paddr)), BX_ACCESS_REASON_NOT_SPECIFIED);
    if (!is_eptptr_valid(temp_eptp)) {
        //BX_ERROR(("vmfunc_eptp_switching: invalid EPTP value in EPTP entry %d", ECX));
        VMexit(VMX_VMEXIT_VMFUNC, 0);
    }

    vm->eptptr = temp_eptp;
    VMwrite64(VMCS_64BIT_CONTROL_EPTPTR, temp_eptp);
    TLB_flush();

    if (BX_SUPPORT_VMX_EXTENSION(BX_VMX_EPT_EXCEPTION)) {
        vm->eptp_index = eptp_list_entry /* & 0xffff */ /* not needed because eptp_list_entry < 512 */;
        VMwrite16(VMCS_16BIT_CONTROL_EPTP_INDEX, vm->eptp_index);
    }
}
#endif