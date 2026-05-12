#pragma once

#include "vmx_ctrls.h"

enum VMX_vmexit_reason {
    VMX_VMEXIT_EXCEPTION_NMI = 0,
    VMX_VMEXIT_EXTERNAL_INTERRUPT = 1,
    VMX_VMEXIT_TRIPLE_FAULT = 2,
    VMX_VMEXIT_INIT = 3,
    VMX_VMEXIT_SIPI = 4,
    VMX_VMEXIT_IO_SMI = 5,
    VMX_VMEXIT_SMI = 6,
    VMX_VMEXIT_INTERRUPT_WINDOW = 7,
    VMX_VMEXIT_NMI_WINDOW = 8,
    VMX_VMEXIT_TASK_SWITCH = 9,
    VMX_VMEXIT_CPUID = 10,
    VMX_VMEXIT_GETSEC = 11,
    VMX_VMEXIT_HLT = 12,
    VMX_VMEXIT_INVD = 13,
    VMX_VMEXIT_INVLPG = 14,
    VMX_VMEXIT_RDPMC = 15,
    VMX_VMEXIT_RDTSC = 16,
    VMX_VMEXIT_RSM = 17,
    VMX_VMEXIT_VMCALL = 18,
    VMX_VMEXIT_VMCLEAR = 19,
    VMX_VMEXIT_VMLAUNCH = 20,
    VMX_VMEXIT_VMPTRLD = 21,
    VMX_VMEXIT_VMPTRST = 22,
    VMX_VMEXIT_VMREAD = 23,
    VMX_VMEXIT_VMRESUME = 24,
    VMX_VMEXIT_VMWRITE = 25,
    VMX_VMEXIT_VMXOFF = 26,
    VMX_VMEXIT_VMXON = 27,
    VMX_VMEXIT_CR_ACCESS = 28,
    VMX_VMEXIT_DR_ACCESS = 29,
    VMX_VMEXIT_IO_INSTRUCTION = 30,
    VMX_VMEXIT_RDMSR = 31,
    VMX_VMEXIT_WRMSR = 32,
    VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE = 33,
    VMX_VMEXIT_VMENTRY_FAILURE_MSR = 34,
    VMX_VMEXIT_RESERVED35 = 35,
    VMX_VMEXIT_MWAIT = 36,
    VMX_VMEXIT_MONITOR_TRAP_FLAG = 37,
    VMX_VMEXIT_RESERVED38 = 38,
    VMX_VMEXIT_MONITOR = 39,
    VMX_VMEXIT_PAUSE = 40,
    VMX_VMEXIT_VMENTRY_FAILURE_MCA = 41, // will never happen in Bochs
    VMX_VMEXIT_RESERVED42 = 42,
    VMX_VMEXIT_TPR_THRESHOLD = 43,
    VMX_VMEXIT_APIC_ACCESS = 44,
    VMX_VMEXIT_VIRTUALIZED_EOI = 45,
    VMX_VMEXIT_GDTR_IDTR_ACCESS = 46,
    VMX_VMEXIT_LDTR_TR_ACCESS = 47,
    VMX_VMEXIT_EPT_VIOLATION = 48,
    VMX_VMEXIT_EPT_MISCONFIGURATION = 49,
    VMX_VMEXIT_INVEPT = 50,
    VMX_VMEXIT_RDTSCP = 51,
    VMX_VMEXIT_VMX_PREEMPTION_TIMER_EXPIRED = 52,
    VMX_VMEXIT_INVVPID = 53,
    VMX_VMEXIT_WBINVD = 54,
    VMX_VMEXIT_XSETBV = 55,
    VMX_VMEXIT_APIC_WRITE = 56,
    VMX_VMEXIT_RDRAND = 57,
    VMX_VMEXIT_INVPCID = 58,
    VMX_VMEXIT_VMFUNC = 59,
    VMX_VMEXIT_ENCLS = 60,
    VMX_VMEXIT_RDSEED = 61,
    VMX_VMEXIT_PML_LOGFULL = 62,
    VMX_VMEXIT_XSAVES = 63,
    VMX_VMEXIT_XRSTORS = 64,
    VMX_VMEXIT_PCONFIG = 65,
    VMX_VMEXIT_SPP = 66,
    VMX_VMEXIT_UMWAIT = 67,
    VMX_VMEXIT_TPAUSE = 68,
    VMX_VMEXIT_LOADIWKEY = 69,
    VMX_VMEXIT_ENCLV = 70,
    VMX_VMEXIT_RESERVED71 = 71,
    VMX_VMEXIT_ENQCMD_PASID = 72,
    VMX_VMEXIT_ENQCMDS_PASID = 73,
    VMX_VMEXIT_BUS_LOCK = 74,
    VMX_VMEXIT_NOTIFY_WINDOW = 75,
    VMX_VMEXIT_SEAMCALL = 76,
    VMX_VMEXIT_TDCALL = 77,
    VMX_VMEXIT_RDMSRLIST = 78,
    VMX_VMEXIT_WRMSRLIST = 79,
    VMX_VMEXIT_URDMSR = 80,
    VMX_VMEXIT_UWRMSR = 81,
    VMX_VMEXIT_LAST_REASON
};

