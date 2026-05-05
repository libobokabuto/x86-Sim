#pragma once
class VmxVmexec3Controls {
private:
	Bit64u vmexec_ctrls;
public:

#define VMX_VM_EXEC_CTRL3_EMULATE_AVX10_VL256       (1 << 13)  //175

	bool EMULATE_AVX10_VL256() const { return vmexec_ctrls & VMX_VM_EXEC_CTRL3_EMULATE_AVX10_VL256; } //184
};