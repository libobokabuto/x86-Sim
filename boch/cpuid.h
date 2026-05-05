#pragma once
class bx_cpuid_t {
public:
	BX_CPP_INLINE bool is_cpu_extension_supported(unsigned extension) const {
		//57
		assert(extension < BX_ISA_EXTENSION_LAST);
		return ia_extensions_bitmask[extension / 32] & (1 << (extension % 32));
	}

	bool support_avx10_512() const; //89

protected:
	//102
	Bit32u ia_extensions_bitmask[BX_ISA_EXTENSIONS_ARRAY_SIZE]; //109

};