#define IS_TRAP_LIKE_VMEXIT(reason) \
      (reason == VMX_VMEXIT_TPR_THRESHOLD || \
       reason == VMX_VMEXIT_VIRTUALIZED_EOI || \
       reason == VMX_VMEXIT_APIC_WRITE || \
       reason == VMX_VMEXIT_BUS_LOCK)

enum VMX_vmabort_code {
    VMABORT_SAVING_GUEST_MSRS_FAILURE = 0,
    VMABORT_HOST_PDPTR_CORRUPTED,
    VMABORT_VMEXIT_VMCS_CORRUPTED,
    VMABORT_LOADING_HOST_MSRS,
    VMABORT_VMEXIT_MACHINE_CHECK_ERROR
};

const Bit32u VMX_APIC_READ_INSTRUCTION_EXECUTION = 0x0000;
const Bit32u VMX_APIC_WRITE_INSTRUCTION_EXECUTION = 0x1000;
const Bit32u VMX_APIC_INSTRUCTION_FETCH = 0x2000; /* won't happen because cpu::prefetch will crash */
const Bit32u VMX_APIC_ACCESS_DURING_EVENT_DELIVERY = 0x3000;

enum VMFunctions {
    VMX_VMFUNC_EPTP_SWITCHING = 0 //197
};
const Bit64u VMX_VMFUNC_EPTP_SWITCHING_MASK = (BX_CONST64(1) << VMX_VMFUNC_EPTP_SWITCHING);
// =============
//  VMCS fields
// =============

/* VMCS 16-bit control fields */
/* binary 0000_00xx_xxxx_xxx0 */
#define VMCS_16BIT_CONTROL_VPID                            0x00000000 /* VPID */
#define VMCS_16BIT_CONTROL_POSTED_INTERRUPT_VECTOR         0x00000002 /* Posted Interrupts */
#define VMCS_16BIT_CONTROL_EPTP_INDEX                      0x00000004 /* #VE Exception */
#define VMCS_16BIT_CONTROL_HLAT_PREFIX                     0x00000006 /* HLAT */
#define VMCS_16BIT_CONTROL_LAST_PID_POINTER_INDEX          0x00000008 /* IPI Virtualization */
#define VMCS_16BIT_CONTROL_VIRTUAL_TIMER_VECTOR            0x0000000A /* APIC timer virtualization (not implemented) */

/* VMCS 16-bit guest-state fields */
/* binary 0000_10xx_xxxx_xxx0 */
#define VMCS_16BIT_GUEST_ES_SELECTOR                       0x00000800
#define VMCS_16BIT_GUEST_CS_SELECTOR                       0x00000802
#define VMCS_16BIT_GUEST_SS_SELECTOR                       0x00000804
#define VMCS_16BIT_GUEST_DS_SELECTOR                       0x00000806
#define VMCS_16BIT_GUEST_FS_SELECTOR                       0x00000808
#define VMCS_16BIT_GUEST_GS_SELECTOR                       0x0000080A
#define VMCS_16BIT_GUEST_LDTR_SELECTOR                     0x0000080C
#define VMCS_16BIT_GUEST_TR_SELECTOR                       0x0000080E
#define VMCS_16BIT_GUEST_INTERRUPT_STATUS                  0x00000810 /* Virtual Interrupt Delivery */
#define VMCS_16BIT_GUEST_PML_INDEX                         0x00000812 /* Page Modification Logging */
#define VMCS_16BIT_GUEST_UINV                              0x00000814 /* UINTR */

