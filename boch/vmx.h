#pragma once

#include "vmx_ctrls.h"
enum VMFunctions {
    VMX_VMFUNC_EPTP_SWITCHING = 0 //197
};
const Bit64u VMX_VMFUNC_EPTP_SWITCHING_MASK = (BX_CONST64(1) << VMX_VMFUNC_EPTP_SWITCHING);

class VMCS_Mapping {  //563

};
typedef struct bx_VMX_Cap //717
{
    Bit32u vmx_pin_vmexec_ctrl_supported_bits;
    Bit32u vmx_proc_vmexec_ctrl_supported_bits;
    Bit32u vmx_vmexec_ctrl2_supported_bits;
    Bit64u vmx_vmexec_ctrl3_supported_bits;
#if BX_SUPPORT_VMX >= 2
    Bit64u vmx_ept_vpid_cap_supported_bits;
    Bit64u vmx_vmfunc_supported_bits;
#endif
} VMX_CAP;
typedef struct bx_VMCS   //750
{
    //750
    VmxPinBasedVmexecControls pin_vmexec_ctrls;
    VmxVmexec3Controls vmexec_ctrls3;

} VMCS_CACHE;