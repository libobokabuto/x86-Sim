#pragma once
struct SVM_HOST_STATE
{
	bx_segment_reg_t sregs[4];

	bx_global_segment_reg_t gdtr;
	bx_global_segment_reg_t idtr;

	bx_efer_t efer;
	bx_cr0_t cr0;
	bx_cr4_t cr4;
	bx_phy_address cr3;
	Bit32u eflags;
	Bit64u rip;
	Bit64u rsp;
	Bit64u rax;

	BxPackedRegister pat_msr;
};
struct SVM_CONTROLS
{
	//286
	Bit16u cr_rd_ctrl;
	Bit16u cr_wr_ctrl;
	Bit16u dr_rd_ctrl;
	Bit16u dr_wr_ctrl;
	Bit32u exceptions_intercept;

	Bit32u intercept_vector[2];

	Bit32u exitintinfo;
	Bit32u exitintinfo_error_code;

	Bit32u eventinj;

	bx_phy_address iopm_base;
	bx_phy_address msrpm_base;

	Bit8u v_tpr;
	Bit8u v_intr_prio;
	bool v_ignore_tpr;
	bool v_intr_masking;
	Bit8u v_intr_vector;

	bool nested_paging;
	Bit64u ncr3;

	Bit16u pause_filter_count;
	//Bit16u pause_filter_threshold;

};
struct VMCB_CACHE
{
	SVM_HOST_STATE host_state;
	SVM_CONTROLS ctrls;
};
#if defined(NEED_CPU_REG_SHORTCUTS)
#define SVM_V_TPR          (BX_CPU_THIS_PTR vmcb->ctrls.v_tpr)
#define SVM_V_INTR_PRIO    (BX_CPU_THIS_PTR vmcb->ctrls.v_intr_prio)
#define SVM_V_IGNORE_TPR   (BX_CPU_THIS_PTR vmcb->ctrls.v_ignore_tpr)
#define SVM_V_INTR_MASKING (BX_CPU_THIS_PTR vmcb->ctrls.v_intr_masking)

#define SVM_HOST_IF (BX_CPU_THIS_PTR vmcb->host_state.eflags & EFlagsIFMask)

#endif