/* VMCS 16-bit host-state fields */
/* binary 0000_11xx_xxxx_xxx0 */
#define VMCS_16BIT_HOST_ES_SELECTOR                        0x00000C00
#define VMCS_16BIT_HOST_CS_SELECTOR                        0x00000C02
#define VMCS_16BIT_HOST_SS_SELECTOR                        0x00000C04
#define VMCS_16BIT_HOST_DS_SELECTOR                        0x00000C06
#define VMCS_16BIT_HOST_FS_SELECTOR                        0x00000C08
#define VMCS_16BIT_HOST_GS_SELECTOR                        0x00000C0A
#define VMCS_16BIT_HOST_TR_SELECTOR                        0x00000C0C

/* VMCS 64-bit control fields */
/* binary 0010_00xx_xxxx_xxx0 */
#define VMCS_64BIT_CONTROL_IO_BITMAP_A                        0x00002000
#define VMCS_64BIT_CONTROL_IO_BITMAP_A_HI                     0x00002001
#define VMCS_64BIT_CONTROL_IO_BITMAP_B                        0x00002002
#define VMCS_64BIT_CONTROL_IO_BITMAP_B_HI                     0x00002003
#define VMCS_64BIT_CONTROL_MSR_BITMAPS                        0x00002004
#define VMCS_64BIT_CONTROL_MSR_BITMAPS_HI                     0x00002005
#define VMCS_64BIT_CONTROL_VMEXIT_MSR_STORE_ADDR              0x00002006
#define VMCS_64BIT_CONTROL_VMEXIT_MSR_STORE_ADDR_HI           0x00002007
#define VMCS_64BIT_CONTROL_VMEXIT_MSR_LOAD_ADDR               0x00002008
#define VMCS_64BIT_CONTROL_VMEXIT_MSR_LOAD_ADDR_HI            0x00002009
#define VMCS_64BIT_CONTROL_VMENTRY_MSR_LOAD_ADDR              0x0000200A
#define VMCS_64BIT_CONTROL_VMENTRY_MSR_LOAD_ADDR_HI           0x0000200B
#define VMCS_64BIT_CONTROL_EXECUTIVE_VMCS_PTR                 0x0000200C
#define VMCS_64BIT_CONTROL_EXECUTIVE_VMCS_PTR_HI              0x0000200D
#define VMCS_64BIT_CONTROL_PML_ADDRESS                        0x0000200E /* Page Modification Logging */
#define VMCS_64BIT_CONTROL_PML_ADDRESS_HI                     0x0000200F
#define VMCS_64BIT_CONTROL_TSC_OFFSET                         0x00002010
#define VMCS_64BIT_CONTROL_TSC_OFFSET_HI                      0x00002011
#define VMCS_64BIT_CONTROL_VIRTUAL_APIC_PAGE_ADDR             0x00002012 /* TPR shadow */
#define VMCS_64BIT_CONTROL_VIRTUAL_APIC_PAGE_ADDR_HI          0x00002013
#define VMCS_64BIT_CONTROL_APIC_ACCESS_ADDR                   0x00002014 /* APIC virtualization */
#define VMCS_64BIT_CONTROL_APIC_ACCESS_ADDR_HI                0x00002015
#define VMCS_64BIT_CONTROL_POSTED_INTERRUPT_DESC_ADDR         0x00002016 /* Posted Interrupts */
#define VMCS_64BIT_CONTROL_POSTED_INTERRUPT_DESC_ADDR_HI      0x00002017
#define VMCS_64BIT_CONTROL_VMFUNC_CTRLS                       0x00002018 /* VM Functions */
#define VMCS_64BIT_CONTROL_VMFUNC_CTRLS_HI                    0x00002019
#define VMCS_64BIT_CONTROL_EPTPTR                             0x0000201A /* EPT */
#define VMCS_64BIT_CONTROL_EPTPTR_HI                          0x0000201B
#define VMCS_64BIT_CONTROL_EOI_EXIT_BITMAP0                   0x0000201C /* Virtual Interrupt Delivery */
#define VMCS_64BIT_CONTROL_EOI_EXIT_BITMAP0_HI                0x0000201D
#define VMCS_64BIT_CONTROL_EOI_EXIT_BITMAP1                   0x0000201E
#define VMCS_64BIT_CONTROL_EOI_EXIT_BITMAP1_HI                0x0000201F
#define VMCS_64BIT_CONTROL_EOI_EXIT_BITMAP2                   0x00002020
#define VMCS_64BIT_CONTROL_EOI_EXIT_BITMAP2_HI                0x00002021
#define VMCS_64BIT_CONTROL_EOI_EXIT_BITMAP3                   0x00002022
#define VMCS_64BIT_CONTROL_EOI_EXIT_BITMAP3_HI                0x00002023
#define VMCS_64BIT_CONTROL_EPTP_LIST_ADDRESS                  0x00002024 /* VM Functions - EPTP switching */
#define VMCS_64BIT_CONTROL_EPTP_LIST_ADDRESS_HI               0x00002025
#define VMCS_64BIT_CONTROL_VMREAD_BITMAP_ADDR                 0x00002026 /* VMCS Shadowing */
#define VMCS_64BIT_CONTROL_VMREAD_BITMAP_ADDR_HI              0x00002027
#define VMCS_64BIT_CONTROL_VMWRITE_BITMAP_ADDR                0x00002028 /* VMCS Shadowing */
#define VMCS_64BIT_CONTROL_VMWRITE_BITMAP_ADDR_HI             0x00002029
#define VMCS_64BIT_CONTROL_VE_EXCEPTION_INFO_ADDR             0x0000202A /* #VE Exception */
#define VMCS_64BIT_CONTROL_VE_EXCEPTION_INFO_ADDR_HI          0x0000202B
#define VMCS_64BIT_CONTROL_XSS_EXITING_BITMAP                 0x0000202C /* XSAVES */
#define VMCS_64BIT_CONTROL_XSS_EXITING_BITMAP_HI              0x0000202D
#define VMCS_64BIT_CONTROL_ENCLS_EXITING_BITMAP               0x0000202E /* ENCLS/SGX (not implemented) */
#define VMCS_64BIT_CONTROL_ENCLS_EXITING_BITMAP_HI            0x0000202F
#define VMCS_64BIT_CONTROL_SPPTP                              0x00002030 /* Sup-Page Write Protection */
#define VMCS_64BIT_CONTROL_SPPTP_HI                           0x00002031
#define VMCS_64BIT_CONTROL_TSC_MULTIPLIER                     0x00002032 /* TSC Scaling */
#define VMCS_64BIT_CONTROL_TSC_MULTIPLIER_HI                  0x00002033
#define VMCS_64BIT_CONTROL_TERTIARY_VMEXEC_CONTROLS           0x00002034
#define VMCS_64BIT_CONTROL_TERTIARY_VMEXEC_CONTROLS_HI        0x00002035
#define VMCS_64BIT_CONTROL_ENCLV_EXITING_BITMAP               0x00002036 /* ENCLV/SGX (not implemented) */
#define VMCS_64BIT_CONTROL_ENCLV_EXITING_BITMAP_HI            0x00002037
#define VMCS_64BIT_CONTROL_LO_PASID_DIRECTORY_ADDR            0x00002038
#define VMCS_64BIT_CONTROL_LO_PASID_DIRECTORY_ADDR_HI         0x00002039
#define VMCS_64BIT_CONTROL_HI_PASID_DIRECTORY_ADDR            0x0000203A
#define VMCS_64BIT_CONTROL_HI_PASID_DIRECTORY_ADDR_HI         0x0000203B
#define VMCS_64BIT_CONTROL_SHARED_EPT_POINTER                 0x0000203C
#define VMCS_64BIT_CONTROL_SHARED_EPT_POINTER_HI              0x0000203D
#define VMCS_64BIT_CONTROL_PCONFIG_EXITING_BITMAP             0x0000203E
#define VMCS_64BIT_CONTROL_PCONFIG_EXITING_BITMAP_HI          0x0000203F
#define VMCS_64BIT_CONTROL_HLAT_POINTER                       0x00002040 /* HLAT (not implemented) */
#define VMCS_64BIT_CONTROL_HLAT_POINTER_HI                    0x00002041
#define VMCS_64BIT_CONTROL_PID_POINTER_TABLE_ADDRESS          0x00002042 /* IPI Virtualization (not implemented) */
#define VMCS_64BIT_CONTROL_PID_POINTER_TABLE_ADDRESS_HI       0x00002043
#define VMCS_64BIT_CONTROL_SECONDARY_VMEXIT_CONTROLS          0x00002044
#define VMCS_64BIT_CONTROL_SECONDARY_VMEXIT_CONTROLS_HI       0x00002045
#define VMCS_64BIT_CONTROL_IA32_SPEC_CTRL_MASK                0x0000204A
#define VMCS_64BIT_CONTROL_IA32_SPEC_CTRL_MASK_HI             0x0000204B
#define VMCS_64BIT_CONTROL_IA32_SPEC_CTRL_SHADOW              0x0000204C
#define VMCS_64BIT_CONTROL_IA32_SPEC_CTRL_SHADOW_HI           0x0000204D
#define VMCS_64BIT_CONTROL_GUEST_DEADLINE_SHADOW              0x0000204E /* APIC timer virtualization (not implemented) */
#define VMCS_64BIT_CONTROL_GUEST_DEADLINE_SHADOW_HI           0x0000204F


