#pragma once

#include "decoder.h" //27行

#  define BX_SMF           static

#define EIP (BX_CPU_THIS_PTR gen_reg[BX_32BIT_REG_EIP].dword.erx) //82行

#define RIP (BX_CPU_THIS_PTR gen_reg[BX_64BIT_REG_RIP].rrx)  //107行


#define BX_CLEAR_64BIT_HIGH(index) {\
  BX_CPU_THIS_PTR gen_reg[index].dword.hrx = 0; \
} //174-176行 

#define USER_PL   (BX_CPU_THIS_PTR user_pl)   //200行

class BX_CPU_C;//391行
class bxInstruction_c;//393行
struct bxICacheEntry_c;

#  define BX_CPU_THIS_PTR  BX_CPU(0)->    //421行
#  define BX_CPU_THIS      BX_CPU(0)


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

#include "descriptor.h"//693行
#include "instr.h" //694行
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

class bx_cpuid_t; //887行

class BOCHSAPI BX_CPU_C{ //889行
public:

    unsigned bx_cpuid;//892行
    #if BX_CPU_LEVEL >= 4
        bx_cpuid_t* cpuid;
    #endif

	BX_CPU_C(unsigned id = 0);
	~BX_CPU_C();
    bx_gen_reg_t gen_reg[BX_GENERAL_REGISTERS + 4]; //930行

    Bit32u eflags;//940行

    bx_address prev_rip; //948行

	/* user segment register set */
	bx_segment_reg_t  sregs[6]; //970行

    #define BX_EVENT_CODE_BREAKPOINT_ASSIST       (1 <<  3) //1167行

    Bit32u  pending_event;//1180行


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
    #define BX_DTLB_SIZE 2048
    #define BX_ITLB_SIZE 1024
        TLB<BX_DTLB_SIZE> DTLB BX_CPP_AlignN(32);
        TLB<BX_ITLB_SIZE> ITLB BX_CPP_AlignN(32);

    bxICache_c iCache BX_CPP_AlignN(32);//1336行
    Bit32u fetchModeMask;//1337行

	void initialize(void);
	BX_SMF void cpu_loop(void);
    BX_SMF bxICacheEntry_c* serveICacheMiss(Bit32u eipBiased, bx_phy_address pAddr);//4490行
	BX_SMF void prefetch(void);//4496行
    BX_SMF BX_CPP_INLINE void invalidate_prefetch_q(void);//4498行
	BX_SMF bxICacheEntry_c* getICacheEntry(void);
    BX_SMF bx_hostpageaddr_t getHostMemAddr(bx_phy_address addr, unsigned rw);//4716行
    BX_SMF bx_phy_address translate_linear(bx_TLB_entry* entry, bx_address laddr, unsigned user, unsigned rw);//4719行
	BX_SMF Bit32u get_laddr32(unsigned seg, Bit32u offset);//5049行

    DECLARE_EFLAG_ACCESSOR(RF, 16) //5070行
};

BX_CPP_INLINE Bit32u BX_CPU_C::get_laddr32(unsigned seg, Bit32u offset) //5524行
{
	return (Bit32u)BX_CPU_THIS_PTR sregs[seg].cache.u.segment.base + offset;
}

BX_SMF BX_CPP_INLINE void invalidate_prefetch_q(void) //4498行
{
    BX_CPU_THIS_PTR eipPageWindowSize = 0;
}

//5785行
class bxInstruction_c;

IMPLEMENT_EFLAG_SET_ACCESSOR_RF(16) //5851行