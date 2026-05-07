#pragma once

#include "decoder.h" //27行
#include "instrument.h" //29行

#  define BX_SMF           static

#define EIP (BX_CPU_THIS_PTR gen_reg[BX_32BIT_REG_EIP].dword.erx) //82行

#define RIP (BX_CPU_THIS_PTR gen_reg[BX_64BIT_REG_RIP].rrx)  //107行

#define BX_WRITE_16BIT_REG(index, val) {\
  BX_CPU_THIS_PTR gen_reg[index].word.rx = val; \
}

#if BX_SUPPORT_X86_64
#define BX_READ_8BIT_REGx(index, extended) ((((index) & 4) == 0 || (extended)) ? \
  (BX_CPU_THIS_PTR gen_reg[index].word.byte.rl) : \
  (BX_CPU_THIS_PTR gen_reg[(index)-4].word.byte.rh))
#define BX_WRITE_8BIT_REGx(index, extended, val) {\
  if (((index) & 4) == 0 || (extended)) \
    BX_CPU_THIS_PTR gen_reg[index].word.byte.rl = val; \
  else \
    BX_CPU_THIS_PTR gen_reg[(index)-4].word.byte.rh = val; \
}
#else
#define BX_READ_8BIT_REGx(index, ext) (((index) & 4) ? \
  (BX_CPU_THIS_PTR gen_reg[(index)-4].word.byte.rh) : \
  (BX_CPU_THIS_PTR gen_reg[index].word.byte.rl))
#define BX_WRITE_8BIT_REGx(index, ext, val) {\
  if ((index) & 4) \
    BX_CPU_THIS_PTR gen_reg[(index)-4].word.byte.rh = val; \
  else \
    BX_CPU_THIS_PTR gen_reg[index].word.byte.rl = val; \
}
#endif

#define BX_CLEAR_64BIT_HIGH(index) {\
  BX_CPU_THIS_PTR gen_reg[index].dword.hrx = 0; \
} //174-176行 

#define USER_PL   (BX_CPU_THIS_PTR user_pl)   //200行

#if BX_SUPPORT_SMP  //202
#define BX_CPU_ID (BX_CPU_THIS_PTR bx_cpuid)
#else
#define BX_CPU_ID (0)
#endif

enum BX_Exception {
    //318
BX_DF_EXCEPTION = 8,
BX_GP_EXCEPTION = 13,//330
BX_PF_EXCEPTION = 14,
};

const unsigned BX_CPU_HANDLED_EXCEPTIONS = 32; //349

enum ExceptionClass {
    //351
    BX_EXCEPTION_CLASS_FAULT = 1,
};
enum BxCpuMode {
    BX_MODE_IA32_REAL = 0,
    BX_MODE_IA32_PROTECTED = 2,
    BX_MODE_LONG_64 = 4           // EFER.LMA = 1, CR0.PE=1, CS.L=1,362行
};

const unsigned BX_MSR_MAX_INDEX = 0x1000;
const Bit32u CACHE_LINE_SIZE = 64;

class BX_CPU_C;//391行
class bxInstruction_c;//393行
struct bxICacheEntry_c;

#  define BX_CPU_THIS_PTR  BX_CPU(0)->    //421行
#  define BX_CPU_THIS      BX_CPU(0)
#  define BX_CPU_CALL_METHOD(func, args) \
            ((BxExecutePtr_tR) (func)) args

#define DECLARE_EFLAG_ACCESSOR(name,bitnum)                     \
  BX_SMF BX_CPP_INLINE unsigned  get_##name ();                 \
  BX_SMF BX_CPP_INLINE unsigned getB_##name ();                 \
  BX_SMF BX_CPP_INLINE void assert_##name ();                   \
  BX_SMF BX_CPP_INLINE void clear_##name ();                    \
  BX_SMF BX_CPP_INLINE void set_##name (bool val);

#define IMPLEMENT_EFLAG_ACCESSOR(name,bitnum)                   \
  BX_CPP_INLINE unsigned BX_CPU_C::getB_##name () {             \
    return 1 & (BX_CPU_THIS_PTR eflags >> bitnum);              \
  }                                                             \
  BX_CPP_INLINE unsigned BX_CPU_C::get_##name () {              \
    return BX_CPU_THIS_PTR eflags & (1 << bitnum);              \
  }

#define IMPLEMENT_EFLAG_SET_ACCESSOR(name,bitnum)                   \
  BX_CPP_INLINE void BX_CPU_C::assert_##name () {                   \
    BX_CPU_THIS_PTR eflags |= (1<<bitnum);                          \
  }                                                                 \
  BX_CPP_INLINE void BX_CPU_C::clear_##name () {                    \
    BX_CPU_THIS_PTR eflags &= ~(1<<bitnum);                         \
  }                                                                 \
  BX_CPP_INLINE void BX_CPU_C::set_##name (bool val) {              \
    BX_CPU_THIS_PTR eflags =                                        \
      (BX_CPU_THIS_PTR eflags&~(1<<bitnum))|(Bit32u(val)<<bitnum);   \
  }

#if BX_CPU_LEVEL >= 4

#define IMPLEMENT_EFLAG_SET_ACCESSOR_AC(bitnum)                 \
  BX_CPP_INLINE void BX_CPU_C::assert_AC() {                    \
    BX_CPU_THIS_PTR eflags |= (1<<bitnum);                      \
    handleAlignmentCheck();                                     \
  }                                                             \
  BX_CPP_INLINE void BX_CPU_C::clear_AC() {                     \
    BX_CPU_THIS_PTR eflags &= ~(1<<bitnum);                     \
    handleAlignmentCheck();                                     \
  }                                                             \
  BX_CPP_INLINE void BX_CPU_C::set_AC(bool val) {                   \
    BX_CPU_THIS_PTR eflags =                                        \
      (BX_CPU_THIS_PTR eflags&~(1<<bitnum))|(Bit32u(val)<<bitnum);  \
    handleAlignmentCheck();                                         \
  }

#endif

#define IMPLEMENT_EFLAG_SET_ACCESSOR_VM(bitnum)                 \
  BX_CPP_INLINE void BX_CPU_C::assert_VM() {                    \
    set_VM(1);                                                  \
  }                                                             \
  BX_CPP_INLINE void BX_CPU_C::clear_VM() {                     \
    set_VM(0);                                                  \
  }                                                             \
  BX_CPP_INLINE void BX_CPU_C::set_VM(bool val) {               \
    if (!long_mode()) {                                         \
      BX_CPU_THIS_PTR eflags =                                  \
        (BX_CPU_THIS_PTR eflags&~(1<<bitnum))|(Bit32u(val)<<bitnum);  \
      handleCpuModeChange();                                    \
    }                                                           \
  }

// need special handling when IF is set
#define IMPLEMENT_EFLAG_SET_ACCESSOR_IF(bitnum)                 \
  BX_CPP_INLINE void BX_CPU_C::assert_IF() {                    \
    BX_CPU_THIS_PTR eflags |= (1<<bitnum);                      \
    handleInterruptMaskChange();                                \
  }                                                             \
  BX_CPP_INLINE void BX_CPU_C::clear_IF() {                     \
    BX_CPU_THIS_PTR eflags &= ~(1<<bitnum);                     \
    handleInterruptMaskChange();                                \
  }                                                             \
  BX_CPP_INLINE void BX_CPU_C::set_IF(bool val) {               \
    if (val) assert_IF();                                       \
    else clear_IF();                                            \
  }

// assert async_event when TF is set
#define IMPLEMENT_EFLAG_SET_ACCESSOR_TF(bitnum)                 \
  BX_CPP_INLINE void BX_CPU_C::assert_TF() {                    \
    BX_CPU_THIS_PTR async_event = 1;                            \
    BX_CPU_THIS_PTR eflags |= (1<<bitnum);                      \
  }                                                             \
  BX_CPP_INLINE void BX_CPU_C::clear_TF() {                     \
    BX_CPU_THIS_PTR eflags &= ~(1<<bitnum);                     \
  }                                                             \
  BX_CPP_INLINE void BX_CPU_C::set_TF(bool val) {                   \
    if (val) BX_CPU_THIS_PTR async_event = 1;                       \
    BX_CPU_THIS_PTR eflags =                                        \
      (BX_CPU_THIS_PTR eflags&~(1<<bitnum))|(Bit32u(val)<<bitnum);  \
  }

// invalidate prefetch queue and call prefetch() when RF is set
#define IMPLEMENT_EFLAG_SET_ACCESSOR_RF(bitnum)                 \
  BX_CPP_INLINE void BX_CPU_C::assert_RF() {                    \
    invalidate_prefetch_q();                                    \
    BX_CPU_THIS_PTR eflags |= (1<<bitnum);                      \
  }                                                             \
  BX_CPP_INLINE void BX_CPU_C::clear_RF() {                     \
    BX_CPU_THIS_PTR eflags &= ~(1<<bitnum);                     \
  }                                                             \
  BX_CPP_INLINE void BX_CPU_C::set_RF(bool val) {                   \
    if (val) invalidate_prefetch_q();                               \
    BX_CPU_THIS_PTR eflags =                                        \
      (BX_CPU_THIS_PTR eflags&~(1<<bitnum))|(Bit32u(val)<<bitnum);  \
  }

#define DECLARE_EFLAG_ACCESSOR_IOPL(bitnum)                     \
  BX_SMF BX_CPP_INLINE void set_IOPL(Bit32u val);               \
  BX_SMF BX_CPP_INLINE Bit32u  get_IOPL(void);

#define IMPLEMENT_EFLAG_ACCESSOR_IOPL(bitnum)                   \
  BX_CPP_INLINE void BX_CPU_C::set_IOPL(Bit32u val) {           \
    BX_CPU_THIS_PTR eflags &= ~(3<<bitnum);                     \
    BX_CPU_THIS_PTR eflags |= ((3&val) << bitnum);              \
  }                                                             \
  BX_CPP_INLINE Bit32u BX_CPU_C::get_IOPL() {                   \
    return 3 & (BX_CPU_THIS_PTR eflags >> bitnum);              \
  }                                                             

BOCHSAPI extern BX_CPU_C** bx_cpu_array;  //462行
BOCHSAPI extern BX_CPU_C  bx_cpu;         //465行

const Bit32u EFlagsRFMask = (1 << 16); //612行

#include "crregs.h"//692
#include "descriptor.h"//693行
#include "instr.h" //694行
#include "lazy_flags.h"
#include "tlb.h" //696行
#include "icache.h"//697行

// general purpose register
#if BX_SUPPORT_X86_64

#ifdef BX_BIG_ENDIAN
typedef struct {
    union {
        struct {
            Bit32u dword_filler;
            Bit16u  word_filler;
            union {
                Bit16u rx;
                struct {
                    Bit8u rh;
                    Bit8u rl;
                } byte;
            };
        } word;
        Bit64u rrx;
        struct {
            Bit32u hrx;  // hi 32 bits
            Bit32u erx;  // lo 32 bits
        } dword;
    };
} bx_gen_reg_t;
#else
typedef struct { //724行
    union {
        struct {
            union {
                Bit16u rx;
                struct {
                    Bit8u rl;
                    Bit8u rh;
                } byte;
            };
            Bit16u  word_filler;
            Bit32u dword_filler;
        } word;
        Bit64u rrx;
        struct {
            Bit32u erx;  // lo 32 bits
            Bit32u hrx;  // hi 32 bits
        } dword;
    };
} bx_gen_reg_t;

#endif

#else  // #if BX_SUPPORT_X86_64

#ifdef BX_BIG_ENDIAN
typedef struct {
    union {
        struct {
            Bit32u erx;
        } dword;
        struct {
            Bit16u word_filler;
            union {
                Bit16u rx;
                struct {
                    Bit8u rh;
                    Bit8u rl;
                } byte;
            };
        } word;
    };
} bx_gen_reg_t;
#else
typedef struct {
    union {
        struct {
            Bit32u erx;
        } dword;
        struct {
            union {
                Bit16u rx;
                struct {
                    Bit8u rl;
                    Bit8u rh;
                } byte;
            };
            Bit16u word_filler;
        } word;
    };
} bx_gen_reg_t;
#endif
#endif  // #if BX_SUPPORT_X86_64
#include "xmm.h"
struct softfloat_status_t;

typedef void (*simd_xmm_shift)(BxPackedXmmRegister* opdst, Bit64u shift_64);
typedef void (*simd_xmm_1op)(BxPackedXmmRegister* opdst);
typedef void (*simd_xmm_2op)(BxPackedXmmRegister* opdst, const BxPackedXmmRegister* op2);
typedef void (*simd_xmm_3op)(BxPackedXmmRegister* opdst, const BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2);
typedef void (*xmm_pfp_1op)     (BxPackedXmmRegister* opdst, softfloat_status_t& status);
typedef void (*xmm_pfp_1op_mask)(BxPackedXmmRegister* opdst, softfloat_status_t& status, Bit32u mask);
typedef void (*xmm_pfp_2op)     (BxPackedXmmRegister* opdst, const BxPackedXmmRegister* op1, softfloat_status_t& status);
typedef void (*xmm_pfp_2op_mask)(BxPackedXmmRegister* opdst, const BxPackedXmmRegister* op1, softfloat_status_t& status, Bit32u mask);
typedef void (*xmm_pfp_3op)     (BxPackedXmmRegister* opdst, const BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status);
typedef void (*xmm_pfp_3op_mask)(BxPackedXmmRegister* opdst, const BxPackedXmmRegister* op1, const BxPackedXmmRegister* op2, softfloat_status_t& status, Bit32u mask);
//805
#if BX_SUPPORT_VMX
#include "vmx.h"
#endif

#if BX_SUPPORT_SVM
struct SVM_HOST_STATE;
struct SVM_CONTROLS;
struct VMCB_CACHE;
#endif

enum monitor_armed_by {
    BX_MONITOR_NOT_ARMED = 0,
    BX_MONITOR_ARMED_BY_MONITOR,
    BX_MONITOR_ARMED_BY_MONITORX,
    BX_MONITOR_ARMED_BY_UMONITOR
};

#if BX_SUPPORT_MONITOR_MWAIT
struct monitor_addr_t {