/* VMCS 64-bit read only data fields */
/* binary 0010_01xx_xxxx_xxx0 */
#define VMCS_64BIT_GUEST_PHYSICAL_ADDR                     0x00002400 /* EPT */
#define VMCS_64BIT_GUEST_PHYSICAL_ADDR_HI                  0x00002401
#define VMCS_64BIT_MSR_DATA                                0x00002402 /* MSRLIST */
#define VMCS_64BIT_MSR_DATA_HI                             0x00002403

/* VMCS 64-bit guest state fields */
/* binary 0010_10xx_xxxx_xxx0 */
#define VMCS_64BIT_GUEST_LINK_POINTER                      0x00002800
#define VMCS_64BIT_GUEST_LINK_POINTER_HI                   0x00002801
#define VMCS_64BIT_GUEST_IA32_DEBUGCTL                     0x00002802
#define VMCS_64BIT_GUEST_IA32_DEBUGCTL_HI                  0x00002803
#define VMCS_64BIT_GUEST_IA32_PAT                          0x00002804 /* PAT */
#define VMCS_64BIT_GUEST_IA32_PAT_HI                       0x00002805
#define VMCS_64BIT_GUEST_IA32_EFER                         0x00002806 /* EFER */
#define VMCS_64BIT_GUEST_IA32_EFER_HI                      0x00002807
#define VMCS_64BIT_GUEST_IA32_PERF_GLOBAL_CTRL             0x00002808 /* Perf Global Ctrl */
#define VMCS_64BIT_GUEST_IA32_PERF_GLOBAL_CTRL_HI          0x00002809
#define VMCS_64BIT_GUEST_IA32_PDPTE0                       0x0000280A /* EPT */
#define VMCS_64BIT_GUEST_IA32_PDPTE0_HI                    0x0000280B
#define VMCS_64BIT_GUEST_IA32_PDPTE1                       0x0000280C
#define VMCS_64BIT_GUEST_IA32_PDPTE1_HI                    0x0000280D
#define VMCS_64BIT_GUEST_IA32_PDPTE2                       0x0000280E
#define VMCS_64BIT_GUEST_IA32_PDPTE2_HI                    0x0000280F
#define VMCS_64BIT_GUEST_IA32_PDPTE3                       0x00002810
#define VMCS_64BIT_GUEST_IA32_PDPTE3_HI                    0x00002811
#define VMCS_64BIT_GUEST_IA32_BNDCFGS                      0x00002812 /* MPX (not implemented) */
#define VMCS_64BIT_GUEST_IA32_BNDCFGS_HI                   0x00002813
#define VMCS_64BIT_GUEST_IA32_RTIT_CTL                     0x00002814 /* Processor Trace (not implemented) */
#define VMCS_64BIT_GUEST_IA32_RTIT_CTL_HI                  0x00002815
#define VMCS_64BIT_GUEST_IA32_PKRS                         0x00002818 /* Supervisor-Mode Protection Keys */
#define VMCS_64BIT_GUEST_IA32_PKRS_HI                      0x00002819
#define VMCS_64BIT_GUEST_IA32_SPEC_CTRL                    0x0000282E /* MSR_IA32_SPEC_CTRL virtualization */
#define VMCS_64BIT_GUEST_IA32_SPEC_CTRL_HI                 0x0000282F
#define VMCS_64BIT_GUEST_DEADLINE                          0x00002830 /* APIC timer virtualization (not implemented) */
#define VMCS_64BIT_GUEST_DEADLINE_HI                       0x00002831

