#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#include "smm.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_SUPPORT_SVM
#include "svm.h"
#endif



#if BX_CPU_LEVEL >= 3

void BX_CPU_C::enter_system_management_mode(void)
{ //89
	//BX_INFO(("Enter to System Management Mode"));

	// debug(BX_CPU_THIS_PTR prev_rip);

	//
	// Processors that support VMX operation perform SMI delivery as follows:
	//

#if BX_SUPPORT_VMX
  // Enter SMM
  // save the following internal to the processor:
  //   * CR4.VMXE
  //   * an indication of whether the logical processor was in VMX operation (root or non-root)
  // IF the logical processor is in VMX operation
  // THEN
  //   leave VMX operation;
  //   save VMX-critical state defined below;
  //   preserve current VMCS pointer as noted below;
  // FI;
  // CR4.VMXE = 0;

	BX_CPU_THIS_PTR cr4.set_VMXE(0);
	BX_CPU_THIS_PTR in_smm_vmx = BX_CPU_THIS_PTR in_vmx;
	BX_CPU_THIS_PTR in_smm_vmx_guest = BX_CPU_THIS_PTR in_vmx_guest;
	BX_CPU_THIS_PTR in_vmx = false;
	BX_CPU_THIS_PTR in_vmx_guest = false;

	//BX_INFO(("enter_system_management_mode: temporary disable VMX while in SMM mode"));

	// perform ordinary SMI delivery:
	//   * save processor state in SMRAM;
	//   * set processor state to standard SMM values
#endif

	BX_CPU_THIS_PTR in_smm = true;

	mask_event(BX_EVENT_SMI | BX_EVENT_NMI | BX_EVENT_VMX_VIRTUAL_NMI);

	Bit32u saved_state[SMM_SAVE_STATE_MAP_SIZE], n;
	// reset reserved bits
	for (n = 0; n < SMM_SAVE_STATE_MAP_SIZE; n++) saved_state[n] = 0;
	// prepare CPU state to be saved in the SMRAM
	smram_save_state(saved_state);

	bx_phy_address base = BX_CPU_THIS_PTR smbase + 0x10000;
	// could be optimized with reading of only non-reserved bytes
	for (n = 0; n < SMM_SAVE_STATE_MAP_SIZE; n++) {
		base -= 4;
		write_physical_dword(base, saved_state[n], BX_MEMTYPE_WB, BX_SMRAM_ACCESS);
	}

	BX_CPU_THIS_PTR setEFlags(0x2); // Bit1 is always set
	BX_CPU_THIS_PTR prev_rip = RIP = 0x00008000;
	BX_CPU_THIS_PTR dr7.set32(0x00000400);

	// CR0 - PE, EM, TS, and PG flags set to 0; others unmodified
	BX_CPU_THIS_PTR cr0.set_PE(0); // real mode (bit 0)
	BX_CPU_THIS_PTR cr0.set_EM(0); // emulate math coprocessor (bit 2)
	BX_CPU_THIS_PTR cr0.set_TS(0); // no task switch (bit 3)
	BX_CPU_THIS_PTR cr0.set_PG(0); // paging disabled (bit 31)

#if BX_CPU_LEVEL >= 5
	BX_CPU_THIS_PTR cr4.set32(0);
#endif

#if BX_CPU_LEVEL >= 5
#if BX_SUPPORT_X86_64
	if (BX_CPU_THIS_PTR efer.get_SVME())
		BX_CPU_THIS_PTR efer.set32(BX_EFER_SVME_MASK);
	else
#endif
		BX_CPU_THIS_PTR efer.set32(0);
#endif

	parse_selector(BX_CPU_THIS_PTR smbase >> 4,
		&BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector);

	BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.valid = SegValidCache | SegAccessROK | SegAccessWOK | SegAccessROK4G | SegAccessWOK4G;
	BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.p = 1;
	BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.dpl = 0;
	BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.segment = 1;  /* data/code segment */
	BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.type = BX_DATA_READ_WRITE_ACCESSED;

	BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.base = BX_CPU_THIS_PTR smbase;
	BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled = 0xffffffff;
	BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.avl = 0;
	BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.g = 1; /* page granular */
	BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.d_b = 0; /* 16bit default size */
#if BX_SUPPORT_X86_64
	BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.l = 0; /* 16bit default size */
#endif

	/* DS (Data Segment) and descriptor cache */
	parse_selector(0x0000,
		&BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].selector);

	BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.valid = SegValidCache | SegAccessROK | SegAccessWOK | SegAccessROK4G | SegAccessWOK4G;
	BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.p = 1;
	BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.dpl = 0;
	BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.segment = 1; /* data/code segment */
	BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.type = BX_DATA_READ_WRITE_ACCESSED;

	BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.base = 0x00000000;
	BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.limit_scaled = 0xffffffff;
	BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.avl = 0;
	BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.g = 1; /* byte granular */
	BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.d_b = 0; /* 16bit default size */