    bx_phy_address monitor_addr;
    unsigned armed_by;
    BX_CPP_INLINE bool armed(void) const { return armed_by != BX_MONITOR_NOT_ARMED; }
};
#endif


class bx_cpuid_t; //887行

class BOCHSAPI BX_CPU_C{ //889-5377行
public:

    unsigned bx_cpuid;//892行
    #if BX_CPU_LEVEL >= 4
        bx_cpuid_t* cpuid;
    #endif
        Bit32u ia_extensions_bitmask[BX_ISA_EXTENSIONS_ARRAY_SIZE]; //898
#define BX_CPUID_SUPPORT_ISA_EXTENSION(feature) \
   (BX_CPU_THIS_PTR ia_extensions_bitmask[feature/32] & (1<<(feature%32)))
#if BX_SUPPORT_VMX
        Bit32u vmx_extensions_bitmask;
#endif
#if BX_SUPPORT_SVM
        Bit32u svm_extensions_bitmask;
#endif
#define BX_SUPPORT_VMX_EXTENSION(feature_mask) \
   (BX_CPU_THIS_PTR vmx_extensions_bitmask & (feature_mask))
#define BX_SUPPORT_SVM_EXTENSION(feature_mask) \
   (BX_CPU_THIS_PTR svm_extensions_bitmask & (feature_mask))



	BX_CPU_C(unsigned id = 0);
	~BX_CPU_C();
    bx_gen_reg_t gen_reg[BX_GENERAL_REGISTERS + 4]; //930行

    Bit32u eflags;//940行
    bx_lazyflags_entry oszapc;
    bx_address prev_rip; //948行

    Bit64u icount;

	/* user segment register set */
	bx_segment_reg_t  sregs[6]; //970行


    bx_efer_t efer; //996

#if BX_SUPPORT_MONITOR_MWAIT
    monitor_addr_t monitor;
#endif

#if BX_CONFIGURE_MSRS
    MSR* msrs[BX_MSR_MAX_INDEX];
#endif

#if BX_SUPPORT_VMX //1104
    bool in_vmx_guest;

    VMCS_CACHE vmcs; //1116
    VMX_CAP vmx_cap;
    VMCS_Mapping* vmcs_map; //118
#endif
#if BX_SUPPORT_SVM//1121
    VMCB_CACHE* vmcb;
#else//1134
#endif//1138
    #define BX_EVENT_CODE_BREAKPOINT_ASSIST       (1 <<  3) //1167行

    Bit32u  pending_event;//1180行
    Bit32u  async_event; //1182

    BX_SMF BX_CPP_INLINE void clear_event(Bit32u event) { //1189行
        
    }
    unsigned cpu_mode;//1219行
	bool  user_pl; //1220行
    Bit32u cpu_state_use_ok;//1225行

    // Boundaries of current code page, based on EIP
    bx_address eipPageBias; //1260行
    Bit32u     eipPageWindowSize;
    const Bit8u* eipFetchPtr;
    bx_phy_address pAddrFetchPage; // Guest physical address of current instruction page

    // Boundaries of current stack page, based on ESP
    bx_address espPageBias;        // Linear address of current stack page
    Bit32u     espPageWindowSize; //1267
    const Bit8u* espHostPtr;
    bx_phy_address pAddrStackPage; // Guest physical address of current stack page


#if BX_INSTRUMENTATION
#else
#define BX_INSTR_FAR_BRANCH_ORIGIN()
#endif
    #define BX_DTLB_SIZE 2048
    #define BX_ITLB_SIZE 1024
        TLB<BX_DTLB_SIZE> DTLB BX_CPP_AlignN(32);
        TLB<BX_ITLB_SIZE> ITLB BX_CPP_AlignN(32);

    bxICache_c iCache BX_CPP_AlignN(32);//1336行
    Bit32u fetchModeMask;//1337行
    BX_SMF BX_CPP_INLINE void clearEFlagsOSZAPC(void) {
        SET_FLAGS_OSZAPC_LOGIC_32(1);
    }
	void initialize(void);
	BX_SMF void cpu_loop(void);
    void init_statistics(void);//1416
    