/* VMCS 64-bit host state fields */
/* binary 0010_11xx_xxxx_xxx0 */
#define VMCS_64BIT_HOST_IA32_PAT                           0x00002C00 /* PAT */
#define VMCS_64BIT_HOST_IA32_PAT_HI                        0x00002C01
#define VMCS_64BIT_HOST_IA32_EFER                          0x00002C02 /* EFER */
#define VMCS_64BIT_HOST_IA32_EFER_HI                       0x00002C03
#define VMCS_64BIT_HOST_IA32_PERF_GLOBAL_CTRL              0x00002C04 /* Perf Global Ctrl */
#define VMCS_64BIT_HOST_IA32_PERF_GLOBAL_CTRL_HI           0x00002C05
#define VMCS_64BIT_HOST_IA32_PKRS                          0x00002C06 /* Supervisor-Mode Protection Keys */
#define VMCS_64BIT_HOST_IA32_PKRS_HI                       0x00002C07
#define VMCS_64BIT_HOST_IA32_SPEC_CTRL                     0x00002C1A /* MSR_IA32_SPEC_CTRL virtualization */
#define VMCS_64BIT_HOST_IA32_SPEC_CTRL_HI                  0x00002C1B

/* VMCS 32_bit control fields */
/* binary 0100_00xx_xxxx_xxx0 */
#define VMCS_32BIT_CONTROL_PIN_BASED_EXEC_CONTROLS         0x00004000
#define VMCS_32BIT_CONTROL_PROCESSOR_BASED_VMEXEC_CONTROLS 0x00004002
#define VMCS_32BIT_CONTROL_EXECUTION_BITMAP                0x00004004
#define VMCS_32BIT_CONTROL_PAGE_FAULT_ERR_CODE_MASK        0x00004006
#define VMCS_32BIT_CONTROL_PAGE_FAULT_ERR_CODE_MATCH       0x00004008
#define VMCS_32BIT_CONTROL_CR3_TARGET_COUNT                0x0000400A
#define VMCS_32BIT_CONTROL_VMEXIT_CONTROLS                 0x0000400C
#define VMCS_32BIT_CONTROL_VMEXIT_MSR_STORE_COUNT          0x0000400E
#define VMCS_32BIT_CONTROL_VMEXIT_MSR_LOAD_COUNT           0x00004010
#define VMCS_32BIT_CONTROL_VMENTRY_CONTROLS                0x00004012
#define VMCS_32BIT_CONTROL_VMENTRY_MSR_LOAD_COUNT          0x00004014
#define VMCS_32BIT_CONTROL_VMENTRY_INTERRUPTION_INFO       0x00004016
#define VMCS_32BIT_CONTROL_VMENTRY_EXCEPTION_ERR_CODE      0x00004018
#define VMCS_32BIT_CONTROL_VMENTRY_INSTRUCTION_LENGTH      0x0000401A
#define VMCS_32BIT_CONTROL_TPR_THRESHOLD                   0x0000401C /* TPR shadow */
#define VMCS_32BIT_CONTROL_SECONDARY_VMEXEC_CONTROLS       0x0000401E
#define VMCS_32BIT_CONTROL_PAUSE_LOOP_EXITING_GAP          0x00004020 /* PAUSE loop exiting */
#define VMCS_32BIT_CONTROL_PAUSE_LOOP_EXITING_WINDOW       0x00004022 /* PAUSE loop exiting */

