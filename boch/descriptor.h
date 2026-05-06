#pragma once

typedef struct { /* bx_selector_t */ //41ÐÐ
	Bit16u value;   /* the 16bit value of the selector */
	/* the following fields are extracted from the value field in protected
	   mode only.  They're used for sake of efficiency */
	Bit16u index;   /* 13bit index extracted from value in protected mode */
	Bit8u  ti;      /* table indicator bit extracted from value */
	Bit8u  rpl;     /* RPL extracted from value */
} bx_selector_t;

typedef struct
{

#define SegValidCache  (0x01)
#define SegAccessROK   (0x02)
#define SegAccessWOK   (0x04)
#define SegAccessROK4G (0x08)
#define SegAccessWOK4G (0x10)
    unsigned valid;        // Holds above values, Or'd together. Used to
    // hold only 0 or 1 once.

    bool p;                /* present */
    Bit8u   dpl;           /* descriptor privilege level 0..3 */
    bool segment;          /* 0 = system/gate, 1 = data/code segment */
    Bit8u   type;          /* For system & gate descriptors:
                            *  0 = invalid descriptor (reserved)
                            *  1 = 286 available Task State Segment (TSS)
                            *  2 = LDT descriptor
                            *  3 = 286 busy Task State Segment (TSS)
                            *  4 = 286 call gate
                            *  5 = task gate
                            *  6 = 286 interrupt gate
                            *  7 = 286 trap gate
                            *  8 = (reserved)
                            *  9 = 386 available TSS
                            * 10 = (reserved)
                            * 11 = 386 busy TSS
                            * 12 = 386 call gate
                            * 13 = (reserved)
                            * 14 = 386 interrupt gate
                            * 15 = 386 trap gate */

    union {
        struct {
            bx_address base;       /* base address: 286=24bits, 386=32bits, long=64 */
            Bit32u  limit_scaled;  /* for efficiency, this contrived field is set to
                                    * limit for byte granular, and
                                    * (limit << 12) | 0xfff for page granular seg's
                                    */
            bool g;                 /* granularity: 0=byte, 1=4K (page) */
            bool d_b;               /* default size: 0=16bit, 1=32bit */
#if BX_SUPPORT_X86_64
            bool l;                 /* long mode: 0=compat, 1=64 bit */
#endif
            bool avl;               /* available for use by system */
        } segment;
        struct {
            Bit8u   param_count;   /* 5bits (0..31) #words/dword to copy from caller's
                                    * stack to called procedure's stack. */
            Bit16u  dest_selector;
            Bit32u  dest_offset;
        } gate;
        struct {                 /* type 5: Task Gate Descriptor */
            Bit16u  tss_selector;  /* TSS segment selector */
        } taskgate;
    } u;

} bx_descriptor_t;

enum {
    BX_DATA_READ_WRITE_ACCESSED = 0x3,
};

typedef struct { //180ÐÐ
	bx_selector_t    selector;
	bx_descriptor_t  cache;
} bx_segment_reg_t;