    static BX_CPP_INLINE void gao_no(int, const char*) {} //319

#if 1
    // <TAG-CLASS-CPU-START> 1426
   // prototypes for CPU instructions...
    BX_SMF void PUSH16_Sw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void POP16_Sw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PUSH32_Sw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void POP32_Sw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void DAA(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void DAS(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void AAA(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void AAS(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void AAM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void AAD(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void PUSHA32(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PUSHA16(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void POPA32(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void POPA16(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ARPL_EwGw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PUSH_Id(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PUSH_Iw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void INSB32_YbDX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void INSB16_YbDX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void INSW32_YwDX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void INSW16_YwDX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void INSD32_YdDX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void INSD16_YdDX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void OUTSB32_DXXb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void OUTSB16_DXXb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void OUTSW32_DXXw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void OUTSW16_DXXw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void OUTSD32_DXXd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void OUTSD16_DXXd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void REP_INSB_YbDX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void REP_INSW_YwDX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void REP_INSD_YdDX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void REP_OUTSB_DXXb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void REP_OUTSW_DXXw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void REP_OUTSD_DXXd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void BOUND_GwMa(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BOUND_GdMa(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void TEST_EbGbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void TEST_EwGwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void TEST_EdGdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void TEST_EbGbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void TEST_EwGwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void TEST_EdGdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void XCHG_EbGbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void XCHG_EwGwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void XCHG_EdGdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void XCHG_EbGbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void XCHG_EwGwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void XCHG_EdGdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void MOV_EbGbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_EwGwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_GbEbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_GbEbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1);
    BX_SMF void MOV_GdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_GwEwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_GwEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void MOV32_GdEdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV32_EdGdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void MOV32S_GdEdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV32S_EdGdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void MOV_EwSwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_EwSwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_SwEw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void LEA_GdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LEA_GwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void CBW(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CWD(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CALL32_Ap(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CALL16_Ap(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PUSHF_Fw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void POPF_Fw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PUSHF_Fd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void POPF_Fd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SAHF(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LAHF(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void MOV_ALOd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_EAXOd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_AXOd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_OdAL(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_OdEAX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_OdAX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    // repeatable instructions
    BX_SMF void REP_MOVSB_YbXb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void REP_MOVSW_YwXw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void REP_MOVSD_YdXd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void REP_CMPSB_XbYb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void REP_CMPSW_XwYw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void REP_CMPSD_XdYd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void REP_STOSB_YbAL(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void REP_LODSB_ALXb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void REP_SCASB_ALYb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void REP_STOSW_YwAX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void REP_LODSW_AXXw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void REP_SCASW_AXYw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void REP_STOSD_YdEAX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void REP_LODSD_EAXXd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void REP_SCASD_EAXYd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    // qualified by address size
    BX_SMF void CMPSB16_XbYb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPSW16_XwYw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPSD16_XdYd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPSB32_XbYb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPSW32_XwYw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPSD32_XdYd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void SCASB16_ALYb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SCASW16_AXYw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SCASD16_EAXYd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SCASB32_ALYb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SCASW32_AXYw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SCASD32_EAXYd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void LODSB16_ALXb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LODSW16_AXXw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LODSD16_EAXXd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LODSB32_ALXb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LODSW32_AXXw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LODSD32_EAXXd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void STOSB16_YbAL(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void STOSW16_YwAX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void STOSD16_YdEAX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void STOSB32_YbAL(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void STOSW32_YwAX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void STOSD32_YdEAX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void MOVSB16_YbXb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVSW16_YwXw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVSD16_YdXd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVSB32_YbXb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVSW32_YwXw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVSD32_YdXd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void MOV_EdIdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_EwIwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_EbIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void ENTER16_IwIb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ENTER32_IwIb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LEAVE16(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LEAVE32(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void INT1(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void INT3(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void INT_Ib(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void INTO(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void IRET32(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void IRET16(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void SALC(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void XLAT(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void LOOPNE16_Jb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOOPE16_Jb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOOP16_Jb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOOPNE32_Jb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOOPE32_Jb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOOP32_Jb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JCXZ_Jb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JECXZ_Jb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void IN_ALIb(bxInstruction_c*) BX_CPP_AttrRegparmN(1);
    BX_SMF void IN_AXIb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void IN_EAXIb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void OUT_IbAL(bxInstruction_c*) BX_CPP_AttrRegparmN(1);
    BX_SMF void OUT_IbAX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void OUT_IbEAX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CALL_Jw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CALL_Jd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JMP_Jd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JMP_Jw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JMP_Ap(bxInstruction_c*) BX_CPP_AttrRegparmN(1);
    BX_SMF void IN_ALDX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void IN_AXDX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void IN_EAXDX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void OUT_DXAL(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void OUT_DXAX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void OUT_DXEAX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void HLT(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMC(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CLC(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void STC(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CLI(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void STI(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CLD(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void STD(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void LAR_GvEw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LSL_GvEw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CLTS(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void INVD(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void WBINVD(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CLFLUSH(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CLZERO(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void MOV_CR0Rd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_CR2Rd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_CR3Rd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_CR4Rd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_RdCR0(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_RdCR2(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_RdCR3(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_RdCR4(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_DdRd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_RdDd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void JO_Jw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JNO_Jw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JB_Jw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JNB_Jw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JZ_Jw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JNZ_Jw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JBE_Jw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JNBE_Jw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JS_Jw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JNS_Jw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JP_Jw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JNP_Jw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JL_Jw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JNL_Jw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JLE_Jw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JNLE_Jw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void JO_Jd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JNO_Jd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JB_Jd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JNB_Jd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JZ_Jd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JNZ_Jd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JBE_Jd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JNBE_Jd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JS_Jd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JNS_Jd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JP_Jd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JNP_Jd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JL_Jd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JNL_Jd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JLE_Jd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JNLE_Jd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void SETO_EbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SETNO_EbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SETB_EbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SETNB_EbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SETZ_EbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SETNZ_EbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SETBE_EbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SETNBE_EbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SETS_EbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SETNS_EbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SETP_EbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SETNP_EbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SETL_EbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SETNL_EbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SETLE_EbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SETNLE_EbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void SETO_EbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SETNO_EbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SETB_EbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SETNB_EbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SETZ_EbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SETNZ_EbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SETBE_EbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SETNBE_EbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SETS_EbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SETNS_EbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SETP_EbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SETNP_EbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SETL_EbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SETNL_EbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SETLE_EbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SETNLE_EbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void CPUID(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void SHRD_EwGwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SHRD_EwGwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SHLD_EwGwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SHLD_EwGwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SHRD_EdGdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SHRD_EdGdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SHLD_EdGdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SHLD_EdGdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void BSF_GwEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BSF_GdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BSR_GwEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BSR_GdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void BT_EwGwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BT_EdGdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BTS_EwGwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BTS_EdGdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BTR_EwGwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BTR_EdGdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BTC_EwGwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BTC_EdGdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void BT_EwGwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BT_EdGdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BTS_EwGwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BTS_EdGdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BTR_EwGwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BTR_EdGdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BTC_EwGwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BTC_EdGdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void BT_EwIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BT_EdIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BTS_EwIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BTS_EdIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BTR_EwIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BTR_EdIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BTC_EwIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BTC_EdIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void BT_EwIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BT_EdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BTS_EwIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BTS_EdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BTR_EwIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BTR_EdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BTC_EwIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BTC_EdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void LES_GwMp(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LDS_GwMp(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LSS_GwMp(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LFS_GwMp(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LGS_GwMp(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LES_GdMp(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LDS_GdMp(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LSS_GdMp(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LFS_GdMp(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LGS_GdMp(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void MOVZX_GwEbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVZX_GdEbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVZX_GdEwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVSX_GwEbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVSX_GdEbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVSX_GdEwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void MOVZX_GwEbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVZX_GdEbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVZX_GdEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVSX_GwEbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVSX_GdEbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVSX_GdEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void BSWAP_RX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BSWAP_ERX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void ZERO_IDIOM_GwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1);
    BX_SMF void ZERO_IDIOM_GdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void ADD_GbEbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void OR_GbEbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ADC_GbEbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SBB_GbEbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void AND_GbEbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SUB_GbEbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void XOR_GbEbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMP_GbEbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void ADD_GbEbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void OR_GbEbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ADC_GbEbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SBB_GbEbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void AND_GbEbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SUB_GbEbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void XOR_GbEbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMP_GbEbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void ADD_EbIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void OR_EbIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ADC_EbIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SBB_EbIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void AND_EbIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SUB_EbIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void XOR_EbIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMP_EbIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void ADD_EbIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void OR_EbIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ADC_EbIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SBB_EbIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void AND_EbIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SUB_EbIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void XOR_EbIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMP_EbIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1);

    BX_SMF void ADD_EbGbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void OR_EbGbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ADC_EbGbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SBB_EbGbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void AND_EbGbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SUB_EbGbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void XOR_EbGbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMP_EbGbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void ADD_EwIwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void OR_EwIwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ADC_EwIwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SBB_EwIwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void AND_EwIwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SUB_EwIwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void XOR_EwIwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMP_EwIwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void ADD_EwIwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void OR_EwIwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ADC_EwIwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SBB_EwIwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void AND_EwIwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SUB_EwIwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void XOR_EwIwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMP_EwIwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void ADD_EdIdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void OR_EdIdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ADC_EdIdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SBB_EdIdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void AND_EdIdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SUB_EdIdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void XOR_EdIdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMP_EdIdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void ADD_EdIdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void OR_EdIdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ADC_EdIdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SBB_EdIdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void AND_EdIdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SUB_EdIdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void XOR_EdIdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMP_EdIdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void ADD_EwGwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void OR_EwGwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ADC_EwGwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SBB_EwGwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void AND_EwGwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SUB_EwGwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void XOR_EwGwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMP_EwGwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void ADD_EdGdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void OR_EdGdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ADC_EdGdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SBB_EdGdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void AND_EdGdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SUB_EdGdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void XOR_EdGdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMP_EdGdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void ADD_GwEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void OR_GwEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ADC_GwEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SBB_GwEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void AND_GwEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SUB_GwEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void XOR_GwEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMP_GwEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void ADD_GwEwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void OR_GwEwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ADC_GwEwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SBB_GwEwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void AND_GwEwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SUB_GwEwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void XOR_GwEwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMP_GwEwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void ADD_GdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void OR_GdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ADC_GdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SBB_GdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void AND_GdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SUB_GdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMP_GdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void XOR_GdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void ADD_GdEdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void OR_GdEdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ADC_GdEdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SBB_GdEdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void AND_GdEdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SUB_GdEdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMP_GdEdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void XOR_GdEdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void NOT_EbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void NOT_EwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void NOT_EdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void NOT_EbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void NOT_EwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void NOT_EdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void NEG_EbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void NEG_EwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void NEG_EdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void NEG_EbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void NEG_EwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void NEG_EdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void ROL_EbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ROR_EbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RCL_EbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RCR_EbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SHL_EbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SHR_EbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SAR_EbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void ROL_EbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ROR_EbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RCL_EbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RCR_EbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SHL_EbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SHR_EbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SAR_EbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void ROL_EwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ROR_EwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RCL_EwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RCR_EwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SHL_EwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SHR_EwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SAR_EwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void ROL_EwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ROR_EwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RCL_EwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RCR_EwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SHL_EwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SHR_EwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SAR_EwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void ROL_EdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ROR_EdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RCL_EdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RCR_EdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SHL_EdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SHR_EdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SAR_EdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void ROL_EdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ROR_EdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RCL_EdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RCR_EdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SHL_EdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SHR_EdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SAR_EdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void TEST_EbIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void TEST_EwIwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void TEST_EdIdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void TEST_EbIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void TEST_EwIwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void TEST_EdIdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void IMUL_GdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void IMUL_GdEdIdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void MUL_ALEbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void IMUL_ALEbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void DIV_ALEbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void IDIV_ALEbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void MUL_EAXEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void IMUL_EAXEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void DIV_EAXEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void IDIV_EAXEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void INC_EbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void INC_EwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void INC_EdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void DEC_EbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void DEC_EwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void DEC_EdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void INC_EbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void INC_EwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void INC_EdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void DEC_EbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void DEC_EwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void DEC_EdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void CALL_EdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CALL_EwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void CALL32_Ep(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CALL16_Ep(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JMP32_Ep(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JMP16_Ep(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void JMP_EdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JMP_EwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void SLDT_Ew(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void STR_Ew(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LLDT_Ew(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LTR_Ew(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VERR_Ew(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VERW_Ew(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void SGDT_Ms(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SIDT_Ms(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LGDT_Ms(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LIDT_Ms(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SMSW_EwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SMSW_EwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LMSW_Ew(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    // LOAD methods
    BX_SMF void LOAD_Eb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOAD_Ew(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOAD_Ed(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#if BX_SUPPORT_X86_64
    BX_SMF void LOAD_Eq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#endif
    BX_SMF void LOADU_Wdq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOAD_Wdq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOAD_Wss(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOAD_Wsd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOAD_Ww(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOAD_Wb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#if BX_SUPPORT_AVX
    BX_SMF void LOAD_Vector(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOAD_Half_Vector(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOAD_MASK_Half_VectorB(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOAD_MASK_Half_VectorW(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOAD_MASK_Half_VectorD(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOAD_Quarter_Vector(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOAD_MASK_Quarter_VectorB(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOAD_MASK_Quarter_VectorW(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOAD_Eighth_Vector(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOAD_MASK_Eighth_VectorB(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#endif
#if BX_SUPPORT_EVEX
    BX_SMF void LOAD_MASK_Ww(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOAD_MASK_Wss(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOAD_MASK_Wsd(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOAD_MASK_VectorB(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOAD_MASK_VectorW(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOAD_MASK_VectorD(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOAD_MASK_VectorQ(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOAD_BROADCAST_VectorW(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOAD_BROADCAST_MASK_VectorW(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOAD_BROADCAST_VectorD(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOAD_BROADCAST_MASK_VectorD(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOAD_BROADCAST_VectorQ(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOAD_BROADCAST_MASK_VectorQ(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOAD_BROADCAST_Half_VectorW(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOAD_BROADCAST_MASK_Half_VectorW(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOAD_BROADCAST_Half_VectorD(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOAD_BROADCAST_MASK_Half_VectorD(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOAD_BROADCAST_Quarter_VectorW(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOAD_BROADCAST_MASK_Quarter_VectorW(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#endif

#if BX_SUPPORT_FPU == 0	// if FPU is disabled
    BX_SMF void FPU_ESC(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#endif

    BX_SMF void FWAIT(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

#if BX_SUPPORT_FPU
    // load/store
    BX_SMF void FLD_STi(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FLD_SINGLE_REAL(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FLD_DOUBLE_REAL(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FLD_EXTENDED_REAL(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FILD_WORD_INTEGER(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FILD_DWORD_INTEGER(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FILD_QWORD_INTEGER(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FBLD_PACKED_BCD(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void FST_STi(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FST_SINGLE_REAL(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FST_DOUBLE_REAL(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FSTP_EXTENDED_REAL(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FIST_WORD_INTEGER(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FIST_DWORD_INTEGER(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FISTP_QWORD_INTEGER(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FBSTP_PACKED_BCD(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void FISTTP16(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); } // SSE3
    BX_SMF void FISTTP32(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FISTTP64(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    // control
    BX_SMF void FNINIT(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FNCLEX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void FRSTOR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FNSAVE(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FLDENV(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FNSTENV(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void FLDCW(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FNSTCW(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FNSTSW(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FNSTSW_AX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    // const
    BX_SMF void FLD1(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FLDL2T(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FLDL2E(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FLDPI(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FLDLG2(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FLDLN2(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FLDZ(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    // add
    BX_SMF void FADD_ST0_STj(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FADD_STi_ST0(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FADD_SINGLE_REAL(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FADD_DOUBLE_REAL(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FIADD_WORD_INTEGER(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FIADD_DWORD_INTEGER(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    // mul
    BX_SMF void FMUL_ST0_STj(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FMUL_STi_ST0(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FMUL_SINGLE_REAL(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FMUL_DOUBLE_REAL(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FIMUL_WORD_INTEGER(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FIMUL_DWORD_INTEGER(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    // sub
    BX_SMF void FSUB_ST0_STj(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FSUBR_ST0_STj(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FSUB_STi_ST0(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FSUBR_STi_ST0(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FSUB_SINGLE_REAL(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FSUBR_SINGLE_REAL(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FSUB_DOUBLE_REAL(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FSUBR_DOUBLE_REAL(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void FISUB_WORD_INTEGER(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FISUBR_WORD_INTEGER(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FISUB_DWORD_INTEGER(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FISUBR_DWORD_INTEGER(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    // div
    BX_SMF void FDIV_ST0_STj(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FDIVR_ST0_STj(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FDIV_STi_ST0(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FDIVR_STi_ST0(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FDIV_SINGLE_REAL(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FDIVR_SINGLE_REAL(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FDIV_DOUBLE_REAL(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FDIVR_DOUBLE_REAL(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void FIDIV_WORD_INTEGER(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FIDIVR_WORD_INTEGER(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FIDIV_DWORD_INTEGER(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FIDIVR_DWORD_INTEGER(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    // compare
    BX_SMF void FCOM_STi(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FUCOM_STi(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FCOMI_ST0_STj(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FUCOMI_ST0_STj(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FCOM_SINGLE_REAL(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FCOM_DOUBLE_REAL(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FICOM_WORD_INTEGER(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FICOM_DWORD_INTEGER(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FCOMPP(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void FCMOVB_ST0_STj(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FCMOVE_ST0_STj(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FCMOVBE_ST0_STj(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FCMOVU_ST0_STj(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FCMOVNB_ST0_STj(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FCMOVNE_ST0_STj(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FCMOVNBE_ST0_STj(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FCMOVNU_ST0_STj(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    // misc
    BX_SMF void FXCH_STi(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FNOP(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FPLEGACY(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FCHS(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FABS(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FTST(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FXAM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FDECSTP(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FINCSTP(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FFREE_STi(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FFREEP_STi(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void F2XM1(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FYL2X(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FPTAN(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FPATAN(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FXTRACT(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FPREM1(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FPREM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FYL2XP1(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FSQRT(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FSINCOS(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FRNDINT(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#undef FSCALE            // <sys/param.h> is #included on Mac OS X from bochs.h
    BX_SMF void FSCALE(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FSIN(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FCOS(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#endif

    /* MMX */
    BX_SMF void PUNPCKLBW_PqQd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PUNPCKLWD_PqQd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PUNPCKLDQ_PqQd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PACKSSWB_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PCMPGTB_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PCMPGTW_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PCMPGTD_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PACKUSWB_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PUNPCKHBW_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PUNPCKHWD_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PUNPCKHDQ_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PACKSSDW_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVD_PqEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVD_PqEdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVQ_PqQqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVQ_PqQqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PCMPEQB_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PCMPEQW_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PCMPEQD_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void EMMS(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVD_EdPqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVD_EdPqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVQ_QqPqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PSRLW_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PSRLD_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PSRLQ_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PMULLW_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PSUBUSB_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PSUBUSW_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PAND_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PADDUSB_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PADDUSW_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PANDN_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PSRAW_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PSRAD_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PMULHW_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PSUBSB_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PSUBSW_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void POR_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PADDSB_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PADDSW_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PXOR_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PSLLW_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PSLLD_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PSLLQ_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PMADDWD_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PSUBB_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PSUBW_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PSUBD_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PADDB_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PADDW_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PADDD_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PSRLW_NqIb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PSRAW_NqIb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PSLLW_NqIb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PSRLD_NqIb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PSRAD_NqIb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PSLLD_NqIb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PSRLQ_NqIb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PSLLQ_NqIb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    /* MMX */

#if BX_SUPPORT_3DNOW
    BX_SMF void PFPNACC_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PI2FW_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PI2FD_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PF2IW_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PF2ID_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PFNACC_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PFCMPGE_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PFMIN_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PFRCP_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PFRSQRT_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PFSUB_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PFADD_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PFCMPGT_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PFMAX_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PFRCPIT1_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PFRSQIT1_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PFSUBR_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PFACC_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PFCMPEQ_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PFMUL_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PMULHRW_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PSWAPD_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#endif

    BX_SMF void SYSCALL(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SYSRET(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    /* SSE */
    BX_SMF void FXSAVE(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void FXRSTOR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LDMXCSR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void STMXCSR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PREFETCH(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    /* SSE */

    /* SSE */
    template <simd_xmm_shift func>
    BX_SMF void HANDLE_SSE_PSHIFT(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    template <simd_xmm_shift func>
    BX_SMF void HANDLE_SSE_SHIFT_IMM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    template <simd_xmm_1op func>
    BX_SMF void HANDLE_SSE_1OP(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    template <simd_xmm_2op func>
    BX_SMF void HANDLE_SSE_2OP(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    template <xmm_pfp_1op func>
    BX_SMF void HANDLE_SSE_PFP_1OP(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    template <xmm_pfp_2op func>
    BX_SMF void HANDLE_SSE_PFP_2OP(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void MOVUPS_VpsWpsM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVUPS_WpsVpsM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVSS_VssWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVSS_VssWssM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVSS_WssVssM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVSD_VsdWsdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVSD_WsdVsdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVHLPS_VpsWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVLPS_VpsMq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVLHPS_VpsWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVHPS_VpsMq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVHPS_MqVps(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVAPS_VpsWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVAPS_VpsWpsM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVAPS_WpsVpsM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CVTPI2PS_VpsQqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CVTPI2PS_VpsQqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CVTSI2SS_VssEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CVTTPS2PI_PqWps(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CVTTSS2SI_GdWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CVTPS2PI_PqWps(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CVTSS2SI_GdWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void UCOMISS_VssWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void COMISS_VssWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVMSKPS_GdUps(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SQRTSS_VssWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RSQRTPS_VpsWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RSQRTSS_VssWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RCPPS_VpsWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RCPSS_VssWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ADDSS_VssWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MULSS_VssWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SUBSS_VssWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MINSS_VssWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void DIVSS_VssWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MAXSS_VssWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PSHUFW_PqQqIb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PSHUFLW_VdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPPS_VpsWpsIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPSS_VssWssIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PINSRW_PqEwIb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PEXTRW_GdNqIb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SHUFPS_VpsWpsIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PMOVMSKB_GdNq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PMINUB_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PMAXUB_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PAVGB_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PAVGW_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PMULHUW_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PMINSW_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PMAXSW_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PSADBW_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MASKMOVQ_PqNq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    /* SSE */

    /* SSE2 */
    BX_SMF void MOVSD_VsdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CVTPI2PD_VpdQqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CVTPI2PD_VpdQqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CVTSI2SD_VsdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CVTTPD2PI_PqWpd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CVTTSD2SI_GdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CVTPD2PI_PqWpd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CVTSD2SI_GdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void UCOMISD_VsdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void COMISD_VsdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVMSKPD_GdUpd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SQRTSD_VsdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ADDSD_VsdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MULSD_VsdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SUBSD_VsdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CVTPS2PD_VpdWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CVTPD2PS_VpsWpdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CVTSD2SS_VssWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CVTSS2SD_VsdWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CVTDQ2PS_VpsWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CVTPS2DQ_VdqWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CVTTPS2DQ_VdqWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MINSD_VsdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void DIVSD_VsdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MAXSD_VsdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVD_VdqEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PSHUFD_VdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PSHUFHW_VdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVD_EdVdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVQ_VqWqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPPD_VpdWpdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPSD_VsdWsdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PINSRW_VdqEwIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PINSRW_VdqEwIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PEXTRW_GdUdqIb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SHUFPD_VpdWpdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVDQ2Q_PqUdq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVQ2DQ_VdqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PMOVMSKB_GdUdq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CVTTPD2DQ_VqWpdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CVTPD2DQ_VqWpdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CVTDQ2PD_VpdWqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PMULUDQ_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MASKMOVDQU_VdqUdq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PADDQ_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PSUBQ_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    /* SSE2 */

    /* SSE3 */
    BX_SMF void MOVDDUP_VpdWqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVSLDUP_VpsWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVSHDUP_VpsWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ADDSUBPD_VpdWpdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ADDSUBPS_VpsWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    /* SSE3 */

#if BX_CPU_LEVEL >= 6
  /* SSSE3 */
    BX_SMF void PSHUFB_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PHADDW_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PHADDD_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PHADDSW_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PMADDUBSW_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PHSUBSW_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PHSUBW_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PHSUBD_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PSIGNB_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PSIGNW_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PSIGND_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PMULHRSW_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PABSB_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PABSW_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PABSD_PqQq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PALIGNR_PqQqIb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void PSHUFB_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PALIGNR_VdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    /* SSSE3 */

    /* SSE4.1 */
    BX_SMF void PBLENDVB_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BLENDVPS_VpsWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BLENDVPD_VpdWpdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PTEST_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PMOVSXBW_VdqWqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PMOVSXBD_VdqWdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PMOVSXBQ_VdqWwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PMOVSXWD_VdqWqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PMOVSXWQ_VdqWdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PMOVSXDQ_VdqWqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PMOVZXBW_VdqWqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PMOVZXBD_VdqWdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PMOVZXBQ_VdqWwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PMOVZXWD_VdqWqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PMOVZXWQ_VdqWdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PMOVZXDQ_VdqWqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PHMINPOSUW_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ROUNDPS_VpsWpsIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ROUNDPD_VpdWpdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ROUNDSS_VssWssIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ROUNDSD_VsdWsdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BLENDPS_VpsWpsIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BLENDPD_VpdWpdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PBLENDW_VdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PEXTRB_EdVdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PEXTRB_MbVdqIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PEXTRW_EdVdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PEXTRW_MwVdqIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PEXTRD_EdVdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PEXTRD_EdVdqIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#if BX_SUPPORT_X86_64
    BX_SMF void PEXTRQ_EqVdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PEXTRQ_EqVdqIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#endif
    BX_SMF void PINSRB_VdqEbIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PINSRB_VdqEbIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PINSRD_VdqEdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PINSRD_VdqEdIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#if BX_SUPPORT_X86_64
    BX_SMF void PINSRQ_VdqEqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PINSRQ_VdqEqIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#endif
    BX_SMF void DPPS_VpsWpsIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void DPPD_VpdHpdWpdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MPSADBW_VdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void INSERTPS_VpsWssIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void INSERTPS_VpsWssIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    /* SSE4.1 */

    /* SSE4.2 */
    BX_SMF void CRC32_GdEbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CRC32_GdEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CRC32_GdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#if BX_SUPPORT_X86_64
    BX_SMF void CRC32_GdEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#endif
    BX_SMF void PCMPESTRM_VdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PCMPESTRI_VdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PCMPISTRM_VdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PCMPISTRI_VdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    /* SSE4.2 */

    /* MOVBE Intel Atom(R) instruction */
    BX_SMF void MOVBE_GwMw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVBE_GdMd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVBE_MwGw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVBE_MdGd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#if BX_SUPPORT_X86_64
    BX_SMF void MOVBE_GqMq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVBE_MqGq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#endif
    /* MOVBE Intel Atom(R) instruction */
#endif

  /* XSAVE/XRSTOR extensions */
    BX_SMF void XSAVE(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void XSAVEC(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void XRSTOR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void XGETBV(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void XSETBV(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    /* XSAVE/XRSTOR extensions */

#if BX_CPU_LEVEL >= 6
  /* AES instructions */
    BX_SMF void AESIMC_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void AESKEYGENASSIST_VdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void AESENC_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void AESENCLAST_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void AESDEC_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void AESDECLAST_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PCLMULQDQ_VdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    /* AES instructions */

    /* SHA instructions */
    BX_SMF void SHA1NEXTE_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SHA1MSG1_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SHA1MSG2_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SHA256RNDS2_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SHA256MSG1_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SHA256MSG2_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SHA1RNDS4_VdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    /* SHA instructions */

    /* GFNI instructions */
    BX_SMF void GF2P8AFFINEINVQB_VdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void GF2P8AFFINEQB_VdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void GF2P8MULB_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    /* GFNI instructions */
#endif

  /* VMX instructions */
    BX_SMF void VMXON(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMXOFF(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMCALL(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMLAUNCH(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMCLEAR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMPTRLD(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMPTRST(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMREAD_EdGd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMWRITE_GdEd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#if BX_SUPPORT_X86_64
    BX_SMF void VMREAD_EqGq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMWRITE_GqEq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#endif
    BX_SMF void VMFUNC(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    /* VMX instructions */

#if BX_SUPPORT_SVM
  /* SVM instructions */
    BX_SMF void VMRUN(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMMCALL(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMLOAD(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMSAVE(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SKINIT(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CLGI(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void STGI(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void INVLPGA(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    /* SVM instructions */
#endif

  /* SMX instructions */
    BX_SMF void GETSEC(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    /* SMX instructions */

#if BX_CPU_LEVEL >= 6
  /* VMXx2 */
    BX_SMF void INVEPT(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void INVVPID(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    /* VMXx2 */

    BX_SMF void INVPCID(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#endif

    /* CET instructions */
#if BX_SUPPORT_CET
    BX_SMF void INCSSPD(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void INCSSPQ(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RDSSPD(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RDSSPQ(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SAVEPREVSSP(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RSTORSSP(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void WRSSD(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void WRUSSD(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void WRSSQ(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void WRUSSQ(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SETSSBSY(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CLRSSBSY(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ENDBRANCH32(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ENDBRANCH64(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#endif
    /* CET instructions */

    BX_SMF void MOVDIR64B(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

#if BX_SUPPORT_AVX
    /* AVX */
    BX_SMF void VZEROUPPER(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VZEROALL(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    template <simd_xmm_shift func>
    BX_SMF void HANDLE_AVX_PSHIFT(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    template <simd_xmm_shift func>
    BX_SMF void HANDLE_AVX_SHIFT_IMM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    template <simd_xmm_1op func>
    BX_SMF void HANDLE_AVX_1OP(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    template <simd_xmm_2op func>
    BX_SMF void HANDLE_AVX_2OP(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    template <simd_xmm_3op func>
    BX_SMF void HANDLE_AVX_3OP(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    template <xmm_pfp_1op func>
    BX_SMF void HANDLE_AVX_PFP_1OP(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    template <xmm_pfp_2op func>
    BX_SMF void HANDLE_AVX_PFP_2OP(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    template <xmm_pfp_3op func>
    BX_SMF void HANDLE_AVX_PFP_3OP(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VMOVSS_VssHpsWssR(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVSD_VsdHpdWsdR(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVAPS_VpsWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVAPS_VpsWpsM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVUPS_VpsWpsM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVAPS_WpsVpsM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVUPS_WpsVpsM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVLPD_VpdHpdMq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVHPD_VpdHpdMq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVLHPS_VpsHpsWps(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVHLPS_VpsHpsWps(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVSHDUP_VpsWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVSLDUP_VpsWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVDDUP_VpdWpdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVMSKPS_GdUps(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVMSKPD_GdUpd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVMSKB_GdUdq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VSQRTSS_VssHpsWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VSQRTSD_VsdHpdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VADDSS_VssHpsWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VADDSD_VsdHpdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMULSS_VssHpsWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMULSD_VsdHpdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VSUBSS_VssHpsWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VSUBSD_VsdHpdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTSS2SD_VsdWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTSD2SS_VssWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTDQ2PS_VpsWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTPS2DQ_VdqWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPS2DQ_VdqWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTPS2PD_VpdWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTPD2PS_VpsWpdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTPD2DQ_VdqWpdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTDQ2PD_VpdWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPD2DQ_VdqWpdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTSI2SD_VsdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTSI2SS_VssEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTSI2SD_VsdEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTSI2SS_VssEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMINSS_VssHpsWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMINSD_VsdHpdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VDIVSS_VssHpsWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VDIVSD_VsdHpdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMAXSS_VssHpsWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMAXSD_VsdHpdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCMPPS_VpsHpsWpsIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCMPSS_VssHpsWssIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCMPPD_VpdHpdWpdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCMPSD_VsdHpdWsdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VROUNDPS_VpsWpsIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VROUNDPD_VpdWpdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VROUNDSS_VssHpsWssIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VROUNDSD_VsdHpdWsdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VDPPS_VpsHpsWpsIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VRSQRTPS_VpsWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VRSQRTSS_VssHpsWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VRCPPS_VpsWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VRCPSS_VssHpsWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VSHUFPS_VpsHpsWpsIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VSHUFPD_VpdHpdWpdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VBLENDPS_VpsHpsWpsIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VBLENDPD_VpdHpdWpdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPBLENDVB_VdqHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPTEST_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VTESTPS_VpsWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VTESTPD_VpdWpdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VBROADCASTF128_VdqMdq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VBLENDVPS_VpsHpsWpsIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VBLENDVPD_VpdHpdWpdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VINSERTF128_VdqHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VEXTRACTF128_WdqVdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VEXTRACTF128_WdqVdqIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPERMILPS_VpsWpsIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPERMILPD_VpdWpdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPERM2F128_VdqHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMASKMOVPS_VpsHpsMps(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMASKMOVPD_VpdHpdMpd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMASKMOVPS_MpsHpsVps(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMASKMOVPD_MpdHpdVpd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPINSRB_VdqHdqEbIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPINSRB_VdqHdqEbIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPINSRW_VdqHdqEwIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPINSRW_VdqHdqEwIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPINSRD_VdqHdqEdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPINSRD_VdqHdqEdIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPINSRQ_VdqHdqEqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPINSRQ_VdqHdqEqIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VINSERTPS_VpsHpsWssIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VINSERTPS_VpsHpsWssIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VCVTPH2PS_VpsWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTPS2PH_WpsVpsIb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    /* AVX */

    /* AVX2 */
    BX_SMF void VPSHUFHW_VdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPSHUFLW_VdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMPSADBW_VdqHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPBLENDW_VdqHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPALIGNR_VdqHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPMOVSXBW_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVSXBD_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVSXBQ_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVSXWD_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVSXWQ_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVSXDQ_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPMOVZXBW_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVZXBD_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVZXBQ_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVZXWD_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVZXWQ_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVZXDQ_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPERMD_VdqHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPERMQ_VdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPBROADCASTB_VdqWbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPBROADCASTW_VdqWwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPBROADCASTD_VdqWdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPBROADCASTQ_VdqWqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VGATHERDPS_VpsHps(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VGATHERQPS_VpsHps(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VGATHERDPD_VpdHpd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VGATHERQPD_VpdHpd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    /* AVX2 */

    /* AVX2 FMA */
    BX_SMF void VFMADDSD_VpdHsdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFMADDSS_VpsHssWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFMSUBSD_VpdHsdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFMSUBSS_VpsHssWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFNMADDSD_VpdHsdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFNMADDSS_VpsHssWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFNMSUBSD_VpdHsdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFNMSUBSS_VpsHssWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    /* AVX2 FMA */

    /* BMI */
    BX_SMF void ANDN_GdBdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MULX_GdBdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BLSI_BdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BLSMSK_BdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BLSR_BdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RORX_GdEdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SHLX_GdEdBdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SHRX_GdEdBdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SARX_GdEdBdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BEXTR_GdEdBdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BZHI_GdEdBdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PEXT_GdBdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PDEP_GdBdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void ANDN_GqBqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MULX_GqBqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BLSI_BqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BLSMSK_BqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BLSR_BqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RORX_GqEqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SHLX_GqEqBqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SHRX_GqEqBqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SARX_GqEqBqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BEXTR_GqEqBqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BZHI_GqEqBqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PEXT_GqBqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PDEP_GqBqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    /* BMI */

    /* CMPccXADD */
    BX_SMF void CMPBEXADD_EdGdBd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPBEXADD_EqGqBq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPBXADD_EdGdBd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPBXADD_EqGqBq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPLEXADD_EdGdBd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPLEXADD_EqGqBq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPLXADD_EdGdBd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPLXADD_EqGqBq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPNBEXADD_EdGdBd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPNBEXADD_EqGqBq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPNBXADD_EdGdBd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPNBXADD_EqGqBq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPNLEXADD_EdGdBd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPNLEXADD_EqGqBq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPNLXADD_EdGdBd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPNLXADD_EqGqBq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPNOXADD_EdGdBd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPNOXADD_EqGqBq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPNPXADD_EdGdBd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPNPXADD_EqGqBq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPNSXADD_EdGdBd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPNSXADD_EqGqBq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPNZXADD_EdGdBd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPNZXADD_EqGqBq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPOXADD_EdGdBd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPOXADD_EqGqBq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPPXADD_EdGdBd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPPXADD_EqGqBq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPSXADD_EdGdBd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPSXADD_EqGqBq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPZXADD_EdGdBd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPZXADD_EqGqBq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    /* CMPccXADD */

    /* FMA4 specific handlers (AMD) */
    BX_SMF void VFMADDSS_VssHssWssVIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFMADDSD_VsdHsdWsdVIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFMSUBSS_VssHssWssVIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFMSUBSD_VsdHsdWsdVIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFNMADDSS_VssHssWssVIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFNMADDSD_VsdHsdWsdVIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFNMSUBSS_VssHssWssVIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFNMSUBSD_VsdHsdWsdVIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    /* FMA4 specific handlers (AMD) */

    /* XOP (AMD) */
    BX_SMF void VPCMOV_VdqHdqWdqVIb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPPERM_VdqHdqWdqVIb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPSHAB_VdqWdqHdq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPSHAW_VdqWdqHdq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPSHAD_VdqWdqHdq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPSHAQ_VdqWdqHdq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPROTB_VdqWdqHdq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPROTW_VdqWdqHdq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPROTD_VdqWdqHdq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPROTQ_VdqWdqHdq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPSHLB_VdqWdqHdq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPSHLW_VdqWdqHdq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPSHLD_VdqWdqHdq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPSHLQ_VdqWdqHdq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMACSSWW_VdqHdqWdqVIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMACSSWD_VdqHdqWdqVIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMACSSDQL_VdqHdqWdqVIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMACSSDD_VdqHdqWdqVIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMACSSDQH_VdqHdqWdqVIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMACSWW_VdqHdqWdqVIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMACSWD_VdqHdqWdqVIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMACSDQL_VdqHdqWdqVIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMACSDD_VdqHdqWdqVIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMACSDQH_VdqHdqWdqVIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMADCSSWD_VdqHdqWdqVIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMADCSWD_VdqHdqWdqVIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPROTB_VdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPROTW_VdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPROTD_VdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPROTQ_VdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPCOMB_VdqHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPCOMW_VdqHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPCOMD_VdqHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPCOMQ_VdqHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPCOMUB_VdqHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPCOMUW_VdqHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPCOMUD_VdqHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPCOMUQ_VdqHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFRCZPS_VpsWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFRCZPD_VpdWpdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFRCZSS_VssWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFRCZSD_VsdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPHADDBW_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPHADDBD_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPHADDBQ_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPHADDWD_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPHADDWQ_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPHADDDQ_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPHADDUBW_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPHADDUBD_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPHADDUBQ_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPHADDUWD_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPHADDUWQ_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPHADDUDQ_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPHSUBBW_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPHSUBWD_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPHSUBDQ_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPERMIL2PS_VdqHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPERMIL2PD_VdqHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    /* XOP (AMD) */

    /* TBM (AMD) */
    BX_SMF void BEXTR_GdEdIdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BLCFILL_BdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BLCI_BdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BLCIC_BdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BLCMSK_BdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BLCS_BdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BLSFILL_BdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BLSIC_BdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void T1MSKC_BdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void TZMSK_BdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void BEXTR_GqEqIdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BLCFILL_BqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BLCI_BqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BLCIC_BqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BLCMSK_BqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BLCS_BqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BLSFILL_BqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BLSIC_BqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void T1MSKC_BqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void TZMSK_BqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    /* TBM (AMD) */
#endif

  // RAO-INT
    BX_SMF void AADD_EdGdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void AAND_EdGdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void AOR_EdGdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void AXOR_EdGdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#if BX_SUPPORT_X86_64
    BX_SMF void AADD_EqGqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void AAND_EqGqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void AOR_EqGqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void AXOR_EqGqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#endif

#if BX_SUPPORT_AVX
    // VAES: VEX extended AES instructions
    BX_SMF void VAESENC_VdqHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VAESENCLAST_VdqHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VAESDEC_VdqHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VAESDECLAST_VdqHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPCLMULQDQ_VdqHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    /* GFNI instructions: VEX extended form */
    BX_SMF void VGF2P8AFFINEINVQB_VdqHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VGF2P8AFFINEQB_VdqHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VGF2P8MULB_VdqHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    /* SHA512 instructions: VEX encoded */
    BX_SMF void VSHA512MSG1_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VSHA512MSG2_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VSHA512RNDS2_VdqHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    /* SM3 instructions: VEX encoded */
    BX_SMF void VSM3MSG1_VdqHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VSM3MSG2_VdqHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VSM3RNDS2_VdqHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    /* SM4 instructions: VEX encoded */
    BX_SMF void VSM4KEY4_VdqHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VSM4RNDS4_VdqHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    /* AVX encoded IFMA instructions */
    BX_SMF void VPMADD52LUQ_VdqHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMADD52HUQ_VdqHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    /* AVX-NE-CONVERT instructions */
    BX_SMF void VBCSTNEBF162PS_VpsWwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VBCSTNESH2PS_VpsWshM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTNEEBF162PS_VpsWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTNEOBF162PS_VpsWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTNEEPH2PS_VpsWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTNEOPH2PS_VpsWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTNEPS2BF16_VphWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    // AVX512 OPMASK instructions (VEX encoded)
    BX_SMF void KADDB_KGbKHbKEbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KANDB_KGbKHbKEbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KANDNB_KGbKHbKEbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KMOVB_KGbKEbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KMOVB_KGbKEbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KMOVB_KEbKGbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KMOVB_KGbEbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KMOVB_GdKEbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KNOTB_KGbKEbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KORB_KGbKHbKEbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KORTESTB_KGbKEbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KSHIFTLB_KGbKEbIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KSHIFTRB_KGbKEbIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KXNORB_KGbKHbKEbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KXORB_KGbKHbKEbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KTESTB_KGbKEbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void KADDW_KGwKHwKEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KANDW_KGwKHwKEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KANDNW_KGwKHwKEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KMOVW_KGwKEwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KMOVW_KGwKEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KMOVW_KEwKGwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KMOVW_KGwEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KMOVW_GdKEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KUNPCKBW_KGwKHbKEbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KNOTW_KGwKEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KORW_KGwKHwKEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KORTESTW_KGwKEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KSHIFTLW_KGwKEwIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KSHIFTRW_KGwKEwIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KXNORW_KGwKHwKEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KXORW_KGwKHwKEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KTESTW_KGwKEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void KADDD_KGdKHdKEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KANDD_KGdKHdKEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KANDND_KGdKHdKEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KMOVD_KGdKEdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KMOVD_KGdKEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KMOVD_KEdKGdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KMOVD_KGdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KMOVD_GdKEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KUNPCKWD_KGdKHwKEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KNOTD_KGdKEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KORD_KGdKHdKEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KORTESTD_KGdKEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KSHIFTLD_KGdKEdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KSHIFTRD_KGdKEdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KXNORD_KGdKHdKEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KXORD_KGdKHdKEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KTESTD_KGdKEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void KADDQ_KGqKHqKEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KANDQ_KGqKHqKEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KANDNQ_KGqKHqKEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KMOVQ_KGqKEqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KMOVQ_KGqKEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KMOVQ_KEqKGqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KMOVQ_KGqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KMOVQ_GqKEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KUNPCKDQ_KGqKHdKEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KNOTQ_KGqKEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KORQ_KGqKHqKEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KORTESTQ_KGqKEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KSHIFTLQ_KGqKEqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KSHIFTRQ_KGqKEqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KXNORQ_KGqKHqKEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KXORQ_KGqKHqKEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void KTESTQ_KGqKEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    // AVX512 OPMASK instructions (VEX encoded)
#endif

#if BX_SUPPORT_EVEX
    template <simd_xmm_1op func>
    BX_SMF void HANDLE_AVX512_1OP_WORD_EL_MASK(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    template <simd_xmm_2op func>
    BX_SMF void HANDLE_AVX512_2OP_QWORD_EL_MASK(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    template <simd_xmm_2op func>
    BX_SMF void HANDLE_AVX512_2OP_DWORD_EL_MASK(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    template <simd_xmm_2op func>
    BX_SMF void HANDLE_AVX512_2OP_WORD_EL_MASK(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    template <simd_xmm_2op func>
    BX_SMF void HANDLE_AVX512_2OP_BYTE_EL_MASK(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    template <simd_xmm_3op func>
    BX_SMF void HANDLE_AVX512_3OP_QWORD_EL_MASK(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    template <simd_xmm_3op func>
    BX_SMF void HANDLE_AVX512_3OP_DWORD_EL_MASK(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    template <simd_xmm_3op func>
    BX_SMF void HANDLE_AVX512_3OP_WORD_EL_MASK(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    template <simd_xmm_shift func>
    BX_SMF void HANDLE_AVX512_PSHIFT_QWORD_EL_MASK(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    template <simd_xmm_shift func>
    BX_SMF void HANDLE_AVX512_PSHIFT_DWORD_EL_MASK(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    template <simd_xmm_shift func>
    BX_SMF void HANDLE_AVX512_PSHIFT_WORD_EL_MASK(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    template <simd_xmm_shift func>
    BX_SMF void HANDLE_AVX512_SHIFT_IMM_QWORD_EL_MASK(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    template <simd_xmm_shift func>
    BX_SMF void HANDLE_AVX512_SHIFT_IMM_DWORD_EL_MASK(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    template <simd_xmm_shift func>
    BX_SMF void HANDLE_AVX512_SHIFT_IMM_WORD_EL_MASK(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    template <xmm_pfp_1op_mask func>
    BX_SMF void HANDLE_AVX512_MASK_PFP_1OP_HALF(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    template <xmm_pfp_1op_mask func>
    BX_SMF void HANDLE_AVX512_MASK_PFP_1OP_SINGLE(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    template <xmm_pfp_1op_mask func>
    BX_SMF void HANDLE_AVX512_MASK_PFP_1OP_DOUBLE(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    template <xmm_pfp_2op_mask func>
    BX_SMF void HANDLE_AVX512_MASK_PFP_2OP_HALF(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    template <xmm_pfp_2op_mask func>
    BX_SMF void HANDLE_AVX512_MASK_PFP_2OP_SINGLE(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    template <xmm_pfp_2op_mask func>
    BX_SMF void HANDLE_AVX512_MASK_PFP_2OP_DOUBLE(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    template <xmm_pfp_3op_mask func>
    BX_SMF void HANDLE_AVX512_MASK_PFP_3OP_HALF(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    template <xmm_pfp_3op_mask func>
    BX_SMF void HANDLE_AVX512_MASK_PFP_3OP_SINGLE(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    template <xmm_pfp_3op_mask func>
    BX_SMF void HANDLE_AVX512_MASK_PFP_3OP_DOUBLE(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VADDSS_MASK_VssHpsWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VADDSD_MASK_VsdHpdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VSUBSS_MASK_VssHpsWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VSUBSD_MASK_VsdHpdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMULSS_MASK_VssHpsWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMULSD_MASK_VsdHpdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VDIVSS_MASK_VssHpsWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VDIVSD_MASK_VsdHpdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMINSS_MASK_VssHpsWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMINSD_MASK_VsdHpdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMAXSS_MASK_VssHpsWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMAXSD_MASK_VsdHpdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VSQRTSS_MASK_VssHpsWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VSQRTSD_MASK_VsdHpdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VFPCLASSPS_MASK_KGwWpsIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFPCLASSPD_MASK_KGbWpdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFPCLASSSS_MASK_KGbWssIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFPCLASSSD_MASK_KGbWsdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VGETEXPSS_VssHpsWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VGETEXPSD_VsdHpdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VGETEXPSS_MASK_VssHpsWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VGETEXPSD_MASK_VsdHpdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VGETMANTPS_MASK_VpsWpsIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VGETMANTPD_MASK_VpdWpdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VGETMANTSS_MASK_VssHpsWssIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VGETMANTSD_MASK_VsdHpdWsdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VRNDSCALEPS_MASK_VpsWpsIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VRNDSCALEPD_MASK_VpdWpdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VRNDSCALESS_MASK_VssHpsWssIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VRNDSCALESD_MASK_VsdHpdWsdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VREDUCEPS_MASK_VpsWpsIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VREDUCEPD_MASK_VpdWpdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VREDUCESS_MASK_VssHpsWssIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VREDUCESD_MASK_VsdHpdWsdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VSCALEFSS_VssHpsWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VSCALEFSD_VsdHpdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VSCALEFSS_MASK_VssHpsWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VSCALEFSD_MASK_VsdHpdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VRANGEPS_MASK_VpsHpsWpsIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VRANGEPD_MASK_VpdHpdWpdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VRANGESS_MASK_VssHpsWssIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VRANGESD_MASK_VsdHpdWsdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VRCP14PS_MASK_VpsWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VRCP14PD_MASK_VpdWpdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VRCP14SS_MASK_VssHpsWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VRCP14SD_MASK_VsdHpdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VRSQRT14PS_MASK_VpsWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VRSQRT14PD_MASK_VpdWpdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VRSQRT14SS_MASK_VssHpsWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VRSQRT14SD_MASK_VsdHpdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VCVTSS2USI_GdWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTSS2USI_GqWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTSD2USI_GdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTSD2USI_GqWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VCVTTSS2USI_GdWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTSS2USI_GqWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTSD2USI_GdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTSD2USI_GqWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VCVTUSI2SD_VsdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTUSI2SS_VssEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTUSI2SD_VsdEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTUSI2SS_VssEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VCVTTPS2UDQ_VdqWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPS2UDQ_MASK_VdqWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPD2UDQ_VdqWpdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPD2UDQ_MASK_VdqWpdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VCVTPS2UDQ_VdqWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTPS2UDQ_MASK_VdqWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTPD2UDQ_VdqWpdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTPD2UDQ_MASK_VdqWpdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VCVTUDQ2PS_VpsWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTUDQ2PS_MASK_VpsWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTUDQ2PD_VpdWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTUDQ2PD_MASK_VpdWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VCVTQQ2PS_VpsWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTQQ2PS_MASK_VpsWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTUQQ2PS_VpsWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTUQQ2PS_MASK_VpsWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VCVTQQ2PD_VpdWdqR(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTQQ2PD_MASK_VpdWdqR(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTUQQ2PD_VpdWdqR(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTUQQ2PD_MASK_VpdWdqR(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VCVTPS2QQ_VdqWpsR(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTPS2QQ_MASK_VdqWpsR(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPS2QQ_VdqWpsR(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPS2QQ_MASK_VdqWpsR(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTPS2UQQ_VdqWpsR(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTPS2UQQ_MASK_VdqWpsR(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPS2UQQ_VdqWpsR(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPS2UQQ_MASK_VdqWpsR(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VCVTPD2QQ_VdqWpdR(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTPD2QQ_MASK_VdqWpdR(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPD2QQ_VdqWpdR(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPD2QQ_MASK_VdqWpdR(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTPD2UQQ_VdqWpdR(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTPD2UQQ_MASK_VdqWpdR(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPD2UQQ_VdqWpdR(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPD2UQQ_MASK_VdqWpdR(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VCVTPD2PS_MASK_VpsWpdR(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTPS2PD_MASK_VpdWpsR(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTSS2SD_MASK_VsdWssR(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTSD2SS_MASK_VssWsdR(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VCVTPS2DQ_MASK_VdqWpsR(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPS2DQ_MASK_VdqWpsR(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTDQ2PS_MASK_VpsWdqR(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VCVTPD2DQ_MASK_VdqWpdR(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPD2DQ_MASK_VdqWpdR(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTDQ2PD_MASK_VpdWdqR(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VCVTPH2PS_MASK_VpsWpsR(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTPS2PH_MASK_WpsVpsIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTPS2PH_MASK_WpsVpsIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VCVTNEPS2BF16_MASK_VphWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTNE2PS2BF16_MASK_VphHpsWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VDPBF16PS_MASK_VpsHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPABSB_MASK_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPABSW_MASK_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPABSD_MASK_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPABSQ_MASK_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VMOVAPS_MASK_VpsWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVAPS_MASK_VpsWpsM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVAPS_MASK_WpsVpsM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVAPD_MASK_VpdWpdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVAPD_MASK_VpdWpdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVAPD_MASK_WpdVpdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VMOVUPS_MASK_VpsWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVUPS_MASK_VpsWpsM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVUPS_MASK_WpsVpsM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVUPD_MASK_VpdWpdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVUPD_MASK_VpdWpdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVUPD_MASK_WpdVpdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VMOVDQU8_MASK_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVDQU8_MASK_VdqWdqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVDQU8_MASK_WdqVdqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VMOVDQU16_MASK_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVDQU16_MASK_VdqWdqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVDQU16_MASK_WdqVdqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VMOVSD_MASK_VsdWsdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVSS_MASK_VssWssM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVSH_MASK_VshWshM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVSH_VshWshM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVSD_MASK_WsdVsdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVSS_MASK_WssVssM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVSH_MASK_WshVshM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVSH_WshVshM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVSD_MASK_VsdHpdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVSS_MASK_VssHpsWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVSH_MASK_VshHphWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVSH_VshHphWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VMOVW_VshEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVW_EdVshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VMOVSHDUP_MASK_VpsWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVSLDUP_MASK_VpsWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVDDUP_MASK_VpdWpdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VFMADDSD_MASK_VpdHsdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFMADDSS_MASK_VpsHssWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFMSUBSD_MASK_VpdHsdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFMSUBSS_MASK_VpsHssWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFNMADDSD_MASK_VpdHsdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFNMADDSS_MASK_VpsHssWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFNMSUBSD_MASK_VpdHsdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFNMSUBSS_MASK_VpsHssWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VFIXUPIMMSS_MASK_VssHssWssIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFIXUPIMMSD_MASK_VsdHsdWsdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFIXUPIMMPS_VpsHpsWpsIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFIXUPIMMPD_VpdHpdWpdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFIXUPIMMPS_MASK_VpsHpsWpsIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFIXUPIMMPD_MASK_VpdHpdWpdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VBLENDMPS_MASK_VpsHpsWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VBLENDMPD_MASK_VpdHpdWpdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPBLENDMB_MASK_VdqHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPBLENDMW_MASK_VdqHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPCMPB_MASK_KGqHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPCMPUB_MASK_KGqHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPCMPW_MASK_KGdHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPCMPUW_MASK_KGdHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPCMPD_MASK_KGwHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPCMPUD_MASK_KGwHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPCMPQ_MASK_KGbHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPCMPUQ_MASK_KGbHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPCMPEQB_MASK_KGqHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPCMPGTB_MASK_KGqHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPCMPEQW_MASK_KGdHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPCMPGTW_MASK_KGdHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPCMPEQD_MASK_KGwHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPCMPGTD_MASK_KGwHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPCMPEQQ_MASK_KGbHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPCMPGTQ_MASK_KGbHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPTESTMB_MASK_KGqHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPTESTNMB_MASK_KGqHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPTESTMW_MASK_KGdHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPTESTNMW_MASK_KGdHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPTESTMD_MASK_KGwHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPTESTNMD_MASK_KGwHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPTESTMQ_MASK_KGbHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPTESTNMQ_MASK_KGbHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VCMPPS_MASK_KGwHpsWpsIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCMPPD_MASK_KGbHpdWpdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCMPSS_MASK_KGbHssWssIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCMPSD_MASK_KGbHsdWsdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPSHUFB_MASK_VdqHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPERMQ_MASK_VdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VSHUFPS_MASK_VpsHpsWpsIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VSHUFPD_MASK_VpdHpdWpdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPSHUFLW_MASK_VdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPSHUFHW_MASK_VdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPERMILPS_MASK_VpsWpsIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPERMILPD_MASK_VpdWpdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VSHUFF32x4_MASK_VpsHpsWpsIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VSHUFF64x2_MASK_VpdHpdWpdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VALIGND_MASK_VdqHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VALIGNQ_MASK_VdqHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPALIGNR_MASK_VdqHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VDBPSADBW_MASK_VdqHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPERMI2B_MASK_VdqHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPERMI2W_MASK_VdqHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPERMT2B_MASK_VdqHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPERMT2W_MASK_VdqHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPERMI2PS_MASK_VpsHpsWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPERMI2PD_MASK_VpdHpdWpdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPERMT2PS_MASK_VpsHpsWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPERMT2PD_MASK_VpdHpdWpdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPERMB_MASK_VdqHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPERMW_MASK_VdqHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPERMPS_MASK_VpsHpsWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPERMPD_MASK_VpdHpdWpdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VINSERTF32x4_MASK_VpsHpsWpsIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VINSERTF64x2_MASK_VpdHpdWpdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VINSERTF64x4_VpdHpdWpdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VINSERTF64x4_MASK_VpdHpdWpdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VINSERTF32x8_MASK_VpsHpsWpsIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VEXTRACTF32x4_MASK_WpsVpsIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VEXTRACTF32x4_MASK_WpsVpsIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VEXTRACTF64x4_WpdVpdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VEXTRACTF64x4_WpdVpdIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VEXTRACTF64x4_MASK_WpdVpdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VEXTRACTF64x4_MASK_WpdVpdIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VEXTRACTF32x8_MASK_WpsVpsIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VEXTRACTF32x8_MASK_WpsVpsIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VEXTRACTF64x2_MASK_WpdVpdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VEXTRACTF64x2_MASK_WpdVpdIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPBROADCASTB_MASK_VdqWbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPBROADCASTW_MASK_VdqWwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPBROADCASTD_MASK_VdqWdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPBROADCASTQ_MASK_VdqWqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPBROADCASTB_MASK_VdqWbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPBROADCASTW_MASK_VdqWwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPBROADCASTD_MASK_VdqWdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPBROADCASTQ_MASK_VdqWqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPBROADCASTB_VdqEbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPBROADCASTW_VdqEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPBROADCASTD_VdqEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPBROADCASTQ_VdqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPBROADCASTB_MASK_VdqEbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPBROADCASTW_MASK_VdqEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPBROADCASTD_MASK_VdqEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPBROADCASTQ_MASK_VdqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VBROADCASTF32x2_MASK_VpsWqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VBROADCASTF32x2_MASK_VpsWqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VBROADCASTF64x2_MASK_VpdMpd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VBROADCASTF32x4_MASK_VpsMps(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VBROADCASTF64x4_VpdMpd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VBROADCASTF32x8_MASK_VpsMps(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VBROADCASTF64x4_MASK_VpdMpd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPTERNLOGD_VdqHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPTERNLOGQ_VdqHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPTERNLOGD_MASK_VdqHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPTERNLOGQ_MASK_VdqHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VGATHERDPS_MASK_VpsVSib(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VGATHERQPS_MASK_VpsVSib(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VGATHERDPD_MASK_VpdVSib(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VGATHERQPD_MASK_VpdVSib(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VSCATTERDPS_MASK_VSibVps(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VSCATTERQPS_MASK_VSibVps(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VSCATTERDPD_MASK_VSibVpd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VSCATTERQPD_MASK_VSibVpd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VCOMPRESSPS_MASK_WpsVps(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCOMPRESSPD_MASK_WpdVpd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VEXPANDPS_MASK_VpsWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VEXPANDPD_MASK_VpdWpdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VEXPANDPS_MASK_VpsWpsM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VEXPANDPD_MASK_VpdWpdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPCOMPRESSB_MASK_WdqVdq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPCOMPRESSW_MASK_WdqVdq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPEXPANDB_MASK_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPEXPANDW_MASK_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPMOVQB_WdqVdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVDB_WdqVdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVWB_WdqVdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVDW_WdqVdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVQW_WdqVdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVQD_WdqVdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPMOVQB_MASK_WdqVdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVDB_MASK_WdqVdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVWB_MASK_WdqVdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVDW_MASK_WdqVdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVQW_MASK_WdqVdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVQD_MASK_WdqVdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPMOVQB_MASK_WdqVdqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVDB_MASK_WdqVdqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVWB_MASK_WdqVdqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVDW_MASK_WdqVdqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVQW_MASK_WdqVdqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVQD_MASK_WdqVdqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPMOVUSQB_WdqVdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVUSDB_WdqVdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVUSWB_WdqVdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVUSDW_WdqVdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVUSQW_WdqVdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVUSQD_WdqVdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPMOVUSQB_MASK_WdqVdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVUSDB_MASK_WdqVdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVUSWB_MASK_WdqVdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVUSDW_MASK_WdqVdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVUSQW_MASK_WdqVdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVUSQD_MASK_WdqVdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPMOVUSQB_MASK_WdqVdqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVUSDB_MASK_WdqVdqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVUSWB_MASK_WdqVdqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVUSDW_MASK_WdqVdqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVUSQW_MASK_WdqVdqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVUSQD_MASK_WdqVdqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPMOVSQB_WdqVdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVSDB_WdqVdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVSWB_WdqVdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVSDW_WdqVdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVSQW_WdqVdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVSQD_WdqVdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPMOVSQB_MASK_WdqVdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVSDB_MASK_WdqVdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVSWB_MASK_WdqVdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVSDW_MASK_WdqVdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVSQW_MASK_WdqVdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVSQD_MASK_WdqVdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPMOVSQB_MASK_WdqVdqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVSDB_MASK_WdqVdqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVSWB_MASK_WdqVdqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVSDW_MASK_WdqVdqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVSQW_MASK_WdqVdqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVSQD_MASK_WdqVdqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPMOVSXBW_MASK_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVSXBD_MASK_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVSXBQ_MASK_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVSXWD_MASK_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVSXWQ_MASK_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVSXDQ_MASK_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPMOVZXBW_MASK_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVZXBD_MASK_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVZXBQ_MASK_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVZXWD_MASK_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVZXWQ_MASK_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVZXDQ_MASK_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPCONFLICTD_MASK_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPCONFLICTQ_MASK_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPLZCNTD_MASK_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPLZCNTQ_MASK_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPOPCNTB_MASK_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPOPCNTW_MASK_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPOPCNTD_MASK_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPOPCNTQ_MASK_VdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPSHUFBITQMB_MASK_KGqHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VP2INTERSECTD_KGqHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VP2INTERSECTQ_KGqHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPBROADCASTMB2Q_VdqKEbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPBROADCASTMW2D_VdqKEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPMOVM2B_VdqKEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVM2W_VdqKEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVM2D_VdqKEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVM2Q_VdqKEbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPMOVB2M_KGqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVW2M_KGdWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVD2M_KGwWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMOVQ2M_KGbWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPMADD52LUQ_MASK_VdqHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMADD52HUQ_MASK_VdqHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPMULTISHIFTQB_VdqHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPMULTISHIFTQB_MASK_VdqHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPSHLDW_MASK_VdqHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPSHLDVW_MASK_VdqHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPSHLDD_MASK_VdqHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPSHLDVD_MASK_VdqHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPSHLDQ_MASK_VdqHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPSHLDVQ_MASK_VdqHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VPSHRDW_MASK_VdqHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPSHRDVW_MASK_VdqHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPSHRDD_MASK_VdqHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPSHRDVD_MASK_VdqHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPSHRDQ_MASK_VdqHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VPSHRDVQ_MASK_VdqHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VFPCLASSPH_MASK_KGdWphIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFPCLASSSH_MASK_KGbWshIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VCOMISH_VshWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VCMPPH_MASK_KGdHphWphIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCMPSH_MASK_KGbHshWshIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VSQRTSH_VshHphWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VSQRTSH_MASK_VshHphWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VGETEXPSH_VshHphWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VGETEXPSH_MASK_VshHphWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VGETMANTPH_MASK_VphWphIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VGETMANTSH_MASK_VshHphWshIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VREDUCEPH_MASK_VphWphIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VREDUCESH_MASK_VshHphWshIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VRNDSCALEPH_MASK_VphWphIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VRNDSCALESH_MASK_VshHphWshIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VRCPPH_MASK_VphWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VRCPSH_MASK_VshHphWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VRSQRTPH_MASK_VphWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VRSQRTSH_MASK_VshHphWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VADDSH_VshHphWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VADDSH_MASK_VshHphWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VSUBSH_VshHphWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VSUBSH_MASK_VshHphWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VDIVSH_VshHphWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VDIVSH_MASK_VshHphWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMULSH_VshHphWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMULSH_MASK_VshHphWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMINSH_VshHphWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMINSH_MASK_VshHphWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMAXSH_VshHphWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMAXSH_MASK_VshHphWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VSCALEFSH_VshHphWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VSCALEFSH_MASK_VshHphWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VFMADDSH_VphHshWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFMADDSH_MASK_VphHshWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFMSUBSH_VphHshWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFMSUBSH_MASK_VphHshWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFNMADDSH_VphHshWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFNMADDSH_MASK_VphHshWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFNMSUBSH_VphHshWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFNMSUBSH_MASK_VphHshWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VCVTPH2UW_VdqWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTPH2W_VdqWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPH2UW_VdqWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPH2W_VdqWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTUW2PH_VphWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTW2PH_VphWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VCVTPH2UW_MASK_VdqWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTPH2W_MASK_VdqWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPH2UW_MASK_VdqWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPH2W_MASK_VdqWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTUW2PH_MASK_VphWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTW2PH_MASK_VphWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VCVTPH2PSX_VpsWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPH2DQ_VdqWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPH2UDQ_VdqWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPH2QQ_VdqWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPH2UQQ_VdqWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTPH2DQ_VdqWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTPH2UDQ_VdqWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTPH2QQ_VdqWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTPH2UQQ_VdqWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTPH2PD_VpdWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VCVTPH2PSX_MASK_VpsWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPH2DQ_MASK_VdqWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPH2UDQ_MASK_VdqWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPH2QQ_MASK_VdqWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPH2UQQ_MASK_VdqWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTPH2DQ_MASK_VdqWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTPH2UDQ_MASK_VdqWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTPH2QQ_MASK_VdqWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTPH2UQQ_MASK_VdqWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTPH2PD_MASK_VpdWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VCVTPS2PHX_VphWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTPS2PHX_MASK_VphWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTDQ2PH_VphWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTUDQ2PH_VphWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTDQ2PH_MASK_VphWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTUDQ2PH_MASK_VphWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VCVTPD2PH_VphWpdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTPD2PH_MASK_VphWpdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTQQ2PH_VphWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTUQQ2PH_VphWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTQQ2PH_MASK_VphWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTUQQ2PH_MASK_VphWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VCVTSD2SH_VshWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTSD2SH_MASK_VshWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTSS2SH_VshWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTSS2SH_MASK_VshWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTSH2SD_VsdWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTSH2SD_MASK_VsdWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTSH2SS_VssWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTSH2SS_MASK_VssWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VCVTSH2SI_GdWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTSH2SI_GqWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTSH2USI_GdWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTSH2USI_GqWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTSH2SI_GdWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTSH2SI_GqWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTSH2USI_GdWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTSH2USI_GqWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VCVTSI2SH_VshEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTSI2SH_VshEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTUSI2SH_VshEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTUSI2SH_VshEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VFCMULCSH_MASK_VshHphWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFCMULCPH_MASK_VphHphWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFCMADDCSH_MASK_VshHphWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VFCMADDCPH_MASK_VphHphWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#endif

#if BX_SUPPORT_AMX
    BX_SMF void LDTILECFG(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void STTILECFG(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void TILELOADD_TnnnMdq(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void TILESTORED_MdqTnnn(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void TDPBSSD_TnnnTrmTreg(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void TDPBSUD_TnnnTrmTreg(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void TDPBUSD_TnnnTrmTreg(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void TDPBUUD_TnnnTrmTreg(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void TDPBF16PS_TnnnTrmTreg(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void TDPFP16PS_TnnnTrmTreg(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void TCMMRLFP16PS_TnnnTrmTreg(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void TCMMIMFP16PS_TnnnTrmTreg(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void TMMULTF32PS_TnnnTrmTreg(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void TILEZERO_Tnnn(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void TILERELEASE(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void TILEMOVROW_VdqTrm(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void TCVTROWD2PS_VpsTrm(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void TCVTROWPS2PHL_VphTrm(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void TCVTROWPS2PHH_VphTrm(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void TCVTROWPS2BF16L_VphTrm(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void TCVTROWPS2BF16H_VphTrm(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#endif

#if BX_SUPPORT_AVX
    // AVX10.2 - VCOMX
    BX_SMF void VCOMXSS_VssWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCOMXSD_VsdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCOMXSH_VshWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VDPPHPS_MASK_VpsHdqWdqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMPSADBW_MASK_VdqHdqWdqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    // AVX10.2 - BF16
    BX_SMF void VFPCLASSPBF16_MASK_KGdWphIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCMPPBF16_MASK_KGdHphWphIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCOMISBF16_VshWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VGETMANTPBF16_MASK_VphWphIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VREDUCEBF16_MASK_VphWphIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VRNDSCALEBF16_MASK_VphWphIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VRSQRTPBF16_VphWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VRCPPBF16_VphWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VRSQRTPBF16_MASK_VphWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VRCPPBF16_MASK_VphWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VMINMAXPD_MASK_VpdHpdWpdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMINMAXSD_MASK_VsdHpdWsdIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMINMAXPS_MASK_VpsHpsWpsIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMINMAXSS_MASK_VssHpsWssIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMINMAXPH_MASK_VphHphWphIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMINMAXSH_MASK_VshHphWshIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMINMAXBF16_MASK_VphHphWphIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    // AVX 10.2 - convert instructions
    BX_SMF void VCVT2PS2PHX_MASK_VphHpsWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    // AVX 10.2 - saturating convert to integer with truncation
    BX_SMF void VCVTTPD2UDQS_VdqWpdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPD2UDQS_MASK_VdqWpdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPD2UQQS_VdqWpdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPD2UQQS_MASK_VdqWpdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPS2UDQS_VdqWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPS2UDQS_MASK_VdqWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPS2UQQS_VdqWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPS2UQQS_MASK_VdqWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPD2DQS_VdqWpdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPD2DQS_MASK_VdqWpdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPD2QQS_VdqWpdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPD2QQS_MASK_VdqWpdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPS2DQS_VdqWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPS2DQS_MASK_VdqWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPS2QQS_VdqWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPS2QQS_MASK_VdqWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VCVTTSS2USIS_GdWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTSS2USIS_GqWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTSD2USIS_GdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTSD2USIS_GqWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTSD2SIS_GdWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTSD2SIS_GqWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTSS2SIS_GdWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTSS2SIS_GqWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    // AVX10.2 zero-extending partial vector register copy
    BX_SMF void VMOVW_VshWshR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VMOVD_VdWdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    // AVX10.2 fp8 convert instructions
    BX_SMF void VCVTHF82PH_VphWf8R(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTPH2BF8_Vf8HdqWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVT2PH2BF8_Vf8HdqWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTBIASPH2BF8_Vf8HdqWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTPH2HF8_Vf8HdqWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVT2PH2HF8_Vf8HdqWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTBIASPH2HF8_Vf8HdqWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    // AVX10.2 convert to int8 with saturation
    BX_SMF void VCVTBF162IBS_V8bWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTBF162IUBS_V8bWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTBF162IBS_MASK_V8bWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTBF162IUBS_MASK_V8bWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTBF162IBS_V8bWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTBF162IUBS_V8bWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTBF162IBS_MASK_V8bWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTBF162IUBS_MASK_V8bWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VCVTPH2IBS_V8bWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTPH2IUBS_V8bWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTPH2IBS_MASK_V8bWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTPH2IUBS_MASK_V8bWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPH2IBS_V8bWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPH2IUBS_V8bWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPH2IBS_MASK_V8bWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPH2IUBS_MASK_V8bWphR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void VCVTPS2IBS_V8bWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTPS2IUBS_V8bWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTPS2IBS_MASK_V8bWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTPS2IUBS_MASK_V8bWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPS2IBS_V8bWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPS2IUBS_V8bWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPS2IBS_MASK_V8bWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void VCVTTPS2IUBS_MASK_V8bWpsR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#endif

    BX_SMF void LZCNT_GwEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LZCNT_GdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#if BX_SUPPORT_X86_64
    BX_SMF void LZCNT_GqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#endif

    /* BMI - TZCNT */
    BX_SMF void TZCNT_GwEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void TZCNT_GdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#if BX_SUPPORT_X86_64
    BX_SMF void TZCNT_GqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#endif
    /* BMI - TZCNT */

    /* SSE4A */
    BX_SMF void EXTRQ_UdqIbIb(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void EXTRQ_VdqUq(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void INSERTQ_VdqUqIbIb(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void INSERTQ_VdqUdq(bxInstruction_c* i) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    /* SSE4A */

    BX_SMF void CMPXCHG8B(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RETnear32_Iw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RETnear16_Iw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RETfar32_Iw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RETfar16_Iw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void XADD_EbGbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void XADD_EwGwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void XADD_EdGdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void XADD_EbGbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void XADD_EwGwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void XADD_EdGdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void CMOVO_GwEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVNO_GwEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVB_GwEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVNB_GwEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVZ_GwEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVNZ_GwEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVBE_GwEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVNBE_GwEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVS_GwEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVNS_GwEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVP_GwEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVNP_GwEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVL_GwEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVNL_GwEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVLE_GwEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVNLE_GwEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void CMOVO_GdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVNO_GdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVB_GdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVNB_GdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVZ_GdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVNZ_GdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVBE_GdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVNBE_GdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVS_GdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVNS_GdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVP_GdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVNP_GdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVL_GdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVNL_GdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVLE_GdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVNLE_GdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void CWDE(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CDQ(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void CMPXCHG_EbGbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPXCHG_EwGwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPXCHG_EdGdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void CMPXCHG_EbGbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPXCHG_EwGwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPXCHG_EdGdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void MUL_AXEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void IMUL_AXEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void DIV_AXEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void IDIV_AXEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void IMUL_GwEwIwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void IMUL_GwEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void NOP(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PAUSE(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_EbIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1);
    BX_SMF void MOV_EwIwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_EdIdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void PUSH_EwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PUSH_EwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PUSH_EdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PUSH_EdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void POP_EwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void POP_EwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void POP_EdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void POP_EdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void POPCNT_GwEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void POPCNT_GdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#if BX_SUPPORT_X86_64
    BX_SMF void POPCNT_GqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#endif

    BX_SMF void ADCX_GdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ADOX_GdEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#if BX_SUPPORT_X86_64
    BX_SMF void ADCX_GqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ADOX_GqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#endif

    // SMAP
    BX_SMF void CLAC(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void STAC(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    // SMAP

    // RDRAND/RDSEED
    BX_SMF void RDRAND_Ew(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RDRAND_Ed(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#if BX_SUPPORT_X86_64
    BX_SMF void RDRAND_Eq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#endif

    BX_SMF void RDSEED_Ew(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RDSEED_Ed(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#if BX_SUPPORT_X86_64
    BX_SMF void RDSEED_Eq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#endif

#if BX_SUPPORT_X86_64
    // 64 bit extensions
    BX_SMF void ADD_GqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void OR_GqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ADC_GqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SBB_GqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void AND_GqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SUB_GqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void XOR_GqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMP_GqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void ADD_GqEqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void OR_GqEqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ADC_GqEqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SBB_GqEqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void AND_GqEqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SUB_GqEqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void XOR_GqEqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMP_GqEqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void ADD_EqGqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void OR_EqGqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ADC_EqGqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SBB_EqGqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void AND_EqGqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SUB_EqGqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void XOR_EqGqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMP_EqGqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void ADD_EqIdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void OR_EqIdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ADC_EqIdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SBB_EqIdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void AND_EqIdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SUB_EqIdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void XOR_EqIdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMP_EqIdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void ADD_EqIdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void OR_EqIdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ADC_EqIdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SBB_EqIdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void AND_EqIdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SUB_EqIdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void XOR_EqIdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMP_EqIdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void TEST_EqGqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void TEST_EqGqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void TEST_RAXId(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void XCHG_EqGqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void XCHG_EqGqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void LEA_GqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void MOV_RAXOq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_OqRAX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_EAXOq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_OqEAX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_AXOq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_OqAX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_ALOq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_OqAL(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void MOV_EqGqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_GqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_GqEqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_EqIdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_EqIdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void MOV64S_EqGqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV64S_GqEqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    // repeatable instructions
    BX_SMF void REP_MOVSQ_YqXq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void REP_CMPSQ_XqYq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void REP_STOSQ_YqRAX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void REP_LODSQ_RAXXq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void REP_SCASQ_RAXYq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    // qualified by address size
    BX_SMF void CMPSB64_XbYb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPSW64_XwYw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPSD64_XdYd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SCASB64_ALYb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SCASW64_AXYw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SCASD64_EAXYd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LODSB64_ALXb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LODSW64_AXXw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LODSD64_EAXXd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void STOSB64_YbAL(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void STOSW64_YwAX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void STOSD64_YdEAX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVSB64_YbXb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVSW64_YwXw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVSD64_YdXd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void CMPSQ32_XqYq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPSQ64_XqYq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SCASQ32_RAXYq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SCASQ64_RAXYq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LODSQ32_RAXXq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LODSQ64_RAXXq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void STOSQ32_YqRAX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void STOSQ64_YqRAX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVSQ32_YqXq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVSQ64_YqXq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void INSB64_YbDX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void INSW64_YwDX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void INSD64_YdDX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void OUTSB64_DXXb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void OUTSW64_DXXw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void OUTSD64_DXXd(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void CALL_Jq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JMP_Jq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void JO_Jq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JNO_Jq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JB_Jq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JNB_Jq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JZ_Jq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JNZ_Jq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JBE_Jq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JNBE_Jq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JS_Jq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JNS_Jq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JP_Jq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JNP_Jq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JL_Jq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JNL_Jq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JLE_Jq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JNLE_Jq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void ENTER64_IwIb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LEAVE64(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void IRET64(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void MOV_CR0Rq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_CR2Rq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_CR3Rq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_CR4Rq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_RqCR0(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_RqCR2(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_RqCR3(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_RqCR4(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_DqRq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV_RqDq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void SHLD_EqGqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SHLD_EqGqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SHRD_EqGqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SHRD_EqGqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void MOV64_GdEdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOV64_EdGdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void MOVZX_GqEbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVZX_GqEwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVSX_GqEbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVSX_GqEwM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVSX_GqEdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void MOVZX_GqEbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVZX_GqEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVSX_GqEbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVSX_GqEwR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVSX_GqEdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void BSF_GqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BSR_GqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void BT_EqGqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BTS_EqGqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BTR_EqGqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BTC_EqGqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void BT_EqGqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BTS_EqGqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BTR_EqGqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BTC_EqGqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void BT_EqIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BTS_EqIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BTR_EqIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BTC_EqIbM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void BT_EqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BTS_EqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BTR_EqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BTC_EqIbR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void BSWAP_RRX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void ROL_EqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ROR_EqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RCL_EqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RCR_EqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SHL_EqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SHR_EqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SAR_EqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void ROL_EqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void ROR_EqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RCL_EqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RCR_EqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SHL_EqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SHR_EqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SAR_EqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void NOT_EqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void NEG_EqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void NOT_EqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void NEG_EqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void TEST_EqIdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void TEST_EqIdM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void MUL_RAXEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void IMUL_RAXEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void DIV_RAXEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void IDIV_RAXEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void IMUL_GqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void IMUL_GqEqIdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void INC_EqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void DEC_EqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void INC_EqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void DEC_EqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CALL_EqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CALL64_Ep(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JMP_EqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JMP64_Ep(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PUSHF_Fq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void POPF_Fq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void CMPXCHG_EqGqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMPXCHG_EqGqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void CDQE(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CQO(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void XADD_EqGqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void XADD_EqGqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void RETnear64_Iw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RETfar64_Iw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void CMOVO_GqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVNO_GqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVB_GqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVNB_GqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVZ_GqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVNZ_GqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVBE_GqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVNBE_GqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVS_GqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVNS_GqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVP_GqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVNP_GqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVL_GqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVNL_GqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVLE_GqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CMOVNLE_GqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void MOV_RRXIq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PUSH_EqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PUSH_EqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void POP_EqM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void POP_EqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void PUSH64_Id(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void PUSH64_Sw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void POP64_Sw(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void LSS_GqMp(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LFS_GqMp(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LGS_GqMp(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void SGDT64_Ms(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SIDT64_Ms(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LGDT64_Ms(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LIDT64_Ms(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void CMPXCHG16B(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void SWAPGS(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RDFSBASE_Ed(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RDGSBASE_Ed(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RDFSBASE_Eq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RDGSBASE_Eq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void WRFSBASE_Ed(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void WRGSBASE_Ed(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void WRFSBASE_Eq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void WRGSBASE_Eq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void LOOPNE64_Jb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOOPE64_Jb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void LOOP64_Jb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void JRCXZ_Jb(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void MOVQ_EqPqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVQ_EqVqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVQ_PqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MOVQ_VdqEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void CVTSI2SS_VssEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CVTSI2SD_VsdEqR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CVTTSD2SI_GqWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CVTTSS2SI_GqWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CVTSD2SI_GqWsdR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CVTSS2SI_GqWssR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#endif  // #if BX_SUPPORT_X86_64

    BX_SMF void RDTSCP(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void INVLPG(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RSM(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void WRMSR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RDTSC(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RDPMC(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RDMSR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SYSENTER(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SYSEXIT(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void MONITOR(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void MWAIT(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void UMONITOR_Eq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void UMWAIT_Ed(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

#if BX_SUPPORT_X86_64
    BX_SMF void WRMSRLIST(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void RDMSRLIST(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#endif

#if BX_SUPPORT_PKEYS
    BX_SMF void RDPKRU(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void WRPKRU(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#endif

#if BX_SUPPORT_UINTR
    BX_SMF void STUI(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void CLUI(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void TESTUI(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void UIRET(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void SENDUIPI_Gq(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#endif

    BX_SMF void RDPID_Ed(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }

    BX_SMF void UndefinedOpcode(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
    BX_SMF void BxError(bxInstruction_c*) BX_CPP_AttrRegparmN(1);
#if BX_SUPPORT_HANDLERS_CHAINING_SPEEDUPS
    BX_SMF void BxEndTrace(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#endif

    BX_SMF void BxNoFPU(bxInstruction_c*) BX_CPP_AttrRegparmN(1);
    BX_SMF void BxNoMMX(bxInstruction_c*) BX_CPP_AttrRegparmN(1);
#if BX_CPU_LEVEL >= 6
    BX_SMF void BxNoSSE(bxInstruction_c*) BX_CPP_AttrRegparmN(1);
#if BX_SUPPORT_AVX
    BX_SMF void BxNoAVX(bxInstruction_c*) BX_CPP_AttrRegparmN(1); 
#endif
#if BX_SUPPORT_EVEX
    BX_SMF void BxNoOpMask(bxInstruction_c*) BX_CPP_AttrRegparmN(1);
    BX_SMF void BxNoEVEX(bxInstruction_c*) BX_CPP_AttrRegparmN(1);
#endif
#if BX_SUPPORT_AMX
    BX_SMF void BxNoAMX(bxInstruction_c*) BX_CPP_AttrRegparmN(1) { gao_no(__LINE__, __func__); }
#endif
#endif

    BX_CPP_INLINE BX_SMF Bit32u BxResolve32(bxInstruction_c*) BX_CPP_AttrRegparmN(1);
#if BX_SUPPORT_X86_64
    BX_CPP_INLINE BX_SMF Bit64u BxResolve64(bxInstruction_c*) BX_CPP_AttrRegparmN(1);
#endif
#if BX_SUPPORT_AVX
    BX_SMF bx_address BxResolveGatherD(bxInstruction_c*, unsigned) BX_CPP_AttrRegparmN(2) { gao_no(__LINE__, __func__); return 0; }
    BX_SMF bx_address BxResolveGatherQ(bxInstruction_c*, unsigned) BX_CPP_AttrRegparmN(2) { gao_no(__LINE__, __func__); return 0; }
#endif
    //4442 <TAG-CLASS-CPU-END>
#endif

    BX_SMF void boundaryFetch(const Bit8u* fetchPtr, unsigned remainingInPage, bxInstruction_c*);//4488

    BX_SMF bxICacheEntry_c* serveICacheMiss(Bit32u eipBiased, bx_phy_address pAddr);//4490行
	BX_SMF bool mergeTraces(bxICacheEntry_c* entry, bxInstruction_c* i, bx_phy_address pAddr);//4492行
	BX_SMF void prefetch(void);//4496行
    BX_SMF BX_CPP_INLINE void invalidate_prefetch_q(void);//4498行
    BX_SMF BX_CPP_INLINE void invalidate_stack_cache(void);//4503
	BX_SMF bxICacheEntry_c* getICacheEntry(void);
    BX_SMF bx_hostpageaddr_t getHostMemAddr(bx_phy_address addr, unsigned rw);//4716行
    BX_SMF bx_phy_address translate_linear(bx_TLB_entry* entry, bx_address laddr, unsigned user, unsigned rw);//4719行
    BX_SMF void real_mode_int(Bit8u vector, bool push_error, Bit16u error_code); //4764
    BX_SMF bool exception_push_error(unsigned vector);//4769
    BX_SMF int  get_exception_type(unsigned vector);//4770
    BX_SMF void exception(unsigned vector, Bit16u error_code)//4771
        BX_CPP_AttrNoReturn();
    BX_SMF void init_SMRAM(void);//4773
    BX_SMF void reset(unsigned source); //4804
    BX_SMF void jump_protected(bxInstruction_c* i, Bit16u cs, bx_address disp) BX_CPP_AttrRegparmN(3);//4872
    BX_SMF void    load_seg_reg(bx_segment_reg_t* seg, Bit16u new_value) BX_CPP_AttrRegparmN(2); //4930
    BX_SMF void jmp_far32(bxInstruction_c* i, Bit16u cs_raw, Bit32u disp32);
    BX_SMF void init_FetchDecodeTables(void); //4985
    BX_SMF int  assignHandler(bxInstruction_c* i, Bit32u fetchModeMask); //4986
    BX_SMF BX_CPP_INLINE bool is_cpu_extension_supported(unsigned extension) {
        assert(extension < BX_ISA_EXTENSION_LAST);
        return BX_CPU_THIS_PTR ia_extensions_bitmask[extension / 32] & (1 << (extension % 32));
    }
	BX_SMF Bit32u get_laddr32(unsigned seg, Bit32u offset);//5049行

    DECLARE_EFLAG_ACCESSOR(RF, 16) //5070行
    BX_SMF BX_CPP_INLINE bool protected_mode(void);//5079
    BX_SMF BX_CPP_INLINE bool long_mode(void);//5081

#if BX_CPU_LEVEL >= 6    //5129
    BX_SMF void xsave_xrestor_init(void); //5130
    BX_SMF bool xsave_x87_state_xinuse(void); //5135
    BX_SMF void xsave_x87_state(bxInstruction_c* i, bx_address offset);
    BX_SMF void xrstor_x87_state(bxInstruction_c* i, bx_address offset);
    BX_SMF void xrstor_init_x87_state(void);

    BX_SMF bool xsave_sse_state_xinuse(void);//5140
    BX_SMF void xsave_sse_state(bxInstruction_c* i, bx_address offset);
    BX_SMF void xrstor_sse_state(bxInstruction_c* i, bx_address offset);
    BX_SMF void xrstor_init_sse_state(void);
#if BX_SUPPORT_AVX
    BX_SMF bool xsave_ymm_state_xinuse(void);
    BX_SMF void xsave_ymm_state(bxInstruction_c* i, bx_address offset);
    BX_SMF void xrstor_ymm_state(bxInstruction_c* i, bx_address offset);
    BX_SMF void xrstor_init_ymm_state(void);
#if BX_SUPPORT_EVEX
    BX_SMF bool xsave_opmask_state_xinuse(void);
    BX_SMF void xsave_opmask_state(bxInstruction_c* i, bx_address offset);
    BX_SMF void xrstor_opmask_state(bxInstruction_c* i, bx_address offset);
    BX_SMF void xrstor_init_opmask_state(void);

    BX_SMF bool xsave_zmm_hi256_state_xinuse(void);
    BX_SMF void xsave_zmm_hi256_state(bxInstruction_c* i, bx_address offset);
    BX_SMF void xrstor_zmm_hi256_state(bxInstruction_c* i, bx_address offset);
    BX_SMF void xrstor_init_zmm_hi256_state(void);

    BX_SMF bool xsave_hi_zmm_state_xinuse(void);
    BX_SMF void xsave_hi_zmm_state(bxInstruction_c* i, bx_address offset);
    BX_SMF void xrstor_hi_zmm_state(bxInstruction_c* i, bx_address offset);
    BX_SMF void xrstor_init_hi_zmm_state(void);

#endif//5165
#endif//5166


#if BX_SUPPORT_PKEYS
    BX_SMF bool xsave_pkru_state_xinuse(void);
    BX_SMF void xsave_pkru_state(bxInstruction_c* i, bx_address offset);
    BX_SMF void xrstor_pkru_state(bxInstruction_c* i, bx_address offset);
    BX_SMF void xrstor_init_pkru_state(void);
#endif

#if BX_SUPPORT_CET
    BX_SMF bool xsave_cet_u_state_xinuse(void);
    BX_SMF void xsave_cet_u_state(bxInstruction_c* i, bx_address offset);
    BX_SMF void xrstor_cet_u_state(bxInstruction_c* i, bx_address offset);
    BX_SMF void xrstor_init_cet_u_state(void);

    BX_SMF bool xsave_cet_s_state_xinuse(void);
    BX_SMF void xsave_cet_s_state(bxInstruction_c* i, bx_address offset);
    BX_SMF void xrstor_cet_s_state(bxInstruction_c* i, bx_address offset);
    BX_SMF void xrstor_init_cet_s_state(void);
#endif
#if BX_SUPPORT_UINTR
    BX_SMF bool xsave_uintr_state_xinuse(void);
    BX_SMF void xsave_uintr_state(bxInstruction_c* i, bx_address offset);
    BX_SMF void xrstor_uintr_state(bxInstruction_c* i, bx_address offset);
    BX_SMF void xrstor_init_uintr_state(void);
#endif

#if BX_SUPPORT_AMX
    BX_SMF bool xsave_tilecfg_state_xinuse(void);
    BX_SMF void xsave_tilecfg_state(bxInstruction_c* i, bx_address offset);
    BX_SMF void xrstor_tilecfg_state(bxInstruction_c* i, bx_address offset);
    BX_SMF void xrstor_init_tilecfg_state(void);

    BX_SMF bool xsave_tiledata_state_xinuse(void);
    BX_SMF void xsave_tiledata_state(bxInstruction_c* i, bx_address offset);
    BX_SMF void xrstor_tiledata_state(bxInstruction_c* i, bx_address offset);
    BX_SMF void xrstor_init_tiledata_state(void);
#endif
#endif //5205
    BX_SMF bool is_monitor(bx_phy_address addr, unsigned len);
    BX_SMF void check_monitor(bx_phy_address addr, unsigned len);

    BX_SMF void init_vmx_capabilities(void);//5260

#if BX_SUPPORT_VMX >= 2
    BX_SMF void init_ept_vpid_capabilities(void);
    BX_SMF void init_vmfunc_capabilities(void);
#endif
    BX_SMF void init_pin_based_vmexec_ctrls(void);
    BX_SMF void init_primary_proc_based_vmexec_ctrls(void);
    BX_SMF void init_secondary_proc_based_vmexec_ctrls(void);
    BX_SMF void init_tertiary_proc_based_vmexec_ctrls(void);
    BX_SMF void init_vmexit_ctrls(void);//5269
    BX_SMF void init_secondary_vmexit_ctrls(void);
    BX_SMF void init_vmentry_ctrls(void);//5271
    BX_SMF void init_VMCS(void);//5272
#if BX_CPU_LEVEL >= 5 //5369
    void init_MSRs();//5370
    void destroy_MSRs();
#if BX_CONFIGURE_MSRS
    int load_MSRs(const char* file);
#endif
#endif//5375
};//5376

BX_CPP_INLINE Bit32u BX_CPU_C::get_laddr32(unsigned seg, Bit32u offset) //5524行
{
	return (Bit32u)BX_CPU_THIS_PTR sregs[seg].cache.u.segment.base + offset;
}

BX_CPP_INLINE void BX_CPU_C::invalidate_prefetch_q(void) //4498行
{
    BX_CPU_THIS_PTR eipPageWindowSize = 0;
}
BX_CPP_INLINE void BX_CPU_C::invalidate_stack_cache(void)
{
    //4503
    BX_CPU_THIS_PTR espPageWindowSize = 0;
}

enum {
    BX_FETCH_MODE_FPU_MMX_OK = (1 << 2),
    BX_FETCH_MODE_SSE_OK = (1 << 3),
    BX_FETCH_MODE_AVX_OK = (1 << 4),
    BX_FETCH_MODE_OPMASK_OK = (1 << 5),
    BX_FETCH_MODE_EVEX_OK = (1 << 6),
};

/*BX_CPP_INLINE bool BX_CPU_C::real_mode(void)
{
    //5780
    return (BX_CPU_THIS_PTR cpu_mode == BX_MODE_IA32_REAL);
}*/

class bxInstruction_c;//5785行
BX_CPP_INLINE bool BX_CPU_C::protected_mode(void)
{
    //5797
    return (BX_CPU_THIS_PTR cpu_mode >= BX_MODE_IA32_PROTECTED);
}
BX_CPP_INLINE bool BX_CPU_C::long_mode(void)
{
    //5800
#if BX_SUPPORT_X86_64
    return BX_CPU_THIS_PTR efer.get_LMA();
#else
    return 0;
#endif
}

IMPLEMENT_EFLAG_SET_ACCESSOR_RF(16) //5851行


#define BX_NEXT_TRACE(i) { return; } //5914
#define BX_NEXT_INSTR(i) { return; }