/* VMCS 32-bit read only data fields */
/* binary 0100_01xx_xxxx_xxx0 */
#define VMCS_32BIT_INSTRUCTION_ERROR                       0x00004400
#define VMCS_32BIT_VMEXIT_REASON                           0x00004402
#define VMCS_32BIT_VMEXIT_INTERRUPTION_INFO                0x00004404
#define VMCS_32BIT_VMEXIT_INTERRUPTION_ERR_CODE            0x00004406
#define VMCS_32BIT_IDT_VECTORING_INFO                      0x00004408
#define VMCS_32BIT_IDT_VECTORING_ERR_CODE                  0x0000440A
#define VMCS_32BIT_VMEXIT_INSTRUCTION_LENGTH               0x0000440C
#define VMCS_32BIT_VMEXIT_INSTRUCTION_INFO                 0x0000440E

/* VMCS 32-bit guest-state fields */
/* binary 0100_10xx_xxxx_xxx0 */
#define VMCS_32BIT_GUEST_ES_LIMIT                          0x00004800
#define VMCS_32BIT_GUEST_CS_LIMIT                          0x00004802
#define VMCS_32BIT_GUEST_SS_LIMIT                          0x00004804
#define VMCS_32BIT_GUEST_DS_LIMIT                          0x00004806
#define VMCS_32BIT_GUEST_FS_LIMIT                          0x00004808
#define VMCS_32BIT_GUEST_GS_LIMIT                          0x0000480A
#define VMCS_32BIT_GUEST_LDTR_LIMIT                        0x0000480C
#define VMCS_32BIT_GUEST_TR_LIMIT                          0x0000480E
#define VMCS_32BIT_GUEST_GDTR_LIMIT                        0x00004810
#define VMCS_32BIT_GUEST_IDTR_LIMIT                        0x00004812
#define VMCS_32BIT_GUEST_ES_ACCESS_RIGHTS                  0x00004814
#define VMCS_32BIT_GUEST_CS_ACCESS_RIGHTS                  0x00004816
#define VMCS_32BIT_GUEST_SS_ACCESS_RIGHTS                  0x00004818
#define VMCS_32BIT_GUEST_DS_ACCESS_RIGHTS                  0x0000481A
#define VMCS_32BIT_GUEST_FS_ACCESS_RIGHTS                  0x0000481C
#define VMCS_32BIT_GUEST_GS_ACCESS_RIGHTS                  0x0000481E
#define VMCS_32BIT_GUEST_LDTR_ACCESS_RIGHTS                0x00004820
#define VMCS_32BIT_GUEST_TR_ACCESS_RIGHTS                  0x00004822
#define VMCS_32BIT_GUEST_INTERRUPTIBILITY_STATE            0x00004824
#define VMCS_32BIT_GUEST_ACTIVITY_STATE                    0x00004826
#define VMCS_32BIT_GUEST_SMBASE                            0x00004828
#define VMCS_32BIT_GUEST_IA32_SYSENTER_CS_MSR              0x0000482A
#define VMCS_32BIT_GUEST_PREEMPTION_TIMER_VALUE            0x0000482E /* VMX preemption timer */