#if BX_SUPPORT_X86_64
	BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.l = 0; /* 16bit default size */
#endif

	// use DS segment as template for the others
	BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS] = BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS];
	BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES] = BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS];
	BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS] = BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS];
	BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS] = BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS];

	handleCpuContextChange();

#if BX_SUPPORT_MONITOR_MWAIT
	BX_CPU_THIS_PTR monitor.reset_monitor();
#endif

	BX_INSTR_TLB_CNTRL(BX_CPU_ID, BX_INSTR_CONTEXT_SWITCH, 0);
}

#define SMRAM_TRANSLATE(addr)    (((0x8000 - (addr)) >> 2) - 1)
static unsigned smram_map[SMRAM_FIELD_LAST];

#if BX_SUPPORT_X86_64
void BX_CPU_C::init_SMRAM(void)
{ //225
	static bool smram_map_ready = false;

	if (smram_map_ready) return;
	smram_map_ready = true;

	smram_map[SMRAM_FIELD_SMBASE_OFFSET] = SMRAM_TRANSLATE(0x7f00);
	smram_map[SMRAM_FIELD_SMM_REVISION_ID] = SMRAM_TRANSLATE(0x7efc);
	smram_map[SMRAM_FIELD_RAX_HI32] = SMRAM_TRANSLATE(0x7ffc);
	smram_map[SMRAM_FIELD_EAX] = SMRAM_TRANSLATE(0x7ff8);
	smram_map[SMRAM_FIELD_RCX_HI32] = SMRAM_TRANSLATE(0x7ff4);
	smram_map[SMRAM_FIELD_ECX] = SMRAM_TRANSLATE(0x7ff0);
	smram_map[SMRAM_FIELD_RDX_HI32] = SMRAM_TRANSLATE(0x7fec);
	smram_map[SMRAM_FIELD_EDX] = SMRAM_TRANSLATE(0x7fe8);
	smram_map[SMRAM_FIELD_RBX_HI32] = SMRAM_TRANSLATE(0x7fe4);
	smram_map[SMRAM_FIELD_EBX] = SMRAM_TRANSLATE(0x7fe0);
	smram_map[SMRAM_FIELD_RSP_HI32] = SMRAM_TRANSLATE(0x7fdc);
	smram_map[SMRAM_FIELD_ESP] = SMRAM_TRANSLATE(0x7fd8);
	smram_map[SMRAM_FIELD_RBP_HI32] = SMRAM_TRANSLATE(0x7fd4);
	smram_map[SMRAM_FIELD_EBP] = SMRAM_TRANSLATE(0x7fd0);
	smram_map[SMRAM_FIELD_RSI_HI32] = SMRAM_TRANSLATE(0x7fcc);
	smram_map[SMRAM_FIELD_ESI] = SMRAM_TRANSLATE(0x7fc8);
	smram_map[SMRAM_FIELD_RDI_HI32] = SMRAM_TRANSLATE(0x7fc4);
	smram_map[SMRAM_FIELD_EDI] = SMRAM_TRANSLATE(0x7fc0);
	smram_map[SMRAM_FIELD_R8_HI32] = SMRAM_TRANSLATE(0x7fbc);
	smram_map[SMRAM_FIELD_R8] = SMRAM_TRANSLATE(0x7fb8);
	smram_map[SMRAM_FIELD_R9_HI32] = SMRAM_TRANSLATE(0x7fb4);
	smram_map[SMRAM_FIELD_R9] = SMRAM_TRANSLATE(0x7fb0);
	smram_map[SMRAM_FIELD_R10_HI32] = SMRAM_TRANSLATE(0x7fac);
	smram_map[SMRAM_FIELD_R10] = SMRAM_TRANSLATE(0x7fa8);
	smram_map[SMRAM_FIELD_R11_HI32] = SMRAM_TRANSLATE(0x7fa4);
	smram_map[SMRAM_FIELD_R11] = SMRAM_TRANSLATE(0x7fa0);
	smram_map[SMRAM_FIELD_R12_HI32] = SMRAM_TRANSLATE(0x7f9c);
	smram_map[SMRAM_FIELD_R12] = SMRAM_TRANSLATE(0x7f98);
	smram_map[SMRAM_FIELD_R13_HI32] = SMRAM_TRANSLATE(0x7f94);
	smram_map[SMRAM_FIELD_R13] = SMRAM_TRANSLATE(0x7f90);
	smram_map[SMRAM_FIELD_R14_HI32] = SMRAM_TRANSLATE(0x7f8c);
	smram_map[SMRAM_FIELD_R14] = SMRAM_TRANSLATE(0x7f88);
	smram_map[SMRAM_FIELD_R15_HI32] = SMRAM_TRANSLATE(0x7f84);
	smram_map[SMRAM_FIELD_R15] = SMRAM_TRANSLATE(0x7f80);
	smram_map[SMRAM_FIELD_RIP_HI32] = SMRAM_TRANSLATE(0x7f7c);
	smram_map[SMRAM_FIELD_EIP] = SMRAM_TRANSLATE(0x7f78);
	smram_map[SMRAM_FIELD_RFLAGS_HI32] = SMRAM_TRANSLATE(0x7f74); // always zero
	smram_map[SMRAM_FIELD_EFLAGS] = SMRAM_TRANSLATE(0x7f70);
	smram_map[SMRAM_FIELD_DR6_HI32] = SMRAM_TRANSLATE(0x7f6c);    // always zero
	smram_map[SMRAM_FIELD_DR6] = SMRAM_TRANSLATE(0x7f68);
	smram_map[SMRAM_FIELD_DR7_HI32] = SMRAM_TRANSLATE(0x7f64);    // always zero
	smram_map[SMRAM_FIELD_DR7] = SMRAM_TRANSLATE(0x7f60);
	smram_map[SMRAM_FIELD_CR0_HI32] = SMRAM_TRANSLATE(0x7f5c);    // always zero
	smram_map[SMRAM_FIELD_CR0] = SMRAM_TRANSLATE(0x7f58);
	smram_map[SMRAM_FIELD_CR3_HI32] = SMRAM_TRANSLATE(0x7f54);    // zero when physical address size 32-bit
	smram_map[SMRAM_FIELD_CR3] = SMRAM_TRANSLATE(0x7f50);
	smram_map[SMRAM_FIELD_CR4_HI32] = SMRAM_TRANSLATE(0x7f4c);    // always zero
	smram_map[SMRAM_FIELD_CR4] = SMRAM_TRANSLATE(0x7f48);
	smram_map[SMRAM_FIELD_SSP_HI32] = SMRAM_TRANSLATE(0x7f44);
	smram_map[SMRAM_FIELD_SSP] = SMRAM_TRANSLATE(0x7f40);
	smram_map[SMRAM_FIELD_EFER_HI32] = SMRAM_TRANSLATE(0x7ed4);   // always zero
	smram_map[SMRAM_FIELD_EFER] = SMRAM_TRANSLATE(0x7ed0);
	smram_map[SMRAM_FIELD_IO_INSTRUCTION_RESTART] = SMRAM_TRANSLATE(0x7ec8);
	smram_map[SMRAM_FIELD_AUTOHALT_RESTART] = SMRAM_TRANSLATE(0x7ec8);
	smram_map[SMRAM_FIELD_NMI_MASK] = SMRAM_TRANSLATE(0x7ec8);
	smram_map[SMRAM_FIELD_TR_BASE_HI32] = SMRAM_TRANSLATE(0x7e9c);
	smram_map[SMRAM_FIELD_TR_BASE] = SMRAM_TRANSLATE(0x7e98);
	smram_map[SMRAM_FIELD_TR_LIMIT] = SMRAM_TRANSLATE(0x7e94);
	smram_map[SMRAM_FIELD_TR_SELECTOR_AR] = SMRAM_TRANSLATE(0x7e90);
	smram_map[SMRAM_FIELD_IDTR_BASE_HI32] = SMRAM_TRANSLATE(0x7e8c);
	smram_map[SMRAM_FIELD_IDTR_BASE] = SMRAM_TRANSLATE(0x7e88);
	smram_map[SMRAM_FIELD_IDTR_LIMIT] = SMRAM_TRANSLATE(0x7e84);
	smram_map[SMRAM_FIELD_LDTR_BASE_HI32] = SMRAM_TRANSLATE(0x7e7c);
	smram_map[SMRAM_FIELD_LDTR_BASE] = SMRAM_TRANSLATE(0x7e78);
	smram_map[SMRAM_FIELD_LDTR_LIMIT] = SMRAM_TRANSLATE(0x7e74);
	smram_map[SMRAM_FIELD_LDTR_SELECTOR_AR] = SMRAM_TRANSLATE(0x7e70);
	smram_map[SMRAM_FIELD_GDTR_BASE_HI32] = SMRAM_TRANSLATE(0x7e6c);
	smram_map[SMRAM_FIELD_GDTR_BASE] = SMRAM_TRANSLATE(0x7e68);
	smram_map[SMRAM_FIELD_GDTR_LIMIT] = SMRAM_TRANSLATE(0x7e64);
	smram_map[SMRAM_FIELD_ES_BASE_HI32] = SMRAM_TRANSLATE(0x7e0c);
	smram_map[SMRAM_FIELD_ES_BASE] = SMRAM_TRANSLATE(0x7e08);
	smram_map[SMRAM_FIELD_ES_LIMIT] = SMRAM_TRANSLATE(0x7e04);
	smram_map[SMRAM_FIELD_ES_SELECTOR_AR] = SMRAM_TRANSLATE(0x7e00);
	smram_map[SMRAM_FIELD_CS_BASE_HI32] = SMRAM_TRANSLATE(0x7e1c);
	smram_map[SMRAM_FIELD_CS_BASE] = SMRAM_TRANSLATE(0x7e18);
	smram_map[SMRAM_FIELD_CS_LIMIT] = SMRAM_TRANSLATE(0x7e14);
	smram_map[SMRAM_FIELD_CS_SELECTOR_AR] = SMRAM_TRANSLATE(0x7e10);
	smram_map[SMRAM_FIELD_SS_BASE_HI32] = SMRAM_TRANSLATE(0x7e2c);
	smram_map[SMRAM_FIELD_SS_BASE] = SMRAM_TRANSLATE(0x7e28);
	smram_map[SMRAM_FIELD_SS_LIMIT] = SMRAM_TRANSLATE(0x7e24);
	smram_map[SMRAM_FIELD_SS_SELECTOR_AR] = SMRAM_TRANSLATE(0x7e20);
	smram_map[SMRAM_FIELD_DS_BASE_HI32] = SMRAM_TRANSLATE(0x7e3c);
	smram_map[SMRAM_FIELD_DS_BASE] = SMRAM_TRANSLATE(0x7e38);
	smram_map[SMRAM_FIELD_DS_LIMIT] = SMRAM_TRANSLATE(0x7e34);
	smram_map[SMRAM_FIELD_DS_SELECTOR_AR] = SMRAM_TRANSLATE(0x7e30);
	smram_map[SMRAM_FIELD_FS_BASE_HI32] = SMRAM_TRANSLATE(0x7e4c);
	smram_map[SMRAM_FIELD_FS_BASE] = SMRAM_TRANSLATE(0x7e48);
	smram_map[SMRAM_FIELD_FS_LIMIT] = SMRAM_TRANSLATE(0x7e44);
	smram_map[SMRAM_FIELD_FS_SELECTOR_AR] = SMRAM_TRANSLATE(0x7e40);
	smram_map[SMRAM_FIELD_GS_BASE_HI32] = SMRAM_TRANSLATE(0x7e5c);
	smram_map[SMRAM_FIELD_GS_BASE] = SMRAM_TRANSLATE(0x7e58);
	smram_map[SMRAM_FIELD_GS_LIMIT] = SMRAM_TRANSLATE(0x7e54);
	smram_map[SMRAM_FIELD_GS_SELECTOR_AR] = SMRAM_TRANSLATE(0x7e50);
	for (unsigned n = 0; n < SMRAM_FIELD_LAST; n++) {
		if (smram_map[n] >= SMM_SAVE_STATE_MAP_SIZE) {
			//BX_PANIC(("smram map[%d] = %d", n, smram_map[n]));
		}
	}
}

