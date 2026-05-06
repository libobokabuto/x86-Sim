#pragma once
#include "bochs.h"
#include "cpu.h"
#include "cpustats.h"

#if BX_SUPPORT_SVM
#include "svm.h"
#endif

BX_CPU_C::BX_CPU_C(unsigned id) : bx_cpuid(id)
#if BX_CPU_LEVEL >= 4
, cpuid(NULL)
#endif
{
	//51ÐÐ
}


#include "cpuid.h"

void BX_CPU_C::initialize(void)
{
	//178
	/*
	#if BX_CPU_LEVEL >= 4
  BX_CPU_THIS_PTR cpuid = cpuid_factory(this);
  if (! BX_CPU_THIS_PTR cpuid) {
    BX_PANIC(("Failed to create CPUID module !"));
  }
  else {
    const char *cpu_model_name = cpuid->get_name();
    BX_INFO(("initialized CPU model %s", cpu_model_name));

    const char* features_to_exclude = SIM->get_param_string(BXPN_CPU_EXCLUDE_FEATURES)->getptr();
    add_remove_cpuid_features(features_to_exclude, false);

    const char* features_to_add = SIM->get_param_string(BXPN_CPU_ADD_FEATURES)->getptr();
    add_remove_cpuid_features(features_to_add, true);
  }
  */
  /*
  BX_CPU_THIS_PTR cpuid->get_cpu_extensions(BX_CPU_THIS_PTR ia_extensions_bitmask);
  
#if BX_SUPPORT_VMX
  BX_CPU_THIS_PTR vmx_extensions_bitmask = BX_CPU_THIS_PTR cpuid->get_vmx_extensions_bitmask();
#endif
#if BX_SUPPORT_SVM
  BX_CPU_THIS_PTR svm_extensions_bitmask = BX_CPU_THIS_PTR cpuid->get_svm_extensions_bitmask();
#endif
*/
  /*
  BX_CPU_THIS_PTR cpuid->sanity_checks();
#endif
	*/
    init_FetchDecodeTables();

#if BX_CPU_LEVEL >= 6
    xsave_xrestor_init();
#endif

#if BX_SUPPORT_AMX
    amx = NULL;
    if (BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_AMX)) {
        amx = new AMX;
    }
#endif

#if BX_SUPPORT_SVM
    vmcb = NULL;
    if (BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_SVM)) {
        vmcb = new VMCB_CACHE;
    }
#endif

#if BX_CPU_LEVEL >= 5 //228
    init_MSRs();


#if BX_CONFIGURE_MSRS
    for (unsigned n = 0; n < BX_MSR_MAX_INDEX; n++) {
        BX_CPU_THIS_PTR msrs[n] = 0;
    }
    //const char* msrs_filename = SIM->get_param_string(BXPN_CONFIGURABLE_MSRS_PATH)->getptr();
    //load_MSRs(msrs_filename);
#endif
    //BX_CPU_THIS_PTR ignore_bad_msrs = SIM->get_param_bool(BXPN_IGNORE_BAD_MSRS)->get();
#endif//241

    init_SMRAM();

#if BX_SUPPORT_VMX
    init_VMCS();
#endif

    init_statistics();
}
void BX_CPU_C::reset(unsigned source)
{
    UNUSED(source);

    BX_CPU_THIS_PTR eflags = 0x2;
    memset(&BX_CPU_THIS_PTR oszapc, 0, sizeof(BX_CPU_THIS_PTR oszapc));
    clearEFlagsOSZAPC();

    BX_CPU_THIS_PTR prev_rip = RIP = 0x0000FFF0;

    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value = 0xF000;
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.index = 0xF000 >> 3;
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.ti = 0;
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.rpl = 0;

    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.valid = SegValidCache | SegAccessROK | SegAccessWOK;
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.p = 1;
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.dpl = 0;
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.segment = 1;
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.type = BX_DATA_READ_WRITE_ACCESSED;
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.base = 0xFFFF0000;
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled = 0xFFFF;
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.g = 0;
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.d_b = 0;
#if BX_SUPPORT_X86_64
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.l = 0;
#endif
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.avl = 0;

    BX_CPU_THIS_PTR eipPageBias = 0;
    BX_CPU_THIS_PTR eipPageWindowSize = 0;
    BX_CPU_THIS_PTR eipFetchPtr = NULL;
    BX_CPU_THIS_PTR pAddrFetchPage = 0;

}

void BX_CPU_C::init_statistics(void)
{
    //253
}
BX_CPU_C::~BX_CPU_C()
{
	//826
}