/* VMCS 32-bit host-state fields */
/* binary 0100_11xx_xxxx_xxx0 */
#define VMCS_32BIT_HOST_IA32_SYSENTER_CS_MSR               0x00004C00

/* VMCS natural width control fields */
/* binary 0110_00xx_xxxx_xxx0 */
#define VMCS_CONTROL_CR0_GUEST_HOST_MASK                   0x00006000
#define VMCS_CONTROL_CR4_GUEST_HOST_MASK                   0x00006002
#define VMCS_CONTROL_CR0_READ_SHADOW                       0x00006004
#define VMCS_CONTROL_CR4_READ_SHADOW                       0x00006006
#define VMCS_CR3_TARGET0                                   0x00006008
#define VMCS_CR3_TARGET1                                   0x0000600A
#define VMCS_CR3_TARGET2                                   0x0000600C
#define VMCS_CR3_TARGET3                                   0x0000600E

/* VMCS natural width read only data fields */
/* binary 0110_01xx_xxxx_xxx0 */
#define VMCS_VMEXIT_QUALIFICATION                          0x00006400
#define VMCS_IO_RCX                                        0x00006402
#define VMCS_IO_RSI                                        0x00006404
#define VMCS_IO_RDI                                        0x00006406
#define VMCS_IO_RIP                                        0x00006408
#define VMCS_GUEST_LINEAR_ADDR                             0x0000640A

/* VMCS natural width guest state fields */
/* binary 0110_10xx_xxxx_xxx0 */
#define VMCS_GUEST_CR0                                     0x00006800
#define VMCS_GUEST_CR3                                     0x00006802
#define VMCS_GUEST_CR4                                     0x00006804
#define VMCS_GUEST_ES_BASE                                 0x00006806
#define VMCS_GUEST_CS_BASE                                 0x00006808
#define VMCS_GUEST_SS_BASE                                 0x0000680A
#define VMCS_GUEST_DS_BASE                                 0x0000680C
#define VMCS_GUEST_FS_BASE                                 0x0000680E
#define VMCS_GUEST_GS_BASE                                 0x00006810
#define VMCS_GUEST_LDTR_BASE                               0x00006812
#define VMCS_GUEST_TR_BASE                                 0x00006814
#define VMCS_GUEST_GDTR_BASE                               0x00006816
#define VMCS_GUEST_IDTR_BASE                               0x00006818
#define VMCS_GUEST_DR7                                     0x0000681A
#define VMCS_GUEST_RSP                                     0x0000681C
#define VMCS_GUEST_RIP                                     0x0000681E
#define VMCS_GUEST_RFLAGS                                  0x00006820
#define VMCS_GUEST_PENDING_DBG_EXCEPTIONS                  0x00006822
#define VMCS_GUEST_IA32_SYSENTER_ESP_MSR                   0x00006824
#define VMCS_GUEST_IA32_SYSENTER_EIP_MSR                   0x00006826
#define VMCS_GUEST_IA32_S_CET                              0x00006828
#define VMCS_GUEST_SSP                                     0x0000682A
#define VMCS_GUEST_INTERRUPT_SSP_TABLE_ADDR                0x0000682C