#else
void BX_CPU_C::init_SMRAM(void)
{
	smram_map[SMRAM_FIELD_SMBASE_OFFSET] = SMRAM_TRANSLATE(0x7ef8);
	smram_map[SMRAM_FIELD_SMM_REVISION_ID] = SMRAM_TRANSLATE(0x7efc);
	smram_map[SMRAM_FIELD_EAX] = SMRAM_TRANSLATE(0x7fd0);
	smram_map[SMRAM_FIELD_ECX] = SMRAM_TRANSLATE(0x7fd4);
	smram_map[SMRAM_FIELD_EDX] = SMRAM_TRANSLATE(0x7fd8);
	smram_map[SMRAM_FIELD_EBX] = SMRAM_TRANSLATE(0x7fdc);
	smram_map[SMRAM_FIELD_ESP] = SMRAM_TRANSLATE(0x7fe0);
	smram_map[SMRAM_FIELD_EBP] = SMRAM_TRANSLATE(0x7fe4);
	smram_map[SMRAM_FIELD_ESI] = SMRAM_TRANSLATE(0x7fe8);
	smram_map[SMRAM_FIELD_EDI] = SMRAM_TRANSLATE(0x7fec);
	smram_map[SMRAM_FIELD_EIP] = SMRAM_TRANSLATE(0x7ff0);
	smram_map[SMRAM_FIELD_EFLAGS] = SMRAM_TRANSLATE(0x7ff4);
	smram_map[SMRAM_FIELD_DR6] = SMRAM_TRANSLATE(0x7fcc);
	smram_map[SMRAM_FIELD_DR7] = SMRAM_TRANSLATE(0x7fc8);
	smram_map[SMRAM_FIELD_CR0] = SMRAM_TRANSLATE(0x7ffc);
	smram_map[SMRAM_FIELD_CR3] = SMRAM_TRANSLATE(0x7ff8);
	smram_map[SMRAM_FIELD_CR4] = SMRAM_TRANSLATE(0x7f14);
	smram_map[SMRAM_FIELD_EFER] = SMRAM_TRANSLATE(0x7f10);
	smram_map[SMRAM_FIELD_IO_INSTRUCTION_RESTART] = SMRAM_TRANSLATE(0x7f00);
	smram_map[SMRAM_FIELD_AUTOHALT_RESTART] = SMRAM_TRANSLATE(0x7f00);
	smram_map[SMRAM_FIELD_NMI_MASK] = SMRAM_TRANSLATE(0x7f00);
	smram_map[SMRAM_FIELD_TR_SELECTOR] = SMRAM_TRANSLATE(0x7fc4);
	smram_map[SMRAM_FIELD_TR_BASE] = SMRAM_TRANSLATE(0x7f64);
	smram_map[SMRAM_FIELD_TR_LIMIT] = SMRAM_TRANSLATE(0x7f60);
	smram_map[SMRAM_FIELD_TR_SELECTOR_AR] = SMRAM_TRANSLATE(0x7f5c);
	smram_map[SMRAM_FIELD_LDTR_SELECTOR] = SMRAM_TRANSLATE(0x7fc0);
	smram_map[SMRAM_FIELD_LDTR_BASE] = SMRAM_TRANSLATE(0x7f80);
	smram_map[SMRAM_FIELD_LDTR_LIMIT] = SMRAM_TRANSLATE(0x7f7c);
	smram_map[SMRAM_FIELD_LDTR_SELECTOR_AR] = SMRAM_TRANSLATE(0x7f78);
	smram_map[SMRAM_FIELD_IDTR_BASE] = SMRAM_TRANSLATE(0x7f58);
	smram_map[SMRAM_FIELD_IDTR_LIMIT] = SMRAM_TRANSLATE(0x7f54);
	smram_map[SMRAM_FIELD_GDTR_BASE] = SMRAM_TRANSLATE(0x7f74);
	smram_map[SMRAM_FIELD_GDTR_LIMIT] = SMRAM_TRANSLATE(0x7f70);
	smram_map[SMRAM_FIELD_ES_SELECTOR] = SMRAM_TRANSLATE(0x7fa8);
	smram_map[SMRAM_FIELD_ES_BASE] = SMRAM_TRANSLATE(0x7f8c);
	smram_map[SMRAM_FIELD_ES_LIMIT] = SMRAM_TRANSLATE(0x7f88);
	smram_map[SMRAM_FIELD_ES_SELECTOR_AR] = SMRAM_TRANSLATE(0x7f84);
	smram_map[SMRAM_FIELD_CS_SELECTOR] = SMRAM_TRANSLATE(0x7fac);
	smram_map[SMRAM_FIELD_CS_BASE] = SMRAM_TRANSLATE(0x7f98);
	smram_map[SMRAM_FIELD_CS_LIMIT] = SMRAM_TRANSLATE(0x7f94);
	smram_map[SMRAM_FIELD_CS_SELECTOR_AR] = SMRAM_TRANSLATE(0x7f90);
	smram_map[SMRAM_FIELD_SS_SELECTOR] = SMRAM_TRANSLATE(0x7fb0);
	smram_map[SMRAM_FIELD_SS_BASE] = SMRAM_TRANSLATE(0x7fa4);
	smram_map[SMRAM_FIELD_SS_LIMIT] = SMRAM_TRANSLATE(0x7fa0);
	smram_map[SMRAM_FIELD_SS_SELECTOR_AR] = SMRAM_TRANSLATE(0x7f9c);
	smram_map[SMRAM_FIELD_DS_SELECTOR] = SMRAM_TRANSLATE(0x7fb4);
	smram_map[SMRAM_FIELD_DS_BASE] = SMRAM_TRANSLATE(0x7f34);
	smram_map[SMRAM_FIELD_DS_LIMIT] = SMRAM_TRANSLATE(0x7f30);
	smram_map[SMRAM_FIELD_DS_SELECTOR_AR] = SMRAM_TRANSLATE(0x7f2c);
	smram_map[SMRAM_FIELD_FS_SELECTOR] = SMRAM_TRANSLATE(0x7fb8);
	smram_map[SMRAM_FIELD_FS_BASE] = SMRAM_TRANSLATE(0x7f40);
	smram_map[SMRAM_FIELD_FS_LIMIT] = SMRAM_TRANSLATE(0x7f3c);
	smram_map[SMRAM_FIELD_FS_SELECTOR_AR] = SMRAM_TRANSLATE(0x7f38);
	smram_map[SMRAM_FIELD_GS_SELECTOR] = SMRAM_TRANSLATE(0x7fbc);
	smram_map[SMRAM_FIELD_GS_BASE] = SMRAM_TRANSLATE(0x7f4c);
	smram_map[SMRAM_FIELD_GS_LIMIT] = SMRAM_TRANSLATE(0x7f48);
	smram_map[SMRAM_FIELD_GS_SELECTOR_AR] = SMRAM_TRANSLATE(0x7f44);

	for (unsigned n = 0; n < SMRAM_FIELD_LAST; n++) {
		if (smram_map[n] >= SMM_SAVE_STATE_MAP_SIZE) {
			//BX_PANIC(("smram map[%d] = %d", n, smram_map[n]));
		}
	}
}
#endif

#define SMRAM_FIELD(state, field) (state[smram_map[field]])

#if BX_SUPPORT_X86_64 //407

BX_CPP_INLINE Bit64u SMRAM_FIELD64(const Bit32u* saved_state, unsigned hi, unsigned lo)
{ //410
	Bit64u tmp = ((Bit64u)SMRAM_FIELD(saved_state, hi)) << 32;
	tmp |= (Bit64u)SMRAM_FIELD(saved_state, lo);
	return tmp;
}

void BX_CPU_C::smram_save_state(Bit32u* saved_state)
{ //416
	// --- General Purpose Registers --- //
	for (int n = 0; n < BX_GENERAL_REGISTERS; n++) {
		Bit64u val_64 = BX_READ_64BIT_REG(n);

		SMRAM_FIELD(saved_state, SMRAM_FIELD_RAX_HI32 + 2 * n) = GET32H(val_64);
		SMRAM_FIELD(saved_state, SMRAM_FIELD_EAX + 2 * n) = GET32L(val_64);
	}

	SMRAM_FIELD(saved_state, SMRAM_FIELD_RIP_HI32) = GET32H(RIP);
	SMRAM_FIELD(saved_state, SMRAM_FIELD_EIP) = EIP;
	SMRAM_FIELD(saved_state, SMRAM_FIELD_EFLAGS) = read_eflags();

#if BX_SUPPORT_CET
	SMRAM_FIELD(saved_state, SMRAM_FIELD_SSP_HI32) = GET32H(SSP);
	SMRAM_FIELD(saved_state, SMRAM_FIELD_SSP) = GET32L(SSP);
#endif

	// --- Debug and Control Registers --- //
	SMRAM_FIELD(saved_state, SMRAM_FIELD_DR6) = BX_CPU_THIS_PTR dr6.get32();
	SMRAM_FIELD(saved_state, SMRAM_FIELD_DR7) = BX_CPU_THIS_PTR dr7.get32();
	SMRAM_FIELD(saved_state, SMRAM_FIELD_CR0) = BX_CPU_THIS_PTR cr0.get32();
	SMRAM_FIELD(saved_state, SMRAM_FIELD_CR3_HI32) = GET32H(BX_CPU_THIS_PTR cr3);
	SMRAM_FIELD(saved_state, SMRAM_FIELD_CR3) = GET32L(BX_CPU_THIS_PTR cr3);
	SMRAM_FIELD(saved_state, SMRAM_FIELD_CR4) = BX_CPU_THIS_PTR cr4.get32();
	SMRAM_FIELD(saved_state, SMRAM_FIELD_EFER) = BX_CPU_THIS_PTR efer.get32();

	SMRAM_FIELD(saved_state, SMRAM_FIELD_SMBASE_OFFSET) = BX_CPU_THIS_PTR smbase;
	SMRAM_FIELD(saved_state, SMRAM_FIELD_SMM_REVISION_ID) = SMM_REVISION_ID;

	// --- Task Register --- //
	SMRAM_FIELD(saved_state, SMRAM_FIELD_TR_BASE_HI32) = GET32H(BX_CPU_THIS_PTR tr.cache.u.segment.base);
	SMRAM_FIELD(saved_state, SMRAM_FIELD_TR_BASE) = GET32L(BX_CPU_THIS_PTR tr.cache.u.segment.base);
	SMRAM_FIELD(saved_state, SMRAM_FIELD_TR_LIMIT) = BX_CPU_THIS_PTR tr.cache.u.segment.limit_scaled;
	bool tr_valid = (BX_CPU_THIS_PTR tr.cache.valid != 0);
	Bit32u tr_ar = ((get_descriptor_h(&BX_CPU_THIS_PTR tr.cache) >> 8) & 0xf0ff) | (tr_valid << 8);
	SMRAM_FIELD(saved_state, SMRAM_FIELD_TR_SELECTOR_AR) = BX_CPU_THIS_PTR tr.selector.value | (tr_ar << 16);

	// --- LDTR --- //
	SMRAM_FIELD(saved_state, SMRAM_FIELD_LDTR_BASE_HI32) = GET32H(BX_CPU_THIS_PTR ldtr.cache.u.segment.base);
	SMRAM_FIELD(saved_state, SMRAM_FIELD_LDTR_BASE) = GET32L(BX_CPU_THIS_PTR ldtr.cache.u.segment.base);
	SMRAM_FIELD(saved_state, SMRAM_FIELD_LDTR_LIMIT) = BX_CPU_THIS_PTR ldtr.cache.u.segment.limit_scaled;
	bool ldtr_valid = (BX_CPU_THIS_PTR ldtr.cache.valid != 0);
	Bit32u ldtr_ar = ((get_descriptor_h(&BX_CPU_THIS_PTR ldtr.cache) >> 8) & 0xf0ff) | (ldtr_valid << 8);
	SMRAM_FIELD(saved_state, SMRAM_FIELD_LDTR_SELECTOR_AR) = BX_CPU_THIS_PTR ldtr.selector.value | (ldtr_ar << 16);

	// --- IDTR --- //
	SMRAM_FIELD(saved_state, SMRAM_FIELD_IDTR_BASE_HI32) = GET32H(BX_CPU_THIS_PTR idtr.base);
	SMRAM_FIELD(saved_state, SMRAM_FIELD_IDTR_BASE) = GET32L(BX_CPU_THIS_PTR idtr.base);
	SMRAM_FIELD(saved_state, SMRAM_FIELD_IDTR_LIMIT) = BX_CPU_THIS_PTR idtr.limit;

	// --- GDTR --- //
	SMRAM_FIELD(saved_state, SMRAM_FIELD_GDTR_BASE_HI32) = GET32H(BX_CPU_THIS_PTR gdtr.base);
	SMRAM_FIELD(saved_state, SMRAM_FIELD_GDTR_BASE) = GET32L(BX_CPU_THIS_PTR gdtr.base);
	SMRAM_FIELD(saved_state, SMRAM_FIELD_GDTR_LIMIT) = BX_CPU_THIS_PTR gdtr.limit;

	for (int segreg = 0; segreg < 6; segreg++) {
		bx_segment_reg_t* seg = &(BX_CPU_THIS_PTR sregs[segreg]);
		SMRAM_FIELD(saved_state, SMRAM_FIELD_ES_BASE_HI32 + 4 * segreg) = GET32H(seg->cache.u.segment.base);
		SMRAM_FIELD(saved_state, SMRAM_FIELD_ES_BASE + 4 * segreg) = GET32L(seg->cache.u.segment.base);
		SMRAM_FIELD(saved_state, SMRAM_FIELD_ES_LIMIT + 4 * segreg) = seg->cache.u.segment.limit_scaled;
		bool seg_valid = (seg->cache.valid != 0);
		Bit32u seg_ar = ((get_descriptor_h(&seg->cache) >> 8) & 0xf0ff) | (seg_valid << 8);
		SMRAM_FIELD(saved_state, SMRAM_FIELD_ES_SELECTOR_AR + 4 * segreg) = seg->selector.value | (seg_ar << 16);
	}
}