/* VMCS natural width host state fields */
/* binary 0110_11xx_xxxx_xxx0 */
#define VMCS_HOST_CR0                                      0x00006C00
#define VMCS_HOST_CR3                                      0x00006C02
#define VMCS_HOST_CR4                                      0x00006C04
#define VMCS_HOST_FS_BASE                                  0x00006C06
#define VMCS_HOST_GS_BASE                                  0x00006C08
#define VMCS_HOST_TR_BASE                                  0x00006C0A
#define VMCS_HOST_GDTR_BASE                                0x00006C0C
#define VMCS_HOST_IDTR_BASE                                0x00006C0E
#define VMCS_HOST_IA32_SYSENTER_ESP_MSR                    0x00006C10
#define VMCS_HOST_IA32_SYSENTER_EIP_MSR                    0x00006C12
#define VMCS_HOST_RSP                                      0x00006C14
#define VMCS_HOST_RIP                                      0x00006C16
#define VMCS_HOST_IA32_S_CET                               0x00006C18
#define VMCS_HOST_SSP                                      0x00006C1A
#define VMCS_HOST_INTERRUPT_SSP_TABLE_ADDR                 0x00006C1C

class VMCS_Mapping {  //563

};
typedef struct bx_VMCS_HOST_STATE
{ //674
    bx_address cr0;
    bx_address cr3;
    bx_address cr4;

    Bit16u segreg_selector[6];

    bx_address fs_base;
    bx_address gs_base;

    bx_address gdtr_base;
    bx_address idtr_base;

    Bit32u tr_selector;
    bx_address tr_base;

    bx_address rsp;
    bx_address rip;

    bx_address sysenter_esp_msr;
    bx_address sysenter_eip_msr;
    Bit32u sysenter_cs_msr;

#if BX_SUPPORT_VMX >= 2
#if BX_SUPPORT_X86_64
    Bit64u efer_msr;
#endif
    Bit64u pat_msr;
    Bit64u ia32_spec_ctrl_msr;
#endif

#if BX_SUPPORT_CET
    Bit64u msr_ia32_s_cet;
    bx_address ssp;
    bx_address interrupt_ssp_table_address;
#endif

#if BX_SUPPORT_PKEYS
    Bit32u pkrs;
#endif
} VMCS_HOST_STATE;

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
    VmxVmexec1Controls vmexec_ctrls1; //764
    VmxVmexec2Controls vmexec_ctrls2; //769

    Bit32u vm_exceptions_bitmap;
    Bit32u vm_pf_mask;
    Bit32u vm_pf_match;
    Bit64u msr_data; //785

#if BX_SUPPORT_X86_64  //799
    bx_phy_address virtual_apic_page_addr;
    Bit32u vm_tpr_threshold;
    bx_phy_address apic_access_page;
    unsigned apic_access;
#endif

#if BX_SUPPORT_VMX >= 2
    Bit64u eptptr;
    Bit16u vpid;
    Bit64u pml_address;
    Bit16u pml_index;
    Bit64u spptp;
#endif

#if BX_SUPPORT_VMX >= 2
    bx_phy_address ve_info_addr;
    Bit16u eptp_index;
#endif

#if BX_SUPPORT_CET  //842
    bool shadow_stack_prematurely_busy;
#endif
    BxVmexit1Controls vmexit_ctrls1; //853
    BxVmexit2Controls vmexit_ctrls2; //854
    Bit32u vmexit_msr_store_cnt;
    bx_phy_address vmexit_msr_store_addr;
    Bit32u vmexit_msr_load_cnt;
    bx_phy_address vmexit_msr_load_addr;

    Bit32u vmentry_interr_info; //877
    Bit32u idt_vector_info;
    Bit32u idt_vector_error_code; //900

    VMCS_HOST_STATE host_state; //906
} VMCS_CACHE;

const Bit32u VMX_MISC_PREEMPTION_TIMER_RATE = 0;  //1105