#else //536

void BX_CPU_C::smram_save_state(Bit32u* saved_state)
{
	SMRAM_FIELD(saved_state, SMRAM_FIELD_SMM_REVISION_ID) = SMM_REVISION_ID;
	SMRAM_FIELD(saved_state, SMRAM_FIELD_SMBASE_OFFSET) = BX_CPU_THIS_PTR smbase;

	for (int n = 0; n < BX_GENERAL_REGISTERS; n++) {
		Bit32u val_32 = BX_READ_32BIT_REG(n);
		SMRAM_FIELD(saved_state, SMRAM_FIELD_EAX + n) = val_32;
	}

	SMRAM_FIELD(saved_state, SMRAM_FIELD_EIP) = EIP;
	SMRAM_FIELD(saved_state, SMRAM_FIELD_EFLAGS) = read_eflags();

	SMRAM_FIELD(saved_state, SMRAM_FIELD_CR0) = BX_CPU_THIS_PTR cr0.get32();
	SMRAM_FIELD(saved_state, SMRAM_FIELD_CR3) = BX_CPU_THIS_PTR cr3;
#if BX_CPU_LEVEL >= 5
	SMRAM_FIELD(saved_state, SMRAM_FIELD_CR4) = BX_CPU_THIS_PTR cr4.get32();
	SMRAM_FIELD(saved_state, SMRAM_FIELD_EFER) = BX_CPU_THIS_PTR efer.get32();
#endif
	SMRAM_FIELD(saved_state, SMRAM_FIELD_DR6) = BX_CPU_THIS_PTR dr6.get32();
	SMRAM_FIELD(saved_state, SMRAM_FIELD_DR7) = BX_CPU_THIS_PTR dr7.get32();

	// --- Task Register --- //
	SMRAM_FIELD(saved_state, SMRAM_FIELD_TR_SELECTOR) = BX_CPU_THIS_PTR tr.selector.value;
	SMRAM_FIELD(saved_state, SMRAM_FIELD_TR_BASE) = BX_CPU_THIS_PTR tr.cache.u.segment.base;
	SMRAM_FIELD(saved_state, SMRAM_FIELD_TR_LIMIT) = BX_CPU_THIS_PTR tr.cache.u.segment.limit_scaled;
	bool tr_valid = (BX_CPU_THIS_PTR tr.cache.valid != 0);
	Bit32u tr_ar = ((get_descriptor_h(&BX_CPU_THIS_PTR tr.cache) >> 8) & 0xf0ff) | (tr_valid << 8);
	SMRAM_FIELD(saved_state, SMRAM_FIELD_TR_SELECTOR_AR) = BX_CPU_THIS_PTR tr.selector.value | (tr_ar << 16);

	// --- LDTR --- //
	SMRAM_FIELD(saved_state, SMRAM_FIELD_LDTR_SELECTOR) = BX_CPU_THIS_PTR ldtr.selector.value;
	SMRAM_FIELD(saved_state, SMRAM_FIELD_LDTR_BASE) = BX_CPU_THIS_PTR ldtr.cache.u.segment.base;
	SMRAM_FIELD(saved_state, SMRAM_FIELD_LDTR_LIMIT) = BX_CPU_THIS_PTR ldtr.cache.u.segment.limit_scaled;
	bool ldtr_valid = (BX_CPU_THIS_PTR ldtr.cache.valid != 0);
	Bit32u ldtr_ar = ((get_descriptor_h(&BX_CPU_THIS_PTR ldtr.cache) >> 8) & 0xf0ff) | (ldtr_valid << 8);
	SMRAM_FIELD(saved_state, SMRAM_FIELD_LDTR_SELECTOR_AR) = BX_CPU_THIS_PTR ldtr.selector.value | (ldtr_ar << 16);

	// --- IDTR --- //
	SMRAM_FIELD(saved_state, SMRAM_FIELD_IDTR_BASE) = BX_CPU_THIS_PTR idtr.base;
	SMRAM_FIELD(saved_state, SMRAM_FIELD_IDTR_LIMIT) = BX_CPU_THIS_PTR idtr.limit;

	// --- GDTR --- //
	SMRAM_FIELD(saved_state, SMRAM_FIELD_GDTR_BASE) = BX_CPU_THIS_PTR gdtr.base;
	SMRAM_FIELD(saved_state, SMRAM_FIELD_GDTR_LIMIT) = BX_CPU_THIS_PTR gdtr.limit;

	for (int segreg = 0; segreg < 6; segreg++) {
		bx_segment_reg_t* seg = &(BX_CPU_THIS_PTR sregs[segreg]);
		SMRAM_FIELD(saved_state, SMRAM_FIELD_ES_SELECTOR + 4 * segreg) = seg->selector.value;
		SMRAM_FIELD(saved_state, SMRAM_FIELD_ES_BASE + 4 * segreg) = seg->cache.u.segment.base;
		SMRAM_FIELD(saved_state, SMRAM_FIELD_ES_LIMIT + 4 * segreg) = seg->cache.u.segment.limit_scaled;
		bool seg_valid = (seg->cache.valid != 0);
		Bit32u seg_ar = ((get_descriptor_h(&seg->cache) >> 8) & 0xf0ff) | (seg_valid << 8);
		SMRAM_FIELD(saved_state, SMRAM_FIELD_ES_SELECTOR_AR + 4 * segreg) = seg->selector.value | (seg_ar << 16);
	}
}


#endif /* BX_SUPPORT_X86_64 */ //607

#endif /* BX_CPU_LEVEL >= 3 */